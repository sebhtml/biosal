
#include "actor.h"

#include "worker.h"
#include "node.h"

#include "scheduler/scheduling_queue.h"

#include <core/structures/vector_iterator.h>
#include <core/structures/map_iterator.h>

#include <core/helpers/actor_helper.h>
#include <core/helpers/vector_helper.h>
#include <core/helpers/message_helper.h>

#include <core/system/memory.h>
#include <core/system/debugger.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Debugging options
 */

/*
#define BSAL_ACTOR_DEBUG

#define BSAL_ACTOR_DEBUG1

#define BSAL_ACTOR_DEBUG_SYNC
#define BSAL_ACTOR_DEBUG_BINOMIAL_TREE

#define BSAL_ACTOR_DEBUG_SPAWN
#define BSAL_ACTOR_DEBUG_10335

#define BSAL_ACTOR_DEBUG_MIGRATE
#define BSAL_ACTOR_DEBUG_FORWARDING
#define BSAL_ACTOR_DEBUG_CLONE
*/


/* some constants
 */
#define BSAL_ACTOR_ACQUAINTANCE_SUPERVISOR 0

#define BSAL_ACTOR_FORWARDING_NONE 0
#define BSAL_ACTOR_FORWARDING_CLONE 1
#define BSAL_ACTOR_FORWARDING_MIGRATE 2

void bsal_actor_init(struct bsal_actor *self, void *concrete_actor,
                struct bsal_script *script, int name, struct bsal_node *node)
{
    bsal_actor_init_fn_t init;
    int capacity;

    bsal_actor_set_priority(self, BSAL_PRIORITY_NORMAL);

    bsal_map_init(&self->received_messages, sizeof(int), sizeof(int));
    bsal_map_init(&self->sent_messages, sizeof(int), sizeof(int));

    /* initialize the dispatcher before calling
     * the concrete initializer
     */
    bsal_dispatcher_init(&self->dispatcher);

    self->concrete_actor = concrete_actor;
    self->name = name;
    self->node = node;
    self->dead = 0;
    self->script = script;
    self->worker = NULL;

    self->spawner_index = BSAL_ACTOR_NO_VALUE;

    self->synchronization_started = 0;
    self->synchronization_expected_responses = 0;
    self->synchronization_responses = 0;

    bsal_lock_init(&self->receive_lock);
    self->locked = BSAL_LOCK_UNLOCKED;

    self->can_pack = BSAL_ACTOR_STATUS_NOT_SUPPORTED;

    self->cloning_status = BSAL_ACTOR_STATUS_NOT_STARTED;
    self->migration_status = BSAL_ACTOR_STATUS_NOT_STARTED;
    self->migration_cloned = 0;
    self->migration_forwarded_messages = 0;

/*
    printf("DEBUG actor %d init can_pack %d\n",
                    bsal_actor_name(self), self->can_pack);
*/
    bsal_vector_init(&self->acquaintance_vector, sizeof(int));
    bsal_vector_init(&self->children, sizeof(int));
    bsal_queue_init(&self->queued_messages_for_clone, sizeof(struct bsal_message));
    bsal_queue_init(&self->queued_messages_for_migration, sizeof(struct bsal_message));
    bsal_queue_init(&self->forwarding_queue, sizeof(struct bsal_message));

    bsal_map_init(&self->acquaintance_map, sizeof(int), sizeof(int));

    /*
    bsal_actor_send_to_self_empty(self, BSAL_ACTOR_UNPIN_FROM_WORKER);
    bsal_actor_send_to_self_empty(self, BSAL_ACTOR_PIN_TO_NODE);
    */

    bsal_queue_init(&self->enqueued_messages, sizeof(struct bsal_message));

    capacity = BSAL_ACTOR_MAILBOX_SIZE;
    bsal_fast_ring_init(&self->mailbox, capacity, sizeof(struct bsal_message));

    /* call the concrete initializer
     * this must be the last call.
     */
    init = bsal_actor_get_init(self);
    init(self);

    BSAL_DEBUGGER_ASSERT(self->name != BSAL_ACTOR_NOBODY);
}

void bsal_actor_destroy(struct bsal_actor *self)
{
    bsal_actor_init_fn_t destroy;
    struct bsal_message message;
    void *buffer;

    /* The concrete actor must first be destroyed.
     */
    destroy = bsal_actor_get_destroy(self);
    destroy(self);

    /*
     * Make sure that everyone see that this actor is
     * dead.
     */
    self->dead = 1;

    bsal_memory_fence();

    bsal_dispatcher_destroy(&self->dispatcher);
    bsal_vector_destroy(&self->acquaintance_vector);
    bsal_vector_destroy(&self->children);
    bsal_queue_destroy(&self->queued_messages_for_clone);
    bsal_queue_destroy(&self->queued_messages_for_migration);
    bsal_queue_destroy(&self->forwarding_queue);
    bsal_map_destroy(&self->acquaintance_map);

    bsal_map_destroy(&self->received_messages);
    bsal_map_destroy(&self->sent_messages);

    while (bsal_queue_dequeue(&self->enqueued_messages, &message)) {
        buffer = bsal_message_buffer(&message);

        if (buffer != NULL) {
            bsal_memory_free(buffer);
        }
    }

    bsal_queue_destroy(&self->enqueued_messages);

    while (bsal_actor_dequeue_mailbox_message(self, &message)) {
        buffer = bsal_message_buffer(&message);

        if (buffer != NULL) {
            bsal_memory_free(buffer);
        }
    }

    self->name = -1;

    self->script = NULL;
    self->worker = NULL;
    self->concrete_actor = NULL;

    /* unlock the actor if the actor is being destroyed while
     * being locked
     */
    bsal_actor_unlock(self);

    bsal_lock_destroy(&self->receive_lock);

    /* when exiting the destructor, the actor is unlocked
     * and destroyed too
     */

    bsal_fast_ring_destroy(&self->mailbox);
}

int bsal_actor_name(struct bsal_actor *self)
{
    BSAL_DEBUGGER_ASSERT(self != NULL);

    return self->name;
}

bsal_actor_receive_fn_t bsal_actor_get_receive(struct bsal_actor *self)
{
    return bsal_script_get_receive(self->script);
}

void bsal_actor_set_name(struct bsal_actor *self, int name)
{
    self->name = name;
}

void bsal_actor_print(struct bsal_actor *self)
{
    /* with -Werror -Wall:
     * engine/bsal_actor.c:58:21: error: ISO C for bids conversion of function pointer to object pointer type [-Werror=edantic]
     */

    int received = (int)bsal_counter_get(&self->counter, BSAL_COUNTER_RECEIVED_MESSAGES);
    int sent = (int)bsal_counter_get(&self->counter, BSAL_COUNTER_SENT_MESSAGES);

    printf("INSPECT actor: %s/%d\n",
                        bsal_actor_script_name(self),
                        bsal_actor_name(self));

    printf("[bsal_actor_print] Name: %i Supervisor %i Node: %i, Thread: %i"
                    " received %i sent %i\n", bsal_actor_name(self),
                    bsal_actor_supervisor(self),
                    bsal_node_name(bsal_actor_node(self)),
                    bsal_worker_name(bsal_actor_worker(self)),
                    received, sent);
}

bsal_actor_init_fn_t bsal_actor_get_init(struct bsal_actor *self)
{
    return bsal_script_get_init(self->script);
}

bsal_actor_destroy_fn_t bsal_actor_get_destroy(struct bsal_actor *self)
{
    return bsal_script_get_destroy(self->script);
}

void bsal_actor_set_worker(struct bsal_actor *self, struct bsal_worker *worker)
{
    self->worker = worker;
}

int bsal_actor_send_system_self(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;

    tag = bsal_message_tag(message);

#if 0
    if (tag == BSAL_ACTOR_PIN_TO_WORKER) {
        bsal_actor_pin_to_worker(self);
        return 1;

    } else if (tag == BSAL_ACTOR_UNPIN_FROM_WORKER) {
        bsal_actor_unpin_from_worker(self);
        return 1;

    if (tag == BSAL_ACTOR_PIN_TO_NODE) {
        bsal_actor_pin_to_node(self);
        return 1;

    } else if (tag == BSAL_ACTOR_UNPIN_FROM_NODE) {
        bsal_actor_unpin_from_node(self);
        return 1;

#endif

    if (tag == BSAL_ACTOR_PACK_ENABLE) {

            /*
        printf("DEBUG actor %d enabling can_pack\n",
                        bsal_actor_name(self));
                        */

        self->can_pack = BSAL_ACTOR_STATUS_SUPPORTED;

        /*
        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_UNPIN_FROM_WORKER);
        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_UNPIN_FROM_NODE);
        */

        return 1;

    } else if (tag == BSAL_ACTOR_PACK_DISABLE) {
        self->can_pack = BSAL_ACTOR_STATUS_NOT_SUPPORTED;
        return 1;

    } else if (tag == BSAL_ACTOR_YIELD) {
        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_YIELD_REPLY);
        return 1;

    } else if (tag == BSAL_ACTOR_STOP) {

        bsal_actor_die(self);
        return 1;
    }

    return 0;
}

int bsal_actor_send_system(struct bsal_actor *self, int name, struct bsal_message *message)
{
    int self_name;

    self_name = bsal_actor_name(self);

    /* Verify if the message is a special message.
     * For instance, it is important to pin an
     * actor right away if it is requested.
     */
    if (name == self_name) {
        if (bsal_actor_send_system_self(self, message)) {
            return 1;
        }
    }

    return 0;
}

void bsal_actor_send(struct bsal_actor *self, int name, struct bsal_message *message)
{
    int source;
    int *bucket;

    /* Update counter
     */
    bucket = (int *)bsal_map_get(&self->sent_messages, &name);

    if (bucket == NULL) {
        bucket = (int *)bsal_map_add(&self->sent_messages, &name);
        (*bucket) = 0;
    }

    (*bucket)++;

    source = bsal_actor_name(self);

    /* update counters
     */
    if (source == name) {
        bsal_counter_add(&self->counter, BSAL_COUNTER_SENT_MESSAGES_TO_SELF, 1);
        bsal_counter_add(&self->counter, BSAL_COUNTER_SENT_BYTES_TO_SELF,
                        bsal_message_count(message));
    } else {
        bsal_counter_add(&self->counter, BSAL_COUNTER_SENT_MESSAGES_NOT_TO_SELF, 1);
        bsal_counter_add(&self->counter, BSAL_COUNTER_SENT_BYTES_NOT_TO_SELF,
                        bsal_message_count(message));
    }

    if (bsal_actor_send_system(self, name, message)) {
        return;
    }

    bsal_actor_send_with_source(self, name, message, source);
}

void bsal_actor_send_with_source(struct bsal_actor *self, int name, struct bsal_message *message,
                int source)
{
    int tag;

    tag = bsal_message_tag(message);
    bsal_message_set_source(message, source);
    bsal_message_set_destination(message, name);

#ifdef BSAL_ACTOR_DEBUG9
    if (bsal_message_tag(message) == 1100) {
        printf("DEBUG bsal_message_set_source 1100\n");
    }
#endif

    /* messages sent in the init or destroy are not sent
     * at all !
     */
    if (self->worker == NULL) {

        printf("Error, message was lost because it was sent in *_init() or *_destroy(), which is not allowed (tag: %d)\n",
                        tag);

        return;
    }

    bsal_worker_send(self->worker, message);
}

int bsal_actor_spawn(struct bsal_actor *self, int script)
{
    int name;

    name = bsal_actor_spawn_real(self, script);

    bsal_actor_add_child(self, name);

#ifdef BSAL_ACTOR_DEBUG_SPAWN
    printf("acquaintances after spawning\n");
    bsal_vector_helper_print_int(&self->acquaintance_vector);
    printf("\n");
#endif

    return name;
}

int bsal_actor_spawn_real(struct bsal_actor *self, int script)
{
    int name;
    int self_name = bsal_actor_name(self);

#ifdef BSAL_ACTOR_DEBUG_SPAWN
    printf("DEBUG bsal_actor_spawn script %d\n", script);
#endif

    name = bsal_node_spawn(bsal_actor_node(self), script);

    if (name == BSAL_ACTOR_NOBODY) {
        printf("Error: problem with spawning! did you register the script ?\n");
        return name;
    }

#ifdef BSAL_ACTOR_DEBUG_SPAWN
    printf("DEBUG bsal_actor_spawn before set_supervisor, spawned %d\n",
                    name);
#endif

    bsal_node_set_supervisor(bsal_actor_node(self), name, self_name);

    bsal_counter_add(&self->counter, BSAL_COUNTER_SPAWNED_ACTORS, 1);

    return name;
}

void bsal_actor_die(struct bsal_actor *self)
{
    bsal_counter_add(&self->counter, BSAL_COUNTER_KILLED_ACTORS, 1);
    self->dead = 1;
}

struct bsal_counter *bsal_actor_counter(struct bsal_actor *self)
{
    return &self->counter;
}

struct bsal_node *bsal_actor_node(struct bsal_actor *self)
{
    if (self->node != NULL) {
        return self->node;
    }

    if (self->worker == NULL) {
        return NULL;
    }

    return bsal_worker_node(bsal_actor_worker(self));
}

void bsal_actor_lock(struct bsal_actor *self)
{
    bsal_lock_lock(&self->receive_lock);
    self->locked = BSAL_LOCK_LOCKED;
}

void bsal_actor_unlock(struct bsal_actor *self)
{
    if (!self->locked) {
        return;
    }

    self->locked = BSAL_LOCK_UNLOCKED;
    bsal_lock_unlock(&self->receive_lock);
}

int bsal_actor_argc(struct bsal_actor *self)
{
    return bsal_node_argc(bsal_actor_node(self));
}

char **bsal_actor_argv(struct bsal_actor *self)
{
    return bsal_node_argv(bsal_actor_node(self));
}

int bsal_actor_supervisor(struct bsal_actor *self)
{
    return self->supervisor;
}

void bsal_actor_set_supervisor(struct bsal_actor *self, int supervisor)
{
    self->supervisor = supervisor;
}

int bsal_actor_receive_system_no_pack(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;

    tag = bsal_message_tag(message);

    if (tag == BSAL_ACTOR_PACK) {

        bsal_actor_send_reply_empty(self, BSAL_ACTOR_PACK_REPLY);
        return 1;

    } else if (tag == BSAL_ACTOR_PACK_SIZE) {
        bsal_actor_send_reply_int(self, BSAL_ACTOR_PACK_SIZE_REPLY, 0);
        return 1;

    } else if (tag == BSAL_ACTOR_UNPACK) {
        bsal_actor_send_reply_empty(self, BSAL_ACTOR_PACK_REPLY);
        return 1;

    } else if (tag == BSAL_ACTOR_CLONE) {

        /* return nothing if the cloning is not supported or
         * if a cloning is already in progress, the message will be queued below.
         */

            /*
        printf("DEBUG actor %d BSAL_ACTOR_CLONE not supported can_pack %d\n", bsal_actor_name(self),
                        self->can_pack);
                        */

        bsal_actor_send_reply_int(self, BSAL_ACTOR_CLONE_REPLY, BSAL_ACTOR_NOBODY);
        return 1;

    } else if (tag == BSAL_ACTOR_MIGRATE) {

        /* return nothing if the cloning is not supported or
         * if a cloning is already in progress
         */

#ifdef BSAL_ACTOR_DEBUG_MIGRATE
        printf("DEBUG bsal_actor_migrate: pack not supported\n");
#endif

        bsal_actor_send_reply_int(self, BSAL_ACTOR_MIGRATE_REPLY, BSAL_ACTOR_NOBODY);

        return 1;
    }

    return 0;
}

int bsal_actor_receive_system(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    int name;
    int source;
    int spawned;
    int script;
    void *buffer;
    int count;
    int old_supervisor;
    int supervisor;
    int new_count;
    void *new_buffer;
    struct bsal_message new_message;
    int offset;
    int bytes;
    struct bsal_memory_pool *ephemeral_memory;
    int new_actor;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
    tag = bsal_message_tag(message);

    /* the concrete actor must catch these otherwise.
     * Also, clone and migrate depend on these.
     */
    if (self->can_pack == BSAL_ACTOR_STATUS_NOT_SUPPORTED) {

        if (bsal_actor_receive_system_no_pack(self, message)) {
            return 1;
        }
    }

    name = bsal_actor_name(self);
    source =bsal_message_source(message);
    buffer = bsal_message_buffer(message);
    count = bsal_message_count(message);

    /* For any remote spawning operation, add the new actor in the list of
     * children
     */
    if (tag == BSAL_ACTOR_SPAWN_REPLY) {

        new_actor = *(int *)buffer;
        bsal_actor_add_child(self, new_actor);
    }

    /* check message tags that are required for migration
     * before attempting to queue messages during hot actor
     * migration
     */

    /* cloning workflow in 4 easy steps !
     */
    if (tag == BSAL_ACTOR_CLONE) {

        if (self->cloning_status == BSAL_ACTOR_STATUS_NOT_STARTED) {

            /* begin the cloning operation */
            bsal_actor_clone(self, message);

        } else {
            /* queue the cloning message */
#ifdef BSAL_ACTOR_DEBUG_MIGRATE
            printf("DEBUG bsal_actor_receive_system queuing message %d because cloning is in progress\n",
                            tag);
#endif

            self->forwarding_selector = BSAL_ACTOR_FORWARDING_CLONE;
            bsal_actor_queue_message(self, message);
        }

        return 1;

    } else if (self->cloning_status == BSAL_ACTOR_STATUS_STARTED) {

        /* call a function called
         * bsal_actor_continue_clone
         */
        self->cloning_progressed = 0;
        bsal_actor_continue_clone(self, message);

        if (self->cloning_progressed) {
            return 1;
        }
    }

    if (self->migration_status == BSAL_ACTOR_STATUS_STARTED) {

        self->migration_progressed = 0;
        bsal_actor_migrate(self, message);

        if (self->migration_progressed) {
            return 1;
        }
    }

    /* spawn an actor.
     * This works even during migration because the supervisor is the
     * source of BSAL_ACTOR_SPAWN...
     */
    if (tag == BSAL_ACTOR_SPAWN) {
        script = *(int *)buffer;
        spawned = bsal_actor_spawn_real(self, script);

#ifdef BSAL_ACTOR_DEBUG_SPAWN
        printf("DEBUG setting supervisor of %d to %d\n", spawned, source);
#endif

        bsal_node_set_supervisor(bsal_actor_node(self), spawned, source);

        new_buffer = bsal_memory_pool_allocate(ephemeral_memory, 2 * sizeof(int));
        offset = 0;

        bytes = sizeof(spawned);
        memcpy((char *)new_buffer + offset, &spawned, bytes);
        offset += bytes;
        bytes = sizeof(script);
        memcpy((char *)new_buffer + offset, &script, bytes);
        offset += bytes;

        new_count = offset;
        bsal_message_init(&new_message, BSAL_ACTOR_SPAWN_REPLY, new_count, new_buffer);
        bsal_actor_send(self, source, &new_message);

        bsal_message_destroy(&new_message);
        bsal_memory_pool_free(ephemeral_memory, new_buffer);

        return 1;

    } else if (tag == BSAL_ACTOR_MIGRATE_NOTIFY_ACQUAINTANCES) {
        bsal_actor_migrate_notify_acquaintances(self, message);
        return 1;

    } else if (tag == BSAL_ACTOR_NOTIFY_NAME_CHANGE) {

        bsal_actor_notify_name_change(self, message);
        return 1;

    } else if (tag == BSAL_ACTOR_NOTIFY_NAME_CHANGE_REPLY) {

        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_MIGRATE_NOTIFY_ACQUAINTANCES);

        return 1;

    } else if (tag == BSAL_ACTOR_PACK && source == name) {
        /* BSAL_ACTOR_PACK has to go through during live migration
         */
        return 0;

    } else if (tag == BSAL_ACTOR_PROXY_MESSAGE) {
        bsal_actor_receive_proxy_message(self, message);
        return 1;

    } else if (tag == BSAL_ACTOR_FORWARD_MESSAGES_REPLY) {
        /* This message is a system message, it is not for the concrete
         * actor
         */
        return 1;
    }

    /* at this point, the remaining possibilities for the message tag
     * of the current message are all not required to perform
     * actor migration
     */

    /* queue messages during a hot migration
     *
     * BSAL_ACTOR_CLONE messsages are also queued during cloning...
     */
    if (self->migration_status == BSAL_ACTOR_STATUS_STARTED) {

#ifdef BSAL_ACTOR_DEBUG_MIGRATE
        printf("DEBUG bsal_actor_receive_system queuing message %d during migration\n",
                        tag);
#endif
        self->forwarding_selector = BSAL_ACTOR_FORWARDING_MIGRATE;
        bsal_actor_queue_message(self, message);
        return 1;
    }


    /* Perform binomial routing.
     */
    if (tag == BSAL_ACTOR_BINOMIAL_TREE_SEND) {
        bsal_actor_receive_binomial_tree_send(self, message);
        return 1;

    } else if (tag == BSAL_ACTOR_MIGRATE) {

        bsal_actor_migrate(self, message);
        return 1;

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE) {
        /* the concrete actor must catch this one */

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE_REPLY) {
        bsal_actor_receive_synchronize_reply(self, message);

        /* we also allow the concrete actor to receive this */

    /* Ignore BSAL_ACTOR_PIN and BSAL_ACTOR_UNPIN
     * because they can only be sent by an actor
     * to itself.
     */

        /*
    } else if (tag == BSAL_ACTOR_PIN_TO_WORKER) {
        return 1;

    } else if (tag == BSAL_ACTOR_UNPIN_FROM_WORKER) {
        return 1;

    } else if (tag == BSAL_ACTOR_PIN_TO_NODE) {
        return 1;

    } else if (tag == BSAL_ACTOR_UNPIN_FROM_NODE) {
        return 1;
*/
    } else if (tag == BSAL_ACTOR_SET_SUPERVISOR
                    /*&& source == bsal_actor_supervisor(self)*/) {

    /* only an actor that knows the name of
     * the current supervisor can assign a new supervisor
     * this information can not be obtained by default
     * for security reasons.
     */

        if (count != 2 * sizeof(int)) {
            return 1;
        }

        bsal_message_helper_unpack_int(message, 0, &old_supervisor);
        bsal_message_helper_unpack_int(message, sizeof(old_supervisor), &supervisor);

#ifdef BSAL_ACTOR_DEBUG_MIGRATE
        printf("DEBUG bsal_actor_receive_system actor %d receives BSAL_ACTOR_SET_SUPERVISOR old supervisor %d (provided %d), new supervisor %d\n",
                        bsal_actor_name(self),
                        bsal_actor_supervisor(self), old_supervisor,
                        supervisor);
#endif

        if (bsal_actor_supervisor(self) == old_supervisor) {

#ifdef BSAL_ACTOR_DEBUG_MIGRATE
            printf("DEBUG bsal_actor_receive_system authentification successful\n");
#endif
            bsal_actor_set_supervisor(self, supervisor);
        }

        bsal_actor_send_reply_empty(self, BSAL_ACTOR_SET_SUPERVISOR_REPLY);

        return 1;

    /* BSAL_ACTOR_SYNCHRONIZE can not be queued.
     */
    } else if (tag == BSAL_ACTOR_SYNCHRONIZE) {
        return 1;

    /* BSAL_ACTOR_SYNCHRONIZED can only be sent to an actor
     * by itself.
     */
    } else if (tag == BSAL_ACTOR_SYNCHRONIZED && name != source) {
        return 1;

    /* block BSAL_ACTOR_STOP if it is not from self
     * acquaintance actors have to use BSAL_ACTOR_ASK_TO_STOP
     * in fact, this message has to be sent by the actor
     * itself.
     */
    } else if (tag == BSAL_ACTOR_STOP) {

        return 1;

    } else if (tag == BSAL_ACTOR_GET_NODE_NAME) {

        bsal_actor_send_reply_int(self, BSAL_ACTOR_GET_NODE_NAME_REPLY,
                        bsal_actor_node_name(self));
        return 1;

    } else if (tag == BSAL_ACTOR_GET_NODE_WORKER_COUNT) {

        bsal_actor_send_reply_int(self, BSAL_ACTOR_GET_NODE_WORKER_COUNT_REPLY,
                        bsal_actor_node_worker_count(self));
        return 1;

    } else  if (tag == BSAL_ACTOR_FORWARD_MESSAGES) {

        bsal_actor_forward_messages(self, message);
        return 1;

    }



    return 0;
}

void bsal_actor_receive(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_receive_fn_t receive;
    int name;
    int source;
    int *bucket;

#ifdef BSAL_ACTOR_DEBUG_SYNC
    printf("\nDEBUG bsal_actor_receive...... tag %d\n",
                    bsal_message_tag(message));

    if (bsal_message_tag(message) == BSAL_ACTOR_SYNCHRONIZED) {
        printf("DEBUG =============\n");
        printf("DEBUG bsal_actor_receive before concrete receive BSAL_ACTOR_SYNCHRONIZED\n");
    }

    printf("DEBUG bsal_actor_receive tag %d for %d\n",
                    bsal_message_tag(message),
                    bsal_actor_name(self));
#endif

    /* Update counter
     */
    source = bsal_message_source(message);

    self->current_source = source;
    bucket = (int *)bsal_map_get(&self->received_messages, &source);

    if (bucket == NULL) {
        bucket = (int *)bsal_map_add(&self->received_messages, &source);
        (*bucket) = 0;
    }

    (*bucket)++;

    /* check if this is a message that the system can
     * figure out what to do with it
     */
    if (bsal_actor_receive_system(self, message)) {
        return;

#ifdef BSAL_ACTOR_DO_DISPATCH_IN_ABSTRACT_ACTOR
    /* otherwise, verify if the actor registered a
     * handler for this tag
     */
    } else if (bsal_actor_call_handler(self, message)) {
        return;
#endif
    }


    /* Otherwise, this is a message for the actor itself.
     */
    receive = bsal_actor_get_receive(self);

    BSAL_DEBUGGER_ASSERT(receive != NULL);

#ifdef BSAL_ACTOR_DEBUG_SYNC
    printf("DEBUG bsal_actor_receive calls concrete receive tag %d\n",
                    bsal_message_tag(message));
#endif

    name = bsal_actor_name(self);

    /* update counters
     */
    if (source == name) {
        bsal_counter_add(&self->counter, BSAL_COUNTER_RECEIVED_MESSAGES_FROM_SELF, 1);
        bsal_counter_add(&self->counter, BSAL_COUNTER_RECEIVED_BYTES_FROM_SELF,
                        bsal_message_count(message));
    } else {
        bsal_counter_add(&self->counter, BSAL_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF, 1);
        bsal_counter_add(&self->counter, BSAL_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF,
                        bsal_message_count(message));
    }

    receive(self, message);
}

void bsal_actor_receive_proxy_message(struct bsal_actor *self,
                struct bsal_message *message)
{
    int source;

    source = bsal_actor_unpack_proxy_message(self, message);
    bsal_actor_send_with_source(self, bsal_actor_name(self),
                    message, source);
}

void bsal_actor_receive_synchronize(struct bsal_actor *self,
                struct bsal_message *message)
{

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG56 replying to %i with BSAL_ACTOR_PRIVATE_SYNCHRONIZE_REPLY\n",
                    bsal_message_source(message));
#endif

    bsal_message_init(message, BSAL_ACTOR_SYNCHRONIZE_REPLY, 0, NULL);
    bsal_actor_send(self, bsal_message_source(message), message);

    bsal_message_destroy(message);
}

void bsal_actor_receive_synchronize_reply(struct bsal_actor *self,
                struct bsal_message *message)
{
    int name;

    if (self->synchronization_started) {

#ifdef BSAL_ACTOR_DEBUG
        printf("DEBUG99 synchronization_reply %i/%i\n",
                        self->synchronization_responses,
                        self->synchronization_expected_responses);
#endif

        self->synchronization_responses++;

        /* send BSAL_ACTOR_SYNCHRONIZED to self
         */
        if (bsal_actor_synchronization_completed(self)) {

#ifdef BSAL_ACTOR_DEBUG_SYNC
            printf("DEBUG sending BSAL_ACTOR_SYNCHRONIZED to self\n");
#endif
            struct bsal_message new_message;
            bsal_message_init(&new_message, BSAL_ACTOR_SYNCHRONIZED,
                            sizeof(self->synchronization_responses),
                            &self->synchronization_responses);

            name = bsal_actor_name(self);

            bsal_actor_send(self, name, &new_message);
            self->synchronization_started = 0;
        }
    }
}

void bsal_actor_synchronize(struct bsal_actor *self, struct bsal_vector *actors)
{
    struct bsal_message message;

    self->synchronization_started = 1;
    self->synchronization_expected_responses = bsal_vector_size(actors);
    self->synchronization_responses = 0;

    /* emit synchronization
     */

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG actor %i emit synchronization %i-%i, expected: %i\n",
                    bsal_actor_name(self), first, last,
                    self->synchronization_expected_responses);
#endif

    bsal_message_init(&message, BSAL_ACTOR_SYNCHRONIZE, 0, NULL);

    /* TODO bsal_actor_send_range_binomial_tree is broken */
    bsal_actor_send_range(self, actors, &message);
    bsal_message_destroy(&message);
}

int bsal_actor_synchronization_completed(struct bsal_actor *self)
{
    if (self->synchronization_started == 0) {
        return 0;
    }

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG32 actor %i bsal_actor_synchronization_completed %i/%i\n",
                    bsal_actor_name(self),
                    self->synchronization_responses,
                    self->synchronization_expected_responses);
#endif

    if (self->synchronization_responses == self->synchronization_expected_responses) {
        return 1;
    }

    return 0;
}

int bsal_actor_unpack_proxy_message(struct bsal_actor *self,
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

    /* TODO, verify if it is new_count or old count
     */
    bsal_message_init(message, tag, new_count, buffer);

    return source;
}

void bsal_actor_pack_proxy_message(struct bsal_actor *self, struct bsal_message *message,
                int real_source)
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
    new_buffer = bsal_memory_allocate(new_count);

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG12 bsal_memory_pool_allocate %p (pack proxy message)\n",
                    new_buffer);
#endif

    memcpy(new_buffer, buffer, count);

    offset = count;
    memcpy((char *)new_buffer + offset, &real_source, sizeof(real_source));
    offset += sizeof(real_source);
    memcpy((char *)new_buffer + offset, &real_tag, sizeof(real_tag));
    offset += sizeof(real_tag);

    bsal_message_init(message, BSAL_ACTOR_PROXY_MESSAGE, new_count, new_buffer);
    bsal_message_set_source(message, real_source);

    /* free the old buffer
     */
    bsal_memory_free(buffer);
    buffer = NULL;
}

int bsal_actor_script(struct bsal_actor *self)
{
    BSAL_DEBUGGER_ASSERT(self != NULL);

    BSAL_DEBUGGER_ASSERT(self->script != NULL);

    return bsal_script_identifier(self->script);
}

void bsal_actor_add_script(struct bsal_actor *self, int name, struct bsal_script *script)
{
    bsal_node_add_script(bsal_actor_node(self), name, script);
}

void bsal_actor_clone(struct bsal_actor *self, struct bsal_message *message)
{
    int spawner;
    void *buffer;
    int script;
    struct bsal_message new_message;
    int source;

    script = bsal_actor_script(self);
    source = bsal_message_source(message);
    buffer = bsal_message_buffer(message);
    spawner = *(int *)buffer;
    self->cloning_spawner = spawner;
    self->cloning_client = source;

#ifdef BSAL_ACTOR_DEBUG_CLONE
    int name;
    name = bsal_actor_name(self);
    printf("DEBUG %d sending BSAL_ACTOR_SPAWN to spawner %d for client %d\n", name, spawner,
                    source);
#endif

    bsal_message_init(&new_message, BSAL_ACTOR_SPAWN, sizeof(script), &script);
    bsal_actor_send(self, spawner, &new_message);

    self->cloning_status = BSAL_ACTOR_STATUS_STARTED;
}

void bsal_actor_continue_clone(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    int source;
    int self_name;
    struct bsal_message new_message;
    int count;
    void *buffer;

    count = bsal_message_count(message);
    buffer = bsal_message_buffer(message);
    self_name = bsal_actor_name(self);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);

#ifdef BSAL_ACTOR_DEBUG_CLONE1
    printf("DEBUG bsal_actor_continue_clone source %d tag %d\n", source, tag);
#endif

    if (tag == BSAL_ACTOR_SPAWN_REPLY && source == self->cloning_spawner) {

        self->cloning_new_actor = *(int *)buffer;

#ifdef BSAL_ACTOR_DEBUG_CLONE
        printf("DEBUG bsal_actor_continue_clone BSAL_ACTOR_SPAWN_REPLY NEW ACTOR IS %d\n",
                        self->cloning_new_actor);
#endif


        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_PACK);

        self->cloning_progressed = 1;

    } else if (tag == BSAL_ACTOR_PACK_REPLY && source == self_name) {

#ifdef BSAL_ACTOR_DEBUG_CLONE
        printf("DEBUG bsal_actor_continue_clone BSAL_ACTOR_PACK_REPLY sending UNPACK to %d\n",
                         self->cloning_new_actor);
#endif

        /* forward the buffer to the new actor */
        bsal_message_init(&new_message, BSAL_ACTOR_UNPACK, count, buffer);
        bsal_actor_send(self, self->cloning_new_actor, &new_message);

        self->cloning_progressed = 1;

        bsal_message_destroy(&new_message);

    } else if (tag == BSAL_ACTOR_UNPACK_REPLY && source == self->cloning_new_actor) {

            /*
    } else if (tag == BSAL_ACTOR_FORWARD_MESSAGES_REPLY) {
#ifdef BSAL_ACTOR_DEBUG_CLONE
        printf("DEBUG bsal_actor_continue_clone BSAL_ACTOR_UNPACK_REPLY\n");
#endif
*/
        /* it is required that the cloning process be concluded at this point because otherwise
         * queued messages will be queued when they are being forwarded.
         */
        bsal_message_init(&new_message, BSAL_ACTOR_CLONE_REPLY, sizeof(self->cloning_new_actor),
                        &self->cloning_new_actor);
        bsal_actor_send(self, self->cloning_client, &new_message);

        /* we are ready for another cloning */
        self->cloning_status = BSAL_ACTOR_STATUS_NOT_STARTED;

#ifdef BSAL_ACTOR_DEBUG_CLONE
        printf("actor:%d sends clone %d to client %d\n", bsal_actor_name(self),
                        self->cloning_new_actor, self->cloning_client);
#endif

        self->forwarding_selector = BSAL_ACTOR_FORWARDING_CLONE;

#ifdef BSAL_ACTOR_DEBUG_CLONE
        printf("DEBUG clone finished, forwarding queued messages (if any) to %d, queue/%d\n",
                        bsal_actor_name(self), self->forwarding_selector);
#endif

        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_FORWARD_MESSAGES);

        self->cloning_progressed = 1;
    }
}

int bsal_actor_source(struct bsal_actor *self)
{
    return self->current_source;
}

int bsal_actor_node_name(struct bsal_actor *self)
{
    return bsal_node_name(bsal_actor_node(self));
}

int bsal_actor_get_node_count(struct bsal_actor *self)
{
    return bsal_node_nodes(bsal_actor_node(self));
}

int bsal_actor_node_worker_count(struct bsal_actor *self)
{
    return bsal_node_worker_count(bsal_actor_node(self));
}

int bsal_actor_call_handler(struct bsal_actor *self, struct bsal_message *message)
{

#ifdef BSAL_ACTOR_DEBUG_10335
    if (bsal_message_tag(message) == 10335) {
        printf("DEBUG actor %d bsal_actor_dispatch 10335\n",
                        bsal_actor_name(self));
    }
#endif

    return bsal_dispatcher_dispatch(&self->dispatcher, self, message);
}

void bsal_actor_add_route(struct bsal_actor *self, int tag, bsal_actor_receive_fn_t handler)
{

#ifdef BSAL_ACTOR_DEBUG_10335
    if (tag == 10335) {
        printf("DEBUG actor %d bsal_actor_register 10335\n",
                        bsal_actor_name(self));
    }
#endif

    bsal_actor_add_route_with_source(self, tag, handler, BSAL_ACTOR_ANYBODY);
}

void bsal_actor_add_route_with_source(struct bsal_actor *self, int tag, bsal_actor_receive_fn_t handler,
                int source)
{
    bsal_actor_add_route_with_source_and_condition(self, tag,
                    handler, source, NULL, -1);
}

struct bsal_dispatcher *bsal_actor_dispatcher(struct bsal_actor *self)
{
    return &self->dispatcher;
}

void bsal_actor_set_node(struct bsal_actor *self, struct bsal_node *node)
{
    self->node = node;
}

void bsal_actor_migrate(struct bsal_actor *self, struct bsal_message *message)
{
    int spawner;
    void *buffer;
    int source;
    int tag;
    int name;
    struct bsal_message new_message;
    int data[2];
    int selector;

    tag = bsal_message_tag(message);
    source = bsal_message_source(message);
    name = bsal_actor_name(self);

    /*
     * For migration, the same name is kept
     */

    bsal_actor_send_reply_int(self, BSAL_ACTOR_MIGRATE_REPLY,
                    bsal_actor_name(self));

    return;

    if (self->migration_cloned == 0) {

#ifdef BSAL_ACTOR_DEBUG_MIGRATE
        printf("DEBUG bsal_actor_migrate bsal_actor_migrate: cloning self...\n");
#endif

        /* clone self
         */
        source = bsal_message_source(message);
        buffer = bsal_message_buffer(message);
        spawner = *(int *)buffer;
        name = bsal_actor_name(self);

        self->migration_spawner = spawner;
        self->migration_client = source;

        bsal_actor_send_to_self_int(self, BSAL_ACTOR_CLONE, spawner);

        self->migration_status = BSAL_ACTOR_STATUS_STARTED;
        self->migration_cloned = 1;

        self->migration_progressed = 1;

    } else if (tag == BSAL_ACTOR_CLONE_REPLY && source == name) {

#ifdef BSAL_ACTOR_DEBUG_MIGRATE
        printf("DEBUG bsal_actor_migrate bsal_actor_migrate: cloned.\n");
#endif

        /* tell acquaintances that the clone is the new original.
         */
        bsal_message_helper_unpack_int(message, 0, &self->migration_new_actor);

        self->acquaintance_index = 0;
        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_MIGRATE_NOTIFY_ACQUAINTANCES);

#ifdef BSAL_ACTOR_DEBUG_MIGRATE
        printf("DEBUG bsal_actor_migrate: notify acquaintance of name change.\n");
#endif
        self->migration_progressed = 1;

    } else if (tag == BSAL_ACTOR_MIGRATE_NOTIFY_ACQUAINTANCES_REPLY && source == name) {

        /* at this point, there should not be any new messages addressed
         * to the old name if all the code implied uses
         * acquaintance vectors.
         */
        /* assign the supervisor of the original version
         * of the migrated actor to the new version
         * of the migrated actor
         */
#ifdef BSAL_ACTOR_DEBUG_MIGRATE
        printf("DEBUG bsal_actor_migrate actor %d setting supervisor of %d to %d\n",
                        bsal_actor_name(self),
                        self->migration_new_actor,
                        bsal_actor_supervisor(self));
#endif

        data[0] = bsal_actor_name(self);
        data[1] = bsal_actor_supervisor(self);

        bsal_message_init(&new_message, BSAL_ACTOR_SET_SUPERVISOR,
                        2 * sizeof(int), data);
        bsal_actor_send(self, self->migration_new_actor, &new_message);
        bsal_message_destroy(&new_message);

        self->migration_progressed = 1;

    } else if (tag == BSAL_ACTOR_FORWARD_MESSAGES_REPLY && self->migration_forwarded_messages) {

        /* send the name of the new copy and die of a peaceful death.
         */
        bsal_actor_send_int(self, self->migration_client, BSAL_ACTOR_MIGRATE_REPLY,
                        self->migration_new_actor);

        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_STOP);

        self->migration_status = BSAL_ACTOR_STATUS_NOT_STARTED;

#ifdef BSAL_ACTOR_DEBUG_MIGRATE
        printf("DEBUG bsal_actor_migrate: OK, now killing self and returning clone name to client.\n");
#endif

        self->migration_progressed = 1;

    } else if (tag == BSAL_ACTOR_SET_SUPERVISOR_REPLY
                    && source == self->migration_new_actor) {

        if (self->forwarding_selector == BSAL_ACTOR_FORWARDING_NONE) {

            /* the forwarding system is ready to be used.
             */
            self->forwarding_selector = BSAL_ACTOR_FORWARDING_MIGRATE;

#ifdef BSAL_ACTOR_DEBUG_MIGRATE
            printf("DEBUG bsal_actor_migrate %d forwarding queued messages to actor %d, queue/%d (forwarding system ready.)\n",
                        bsal_actor_name(self),
                        self->migration_new_actor, self->forwarding_selector);
#endif

            bsal_actor_send_to_self_empty(self, BSAL_ACTOR_FORWARD_MESSAGES);
            self->migration_forwarded_messages = 1;

        /* wait for the clone queue to be empty.
         */
        } else {

#ifdef BSAL_ACTOR_DEBUG_MIGRATE
            printf("DEBUG bsal_actor_migrate queuing system is busy (used by queue/%d), queuing selector\n",
                            self->forwarding_selector);
#endif
            /* queue the selector into the forwarding system
             */
            selector = BSAL_ACTOR_FORWARDING_MIGRATE;
            bsal_queue_enqueue(&self->forwarding_queue, &selector);
        }

        self->migration_progressed = 1;
    }
}

void bsal_actor_notify_name_change(struct bsal_actor *self, struct bsal_message *message)
{
    int old_name;
    int new_name;
    int source;
    int index;
    int *bucket;
    struct bsal_message new_message;
    int enqueued_messages;

    source = bsal_message_source(message);
    old_name = source;
    bsal_message_helper_unpack_int(message, 0, &new_name);

    /* update the acquaintance vector
     */
    index = bsal_actor_get_acquaintance_index_private(self, old_name);

    bucket = bsal_vector_at(&self->acquaintance_vector, index);

    /*
     * Change it only if it exists
     */
    if (bucket != NULL) {
        *bucket = new_name;
    }

    /* update userland queued messages
     */
    enqueued_messages = bsal_actor_enqueued_message_count(self);

    while (enqueued_messages--) {

        bsal_actor_dequeue_message(self, &new_message);
        if (bsal_message_source(&new_message) == old_name) {
            bsal_message_set_source(&new_message, new_name);
        }
        bsal_actor_enqueue_message(self, &new_message);
    }

    bsal_actor_send_reply_empty(self, BSAL_ACTOR_NOTIFY_NAME_CHANGE_REPLY);
}

void bsal_actor_migrate_notify_acquaintances(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_vector *acquaintance_vector;
    int acquaintance;

    acquaintance_vector = bsal_actor_acquaintance_vector_private(self);

    if (self->acquaintance_index < bsal_vector_size(acquaintance_vector)) {

        acquaintance = bsal_vector_helper_at_as_int(acquaintance_vector, self->acquaintance_index);
        bsal_actor_send_int(self, acquaintance, BSAL_ACTOR_NOTIFY_NAME_CHANGE,
                        self->migration_new_actor);
        self->acquaintance_index++;

    } else {

        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_MIGRATE_NOTIFY_ACQUAINTANCES_REPLY);
    }
}

void bsal_actor_send_to_self_proxy(struct bsal_actor *self,
                struct bsal_message *message, int real_source)
{
    int destination;

    destination = bsal_actor_name(self);
    bsal_actor_send_proxy(self, destination, message, real_source);
}

void bsal_actor_send_proxy(struct bsal_actor *self, int destination,
                struct bsal_message *message, int real_source)
{
    struct bsal_message new_message;

    bsal_message_init_copy(&new_message, message);

    bsal_actor_pack_proxy_message(self, &new_message, real_source);
    bsal_actor_send(self, destination, &new_message);

    bsal_message_destroy(&new_message);
}

void bsal_actor_queue_message(struct bsal_actor *self,
                struct bsal_message *message)
{
    void *buffer;
    void *new_buffer;
    int count;
    struct bsal_message new_message;
    struct bsal_queue *queue;
    int tag;
    int source;

    bsal_message_helper_get_all(message, &tag, &count, &buffer, &source);

    new_buffer = NULL;

#ifdef BSAL_ACTOR_DEBUG_MIGRATE
    printf("DEBUG bsal_actor_queue_message queuing message tag= %d to queue queue/%d\n",
                        tag, self->forwarding_selector);
#endif

    if (count > 0) {
        new_buffer = bsal_memory_allocate(count);
        memcpy(new_buffer, buffer, count);
    }

    bsal_message_init(&new_message, tag, count, new_buffer);
    bsal_message_set_source(&new_message,
                    bsal_message_source(message));
    bsal_message_set_destination(&new_message,
                    bsal_message_destination(message));

    queue = NULL;

    if (self->forwarding_selector == BSAL_ACTOR_FORWARDING_CLONE) {
        queue = &self->queued_messages_for_clone;
    } else if (self->forwarding_selector == BSAL_ACTOR_FORWARDING_MIGRATE) {

        queue = &self->queued_messages_for_migration;
    }

    bsal_queue_enqueue(queue, &new_message);
}

void bsal_actor_forward_messages(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_message new_message;
    struct bsal_queue *queue;
    int destination;
    void *buffer_to_release;

    queue = NULL;
    destination = -1;

#ifdef BSAL_ACTOR_DEBUG_FORWARDING
    printf("DEBUG bsal_actor_forward_messages using queue/%d\n",
                    self->forwarding_selector);
#endif

    if (self->forwarding_selector == BSAL_ACTOR_FORWARDING_CLONE) {
        queue = &self->queued_messages_for_clone;
        destination = bsal_actor_name(self);

    } else if (self->forwarding_selector == BSAL_ACTOR_FORWARDING_MIGRATE) {

        queue = &self->queued_messages_for_migration;
        destination = self->migration_new_actor;
    }

    if (queue == NULL) {

#ifdef BSAL_ACTOR_DEBUG_FORWARDING
        printf("DEBUG bsal_actor_forward_messages error could not select queue\n");
#endif
        return;
    }

    if (bsal_queue_dequeue(queue, &new_message)) {

#ifdef BSAL_ACTOR_DEBUG_FORWARDING
        printf("DEBUG bsal_actor_forward_messages actor %d forwarding message to actor %d tag is %d,"
                            " real source is %d\n",
                            bsal_actor_name(self),
                            destination,
                            bsal_message_tag(&new_message),
                            bsal_message_source(&new_message));
#endif

        bsal_actor_pack_proxy_message(self, &new_message,
                        bsal_message_source(&new_message));
        bsal_actor_send(self, destination, &new_message);

        buffer_to_release = bsal_message_buffer(&new_message);
        bsal_memory_free(buffer_to_release);

        /* recursive actor call
         */
        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_FORWARD_MESSAGES);
    } else {

#ifdef BSAL_ACTOR_DEBUG_FORWARDING
        printf("DEBUG bsal_actor_forward_messages actor %d has no more messages to forward in queue/%d\n",
                        bsal_actor_name(self), self->forwarding_selector);
#endif

        if (bsal_queue_dequeue(&self->forwarding_queue, &self->forwarding_selector)) {

#ifdef BSAL_ACTOR_DEBUG_FORWARDING
            printf("DEBUG bsal_actor_forward_messages will now using queue (FIFO pop)/%d\n",
                            self->forwarding_selector);
#endif
            if (self->forwarding_selector == BSAL_ACTOR_FORWARDING_MIGRATE) {
                self->migration_forwarded_messages = 1;
            }

            /* do a recursive call
             */
            bsal_actor_send_to_self_empty(self, BSAL_ACTOR_FORWARD_MESSAGES);
        } else {

#ifdef BSAL_ACTOR_DEBUG_FORWARDING
            printf("DEBUG bsal_actor_forward_messages the forwarding system is now available.\n");
#endif

            self->forwarding_selector = BSAL_ACTOR_FORWARDING_NONE;
            /* this is finished
             */
            bsal_actor_send_to_self_empty(self, BSAL_ACTOR_FORWARD_MESSAGES_REPLY);

        }
    }
}

void bsal_actor_pin_to_node(struct bsal_actor *self)
{

}

void bsal_actor_unpin_from_node(struct bsal_actor *self)
{

}

int bsal_actor_acquaintance_count(struct bsal_actor *self)
{
    return bsal_vector_size(&self->acquaintance_vector);
}

int bsal_actor_get_child(struct bsal_actor *self, int index)
{
    int index2;

    if (index < bsal_vector_size(&self->children)) {
        index2 = *(int *)bsal_vector_at(&self->children, index);
        return bsal_actor_get_acquaintance_private(self, index2);
    }

    return BSAL_ACTOR_NOBODY;
}

int bsal_actor_child_count(struct bsal_actor *self)
{
    return bsal_vector_size(&self->children);
}

int bsal_actor_add_child(struct bsal_actor *self, int name)
{
    int index;

    index = bsal_actor_add_acquaintance_private(self, name);
    bsal_vector_push_back(&self->children, &index);

    return index;
}

int bsal_actor_add_acquaintance_private(struct bsal_actor *self, int name)
{
    int index;
    int *bucket;

    index = bsal_actor_get_acquaintance_index_private(self, name);

    if (index >= 0) {
        return index;
    }

    if (name == BSAL_ACTOR_NOBODY || name < 0) {
        return -1;
    }

    bsal_vector_helper_push_back_int(bsal_actor_acquaintance_vector_private(self),
                    name);

    index = bsal_vector_size(bsal_actor_acquaintance_vector_private(self)) - 1;

    bucket = bsal_map_add(&self->acquaintance_map, &name);
    *bucket = index;

    return index;
}

int bsal_actor_get_acquaintance_index_private(struct bsal_actor *self, int name)
{
    int *bucket;

#if 0
    return bsal_vector_index_of(&self->acquaintance_vector, &name);
#endif

    bucket = bsal_map_get(&self->acquaintance_map, &name);

    if (bucket == NULL) {
        return -1;
    }

    return *bucket;
}

int bsal_actor_get_child_index(struct bsal_actor *self, int name)
{
    int i;
    int index;
    int child;

    if (name == BSAL_ACTOR_NOBODY) {
        return -1;
    }

    for (i = 0; i < bsal_actor_child_count(self); i++) {
        index = *(int *)bsal_vector_at(&self->children, i);

#ifdef BSAL_ACTOR_DEBUG_CHILDREN
        printf("DEBUG index %d\n", index);
#endif

        child = bsal_actor_get_acquaintance_private(self, index);

        if (child == name) {
            return i;
        }
    }

    return -1;
}

void bsal_actor_enqueue_message(struct bsal_actor *self, struct bsal_message *message)
{
    void *new_buffer;
    int count;
    void *buffer;
    int source;
    int tag;
    struct bsal_message new_message;
    int destination;

    bsal_message_helper_get_all(message, &tag, &count, &buffer, &source);
    destination = bsal_message_destination(message);

    new_buffer = NULL;

    if (buffer != NULL) {
        new_buffer = bsal_memory_allocate(count);
        memcpy(new_buffer, buffer, count);
    }

    bsal_message_init(&new_message, tag, count, new_buffer);
    bsal_message_set_source(&new_message, source);
    bsal_message_set_destination(&new_message, destination);

    bsal_queue_enqueue(&self->enqueued_messages, &new_message);
    bsal_message_destroy(&new_message);
}

void bsal_actor_dequeue_message(struct bsal_actor *self, struct bsal_message *message)
{
    if (bsal_actor_enqueued_message_count(self) == 0) {
        return;
    }

    bsal_queue_dequeue(&self->enqueued_messages, message);
}

int bsal_actor_enqueued_message_count(struct bsal_actor *self)
{
    return bsal_queue_size(&self->enqueued_messages);
}

struct bsal_map *bsal_actor_get_received_messages(struct bsal_actor *self)
{
    return &self->received_messages;
}

struct bsal_map *bsal_actor_get_sent_messages(struct bsal_actor *self)
{
    return &self->sent_messages;
}

int bsal_actor_enqueue_mailbox_message(struct bsal_actor *self, struct bsal_message *message)
{
    return bsal_fast_ring_push_from_producer(&self->mailbox, message);
}

int bsal_actor_dequeue_mailbox_message(struct bsal_actor *self, struct bsal_message *message)
{
    return bsal_fast_ring_pop_from_consumer(&self->mailbox, message);
}

int bsal_actor_work(struct bsal_actor *self)
{
    struct bsal_message message;
    void *buffer;
    int source_worker;

    if (!bsal_actor_dequeue_mailbox_message(self, &message)) {
        printf("Error, no message...\n");
        bsal_actor_print(self);

        return 0;
    }

    /* Make a copy of the buffer and of the worker
     * because actors can not be trusted.
     */
    buffer = bsal_message_buffer(&message);
    source_worker = bsal_message_get_worker(&message);

    /*
     * Receive the message !
     */
    bsal_actor_receive(self, &message);

    /* Restore the important stuff
     */

    bsal_message_set_buffer(&message, buffer);
    bsal_message_set_worker(&message, source_worker);

    /*
     * Send the buffer back to the source to be recycled.
     */
    bsal_worker_free_message(self->worker, &message);

    return 1;
}

int bsal_actor_get_mailbox_size(struct bsal_actor *self)
{
    if (self->dead) {
        return 0;
    }
    return bsal_fast_ring_size_from_producer(&self->mailbox);
}

int bsal_actor_get_sum_of_received_messages(struct bsal_actor *self)
{
    struct bsal_map_iterator map_iterator;
    struct bsal_map *map;
    int value;
    int messages;

    if (self->dead) {
        return 0;
    }
    map = bsal_actor_get_received_messages(self);

    value = 0;

    bsal_map_iterator_init(&map_iterator, map);

    while (bsal_map_iterator_get_next_key_and_value(&map_iterator, NULL, &messages)) {
        value += messages;
    }

    bsal_map_iterator_destroy(&map_iterator);

    return value;
}

char *bsal_actor_script_name(struct bsal_actor *self)
{
    return bsal_script_name(self->script);
}

void bsal_actor_reset_counters(struct bsal_actor *self)
{
#if 0
    struct bsal_map_iterator map_iterator;
    struct bsal_map *map;
    int name;
    int messages;

    map = bsal_actor_get_received_messages(self);

    bsal_map_iterator_init(&map_iterator, map);

    while (bsal_map_iterator_get_next_key_and_value(&map_iterator, &name, &messages)) {
        messages = 0;
        bsal_map_update_value(map, &name, &messages);
    }

    bsal_map_iterator_destroy(&map_iterator);
#endif

    bsal_map_destroy(&self->received_messages);
    bsal_map_init(&self->received_messages, sizeof(int), sizeof(int));
}

int bsal_actor_get_priority(struct bsal_actor *self)
{
    return self->priority;
}

int bsal_actor_get_source_count(struct bsal_actor *self)
{
    return bsal_map_size(&self->received_messages);
}

void bsal_actor_set_priority(struct bsal_actor *self, int priority)
{
    self->priority = priority;
}

void *bsal_actor_concrete_actor(struct bsal_actor *self)
{
    return self->concrete_actor;
}

struct bsal_worker *bsal_actor_worker(struct bsal_actor *self)
{
    return self->worker;
}

int bsal_actor_dead(struct bsal_actor *self)
{
    return self->dead;
}

/* return 0 if successful
 */
int bsal_actor_trylock(struct bsal_actor *self)
{
    int result;

    result = bsal_lock_trylock(&self->receive_lock);

    if (result == BSAL_LOCK_SUCCESS) {
        self->locked = BSAL_LOCK_LOCKED;
        return result;
    }

    return result;
}

struct bsal_vector *bsal_actor_acquaintance_vector_private(struct bsal_actor *self)
{
    return &self->acquaintance_vector;
}

int bsal_actor_get_acquaintance_private(struct bsal_actor *self, int index)
{
    if (index < bsal_vector_size(bsal_actor_acquaintance_vector_private(self))) {
        return bsal_vector_helper_at_as_int(bsal_actor_acquaintance_vector_private(self),
                        index);
    }

    return BSAL_ACTOR_NOBODY;
}

struct bsal_memory_pool *bsal_actor_get_ephemeral_memory(struct bsal_actor *self)
{
    struct bsal_worker *worker;

    worker = bsal_actor_worker(self);

    if (worker == NULL) {
        return NULL;
    }

    return bsal_worker_get_ephemeral_memory(worker);
}

int bsal_actor_get_spawner(struct bsal_actor *self, struct bsal_vector *spawners)
{
    int actor;

    if (bsal_vector_size(spawners) == 0) {
        return BSAL_ACTOR_NOBODY;
    }

    if (self->spawner_index == BSAL_ACTOR_NO_VALUE) {
        self->spawner_index = bsal_vector_size(spawners) - 1;
    }

    if (self->spawner_index >= bsal_vector_size(spawners)) {
        self->spawner_index = bsal_vector_size(spawners) - 1;
    }

    actor = bsal_vector_helper_at_as_int(spawners, self->spawner_index);

    --self->spawner_index;

    if (self->spawner_index < 0) {

        self->spawner_index = bsal_vector_size(spawners) - 1;
    }

    return actor;
}

struct bsal_script *bsal_actor_get_script(struct bsal_actor *self)
{
    return self->script;
}

void bsal_actor_add_route_with_sources(struct bsal_actor *self, int tag,
                bsal_actor_receive_fn_t handler, struct bsal_vector *sources)
{
    int i;
    int source;
    int size;

    size = bsal_vector_size(sources);

    for (i = 0; i < size; i++) {

        source = bsal_vector_helper_at_as_int(sources, i);

        bsal_actor_add_route_with_source(self, tag, handler, source);
    }
}

void bsal_actor_add_route_with_condition(struct bsal_actor *self, int tag, bsal_actor_receive_fn_t handler, int *actual,
                int expected)
{
    bsal_actor_add_route_with_source_and_condition(self, tag, handler, BSAL_ACTOR_ANYBODY, actual, expected);
}

void bsal_actor_add_route_with_source_and_condition(struct bsal_actor *self, int tag, bsal_actor_receive_fn_t handler, int source,
        int *actual, int expected)
{
    bsal_dispatcher_add_route(&self->dispatcher, tag, handler, source, actual, expected);
}
