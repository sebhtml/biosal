
#include "worker.h"

#include "message.h"
#include "node.h"

#include <core/structures/map.h>
#include <core/structures/vector.h>
#include <core/structures/vector_iterator.h>
#include <core/structures/map_iterator.h>

#include <core/helpers/vector_helper.h>
#include <core/system/memory.h>
#include <core/system/timer.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*#define BSAL_WORKER_DEBUG
  */

void bsal_worker_init(struct bsal_worker *worker, int name, struct bsal_node *node)
{
    int capacity;

    capacity = BSAL_WORKER_RING_CAPACITY;
    /*worker->work_queue = work_queue;*/
    worker->node = node;
    worker->name = name;
    worker->dead = 0;
    worker->last_warning = 0;

#ifdef BSAL_WORKER_HAS_OWN_QUEUES
    /*worker->work_queue = &worker->works;*/

    /* There are two options:
     * 1. enable atomic operations for change visibility
     * 2. Use volatile head and tail.
     */
    bsal_fast_ring_init(&worker->scheduled_actor_queue, capacity, sizeof(struct bsal_actor *));
    bsal_ring_queue_init(&worker->scheduled_actor_queue_real, sizeof(struct bsal_message));
    bsal_map_init(&worker->actors, sizeof(int), sizeof(int));

    bsal_fast_ring_init(&worker->outbound_message_queue, capacity, sizeof(struct bsal_message));

    bsal_ring_queue_init(&worker->outbound_message_queue_buffer, sizeof(struct bsal_message));

#endif

    worker->debug = 0;
    worker->busy = 0;

    worker->last_report = time(NULL);

    worker->epoch_start_in_nanoseconds = bsal_timer_get_nanoseconds();
    worker->epoch_used_nanoseconds = 0;
    worker->epoch_load = 0;

    worker->loop_start_in_nanoseconds = worker->epoch_start_in_nanoseconds;
    worker->loop_used_nanoseconds = 0;
    worker->loop_load = 0;
    worker->start = 0;

/* 2 MiB is the default size for Linux huge pages.
 * \see https://wiki.debian.org/Hugepages
 * \see http://lwn.net/Articles/376606/
 */
    bsal_memory_pool_init(&worker->ephemeral_memory, 2097152);
    bsal_memory_pool_disable_tracking(&worker->ephemeral_memory);

#ifdef BSAL_WORKER_EVICTION
    bsal_lock_init(&worker->eviction_lock);
    bsal_set_init(&worker->actors_to_evict, sizeof(int));
#endif
}

void bsal_worker_destroy(struct bsal_worker *worker)
{

    bsal_fast_ring_destroy(&worker->scheduled_actor_queue);
    bsal_ring_queue_destroy(&worker->scheduled_actor_queue_real);
    bsal_fast_ring_destroy(&worker->outbound_message_queue);
    bsal_ring_queue_destroy(&worker->outbound_message_queue_buffer);

    bsal_map_destroy(&worker->actors);

    worker->node = NULL;

    worker->name = -1;
    worker->dead = 1;

    bsal_memory_pool_destroy(&worker->ephemeral_memory);

#ifdef BSAL_WORKER_EVICTION
    bsal_lock_destroy(&worker->eviction_lock);
    bsal_set_destroy(&worker->actors_to_evict);
#endif
}

struct bsal_node *bsal_worker_node(struct bsal_worker *worker)
{
    return worker->node;
}

void bsal_worker_send(struct bsal_worker *worker, struct bsal_message *message)
{
    struct bsal_message copy;
    void *buffer;
    int count;
    int metadata_size;
    int all;
    void *old_buffer;

    memcpy(&copy, message, sizeof(struct bsal_message));
    count = bsal_message_count(&copy);
    metadata_size = bsal_message_metadata_size(message);
    all = count + metadata_size;

    /* TODO use slab allocator to allocate buffer... */
    buffer = (char *)bsal_memory_allocate(all * sizeof(char));

#ifdef BSAL_WORKER_DEBUG
    printf("[bsal_worker_send] allocated %i bytes (%i + %i) for buffer %p\n",
                    all, count, metadata_size, buffer);

    printf("bsal_worker_send old buffer: %p\n",
                    bsal_message_buffer(message));
#endif

    /* according to
     * http://stackoverflow.com/questions/3751797/can-i-call-memcpy-and-memmove-with-number-of-bytes-set-to-zero
     * memcpy works with a count of 0, but the addresses must be valid
     * nonetheless
     *
     * Copy the message data.
     */
    if (count > 0) {
        old_buffer = bsal_message_buffer(message);
        memcpy(buffer, old_buffer, count);

        /* TODO use slab allocator */
    }

    bsal_message_set_buffer(&copy, buffer);
    bsal_message_write_metadata(&copy);

#ifdef BSAL_WORKER_DEBUG_20140601
    if (bsal_message_tag(message) == 1100) {
        printf("DEBUG bsal_worker_send 1100\n");
    }
#endif

    /* if the destination is on the same node,
     * handle that directly here to avoid locking things
     * with the node.
     */

    bsal_worker_enqueue_message(worker, &copy);
}

void bsal_worker_start(struct bsal_worker *worker, int processor)
{
    bsal_thread_init(&worker->thread, bsal_worker_main, worker);
    bsal_thread_set_affinity(&worker->thread, processor);
    bsal_thread_start(&worker->thread);
}

void *bsal_worker_main(void *worker1)
{
    struct bsal_worker *worker;

    worker = (struct bsal_worker*)worker1;

#ifdef BSAL_WORKER_DEBUG
    bsal_worker_display(worker);
    printf("Starting worker thread\n");
#endif

    while (!worker->dead) {

        bsal_worker_run(worker);
    }

    return NULL;
}

void bsal_worker_display(struct bsal_worker *worker)
{
    printf("[bsal_worker_main] node %i worker %i\n",
                    bsal_node_name(worker->node),
                    bsal_worker_name(worker));
}

int bsal_worker_name(struct bsal_worker *worker)
{
    return worker->name;
}

void bsal_worker_stop(struct bsal_worker *worker)
{
#ifdef BSAL_WORKER_DEBUG
    bsal_worker_display(worker);
    printf("stopping worker!\n");
#endif

    /*
     * worker->dead is changed and will be read
     * by the running thread.
     */
    worker->dead = 1;

    bsal_thread_join(&worker->thread);
}

int bsal_worker_is_busy(struct bsal_worker *self)
{
    return self->busy;
}

#ifdef BSAL_WORKER_HAS_OWN_QUEUES

int bsal_worker_get_work_scheduling_score(struct bsal_worker *self)
{
    int score;

    score = 0;

    if (bsal_worker_is_busy(self)) {
        score++;
    }

    score += bsal_fast_ring_size_from_consumer(&self->scheduled_actor_queue);

    return score;
}
#endif

float bsal_worker_get_epoch_load(struct bsal_worker *self)
{
    return self->epoch_load;
}

float bsal_worker_get_loop_load(struct bsal_worker *self)
{
    return self->loop_load;
}

struct bsal_memory_pool *bsal_worker_get_ephemeral_memory(struct bsal_worker *worker)
{
    return &worker->ephemeral_memory;
}

#ifdef BSAL_WORKER_EVICTION
void bsal_worker_evict_actor(struct bsal_worker *worker, int actor_name)
{
    bsal_lock_lock(&worker->eviction_lock);

    bsal_set_add(&worker->actors_to_evict, &actor_name);

#if 0
    printf("EVICTION will evict %d\n", actor_name);
#endif

    bsal_lock_unlock(&worker->eviction_lock);
}

void bsal_worker_evict_actors(struct bsal_worker *worker)
{
    struct bsal_message *message;
    struct bsal_actor *actor;
    struct bsal_work work;
    int name;
    int local_works;

    bsal_lock_lock(&worker->eviction_lock);

    local_works = bsal_ring_queue_size(&worker->local_work_queue);

    printf("EVICTION worker %d evicts actors, %d actors to evict, %d local works\n",
                    worker->name, (int)bsal_set_size(&worker->actors_to_evict),
                    local_works);

    /* evict the actor from the fast ring for works
     */
    while (bsal_fast_ring_pop_from_consumer(&worker->work_queue, &work)) {
        actor = bsal_work_actor(&work);
        name = bsal_actor_name(actor);

        /* This message will be returned to the source.
         */
        if (bsal_set_find(&worker->actors_to_evict, &name)) {
            message = bsal_work_message(&work);
            bsal_worker_push_message(worker, message);

            printf("EVICTION %d was evicted from worker %d FAST RING\n",
                            name, bsal_worker_name(worker));
        } else {

            /* Otherwise, it is kept.
             */
            bsal_worker_enqueue_work(worker, &work);
        }
    }

    /* evict the actor from the local work queue
     */
    while (local_works-- && bsal_worker_dequeue_work(worker, &work)) {

        actor = bsal_work_actor(&work);
        name = bsal_actor_name(actor);

        /* This message will be returned to the source.
         */
        if (bsal_set_find(&worker->actors_to_evict, &name)) {
            message = bsal_work_message(&work);
            bsal_worker_push_message(worker, message);

            printf("EVICTION %d was evicted from worker %d LOCAL WORK QUEUE\n",
                            name, bsal_worker_name(worker));
        } else {

            /* Otherwise, it is kept.
             */
            bsal_worker_enqueue_work(worker, &work);
        }
    }

    bsal_set_destroy(&worker->actors_to_evict);
    bsal_set_init(&worker->actors_to_evict, sizeof(int));

    bsal_lock_unlock(&worker->eviction_lock);
}
#endif

/* This can only be called from the CONSUMER
 */
int bsal_worker_dequeue_actor(struct bsal_worker *worker, struct bsal_actor **actor)
{
    int value;
    int name;
    int count;
    struct bsal_actor *other_actor;
    int other_name;
    int operations;

    operations = 4;

    /* Move an actor from the ring to the real actor scheduling queue
     */
    while (operations--
                    && bsal_fast_ring_pop_from_consumer(&worker->scheduled_actor_queue, &other_actor)) {

        other_name = bsal_actor_name(other_actor);

        /* This is the first time this actor is
         * provided
         */
        if (!bsal_map_get_value(&worker->actors, &other_name, &count)) {

            bsal_ring_queue_enqueue(&worker->scheduled_actor_queue_real, &other_actor);

            count = 1;
            bsal_map_add_value(&worker->actors, &other_name, &count);

#ifdef BSAL_WORKER_DEBUG_ACTOR_SCHEDULING_QUEUE
            printf("ENQUEUE ACTOR %d MESSAGES %d (first)\n", other_name, count);
#endif

        /* This actor was seen before, but it is not scheduled
         * to run
         */
        } else if (count == 0) {

#ifdef BSAL_WORKER_DEBUG_ACTOR_SCHEDULING_QUEUE
            printf("ENQUEUE ACTOR %d MESSAGES %d + 1 (adding to scheduling queue)\n", other_name, count);
#endif

            bsal_ring_queue_enqueue(&worker->scheduled_actor_queue_real, &other_actor);
            ++count;
            bsal_map_update_value(&worker->actors, &other_name, &count);


        /* Otherwise, this actor is already in the scheduling queue.
         * Just update the scheduling value.
         */
        } else {

#ifdef BSAL_WORKER_DEBUG_ACTOR_SCHEDULING_QUEUE
            printf("ENQUEUE ACTOR %d MESSAGES %d + 1 (already in scheduling queue)\n", other_name, count);
#endif

            ++count;
            bsal_map_update_value(&worker->actors, &other_name, &count);

        }
    }

    /* Now, dequeue an actor from the real queue.
     * If it has more than 1 message, re-enqueue it
     */
    value = bsal_ring_queue_dequeue(&worker->scheduled_actor_queue_real, actor);

    if (value) {

        /*
         * Decrement the number of messages
         */
        name = bsal_actor_name(*actor);
        bsal_map_get_value(&worker->actors, &name, &count);

#ifdef BSAL_WORKER_DEBUG_ACTOR_SCHEDULING_QUEUE
        printf("DEQUEUE ACTOR %d MESSAGES %d - 1\n", name, count);
#endif

        --count;
        bsal_map_update_value(&worker->actors, &name, &count);

        /* Enqueue it again if it has one message or more
         */
        if (count > 0) {
            bsal_ring_queue_enqueue(&worker->scheduled_actor_queue_real, actor);
        }

#if 0
        if (count < 0) {
            printf("Error: count is %d for actor %d (DEQUEUE)\n", count, name);
        }
#endif
    }

    return value;
}

/* This can only be called from the PRODUCER
 */
int bsal_worker_enqueue_actor(struct bsal_worker *worker, struct bsal_actor **actor)
{
    return bsal_fast_ring_push_from_producer(&worker->scheduled_actor_queue, actor);
}

int bsal_worker_enqueue_message(struct bsal_worker *worker, struct bsal_message *message)
{

    /* Try to push the message in the output ring
     */
    if (!bsal_fast_ring_push_from_producer(&worker->outbound_message_queue, message)) {

        /* If that does not work, push the message in the queue buffer.
         */
        bsal_ring_queue_enqueue(&worker->outbound_message_queue_buffer, message);

    }

    return 1;
}

int bsal_worker_dequeue_message(struct bsal_worker *worker, struct bsal_message *message)
{
    return bsal_fast_ring_pop_from_consumer(&worker->outbound_message_queue, message);
}

void bsal_worker_print_actors(struct bsal_worker *worker)
{
    struct bsal_map_iterator iterator;
    int name;
    int count;

    bsal_map_iterator_init(&iterator, &worker->actors);

    printf("INFO worker: %d, busy: %d, actors in ring: %d scheduled actors: %d\n",
                    bsal_worker_name(worker),
                    bsal_worker_is_busy(worker),
                    bsal_fast_ring_size_from_producer(&worker->scheduled_actor_queue),
                    bsal_ring_queue_size(&worker->scheduled_actor_queue_real));

    while (bsal_map_iterator_get_next_key_and_value(&iterator, &name, &count)) {
        printf("  ACTOR %d  MESSAGES %d\n", name, count);
    }
    bsal_map_iterator_destroy(&iterator);
}
