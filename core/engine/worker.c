
#include "worker.h"

#include "message.h"
#include "node.h"
#include "scheduler.h"

#include <core/structures/map.h>
#include <core/structures/vector.h>
#include <core/structures/vector_iterator.h>
#include <core/structures/map_iterator.h>
#include <core/structures/set_iterator.h>

#include <core/helpers/vector_helper.h>
#include <core/system/memory.h>
#include <core/system/timer.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*#define BSAL_WORKER_DEBUG
  */

#define STATUS_IDLE 0
#define STATUS_QUEUED 1

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
    bsal_ring_queue_init(&worker->scheduled_actor_queue_real, sizeof(struct bsal_actor *));
    bsal_map_init(&worker->actors, sizeof(int), sizeof(int));

    bsal_fast_ring_init(&worker->outbound_message_queue, capacity, sizeof(struct bsal_message));

    bsal_ring_queue_init(&worker->outbound_message_queue_buffer, sizeof(struct bsal_message));

#endif

    worker->debug = 0;
    worker->busy = 0;

    worker->epoch_used_nanoseconds = 0;
    worker->loop_used_nanoseconds = 0;
    worker->scheduling_epoch_used_nanoseconds = 0;

    worker->start = 0;

/* 2 MiB is the default size for Linux huge pages.
 * \see https://wiki.debian.org/Hugepages
 * \see http://lwn.net/Articles/376606/
 */
    bsal_memory_pool_init(&worker->ephemeral_memory, 2097152);
    bsal_memory_pool_disable_tracking(&worker->ephemeral_memory);

    bsal_lock_init(&worker->lock);
    bsal_set_init(&worker->evicted_actors, sizeof(int));
}

void bsal_worker_destroy(struct bsal_worker *worker)
{

    bsal_lock_destroy(&worker->lock);

    bsal_fast_ring_destroy(&worker->scheduled_actor_queue);
    bsal_ring_queue_destroy(&worker->scheduled_actor_queue_real);
    bsal_fast_ring_destroy(&worker->outbound_message_queue);
    bsal_ring_queue_destroy(&worker->outbound_message_queue_buffer);

    bsal_map_destroy(&worker->actors);
    bsal_set_destroy(&worker->evicted_actors);

    worker->node = NULL;

    worker->name = -1;
    worker->dead = 1;

    bsal_memory_pool_destroy(&worker->ephemeral_memory);

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

    worker->last_report = time(NULL);
    worker->epoch_start_in_nanoseconds = bsal_timer_get_nanoseconds();
    worker->loop_start_in_nanoseconds = worker->epoch_start_in_nanoseconds;
    worker->scheduling_epoch_start_in_nanoseconds = worker->epoch_start_in_nanoseconds;
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

    worker->loop_start_in_nanoseconds = bsal_timer_get_nanoseconds();
}

int bsal_worker_is_busy(struct bsal_worker *self)
{
    return self->busy;
}


int bsal_worker_get_scheduled_message_count(struct bsal_worker *self)
{
    int value;
    struct bsal_map_iterator map_iterator;
    int actor_name;
    int messages;
    struct bsal_actor *actor;

    bsal_map_iterator_init(&map_iterator, &self->actors);

    value = 0;
    while (bsal_map_iterator_get_next_key_and_value(&map_iterator, &actor_name, NULL)) {

        actor = bsal_node_get_actor_from_name(self->node, actor_name);

        if (actor == NULL) {
            continue;
        }

        messages = bsal_actor_get_mailbox_size(actor);
        value += messages;
    }

    bsal_map_iterator_destroy(&map_iterator);

    return value;
}

float bsal_worker_get_epoch_load(struct bsal_worker *self)
{
    return self->epoch_load;
}

float bsal_worker_get_loop_load(struct bsal_worker *self)
{
    float loop_load;
    uint64_t elapsed_from_start;
    uint64_t current_nanoseconds;

    current_nanoseconds = self->loop_end_in_nanoseconds;
    elapsed_from_start = current_nanoseconds - self->loop_start_in_nanoseconds;

    loop_load = (0.0 + self->loop_used_nanoseconds) / elapsed_from_start;

    /* Avoid negative zeros
     */
    if (loop_load == 0) {
        loop_load = 0;
    }

    return loop_load;
}

struct bsal_memory_pool *bsal_worker_get_ephemeral_memory(struct bsal_worker *worker)
{
    return &worker->ephemeral_memory;
}

/* This can only be called from the CONSUMER
 */
int bsal_worker_dequeue_actor(struct bsal_worker *worker, struct bsal_actor **actor)
{
    int value;
    int name;
    struct bsal_actor *other_actor;
    int other_name;
    int operations;
    int status;

    operations = 4;

    /* Move an actor from the ring to the real actor scheduling queue
     */
    while (operations--
                    && bsal_fast_ring_pop_from_consumer(&worker->scheduled_actor_queue, &other_actor)) {

        other_name = bsal_actor_name(other_actor);

#ifdef BSAL_WORKER_DEBUG_SCHEDULER
        printf("ring.DEQUEUE %d\n", other_name);
#endif

        if (bsal_set_find(&worker->evicted_actors, &other_name)) {

#ifdef BSAL_WORKER_DEBUG_SCHEDULER
            printf("ALREADY EVICTED\n");
#endif
            continue;
        }

        if (!bsal_map_get_value(&worker->actors, &other_name, &status)) {
            /* Add the actor to the list of actors.
             * This does nothing if it is already in the list.
             */

            status = STATUS_IDLE;
            bsal_map_add_value(&worker->actors, &other_name, &status);
        }

        /* If the actor is not queued, queue it
         */
        if (status == STATUS_IDLE) {
            status = STATUS_QUEUED;

            bsal_map_update_value(&worker->actors, &other_name, &status);

            bsal_ring_queue_enqueue(&worker->scheduled_actor_queue_real, &other_actor);
        } else {

#ifdef BSAL_WORKER_DEBUG_SCHEDULER
            printf("SCHEDULER %d already scheduled to run, scheduled: %d\n", other_name,
                            (int)bsal_set_size(&worker->queued_actors));
#endif
        }
    }

    /* Now, dequeue an actor from the real queue.
     * If it has more than 1 message, re-enqueue it
     */
    value = bsal_ring_queue_dequeue(&worker->scheduled_actor_queue_real, actor);

    if (value) {
        name = bsal_actor_name(*actor);

#ifdef BSAL_WORKER_DEBUG_SCHEDULER
        printf("scheduler.DEQUEUE actor %d, removed from queued actors...\n", name);
#endif

        /* Add the actor to the scheduling queue if it
         * still has messages
         */
        if (bsal_actor_get_mailbox_size(*actor) >= 2) {
#ifdef BSAL_WORKER_DEBUG_SCHEDULER
            printf("Scheduling actor %d again, messages: %d\n",
                        name,
                        bsal_actor_get_mailbox_size(*actor));
#endif

            /* The status is still STATUS_QUEUED
             */
            bsal_ring_queue_enqueue(&worker->scheduled_actor_queue_real, actor);

        } else {
#ifdef BSAL_WORKER_DEBUG_SCHEDULER
            printf("SCHEDULER %d has no message to schedule...\n", name);
#endif
            /* Set the status of the worker to STATUS_IDLE
             */
            status = STATUS_IDLE;
            bsal_map_update_value(&worker->actors, &name, &status);
        }
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

void bsal_worker_print_actors(struct bsal_worker *worker, struct bsal_scheduler *scheduler)
{
    struct bsal_map_iterator iterator;
    int name;
    int count;
    struct bsal_actor *actor;
    int producers;
    int consumers;
    int received;
    int difference;

    bsal_map_iterator_init(&iterator, &worker->actors);

    printf("worker/%d %d queued messages, received: %d busy: %d load: %f ring: %d scheduled actors: %d/%d\n",
                    bsal_worker_name(worker),
                    bsal_worker_get_scheduled_message_count(worker),
                    bsal_worker_get_sum_of_received_actor_messages(worker),
                    bsal_worker_is_busy(worker),
                    bsal_worker_get_scheduling_epoch_load(worker),
                    bsal_fast_ring_size_from_producer(&worker->scheduled_actor_queue),
                    bsal_ring_queue_size(&worker->scheduled_actor_queue_real),
                    (int)bsal_map_size(&worker->actors));

    while (bsal_map_iterator_get_next_key_and_value(&iterator, &name, NULL)) {

        actor = bsal_node_get_actor_from_name(worker->node, name);

        if (actor == NULL) {
            continue;
        }

        count = bsal_actor_get_mailbox_size(actor);
        received = bsal_actor_get_sum_of_received_messages(actor);
        producers = bsal_map_size(bsal_actor_get_received_messages(actor));
        consumers = bsal_map_size(bsal_actor_get_sent_messages(actor));
        difference = bsal_scheduler_get_actor_production(scheduler, actor);

        printf("  [%s/%d] mailbox: %d received: %d (+%d) producers: %d consumers: %d\n",
                        bsal_actor_get_description(actor),
                        name, count, received,
                       difference,
                       producers, consumers);
    }


    /*printf("\n");*/
    bsal_map_iterator_destroy(&iterator);
}

void bsal_worker_evict_actor(struct bsal_worker *worker, int actor_name)
{
    int count;
    struct bsal_actor *actor;
    int name;

    bsal_set_add(&worker->evicted_actors, &actor_name);
    bsal_map_delete(&worker->actors, &actor_name);

    count = bsal_ring_queue_size(&worker->scheduled_actor_queue_real);

    /* evict the actor from the scheduling queue
     */
    while (count--
                    && bsal_ring_queue_dequeue(&worker->scheduled_actor_queue_real, &actor)) {

        name = bsal_actor_name(actor);

        if (name != actor_name) {

            bsal_ring_queue_enqueue(&worker->scheduled_actor_queue_real,
                            &actor);
        }
    }

    /* Evict the actor from the ring
     */

    count = bsal_fast_ring_size_from_consumer(&worker->scheduled_actor_queue);

    while (count-- && bsal_fast_ring_pop_from_consumer(&worker->scheduled_actor_queue,
                            &actor)) {

        name = bsal_actor_name(actor);

        if (name != actor_name) {

            bsal_fast_ring_push_from_producer(&worker->scheduled_actor_queue,
                            &actor);
        }
    }
}

void bsal_worker_lock(struct bsal_worker *worker)
{
    bsal_lock_lock(&worker->lock);
}

void bsal_worker_unlock(struct bsal_worker *worker)
{
    bsal_lock_unlock(&worker->lock);
}

struct bsal_map *bsal_worker_get_actors(struct bsal_worker *worker)
{
    return &worker->actors;
}

int bsal_worker_enqueue_actor_special(struct bsal_worker *worker, struct bsal_actor **actor)
{
    int name;

    name = bsal_actor_name(*actor);

    bsal_set_delete(&worker->evicted_actors, &name);

    return bsal_worker_enqueue_actor(worker, actor);
}

int bsal_worker_get_sum_of_received_actor_messages(struct bsal_worker *self)
{
    int value;
    struct bsal_map_iterator iterator;
    int actor_name;
    int messages;
    struct bsal_actor *actor;

    bsal_map_iterator_init(&iterator, &self->actors);

    value = 0;
    while (bsal_map_iterator_get_next_key_and_value(&iterator, &actor_name, NULL)) {

        actor = bsal_node_get_actor_from_name(self->node, actor_name);

        if (actor == NULL) {
            continue;
        }

        messages = bsal_actor_get_sum_of_received_messages(actor);

        value += messages;
    }

    bsal_map_iterator_destroy(&iterator);

    return value;
}

int bsal_worker_get_queued_messages(struct bsal_worker *self)
{
    int value;
    struct bsal_map_iterator map_iterator;
    int actor_name;
    int messages;
    struct bsal_actor *actor;

    bsal_map_iterator_init(&map_iterator, &self->actors);

    value = 0;
    while (bsal_map_iterator_get_next_key_and_value(&map_iterator, &actor_name, NULL)) {

        actor = bsal_node_get_actor_from_name(self->node, actor_name);

        if (actor == NULL) {
            continue;
        }

        messages = bsal_actor_get_mailbox_size(actor);

        value += messages;
    }

    bsal_map_iterator_destroy(&map_iterator);

    return value;

}

float bsal_worker_get_scheduling_epoch_load(struct bsal_worker *worker)
{
    uint64_t end_time;
    uint64_t period;

    end_time = bsal_timer_get_nanoseconds();

    period = end_time - worker->scheduling_epoch_start_in_nanoseconds;

    if (period == 0) {
        return 0;
    }

    return (0.0 + worker->scheduling_epoch_used_nanoseconds) / period;
}

void bsal_worker_reset_scheduling_epoch(struct bsal_worker *worker)
{
    worker->scheduling_epoch_start_in_nanoseconds = bsal_timer_get_nanoseconds();

    worker->scheduling_epoch_used_nanoseconds = 0;
}

int bsal_worker_get_production(struct bsal_worker *worker, struct bsal_scheduler *scheduler)
{
    struct bsal_map_iterator iterator;
    int name;
    struct bsal_actor *actor;
    int production;

    production = 0;
    bsal_map_iterator_init(&iterator, &worker->actors);

    while (bsal_map_iterator_get_next_key_and_value(&iterator, &name, NULL)) {

        actor = bsal_node_get_actor_from_name(worker->node, name);

        if (actor == NULL) {
            continue;
        }

        production += bsal_scheduler_get_actor_production(scheduler, actor);

    }

    bsal_map_iterator_destroy(&iterator);

    return production;
}

int bsal_worker_get_producer_count(struct bsal_worker *worker, struct bsal_scheduler *scheduler)
{
    struct bsal_map_iterator iterator;
    int name;
    struct bsal_actor *actor;
    int count;

    count = 0;
    bsal_map_iterator_init(&iterator, &worker->actors);

    while (bsal_map_iterator_get_next_key_and_value(&iterator, &name, NULL)) {

        actor = bsal_node_get_actor_from_name(worker->node, name);

        if (actor == NULL) {
            continue;
        }

        if (bsal_scheduler_get_actor_production(scheduler, actor) > 0) {
            ++count;
        }

    }

    bsal_map_iterator_destroy(&iterator);
    return count;
}


