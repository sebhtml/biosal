
#include "actor_helper.h"
#include "vector_helper.h"

#include <engine/thorium/actor.h>
#include <engine/thorium/message.h>

#include <core/structures/vector_iterator.h>

#include <core/system/debugger.h>
#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
*/
#define USE_BINOMIAL_TREE

void thorium_actor_send_vector(struct thorium_actor *actor, int destination,
                int tag, struct bsal_vector *vector)
{
    int count;
    struct thorium_message message;
    void *buffer;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    count = bsal_vector_pack_size(vector);
    buffer = bsal_memory_pool_allocate(ephemeral_memory, count);
    bsal_vector_pack(vector, buffer);

    thorium_message_init(&message, tag, count, buffer);

    thorium_actor_send(actor, destination, &message);

    bsal_memory_pool_free(ephemeral_memory, buffer);

    thorium_message_destroy(&message);
}

void thorium_actor_send_reply_vector(struct thorium_actor *actor, int tag, struct bsal_vector *vector)
{
    thorium_actor_send_vector(actor, thorium_actor_source(actor), tag, vector);
}

void thorium_actor_send_reply_empty(struct thorium_actor *actor, int tag)
{
    thorium_actor_send_empty(actor, thorium_actor_source(actor), tag);
}

void thorium_actor_send_to_self_empty(struct thorium_actor *actor, int tag)
{
    thorium_actor_send_empty(actor, thorium_actor_name(actor), tag);
}

void thorium_actor_send_empty(struct thorium_actor *actor, int destination, int tag)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, 0, NULL);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_to_supervisor_empty(struct thorium_actor *actor, int tag)
{
    thorium_actor_send_empty(actor, thorium_actor_supervisor(actor), tag);
}

void thorium_actor_send_to_supervisor_int(struct thorium_actor *actor, int tag, int value)
{
    thorium_actor_send_int(actor, thorium_actor_supervisor(actor), tag, value);
}

void thorium_actor_send_to_self_int(struct thorium_actor *actor, int tag, int value)
{
    thorium_actor_send_int(actor, thorium_actor_name(actor), tag, value);
}

void thorium_actor_send_reply_int(struct thorium_actor *actor, int tag, int value)
{
    thorium_actor_send_int(actor, thorium_actor_source(actor), tag, value);
}

void thorium_actor_send_reply_int64_t(struct thorium_actor *actor, int tag, int64_t value)
{
    thorium_actor_send_int64_t(actor, thorium_actor_source(actor), tag, value);
}

void thorium_actor_send_reply_uint64_t(struct thorium_actor *actor, int tag, uint64_t value)
{
    thorium_actor_send_uint64_t(actor, thorium_actor_source(actor), tag, value);
}

void thorium_actor_send_int(struct thorium_actor *actor, int destination, int tag, int value)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, sizeof(value), &value);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_uint64_t(struct thorium_actor *actor, int destination, int tag, uint64_t value)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, sizeof(value), &value);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_int64_t(struct thorium_actor *actor, int destination, int tag, int64_t value)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, sizeof(value), &value);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
void thorium_actor_get_acquaintances(struct thorium_actor *actor, struct bsal_vector *indices,
                struct bsal_vector *names)
{
    struct bsal_vector_iterator iterator;
    int index;
    int *bucket;
    int name;

#if 0 /* The caller must call the constructor first.
         */
    bsal_vector_init(names, sizeof(int));
#endif

    bsal_vector_iterator_init(&iterator, indices);

    while (bsal_vector_iterator_has_next(&iterator)) {
        bsal_vector_iterator_next(&iterator, (void **)&bucket);

        index = *bucket;
        name = thorium_actor_get_acquaintance(actor, index);

        bsal_vector_push_back(names, &name);
    }

    bsal_vector_iterator_destroy(&iterator);
}

#endif

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
void thorium_actor_add_acquaintances(struct thorium_actor *actor,
                struct bsal_vector *names, struct bsal_vector *indices)
{
    struct bsal_vector_iterator iterator;
    int index;
    int *bucket;
    int name;

    bsal_vector_iterator_init(&iterator, names);

    while (bsal_vector_iterator_has_next(&iterator)) {
        bsal_vector_iterator_next(&iterator, (void **)&bucket);

        name = *bucket;
        index = thorium_actor_add_acquaintance(actor, name);

#ifdef THORIUM_ACTOR_HELPER_DEBUG
        printf("DEBUG thorium_actor_add_acquaintances name %d index %d\n",
                        name, index);
#endif

        bsal_vector_push_back(indices, &index);
    }

    bsal_vector_iterator_destroy(&iterator);

#ifdef THORIUM_ACTOR_HELPER_DEBUG
    bsal_vector_print_int(names);
    printf("\n");
    bsal_vector_print_int(indices);
    printf("\n");
#endif
}
#endif

void thorium_actor_send_range(struct thorium_actor *actor, struct bsal_vector *actors,
                struct thorium_message *message)
{
    int first;
    int last;

    first = 0;
    last = bsal_vector_size(actors) - 1;
        /*
    int real_source;

    real_source = thorium_actor_name(actor);
    */

    thorium_actor_send_range_default(actor, actors, first, last, message);
}

void thorium_actor_send_range_default(struct thorium_actor *actor, struct bsal_vector *actors,
                int first, int last,
                struct thorium_message *message)
{
#ifdef USE_BINOMIAL_TREE
    struct bsal_vector destinations;
    struct bsal_memory_pool *ephemeral_memory;
    int name;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    bsal_vector_init(&destinations, sizeof(int));
    bsal_vector_set_memory_pool(&destinations, ephemeral_memory);
    bsal_vector_copy_range(actors, first, last, &destinations);

    /*
     * Set the source now.
     */
    name = thorium_actor_name(actor);
    thorium_message_set_source(message, name);

    thorium_actor_send_range_binomial_tree(actor, &destinations, message);

    bsal_vector_destroy(&destinations);
#else

    thorium_actor_send_range_loop(actor, actors, first, last, message);
#endif
}

void thorium_actor_send_range_loop(struct thorium_actor *actor, struct bsal_vector *actors,
                int first, int last,
                struct thorium_message *message)
{
    int i;
    int the_actor;

#ifdef THORIUM_ACTOR_DEBUG1
    printf("DEBUG thorium_actor_send_range_default%i-%i\n",
                    first, last);
#endif

    i = first;

    while (i <= last) {

#ifdef THORIUM_ACTOR_DEBUG_20140601_1
        printf("DEBUG sending %d to %d\n",
                       thorium_message_tag(message), i);
#endif
        the_actor = *(int *)bsal_vector_at(actors, i);
        thorium_actor_send(actor, the_actor, message);
        i++;
    }
}

void thorium_actor_send_range_empty(struct thorium_actor *actor, struct bsal_vector *actors,
                int tag)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, 0, NULL);
    thorium_actor_send_range(actor, actors, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_range_int(struct thorium_actor *actor, struct bsal_vector *actors,
                int tag, int value)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, sizeof(value), &value);
    thorium_actor_send_range(actor, actors, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_range_buffer(struct thorium_actor *actor, struct bsal_vector *destinations,
                int tag, int count, void *buffer)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, count, buffer);
    thorium_actor_send_range(actor, destinations, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_range_vector(struct thorium_actor *actor, struct bsal_vector *actors,
                int tag, struct bsal_vector *vector)
{
    struct thorium_message message;
    int count;
    void *buffer;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    count = bsal_vector_pack_size(vector);
    buffer = bsal_memory_pool_allocate(ephemeral_memory, count);
    bsal_vector_pack(vector, buffer);
    thorium_message_init(&message, tag, count, buffer);
    thorium_actor_send_range(actor, actors, &message);
    thorium_message_destroy(&message);

    bsal_memory_pool_free(ephemeral_memory, buffer);
}

void thorium_actor_ask_to_stop(struct thorium_actor *actor, struct thorium_message *message)
{
    /* only the supervisor or self can
     * call this.
     */

    int source = thorium_message_source(message);
    int name = thorium_actor_name(actor);
    int supervisor = thorium_actor_supervisor(actor);

#ifdef THORIUM_ACTOR_STORE_CHILDREN
    int i;
    int child;
#endif

    if (source != name && source != supervisor) {
        printf("actor/%d: permission denied, will not stop (source: %d, name: %d, supervisor: %d\n",
                        thorium_actor_name(actor), source, name, supervisor);
#ifdef THORIUM_ACTOR_HELPER_DEBUG_STOP
#endif
        return;
    }

#ifdef THORIUM_ACTOR_STORE_CHILDREN
    for (i = 0; i < thorium_actor_child_count(actor); i++) {

        child = thorium_actor_get_child(actor, i);

#ifdef THORIUM_ACTOR_HELPER_DEBUG_STOP
        printf("actor/%d tells actor %d to stop\n",
                            thorium_actor_name(actor), child);
#endif

        thorium_actor_send_empty(actor, child, ACTION_ASK_TO_STOP);
    }

#ifdef THORIUM_ACTOR_HELPER_DEBUG_STOP
    printf("DEBUG121212 actor/%d dies\n",
                    thorium_actor_name(actor));

    printf("DEBUG actor/%d send ACTION_STOP to self\n",
                    thorium_actor_name(actor));
#endif

#endif

    /*
     * Stop self only if the message was sent by supervisor or self.
     * This is the default behavior and can be overwritten by
     * the concrete actor
     */
    thorium_actor_send_to_self_empty(actor, ACTION_STOP);

    thorium_actor_send_reply_empty(actor, ACTION_ASK_TO_STOP_REPLY);
}

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
int thorium_actor_get_acquaintance(struct thorium_actor *actor, struct bsal_vector *indices,
                int index)
{
    int index2;

    if (index >= bsal_vector_size(indices)) {
        return THORIUM_ACTOR_NOBODY;
    }

    index2 = bsal_vector_at_as_int(indices, index);

    return thorium_actor_get_acquaintance(actor, index2);
}
#endif

void thorium_actor_send_double(struct thorium_actor *actor, int destination, int tag, double value)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, sizeof(value), &value);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
int thorium_actor_get_acquaintance_index(struct thorium_actor *actor, struct bsal_vector *indices,
                int name)
{
    int i;
    int index;
    int actor_name;
    int size;

    size = bsal_vector_size(indices);


    for (i = 0; i < size; ++i) {
        index = bsal_vector_at_as_int(indices, i);
        actor_name = thorium_actor_get_acquaintance(actor, index);

        if (actor_name == name) {
            return i;
        }
    }

    return -1;
}
#endif

void thorium_actor_send_reply(struct thorium_actor *actor, struct thorium_message *message)
{
    thorium_actor_send(actor, thorium_actor_source(actor), message);
}

void thorium_actor_send_to_self(struct thorium_actor *actor, struct thorium_message *message)
{
    thorium_actor_send(actor, thorium_actor_name(actor), message);
}

void thorium_actor_send_to_supervisor(struct thorium_actor *actor, struct thorium_message *message)
{
    thorium_actor_send(actor, thorium_actor_supervisor(actor), message);
}

void thorium_actor_send_to_self_buffer(struct thorium_actor *actor, int tag, int count, void *buffer)
{
    thorium_actor_send_buffer(actor, thorium_actor_name(actor),
                    tag, count, buffer);
}

void thorium_actor_send_buffer(struct thorium_actor *actor, int destination, int tag,
                int count, void *buffer)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, count, buffer);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_range_positions_vector(struct thorium_actor *actor, struct bsal_vector *actors,
                int first, int last,
                int tag, struct bsal_vector *vector)
{
    struct thorium_message message;
    int count;
    void *buffer;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    count = bsal_vector_pack_size(vector);
    buffer = bsal_memory_pool_allocate(ephemeral_memory, count);
    bsal_vector_pack(vector, buffer);
    thorium_message_init(&message, tag, count, buffer);
    thorium_actor_send_range_default(actor, actors, first, last, &message);
    thorium_message_destroy(&message);

    bsal_memory_pool_free(ephemeral_memory, buffer);
}
