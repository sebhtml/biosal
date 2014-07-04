
#include "worker_pool.h"

#include "actor.h"
#include "work.h"
#include "worker.h"
#include "node.h"

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
#define BSAL_WORKER_POOL_WORK_SCHEDULING_WINDOW 1024

#define BSAL_WORKER_POOL_BALANCE

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

    bsal_ring_queue_init(&pool->local_work_queue, sizeof(struct bsal_work));

    bsal_map_init(&pool->actor_affinities, sizeof(int), sizeof(int));
    bsal_map_init(&pool->actor_inbound_messages, sizeof(int), sizeof(int));
    bsal_vector_init(&pool->worker_actors, sizeof(struct bsal_set));

    bsal_vector_resize(&pool->worker_actors, pool->workers);

    for (i = 0; i < pool->workers; i++) {

        set = (struct bsal_set *)bsal_vector_at(&pool->worker_actors, i);

        bsal_set_init(set, sizeof(int));
    }

    pool->received_works = 0;

    pool->balance_period = pool->workers * 8192 * 2;
}

void bsal_worker_pool_destroy(struct bsal_worker_pool *pool)
{
    int i;
    struct bsal_set *set;

    for (i = 0; i < pool->workers; i++) {
        set= (struct bsal_set *)bsal_vector_at(&pool->worker_actors, i);

        bsal_set_destroy(set);
    }

    bsal_worker_pool_delete_workers(pool);

    pool->node = NULL;

#ifdef BSAL_WORKER_POOL_HAS_SPECIAL_QUEUES
    bsal_work_queue_destroy(&pool->work_queue);
    bsal_message_queue_destroy(&pool->message_queue);
#endif

    bsal_ring_queue_destroy(&pool->local_work_queue);

    bsal_map_destroy(&pool->actor_affinities);
    bsal_map_destroy(&pool->actor_inbound_messages);
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

/* select the worker to push work to */
struct bsal_worker *bsal_worker_pool_select_worker_for_work(
                struct bsal_worker_pool *pool, struct bsal_work *work)
{
#ifdef BSAL_WORKER_POOL_PUSH_WORK_ON_SAME_WORKER

/*#error "BAD"*/
    /* check first if the actor is active.
     * If it is active, just enqueue the work at the same
     * place (on the same worker).
     * This avoids contention.
     * This is only required for important actors...
     * Important actors are those who receive a lot of messages.
     * This is roughly equivalent to have a dedicated worker for
     * an important worker.
     */
    struct bsal_actor *actor;
    struct bsal_worker *worker;
    struct bsal_worker *affinity_worker;
    int best_score;
    int name;
    int worker_index;
    struct bsal_set *set;

    actor = bsal_work_actor(work);
    name = bsal_actor_name(actor);


#ifdef BSAL_WORKER_POOL_USE_CURRENT_WORKER
    worker = bsal_actor_worker(actor);

    if (worker != NULL) {
#if 0
        printf("USING current worker\n");
#endif

#ifdef BSAL_WORKER_POOL_DEBUG_SCHEDULING_SAME_WORKER
        if (bsal_actor_script(actor) == (int)0x82673850) {
            printf("DEBUG7890 node %d scheduling work for aggregator on worker %d\n",
                            bsal_node_name(pool->node),
                            bsal_worker_name(worker));
        }
#endif

        return worker;
    }
#endif
#endif

    /* Check if the affinity worker has a score lower than a certain
     * value.
     */

    affinity_worker = NULL;

    /* Use the affinity worker if any.
     */
    if (bsal_map_get_value(&pool->actor_affinities, &name, &worker_index)) {

        affinity_worker = bsal_worker_pool_get_worker(pool, worker_index);

        if (bsal_worker_get_work_scheduling_score(affinity_worker) <= 4) {
            return affinity_worker;
        }
    }


    /* Use the least busy worker (use a idle worker).
     */

#ifdef BSAL_WORKER_POOL_USE_LEAST_BUSY
    worker_index = bsal_worker_pool_select_worker_least_busy(pool, work, &best_score);

    worker = bsal_worker_pool_get_worker(pool, worker_index);

    if (best_score == 0) {
        return worker;
    }

    if (affinity_worker != NULL) {
        return affinity_worker;
    }

    /* Otherwise, assign the worker
     */
    /*
     * Assign the actor to the worker.
     */
    bsal_map_add_value(&pool->actor_affinities, &name, &worker_index);
    set = (struct bsal_set *)bsal_vector_at(&pool->worker_actors, worker_index);
    bsal_set_add(set, &name);

    printf("SCHEDULER scheduling actor %d on node %d, worker %d\n", name,
                    bsal_node_name(pool->node),
                    bsal_worker_name(worker));

    /* return the worker with the lowest load
     */
    return worker;

#else
    return bsal_worker_pool_select_worker_round_robin(pool, work);
#endif
}

struct bsal_worker *bsal_worker_pool_select_worker_round_robin(
                struct bsal_worker_pool *pool, struct bsal_work *work)
{
    int index;

#if 0
    struct bsal_worker *worker;

    /* check if actor has an affinity worker */
    worker = bsal_actor_affinity_worker(bsal_work_actor(work));

    if (worker != NULL) {
        return worker;
    }
#endif

    /* otherwise, pick a worker with round robin */
    index = pool->worker_for_message;
    pool->worker_for_message = bsal_worker_pool_next_worker(pool, pool->worker_for_message);

    return bsal_worker_pool_get_worker(pool, index);
}

#ifdef BSAL_WORKER_HAS_OWN_QUEUES
int bsal_worker_pool_select_worker_least_busy(
                struct bsal_worker_pool *self, struct bsal_work *work, int *worker_score)
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

        score = bsal_worker_get_work_scheduling_score(worker);

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

void bsal_worker_pool_schedule_work(struct bsal_worker_pool *pool, struct bsal_work *work)
{
    struct bsal_work other_work;
    struct bsal_actor *actor;
    int name;
    int value;

#ifdef BSAL_WORKER_POOL_DEBUG
    if (pool->debug_mode) {
        printf("DEBUG bsal_worker_pool_schedule_work called\n");
    }

    int tag;
    int destination;
    struct bsal_message *message;

    message = bsal_work_message(work);
    tag = bsal_message_tag(message);
    destination = bsal_message_destination(message);

    if (tag == BSAL_ACTOR_ASK_TO_STOP) {
        printf("DEBUG bsal_worker_pool_schedule_work tag BSAL_ACTOR_ASK_TO_STOP actor %d\n",
                        destination);
    }
#endif

#ifdef BSAL_WORKER_POOL_BALANCE
    /* balance the pool regularly
     */
    if (pool->received_works % pool->balance_period == 0) {
        bsal_worker_pool_balance(pool);
    }
#endif

    pool->received_works++;

    actor = bsal_work_actor(work);
    name = bsal_actor_name(actor);
    value = 1;

    if (!bsal_map_get_value(&pool->actor_inbound_messages, &name, &value)) {
        bsal_map_add_value(&pool->actor_inbound_messages, &name, &value);
    } else {
        ++value;
        bsal_map_update_value(&pool->actor_inbound_messages, &name, &value);
    }

#ifdef BSAL_WORKER_HAS_OWN_QUEUES
    bsal_worker_pool_schedule_work_classic(pool, work);

    if (bsal_ring_queue_dequeue(&pool->local_work_queue, &other_work)) {
        bsal_worker_pool_schedule_work_classic(pool, &other_work);
    }
#else
    bsal_work_queue_enqueue(&pool->work_queue, work);
#endif
}

#ifdef BSAL_WORKER_HAS_OWN_QUEUES
/*
 * names are based on names found in:
 * \see http://lxr.free-electrons.com/source/include/linux/workqueue.h
 * \see http://lxr.free-electrons.com/source/kernel/workqueue.c
 */
void bsal_worker_pool_schedule_work_classic(struct bsal_worker_pool *pool, struct bsal_work *work)
{
    struct bsal_worker *worker;
    int count;

    worker = bsal_worker_pool_select_worker_for_work(pool, work);

    /* if the work can not be queued,
     * it will be queued later
     */
    if (!bsal_worker_push_work(worker, work)) {

        printf("Warning: CONTENTION detected on node %d, worker %d\n",
                        bsal_node_name(pool->node), bsal_worker_name(worker));
        bsal_ring_queue_enqueue(&pool->local_work_queue, work);

        count = bsal_ring_queue_size(&pool->local_work_queue);

        if (count >= BSAL_WORKER_WARNING_THRESHOLD
                        && (pool->last_warning == 0
                                || count >= pool->last_warning + BSAL_WORKER_WARNING_THRESHOLD_STRIDE)) {
            printf("Warning node %d has %d works in its local queue\n",
                            bsal_node_name(pool->node), count);

            pool->last_warning = count;
        }
    }
}
#endif

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
    char loop[] = "LOOP";
    char epoch[] = "EPOCH";
    char *description;

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

    while (i < count && offset + extra < allocated) {

        worker = bsal_worker_pool_get_worker(self, i);
        epoch_load = bsal_worker_get_epoch_load(worker);
        loop_load = bsal_worker_get_loop_load(worker);
        /*
        scheduling_score = bsal_worker_get_work_scheduling_score(worker);
        */

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

        ++i;
    }

    printf("LOAD %s %d s node/%d%s\n", description, elapsed, node_name, buffer);

    bsal_memory_free(buffer);
}

void bsal_worker_pool_toggle_debug_mode(struct bsal_worker_pool *self)
{
    self->debug_mode = !self->debug_mode;
}

void bsal_worker_pool_balance(struct bsal_worker_pool *pool)
{
    double mean;
    double standard_deviation;
    double coefficient_of_variation;

    int i;
    struct bsal_vector scheduling_scores;
    struct bsal_vector burdened_workers;
    struct bsal_vector stalled_workers;
    int value;
    struct bsal_worker *worker;
    struct bsal_set *set;
    int actor_name;
    struct bsal_set_iterator iterator;
    struct bsal_map_iterator map_iterator;
    struct bsal_vector_iterator vector_iterator;
    struct bsal_vector_iterator vector_iterator2;
    int score;
    int transfers;
    struct bsal_pair pair;
    struct bsal_vector sorted_actors;
    struct bsal_vector global_sorted_actors;
    int transfers_per_worker;
    int actor_index;
    int worker_index;
    int stalled_index;
    int stalled_count;
    int new_worker;
    int average_actor_score;
    int median;
    int multiplier;
    int minimum;
    float load;

    printf("BALANCING\n");

    bsal_vector_init(&burdened_workers, sizeof(struct bsal_pair));
    bsal_vector_init(&global_sorted_actors, sizeof(struct bsal_pair));
    bsal_vector_init(&stalled_workers, sizeof(struct bsal_pair));
    bsal_vector_init(&scheduling_scores, sizeof(int));

    for (i = 0; i < pool->workers; i++) {
        worker = bsal_worker_pool_get_worker(pool, i);
        value = bsal_worker_get_work_scheduling_score(worker);

        bsal_vector_push_back(&scheduling_scores, &value);
    }

    multiplier = 1;
    minimum = 16;
    mean = bsal_statistics_get_mean_int(&scheduling_scores);
    standard_deviation = bsal_statistics_get_standard_deviation_int(&scheduling_scores);
    coefficient_of_variation = 0;
    if (standard_deviation != 0) {
        coefficient_of_variation = standard_deviation / mean;
    }

    printf("Mean: %f Standard deviation: %f Coefficient %f\n", mean, standard_deviation,
                    coefficient_of_variation);

    for (i = 0; i < pool->workers; i++) {
        value = bsal_vector_helper_at_as_int(&scheduling_scores, i);
        worker = bsal_worker_pool_get_worker(pool, i);
        load = bsal_worker_get_epoch_load(worker);

        set = (struct bsal_set *)bsal_vector_at(&pool->worker_actors, i);

        printf(" worker %i messages... %d (%d actors)", i, value,
                        (int)bsal_set_size(set));

        if (value == 0 || value < mean - multiplier * standard_deviation) {
            printf(" STALLED !\n");

            bsal_pair_init(&pair, value, i);
            bsal_vector_push_back(&stalled_workers, &pair);

        } else if (value >= minimum && load >= 0.50
                       && value > mean + multiplier * standard_deviation) {
            printf(" BURDENED...\n");

            bsal_pair_init(&pair, value, i);
            bsal_vector_push_back(&burdened_workers, &pair);
        } else {
            printf("\n");
        }
    }

    transfers = bsal_vector_size(&stalled_workers);

    if (bsal_vector_size(&burdened_workers) > transfers) {
        transfers = bsal_vector_size(&burdened_workers);
    }

    transfers_per_worker = 0;

    if (bsal_vector_size(&burdened_workers) > 0) {
        transfers_per_worker = transfers / bsal_vector_size(&burdened_workers);
    }

    printf("SUMMARY: stalled: %d, burdened: %d, required transfers: %d, transfers_per_worker: %d\n",
                    (int)bsal_vector_size(&stalled_workers),
                    (int)bsal_vector_size(&burdened_workers),
                    transfers, transfers_per_worker);

    bsal_vector_helper_sort_int_reverse(&burdened_workers);

    bsal_vector_iterator_init(&vector_iterator, &burdened_workers);

    while (bsal_vector_iterator_get_next_value(&vector_iterator, &pair)) {

        value = bsal_pair_get_first(&pair);
        i = bsal_pair_get_second(&pair);

        printf("SORTED burden %d worker %d\n", value, i);

        /*
         * Pop an actor from this worker.
         * The scheduler will schedule this actor on another worker next time.
         */

        worker = bsal_worker_pool_get_worker(pool, i);
        set = (struct bsal_set *)bsal_vector_at(&pool->worker_actors, i);

        bsal_set_iterator_init(&iterator, set);
        bsal_vector_init(&sorted_actors, sizeof(struct bsal_pair));
        while (bsal_set_iterator_get_next_value(&iterator, &actor_name)) {

            score = 0;
            bsal_map_get_value(&pool->actor_inbound_messages, &actor_name, &score);

            bsal_pair_init(&pair, score, actor_name);

            bsal_vector_push_back(&sorted_actors, &pair);
        }
        bsal_set_iterator_destroy(&iterator);
        bsal_vector_helper_sort_int_reverse(&sorted_actors);

        bsal_vector_iterator_init(&vector_iterator2, &sorted_actors);

        actor_index = 0;

        while (bsal_vector_iterator_get_next_value(&vector_iterator2, &pair)) {

            score = bsal_pair_get_first(&pair);
            actor_name = bsal_pair_get_second(&pair);

            printf(" ----> SCORE %d ACTOR %d", score, actor_name);

            /*
             * Skip the largest one because it is fine on this
             * worker.
             */
            if (actor_index != 0) {
                printf(" CANDIDATE for MIGRATION\n");

                bsal_vector_push_back(&global_sorted_actors, &pair);
            } else {
                printf("\n");
            }

            ++actor_index;
        }

        bsal_vector_iterator_destroy(&vector_iterator2);
        bsal_vector_destroy(&sorted_actors);
    }

    bsal_vector_iterator_destroy(&vector_iterator);

    /*
     * move @transfers actors
     */

    bsal_vector_helper_sort_int_reverse(&global_sorted_actors);

    average_actor_score = bsal_statistics_get_mean_int(&global_sorted_actors);
    bsal_vector_helper_sort_int(&stalled_workers);
    stalled_count = bsal_vector_size(&stalled_workers);
    median = bsal_statistics_get_median_int(&global_sorted_actors);

    printf("AVERAGE %d MEDIAN %d\n", average_actor_score, median);

    bsal_vector_iterator_init(&vector_iterator, &global_sorted_actors);

    stalled_index = 0;

    while (transfers
                    && bsal_vector_iterator_get_next_value(&vector_iterator, &pair)) {

        score = bsal_pair_get_first(&pair);

        if (!stalled_count) {
            break;
        }
        if (score < average_actor_score) {
            break;
        }

        actor_name = bsal_pair_get_second(&pair);
        bsal_map_get_value(&pool->actor_affinities, &actor_name, &worker_index);

        /*
         * Remove the actor from the affinity
         * table.
         */
        set = (struct bsal_set *)bsal_vector_at(&pool->worker_actors, worker_index);
        bsal_set_delete(set, &actor_name);

        bsal_vector_get_value(&stalled_workers, stalled_index, &pair);
        value = bsal_pair_get_first(&pair);
        new_worker = bsal_pair_get_second(&pair);

        printf("MIGRATING actor %d (%d) from worker %d to worker %d (%d, index %d/%d)\n",
                        actor_name, score, worker_index, new_worker, value,
                        stalled_index, stalled_count);

        set = (struct bsal_set *)bsal_vector_at(&pool->worker_actors, new_worker);
        bsal_set_add(set, &actor_name);

        bsal_map_update_value(&pool->actor_affinities, &actor_name, &new_worker);

        --transfers;
        ++stalled_index;

        if (stalled_index >= stalled_count) {
            stalled_index -= stalled_count;
        }
    }

    bsal_vector_iterator_destroy(&vector_iterator);

    bsal_vector_destroy(&stalled_workers);
    bsal_vector_destroy(&scheduling_scores);
    bsal_vector_destroy(&burdened_workers);
    bsal_vector_destroy(&global_sorted_actors);

    printf("Update metrics\n");
    /*
     * Update metrics
     */
    bsal_map_iterator_init(&map_iterator, &pool->actor_inbound_messages);

    while (bsal_map_iterator_get_next_key_and_value(&map_iterator, &actor_name, &value)) {
        value /= 2;
        bsal_map_update_value(&pool->actor_inbound_messages, &actor_name, &value);
    }
    bsal_map_iterator_destroy(&map_iterator);
}
