
#include "worker.h"

#include "work.h"
#include "message.h"
#include "node.h"

#include <structures/map.h>
#include <structures/vector.h>
#include <structures/vector_iterator.h>
#include <structures/map_iterator.h>

#include <helpers/vector_helper.h>
#include <system/memory.h>
#include <system/timer.h>

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
}

void bsal_worker_run(struct bsal_worker *worker)
{
    struct bsal_work work;
    struct bsal_message other_message;

#ifdef BSAL_NODE_ENABLE_LOAD_REPORTING
    clock_t current_time;
    clock_t elapsed;
    int period;
    uint64_t current_nanoseconds;
    uint64_t start_time;
    uint64_t end_time;
    uint64_t elapsed_nanoseconds;
    uint64_t elapsed_from_start;
#endif

#ifdef BSAL_WORKER_DEBUG
    int tag;
    int destination;
    struct bsal_message *message;
#endif

#ifdef BSAL_NODE_ENABLE_LOAD_REPORTING
    period = 1;
    current_time = time(NULL);

    elapsed = current_time - worker->last_report;

    if (elapsed >= period) {

        current_nanoseconds = bsal_timer_get_nanoseconds();

#ifdef BSAL_WORKER_DEBUG_LOAD
        printf("DEBUG Updating load report\n");
#endif
        elapsed_nanoseconds = current_nanoseconds - worker->epoch_start_in_nanoseconds;
        elapsed_from_start = current_nanoseconds - worker->loop_start_in_nanoseconds;

        if (elapsed_nanoseconds > 0) {
            worker->epoch_load = (0.0 + worker->epoch_used_nanoseconds) / elapsed_nanoseconds;
            worker->loop_load = (0.0 + worker->loop_used_nanoseconds) / elapsed_from_start;

            /* \see http://stackoverflow.com/questions/9657993/negative-zero-in-c
             */
            if (worker->epoch_load == 0) {
                worker->epoch_load = 0;
            }
            if (worker->loop_load == 0) {
                worker->loop_load = 0;
            }
            worker->epoch_used_nanoseconds = 0;
            worker->epoch_start_in_nanoseconds = current_nanoseconds;
            worker->last_report = current_time;
        }
    }
#endif

#ifdef BSAL_WORKER_DEBUG
    if (worker->debug) {
        printf("DEBUG worker/%d bsal_worker_run\n",
                        bsal_worker_name(worker));
    }
#endif

    /* check for messages in inbound FIFO */
    if (bsal_worker_pull_work(worker, &work)) {

#ifdef BSAL_WORKER_DEBUG
        message = bsal_work_message(&work);
        tag = bsal_message_tag(message);
        destination = bsal_message_destination(message);

        if (tag == BSAL_ACTOR_ASK_TO_STOP) {
            printf("DEBUG pulled BSAL_ACTOR_ASK_TO_STOP for %d\n",
                            destination);
        }
#endif

#ifdef BSAL_NODE_ENABLE_LOAD_REPORTING
        start_time = bsal_timer_get_nanoseconds();
#endif

        /* dispatch message to a worker */
        bsal_worker_work(worker, &work);

#ifdef BSAL_NODE_ENABLE_LOAD_REPORTING
        end_time = bsal_timer_get_nanoseconds();

        elapsed_nanoseconds = end_time - start_time;
        worker->epoch_used_nanoseconds += elapsed_nanoseconds;
        worker->loop_used_nanoseconds += elapsed_nanoseconds;
#endif
    }

    /* queue buffered message
     */
    if (bsal_ring_queue_dequeue(&worker->local_message_queue, &other_message)) {

        bsal_worker_push_message(worker, &other_message);
    }
}

void bsal_worker_work(struct bsal_worker *worker, struct bsal_work *work)
{
    struct bsal_actor *actor;
    struct bsal_message *message;
    int dead;
    char *buffer;

#ifdef BSAL_WORKER_DEBUG
    int tag;
    int destination;
#endif

    actor = bsal_work_actor(work);
    message = bsal_work_message(work);

#ifdef BSAL_WORKER_DEBUG
    tag = bsal_message_tag(message);
    destination = bsal_message_destination(message);

    if (tag == BSAL_ACTOR_ASK_TO_STOP) {
        printf("DEBUG bsal_worker_work BSAL_ACTOR_ASK_TO_STOP %d\n", destination);
    }
#endif

    /* Store the buffer location before calling the user
     * code because the user may change the message buffer.
     * We need to free the buffer regardless if the
     * actor code changes it.
     */
    buffer = bsal_message_buffer(message);

    /* the actor died while the work was queued.
     */
    if (bsal_actor_dead(actor)) {

        printf("NOTICE actor/%d is dead already (bsal_worker_work)\n",
                        bsal_message_destination(message));
        bsal_memory_free(buffer);
        bsal_memory_free(message);

        return;
    }

    /* lock the actor to prevent another worker from making work
     * on the same actor at the same time
     */
    if (bsal_actor_trylock(actor) != BSAL_LOCK_SUCCESS) {

        bsal_worker_queue_work(worker, work);
        return;
    }

    /* the actor died while this worker was waiting for the lock
     */
    if (bsal_actor_dead(actor)) {

        printf("DEBUG bsal_worker_work actor died while the worker was waiting for the lock.\n");
#ifdef BSAL_WORKER_DEBUG
#endif
        /* TODO free the buffer with the slab allocator */
        bsal_memory_free(buffer);

        /* TODO replace with slab allocator */
        bsal_memory_free(message);

        /*bsal_actor_unlock(actor);*/
        return;
    }

    /* call the actor receive code
     */
    bsal_actor_set_worker(actor, worker);
    bsal_actor_receive(actor, message);

    bsal_actor_set_worker(actor, NULL);

    /* Free ephemeral memory
     */
    bsal_memory_pool_free_all(&worker->ephemeral_memory);

    dead = bsal_actor_dead(actor);

    if (dead) {
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

    /* TODO free the buffer with the slab allocator */

    /*printf("DEBUG182 Worker free %p\n", buffer);*/
    bsal_memory_free(buffer);

    /* TODO replace with slab allocator */
    bsal_memory_free(message);

#ifdef BSAL_WORKER_DEBUG_20140601
    if (worker->debug) {
        printf("DEBUG worker/%d exiting bsal_worker_work\n",
                        bsal_worker_name(worker));
    }
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

void bsal_worker_start(struct bsal_worker *worker)
{
    /*
     * http://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_create.html
     */

    pthread_create(bsal_worker_thread(worker), NULL, bsal_worker_main,
                    worker);
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

    /* http://man7.org/linux/man-pages/man3/pthread_join.3.html
     */
    pthread_join(worker->thread, NULL);
}

pthread_t *bsal_worker_thread(struct bsal_worker *worker)
{
    return &worker->thread;
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

int bsal_worker_pull_work(struct bsal_worker *worker, struct bsal_work *work)
{
#ifdef BSAL_WORKER_USE_FAST_RINGS
    int result;

    /* first, pull from the ring.
     * If that fails, pull from the local queue
     */
    result = bsal_fast_ring_pop_from_consumer(&worker->work_queue, work);
    if (result) {
        return result;
    }

    return bsal_ring_queue_dequeue(&worker->local_work_queue, work);

#else
    return bsal_ring_pop(&worker->work_queue, work);
#endif
}

void bsal_worker_push_message(struct bsal_worker *worker, struct bsal_message *message)
{
    /* if the message can not be queued in the ring,
     * queue it in the queue
     */

#ifdef BSAL_WORKER_USE_FAST_RINGS
    if (!bsal_fast_ring_push_from_producer(&worker->message_queue, message)) {
        bsal_ring_queue_enqueue(&worker->local_message_queue, message);
    }
#else
    if (!bsal_ring_push(&worker->message_queue, message)) {
        bsal_ring_queue_enqueue(&worker->local_message_queue, message);
    }

#endif
}

#ifdef BSAL_WORKER_HAS_OWN_QUEUES
int bsal_worker_pull_message(struct bsal_worker *worker, struct bsal_message *message)
{
#ifdef BSAL_WORKER_USE_FAST_RINGS
    return bsal_fast_ring_pop_from_consumer(&worker->message_queue, message);
#else
    return bsal_ring_pop(&worker->message_queue, message);
#endif
}
#endif

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

/* Just return the number of queued messages.
 */
int bsal_worker_get_message_production_score(struct bsal_worker *self)
{
    int score;

    score = 0;

#ifdef BSAL_WORKER_USE_FAST_RINGS
    score += bsal_fast_ring_size_from_producer(&self->message_queue);
#else
    score += bsal_ring_size(&self->message_queue);
#endif

    score += bsal_ring_queue_size(&self->local_message_queue);

    return score;
}

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

void bsal_worker_queue_work(struct bsal_worker *worker, struct bsal_work *work)
{
    int count;
    struct bsal_work local_work;
    struct bsal_actor *actor;
    int i;
    struct bsal_map frequencies;
    struct bsal_map_iterator iterator;
    int actor_name;
    struct bsal_vector counts;
    struct bsal_vector_iterator vector_iterator;
    int actor_works;

    /* just put it in the local queue.
     */
    bsal_ring_queue_enqueue(&worker->local_work_queue, work);

    count = bsal_ring_queue_size(&worker->local_work_queue);

    if (count >= BSAL_WORKER_WARNING_THRESHOLD
                   && (worker->last_warning == 0
                           || count >= worker->last_warning + BSAL_WORKER_WARNING_THRESHOLD_STRIDE)) {

        bsal_map_init(&frequencies, sizeof(int), sizeof(int));

        printf("Warning: node %d, worker %d has %d works in its local queue.\n",
                        bsal_node_name(worker->node),
                        bsal_worker_name(worker),
                        count);

        worker->last_warning = count;

        printf("List: \n");

        i = 0;

        while (i < count && bsal_ring_queue_dequeue(&worker->local_work_queue,
                                &local_work)) {

            actor = bsal_work_actor(&local_work);

            actor_name = bsal_actor_name(actor);

            printf("WORK node %d worker %d work %d/%d actor %d\n",
                            bsal_node_name(worker->node),
                            bsal_worker_name(worker),
                            i,
                            count,
                            actor_name);

            bsal_ring_queue_enqueue(&worker->local_work_queue, &local_work);

            if (!bsal_map_get_value(&frequencies, &actor_name, &actor_works)) {
                actor_works = 0;
                bsal_map_add_value(&frequencies, &actor_name, &actor_works);
            }

            ++actor_works;
            bsal_map_update_value(&frequencies, &actor_name, &actor_works);

            ++i;
        }

        bsal_map_iterator_init(&iterator, &frequencies);
        bsal_vector_init(&counts, sizeof(int));

        while (bsal_map_iterator_get_next_key_and_value(&iterator, &actor_name, &actor_works)) {

            printf("actor %d ... %d messages\n",
                            actor_name, actor_works);

            bsal_vector_push_back(&counts, &actor_works);
        }

        bsal_map_iterator_destroy(&iterator);

        bsal_vector_helper_sort_int(&counts);

        bsal_vector_iterator_init(&vector_iterator, &counts);

        printf("Sorted counts:\n");
        while (bsal_vector_iterator_get_next_value(&vector_iterator, &actor_works)) {

            printf(" %d\n", actor_works);
        }

        bsal_vector_iterator_destroy(&vector_iterator);

        bsal_vector_destroy(&counts);
        bsal_map_destroy(&frequencies);
    }
}
