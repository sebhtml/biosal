
#include "worker_pool.h"

#include "actor.h"
#include "worker.h"
#include "node.h"
#include "migration.h"

#include <core/helpers/vector_helper.h>
#include <core/helpers/statistics.h>
#include <core/helpers/pair.h>

#include <core/structures/set_iterator.h>
#include <core/structures/vector_iterator.h>

#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>


/*
#define BSAL_WORKER_POOL_DEBUG
#define BSAL_WORKER_POOL_DEBUG_ISSUE_334
#define BSAL_WORKER_POOL_USE_CURRENT_WORKER
*/

/*
 * Scheduling options.
 */
#define BSAL_WORKER_POOL_PUSH_WORK_ON_SAME_WORKER
#define BSAL_WORKER_POOL_FORCE_LAST_WORKER 1
#define BSAL_WORKER_POOL_USE_LEAST_BUSY
#define BSAL_WORKER_POOL_WORK_SCHEDULING_WINDOW 8192

/*
*/
#define BSAL_WORKER_POOL_BALANCE
#define BSAL_BALANCER_REDUCTIONS_PER_WORKER 1024

/*
 * Definitions for scheduling classes
 * See bsal_worker_pool_balance for classification requirements.
 */

#define BSAL_CLASS_STALLED_STRING "BSAL_CLASS_STALLED"
#define BSAL_CLASS_OPERATING_AT_FULL_CAPACITY_STRING "BSAL_CLASS_OPERATING_AT_FULL_CAPACITY"
#define BSAL_CLASS_NORMAL_STRING "BSAL_CLASS_NORMAL"
#define BSAL_CLASS_HUB_STRING "BSAL_CLASS_HUB"
#define BSAL_CLASS_BURDENED_STRING "BSAL_CLASS_BURDENED"

void bsal_worker_pool_init(struct bsal_worker_pool *pool, int workers,
                struct bsal_node *node)
{
    int i;
    struct bsal_set *set;

    pool->debug_mode = 0;
    pool->node = node;

#if 0
    pool->ticks_without_messages = 0;
#endif

    pool->last_warning = 0;
    pool->last_scheduling_warning = 0;

    pool->workers = workers;

    /* with only one thread,  the main thread
     * handles everything.
     */
    if (pool->workers >= 1) {
        pool->worker_for_run = 0;
        pool->worker_for_message = 0;
        pool->worker_for_work = 0;
    } else {
        printf("Error: the number of workers must be at least 1.\n");
        exit(1);
    }

#ifdef BSAL_WORKER_POOL_HAS_SPECIAL_QUEUES
    bsal_work_queue_init(&pool->work_queue);
    bsal_message_queue_init(&pool->message_queue);
#endif

    bsal_worker_pool_create_workers(pool);

    pool->starting_time = time(NULL);

    bsal_ring_queue_init(&pool->scheduled_actor_queue_buffer, sizeof(struct bsal_actor *));
    bsal_ring_queue_init(&pool->inbound_message_queue_buffer, sizeof(struct bsal_message));

    bsal_map_init(&pool->actor_affinities, sizeof(int), sizeof(int));
    bsal_map_init(&pool->last_actor_received_messages, sizeof(int), sizeof(int));
    bsal_vector_init(&pool->worker_actors, sizeof(struct bsal_set));

    bsal_vector_resize(&pool->worker_actors, pool->workers);

    for (i = 0; i < pool->workers; i++) {

        set = (struct bsal_set *)bsal_vector_at(&pool->worker_actors, i);

        bsal_set_init(set, sizeof(int));
    }

    pool->received_works = 0;

    pool->balance_period = pool->workers * BSAL_BALANCER_REDUCTIONS_PER_WORKER;
}

void bsal_worker_pool_destroy(struct bsal_worker_pool *pool)
{
    int i;
    struct bsal_set *set;

    bsal_worker_pool_print_efficiency(pool);

    for (i = 0; i < pool->workers; i++) {
        set= (struct bsal_set *)bsal_vector_at(&pool->worker_actors, i);

        bsal_set_destroy(set);
    }

    bsal_worker_pool_delete_workers(pool);

    pool->node = NULL;

    bsal_ring_queue_destroy(&pool->inbound_message_queue_buffer);
    bsal_ring_queue_destroy(&pool->scheduled_actor_queue_buffer);

    bsal_map_destroy(&pool->actor_affinities);
    bsal_map_destroy(&pool->last_actor_received_messages);
    bsal_vector_destroy(&pool->worker_actors);
}

void bsal_worker_pool_delete_workers(struct bsal_worker_pool *pool)
{
    int i = 0;
    struct bsal_worker *worker;

    if (pool->workers <= 0) {
        return;
    }

    for (i = 0; i < pool->workers; i++) {
        worker = bsal_worker_pool_get_worker(pool, i);

#if 0
        printf("worker/%d loop_load %f\n", bsal_worker_name(worker),
                    bsal_worker_get_loop_load(worker));
#endif

        bsal_worker_destroy(worker);
    }

    bsal_vector_destroy(&pool->worker_array);
    bsal_vector_destroy(&pool->message_count_cache);
}

void bsal_worker_pool_create_workers(struct bsal_worker_pool *pool)
{
    int i;

    if (pool->workers <= 0) {
        return;
    }

    bsal_vector_init(&pool->worker_array, sizeof(struct bsal_worker));
    bsal_vector_init(&pool->message_count_cache, sizeof(int));

    bsal_vector_resize(&pool->worker_array, pool->workers);
    bsal_vector_resize(&pool->message_count_cache, pool->workers);

    pool->worker_cache = (struct bsal_worker *)bsal_vector_at(&pool->worker_array, 0);
    pool->message_cache = (int *)bsal_vector_at(&pool->message_count_cache, 0);

    for (i = 0; i < pool->workers; i++) {
        bsal_worker_init(bsal_worker_pool_get_worker(pool, i), i, pool->node);
        bsal_vector_helper_set_int(&pool->message_count_cache, i, 0);
    }
}

void bsal_worker_pool_start(struct bsal_worker_pool *pool)
{
    int i;
    int processor;

    /* start workers
     *
     * we start at 1 because the thread 0 is
     * used by the main thread...
     */
    for (i = 0; i < pool->workers; i++) {
        processor = i;

        if (bsal_node_nodes(pool->node) != 1) {
            processor = -1;
        }

        bsal_worker_start(bsal_worker_pool_get_worker(pool, i), processor);
    }
}

void bsal_worker_pool_run(struct bsal_worker_pool *pool)
{
    /* make the thread work (this is the main thread) */
    bsal_worker_run(bsal_worker_pool_select_worker_for_run(pool));
}

void bsal_worker_pool_stop(struct bsal_worker_pool *pool)
{
    int i;
    /*
     * stop workers
     */

#ifdef BSAL_WORKER_POOL_DEBUG
    printf("Stop workers\n");
#endif

    for (i = 0; i < pool->workers; i++) {
        bsal_worker_stop(bsal_worker_pool_get_worker(pool, i));
    }
}

#ifdef BSAL_WORKER_HAS_OWN_QUEUES
int bsal_worker_pool_select_worker_least_busy(
                struct bsal_worker_pool *self, struct bsal_message *message, int *worker_score)
{
    int to_check;
    int score;
    int best_score;
    struct bsal_worker *worker;
    struct bsal_worker *best_worker;
    int selected_worker;

#if 0
    int last_worker_score;
#endif

#ifdef BSAL_WORKER_DEBUG
    int tag;
    int destination;
    struct bsal_message *message;
#endif

    best_worker = NULL;
    best_score = 99;

    to_check = BSAL_WORKER_POOL_WORK_SCHEDULING_WINDOW;

    while (to_check--) {

        /*
         * get the worker to test for this iteration.
         */
        worker = bsal_worker_pool_get_worker(self, self->worker_for_work);

        score = bsal_worker_get_queued_messages(worker);

#ifdef BSAL_WORKER_POOL_DEBUG_ISSUE_334
        if (score >= BSAL_WORKER_WARNING_THRESHOLD
                        && (self->last_scheduling_warning == 0
                             || score >= self->last_scheduling_warning + BSAL_WORKER_WARNING_THRESHOLD_STRIDE)) {
            printf("Warning: node %d worker %d has a scheduling score of %d\n",
                            bsal_node_name(self->node),
                            self->worker_for_work, score);

            self->last_scheduling_warning = score;
        }
#endif

        /* if the worker is not busy and it has no work to do,
         * select it right away...
         */
        if (score == 0) {
            best_worker = worker;
            best_score = 0;
            break;
        }

        /* Otherwise, test the worker
         */
        if (best_worker == NULL || score < best_score) {
            best_worker = worker;
            best_score = score;
        }

        /*
         * assign the next worker
         */
        self->worker_for_work = bsal_worker_pool_next_worker(self, self->worker_for_work);
    }

#ifdef BSAL_WORKER_POOL_DEBUG
    message = bsal_work_message(work);
    tag = bsal_message_tag(message);
    destination = bsal_message_destination(message);

    if (tag == BSAL_ACTOR_ASK_TO_STOP) {
        printf("DEBUG dispatching BSAL_ACTOR_ASK_TO_STOP for actor %d to worker %d\n",
                        destination, *start);
    }


#endif

    selected_worker = self->worker_for_work;

    /*
     * assign the next worker
     */
    self->worker_for_work = bsal_worker_pool_next_worker(self, self->worker_for_work);

    *worker_score = best_score;
    /* This is a best effort algorithm
     */
    return selected_worker;
}

#endif

struct bsal_worker *bsal_worker_pool_select_worker_for_run(struct bsal_worker_pool *pool)
{
    int index;

    index = pool->worker_for_run;
    return bsal_worker_pool_get_worker(pool, index);
}

int bsal_worker_pool_enqueue_message(struct bsal_worker_pool *pool, struct bsal_message *message)
{
    struct bsal_message other_message;
    struct bsal_actor *actor;
    int worker_index;
    struct bsal_worker *worker;
    int name;
    int destination;

    destination = bsal_message_destination(message);

#if 0
    printf("DEBUG pool receives message for actor %d\n",
                    destination);
#endif

#ifdef BSAL_WORKER_POOL_BALANCE
    /* balance the pool regularly
     */
    if (pool->received_works % pool->balance_period == 0) {
        bsal_worker_pool_balance(pool);
    }
#endif

    pool->received_works++;

    name = destination;
    actor = bsal_node_get_actor_from_name(pool->node, name);

    bsal_worker_pool_give_message_to_actor(pool, message);

    /* If there are messages in the inbound message buffer,
     * Try to give  them too.
     */
    if (bsal_ring_queue_dequeue(&pool->inbound_message_queue_buffer, &other_message)) {
        bsal_worker_pool_give_message_to_actor(pool, &other_message);
    }

    /* Try to dequeue an actor for scheduling
     */

    if (bsal_ring_queue_dequeue(&pool->scheduled_actor_queue_buffer, &actor)) {

        name = bsal_actor_name(actor);
        bsal_map_get_value(&pool->actor_affinities, &name, &worker_index);

        worker = bsal_worker_pool_get_worker(pool, worker_index);

        if (!bsal_worker_enqueue_actor(worker, &actor)) {
            bsal_ring_queue_enqueue(&pool->scheduled_actor_queue_buffer, &actor);
        }
    }

    return 1;
}

int bsal_worker_pool_worker_count(struct bsal_worker_pool *pool)
{
    return pool->workers;
}

#if 0
int bsal_worker_pool_has_messages(struct bsal_worker_pool *pool)
{
    int threshold;

    threshold = 200000;

    if (pool->ticks_without_messages > threshold) {
        return 0;
    }

    return 1;
}
#endif

void bsal_worker_pool_print_load(struct bsal_worker_pool *self, int type)
{
    int count;
    int i;
    float epoch_load;
    struct bsal_worker *worker;
    float loop_load;
    /*
    int scheduling_score;
    */
    int node_name;
    char *buffer;
    int allocated;
    int offset;
    int extra;
    clock_t current_time;
    int elapsed;
    float selected_load;
    float sum;
    char loop[] = "LOOP";
    char epoch[] = "EPOCH";
    char *description;
    float efficiency;

    description = NULL;

    if (type == BSAL_WORKER_POOL_LOAD_LOOP) {
        description = loop;
    } else if (type == BSAL_WORKER_POOL_LOAD_EPOCH) {
        description = epoch;
    } else {
        return;
    }

    current_time = time(NULL);
    elapsed = current_time - self->starting_time;

    extra = 100;

    count = bsal_worker_pool_worker_count(self);
    allocated = count * 20 + 20 + extra;

    buffer = bsal_memory_allocate(allocated);
    node_name = bsal_node_name(self->node);
    offset = 0;
    i = 0;
    sum = 0;

    while (i < count && offset + extra < allocated) {

        worker = bsal_worker_pool_get_worker(self, i);
        epoch_load = bsal_worker_get_epoch_load(worker);
        loop_load = bsal_worker_get_loop_load(worker);

        selected_load = epoch_load;

        if (type == BSAL_WORKER_POOL_LOAD_EPOCH) {
            selected_load = epoch_load;
        } else if (type == BSAL_WORKER_POOL_LOAD_LOOP) {
            selected_load = loop_load;
        }

        /*
        offset += sprintf(buffer + offset, " [%d %d %.2f]", i,
                        scheduling_score,
                        selected_load);
                        */
        offset += sprintf(buffer + offset, " %.2f",
                        selected_load);

        sum += epoch_load;

        ++i;
    }

    efficiency = sum / count;

    printf("LOAD %s %d s node/%d %.2f/%d (%.2f)%s\n",
                    description, elapsed, node_name,
                    sum, count, efficiency, buffer);


    bsal_memory_free(buffer);
}

void bsal_worker_pool_toggle_debug_mode(struct bsal_worker_pool *self)
{
    self->debug_mode = !self->debug_mode;
}

#define SCHEDULER_PRECISION 1000000
#define SCHEDULER_WINDOW 10
void bsal_worker_pool_balance(struct bsal_worker_pool *pool)
{
    /*
     * The 95th percentile is useful:
     * \see http://en.wikipedia.org/wiki/Burstable_billing
     * \see http://www.init7.net/en/backbone/95-percent-rule
     */
    int load_percentile_50;

    int i;
    struct bsal_vector loads;
    struct bsal_vector loads_unsorted;
    struct bsal_vector burdened_workers;
    struct bsal_vector stalled_workers;
    struct bsal_worker *worker;

    /*struct bsal_set *set;*/
    struct bsal_pair pair;
    struct bsal_vector_iterator vector_iterator;
    int old_worker;
    int actor_name;
    int messages;
    int maximum;
    int with_maximum;
    struct bsal_map *set;
    struct bsal_map_iterator set_iterator;
    int stalled_index;
    int stalled_count;
    int new_worker_index;
    struct bsal_vector migrations;
    struct bsal_migration migration;
    struct bsal_migration *migration_to_do;
    struct bsal_actor *actor;
    int candidates;

    int load_value;
    int remaining_load;
    int projected_load;

    struct bsal_vector actors_to_migrate;
    int total;
    int with_messages;
    int stalled_percentile;
    int burdened_percentile;

    int old_total;
    int old_load;
    int new_load;
    int predicted_new_load;
    struct bsal_pair *pair_pointer;
    struct bsal_worker *new_worker;
    /*int new_total;*/
    int actor_load;

    int test_stalled_index;
    int tests;
    int found_match;

    /* Lock all workers first
     */
    for (i = 0; i < pool->workers; i++) {
        worker = bsal_worker_pool_get_worker(pool, i);

        bsal_worker_lock(worker);
    }


    bsal_vector_init(&migrations, sizeof(struct bsal_migration));
    printf("BALANCING\n");

    bsal_vector_init(&loads, sizeof(int));
    bsal_vector_init(&loads_unsorted, sizeof(int));
    bsal_vector_init(&burdened_workers, sizeof(struct bsal_pair));
    bsal_vector_init(&stalled_workers, sizeof(struct bsal_pair));

    bsal_vector_init(&actors_to_migrate, sizeof(struct bsal_pair));

    for (i = 0; i < pool->workers; i++) {
        worker = bsal_worker_pool_get_worker(pool, i);
        load_value = bsal_worker_get_scheduling_epoch_load(worker) * SCHEDULER_PRECISION;

#if 0
        printf("DEBUG LOAD %d %d\n", i, load_value);
#endif

        bsal_vector_push_back(&loads, &load_value);
        bsal_vector_push_back(&loads_unsorted, &load_value);
    }

    bsal_vector_helper_sort_int(&loads);

    stalled_percentile = bsal_statistics_get_percentile_int(&loads, SCHEDULER_WINDOW);
    /*load_percentile_25 = bsal_statistics_get_percentile_int(&loads, 25);*/
    load_percentile_50 = bsal_statistics_get_percentile_int(&loads, 50);
    /*load_percentile_75 = bsal_statistics_get_percentile_int(&loads, 75);*/
    burdened_percentile = bsal_statistics_get_percentile_int(&loads, 100 - SCHEDULER_WINDOW);

    printf("Percentiles for epoch loads: ");
    bsal_statistics_get_print_percentiles_int(&loads);

    for (i = 0; i < pool->workers; i++) {
        worker = bsal_worker_pool_get_worker(pool, i);
        load_value = bsal_vector_helper_at_as_int(&loads_unsorted, i);

        set = bsal_worker_get_actors(worker);

        if (stalled_percentile == burdened_percentile) {

            printf("scheduling_class:%s ",
                            BSAL_CLASS_NORMAL_STRING);

        } else if (load_value <= stalled_percentile) {

            printf("scheduling_class:%s ",
                            BSAL_CLASS_STALLED_STRING);
            bsal_pair_init(&pair, load_value, i);
            bsal_vector_push_back(&stalled_workers, &pair);

        } else if (load_value >= burdened_percentile) {

            printf("scheduling_class:%s ",
                            BSAL_CLASS_BURDENED_STRING);

            bsal_pair_init(&pair, load_value, i);
            bsal_vector_push_back(&burdened_workers, &pair);
        } else {
            printf("scheduling_class:%s ",
                            BSAL_CLASS_NORMAL_STRING);
        }

        bsal_worker_print_actors(worker);

    }

    bsal_vector_helper_sort_int_reverse(&burdened_workers);
    bsal_vector_helper_sort_int(&stalled_workers);

    stalled_count = bsal_vector_size(&stalled_workers);
    printf("MIGRATIONS (stalled: %d, burdened: %d)\n", (int)bsal_vector_size(&stalled_workers),
                    (int)bsal_vector_size(&burdened_workers));

    stalled_index = 0;
    bsal_vector_iterator_init(&vector_iterator, &burdened_workers);

    while (stalled_count > 0
                    && bsal_vector_iterator_get_next_value(&vector_iterator, &pair)) {

        old_worker = bsal_pair_get_second(&pair);

        worker = bsal_worker_pool_get_worker(pool, old_worker);
        set = bsal_worker_get_actors(worker);

        /*
        bsal_worker_print_actors(worker);
        printf("\n");
        */

        /*
         * Lock the worker and try to select actors for migration
         */
        bsal_map_iterator_init(&set_iterator, set);

        maximum = -1;
        with_maximum = 0;
        total = 0;
        with_messages = 0;

        while (bsal_map_iterator_get_next_key_and_value(&set_iterator, &actor_name, NULL)) {

            actor = bsal_node_get_actor_from_name(worker->node, actor_name);
            messages = bsal_worker_pool_get_actor_production(pool, actor);

            if (maximum == -1 || messages > maximum) {
                maximum = messages;
                with_maximum = 1;
            } else if (messages == maximum) {
                with_maximum++;
            }

            if (messages > 0) {
                ++with_messages;
            }

            total += messages;
        }

        bsal_map_iterator_destroy(&set_iterator);

        bsal_map_iterator_init(&set_iterator, set);

        --with_maximum;

        candidates = 0;
        load_value = bsal_worker_get_scheduling_epoch_load(worker) * SCHEDULER_PRECISION;

        remaining_load = load_value;

#if 0
        printf("maximum %d with_maximum %d\n", maximum, with_maximum);
#endif

        while (bsal_map_iterator_get_next_key_and_value(&set_iterator, &actor_name, NULL)) {

            actor = bsal_node_get_actor_from_name(worker->node, actor_name);
            messages = bsal_worker_pool_get_actor_production(pool, actor);

            /* Simulate the remaining load
             */
            projected_load = remaining_load;
            projected_load -= ((0.0 + messages) / total) * load_value;

            printf(" TESTING actor %d, production was %d, projected_load is %d (- %d * (1 - %d/%d)\n",
                            actor_name, messages, projected_load,
                            load_value, messages, total);

            /* An actor without any queued messages should not be migrated
             */
            if (messages > 0
                            && ((with_maximum > 0 && messages == maximum) || messages < maximum)
                /*
                 * Avoid removing too many actors because
                 * generating a stalled one is not desired
                 */
                    && (projected_load >= load_percentile_50

                /*
                 * The previous rule does not apply when there
                 * are 2 actors.
                 */
                   || with_messages == 2) ) {

                remaining_load = projected_load;

                candidates++;

                if (messages == maximum) {
                    --with_maximum;
                }


                bsal_pair_init(&pair, messages, actor_name);
                bsal_vector_push_back(&actors_to_migrate, &pair);

                printf("early CANDIDATE for migration: actor %d, worker %d\n",
                                actor_name, old_worker);
            }
        }
        bsal_map_iterator_destroy(&set_iterator);

    }

    bsal_vector_iterator_destroy(&vector_iterator);

    /* Sort the candidates
     */

    bsal_vector_helper_sort_int(&actors_to_migrate);

    /*
    printf("Percentiles for production: ");
    bsal_statistics_get_print_percentiles_int(&actors_to_migrate);
    */

    /* Sort them in reverse order.
     */
    bsal_vector_helper_sort_int_reverse(&actors_to_migrate);

    bsal_vector_iterator_init(&vector_iterator, &actors_to_migrate);

    while (bsal_vector_iterator_get_next_value(&vector_iterator, &pair)) {

        actor_name = bsal_pair_get_second(&pair);

        actor = bsal_node_get_actor_from_name(pool->node, actor_name);

        if (actor == NULL) {
           continue;
        }

        messages = bsal_worker_pool_get_actor_production(pool, actor);
        bsal_map_get_value(&pool->actor_affinities, &actor_name, &old_worker);

        worker = bsal_worker_pool_get_worker(pool, old_worker);

        /* old_total can not be 0 because otherwise the would not
         * be burdened.
         */
        old_total = bsal_worker_get_production(worker);
        with_messages = bsal_worker_get_producer_count(worker);
        old_load = bsal_worker_get_scheduling_epoch_load(worker) * SCHEDULER_PRECISION;
        actor_load = ((0.0 + messages) / old_total) * old_load;

        /* Try to find a stalled worker that can take it.
         */

        test_stalled_index = stalled_index;
        tests = 0;
        predicted_new_load = 0;

        found_match = 0;
        while (tests < stalled_count) {

            bsal_vector_get_value(&stalled_workers, test_stalled_index, &pair);
            new_worker_index = bsal_pair_get_second(&pair);

            new_worker = bsal_worker_pool_get_worker(pool, new_worker_index);
            new_load = bsal_worker_get_scheduling_epoch_load(new_worker) * SCHEDULER_PRECISION;
        /*new_total = bsal_worker_get_production(new_worker);*/

            predicted_new_load = new_load + actor_load;

            if (predicted_new_load > SCHEDULER_PRECISION /* && with_messages != 2 */) {
                printf("Scheduler: skipping actor %d, predicted load is %d >= 100\n",
                           actor_name, predicted_new_load);

                ++tests;
                ++test_stalled_index;

                if (test_stalled_index == stalled_count) {
                    test_stalled_index = 0;
                }
                continue;
            }

            /* Otherwise, this stalled worker is fine...
             */
            stalled_index = test_stalled_index;
            found_match = 1;

            break;
        }

        /* This actor can not be migrated to any stalled worker.
         */
        if (!found_match) {
            continue;
        }

        /* Otherwise, update the load of the stalled one and go forward with the change.
         */
        
        pair_pointer = (struct bsal_pair *)bsal_vector_at(&stalled_workers, stalled_index);

        bsal_pair_set_first(pair_pointer, predicted_new_load);

        ++stalled_index;

        if (stalled_index == stalled_count) {
            stalled_index = 0;
        }


#if 0
        new_worker = bsal_worker_pool_get_worker(pool, new_worker_index);
        printf(" CANDIDATE: actor %d old worker %d (%d - %d = %d) new worker %d (%d + %d = %d)\n",
                        actor_name,
                        old_worker, value, messages, 2new_score,
                        new_worker_index, new_worker_old_score, messages, new_worker_new_score);
#endif

        bsal_migration_init(&migration, actor_name, old_worker, new_worker_index);
        bsal_vector_push_back(&migrations, &migration);
        bsal_migration_destroy(&migration);

    }

    bsal_vector_iterator_destroy(&vector_iterator);

    bsal_vector_destroy(&stalled_workers);
    bsal_vector_destroy(&burdened_workers);
    bsal_vector_destroy(&loads);
    bsal_vector_destroy(&loads_unsorted);
    bsal_vector_destroy(&actors_to_migrate);

    /* Update the last values
     */
    for (i = 0; i < pool->workers; i++) {

        worker = bsal_worker_pool_get_worker(pool, i);

        set = bsal_worker_get_actors(worker);

        bsal_map_iterator_init(&set_iterator, set);

        while (bsal_map_iterator_get_next_key_and_value(&set_iterator, &actor_name, NULL)) {
            actor = bsal_node_get_actor_from_name(pool->node, actor_name);
            bsal_worker_pool_update_actor_production(pool, actor);
        }
        bsal_map_iterator_destroy(&set_iterator);

        bsal_worker_reset_scheduling_epoch(worker);
    }

    /* Actually do the migrations
     */
    bsal_vector_iterator_init(&vector_iterator, &migrations);

    while (bsal_vector_iterator_next(&vector_iterator, (void **)&migration_to_do)) {

        bsal_worker_pool_migrate(pool, migration_to_do);
    }

    bsal_vector_iterator_destroy(&vector_iterator);

    bsal_vector_destroy(&migrations);

    /* Unlock all workers
     */
    for (i = 0; i < pool->workers; i++) {
        worker = bsal_worker_pool_get_worker(pool, i);

        bsal_worker_unlock(worker);
    }

}

void bsal_worker_pool_give_message_to_actor(struct bsal_worker_pool *pool, struct bsal_message *message)
{
    int destination;
    struct bsal_actor *actor;
    struct bsal_worker *affinity_worker;
    int worker_index;
    int score;
    int name;
    struct bsal_set *set;

    destination = bsal_message_destination(message);
    actor = bsal_node_get_actor_from_name(pool->node, destination);

#if 0
    printf("DEBUG bsal_worker_pool_give_message_to_actor %d\n", destination);
#endif

    if (actor == NULL) {
        printf("DEAD LETTER CHANNEL...\n");
        return;
    }

    name = bsal_actor_name(actor);

    /* give the message to the actor
     */
    if (!bsal_actor_enqueue_mailbox_message(actor, message)) {
        bsal_ring_queue_enqueue(&pool->inbound_message_queue_buffer, message);

    /* Check if the actor is assigned to a worker
     */
    } else {
/*
        printf("DEBUG message was enqueued in actor mailbox\n");
        */

        if (bsal_map_get_value(&pool->actor_affinities, &name, &worker_index)) {

            affinity_worker = bsal_worker_pool_get_worker(pool, worker_index);

            /*
            printf("DEBUG actor has an assigned worker\n");
            */

            if (!bsal_worker_enqueue_actor(affinity_worker, &actor)) {
                bsal_ring_queue_enqueue(&pool->scheduled_actor_queue_buffer, &actor);
            }

        } else {

                /*
            printf("DEBUG Needs to do actor placement\n");
            */
            /* assign this actor to the least busy actor
             */
            worker_index = bsal_worker_pool_select_worker_least_busy(pool, message, &score);


            bsal_map_add_value(&pool->actor_affinities, &name, &worker_index);
            set = (struct bsal_set *)bsal_vector_at(&pool->worker_actors, worker_index);
            bsal_set_add(set, &name);

            affinity_worker = bsal_worker_pool_get_worker(pool, worker_index);

            if (!bsal_worker_enqueue_actor(affinity_worker, &actor)) {
                bsal_ring_queue_enqueue(&pool->scheduled_actor_queue_buffer, &actor);
            }
        }
    }
}

void bsal_worker_pool_migrate(struct bsal_worker_pool *pool, struct bsal_migration *migration)
{
    struct bsal_worker *old_worker_object;
    struct bsal_worker *new_worker_object;
    int old_worker;
    int new_worker;
    int actor_name;
    struct bsal_actor *actor;

    old_worker = bsal_migration_get_old_worker(migration);
    new_worker = bsal_migration_get_new_worker(migration);
    actor_name = bsal_migration_get_actor(migration);
    actor = bsal_node_get_actor_from_name(pool->node, actor_name);

    printf("MIGRATION node %d migrated actor %d from worker %d to worker %d\n",
                    bsal_node_name(pool->node), actor_name,
                    old_worker, new_worker);

    old_worker_object = bsal_worker_pool_get_worker(pool, old_worker);
    new_worker_object = bsal_worker_pool_get_worker(pool, new_worker);

    /* evict the actor from the old worker
     */
    bsal_worker_evict_actor(old_worker_object, actor_name);

    /* Redirect messages for this actor to the
     * new worker
     */
    bsal_map_update_value(&pool->actor_affinities, &actor_name, &new_worker);

#ifdef BSAL_WORKER_POOL_DEBUG_MIGRATION
    printf("ROUTE actor %d ->  worker %d\n", actor_name, new_worker);
#endif

    bsal_worker_enqueue_actor_special(new_worker_object, &actor);

}

void bsal_worker_pool_print_efficiency(struct bsal_worker_pool *pool)
{
    double efficiency;
    struct bsal_worker *worker;
    int i;

    efficiency = 0;

    for (i = 0; i < pool->workers; i++) {
        worker = bsal_worker_pool_get_worker(pool, i);
        efficiency += bsal_worker_get_loop_load(worker);
    }

    efficiency /= pool->workers;

    printf("node %d efficiency: %.2f\n",
                    bsal_node_name(pool->node),
                    efficiency);

}

int bsal_worker_pool_get_actor_production(struct bsal_worker_pool *pool, struct bsal_actor *actor)
{
    int messages;
    int last_messages;
    int name;
    int result;

    if (actor == NULL) {
        return 0;
    }

    messages = bsal_actor_get_sum_of_received_messages(actor);

    last_messages = 0;
    name = bsal_actor_name(actor);
    bsal_map_get_value(&pool->last_actor_received_messages, &name, &last_messages);

    result = messages - last_messages;

    return result;
}

void bsal_worker_pool_update_actor_production(struct bsal_worker_pool *pool, struct bsal_actor *actor)
{
    int messages;
    int name;

    if (actor == NULL) {
        return;
    }

    messages = bsal_actor_get_sum_of_received_messages(actor);
    name = bsal_actor_name(actor);

    if (!bsal_map_update_value(&pool->last_actor_received_messages, &name, &messages)) {
        bsal_map_add_value(&pool->last_actor_received_messages, &name, &messages);
    }
}
