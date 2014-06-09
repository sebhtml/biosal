
#include "worker.h"

#include "work.h"
#include "message.h"
#include "node.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*#define BSAL_THREAD_DEBUG*/

void bsal_worker_init(struct bsal_worker *worker, int name, struct bsal_node *node)
{
    bsal_queue_init(&worker->works, sizeof(struct bsal_work));
    bsal_queue_init(&worker->messages, sizeof(struct bsal_message));

    worker->node = node;
    worker->name = name;
    worker->dead = 0;

#ifdef BSAL_WORKER_USE_LOCK
    bsal_lock_init(&worker->work_lock);
    bsal_lock_init(&worker->message_lock);
#endif

    worker->debug = 0;
}

void bsal_worker_destroy(struct bsal_worker *worker)
{
#ifdef BSAL_WORKER_USE_LOCK
    bsal_lock_destroy(&worker->message_lock);
    bsal_lock_destroy(&worker->work_lock);
#endif

    worker->node = NULL;

    bsal_queue_destroy(&worker->messages);
    bsal_queue_destroy(&worker->works);
    worker->name = -1;
    worker->dead = 1;
}

struct bsal_queue *bsal_worker_works(struct bsal_worker *worker)
{
    return &worker->works;
}

struct bsal_queue *bsal_worker_messages(struct bsal_worker *worker)
{
    return &worker->messages;
}

void bsal_worker_run(struct bsal_worker *worker)
{
    struct bsal_work work;

#ifdef BSAL_WORKER_DEBUG
    if (worker->debug) {
        printf("DEBUG worker/%d bsal_worker_run\n",
                        bsal_worker_name(worker));
    }
#endif

    /* check for messages in inbound FIFO */
    if (bsal_worker_pull_work(worker, &work)) {

        /* dispatch message to a worker */
        bsal_worker_work(worker, &work);
    }
}

void bsal_worker_work(struct bsal_worker *worker, struct bsal_work *work)
{
    struct bsal_actor *actor;
    struct bsal_message *message;
    int dead;
    char *buffer;

    actor = bsal_work_actor(work);

    /* lock the actor to prevent another worker from making work
     * on the same actor at the same time
     */
    bsal_actor_lock(actor);

    message = bsal_work_message(work);

    /* Store the buffer location before calling the user
     * code because the user may change the message buffer.
     * We need to free the buffer regardless if the
     * actor code changes it.
     */
    buffer = bsal_message_buffer(message);

    /* the actor died while this worker was waiting for the lock
     */
    if (bsal_actor_dead(actor)) {

#ifdef BSAL_WORKER_DEBUG
        printf("DEBUG bsal_worker_work actor died while the worker was waiting for the lock.\n");
#endif
        /* TODO free the buffer with the slab allocator */
        free(buffer);

        /* TODO replace with slab allocator */
        free(message);

        bsal_actor_unlock(actor);
        return;
    }

    /* call the actor receive code
     */
    bsal_actor_set_worker(actor, worker);
    bsal_actor_receive(actor, message);

    bsal_actor_set_worker(actor, NULL);
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

#ifdef BSAL_THREAD_DEBUG
    printf("bsal_worker_work Freeing buffer %p %i tag %i\n",
                    buffer, bsal_message_count(message),
                    bsal_message_tag(message));
#endif

    /* TODO free the buffer with the slab allocator */

    /*printf("DEBUG182 Worker free %p\n", buffer);*/
    free(buffer);

    /* TODO replace with slab allocator */
    free(message);

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

    memcpy(&copy, message, sizeof(struct bsal_message));
    count = bsal_message_count(&copy);
    metadata_size = bsal_message_metadata_size(message);
    all = count + metadata_size;

    /* TODO use slab allocator to allocate buffer... */
    buffer = (char *)malloc(all * sizeof(char));

#ifdef BSAL_THREAD_DEBUG
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

    bsal_worker_push_message(worker, &copy);
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

#ifdef BSAL_THREAD_DEBUG
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
#ifdef BSAL_THREAD_DEBUG
    bsal_worker_display(worker);
    printf("stopping thread!\n");
#endif

    /*
     * worker->dead is volatile and will be read
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

void bsal_worker_push_work(struct bsal_worker *worker, struct bsal_work *work)
{
#ifdef BSAL_WORKER_USE_LOCK
    bsal_lock_lock(&worker->work_lock);
#endif

    bsal_queue_enqueue(bsal_worker_works(worker), work);

#ifdef BSAL_WORKER_USE_LOCK
    bsal_lock_unlock(&worker->work_lock);
#endif
}

int bsal_worker_pull_work(struct bsal_worker *worker, struct bsal_work *work)
{
    int value;

    /* avoid the lock by checking first if
     * there is something to actually pull...
     */
    if (bsal_queue_empty(bsal_worker_works(worker))) {
        return 0;
    }

#ifdef BSAL_WORKER_USE_LOCK
    bsal_lock_lock(&worker->work_lock);
#endif

    value = bsal_queue_dequeue(bsal_worker_works(worker), work);

#ifdef BSAL_WORKER_USE_LOCK
    bsal_lock_unlock(&worker->work_lock);
#endif

    return value;
}

void bsal_worker_push_message(struct bsal_worker *worker, struct bsal_message *message)
{
#ifdef BSAL_WORKER_DEBUG_20140601
    if (bsal_message_tag(message) == 1100) {
        printf("DEBUG worker/%d bsal_worker_push_message 1100 before %d\n",
                        bsal_worker_name(worker),
                        bsal_queue_size(bsal_worker_messages(worker)));
        worker->debug = 1;
    }
#endif

#ifdef BSAL_WORKER_USE_LOCK
    bsal_lock_lock(&worker->message_lock);
#endif

#ifdef BSAL_THREAD_DEBUG
    printf("bsal_worker_push_message message %p buffer %p\n",
                    (void *)message, bsal_message_buffer(message));
#endif

    bsal_queue_enqueue(bsal_worker_messages(worker), message);

#ifdef BSAL_WORKER_DEBUG_20140601
    if (bsal_message_tag(message) == 1100) {
        printf("DEBUG worker/%d bsal_worker_push_message 1100 after %d\n",
                        bsal_worker_name(worker),
                        bsal_queue_size(bsal_worker_messages(worker)));
    }
#endif

#ifdef BSAL_WORKER_USE_LOCK
    bsal_lock_unlock(&worker->message_lock);
#endif
}

int bsal_worker_pull_message(struct bsal_worker *worker, struct bsal_message *message)
{
    int value;

#ifdef BSAL_WORKER_DEBUG
    if (worker->debug) {
        printf("DEBUG worker/%d bsal_worker_pull_message 1100 remaining messages %d\n",
                        bsal_worker_name(worker),
                        bsal_queue_size(bsal_worker_messages(worker)));
    }
#endif

    /* avoid the lock by checking first if
     * there is something to actually pull...
     */
    if (bsal_queue_empty(bsal_worker_messages(worker))) {
        return 0;
    }

#ifdef BSAL_WORKER_USE_LOCK
    bsal_lock_lock(&worker->message_lock);
#endif

    value = bsal_queue_dequeue(bsal_worker_messages(worker), message);

#ifdef BSAL_WORKER_DEBUG_20140601
    if (value && bsal_message_tag(message) == 1100) {
        printf("DEBUG worker/%d bsal_worker_pull_message 1100 after %d\n",
                        bsal_worker_name(worker),
                        bsal_queue_size(bsal_worker_messages(worker)));

    }
#endif

#ifdef BSAL_WORKER_USE_LOCK
    bsal_lock_unlock(&worker->message_lock);
#endif

    return value;
}
