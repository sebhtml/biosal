
#include "worker.h"

#include "work.h"
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

/*
#define BSAL_WORKER_HAS_OWN_QUEUES_AND_PUSH_LOCALLY
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

#ifdef BSAL_WORKER_USE_FAST_RINGS
    /* There are two options:
     * 1. enable atomic operations for change visibility
     * 2. Use volatile head and tail.
     */
    bsal_fast_ring_init(&worker->work_queue, capacity, sizeof(struct bsal_work));
    /*bsal_ring_enable_atomicity(&worker->work_queue);*/

    bsal_fast_ring_init(&worker->message_queue, capacity, sizeof(struct bsal_message));
    /*bsal_ring_enable_atomicity(&worker->message_queue);*/
#else

    bsal_ring_init(&worker->work_queue, capacity, sizeof(struct bsal_work));
    bsal_ring_init(&worker->message_queue, capacity, sizeof(struct bsal_message));
#endif

    bsal_ring_queue_init(&worker->local_message_queue, sizeof(struct bsal_message));
    bsal_ring_queue_init(&worker->local_work_queue, sizeof(struct bsal_work));

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
#ifdef BSAL_WORKER_HAS_OWN_QUEUES

#ifdef BSAL_WORKER_USE_FAST_RINGS
    bsal_fast_ring_destroy(&worker->work_queue);
    bsal_fast_ring_destroy(&worker->message_queue);
#else
    bsal_ring_destroy(&worker->work_queue);
    bsal_ring_destroy(&worker->message_queue);

#endif
    bsal_ring_queue_destroy(&worker->local_message_queue);
    bsal_ring_queue_destroy(&worker->local_work_queue);
#endif

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

#ifdef BSAL_WORKER_HAS_OWN_QUEUES_AND_PUSH_LOCALLY
    int maximum;
    struct bsal_message *new_message;
    int destination;
    struct bsal_work work;
    struct bsal_actor *actor;
    int works;
    int push_locally;
#endif

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

#ifdef BSAL_WORKER_HAS_OWN_QUEUES_AND_PUSH_LOCALLY
    destination = bsal_message_destination(message);
    push_locally = 0;
    maximum = 0;

    if (push_locally && bsal_node_has_actor(worker->node, destination)) {

        /*
         * TODO maybe it would be better to check the
         * scheduling score of the current worker.
         */
        actor = bsal_node_get_actor_from_name(worker->node, destination);

        new_message = (struct bsal_message *)bsal_memory_allocate(sizeof(struct bsal_message));
        memcpy(new_message, &copy, sizeof(struct bsal_message));

        /*
        printf("Local stuff: %p %p\n", (void *)actor, (void *)new_message);
        */

        bsal_work_init(&work, actor, new_message);

        works = bsal_ring_size(worker->wor1k_queue);

#ifdef BSAL_WORKER_DEBUG_SCHEDULING
        if (works > 0) {
            printf("NOTICE queuing work in queue with %d works\n",
                    works);
        }
#endif

        if (works <= maximum) {
            bsal_worker_push_work(worker, &work);
        } else {
#ifdef BSAL_WORKER_DEBUG_SCHEDULING
            printf("DEBUG WORKER LOCAL PUSH %d\n", worker->start);
#endif
            bsal_worker_pool_schedule_work(bsal_node_get_worker_pool(worker->node), &work,
                            &worker->start);
        }

    } else {
        /* Otherwise, the message will be sent
         * on the network.
         */
        bsal_worker_push_message(worker, &copy);
    }
#else

    bsal_worker_push_message(worker, &copy);
#endif
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

#ifdef BSAL_WORKER_HAS_OWN_QUEUES
int bsal_worker_push_work(struct bsal_worker *worker, struct bsal_work *work)
{
#ifdef BSAL_WORKER_DEBUG
    struct bsal_message *message;
    int tag;
    int destination;

    message = bsal_work_message(work);
    tag = bsal_message_tag(message);
    destination = bsal_message_destination(message);

    if (tag == BSAL_ACTOR_ASK_TO_STOP) {
        printf("DEBUG worker/%d before queuing work BSAL_ACTOR_ASK_TO_STOP for actor %d, %d works\n",
                        worker->name,
                        destination, bsal_work_queue_size(worker->1work_queue));
    }
#endif

#ifdef BSAL_WORKER_USE_FAST_RINGS
    return bsal_fast_ring_push_from_producer(&worker->work_queue, work);
#else
    return bsal_ring_push(&worker->work_queue, work);

#endif

#ifdef BSAL_WORKER_DEBUG
    if (tag == BSAL_ACTOR_ASK_TO_STOP) {
        printf("DEBUG worker/%d after queuing work BSAL_ACTOR_ASK_TO_STOP for actor %d, %d works\n",
                        worker->name,
                        destination, bsal_work_queue_size(worker->work_queue));
    }
#endif
}
#endif

void bsal_worker_push_message(struct bsal_worker *worker, struct bsal_message *message)
{
    /* if the message can not be queued in the ring,
     * queue it in the queue
     */

#ifdef BSAL_WORKER_USE_FAST_RINGS
    if (!bsal_fast_ring_push_from_producer(&worker->message_queue, message)) {

        printf("Warning: CONTENTION worker %d on node %d buffered a message\n",
                        bsal_worker_name(worker),
                        bsal_node_name(worker->node));

        bsal_ring_queue_enqueue(&worker->local_message_queue, message);
    }
#else
    /*This code is not enabled because fast rings are used.
     */
    if (!bsal_ring_push(&worker->message_queue, message)) {
        bsal_ring_queue_enqueue(&worker->local_message_queue, message);
    }

#endif
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

#ifdef BSAL_WORKER_USE_FAST_RINGS
    score += bsal_fast_ring_size_from_consumer(&self->work_queue);
#else
    score += bsal_ring_size(&self->work_queue);
#endif

    score += bsal_ring_queue_size(&self->local_work_queue);

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

int bsal_worker_enqueue_work(struct bsal_worker *worker, struct bsal_work *work)
{
    int value;

#ifdef BSAL_WORKER_EVICTION
    bsal_lock_lock(&worker->eviction_lock);
#endif
    value = bsal_ring_queue_enqueue(&worker->local_work_queue, work);
#ifdef BSAL_WORKER_EVICTION
    bsal_lock_unlock(&worker->eviction_lock);
#endif

    return value;
}

int bsal_worker_dequeue_work(struct bsal_worker *worker, struct bsal_work *work)
{
    int value;

#ifdef BSAL_WORKER_EVICTION
    bsal_lock_lock(&worker->eviction_lock);
#endif
    value = bsal_ring_queue_dequeue(&worker->local_work_queue, work);
#ifdef BSAL_WORKER_EVICTION
    bsal_lock_unlock(&worker->eviction_lock);
#endif

    return value;
}
