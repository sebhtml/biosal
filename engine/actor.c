
#include "actor.h"
#include "worker.h"
#include "node.h"

#include <stdlib.h>
#include <stdio.h>

void bsal_actor_init(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable)
{
    actor->pointer = pointer;
    actor->name = -1;
    actor->supervisor = -1;
    actor->dead = 0;
    actor->vtable = vtable;
    actor->thread = NULL;

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
    actor->thread = NULL;
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
                    bsal_worker_name(bsal_actor_thread(actor)),
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

void bsal_actor_set_thread(struct bsal_actor *actor, struct bsal_worker *thread)
{
    actor->thread = thread;
}

void bsal_actor_send(struct bsal_actor *actor, int name, struct bsal_message *message)
{
    int source;

    source = bsal_actor_name(actor);
    bsal_message_set_source(message, source);
    bsal_message_set_destination(message, name);
    bsal_worker_send(actor->thread, message);

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

struct bsal_worker *bsal_actor_thread(struct bsal_actor *actor)
{
    return actor->thread;
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
    if (actor->thread == NULL) {
        return NULL;
    }

    return bsal_worker_node(bsal_actor_thread(actor));
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

int bsal_actor_threads(struct bsal_actor *actor)
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
    actor->affinity_thread = actor->thread;
}

struct bsal_worker *bsal_actor_affinity_thread(struct bsal_actor *actor)
{
    return actor->affinity_thread;
}

void bsal_actor_unpin(struct bsal_actor *actor)
{
    actor->affinity_thread = NULL;
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
