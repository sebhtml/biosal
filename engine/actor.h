
#ifndef _BSAL_ACTOR_H
#define _BSAL_ACTOR_H

#include "message.h"
#include "actor_vtable.h"

#include <pthread.h>
#include <stdint.h>

/* engine/actor.h */
#define BSAL_TAG_OFFSET_ACTOR 0
#define BSAL_TAG_COUNT_ACTOR 1

/* input/input_actor.h */
#define BSAL_TAG_OFFSET_INPUT_ACTOR ( BSAL_TAG_OFFSET_ACTOR + BSAL_TAG_COUNT_ACTOR )
#define BSAL_TAG_COUNT_INPUT_ACTOR 11

/* the user can start with this value */
#define BSAL_TAG_OFFSET_USER ( BSAL_TAG_OFFSET_INPUT_ACTOR + BSAL_TAG_COUNT_INPUT_ACTOR )

enum {
    BSAL_ACTOR_START = BSAL_TAG_OFFSET_ACTOR
};

struct bsal_node;
struct bsal_worker;

/*
 * the actor attribute is a void *
 */
struct bsal_actor {
    struct bsal_actor_vtable *vtable;
    struct bsal_worker *worker;
    struct bsal_worker *affinity_worker;
    void *pointer;

    pthread_spinlock_t lock;

    int locked;
    int name;
    int dead;
    int supervisor;
    uint64_t received_messages;
    uint64_t sent_messages;
};

void bsal_actor_init(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable);
void bsal_actor_destroy(struct bsal_actor *actor);

int bsal_actor_name(struct bsal_actor *actor);
void *bsal_actor_actor(struct bsal_actor *actor);
void bsal_actor_set_name(struct bsal_actor *actor, int name);

void bsal_actor_set_worker(struct bsal_actor *actor, struct bsal_worker *worker);
struct bsal_worker *bsal_actor_worker(struct bsal_actor *actor);
struct bsal_worker *bsal_actor_affinity_worker(struct bsal_actor *actor);

void bsal_actor_print(struct bsal_actor *actor);
int bsal_actor_dead(struct bsal_actor *actor);
int bsal_actor_nodes(struct bsal_actor *actor);
void bsal_actor_die(struct bsal_actor *actor);

bsal_actor_init_fn_t bsal_actor_get_init(struct bsal_actor *actor);
bsal_actor_destroy_fn_t bsal_actor_get_destroy(struct bsal_actor *actor);
bsal_actor_receive_fn_t bsal_actor_get_receive(struct bsal_actor *actor);

void bsal_actor_send(struct bsal_actor *actor, int name, struct bsal_message *message);
void bsal_actor_receive(struct bsal_actor *actor, struct bsal_message *message);

struct bsal_node *bsal_actor_node(struct bsal_actor *actor);

/*
 * \return This function returns the name of the spawned actor.
 */
int bsal_actor_spawn(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable);

void bsal_actor_lock(struct bsal_actor *actor);
void bsal_actor_unlock(struct bsal_actor *actor);

int bsal_actor_workers(struct bsal_actor *actor);
int bsal_actor_threads(struct bsal_actor *actor);
int bsal_actor_argc(struct bsal_actor *actor);
char **bsal_actor_argv(struct bsal_actor *actor);

/* an actor can be pinned to a worker
 * so that the next message is processed
 * on the same worker.
 * this has implications for memory affinity in
 * NUMA systems
 */
void bsal_actor_pin(struct bsal_actor *actor);
void bsal_actor_unpin(struct bsal_actor *actor);

int bsal_actor_supervisor(struct bsal_actor *actor);
void bsal_actor_set_supervisor(struct bsal_actor *actor, int supervisor);

uint64_t bsal_actor_received_messages(struct bsal_actor *actor);
void bsal_actor_increase_received_messages(struct bsal_actor *actor);
uint64_t bsal_actor_sent_messages(struct bsal_actor *actor);
void bsal_actor_increase_sent_messages(struct bsal_actor *actor);

#endif
