
#ifndef THORIUM_ACTOR_H
#define THORIUM_ACTOR_H

#include "message.h"
#include "script.h"
#include "dispatcher.h"
#include "actor_profiler.h"

#include "cache/message_cache.h"
#include "cache/cache_actor_adapter.h"

#include "modules/binomial_tree_message.h"
#include "modules/proxy_message.h"
#include "modules/actions.h"
#include "modules/send_helpers.h"
#include "modules/send_macros.h"
#include "modules/active_message_limit.h"
#include "modules/stop.h"
#include "modules/time_in_seconds.h"
#include "modules/log.h"
#include "modules/adaptive_actor.h"
#include "modules/then.h"
#include "modules/test.h"

/*
 * For priority levels.
 */
#include "scheduler/scheduler.h"

#include "configuration.h"

#include <core/structures/vector.h>
#include <core/structures/map.h>
#include <core/structures/queue.h>
#include <core/structures/fast_ring.h>

#include <core/system/spinlock.h>
#include <core/system/memory_pool.h>
#include <core/system/counter.h>

#include <core/helpers/bitmap.h>

#include <pthread.h>
#include <stdint.h>

/*
 * Enable actor locks.
 * This is not a good thing to enable !
 */
/*
#define THORIUM_ACTOR_ENABLE_LOCK
*/

/*
 * Expose the actor acquaintance API.
 *
 * This is not a good thing to do.
 */
/*
#define THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
*/

#define MEMORY_POOL_NAME_ABSTRACT_ACTOR 0x9739a8fa
#define MEMORY_POOL_NAME_CONCRETE_ACTOR 0x39acca5f

/*
 * We are going to use the Minix convention:
 * - predefined actions all have negative values
 * - each module where ACTION_ are defined has a unique base
 *   in actor.h, the base is #defined as ACTOR_ACTION_BASE
 * - each action is a displacement from the base
 */
#define ACTOR_ACTION_BASE -1000

/* for control */
#define ACTION_START (ACTOR_ACTION_BASE + 0)
#define ACTION_START_REPLY (ACTOR_ACTION_BASE + 1)
#define ACTION_STOP (ACTOR_ACTION_BASE + 2)
#define ACTION_STOP_REPLY (ACTOR_ACTION_BASE + 3)
#define ACTION_ASK_TO_STOP (ACTOR_ACTION_BASE + 4)
#define ACTION_ASK_TO_STOP_REPLY (ACTOR_ACTION_BASE + 5)

#define ACTION_SET_CONSUMER (ACTOR_ACTION_BASE + 6)
#define ACTION_SET_CONSUMER_REPLY (ACTOR_ACTION_BASE + 7)

#define ACTION_SET_PRODUCER (ACTOR_ACTION_BASE + 8)
#define ACTION_SET_PRODUCER_REPLY (ACTOR_ACTION_BASE + 9)
#define ACTION_SET_PRODUCERS (ACTOR_ACTION_BASE + 10)
#define ACTION_SET_PRODUCERS_REPLY (ACTOR_ACTION_BASE + 11)

#define ACTION_SET_CONSUMERS (ACTOR_ACTION_BASE + 12)
#define ACTION_SET_CONSUMERS_REPLY (ACTOR_ACTION_BASE + 13)

/* runtime info */
#define ACTION_GET_NODE_NAME (ACTOR_ACTION_BASE + 14)
#define ACTION_GET_NODE_NAME_REPLY (ACTOR_ACTION_BASE + 15)
#define ACTION_GET_NODE_WORKER_COUNT (ACTOR_ACTION_BASE + 16)
#define ACTION_GET_NODE_WORKER_COUNT_REPLY (ACTOR_ACTION_BASE + 17)

/* control YIELD is used as a yielding process.
 * an actor sends this to itself
 * when it receives ACTION_YIELD_REPLY, it continues
 * its work
 */
#define ACTION_YIELD (ACTOR_ACTION_BASE + 18)
#define ACTION_YIELD_REPLY (ACTOR_ACTION_BASE + 19)

#define ACTION_PROXY_MESSAGE (ACTOR_ACTION_BASE + 20)

/* affinity */
/*
#define ACTION_PIN_TO_WORKER 0x000017a1
#define ACTION_UNPIN_FROM_WORKER 0x00007b66
#define ACTION_PIN_TO_NODE 0x00007b38
#define ACTION_UNPIN_FROM_NODE 0x00006dab
*/

/* synchronization */
#define ACTION_SYNCHRONIZE (ACTOR_ACTION_BASE + 21)
#define ACTION_SYNCHRONIZE_REPLY (ACTOR_ACTION_BASE + 22)
#define ACTION_SYNCHRONIZED (ACTOR_ACTION_BASE + 23)

/* spawn new actors remotely */
#define ACTION_SPAWN (ACTOR_ACTION_BASE + 24)
#define ACTION_SPAWN_REPLY (ACTOR_ACTION_BASE + 25)

/* for import and export */
#define ACTION_PACK_ENABLE (ACTOR_ACTION_BASE + 26)
#define ACTION_PACK_DISABLE (ACTOR_ACTION_BASE + 27)
#define ACTION_PACK (ACTOR_ACTION_BASE + 28)
#define ACTION_PACK_REPLY (ACTOR_ACTION_BASE + 29)
#define ACTION_UNPACK (ACTOR_ACTION_BASE + 30)
#define ACTION_UNPACK_REPLY (ACTOR_ACTION_BASE + 31)
#define ACTION_PACK_SIZE (ACTOR_ACTION_BASE + 32)
#define ACTION_PACK_SIZE_REPLY (ACTOR_ACTION_BASE + 33)

/* cloning */
/* design notes:

   clone a process

   ACTION_CLONE
   THORIUM_ACTOR_REPLY (contains the clone name)

cloning_state: not supported, supported, started, finished

does this: ACTION_SPAWN
ACTION_PACK to self
forward ACTION_PACK_REPLY to new spawnee as UNPACK
reply THORIUM_ CLONE_REPLY with newly spawned actor
*/

/*
 * CLONE takes one int (the spawner) and returns CLONE_REPLY
 *
 */
#define ACTION_CLONE (ACTOR_ACTION_BASE + 34)
/* CLONE_REPLY returns one int: the clone name */
#define ACTION_CLONE_REPLY (ACTOR_ACTION_BASE + 35)

/* for migration */
#define ACTION_MIGRATE (ACTOR_ACTION_BASE + 36)
#define ACTION_MIGRATE_REPLY (ACTOR_ACTION_BASE + 37)
#define ACTION_MIGRATE_NOTIFY_ACQUAINTANCES (ACTOR_ACTION_BASE + 38)
#define ACTION_MIGRATE_NOTIFY_ACQUAINTANCES_REPLY (ACTOR_ACTION_BASE + 39)
#define ACTION_FORWARD_MESSAGES (ACTOR_ACTION_BASE + 39)
#define ACTION_FORWARD_MESSAGES_REPLY (ACTOR_ACTION_BASE + 40)
#define ACTION_SET_SUPERVISOR (ACTOR_ACTION_BASE + 41)
#define ACTION_SET_SUPERVISOR_REPLY (ACTOR_ACTION_BASE + 42)

/* name change for acquaintances

Design notes:

in engine/actor.c
change of name

notification_name_change = THORIUM_ACTOR_NOTIFICATION_NAME_CHANGE_NOT_SUPPORTED
THORIUM_ACTOR_NOTIFICATION_NAME_CHANGE_SUPPORTED

if unsupported, auto-reply

ACTION_NOTIFY_NAME_CHANGE (source is old name, name is new name)

the actor just need to change any acquaintance with old name to
new name.
*/
#define ACTION_NOTIFY_NAME_CHANGE (ACTOR_ACTION_BASE + 43)
#define ACTION_NOTIFY_NAME_CHANGE_REPLY (ACTOR_ACTION_BASE + 44)

/*
 * Messages for actors that are data stores
 */

/* Auto-scaling stuff
 */

#define ACTION_ENABLE_AUTO_SCALING (ACTOR_ACTION_BASE + 45)
#define ACTION_DISABLE_AUTO_SCALING (ACTOR_ACTION_BASE + 46)
#define ACTION_DO_AUTO_SCALING (ACTOR_ACTION_BASE + 47)

/*
 * Type: Request
 * Input: script (int), actor_count (int)
 * Output: a vector contains actor names.
 */
#define ACTION_SPAWN_MANY (ACTOR_ACTION_BASE + 48)

/*
 * Type: Reply
 * The buffer contains a vector with actor names.
 */
#define ACTION_SPAWN_MANY_REPLY (ACTOR_ACTION_BASE + 49)

/*
 * End of action specifiers
 */

/*
 * some actor constants
 */

/* states */
#define THORIUM_ACTOR_STATUS_NOT_SUPPORTED 0
#define THORIUM_ACTOR_STATUS_SUPPORTED 1
#define THORIUM_ACTOR_STATUS_NOT_STARTED 2
#define THORIUM_ACTOR_STATUS_STARTED 3

/* special names */
#define THORIUM_ACTOR_SELF 0
#define THORIUM_ACTOR_SUPERVISOR 1
#define THORIUM_ACTOR_SOURCE 2
#define THORIUM_ACTOR_NOBODY (-1)
#define THORIUM_ACTOR_ANYBODY (-2)
#define THORIUM_ACTOR_SPAWNING_IN_PROGRESS (-3)

#define THORIUM_ACTOR_NO_VALUE -1

/*
 ********************************************
 */

/*
 * Flags.
 */
#define THORIUM_ACTOR_FLAG_DEAD                           CORE_BITMAP_MAKE_FLAG(0)
#define THORIUM_ACTOR_FLAG_CAN_PACK                       CORE_BITMAP_MAKE_FLAG(1)
#define THORIUM_ACTOR_FLAG_MIGRATION_PROGRESSED           CORE_BITMAP_MAKE_FLAG(2)
#define THORIUM_ACTOR_FLAG_LOCKED                         CORE_BITMAP_MAKE_FLAG(3)
#define THORIUM_ACTOR_FLAG_MIGRATION_CLONED               CORE_BITMAP_MAKE_FLAG(4)
#define THORIUM_ACTOR_FLAG_MIGRATION_FORWARDED_MESSAGES   CORE_BITMAP_MAKE_FLAG(5)
#define THORIUM_ACTOR_FLAG_CLONING_PROGRESSED             CORE_BITMAP_MAKE_FLAG(6)
#define THORIUM_ACTOR_FLAG_SYNCHRONIZATION_STARTED        CORE_BITMAP_MAKE_FLAG(7)
#define THORIUM_ACTOR_FLAG_ENABLE_LOAD_PROFILER           CORE_BITMAP_MAKE_FLAG(8)
#define THORIUM_ACTOR_FLAG_ENABLE_MULTIPLEXER             CORE_BITMAP_MAKE_FLAG(9)
#define THORIUM_ACTOR_FLAG_ENABLE_MESSAGE_CACHE           CORE_BITMAP_MAKE_FLAG(10)
#define THORIUM_ACTOR_FLAG_DEFAULT_LOG_LEVEL              CORE_BITMAP_MAKE_FLAG(11)

struct thorium_node;
struct thorium_worker;
struct core_memory_pool;

/*
 * The mailbox size of an actor.
 * When it is full, messages are buffered upstream.
 */

/*
 * The size of actor mailboxes.
 */
/*
#define THORIUM_ACTOR_MAILBOX_SIZE 2
*/
#define THORIUM_ACTOR_MAILBOX_SIZE 256

#define THORIUM_ENABLE_MESSAGE_CACHE

/*
 * The actor attribute is a void *
 */
struct thorium_actor {
    struct thorium_actor_profiler profiler;

#ifdef THORIUM_ENABLE_MESSAGE_CACHE
    struct thorium_message_cache message_cache;
#endif

    struct thorium_script *script;
    struct thorium_worker *worker;

    /*
     * The affinity worker for this actor.
     */
    int assigned_worker;

    struct thorium_node *node;

    /*
     * The priority of an actor should be updated by the actor.
     * The Thorium scheduler does a better job at this.
     */
    int priority;
    uint32_t flags;
    struct core_fast_ring mailbox;
    struct core_memory_pool abstract_memory_pool;
    struct core_memory_pool concrete_memory_pool;

#ifdef THORIUM_ACTOR_GATHER_MESSAGE_METADATA
    struct core_map received_messages;
    struct core_map sent_messages;
    struct core_counter counter;
#endif

#ifdef THORIUM_ACTOR_STORE_CHILDREN
    struct core_vector acquaintance_vector;
    struct core_map acquaintance_map;

    struct core_vector children;
    int acquaintance_index;
#endif

    struct core_queue enqueued_messages;

    struct thorium_dispatcher dispatcher;
    struct thorium_message *current_message;
    void *concrete_actor;

#ifdef THORIUM_ACTOR_ENABLE_LOCK
    struct core_spinlock receive_lock;
#endif

    /*
     * The name of the actor
     */
    int name;

    /*
     * The name of the supervisor for this
     * actor.
     */
    int supervisor;

    /*
     * The inder of the initial actor to use
     * for spawning new colleagues in the company.
     */
    int spawner_index;

    int synchronization_responses;
    int synchronization_expected_responses;

    int forwarding_selector;
    struct core_queue forwarding_queue;
    struct core_queue queued_messages_for_clone;
    struct core_queue queued_messages_for_migration;

    int cloning_status;
    int cloning_spawner;
    int cloning_new_actor;
    int cloning_client;

    int migration_status;
    int migration_spawner;
    int migration_new_actor;
    int migration_client;

    /*
     * \see https://www.kernel.org/doc/Documentation/scheduler/sched-design-CFS.txt
     */
    uint64_t virtual_runtime;
    struct core_timer timer;

    int message_number;

    unsigned int random_seed;

    int counter_received_message_count;
    int counter_sent_message_count;
    int counter_sent_message_count_local;
    int counter_sent_message_count_remote;
};

void thorium_actor_init(struct thorium_actor *self, void *state,
                struct thorium_script *script, int name, struct thorium_node *node);
void thorium_actor_destroy(struct thorium_actor *self);

/*
 * Get the current actor name.
 */
int thorium_actor_name(struct thorium_actor *self);

/*
 * Get the concrete actor (as a void *) from the abstract actor (struct thorium_actor *
 */
void *thorium_actor_concrete_actor(struct thorium_actor *self);

void thorium_actor_set_name(struct thorium_actor *self, int name);

void thorium_actor_set_worker(struct thorium_actor *self, struct thorium_worker *worker);
struct thorium_worker *thorium_actor_worker(struct thorium_actor *self);

void thorium_actor_print(struct thorium_actor *self);
int thorium_actor_dead(struct thorium_actor *self);
void thorium_actor_die(struct thorium_actor *self);

thorium_actor_init_fn_t thorium_actor_get_init(struct thorium_actor *self);
thorium_actor_destroy_fn_t thorium_actor_get_destroy(struct thorium_actor *self);
thorium_actor_receive_fn_t thorium_actor_get_receive(struct thorium_actor *self);

/*
 * Send function
 */
void thorium_actor_send(struct thorium_actor *self, int destination, struct thorium_message *message);

void thorium_actor_send_with_source(struct thorium_actor *self, int name, struct thorium_message *message,
                int source);
int thorium_actor_send_system(struct thorium_actor *self, int name, struct thorium_message *message);
int thorium_actor_send_system_self(struct thorium_actor *self, struct thorium_message *message);

int thorium_actor_source(struct thorium_actor *self);

void thorium_actor_receive(struct thorium_actor *self, struct thorium_message *message);
int thorium_actor_receive_system(struct thorium_actor *self, struct thorium_message *message);
int thorium_actor_receive_system_no_pack(struct thorium_actor *self, struct thorium_message *message);

struct thorium_node *thorium_actor_node(struct thorium_actor *self);
int thorium_actor_node_name(struct thorium_actor *self);
int thorium_actor_worker_name(struct thorium_actor *self);
int thorium_actor_get_node_count(struct thorium_actor *self);
int thorium_actor_node_worker_count(struct thorium_actor *self);

/*
 * \return This function returns the name of the spawned actor.
 */
int thorium_actor_spawn(struct thorium_actor *self, int script);

#ifdef THORIUM_ACTOR_ENABLE_LOCK
void thorium_actor_lock(struct thorium_actor *self);
int thorium_actor_trylock(struct thorium_actor *self);
void thorium_actor_unlock(struct thorium_actor *self);
#endif

int thorium_actor_argc(struct thorium_actor *self);
char **thorium_actor_argv(struct thorium_actor *self);

int thorium_actor_supervisor(struct thorium_actor *self);
void thorium_actor_set_supervisor(struct thorium_actor *self, int supervisor);

int thorium_actor_script(struct thorium_actor *self);
struct thorium_script *thorium_actor_get_script(struct thorium_actor *self);
void thorium_actor_add_script(struct thorium_actor *self, int name, struct thorium_script *script);

struct core_counter *thorium_actor_counter(struct thorium_actor *self);

/*
 * Functions to use and register handlers
 */
int thorium_actor_take_action(struct thorium_actor *self, struct thorium_message *message);

void thorium_actor_add_action_with_source_and_condition(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler,
                int source, int *actual, int expected);

struct thorium_dispatcher *thorium_actor_dispatcher(struct thorium_actor *self);
void thorium_actor_set_node(struct thorium_actor *self, struct thorium_node *node);

struct core_map *thorium_actor_get_received_messages(struct thorium_actor *self);
struct core_map *thorium_actor_get_sent_messages(struct thorium_actor *self);

struct core_memory_pool *thorium_actor_get_ephemeral_memory(struct thorium_actor *self);
struct core_memory_pool *thorium_actor_get_ephemeral_memory_pool(struct thorium_actor *self);
struct core_memory_pool *thorium_actor_get_persistent_memory_pool(struct thorium_actor *self);
struct core_memory_pool *thorium_actor_get_abstract_memory_pool(struct thorium_actor *self);
struct core_memory_pool *thorium_actor_get_concrete_memory_pool(struct thorium_actor *self);
struct core_memory_pool *thorium_actor_get_memory_pool(struct thorium_actor *self, int pool);

struct thorium_worker *thorium_actor_get_last_worker(struct thorium_actor *self);

int thorium_actor_enqueue_mailbox_message(struct thorium_actor *self, struct thorium_message *message);
int thorium_actor_dequeue_mailbox_message(struct thorium_actor *self, struct thorium_message *message);
int thorium_actor_get_mailbox_size(struct thorium_actor *self);

int thorium_actor_get_sum_of_received_messages(struct thorium_actor *self);
int thorium_actor_work(struct thorium_actor *self);
char *thorium_actor_script_name(struct thorium_actor *self);
void thorium_actor_reset_counters(struct thorium_actor *self);

int thorium_actor_get_priority(struct thorium_actor *self);
void thorium_actor_set_priority(struct thorium_actor *self, int priority);
int thorium_actor_get_source_count(struct thorium_actor *self);

int thorium_actor_get_spawner(struct thorium_actor *self, struct core_vector *spawners);
int thorium_actor_get_random_spawner(struct thorium_actor *self, struct core_vector *spawners);

void thorium_actor_enable_profiler(struct thorium_actor *self);
void thorium_actor_disable_profiler(struct thorium_actor *self);

void thorium_actor_write_profile(struct thorium_actor *self,
               struct core_buffered_file_writer *writer);

/*
 * Expose the acquaintance API if required.
 */
#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR

#define thorium_actor_acquaintance_vector thorium_actor_acquaintance_vector_private
#define thorium_actor_add_acquaintance thorium_actor_add_acquaintance_private
#define thorium_actor_get_acquaintance thorium_actor_get_acquaintance_private

#define thorium_actor_get_acquaintance_index thorium_actor_get_acquaintance_index_private
#define thorium_actor_acquaintance_count thorium_actor_acquaintance_count_private

#endif

void *thorium_actor_allocate(struct thorium_actor *self, size_t count);
void thorium_actor_synchronize(struct thorium_actor *self, struct core_vector *actors);

int thorium_actor_assigned_worker(struct thorium_actor *self);
void thorium_actor_set_assigned_worker(struct thorium_actor *self, int worker);

int thorium_actor_get_random_number(struct thorium_actor *self);
int thorium_actor_multiplexer_is_enabled(struct thorium_actor *self);

int thorium_actor_get_counter_value(struct thorium_actor *self, int field);

int thorium_actor_get_flag(struct thorium_actor *self, int flag);
void thorium_actor_set_flag(struct thorium_actor *self, int flag);
void thorium_actor_clear_flag(struct thorium_actor *self, int flag);

void thorium_actor_print_communication_report(struct thorium_actor *self);

void thorium_actor_increment_counter(struct thorium_actor *self, int event);
int thorium_actor_get_message_number(struct thorium_actor *self);

#define NAME() \
        thorium_actor_name(self)

#endif
