
#ifndef _BSAL_ACTOR_H
#define _BSAL_ACTOR_H

#include "message.h"
#include "actor_vtable.h"

#include <pthread.h>
#include <stdint.h>

/* engine/actor.h */
#define BSAL_TAG_OFFSET_ACTOR 0
#define BSAL_TAG_COUNT_ACTOR 7

/* input/input_actor.h */
#define BSAL_TAG_OFFSET_INPUT_ACTOR ( BSAL_TAG_OFFSET_ACTOR + BSAL_TAG_COUNT_ACTOR )
#define BSAL_TAG_COUNT_INPUT_ACTOR 11

/* the user can start with this value */
#define BSAL_TAG_OFFSET_USER ( BSAL_TAG_OFFSET_INPUT_ACTOR + BSAL_TAG_COUNT_INPUT_ACTOR )

enum {
    BSAL_ACTOR_START = BSAL_TAG_OFFSET_ACTOR,
    BSAL_ACTOR_SYNCHRONIZE,
    BSAL_ACTOR_SYNCHRONIZE_REPLY,
    BSAL_ACTOR_BINOMIAL_TREE_SEND,
    BSAL_ACTOR_PROXY_MESSAGE,
    BSAL_ACTOR_PIN,
    BSAL_ACTOR_UNPIN
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

    int synchronization_started;
    int synchronization_responses;
    int synchronization_expected_responses;
};

void bsal_actor_init(struct bsal_actor *actor, void *pointer,
                struct bsal_actor_vtable *vtable);
void bsal_actor_destroy(struct bsal_actor *actor);

int bsal_actor_name(struct bsal_actor *actor);
void *bsal_actor_pointer(struct bsal_actor *actor);
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

void bsal_actor_send(struct bsal_actor *actor, int destination, struct bsal_message *message);

void bsal_actor_send_with_source(struct bsal_actor *actor, int name, struct bsal_message *message,
                int source);

/* Send a message to a range of actors.
 * The implementation uses a binomial tree.
 */
void bsal_actor_send_range(struct bsal_actor *actor, int first, int last,
                struct bsal_message *message);
void bsal_actor_send_range_standard(struct bsal_actor *actor, int first, int last,
                struct bsal_message *message);
void bsal_actor_send_range_binomial_tree(struct bsal_actor *actor, int first, int last,
                struct bsal_message *message);

void bsal_actor_receive(struct bsal_actor *actor, struct bsal_message *message);
void bsal_actor_receive_binomial_tree_send(struct bsal_actor *actor,
                struct bsal_message *message);

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

/* synchronization functions
 */
void bsal_actor_receive_synchronize(struct bsal_actor *actor,
                struct bsal_message *message);
void bsal_actor_receive_synchronize_reply(struct bsal_actor *actor,
                struct bsal_message *message);
int bsal_actor_synchronization_completed(struct bsal_actor *actor);
void bsal_actor_synchronize(struct bsal_actor *actor, int first_actor, int last_actor);

void bsal_actor_receive_proxy_message(struct bsal_actor *actor,
                struct bsal_message *message);
void bsal_actor_pack_proxy_message(struct bsal_actor *actor,
                int real_source, struct bsal_message *message);
int bsal_actor_unpack_proxy_message(struct bsal_actor *actor,
                struct bsal_message *message);

#endif
