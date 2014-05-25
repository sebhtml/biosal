
#include "bsal_thread.h"
#include "bsal_work.h"
#include "bsal_message.h"
#include "bsal_node.h"

#include <stdlib.h>

void bsal_thread_construct(struct bsal_thread *thread, struct bsal_node *node)
{
    bsal_fifo_construct(&thread->inbound_messages, 16, sizeof(struct bsal_work));
    bsal_fifo_construct(&thread->outbound_messages, 16, sizeof(struct bsal_message));

    thread->node = node;
}

void bsal_thread_destruct(struct bsal_thread *thread)
{
    bsal_fifo_destruct(&thread->outbound_messages);
    bsal_fifo_destruct(&thread->inbound_messages);
}

struct bsal_fifo *bsal_thread_inbound_messages(struct bsal_thread *thread)
{
    return &thread->inbound_messages;
}

struct bsal_fifo *bsal_thread_outbound_messages(struct bsal_thread *thread)
{
    return &thread->outbound_messages;
}

void bsal_thread_run(struct  bsal_thread *thread)
{
    struct bsal_work work;

/* check for messages in inbound FIFO */
    if (bsal_fifo_pop(&thread->inbound_messages, &work)) {

        /* printf("[bsal_node_run] popped message\n"); */
        /* dispatch message to a worker thread (currently, this is the main thread) */
        bsal_thread_work(thread, &work);
    }
}

void bsal_thread_work(struct bsal_thread *thread, struct bsal_work *work)
{
    bsal_actor_receive_fn_t receive;
    struct bsal_actor *actor;
    struct bsal_message *message;

    actor = bsal_work_actor(work);

    /* TODO: lock actor */
    bsal_actor_set_thread(actor, thread);
    message = bsal_work_message(work);

    /* bsal_actor_print(actor); */
    receive = bsal_actor_get_receive(actor);
    /* printf("bsal_node_send %p %p %p %p\n", (void*)actor, (void*)receive,
                    (void*)pointer, (void*)message); */

    receive(actor, message);

    bsal_actor_set_thread(actor, NULL);
    /* TODO unlock actor */
    int dead = bsal_actor_dead(actor);

    /*
    printf("[bsal_node_work] -> ");
    bsal_work_print(work);
    */

    if (dead) {
        bsal_node_notify_death(thread->node, actor);

        /* printf("bsal_node_receive alive %i\n", node->alive_actors); */
    }

    /* TODO replace with slab allocator */
    free((void*)message);
}

struct bsal_node *bsal_thread_node(struct bsal_thread *thread)
{
    return thread->node;
}

void bsal_thread_send(struct bsal_thread *thread, struct bsal_message *message)
{
    bsal_fifo_push(bsal_thread_outbound_messages(thread), message);
}

void bsal_thread_receive(struct bsal_thread *thread, struct bsal_message *message)
{
    bsal_fifo_push(bsal_thread_inbound_messages(thread), message);
}
