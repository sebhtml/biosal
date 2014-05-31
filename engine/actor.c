
#include "actor.h"
#include "worker.h"
#include "node.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#define BSAL_ACTOR_DEBUG
*/

void bsal_actor_init(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable)
{
    actor->pointer = pointer;
    actor->name = -1;
    actor->supervisor = -1;
    actor->dead = 0;
    actor->vtable = vtable;
    actor->worker = NULL;

    actor->received_messages = 0;
    actor->sent_messages = 0;

    pthread_spin_init(&actor->lock, 0);
    actor->locked = 0;

    bsal_actor_unpin(actor);
}

void bsal_actor_destroy(struct bsal_actor *actor)
{
    actor->name = -1;
    actor->dead = 1;

    actor->vtable = NULL;
    actor->worker = NULL;
    actor->pointer = NULL;

    /* unlock the actor if the actor is being destroyed while
     * being locked
     */
    bsal_actor_unlock(actor);

    pthread_spin_destroy(&actor->lock);

    /* when exiting the destructor, the actor is unlocked
     * and destroyed too
     */
}

int bsal_actor_name(struct bsal_actor *actor)
{
    return actor->name;
}

void *bsal_actor_actor(struct bsal_actor *actor)
{
    return actor->pointer;
}

bsal_actor_receive_fn_t bsal_actor_get_receive(struct bsal_actor *actor)
{
    return bsal_actor_vtable_get_receive(actor->vtable);
}

void bsal_actor_set_name(struct bsal_actor *actor, int name)
{
    actor->name = name;
}

void bsal_actor_print(struct bsal_actor *actor)
{
    /* with -Werror -Wall:
     * engine/bsal_actor.c:58:21: error: ISO C for bids conversion of function pointer to object pointer type [-Werror=edantic]
     */

    int received = bsal_actor_received_messages(actor);
    int sent = bsal_actor_sent_messages(actor);

    printf("[bsal_actor_print] Name: %i Supervisor %i Node: %i, Thread: %i"
                    " received %i sent %i\n", bsal_actor_name(actor),
                    bsal_actor_supervisor(actor),
                    bsal_node_name(bsal_actor_node(actor)),
                    bsal_worker_name(bsal_actor_worker(actor)),
                    received, sent);
}

bsal_actor_init_fn_t bsal_actor_get_init(struct bsal_actor *actor)
{
    return bsal_actor_vtable_get_init(actor->vtable);
}

bsal_actor_destroy_fn_t bsal_actor_get_destroy(struct bsal_actor *actor)
{
    return bsal_actor_vtable_get_destroy(actor->vtable);
}

void bsal_actor_set_worker(struct bsal_actor *actor, struct bsal_worker *worker)
{
    actor->worker = worker;
}

void bsal_actor_send(struct bsal_actor *actor, int name, struct bsal_message *message)
{
    int source;

    source = bsal_actor_name(actor);
    bsal_message_set_source(message, source);
    bsal_message_set_destination(message, name);
    bsal_worker_send(actor->worker, message);

    bsal_actor_increase_sent_messages(actor);
}

int bsal_actor_spawn(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable)
{
    int name;

    name = bsal_node_spawn(bsal_actor_node(actor), pointer, vtable);
    bsal_node_set_supervisor(bsal_actor_node(actor), name, bsal_actor_name(actor));

    return name;
}

struct bsal_worker *bsal_actor_worker(struct bsal_actor *actor)
{
    return actor->worker;
}

int bsal_actor_dead(struct bsal_actor *actor)
{
    return actor->dead;
}

void bsal_actor_die(struct bsal_actor *actor)
{
    actor->dead = 1;
}

int bsal_actor_nodes(struct bsal_actor *actor)
{
    return bsal_node_nodes(bsal_actor_node(actor));
}

struct bsal_node *bsal_actor_node(struct bsal_actor *actor)
{
    if (actor->worker == NULL) {
        return NULL;
    }

    return bsal_worker_node(bsal_actor_worker(actor));
}

void bsal_actor_lock(struct bsal_actor *actor)
{
    pthread_spin_lock(&actor->lock);
    actor->locked = 1;
}

void bsal_actor_unlock(struct bsal_actor *actor)
{
    if (!actor->locked) {
        return;
    }

    actor->locked = 0;
    pthread_spin_unlock(&actor->lock);
}

int bsal_actor_workers(struct bsal_actor *actor)
{
    return bsal_node_workers(bsal_actor_node(actor));
}

int bsal_actor_argc(struct bsal_actor *actor)
{
    return bsal_node_argc(bsal_actor_node(actor));
}

char **bsal_actor_argv(struct bsal_actor *actor)
{
    return bsal_node_argv(bsal_actor_node(actor));
}

void bsal_actor_pin(struct bsal_actor *actor)
{
    actor->affinity_worker = actor->worker;
}

struct bsal_worker *bsal_actor_affinity_worker(struct bsal_actor *actor)
{
    return actor->affinity_worker;
}

void bsal_actor_unpin(struct bsal_actor *actor)
{
    actor->affinity_worker = NULL;
}

int bsal_actor_supervisor(struct bsal_actor *actor)
{
    return actor->supervisor;
}

void bsal_actor_set_supervisor(struct bsal_actor *actor, int supervisor)
{
    actor->supervisor = supervisor;
}

uint64_t bsal_actor_received_messages(struct bsal_actor *actor)
{
    return actor->received_messages;
}

void bsal_actor_increase_received_messages(struct bsal_actor *actor)
{
    actor->received_messages++;
}

uint64_t bsal_actor_sent_messages(struct bsal_actor *actor)
{
    return actor->sent_messages;
}

void bsal_actor_increase_sent_messages(struct bsal_actor *actor)
{
    actor->sent_messages++;
}

int bsal_actor_threads(struct bsal_actor *actor)
{
    return bsal_node_threads(bsal_actor_node(actor));
}

void bsal_actor_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    bsal_actor_receive_fn_t receive;

    tag = bsal_message_tag(message);

    /* Perform binomial routing.
     */
    if (tag == BSAL_ACTOR_BINOMIAL_TREE_SEND) {
        bsal_actor_receive_binomial_tree_send(actor, message);
        return;
    }

    /* Otherwise, this is a message for the actor itself.
     */
    receive = bsal_actor_get_receive(actor);
    bsal_actor_increase_received_messages(actor);

    receive(actor, message);
}

void bsal_actor_receive_binomial_tree_send(struct bsal_actor *actor, struct bsal_message *message)
{
    int real_tag;
    int first;
    int last;
    void *buffer;
    int count;
    int amount;
    int new_count;
    int limit;

    limit = 42;
    count = bsal_message_count(message);
    buffer = bsal_message_buffer(message);
    new_count = count - sizeof(real_tag) - sizeof(first) - sizeof(last);

    real_tag = *(int *)((char *)buffer + new_count);
    first = *(int *)((char *)buffer + new_count + sizeof(real_tag));
    last = *(int *)((char *)buffer + new_count + sizeof(real_tag) + sizeof(first));

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG actor %i received BSAL_ACTOR_BINOMIAL_TREE_SEND "
                    "real_tag: %i first: %i last: %i\n",
                    bsal_actor_name(actor), real_tag, first, last);
#endif

    amount = last - first + 1;

    bsal_message_set_tag(message, real_tag);
    bsal_message_set_count(message, new_count);

    if (amount < limit) {
        bsal_actor_send_range_standard(actor, first, last, message);
    } else {
        bsal_actor_send_range_binomial_tree(actor, first, last, message);
    }
}

void bsal_actor_send_range(struct bsal_actor *actor, int first, int last,
                struct bsal_message *message)
{
    /*
    bsal_actor_send_range_standard(actor, first, last, message);
    */

    bsal_actor_send_range_binomial_tree(actor, first, last, message);
}

void bsal_actor_send_range_standard(struct bsal_actor *actor, int first, int last,
                struct bsal_message *message)
{
    int i;

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG bsal_actor_send_range_standard %i-%i\n",
                    first, last);
#endif

    i = first;

    while (i <= last) {
        bsal_actor_send(actor, i, message);
        i++;
    }
}

void bsal_actor_send_range_binomial_tree(struct bsal_actor *actor, int first, int last,
                struct bsal_message *message)
{
    int middle;
    int first1;
    int last1;
    int first2;
    int last2;
    void *buffer;
    void *new_buffer;
    int tag;
    int count;
    int new_count;

    middle = first + (last - first) / 2;

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG bsal_actor_send_range_binomial_tree\n");
    printf("DEBUG first: %i last: %i middle: %i\n", first, last, middle);
#endif

    first1 = first;
    last1 = middle - 1;
    first2 = middle;
    last2 = last;

    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    count = bsal_message_count(message);

    /* TODO use slab allocator */
    new_count = count + sizeof(tag) + sizeof(first1) + sizeof(last1);
    new_buffer = malloc(new_count);

    memcpy(new_buffer, buffer, count);

    /* send to the left actor
     */
    memcpy((char *)new_buffer + count, &tag, sizeof(tag));
    memcpy((char *)new_buffer + count + sizeof(tag), &first1, sizeof(first1));
    memcpy((char *)new_buffer + count + sizeof(tag) + sizeof(first1),
                    &last1, sizeof(last1));
    bsal_message_set_buffer(message, new_buffer);
    bsal_message_set_count(message, new_count);
    bsal_message_set_tag(message, BSAL_ACTOR_BINOMIAL_TREE_SEND);

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG sending BSAL_ACTOR_BINOMIAL_TREE_SEND to %i, range is %i-%i\n",
                    first1, first1, last1);
#endif

    bsal_actor_send(actor, first1, message);

    /* send to the right actor
     */
    memcpy((char *)new_buffer + count, &tag, sizeof(tag));
    memcpy((char *)new_buffer + count + sizeof(tag), &first2, sizeof(first2));
    memcpy((char *)new_buffer + count + sizeof(tag) + sizeof(first2),
                    &last2, sizeof(last2));
    bsal_message_set_buffer(message, new_buffer);
    bsal_message_set_count(message, new_count);
    bsal_message_set_tag(message, BSAL_ACTOR_BINOMIAL_TREE_SEND);

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG sending BSAL_ACTOR_BINOMIAL_TREE_SEND to %i, range is %i-%i\n",
                    first2, first2, last2);
#endif

    bsal_actor_send(actor, first2, message);

    free(new_buffer);
}
