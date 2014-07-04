
#include "worker_pool.h"

#include "actor.h"
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

/*
#define BSAL_WORKER_POOL_BALANCE
*/

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
    bsal_vector_init(&pool->worker_actors, sizeof(struct bsal_set));

    bsal_vector_resize(&pool->worker_actors, pool->workers);

    for (i = 0; i < pool->workers; i++) {

        set = (struct bsal_set *)bsal_vector_at(&pool->worker_actors, i);

        bsal_set_init(set, sizeof(int));
    }

    pool->received_works = 0;

    pool->balance_period = pool->workers * 1024;
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

    bsal_ring_queue_destroy(&pool->inbound_message_queue_buffer);
    bsal_ring_queue_destroy(&pool->scheduled_actor_queue_buffer);

    bsal_map_destroy(&pool->actor_affinities);
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
    struct bsal_pair pair;
    int multiplier;
    int minimum;
    float load;

    printf("BALANCING\n");

    bsal_vector_init(&burdened_workers, sizeof(struct bsal_pair));
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
        bsal_worker_print_actors(worker);
    }


    bsal_vector_destroy(&stalled_workers);
    bsal_vector_destroy(&scheduling_scores);
    bsal_vector_destroy(&burdened_workers);
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


