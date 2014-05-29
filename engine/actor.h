
#ifndef _BSAL_ACTOR_H
#define _BSAL_ACTOR_H

#include "message.h"
#include "actor_vtable.h"

#include <pthread.h>

enum {
    BSAL_START = 1000 /* FIRST_TAG */ /* LAST_TAG */
};

struct bsal_node;
struct bsal_worker_thread;

/*
 * the actor attribute is a void *
 */
struct bsal_actor {
    struct bsal_actor_vtable *vtable;
    struct bsal_worker_thread *thread;
    void *pointer;

    pthread_spinlock_t lock;

    int locked;
    int name;
    int dead;
};

void bsal_actor_init(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable);
void bsal_actor_destroy(struct bsal_actor *actor);

int bsal_actor_name(struct bsal_actor *actor);
void *bsal_actor_actor(struct bsal_actor *actor);
void bsal_actor_set_name(struct bsal_actor *actor, int name);

void bsal_actor_set_thread(struct bsal_actor *actor, struct bsal_worker_thread *thread);
struct bsal_worker_thread *bsal_actor_thread(struct bsal_actor *actor);

void bsal_actor_print(struct bsal_actor *actor);
int bsal_actor_dead(struct bsal_actor *actor);
int bsal_actor_size(struct bsal_actor *actor);
void bsal_actor_die(struct bsal_actor *actor);

bsal_actor_init_fn_t bsal_actor_get_init(struct bsal_actor *actor);
bsal_actor_destroy_fn_t bsal_actor_get_destroy(struct bsal_actor *actor);
bsal_actor_receive_fn_t bsal_actor_get_receive(struct bsal_actor *actor);
void bsal_actor_send(struct bsal_actor *actor, int name, struct bsal_message *message);

struct bsal_node *bsal_actor_node(struct bsal_actor *actor);

/*
 * \return This function returns the name of the spawned actor.
 */
int bsal_actor_spawn(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable);

void bsal_actor_lock(struct bsal_actor *actor);
void bsal_actor_unlock(struct bsal_actor *actor);

int bsal_actor_threads(struct bsal_actor *actor);

#endif
