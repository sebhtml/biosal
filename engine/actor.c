
#include "actor.h"
#include "thread.h"
#include "node.h"

#include <stdlib.h>
#include <stdio.h>

void bsal_actor_init(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable)
{
    actor->actor = pointer;
    actor->name = -1;
    actor->dead = 0;
    actor->vtable = vtable;
    actor->thread = NULL;

    pthread_mutex_init(&actor->mutex, NULL);
    actor->locked = 0;
}

void bsal_actor_destroy(struct bsal_actor *actor)
{
    actor->actor = NULL;
    actor->name = -1;
    actor->dead = 1;
    actor->vtable = NULL;
    actor->thread = NULL;

    bsal_actor_unlock(actor);

    pthread_mutex_destroy(&actor->mutex);
}

int bsal_actor_name(struct bsal_actor *actor)
{
    return actor->name;
}

void *bsal_actor_actor(struct bsal_actor *actor)
{
    return actor->actor;
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
        /*  with -Werror -Wall:
           engine/bsal_actor.c:58:21: error: ISO C for bids conversion of function pointer to object pointer type [-Werror=edantic]
           */
    printf("bsal_actor_print name: %i bsal_actor %p pointer %p\n", bsal_actor_name(actor),
                    (void*)actor, (void*)bsal_actor_actor(actor));
}

bsal_actor_init_fn_t bsal_actor_get_init(struct bsal_actor *actor)
{
    return bsal_actor_vtable_get_init(actor->vtable);
}

bsal_actor_destroy_fn_t bsal_actor_get_destroy(struct bsal_actor *actor)
{
    return bsal_actor_vtable_get_destroy(actor->vtable);
}

void bsal_actor_set_thread(struct bsal_actor *actor, struct bsal_thread *thread)
{
    actor->thread = thread;
}

void bsal_actor_send(struct bsal_actor *actor, int name, struct bsal_message *message)
{
    int source;

    source = bsal_actor_name(actor);
    bsal_message_set_source(message, source);
    bsal_message_set_destination(message, name);
    bsal_thread_send(actor->thread, message);
}

int bsal_actor_spawn(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable)
{
    return bsal_node_spawn(bsal_actor_node(actor), pointer, vtable);
}

struct bsal_thread *bsal_actor_thread(struct bsal_actor *actor)
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

int bsal_actor_size(struct bsal_actor *actor)
{
    return bsal_node_size(bsal_actor_node(actor));
}

struct bsal_node *bsal_actor_node(struct bsal_actor *actor)
{
    if (actor->thread == NULL) {
        return NULL;
    }

    return bsal_thread_node(bsal_actor_thread(actor));
}

void bsal_actor_lock(struct bsal_actor *actor)
{
    pthread_mutex_lock(&actor->mutex);
    actor->locked = 1;
}

void bsal_actor_unlock(struct bsal_actor *actor)
{
    if (!actor->locked) {
        return;
    }

    actor->locked = 0;
    pthread_mutex_unlock(&actor->mutex);
}
