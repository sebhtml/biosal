
#include "actor.h"
#include "worker.h"
#include "node.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#define BSAL_ACTOR_DEBUG

#define BSAL_ACTOR_DEBUG1

#define BSAL_ACTOR_DEBUG_SYNC
#define BSAL_ACTOR_DEBUG_BINOMIAL_TREE

#define BSAL_ACTOR_DEBUG_SPAWN
#define BSAL_ACTOR_DEBUG_CLONE
*/

void bsal_actor_init(struct bsal_actor *actor, void *state,
                struct bsal_script *script)
{
    bsal_actor_init_fn_t init;

    actor->state = state;
    actor->name = -1;
    actor->supervisor = -1;
    actor->dead = 0;
    actor->script = script;
    actor->worker = NULL;

    actor->synchronization_started = 0;
    actor->synchronization_expected_responses = 0;
    actor->synchronization_responses = 0;

    bsal_lock_init(&actor->receive_lock);
    actor->locked = 0;

    bsal_actor_unpin(actor);

    init = bsal_actor_get_init(actor);
    init(actor);

    actor->cloning_status = BSAL_ACTOR_STATUS_NOT_SUPPORTED;
    actor->acquaintance_name_change_status = BSAL_ACTOR_STATUS_NOT_SUPPORTED;
}

void bsal_actor_destroy(struct bsal_actor *actor)
{
    bsal_actor_init_fn_t destroy;

    destroy = bsal_actor_get_destroy(actor);
    destroy(actor);

    actor->name = -1;
    actor->dead = 1;

    bsal_actor_unpin(actor);

    actor->script = NULL;
    actor->worker = NULL;
    actor->state = NULL;

    /* unlock the actor if the actor is being destroyed while
     * being locked
     */
    bsal_actor_unlock(actor);

    bsal_lock_destroy(&actor->receive_lock);

    /* when exiting the destructor, the actor is unlocked
     * and destroyed too
     */
}

int bsal_actor_name(struct bsal_actor *actor)
{
    return actor->name;
}

void *bsal_actor_concrete_actor(struct bsal_actor *actor)
{
    return actor->state;
}

bsal_actor_receive_fn_t bsal_actor_get_receive(struct bsal_actor *actor)
{
    return bsal_script_get_receive(actor->script);
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

    int received = (int)bsal_counter_get(&actor->counter, BSAL_COUNTER_RECEIVED_MESSAGES);
    int sent = (int)bsal_counter_get(&actor->counter, BSAL_COUNTER_SENT_MESSAGES);

    printf("[bsal_actor_print] Name: %i Supervisor %i Node: %i, Thread: %i"
                    " received %i sent %i\n", bsal_actor_name(actor),
                    bsal_actor_supervisor(actor),
                    bsal_node_name(bsal_actor_node(actor)),
                    bsal_worker_name(bsal_actor_worker(actor)),
                    received, sent);
}

bsal_actor_init_fn_t bsal_actor_get_init(struct bsal_actor *actor)
{
    return bsal_script_get_init(actor->script);
}

bsal_actor_destroy_fn_t bsal_actor_get_destroy(struct bsal_actor *actor)
{
    return bsal_script_get_destroy(actor->script);
}

void bsal_actor_set_worker(struct bsal_actor *actor, struct bsal_worker *worker)
{
    actor->worker = worker;
}

int bsal_actor_send_system(struct bsal_actor *actor, int name, struct bsal_message *message)
{
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
            return 1;
        } else if (tag == BSAL_ACTOR_UNPIN) {
            bsal_actor_unpin(actor);
            return 1;
        } else if (tag == BSAL_ACTOR_CLONE_ENABLE) {
            if (actor->cloning_status == BSAL_ACTOR_STATUS_NOT_SUPPORTED) {

#ifdef BSAL_ACTOR_DEBUG_CLONE
                printf("DEBUG enable cloning\n");
#endif

                actor->cloning_status = BSAL_ACTOR_STATUS_SUPPORTED;
                return 1;
            }
        } else if (tag == BSAL_ACTOR_CLONE_DISABLE) {
            if (actor->cloning_status == BSAL_ACTOR_STATUS_SUPPORTED) {
                actor->cloning_status = BSAL_ACTOR_STATUS_NOT_SUPPORTED;
                return 1;
            }
        } else if (tag == BSAL_ACTOR_NOTIFY_NAME_ENABLE) {
            if (actor->acquaintance_name_change_status == BSAL_ACTOR_STATUS_NOT_SUPPORTED) {

                actor->acquaintance_name_change_status = BSAL_ACTOR_STATUS_SUPPORTED;
                return 1;
            }
        } else if (tag == BSAL_ACTOR_NOTIFY_NAME_DISABLE) {
            if (actor->acquaintance_name_change_status == BSAL_ACTOR_STATUS_SUPPORTED) {
                actor->acquaintance_name_change_status = BSAL_ACTOR_STATUS_NOT_SUPPORTED;
                return 1;
            }
        } else if (tag == BSAL_ACTOR_YIELD) {
            bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_YIELD_REPLY);
            return 1;

        } else if (tag == BSAL_ACTOR_STOP) {

            bsal_actor_die(actor);
            return 1;
        }
    }

    if (tag == BSAL_ACTOR_PACK) {
        /* the concrete actor must catch this one */

    } else if (tag == BSAL_ACTOR_PACK_SIZE) {
        /* the concrete actor must catch this one */

    } else if (tag == BSAL_ACTOR_UNPACK) {
        /* the concrete actor must catch this one */

    } else if (tag == BSAL_ACTOR_CLONE_REPLY) {
        /* the concrete actor must catch this one */

    }

    return 0;
}

void bsal_actor_send(struct bsal_actor *actor, int name, struct bsal_message *message)
{
    int source;
    source = bsal_actor_name(actor);

    /* update counters
     */
    if (source == name) {
        bsal_counter_add(&actor->counter, BSAL_COUNTER_SENT_MESSAGES_TO_SELF, 1);
        bsal_counter_add(&actor->counter, BSAL_COUNTER_SENT_BYTES_TO_SELF,
                        bsal_message_count(message));
    } else {
        bsal_counter_add(&actor->counter, BSAL_COUNTER_SENT_MESSAGES_NOT_TO_SELF, 1);
        bsal_counter_add(&actor->counter, BSAL_COUNTER_SENT_BYTES_NOT_TO_SELF,
                        bsal_message_count(message));
    }

    if (bsal_actor_send_system(actor, name, message)) {
        return;
    }

    bsal_actor_send_with_source(actor, name, message, source);
}

void bsal_actor_send_with_source(struct bsal_actor *actor, int name, struct bsal_message *message,
                int source)
{
    bsal_message_set_source(message, source);
    bsal_message_set_destination(message, name);

#ifdef BSAL_ACTOR_DEBUG9
    if (bsal_message_tag(message) == 1100) {
        printf("DEBUG bsal_message_set_source 1100\n");
    }
#endif

    bsal_worker_send(actor->worker, message);
}

int bsal_actor_spawn(struct bsal_actor *actor, int script)
{
    int name;
    int self_name = bsal_actor_name(actor);

#ifdef BSAL_ACTOR_DEBUG_SPAWN
    printf("DEBUG bsal_actor_spawn script %d\n", script);
#endif

    name = bsal_node_spawn(bsal_actor_node(actor), script);

#ifdef BSAL_ACTOR_DEBUG_SPAWN
    printf("DEBUG bsal_actor_spawn before set_supervisor, spawned %d\n",
                    name);
#endif

    bsal_node_set_supervisor(bsal_actor_node(actor), name, self_name);

    bsal_counter_add(&actor->counter, BSAL_COUNTER_SPAWNED_ACTORS, 1);

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
    bsal_counter_add(&actor->counter, BSAL_COUNTER_KILLED_ACTORS, 1);
    actor->dead = 1;
}

struct bsal_counter *bsal_actor_counter(struct bsal_actor *actor)
{
    return &actor->counter;
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
    bsal_lock_lock(&actor->receive_lock);
    actor->locked = 1;
}

void bsal_actor_unlock(struct bsal_actor *actor)
{
    if (!actor->locked) {
        return;
    }

    actor->locked = 0;
    bsal_lock_unlock(&actor->receive_lock);
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

int bsal_actor_receive_system(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int name;
    int source;
    int spawned;
    int script;
    void *buffer;

    tag = bsal_message_tag(message);
    name = bsal_actor_name(actor);
    source =bsal_message_source(message);

    /* Perform binomial routing.
     */
    if (tag == BSAL_ACTOR_BINOMIAL_TREE_SEND) {
        bsal_actor_receive_binomial_tree_send(actor, message);
        return 1;

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE) {
        /* the concrete actor must catch this one */

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE_REPLY) {
        bsal_actor_receive_synchronize_reply(actor, message);

        /* we also allow the concrete actor to receive this */

    } else if (tag == BSAL_ACTOR_PROXY_MESSAGE) {
        bsal_actor_receive_proxy_message(actor, message);
        return 1;

    /* Ignore BSAL_ACTOR_PIN and BSAL_ACTOR_UNPIN
     * because they can only be sent by an actor
     * to itself.
     */
    } else if (tag == BSAL_ACTOR_PIN) {
        return 1;
    } else if (tag == BSAL_ACTOR_UNPIN) {
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
     * acquaintances have to use BSAL_ACTOR_ASK_TO_STOP
     */
    } else if (tag == BSAL_ACTOR_STOP && source != name ) {

        return 1;

    /* spawn an actor
     */
    } else if (tag == BSAL_ACTOR_SPAWN) {
        buffer = bsal_message_buffer(message);
        script = *(int *)buffer;
        spawned = bsal_actor_spawn(actor, script);
        bsal_node_set_supervisor(bsal_actor_node(actor), spawned, source);
        bsal_message_init(message, BSAL_ACTOR_SPAWN_REPLY, sizeof(spawned), &spawned);
        bsal_actor_send(actor, source, message);

        return 1;

    } else if (tag == BSAL_ACTOR_CLONE) {

        /* begin the cloning operation */
        bsal_actor_clone(actor, message);

        return 1;

    } else if (tag == BSAL_ACTOR_NOTIFY_NAME_CHANGE &&
                    actor->acquaintance_name_change_status == BSAL_ACTOR_STATUS_NOT_SUPPORTED) {

        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_NOTIFY_NAME_CHANGE_REPLY);
        return 1;

    } else if (tag == BSAL_ACTOR_GET_NODE_NAME) {

        bsal_actor_send_reply_int(actor, BSAL_ACTOR_GET_NODE_NAME_REPLY,
                        bsal_actor_node_name(actor));
        return 1;

    } else if (tag == BSAL_ACTOR_GET_NODE_WORKER_COUNT) {

        bsal_actor_send_reply_int(actor, BSAL_ACTOR_GET_NODE_WORKER_COUNT_REPLY,
                        bsal_actor_node_worker_count(actor));
        return 1;
    }

    /* cloning workflow in 4 easy steps ! */
    if (actor->cloning_status == BSAL_ACTOR_STATUS_STARTED) {

        /* call a function called
         * bsal_actor_continue_clone
         */
        if (bsal_actor_continue_clone(actor, message)) {
            return 1;
        }
    }

    return 0;
}

void bsal_actor_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    bsal_actor_receive_fn_t receive;
    int name;
    int source;

#ifdef BSAL_ACTOR_DEBUG_SYNC
    printf("\nDEBUG bsal_actor_receive...... tag %d\n",
                    bsal_message_tag(message));

    if (bsal_message_tag(message) == BSAL_ACTOR_SYNCHRONIZED) {
        printf("DEBUG =============\n");
        printf("DEBUG bsal_actor_receive before concrete receive BSAL_ACTOR_SYNCHRONIZED\n");
    }

    printf("DEBUG bsal_actor_receive tag %d for %d\n",
                    bsal_message_tag(message),
                    bsal_actor_name(actor));
#endif

    actor->current_source = bsal_message_source(message);

    if (bsal_actor_receive_system(actor, message)) {
        return;
    }

    /* Otherwise, this is a message for the actor itself.
     */
    receive = bsal_actor_get_receive(actor);

#ifdef BSAL_ACTOR_DEBUG_SYNC
    printf("DEBUG bsal_actor_receive calls concrete receive tag %d\n",
                    bsal_message_tag(message));
#endif

    name = bsal_actor_name(actor);
    source = bsal_message_source(message);

    /* update counters
     */
    if (source == name) {
        bsal_counter_add(&actor->counter, BSAL_COUNTER_RECEIVED_MESSAGES_FROM_SELF, 1);
        bsal_counter_add(&actor->counter, BSAL_COUNTER_RECEIVED_BYTES_FROM_SELF,
                        bsal_message_count(message));
    } else {
        bsal_counter_add(&actor->counter, BSAL_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF, 1);
        bsal_counter_add(&actor->counter, BSAL_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF,
                        bsal_message_count(message));
    }

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
    printf("DEBUG56 replying to %i with BSAL_ACTOR_PRIVATE_SYNCHRONIZE_REPLY\n",
                    bsal_message_source(message));
#endif

    bsal_message_init(message, BSAL_ACTOR_SYNCHRONIZE_REPLY, 0, NULL);
    bsal_actor_send(actor, bsal_message_source(message), message);
}

void bsal_actor_receive_synchronize_reply(struct bsal_actor *actor,
                struct bsal_message *message)
{
    int name;

    if (actor->synchronization_started) {

#ifdef BSAL_ACTOR_DEBUG
        printf("DEBUG99 synchronization_reply %i/%i\n",
                        actor->synchronization_responses,
                        actor->synchronization_expected_responses);
#endif

        actor->synchronization_responses++;

        /* send BSAL_ACTOR_SYNCHRONIZED to self
         */
        if (bsal_actor_synchronization_completed(actor)) {

#ifdef BSAL_ACTOR_DEBUG_SYNC
            printf("DEBUG sending BSAL_ACTOR_SYNCHRONIZED to self\n");
#endif
            struct bsal_message new_message;
            bsal_message_init(&new_message, BSAL_ACTOR_SYNCHRONIZED,
                            sizeof(actor->synchronization_responses),
                            &actor->synchronization_responses);

            name = bsal_actor_name(actor);

            bsal_actor_send(actor, name, &new_message);
            actor->synchronization_started = 0;
        }
    }
}

void bsal_actor_synchronize(struct bsal_actor *actor, struct bsal_vector *actors)
{
    struct bsal_message message;

    actor->synchronization_started = 1;
    actor->synchronization_expected_responses = bsal_vector_size(actors);
    actor->synchronization_responses = 0;

    /* emit synchronization
     */

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG actor %i emit synchronization %i-%i, expected: %i\n",
                    bsal_actor_name(actor), first, last,
                    actor->synchronization_expected_responses);
#endif

    bsal_message_init(&message, BSAL_ACTOR_SYNCHRONIZE, 0, NULL);

    /* TODO bsal_actor_send_range_binomial_tree is broken */
    bsal_actor_send_range_standard(actor, actors, &message);
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
    void *buffer;
    int count;
    int amount;
    int new_count;
    int limit;
    int real_source;
    int offset;
    struct bsal_vector actors;
    int magic_offset;

    limit = 42;
    count = bsal_message_count(message);
    buffer = bsal_message_buffer(message);

    offset = count - 1 - sizeof(magic_offset);

    magic_offset = *(int *)(char *)buffer + offset;
    bsal_vector_unpack(&actors, (char *)buffer + magic_offset);

    new_count = magic_offset;
    new_count -= sizeof(real_source);
    new_count -= sizeof(real_tag);

    offset = new_count;

    real_source = *(int *)((char *)buffer + offset);
    offset += sizeof(real_source);
    real_tag = *(int *)((char *)buffer + offset);
    offset += sizeof(real_tag);

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG78 actor %i received BSAL_ACTOR_BINOMIAL_TREE_SEND "
                    "real_source: %i real_tag: %i first: %i last: %i\n",
                    bsal_actor_name(actor), real_source, real_tag, first,
                    last);
#endif

    amount = bsal_vector_size(&actors);

    bsal_message_init(message, real_tag, new_count, buffer);

    if (amount < limit) {
        bsal_actor_send_range_standard(actor, &actors, message);
    } else {
        bsal_actor_send_range_binomial_tree(actor, &actors, message);
    }

    bsal_vector_destroy(&actors);
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

    /* TODO, verify if it is new_count or old count
     */
    bsal_message_init(message, tag, new_count, buffer);

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

    bsal_message_init(message, BSAL_ACTOR_PROXY_MESSAGE, new_count, new_buffer);
    bsal_message_set_source(message, real_source);
}

void bsal_actor_send_range(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message)
{
    int real_source;

    real_source = bsal_actor_name(actor);

    /*
    bsal_actor_send_range_standard(actor, first, last, message);
    */

    bsal_actor_pack_proxy_message(actor, real_source, message);
    bsal_actor_send_range_binomial_tree(actor, actors, message);
}

void bsal_actor_send_range_standard(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message)
{
    int i;
    int first;
    int last;
    int the_actor;

    first = 0;
    last = bsal_vector_size(actors) - 1;

#ifdef BSAL_ACTOR_DEBUG1
    printf("DEBUG bsal_actor_send_range_standard %i-%i\n",
                    first, last);
#endif

    i = first;

    while (i <= last) {

#ifdef BSAL_ACTOR_DEBUG_20140601_1
        printf("DEBUG sending %d to %d\n",
                       bsal_message_tag(message), i);
#endif
        the_actor = *(int *)bsal_vector_at(actors, i);
        bsal_actor_send(actor, the_actor, message);
        i++;
    }
}

void bsal_actor_send_range_binomial_tree(struct bsal_actor *actor, struct bsal_vector *actors,
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
    int first;
    int last;
    struct bsal_vector left_part;
    struct bsal_vector right_part;
    int left_actor;
    int right_actor;
    struct bsal_message new_message;
    int magic_offset;
    int limit;

    limit = 0;

    if (bsal_vector_size(actors) < limit) {
        bsal_actor_send_range_standard(actor, actors, message);
        return;
    }

    bsal_vector_init(&left_part, sizeof(int));
    bsal_vector_init(&right_part, sizeof(int));

#ifdef BSAL_ACTOR_DEBUG_BINOMIAL_TREE
    int name;

    name = bsal_actor_name(actor);
#endif

    source = bsal_actor_name(actor);
    bsal_message_set_source(message, source);

    first = 0;
    last = bsal_vector_size(actors) - 1;
    middle = first + (last - first) / 2;

#ifdef BSAL_ACTOR_DEBUG_BINOMIAL_TREE
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

    magic_offset = count + sizeof(source) + sizeof(tag);

    if (bsal_vector_size(&left_part) > 0) {
        bsal_vector_copy_range(actors, first1, last1, &left_part);

        /* TODO use slab allocator */
        new_count = count + sizeof(source) + sizeof(tag) + bsal_vector_pack_size(&left_part) + sizeof(magic_offset);
        new_buffer = malloc(new_count);

#ifdef BSAL_ACTOR_DEBUG_BINOMIAL_TREE
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
        bsal_vector_pack(&left_part, (char *)new_buffer + offset);
        offset += bsal_vector_pack_size(&left_part);
        memcpy((char *)new_buffer + offset, &magic_offset, sizeof(magic_offset));
        offset += sizeof(magic_offset);

        bsal_message_init(&new_message, BSAL_ACTOR_BINOMIAL_TREE_SEND, new_count, new_buffer);

    #ifdef BSAL_ACTOR_DEBUG_BINOMIAL_TREE
        printf("DEBUG111111 actor %i sending BSAL_ACTOR_BINOMIAL_TREE_SEND to %i, range is %i-%i\n",
                        name, middle1, first1, last1);
    #endif

        printf("DEBUG left part size: %d\n", bsal_vector_size(&left_part));

        left_actor = *(int *)bsal_vector_at(&left_part, middle1);
        bsal_actor_send(actor, left_actor, &new_message);

        /* restore the buffer for the user */
        free(new_buffer);
        bsal_vector_destroy(&left_part);
    }

    /* send to the right actor
     */

    bsal_vector_copy_range(actors, first2, last2, &right_part);

    if (bsal_vector_size(&right_part) > 0) {

        new_count = count + sizeof(source) + sizeof(tag) + bsal_vector_pack_size(&right_part) + sizeof(magic_offset);
        new_buffer = malloc(new_count);

        memcpy(new_buffer, buffer, count);
        offset = count;
        memcpy((char *)new_buffer + offset, &source, sizeof(source));
        offset += sizeof(source);
        memcpy((char *)new_buffer + offset, &tag, sizeof(tag));
        offset += sizeof(tag);
        bsal_vector_pack(&right_part, (char *)new_buffer + offset);
        offset += bsal_vector_pack_size(&right_part);
        memcpy((char *)new_buffer + offset, &magic_offset, sizeof(magic_offset));
        offset += sizeof(magic_offset);

#ifdef BSAL_ACTOR_DEBUG_BINOMIAL_TREE2
        printf("DEBUG78 %i source: %i tag: %i BSAL_ACTOR_BINOMIAL_TREE_SEND\n",
                    name, source, tag);
#endif

        bsal_message_init(&new_message, BSAL_ACTOR_BINOMIAL_TREE_SEND, new_count, new_buffer);

#ifdef BSAL_ACTOR_DEBUG_BINOMIAL_TREE
        printf("DEBUG %i sending BSAL_ACTOR_BINOMIAL_TREE_SEND to %i, range is %i-%i\n",
                    name, middle2, first2, last2);
#endif

        right_actor = *(int *)bsal_vector_at(&right_part, middle2);
        bsal_actor_send(actor, right_actor, &new_message);

        bsal_vector_destroy(&right_part);
        free(new_buffer);
        new_buffer = NULL;
    }
}

int bsal_actor_script(struct bsal_actor *actor)
{
    return bsal_script_name(actor->script);
}

void bsal_actor_add_script(struct bsal_actor *actor, int name, struct bsal_script *script)
{
    bsal_node_add_script(bsal_actor_node(actor), name, script);
}

void bsal_actor_send_reply_empty(struct bsal_actor *actor, int tag)
{
    bsal_actor_send_empty(actor, bsal_actor_source(actor), tag);
}

void bsal_actor_send_to_self_empty(struct bsal_actor *actor, int tag)
{
    bsal_actor_send_empty(actor, bsal_actor_name(actor), tag);
}

void bsal_actor_send_empty(struct bsal_actor *actor, int destination, int tag)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, 0, NULL);
    bsal_actor_send(actor, destination, &message);
}

void bsal_actor_send_reply(struct bsal_actor *actor, struct bsal_message *message)
{
    bsal_actor_send(actor, bsal_actor_source(actor), message);
}

void bsal_actor_send_to_self(struct bsal_actor *actor, struct bsal_message *message)
{
    bsal_actor_send(actor, bsal_actor_name(actor), message);
}

void bsal_actor_send_range_standard_empty(struct bsal_actor *actor, struct bsal_vector *actors,
                int tag)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, 0, NULL);
    bsal_actor_send_range_standard(actor, actors, &message);
}

void bsal_actor_send_to_supervisor(struct bsal_actor *actor, struct bsal_message *message)
{
    bsal_actor_send(actor, bsal_actor_supervisor(actor), message);
}

void bsal_actor_send_to_supervisor_empty(struct bsal_actor *actor, int tag)
{
    bsal_actor_send_empty(actor, bsal_actor_supervisor(actor), tag);
}

void bsal_actor_clone(struct bsal_actor *actor, struct bsal_message *message)
{
    int spawner;
    void *buffer;
    int script;
    struct bsal_message new_message;
    int source;

    script = bsal_actor_script(actor);
    source = bsal_message_source(message);
    actor->cloning_status = BSAL_ACTOR_STATUS_STARTED;
    buffer = bsal_message_buffer(message);
    spawner = *(int *)buffer;
    actor->cloning_spawner = spawner;
    actor->cloning_client = source;

#ifdef BSAL_ACTOR_DEBUG_CLONE
    int name;
    name = bsal_actor_name(actor);
    printf("DEBUG %d sending BSAL_ACTOR_SPAWN to spawner %d for client %d\n", name, spawner,
                    source);
#endif

    bsal_message_init(&new_message, BSAL_ACTOR_SPAWN, sizeof(script), &script);
    bsal_actor_send(actor, spawner, &new_message);
}

int bsal_actor_continue_clone(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int self;
    struct bsal_message new_message;
    int count;
    void *buffer;

    count = bsal_message_count(message);
    buffer = bsal_message_buffer(message);
    self = bsal_actor_name(actor);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);

#ifdef BSAL_ACTOR_DEBUG_CLONE1
    printf("DEBUG bsal_actor_continue_clone source %d tag %d\n", source, tag);
#endif

    if (tag == BSAL_ACTOR_SPAWN_REPLY && source == actor->cloning_spawner) {

#ifdef BSAL_ACTOR_DEBUG_CLONE
        printf("DEBUG bsal_actor_continue_clone BSAL_ACTOR_SPAWN_REPLY\n");
#endif

        actor->cloning_new_actor = *(int *)buffer;
        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_PACK);

        return 1;

    } else if (tag == BSAL_ACTOR_PACK_REPLY && source == self) {

#ifdef BSAL_ACTOR_DEBUG_CLONE
        printf("DEBUG bsal_actor_continue_clone BSAL_ACTOR_PACK_REPLY\n");
#endif

        /* forward the buffer to the new actor */
        bsal_message_init(&new_message, BSAL_ACTOR_UNPACK, count, buffer);
        bsal_actor_send(actor, actor->cloning_new_actor, &new_message);

        return 1;
    } else if (tag == BSAL_ACTOR_UNPACK_REPLY && source == actor->cloning_new_actor) {

#ifdef BSAL_ACTOR_DEBUG_CLONE
        printf("DEBUG bsal_actor_continue_clone BSAL_ACTOR_UNPACK_REPLY\n");
#endif

        bsal_message_init(&new_message, BSAL_ACTOR_CLONE_REPLY, sizeof(actor->cloning_new_actor),
                        &actor->cloning_new_actor);
        bsal_actor_send(actor, actor->cloning_client, &new_message);

        /* we are ready for another cloning */
        actor->cloning_status = BSAL_ACTOR_STATUS_SUPPORTED;

#ifdef BSAL_ACTOR_DEBUG_CLONE
        printf("actor:%d sends clone %d to client %d\n", bsal_actor_name(actor),
                        actor->cloning_new_actor, actor->cloning_client);
#endif
        return 1;
    }

    return 0;
}

void bsal_actor_send_reply_int(struct bsal_actor *actor, int tag, int value)
{
    bsal_actor_send_int(actor, bsal_actor_source(actor), tag, value);
}

int bsal_actor_source(struct bsal_actor *actor)
{
    return actor->current_source;
}

void bsal_actor_send_int(struct bsal_actor *actor, int destination, int tag, int value)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, sizeof(value), &value);
    bsal_actor_send(actor, destination, &message);
}

void bsal_actor_send_to_supervisor_int(struct bsal_actor *actor, int tag, int value)
{
    bsal_actor_send_int(actor, bsal_actor_supervisor(actor), tag, value);
}

void bsal_actor_send_to_self_int(struct bsal_actor *actor, int tag, int value)
{
    bsal_actor_send_int(actor, bsal_actor_name(actor), tag, value);
}

int bsal_actor_node_name(struct bsal_actor *actor)
{
    return bsal_node_name(bsal_actor_node(actor));
}

int bsal_actor_node_worker_count(struct bsal_actor *actor)
{
    return bsal_node_worker_count(bsal_actor_node(actor));
}
