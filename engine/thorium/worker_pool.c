
#include "worker_pool.h"

#include "actor.h"
#include "worker.h"
#include "node.h"

#include "scheduler/migration.h"

#include <core/helpers/vector_helper.h>
#include <core/helpers/statistics.h>
#include <core/helpers/pair.h>

#include <core/structures/set_iterator.h>
#include <core/structures/vector_iterator.h>

#include <core/system/debugger.h>
#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>


/*
#define THORIUM_WORKER_POOL_DEBUG
#define THORIUM_WORKER_POOL_DEBUG_ISSUE_334
*/

#define THORIUM_WORKER_POOL_MESSAGE_SCHEDULING_WINDOW 4

/*
 * Scheduling options.
 */
#define THORIUM_WORKER_POOL_PUSH_WORK_ON_SAME_WORKER
#define THORIUM_WORKER_POOL_FORCE_LAST_WORKER 1

/*
 * Configuration for initial placement.
 */
/*
#define THORIUM_WORKER_POOL_USE_LEAST_BUSY
*/
#define THORIUM_WORKER_POOL_USE_SCRIPT_ROUND_ROBIN

/*
 * Enable the load balancer.
 * The implementation needs to lock everything up,
 * so it is not very good for performance.
 * The next-generation load balancing will use an eviction approach
 * instead.
 */
/*
#define THORIUM_WORKER_POOL_BALANCE
*/

void thorium_worker_pool_init(struct thorium_worker_pool *pool, int workers,
                struct thorium_node *node)
{
    pool->debug_mode = 0;
    pool->node = node;
    pool->waiting_is_enabled = 0;

    thorium_scheduler_init(&pool->scheduler, pool);

    pool->ticks_without_messages = 0;

    bsal_fast_queue_init(&pool->messages_for_triage, sizeof(struct thorium_message));

    pool->last_warning = 0;
    pool->last_scheduling_warning = 0;

    pool->workers = workers;

    /* with only one thread,  the main thread
     * handles everything.
     */
    if (pool->workers >= 1) {
        pool->worker_for_run = 0;
        pool->worker_for_message = 0;
    } else {
        printf("Error: the number of workers must be at least 1.\n");
        exit(1);
    }

#ifdef THORIUM_WORKER_POOL_HAS_SPECIAL_QUEUES
    bsal_work_queue_init(&pool->work_queue);
    thorium_message_queue_init(&pool->message_queue);
#endif

    /*
     * Enable the wait/notify algorithm if running on more than
     * one node.
     */
    if (thorium_node_nodes(pool->node) >= 2) {
        pool->waiting_is_enabled = 1;
    }

    thorium_worker_pool_create_workers(pool);

    pool->starting_time = time(NULL);

    bsal_fast_queue_init(&pool->scheduled_actor_queue_buffer, sizeof(struct thorium_actor *));
    bsal_fast_queue_init(&pool->inbound_message_queue_buffer, sizeof(struct thorium_message));

    pool->last_balancing = pool->starting_time;
    pool->last_signal_check = pool->starting_time;

    pool->balance_period = THORIUM_SCHEDULER_PERIOD_IN_SECONDS;
}

void thorium_worker_pool_destroy(struct thorium_worker_pool *pool)
{
    thorium_scheduler_destroy(&pool->scheduler);

    thorium_worker_pool_delete_workers(pool);

    pool->node = NULL;

    bsal_fast_queue_destroy(&pool->inbound_message_queue_buffer);
    bsal_fast_queue_destroy(&pool->scheduled_actor_queue_buffer);
    bsal_fast_queue_destroy(&pool->messages_for_triage);
}

void thorium_worker_pool_delete_workers(struct thorium_worker_pool *pool)
{
    int i = 0;
    struct thorium_worker *worker;

    if (pool->workers <= 0) {
        return;
    }

    for (i = 0; i < pool->workers; i++) {
        worker = thorium_worker_pool_get_worker(pool, i);

#if 0
        printf("worker/%d loop_load %f\n", thorium_worker_name(worker),
                    thorium_worker_get_loop_load(worker));
#endif

        thorium_worker_destroy(worker);
    }

    bsal_vector_destroy(&pool->worker_array);
    bsal_vector_destroy(&pool->message_count_cache);
}

void thorium_worker_pool_create_workers(struct thorium_worker_pool *pool)
{
    int i;
    struct thorium_worker *worker;

    if (pool->workers <= 0) {
        return;
    }

    bsal_vector_init(&pool->worker_array, sizeof(struct thorium_worker));
    bsal_vector_init(&pool->message_count_cache, sizeof(int));

    bsal_vector_resize(&pool->worker_array, pool->workers);
    bsal_vector_resize(&pool->message_count_cache, pool->workers);

    pool->worker_cache = (struct thorium_worker *)bsal_vector_at(&pool->worker_array, 0);
    pool->message_cache = (int *)bsal_vector_at(&pool->message_count_cache, 0);

    for (i = 0; i < pool->workers; i++) {

        worker = thorium_worker_pool_get_worker(pool, i);
        thorium_worker_init(worker, i, pool->node);

        if (pool->waiting_is_enabled) {
            thorium_worker_enable_waiting(worker);
        }

        bsal_vector_set_int(&pool->message_count_cache, i, 0);
    }
}

void thorium_worker_pool_start(struct thorium_worker_pool *pool)
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

        if (thorium_node_nodes(pool->node) != 1) {
            processor = -1;
        }

        thorium_worker_start(thorium_worker_pool_get_worker(pool, i), processor);
    }
}

void thorium_worker_pool_run(struct thorium_worker_pool *pool)
{
    /* make the thread work (this is the main thread) */
    thorium_worker_run(thorium_worker_pool_select_worker_for_run(pool));
}

void thorium_worker_pool_stop(struct thorium_worker_pool *pool)
{
    int i;
    /*
     * stop workers
     */

#ifdef THORIUM_WORKER_POOL_DEBUG
    printf("Stop workers\n");
#endif

    for (i = 0; i < pool->workers; i++) {
        thorium_worker_stop(thorium_worker_pool_get_worker(pool, i));
    }
}

struct thorium_worker *thorium_worker_pool_select_worker_for_run(struct thorium_worker_pool *pool)
{
    int index;

    index = pool->worker_for_run;
    return thorium_worker_pool_get_worker(pool, index);
}

/* All messages go through here.
 */
int thorium_worker_pool_enqueue_message(struct thorium_worker_pool *pool, struct thorium_message *message)
{

    return thorium_worker_pool_give_message_to_actor(pool, message);
}

int thorium_worker_pool_worker_count(struct thorium_worker_pool *pool)
{
    return pool->workers;
}

void thorium_worker_pool_print_load(struct thorium_worker_pool *self, int type)
{
    int count;
    int i;
    float epoch_load;
    struct thorium_worker *worker;
    float loop_load;

    uint64_t epoch_wake_up_count;
    uint64_t loop_wake_up_count;
    /*
    int scheduling_score;
    */
    int node_name;
    char *buffer;
    char *buffer_for_wake_up_events;
    int allocated;
    int offset;
    int offset_for_wake_up;
    int extra;
    time_t current_time;
    int elapsed;
    float selected_load;
    uint64_t selected_wake_up_count;
    float sum;
    char loop[] = "COMPUTATION";
    char epoch[] = "EPOCH";
    char *description;
    float load;

    description = NULL;

    if (type == THORIUM_WORKER_POOL_LOAD_LOOP) {
        description = loop;
    } else if (type == THORIUM_WORKER_POOL_LOAD_EPOCH) {
        description = epoch;
    } else {
        return;
    }

    current_time = time(NULL);
    elapsed = current_time - self->starting_time;

    extra = 100;

    count = thorium_worker_pool_worker_count(self);
    allocated = count * 20 + 20 + extra;

    buffer = bsal_memory_allocate(allocated);
    buffer_for_wake_up_events = bsal_memory_allocate(allocated);
    node_name = thorium_node_name(self->node);
    offset = 0;
    offset_for_wake_up = 0;
    i = 0;
    sum = 0;

    while (i < count && offset + extra < allocated) {

        worker = thorium_worker_pool_get_worker(self, i);

        epoch_load = thorium_worker_get_epoch_load(worker);
        loop_load = thorium_worker_get_loop_load(worker);
        epoch_wake_up_count = thorium_worker_get_epoch_wake_up_count(worker);
        loop_wake_up_count = thorium_worker_get_loop_wake_up_count(worker);

        selected_load = epoch_load;
        selected_wake_up_count = epoch_wake_up_count;

        if (type == THORIUM_WORKER_POOL_LOAD_EPOCH) {
            selected_load = epoch_load;
            selected_wake_up_count = epoch_wake_up_count;

        } else if (type == THORIUM_WORKER_POOL_LOAD_LOOP) {
            selected_load = loop_load;
            selected_wake_up_count = loop_wake_up_count;
        }

        /*
        offset += sprintf(buffer + offset, " [%d %d %.2f]", i,
                        scheduling_score,
                        selected_load);
                        */
        offset += sprintf(buffer + offset, " %.2f",
                        selected_load);

        offset_for_wake_up += sprintf(buffer_for_wake_up_events + offset_for_wake_up, " %" PRIu64 "",
                        selected_wake_up_count);

        sum += selected_load;

        ++i;
    }

    load = sum / count;

    printf("%s node/%d %s LOAD %d s %.2f/%d (%.2f)%s\n",
                    THORIUM_NODE_THORIUM_PREFIX,
                    node_name,
                    description, elapsed,
                    sum, count, load, buffer);

    printf("%s node/%d %s WAKE_UP_COUNT %d s %s\n",
                    THORIUM_NODE_THORIUM_PREFIX,
                    node_name,
                    description, elapsed,
                    buffer_for_wake_up_events);

    bsal_memory_free(buffer);
    bsal_memory_free(buffer_for_wake_up_events);
}

void thorium_worker_pool_toggle_debug_mode(struct thorium_worker_pool *self)
{
    self->debug_mode = !self->debug_mode;
}

float thorium_worker_pool_get_computation_load(struct thorium_worker_pool *pool)
{
    double load;
    struct thorium_worker *worker;
    int i;

    load = 0;

    for (i = 0; i < pool->workers; i++) {
        worker = thorium_worker_pool_get_worker(pool, i);
        load += thorium_worker_get_loop_load(worker);
    }

    if (pool->workers != 0) {
        load /= pool->workers;
    }

    return load;
}

struct thorium_node *thorium_worker_pool_get_node(struct thorium_worker_pool *pool)
{
    return pool->node;
}


int thorium_worker_pool_give_message_to_actor(struct thorium_worker_pool *pool, struct thorium_message *message)
{
    int destination;
    struct thorium_actor *actor;
    struct thorium_worker *affinity_worker;
    int worker_index;
    int name;
    int dead;

    /*
    void *buffer;

    buffer = thorium_message_buffer(message);
    */
    destination = thorium_message_destination(message);
    actor = thorium_node_get_actor_from_name(pool->node, destination);

    if (actor == NULL) {
#ifdef THORIUM_WORKER_POOL_DEBUG_DEAD_CHANNEL
        printf("DEAD LETTER CHANNEL...\n");
#endif

        bsal_fast_queue_enqueue(&pool->messages_for_triage, message);

        return 0;
    }

    dead = thorium_actor_dead(actor);

    /* If the actor is dead, don't use it.
     */
    if (dead) {

        bsal_fast_queue_enqueue(&pool->messages_for_triage, message);

        return 0;
    }

    name = thorium_actor_name(actor);

    /* give the message to the actor
     */
    if (!thorium_actor_enqueue_mailbox_message(actor, message)) {

#ifdef THORIUM_WORKER_POOL_DEBUG_MESSAGE_BUFFERING
        printf("DEBUG897 could not enqueue message, buffering...\n");
#endif

        bsal_fast_queue_enqueue(&pool->inbound_message_queue_buffer, message);

    } else {
        /*
         * At this point, the message has been pushed to the actor.
         * Now, the actor must be scheduled on a worker.
         */
/*
        printf("DEBUG message was enqueued in actor mailbox\n");
        */

        /* Check if the actor is already assigned to a worker
         */
        worker_index = thorium_scheduler_get_actor_worker(&pool->scheduler, name);

        /* If not, ask the scheduler to assign the actor to a worker
         */
        if (worker_index < 0) {

            thorium_worker_pool_assign_worker_to_actor(pool, name);
            worker_index = thorium_scheduler_get_actor_worker(&pool->scheduler, name);
        }

        affinity_worker = thorium_worker_pool_get_worker(pool, worker_index);

        /*
        printf("DEBUG actor has an assigned worker\n");
        */

        /*
         * Push the actor on the scheduling queue of the worker.
         * If that fails, queue the actor.
         */
        if (!thorium_worker_enqueue_actor(affinity_worker, actor)) {
            bsal_fast_queue_enqueue(&pool->scheduled_actor_queue_buffer, &actor);
        }
    }

    return 1;
}

void thorium_worker_pool_work(struct thorium_worker_pool *pool)
{
    struct thorium_message other_message;
    struct thorium_actor *actor;
    int worker_index;
    struct thorium_worker *worker;
    int name;

#ifdef THORIUM_WORKER_POOL_BALANCE
#endif

    /* If there are messages in the inbound message buffer,
     * Try to give  them too.
     */
    if (bsal_fast_queue_dequeue(&pool->inbound_message_queue_buffer, &other_message)) {
        thorium_worker_pool_give_message_to_actor(pool, &other_message);
    }

    /* Try to dequeue an actor for scheduling
     */

    if (bsal_fast_queue_dequeue(&pool->scheduled_actor_queue_buffer, &actor)) {

        name = thorium_actor_name(actor);
        worker_index = thorium_scheduler_get_actor_worker(&pool->scheduler, name);

        if (worker_index < 0) {
            /*
             * This case is very rare, but will happen since the number of
             * messages and the number of actors are both very large.
             *
             * This case will happen if these 3 conditions are satisfied:
             *
             * A message was not queued in the mailbox of the actor X because its mailbox was full.
             * The message, in that case, will be queued in the actor mailbox at a later time.
             * But this code path will not queue the actor on any worker since the actor has yet.
             * But the actor do have messages, but the problem is that all the actor scheduling queue
             * on every worker are full.
             */
#ifdef THORIUM_WORKER_POOL_DEBUG_ACTOR_ASSIGNMENT_PROBLEM
            printf("Notice: actor %d has no assigned worker\n", name);
#endif
            thorium_worker_pool_assign_worker_to_actor(pool, name);
            worker_index = thorium_scheduler_get_actor_worker(&pool->scheduler, name);
        }

        worker = thorium_worker_pool_get_worker(pool, worker_index);

        if (!thorium_worker_enqueue_actor(worker, actor)) {
            bsal_fast_queue_enqueue(&pool->scheduled_actor_queue_buffer, &actor);
        }
    }

#if 0
    printf("DEBUG pool receives message for actor %d\n",
                    destination);
#endif

#ifdef THORIUM_WORKER_POOL_BALANCE
    /* balance the pool regularly
     */


    if (current_time - pool->last_balancing >= pool->balance_period) {
        thorium_scheduler_balance(&pool->scheduler);

        pool->last_balancing = current_time;
    }
#endif

    /*
     * If waiting is enabled, it is required to wake up workers
     * once in a while because of the ordering
     * of events for the actor scheduling queue
     */
    /* Example, thorium_worker_signal is called just a bit before
     * thorium_worker_wait.
     *
     */

    if (pool->waiting_is_enabled) {
        thorium_worker_pool_wake_up_workers(pool);
    }
}

void thorium_worker_pool_assign_worker_to_actor(struct thorium_worker_pool *pool, int name)
{
    int worker_index;

#ifdef THORIUM_WORKER_POOL_USE_LEAST_BUSY
    int score;
#endif

#ifdef THORIUM_WORKER_POOL_USE_SCRIPT_ROUND_ROBIN
    int script;
    struct thorium_actor *actor;
#endif

                /*
    printf("DEBUG Needs to do actor placement\n");
    */
    /* assign this actor to the least busy actor
     */

    worker_index = -1;

#ifdef THORIUM_WORKER_POOL_USE_LEAST_BUSY
    worker_index = thorium_scheduler_select_worker_least_busy(&pool->scheduler, &score);

#elif defined(THORIUM_WORKER_POOL_USE_SCRIPT_ROUND_ROBIN)
    actor = thorium_node_get_actor_from_name(pool->node, name);

    /*
     * Somehow this actor dead a while ago.
     */
    if (actor == NULL) {
        return;
    }

    /* The actor can't be dead if it does not have an initial
     * placement...
     */
    BSAL_DEBUGGER_ASSERT(actor != NULL);

    script = thorium_actor_script(actor);

    worker_index = thorium_scheduler_select_worker_script_round_robin(&pool->scheduler, script);
#endif

    BSAL_DEBUGGER_ASSERT(worker_index >= 0);

#ifdef THORIUM_WORKER_POOL_DEBUG
    printf("ASSIGNING %d to %d\n", name, worker_index);
#endif

    thorium_scheduler_set_actor_worker(&pool->scheduler, name, worker_index);
}

float thorium_worker_pool_get_current_load(struct thorium_worker_pool *pool)
{
    float load;
    int workers;
    int i;

    workers = thorium_worker_pool_worker_count(pool);

    load = 0;
    i = 0;

    while (i < workers) {

        load += thorium_worker_get_epoch_load(thorium_worker_pool_get_worker(pool, i));

        ++i;
    }

    load /= workers;

    return load;
}

int thorium_worker_pool_next_worker(struct thorium_worker_pool *pool, int worker)
{
    worker++;

    /* wrap the counter
     */
    if (worker == pool->workers) {
        worker = 0;
    }

    return worker;
}

struct thorium_worker *thorium_worker_pool_get_worker(
                struct thorium_worker_pool *self, int index)
{
    return self->worker_cache + index;
}

void thorium_worker_pool_set_cached_value(struct thorium_worker_pool *self, int index, int value)
{
    self->message_cache[index] = value;
}

int thorium_worker_pool_get_cached_value(struct thorium_worker_pool *self, int index)
{
    return self->message_cache[index];
}

/* select a worker to pull from */
struct thorium_worker *thorium_worker_pool_select_worker_for_message(struct thorium_worker_pool *pool)
{
    int index;
    int i;
    int score;
    struct thorium_worker *worker;
    int attempts;
    struct thorium_worker *best_worker;
    int best_score;
    int best_index;

    best_index = -1;
    best_score = 0;
    best_worker = NULL;

    i = 0;
    attempts = THORIUM_WORKER_POOL_MESSAGE_SCHEDULING_WINDOW;

    /* select thet worker with the most messages in the window.
     */
    while (i < attempts) {

        index = pool->worker_for_message;
        pool->worker_for_message = thorium_worker_pool_next_worker(pool, index);
        worker = thorium_worker_pool_get_worker(pool, index);
        score = thorium_worker_pool_get_cached_value(pool, index);

        /* Update the cache.
         * This is expensive because it will touch the cache line.
         * Only the worker is increasing the number of messages, and
         * only the worker pool is decreasing it.
         * As long as the cached value is greater than 0, then there is
         * definitely something to pull without the need
         * to break the CPU cache line
         *
         */
        /* always update cache because otherwise there will be
         * starvation
         */
        if (1 || score == 0) {
            score = thorium_worker_get_message_production_score(worker);
            thorium_worker_pool_set_cached_value(pool, index, score);
        }

        if (best_worker == NULL || score > best_score) {
            best_worker = worker;
            best_score = score;
            best_index = index;
        }

        ++i;
    }

    /* Update the cached value for the winning worker to have an
     * accurate value for this worker.
     */
    bsal_vector_set_int(&pool->message_count_cache, best_index, best_score - 1);

    return best_worker;
}

int thorium_worker_pool_pull_classic(struct thorium_worker_pool *pool, struct thorium_message *message)
{
    struct thorium_worker *worker;
    int answer;

    worker = thorium_worker_pool_select_worker_for_message(pool);
    answer = thorium_worker_dequeue_message(worker, message);

    return answer;
}

int thorium_worker_pool_dequeue_message(struct thorium_worker_pool *pool, struct thorium_message *message)
{
    int answer;

    answer = thorium_worker_pool_pull_classic(pool, message);

    return answer;
}

void thorium_worker_pool_wake_up_workers(struct thorium_worker_pool *pool)
{
    float load;
    int i;
    time_t current_time;
    int period;
    struct thorium_worker *worker;
    int elapsed;

    period = 1;

    /*
     * Send a signal to any worker without activity.
     * This is required because a thread can go to sleep after the signal was sent
     * (the first signal).
     */

    i = 0;

    current_time = time(NULL);

    if (current_time - pool->last_signal_check >= period) {
        while (i < pool->workers) {

            worker = thorium_worker_pool_get_worker(pool, i);
            load = thorium_worker_get_epoch_load(worker);
            elapsed = current_time - thorium_worker_get_last_report_time(worker);

            /*
             * Wake up the worker (for instance, worker/8)
             * so that it pulls something.
             */
            if (load < 0.1 || elapsed >= 1) {
                thorium_worker_signal(worker);
            }

            ++i;
        }
        pool->last_signal_check = current_time;
    }
}

int thorium_worker_pool_dequeue_message_for_triage(struct thorium_worker_pool *self,
                struct thorium_message *message)
{
    return bsal_fast_queue_dequeue(&self->messages_for_triage, message);
}
