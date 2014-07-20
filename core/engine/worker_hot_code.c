
#include "worker.h"

#include "message.h"
#include "node.h"

#include <core/structures/map_iterator.h>
#include <core/structures/vector_iterator.h>

#include <core/helpers/vector_helper.h>

#include <core/system/timer.h>

#include <stdio.h>

/*
 * Print scheduling queue for debugging purposes
 */
#define BSAL_WORKER_PRINT_SCHEDULING_QUEUE

#define BSAL_WORKER_DEBUG_SYMMETRIC_PLACEMENT

/* Just return the number of queued messages.
 */
int bsal_worker_get_message_production_score(struct bsal_worker *self)
{
    int score;

    score = 0;

    score += bsal_fast_ring_size_from_producer(&self->outbound_message_queue);

    score += bsal_ring_queue_size(&self->outbound_message_queue_buffer);

    return score;
}

void bsal_worker_run(struct bsal_worker *worker)
{
    struct bsal_actor *actor;
    struct bsal_message other_message;

#ifdef BSAL_NODE_USE_MESSAGE_RECYCLING
    void *buffer;
#endif

#ifdef BSAL_NODE_ENABLE_INSTRUMENTATION
    clock_t current_time;
    clock_t elapsed;
    int period;
    uint64_t current_nanoseconds;
    uint64_t start_time;
    uint64_t end_time;
    uint64_t elapsed_nanoseconds;
#endif

#ifdef BSAL_WORKER_DEBUG
    int tag;
    int destination;
    struct bsal_message *message;
#endif

    bsal_worker_lock(worker);

#ifdef BSAL_NODE_ENABLE_INSTRUMENTATION
    period = BSAL_NODE_LOAD_PERIOD;
    current_time = time(NULL);

    elapsed = current_time - worker->last_report;

    if (elapsed >= period) {

        current_nanoseconds = bsal_timer_get_nanoseconds();

#ifdef BSAL_WORKER_DEBUG_LOAD
        printf("DEBUG Updating load report\n");
#endif
        elapsed_nanoseconds = current_nanoseconds - worker->epoch_start_in_nanoseconds;

        if (elapsed_nanoseconds > 0) {
            worker->epoch_load = (0.0 + worker->epoch_used_nanoseconds) / elapsed_nanoseconds;
            worker->epoch_used_nanoseconds = 0;

            /* \see http://stackoverflow.com/questions/9657993/negative-zero-in-c
             */
            if (worker->epoch_load == 0) {
                worker->epoch_load = 0;
            }

            worker->epoch_start_in_nanoseconds = current_nanoseconds;
            worker->last_report = current_time;
        }

#ifdef BSAL_WORKER_PRINT_SCHEDULING_QUEUE

        /*
        if (bsal_node_name(worker->node) == 0
                        && worker->name == 0) {
                        */

        bsal_scheduling_queue_print(&worker->scheduling_queue,
                        bsal_node_name(worker->node),
                        worker->name);
        /*
        }
        */
#endif

#ifdef BSAL_WORKER_DEBUG_SYMMETRIC_PLACEMENT
        bsal_worker_print_actors(worker, NULL);
#endif
    }
#endif

#ifdef BSAL_WORKER_DEBUG
    if (worker->debug) {
        printf("DEBUG worker/%d bsal_worker_run\n",
                        bsal_worker_name(worker));
    }
#endif

    /* check for messages in inbound FIFO */
    if (bsal_worker_dequeue_actor(worker, &actor)) {

#ifdef BSAL_WORKER_DEBUG
        message = bsal_work_message(&work);
        tag = bsal_message_tag(message);
        destination = bsal_message_destination(message);

        if (tag == BSAL_ACTOR_ASK_TO_STOP) {
            printf("DEBUG pulled BSAL_ACTOR_ASK_TO_STOP for %d\n",
                            destination);
        }
#endif

        /* Update the priority of the actor
         * before starting the timer because this is part of the
         * runtime system (RTS).
         */
        bsal_priority_scheduler_update(&worker->scheduler, actor);

#ifdef BSAL_NODE_ENABLE_INSTRUMENTATION
        start_time = bsal_timer_get_nanoseconds();
#endif

        worker->busy = 1;
        /* dispatch message to a worker */
        bsal_worker_work(worker, actor);

        worker->busy = 0;

#ifdef BSAL_NODE_ENABLE_INSTRUMENTATION
        end_time = bsal_timer_get_nanoseconds();

        elapsed_nanoseconds = end_time - start_time;

        worker->epoch_used_nanoseconds += elapsed_nanoseconds;
        worker->loop_used_nanoseconds += elapsed_nanoseconds;
        worker->scheduling_epoch_used_nanoseconds += elapsed_nanoseconds;
#endif
    }

    /* queue buffered message
     */
    if (bsal_ring_queue_dequeue(&worker->outbound_message_queue_buffer, &other_message)) {

        if (!bsal_fast_ring_push_from_producer(&worker->outbound_message_queue, &other_message)) {

            bsal_ring_queue_enqueue(&worker->outbound_message_queue_buffer, &other_message);
        }
    }

#ifdef BSAL_NODE_USE_MESSAGE_RECYCLING
    /* free outbound buffers, if any
     */

    if (bsal_fast_ring_pop_from_consumer(&worker->outbound_buffers, &buffer)) {

        bsal_memory_pool_free(&worker->outbound_message_memory_pool, buffer);
    }
#endif

    bsal_worker_unlock(worker);
}

void bsal_worker_work(struct bsal_worker *worker, struct bsal_actor *actor)
{
    int dead;
    int actor_name;

#ifdef BSAL_WORKER_DEBUG
    int tag;
    int destination;
#endif

    actor_name = bsal_actor_get_name(actor);

#ifdef BSAL_WORKER_DEBUG_SCHEDULER
    printf("WORK actor %d\n", actor_name);
#endif

    /* the actor died while the work was queued.
     */
    if (bsal_actor_dead(actor)) {

        printf("NOTICE actor is dead already (bsal_worker_work)\n");

        return;
    }

    /* lock the actor to prevent another worker from making work
     * on the same actor at the same time
     */
    if (bsal_actor_trylock(actor) != BSAL_LOCK_SUCCESS) {

        printf("Warning: CONTENTION worker %d could not lock actor %d, returning the message...\n",
                        bsal_worker_name(worker),
                        actor_name);

        return;
    }

    /* the actor died while this worker was waiting for the lock
     */
    if (bsal_actor_dead(actor)) {

        printf("DEBUG bsal_worker_work actor died while the worker was waiting for the lock.\n");
#ifdef BSAL_WORKER_DEBUG
#endif
        /*bsal_actor_unlock(actor);*/
        return;
    }

    /* call the actor receive code
     */
    bsal_actor_set_worker(actor, worker);

    bsal_actor_work(actor);

    bsal_actor_set_worker(actor, NULL);

    /* Free ephemeral memory
     */
    bsal_memory_pool_free_all(&worker->ephemeral_memory);

    dead = bsal_actor_dead(actor);

    if (dead) {

        bsal_map_delete(&worker->actors, &actor_name);

        bsal_node_notify_death(worker->node, actor);
    }

#ifdef BSAL_WORKER_DEBUG_20140601
    if (worker->debug) {
        printf("DEBUG worker/%d after dead call\n",
                        bsal_worker_name(worker));
    }
#endif

    /* Unlock the actor.
     * This does not do anything if a death notification
     * was sent to the node
     */
    bsal_actor_unlock(actor);

#ifdef BSAL_WORKER_DEBUG
    printf("bsal_worker_work Freeing buffer %p %i tag %i\n",
                    buffer, bsal_message_count(message),
                    bsal_message_tag(message));
#endif

#ifdef BSAL_WORKER_DEBUG_20140601
    if (worker->debug) {
        printf("DEBUG worker/%d exiting bsal_worker_work\n",
                        bsal_worker_name(worker));
    }
#endif
}

