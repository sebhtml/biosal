
#ifndef _BSAL_ACTOR_H
#define _BSAL_ACTOR_H

#include "message.h"
#include "script.h"

#include <structures/vector.h>

#include <engine/dispatcher.h>
#include <system/lock.h>
#include <system/counter.h>

#include <pthread.h>
#include <stdint.h>

/* for control */
#define BSAL_ACTOR_START 0x00000885
#define BSAL_ACTOR_START_REPLY 0x0000232f
#define BSAL_ACTOR_STOP 0x0000388c
#define BSAL_ACTOR_STOP_REPLY 0x00006fd8
#define BSAL_ACTOR_ASK_TO_STOP 0x0000607b
#define BSAL_ACTOR_ASK_TO_STOP_REPLY 0x00003602

/* runtime info */
#define BSAL_ACTOR_GET_NODE_NAME 0x00003323
#define BSAL_ACTOR_GET_NODE_NAME_REPLY 0x00004d9a
#define BSAL_ACTOR_GET_NODE_WORKER_COUNT 0x0000147d
#define BSAL_ACTOR_GET_NODE_WORKER_COUNT_REPLY 0x000004ec

/* control YIELD is used as a yielding process.
 * an actor sends this to itself
 * when it receives BSAL_ACTOR_YIELD_REPLY, it continues
 * its work
 */
#define BSAL_ACTOR_YIELD 0x00000173
#define BSAL_ACTOR_YIELD_REPLY 0x000016f1

/* binomial-tree */
#define BSAL_ACTOR_BINOMIAL_TREE_SEND 0x00005b36
#define BSAL_ACTOR_PROXY_MESSAGE 0x00004bed

/* affinity */
#define BSAL_ACTOR_PIN 0x000017a1
#define BSAL_ACTOR_UNPIN 0x00007b66

/* synchronization */
#define BSAL_ACTOR_SYNCHRONIZE 0x00004ac9
#define BSAL_ACTOR_SYNCHRONIZE_REPLY 0x00000663
#define BSAL_ACTOR_SYNCHRONIZED 0x0000453d

/* spawn new actors remotely */
#define BSAL_ACTOR_SPAWN 0x00000119
#define BSAL_ACTOR_SPAWN_REPLY 0x00007b68

/* for import and export */
#define BSAL_ACTOR_PACK_ENABLE 0x000015d3
#define BSAL_ACTOR_PACK_DISABLE 0x00007f0f
#define BSAL_ACTOR_PACK 0x00004cae
#define BSAL_ACTOR_PACK_REPLY 0x000024fc
#define BSAL_ACTOR_UNPACK 0x00001c73
#define BSAL_ACTOR_UNPACK_REPLY 0x000064e4
#define BSAL_ACTOR_PACK_SIZE 0x00003307
#define BSAL_ACTOR_PACK_SIZE_REPLY 0x00005254

/* cloning */
/* design notes:

   clone a process

   BSAL_ACTOR_CLONE
   BSAL_ACTOR_REPLY (contains the clone name)

cloning_state: not supported, supported, started, finished

does this: BSAL_ACTOR_SPAWN
BSAL_ACTOR_PACK to self
forward BSAL_ACTOR_PACK_REPLY to new spawnee as UNPACK
reply BSAL_ CLONE_REPLY with newly spawned actor
*/
#define BSAL_ACTOR_CLONE 0x00000d60
#define BSAL_ACTOR_CLONE_REPLY 0x00006881

/* for migration */
#define BSAL_ACTOR_MIGRATE 0x000073ff
#define BSAL_ACTOR_MIGRATE_REPLY 0x00001442
#define BSAL_ACTOR_MIGRATE_NOTIFY_ACQUAINTANCES 0x000029b6
#define BSAL_ACTOR_MIGRATE_NOTIFY_ACQUAINTANCES_REPLY 0x00007fe2

/* name change for acquaintances

Design notes:

in engine/actor.c
change of name

notification_name_change = BSAL_ACTOR_NOTIFICATION_NAME_CHANGE_NOT_SUPPORTED
BSAL_ACTOR_NOTIFICATION_NAME_CHANGE_SUPPORTED

if unsupported, auto-reply

BSAL_ACTOR_NOTIFY_NAME_CHANGE (source is old name, name is new name)

the actor just need to change any acquaintance with old name to
new name.
*/
#define BSAL_ACTOR_NOTIFY_NAME_CHANGE 0x000068b9
#define BSAL_ACTOR_NOTIFY_NAME_CHANGE_REPLY 0x00003100

#define BSAL_ACTOR_PING 0x000040b3
#define BSAL_ACTOR_PING_REPLY 0x00006eda

/* states */
#define BSAL_ACTOR_STATUS_NOT_SUPPORTED 0
#define BSAL_ACTOR_STATUS_SUPPORTED 1
#define BSAL_ACTOR_STATUS_NOT_STARTED 2
#define BSAL_ACTOR_STATUS_STARTED 3
#define BSAL_ACTOR_STATUS_FINISHED 4

/* special names */
#define BSAL_ACTOR_SELF 0
#define BSAL_ACTOR_SUPERVISOR 1
#define BSAL_ACTOR_SOURCE 2
#define BSAL_ACTOR_NOBODY -1

struct bsal_node;
struct bsal_worker;

/*
 * the actor attribute is a void *
 */
struct bsal_actor {
    struct bsal_script *script;
    struct bsal_worker *worker;
    struct bsal_node *node;
    struct bsal_worker *affinity_worker;
    struct bsal_vector acquaintance_vector;

    struct bsal_dispatcher dispatcher;
    int current_source;
    void *state;

    struct bsal_lock receive_lock;

    int locked;
    int name;
    int dead;

    struct bsal_counter counter;

    int synchronization_started;
    int synchronization_responses;
    int synchronization_expected_responses;

    int can_pack;

    int cloning_status;
    int cloning_spawner;
    int cloning_new_actor;
    int cloning_client;

    int migration_status;
    int migration_spawner;
    int migration_new_actor;
    int migration_client;
    int migration_progressed;
    int migration_cloned;

    int acquaintance_index;
};

void bsal_actor_init(struct bsal_actor *actor, void *state,
                struct bsal_script *script, int name, struct bsal_node *node);
void bsal_actor_destroy(struct bsal_actor *actor);

int bsal_actor_name(struct bsal_actor *actor);
void *bsal_actor_concrete_actor(struct bsal_actor *actor);
void bsal_actor_set_name(struct bsal_actor *actor, int name);

void bsal_actor_set_worker(struct bsal_actor *actor, struct bsal_worker *worker);
struct bsal_worker *bsal_actor_worker(struct bsal_actor *actor);
struct bsal_worker *bsal_actor_affinity_worker(struct bsal_actor *actor);

void bsal_actor_print(struct bsal_actor *actor);
int bsal_actor_dead(struct bsal_actor *actor);
void bsal_actor_die(struct bsal_actor *actor);

bsal_actor_init_fn_t bsal_actor_get_init(struct bsal_actor *actor);
bsal_actor_destroy_fn_t bsal_actor_get_destroy(struct bsal_actor *actor);
bsal_actor_receive_fn_t bsal_actor_get_receive(struct bsal_actor *actor);

/* send functions
 */
void bsal_actor_send(struct bsal_actor *actor, int destination, struct bsal_message *message);
void bsal_actor_send_int(struct bsal_actor *actor, int destination, int tag, int value);
void bsal_actor_send_empty(struct bsal_actor *actor, int destination, int tag);

void bsal_actor_send_reply(struct bsal_actor *actor, struct bsal_message *message);
void bsal_actor_send_reply_int(struct bsal_actor *actor, int tag, int error);
void bsal_actor_send_reply_empty(struct bsal_actor *actor, int tag);

void bsal_actor_send_to_self(struct bsal_actor *actor, struct bsal_message *message);
void bsal_actor_send_to_self_int(struct bsal_actor *actor, int tag, int value);
void bsal_actor_send_to_self_empty(struct bsal_actor *actor, int tag);

void bsal_actor_send_to_supervisor(struct bsal_actor *actor, struct bsal_message *message);
void bsal_actor_send_to_supervisor_int(struct bsal_actor *actor, int tag, int value);
void bsal_actor_send_to_supervisor_empty(struct bsal_actor *actor, int tag);

void bsal_actor_send_with_source(struct bsal_actor *actor, int name, struct bsal_message *message,
                int source);
int bsal_actor_send_system(struct bsal_actor *actor, int name, struct bsal_message *message);

int bsal_actor_source(struct bsal_actor *actor);

/* Send a message to a range of actors.
 * The implementation uses a binomial tree.
 */
void bsal_actor_send_range(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message);
void bsal_actor_send_range_standard(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message);
void bsal_actor_send_range_standard_empty(struct bsal_actor *actor, struct bsal_vector *actors,
                int tag);
void bsal_actor_send_range_binomial_tree(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message);

void bsal_actor_receive(struct bsal_actor *actor, struct bsal_message *message);
int bsal_actor_receive_system(struct bsal_actor *actor, struct bsal_message *message);
void bsal_actor_receive_binomial_tree_send(struct bsal_actor *actor,
                struct bsal_message *message);

struct bsal_node *bsal_actor_node(struct bsal_actor *actor);
int bsal_actor_node_name(struct bsal_actor *actor);
int bsal_actor_node_worker_count(struct bsal_actor *actor);

/*
 * \return This function returns the name of the spawned actor.
 */
int bsal_actor_spawn(struct bsal_actor *actor, int script);

void bsal_actor_lock(struct bsal_actor *actor);
void bsal_actor_unlock(struct bsal_actor *actor);

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

/* synchronization functions
 */
void bsal_actor_receive_synchronize(struct bsal_actor *actor,
                struct bsal_message *message);
void bsal_actor_receive_synchronize_reply(struct bsal_actor *actor,
                struct bsal_message *message);
int bsal_actor_synchronization_completed(struct bsal_actor *actor);
void bsal_actor_synchronize(struct bsal_actor *actor, struct bsal_vector *actors);

void bsal_actor_receive_proxy_message(struct bsal_actor *actor,
                struct bsal_message *message);
void bsal_actor_pack_proxy_message(struct bsal_actor *actor,
                int real_source, struct bsal_message *message);
int bsal_actor_unpack_proxy_message(struct bsal_actor *actor,
                struct bsal_message *message);
int bsal_actor_script(struct bsal_actor *actor);
void bsal_actor_add_script(struct bsal_actor *actor, int name, struct bsal_script *script);

/* actor cloning */
void bsal_actor_clone(struct bsal_actor *actor, struct bsal_message *message);
int bsal_actor_continue_clone(struct bsal_actor *actor, struct bsal_message *message);

struct bsal_counter *bsal_actor_counter(struct bsal_actor *actor);
int bsal_actor_dispatch(struct bsal_actor *actor, struct bsal_message *message);
void bsal_actor_register(struct bsal_actor *actor, int tag, bsal_actor_receive_fn_t handler);
struct bsal_dispatcher *bsal_actor_dispatcher(struct bsal_actor *actor);
void bsal_actor_set_node(struct bsal_actor *actor, struct bsal_node *node);

void bsal_actor_migrate(struct bsal_actor *actor, struct bsal_message *message);
void bsal_actor_notify_name_change(struct bsal_actor *actor, struct bsal_message *message);

struct bsal_vector *bsal_actor_acquaintance_vector(struct bsal_actor *actor);
void bsal_actor_migrate_notify_acquaintances(struct bsal_actor *actor, struct bsal_message *message);

#endif
