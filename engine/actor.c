
#include "actor.h"
#include "worker.h"
#include "node.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#define BSAL_ACTOR_DEBUG

#define BSAL_ACTOR_DEBUG1
*/

void bsal_actor_init(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable)
{
    /*bsal_actor_init_fn_t init;*/

    actor->pointer = pointer;
    actor->name = -1;
    actor->supervisor = -1;
    actor->dead = 0;
    actor->vtable = vtable;
    actor->worker = NULL;

    actor->synchronization_started = 0;
    actor->synchronization_expected_responses = 0;
    actor->synchronization_responses = 0;

    actor->received_messages = 0;
    actor->sent_messages = 0;

    pthread_spin_init(&actor->lock, 0);
    actor->locked = 0;

    bsal_actor_unpin(actor);

    /*
    init = bsal_actor_get_init(actor);
    init(actor);
    */
}

void bsal_actor_destroy(struct bsal_actor *actor)
{
    /*bsal_actor_init_fn_t destroy;*/
/*
    destroy = bsal_actor_get_destroy(actor);
    destroy(actor);
    */

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

void *bsal_actor_pointer(struct bsal_actor *actor)
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
    int tag;
    int self;

    tag = bsal_message_tag(message);
    self = bsal_actor_name(actor);

    /* Verify if the message is a special message.
     * For instance, it is important to pin an
     * actor right away if it is requested.
     */
    if (name == self) {
        if (tag == BSAL_ACTOR_PIN) {
            bsal_actor_pin(actor);
            return;
        } else if (tag == BSAL_ACTOR_UNPIN) {
            bsal_actor_unpin(actor);
            return;
        }
    }

    source = bsal_actor_name(actor);
    bsal_actor_send_with_source(actor, name, message, source);
}

void bsal_actor_send_with_source(struct bsal_actor *actor, int name, struct bsal_message *message,
                int source)
{
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

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE) {
        bsal_actor_receive_synchronize(actor, message);
        return;

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE_REPLY) {
        bsal_actor_receive_synchronize_reply(actor, message);
    } else if (tag == BSAL_ACTOR_PROXY_MESSAGE) {
        bsal_actor_receive_proxy_message(actor, message);
        return;

    /* Ignore BSAL_ACTOR_PIN and BSAL_ACTOR_UNPIN
     * because they can only be sent by an actor
     * to itself.
     */
    } else if (tag == BSAL_ACTOR_PIN) {
        return;
    } else if (tag == BSAL_ACTOR_UNPIN) {
        return;
    }

    /* Otherwise, this is a message for the actor itself.
     */
    receive = bsal_actor_get_receive(actor);
    bsal_actor_increase_received_messages(actor);

    receive(actor, message);
}

void bsal_actor_receive_proxy_message(struct bsal_actor *actor,
                struct bsal_message *message)
{
    int source;

    source = bsal_actor_unpack_proxy_message(actor, message);
    bsal_actor_send_with_source(actor, bsal_actor_name(actor),
                    message, source);
}

void bsal_actor_receive_synchronize(struct bsal_actor *actor,
                struct bsal_message *message)
{
#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG56 replying to %i with BSAL_ACTOR_SYNCHRONIZE_REPLY\n",
                    bsal_message_source(message));
#endif

    bsal_message_set_tag(message, BSAL_ACTOR_SYNCHRONIZE_REPLY);
    bsal_actor_send(actor, bsal_message_source(message), message);
}

void bsal_actor_receive_synchronize_reply(struct bsal_actor *actor,
                struct bsal_message *message)
{
    if (actor->synchronization_started) {

#ifdef BSAL_ACTOR_DEBUG
        printf("DEBUG99 synchronization_reply %i/%i\n",
                        actor->synchronization_responses,
                        actor->synchronization_expected_responses);
#endif

        actor->synchronization_responses++;
    }
}

void bsal_actor_synchronize(struct bsal_actor *actor, int first, int last)
{
    struct bsal_message message;

    actor->synchronization_started = 1;
    actor->synchronization_expected_responses = last - first + 1;
    actor->synchronization_responses = 0;

    /* emit synchronization
     */

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG actor %i emit synchronization %i-%i, expected: %i\n",
                    bsal_actor_name(actor), first, last,
                    actor->synchronization_expected_responses);
#endif

    bsal_message_init(&message, BSAL_ACTOR_SYNCHRONIZE, 0, NULL);
    bsal_actor_send_range(actor, first, last, &message);
    bsal_message_destroy(&message);
}

int bsal_actor_synchronization_completed(struct bsal_actor *actor)
{
    if (actor->synchronization_started == 0) {
        return 0;
    }

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG32 actor %i bsal_actor_synchronization_completed %i/%i\n",
                    bsal_actor_name(actor),
                    actor->synchronization_responses,
                    actor->synchronization_expected_responses);
#endif

    if (actor->synchronization_responses == actor->synchronization_expected_responses) {
        return 1;
    }

    return 0;
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
    int real_source;
    int offset;

    limit = 42;
    count = bsal_message_count(message);
    buffer = bsal_message_buffer(message);
    new_count = count;
    new_count -= sizeof(real_source);
    new_count -= sizeof(real_tag);
    new_count -= sizeof(first);
    new_count -= sizeof(last);

    offset = new_count;

    real_source = *(int *)((char *)buffer + offset);
    offset += sizeof(real_source);
    real_tag = *(int *)((char *)buffer + offset);
    offset += sizeof(real_tag);
    first = *(int *)((char *)buffer + offset);
    offset += sizeof(first);
    last = *(int *)((char *)buffer + offset);
    offset += sizeof(last);

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG78 actor %i received BSAL_ACTOR_BINOMIAL_TREE_SEND "
                    "real_source: %i real_tag: %i first: %i last: %i\n",
                    bsal_actor_name(actor), real_source, real_tag, first,
                    last);
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

int bsal_actor_unpack_proxy_message(struct bsal_actor *actor,
                struct bsal_message *message)
{
    int new_count;
    int tag;
    int source;
    void *buffer;
    int offset;

    buffer = bsal_message_buffer(message);
    new_count = bsal_message_count(message);
    new_count -= sizeof(source);
    new_count -= sizeof(tag);

    offset = new_count;

    source = *(int *)((char *)buffer + offset);
    offset += sizeof(source);
    tag = *(int *)((char *)buffer + offset);
    offset += sizeof(tag);

    bsal_message_set_tag(message, tag);

    return source;
}

void bsal_actor_pack_proxy_message(struct bsal_actor *actor,
                int real_source, struct bsal_message *message)
{
    int real_tag;
    int count;
    int new_count;
    void *buffer;
    void *new_buffer;
    int offset;

    real_tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    count = bsal_message_count(message);

    new_count = count + sizeof(real_source) + sizeof(real_tag);

    /* TODO: use slab allocator */
    new_buffer = malloc(new_count);

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG12 malloc %p (pack proxy message)\n",
                    new_buffer);
#endif

    memcpy(new_buffer, buffer, count);

    offset = count;
    memcpy((char *)new_buffer + offset, &real_source, sizeof(real_source));
    offset += sizeof(real_source);
    memcpy((char *)new_buffer + offset, &real_tag, sizeof(real_tag));
    offset += sizeof(real_tag);

    bsal_message_set_buffer(message, new_buffer);
    bsal_message_set_count(message, new_count);
    bsal_message_set_tag(message, BSAL_ACTOR_PROXY_MESSAGE);
    bsal_message_set_source(message, real_source);
}

void bsal_actor_send_range(struct bsal_actor *actor, int first, int last,
                struct bsal_message *message)
{
    int real_source;

    real_source = bsal_actor_name(actor);

    /*
    bsal_actor_send_range_standard(actor, first, last, message);
    */

    bsal_actor_pack_proxy_message(actor, real_source, message);
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
    int middle1;
    int first2;
    int last2;
    int middle2;
    void *buffer;
    void *new_buffer;
    int tag;
    int count;
    int new_count;
    int source;
    int offset;

#ifdef BSAL_ACTOR_DEBUG
    int name;

    name = bsal_actor_name(actor);
#endif

    source = bsal_actor_name(actor);
    bsal_message_set_source(message, source);

    middle = first + (last - first) / 2;

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG %i bsal_actor_send_range_binomial_tree\n", name);
    printf("DEBUG %i first: %i last: %i middle: %i\n", name, first, last, middle);
#endif

    first1 = first;
    last1 = middle - 1;
    first2 = middle;
    last2 = last;
    middle1 = first1 + (last1 - first1) / 2;
    middle2 = first2 + (last2 - first2) / 2;

    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    count = bsal_message_count(message);

    /* TODO use slab allocator */
    new_count = count + sizeof(source) + sizeof(tag) + sizeof(first1) + sizeof(last1);
    new_buffer = malloc(new_count);

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG12 malloc %p (send_binomial_range)\n",
                    new_buffer);
#endif

    memcpy(new_buffer, buffer, count);

    /* send to the left actor
     */
    offset = count;
    memcpy((char *)new_buffer + offset, &source, sizeof(source));
    offset += sizeof(source);
    memcpy((char *)new_buffer + offset, &tag, sizeof(tag));
    offset += sizeof(tag);
    memcpy((char *)new_buffer + offset, &first1, sizeof(first1));
    offset += sizeof(first1);
    memcpy((char *)new_buffer + offset, &last1, sizeof(last1));
    offset += sizeof(last1);

    bsal_message_set_buffer(message, new_buffer);
    bsal_message_set_count(message, new_count);
    bsal_message_set_tag(message, BSAL_ACTOR_BINOMIAL_TREE_SEND);

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG111111 actor %i sending BSAL_ACTOR_BINOMIAL_TREE_SEND to %i, range is %i-%i\n",
                    name, middle1, first1, last1);
#endif

    bsal_actor_send(actor, middle1, message);

    /* send to the right actor
     */
    offset = count;
    memcpy((char *)new_buffer + offset, &source, sizeof(source));
    offset += sizeof(source);
    memcpy((char *)new_buffer + offset, &tag, sizeof(tag));
    offset += sizeof(tag);
    memcpy((char *)new_buffer + offset, &first2, sizeof(first2));
    offset += sizeof(first2);
    memcpy((char *)new_buffer + offset, &last2, sizeof(last2));
    offset += sizeof(last2);

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG78 %i source: %i tag: %i BSAL_ACTOR_BINOMIAL_TREE_SEND\n",
                    name, source, tag);
#endif

    bsal_message_set_buffer(message, new_buffer);
    bsal_message_set_count(message, new_count);
    bsal_message_set_tag(message, BSAL_ACTOR_BINOMIAL_TREE_SEND);

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG %i sending BSAL_ACTOR_BINOMIAL_TREE_SEND to %i, range is %i-%i\n",
                    name, middle2, first2, last2);
#endif

    bsal_actor_send(actor, middle2, message);

    /* restore the buffer for the user */
    free(new_buffer);
    bsal_message_set_buffer(message, buffer);
}
