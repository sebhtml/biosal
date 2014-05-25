
#include "bsal_actor.h"
#include "bsal_thread.h"
#include "bsal_node.h"

#include <stdlib.h>
#include <stdio.h>

void bsal_actor_construct(struct bsal_actor *actor, void *pointer,
                bsal_actor_receive_fn_t receive)
{
    /* bsal_actor_construct_fn_t construct; */

    actor->actor = pointer;
    actor->name = -1;
    actor->dead = 0;

    /* bsal_actor->receive = receive; */

    /*
    use a vtable... (where do I allocate memory for  the vtable ? answer: it is in the .o directly.)
    struct bsal_actor_vtable *vtable = ...;
    bsal_actor_vtable_construct(vtable,  receive);
    */

    /* bsal_actor->vtable = vtable; */
    actor->receive = receive;

    /* printf("bsal_actor_construct %p %p %p\n", (void*)bsal_actor, (void*)actor, (void*)receive); */

    /* call the specialized constructor too. */
    /*
    construct = bsal_actor_get_construct(bsal_actor);
    construct(bsal_actor);
    */
}

void bsal_actor_destruct(struct bsal_actor *actor)
{
    /* bsal_actor_destruct_fn_t destruct; */

    /* call the specialized destructor */
    /*
    destruct = bsal_actor_get_destruct(bsal_actor);
    destruct(bsal_actor);
    */

    actor->actor = NULL;
    actor->name = -1;
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
    /* printf("bsal_actor_handler %p %p\n", (void*)bsal_actor, (void*)bsal_actor->receive); */

    return actor->receive;
    /* return bsal_actor_vtable_get_receive(bsal_actor->vtable); */
}

void bsal_actor_set_name(struct bsal_actor *actor, int name)
{
    /*
       printf("bsal_actor_set_name %p %i\n", (void*)actor, name);
       */
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

bsal_actor_construct_fn_t bsal_actor_get_construct(struct bsal_actor *actor)
{
    return NULL;
    /* return bsal_actor_vtable_get_construct(actor->vtable); */
}

bsal_actor_destruct_fn_t bsal_actor_get_destruct(struct bsal_actor *actor)
{
    return NULL;
    /* return bsal_actor_vtable_get_destruct(actor->vtable); */
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

int bsal_actor_spawn(struct bsal_actor *actor, void *new_actor,
                bsal_actor_receive_fn_t receive)
{
    return bsal_node_spawn(bsal_actor_node(actor), new_actor, receive);
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
}

void bsal_actor_unlock(struct bsal_actor *actor)
{
    pthread_mutex_unlock(&actor->mutex);
}
