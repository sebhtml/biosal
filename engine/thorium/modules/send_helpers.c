
#include "send_helpers.h"

#include "binomial_tree_message.h" /* for DEBUG_BINOMIAL_TREE */

#include <engine/thorium/tracepoints/tracepoints.h>

#include <engine/thorium/actor.h>
#include <engine/thorium/message.h>

#include <core/structures/vector_iterator.h>
#include <core/helpers/vector_helper.h>

#include <core/system/debugger.h>
#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Enable the binomial tree subsystem.
 */
/*
#define USE_BINOMIAL_TREE
*/

void thorium_actor_send_vector(struct thorium_actor *actor, int destination,
                int tag, struct core_vector *vector)
{
    int count;
    struct thorium_message message;
    void *buffer;

    count = core_vector_pack_size(vector);
    buffer = thorium_actor_allocate(actor, count);
    core_vector_pack(vector, buffer);

    thorium_message_init(&message, tag, count, buffer);

    thorium_actor_send(actor, destination, &message);

    thorium_message_destroy(&message);
}

void thorium_actor_send_reply_vector(struct thorium_actor *actor, int tag, struct core_vector *vector)
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
#if 0
    void *buffer;

    /*
     * Allocate a buffer for metadata.
     */
    buffer = thorium_actor_allocate(actor, 0);
#endif
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
    void *buffer;
    int count;

    count = sizeof(value);
    buffer = thorium_actor_allocate(actor, count);
    core_memory_copy(buffer, &value, count);

    thorium_message_init(&message, tag, count, buffer);
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

void thorium_actor_send_range(struct thorium_actor *actor, struct core_vector *actors,
                struct thorium_message *message)
{
    int first;
    int last;

    first = 0;
    last = core_vector_size(actors) - 1;
        /*
    int real_source;

    real_source = thorium_actor_name(actor);
    */

    thorium_actor_send_range_default(actor, actors, first, last, message);
}

void thorium_actor_send_range_default(struct thorium_actor *actor, struct core_vector *actors,
                int first, int last,
                struct thorium_message *message)
{
    int use_binomial_tree;
    struct core_vector destinations;
    struct core_memory_pool *ephemeral_memory;
    int name;
    int action;

    action = thorium_message_action(message);
    use_binomial_tree = 0;

#ifdef USE_BINOMIAL_TREE

    /*
     * ACTION_ASK_TO_STOP basically kills actors (if they agree to).
     * It is a bad idea to use a binomial tree to send this death signal
     * since intermediate actors can die before acting as relays.
     */
    if (action != ACTION_ASK_TO_STOP)
        use_binomial_tree = 1;
#endif

    if (!use_binomial_tree) {
        thorium_actor_send_range_loop(actor, actors, first, last, message);
        return;
    }

    tracepoint(thorium_binomial_tree, send_range, message, (int)core_vector_size(actors));

    /*
     * Otherwise, use the binomial tree code path. This algorithm is better since it distributed
     * the sending operations intot a binomial tree where there are a lot of intermediate
     * actors (to be exact, the number of intermediate actors is close to log2(actors.size)).
     */
    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    core_vector_init(&destinations, sizeof(int));
    core_vector_set_memory_pool(&destinations, ephemeral_memory);
    core_vector_copy_range(actors, first, last, &destinations);

    /*
     * Set the source now.
     */
    name = thorium_actor_name(actor);
    thorium_message_set_source(message, name);

    thorium_actor_send_range_binomial_tree(actor, &destinations, message);

    core_vector_destroy(&destinations);
}

void thorium_actor_send_range_loop(struct thorium_actor *actor, struct core_vector *actors,
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
                       thorium_message_action(message), i);
#endif
        the_actor = *(int *)core_vector_at(actors, i);

#ifdef DEBUG_BINOMIAL_TREE
        printf("send_range_loop, sending to %d\n", the_actor);
#endif

        thorium_actor_send(actor, the_actor, message);
        i++;
    }
}

void thorium_actor_send_range_empty(struct thorium_actor *actor, struct core_vector *actors,
                int tag)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, 0, NULL);
    thorium_actor_send_range(actor, actors, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_range_int(struct thorium_actor *actor, struct core_vector *actors,
                int tag, int value)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, sizeof(value), &value);
    thorium_actor_send_range(actor, actors, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_range_buffer(struct thorium_actor *actor, struct core_vector *destinations,
                int tag, int count, void *buffer)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, count, buffer);
    thorium_actor_send_range(actor, destinations, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_range_vector(struct thorium_actor *actor, struct core_vector *actors,
                int tag, struct core_vector *vector)
{
    struct thorium_message message;
    int count;
    void *buffer;
    struct core_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    count = core_vector_pack_size(vector);
    buffer = core_memory_pool_allocate(ephemeral_memory, count);
    core_vector_pack(vector, buffer);
    thorium_message_init(&message, tag, count, buffer);
    thorium_actor_send_range(actor, actors, &message);
    thorium_message_destroy(&message);

    core_memory_pool_free(ephemeral_memory, buffer);
}

void thorium_actor_send_double(struct thorium_actor *actor, int destination, int tag, double value)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, sizeof(value), &value);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}

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

void thorium_actor_send_range_positions_vector(struct thorium_actor *actor, struct core_vector *actors,
                int first, int last,
                int tag, struct core_vector *vector)
{
    struct thorium_message message;
    int count;
    void *buffer;
    struct core_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    count = core_vector_pack_size(vector);
    buffer = core_memory_pool_allocate(ephemeral_memory, count);
    core_vector_pack(vector, buffer);
    thorium_message_init(&message, tag, count, buffer);
    thorium_actor_send_range_default(actor, actors, first, last, &message);
    thorium_message_destroy(&message);

    core_memory_pool_free(ephemeral_memory, buffer);
}
