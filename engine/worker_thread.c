
#include "worker_thread.h"

#include "work.h"
#include "message.h"
#include "node.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*#define BSAL_THREAD_DEBUG*/

void bsal_worker_thread_init(struct bsal_worker_thread *thread, int name, struct bsal_node *node)
{
    bsal_fifo_init(&thread->works, 16, sizeof(struct bsal_work));
    bsal_fifo_init(&thread->messages, 16, sizeof(struct bsal_message));

    thread->node = node;
    thread->name = name;
    thread->dead = 0;


#ifdef BSAL_THREAD_USE_LOCK
    pthread_spin_init(&thread->work_lock, 0);
    pthread_spin_init(&thread->message_lock, 0);
#endif
}

void bsal_worker_thread_destroy(struct bsal_worker_thread *thread)
{
#ifdef BSAL_THREAD_USE_LOCK
    pthread_spin_destroy(&thread->message_lock);
    pthread_spin_destroy(&thread->work_lock);
#endif

    thread->node = NULL;

    bsal_fifo_destroy(&thread->messages);
    bsal_fifo_destroy(&thread->works);
    thread->name = -1;
    thread->dead = 1;
}

struct bsal_fifo *bsal_worker_thread_works(struct bsal_worker_thread *thread)
{
    return &thread->works;
}

struct bsal_fifo *bsal_worker_thread_messages(struct bsal_worker_thread *thread)
{
    return &thread->messages;
}

void bsal_worker_thread_run(struct bsal_worker_thread *thread)
{
    struct bsal_work work;

    /* check for messages in inbound FIFO */
    if (bsal_worker_thread_pull_work(thread, &work)) {

        /* dispatch message to a worker thread (currently, this is the main thread) */
        bsal_worker_thread_work(thread, &work);
    }
}

void bsal_worker_thread_work(struct bsal_worker_thread *thread, struct bsal_work *work)
{
    struct bsal_actor *actor;
    struct bsal_message *message;
    bsal_actor_receive_fn_t receive;
    int dead;
    char *buffer;

    actor = bsal_work_actor(work);

    /* lock the actor to prevent another thread from making work
     * on the same actor at the same time
     */
    bsal_actor_lock(actor);

    bsal_actor_set_thread(actor, thread);
    message = bsal_work_message(work);

    bsal_actor_increase_received_messages(actor);
    receive = bsal_actor_get_receive(actor);

    /* Store the buffer location before calling the user
     * code because the user may change the message buffer
     * pointer. We need to free the buffer regardless if the
     * actor code changes it.
     */
    buffer = bsal_message_buffer(message);

    /* call the actor receive code
     */
    receive(actor, message);

    bsal_actor_set_thread(actor, NULL);
    dead = bsal_actor_dead(actor);

    if (dead) {
        bsal_node_notify_death(thread->node, actor);
    }

    /* Unlock the actor.
     * This does not do anything if a death notification
     * was sent to the node
     */
    bsal_actor_unlock(actor);

#ifdef BSAL_THREAD_DEBUG
    printf("bsal_worker_thread_work Freeing buffer %p %i tag %i\n",
                    buffer, bsal_message_count(message),
                    bsal_message_tag(message));
#endif

    /* TODO free the buffer with the slab allocator */
    free(buffer);

    /* TODO replace with slab allocator */
    free(message);
}

struct bsal_node *bsal_worker_thread_node(struct bsal_worker_thread *thread)
{
    return thread->node;
}

void bsal_worker_thread_send(struct bsal_worker_thread *thread, struct bsal_message *message)
{
    struct bsal_message copy;
    char *buffer;
    int count;
    int metadata_size;
    int all;

    memcpy(&copy, message, sizeof(struct bsal_message));
    count = bsal_message_count(&copy);
    metadata_size = bsal_message_metadata_size(message);
    all = count + metadata_size;

    /* TODO use slab allocator to allocate buffer... */
    buffer = (char *)malloc(all * sizeof(char));

#ifdef BSAL_THREAD_DEBUG
    printf("[bsal_worker_thread_send] allocated %i bytes (%i + %i) for buffer %p\n",
                    all, count, metadata_size, buffer);

    printf("bsal_worker_thread_send old buffer: %p\n",
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
        memcpy(buffer, bsal_message_buffer(message), count);
    }

    bsal_message_set_buffer(&copy, buffer);
    bsal_message_write_metadata(&copy);

    bsal_worker_thread_push_message(thread, &copy);
}

void bsal_worker_thread_start(struct bsal_worker_thread *thread)
{
    /*
     * http://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_create.html
     */

    pthread_create(bsal_worker_thread_thread(thread), NULL, bsal_worker_thread_main,
                    thread);
}

void *bsal_worker_thread_main(void *pointer)
{
    struct bsal_worker_thread *thread;

    thread = (struct bsal_worker_thread*)pointer;

#ifdef BSAL_THREAD_DEBUG
    bsal_worker_thread_display(thread);
    printf("Starting thread\n");
#endif

    while (!thread->dead) {

        bsal_worker_thread_run(thread);
    }

    return NULL;
}

void bsal_worker_thread_display(struct bsal_worker_thread *thread)
{
    printf("[bsal_worker_thread_main] node %i thread %i\n",
                    bsal_node_name(thread->node),
                    bsal_worker_thread_name(thread));
}

int bsal_worker_thread_name(struct bsal_worker_thread *thread)
{
    return thread->name;
}

void bsal_worker_thread_stop(struct bsal_worker_thread *thread)
{
#ifdef BSAL_THREAD_DEBUG
    bsal_worker_thread_display(thread);
    printf("stopping thread!\n");
#endif

    /*
     * thread->dead is volatile and will be read
     * by the running thread.
     */
    thread->dead = 1;

    /* http://man7.org/linux/man-pages/man3/pthread_join.3.html
     */
    pthread_join(thread->thread, NULL);
}

pthread_t *bsal_worker_thread_thread(struct bsal_worker_thread *thread)
{
    return &thread->thread;
}

void bsal_worker_thread_push_work(struct bsal_worker_thread *thread, struct bsal_work *work)
{
#ifdef BSAL_THREAD_USE_LOCK
    pthread_spin_lock(&thread->work_lock);
#endif

    bsal_fifo_push(bsal_worker_thread_works(thread), work);

#ifdef BSAL_THREAD_USE_LOCK
    pthread_spin_unlock(&thread->work_lock);
#endif
}

int bsal_worker_thread_pull_work(struct bsal_worker_thread *thread, struct bsal_work *work)
{
    int value;

    /* avoid the spinlock by checking first if
     * there is something to actually pull...
     */
    if (bsal_fifo_empty(bsal_worker_thread_works(thread))) {
        return 0;
    }

#ifdef BSAL_THREAD_USE_LOCK
    pthread_spin_lock(&thread->work_lock);
#endif

    value = bsal_fifo_pop(bsal_worker_thread_works(thread), work);

#ifdef BSAL_THREAD_USE_LOCK
    pthread_spin_unlock(&thread->work_lock);
#endif

    return value;
}

void bsal_worker_thread_push_message(struct bsal_worker_thread *thread, struct bsal_message *message)
{
#ifdef BSAL_THREAD_USE_LOCK
    pthread_spin_lock(&thread->message_lock);
#endif

#ifdef BSAL_THREAD_DEBUG
    printf("bsal_worker_thread_push_message message %p buffer %p\n",
                    (void *)message, bsal_message_buffer(message));
#endif

    bsal_fifo_push(bsal_worker_thread_messages(thread), message);

#ifdef BSAL_THREAD_USE_LOCK
    pthread_spin_unlock(&thread->message_lock);
#endif
}

int bsal_worker_thread_pull_message(struct bsal_worker_thread *thread, struct bsal_message *message)
{
    int value;

#ifdef BSAL_THREAD_USE_LOCK
    pthread_spin_lock(&thread->message_lock);
#endif

    value = bsal_fifo_pop(bsal_worker_thread_messages(thread), message);

#ifdef BSAL_THREAD_USE_LOCK
    pthread_spin_unlock(&thread->message_lock);
#endif

    return value;
}
