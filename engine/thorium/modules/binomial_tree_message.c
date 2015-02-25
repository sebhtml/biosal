
#include "binomial_tree_message.h"

#include <engine/thorium/actor.h>
#include <engine/thorium/message.h>

#include <core/system/memory_pool.h>
#include <core/system/debugger.h>

#include <string.h>

#define BINOMIAL_TREE_MINIMUM 4

void thorium_actor_send_range_binomial_tree_part(struct thorium_actor *actor,
               int destination, struct core_vector *actors,
               struct thorium_message *message);

void thorium_actor_receive_binomial_tree_send(struct thorium_actor *actor, struct thorium_message *message)
{
    char *buffer;
    int amount;
    int limit;
    int offset;
    struct core_vector actors;
    struct core_memory_pool *ephemeral_memory;
    int real_tag;
    int real_count;
    char *real_buffer;
    int real_source;
    struct thorium_message new_message;
    void *new_buffer;

    /*
     * Content of the payload:
     * new_count = core_vector_pack_size(actors) + sizeof(real_tag) + sizeof(real_source) + sizeof(real_count) + real_count;
     */
    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);

    CORE_DEBUGGER_LEAK_DETECTION_BEGIN(ephemeral_memory, binomial_send);

    limit = BINOMIAL_TREE_MINIMUM;
    buffer = thorium_message_buffer(message);

    offset = 0;
    core_vector_init(&actors, sizeof(int));
    core_vector_set_memory_pool(&actors, ephemeral_memory);
    offset += core_vector_unpack(&actors, (char *)buffer + offset);

    core_memory_copy(&real_tag, buffer + offset, sizeof(real_tag));
    offset += sizeof(real_tag);
    core_memory_copy(&real_source, buffer + offset, sizeof(real_source));
    offset += sizeof(real_source);
    core_memory_copy(&real_count, buffer + offset, sizeof(real_count));
    offset += sizeof(real_count);
    real_buffer = buffer + offset;
    offset += real_count;

    CORE_DEBUGGER_ASSERT(offset == count);

    amount = core_vector_size(&actors);

#ifdef DEBUG_BINOMIAL_TREE
    printf("DEBUG_BINOMIAL_TREE actor %i received ACTION_BINOMIAL_TREE_SEND "
                    "real_source: %i real_tag: %i destinations: %d ",
                    thorium_actor_name(actor), real_source, real_tag, amount);

    core_vector_print_int(&actors);
    printf("\n");
#endif

    thorium_message_init(&new_message, real_tag, real_count, real_buffer);
    thorium_message_set_source(&new_message, real_source);

    /*
     * If the number is small, send PACKED messages.
     */
    if (amount < limit) {

        CORE_DEBUGGER_LEAK_DETECTION_BEGIN(ephemeral_memory, send_range);

        thorium_actor_pack_proxy_message(actor, &new_message,
                        real_source);

#ifdef DEBUG_BINOMIAL_TREE
        printf("DEBUG_BINOMIAL_TREE below limit, using send_range_loop\n");
#endif

        thorium_actor_send_range_loop(actor, &actors, 0, core_vector_size(&actors) - 1, &new_message);

        /*
         * Free the buffer for the proxy message.
         */
        new_buffer = thorium_message_buffer(&new_message);
        core_memory_pool_free(ephemeral_memory, new_buffer);

        CORE_DEBUGGER_LEAK_DETECTION_END(ephemeral_memory, send_range);
    } else {
        CORE_DEBUGGER_LEAK_DETECTION_BEGIN(ephemeral_memory, binomial_tree);

#ifdef DEBUG_BINOMIAL_TREE
        printf("DEBUG_BINOMIAL_TREE parallel-recursive call to send_range_binomial_tree\n");
#endif

        thorium_actor_send_range_binomial_tree(actor, &actors, &new_message);
        CORE_DEBUGGER_LEAK_DETECTION_END(ephemeral_memory, binomial_tree);
    }

    thorium_message_destroy(&new_message);
    core_vector_destroy(&actors);

    CORE_DEBUGGER_LEAK_DETECTION_END(ephemeral_memory, binomial_send);
}

void thorium_actor_send_range_binomial_tree(struct thorium_actor *actor, struct core_vector *actors,
                struct thorium_message *message)
{
    int middle;
    int first1;
    int last1;
    int middle1;
    int first2;
    int last2;
    int middle2;
    int first;
    int last;
    struct core_vector left_part;
    struct core_vector right_part;
    int left_actor;
    int right_actor;
    int limit;
    struct core_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    limit = BINOMIAL_TREE_MINIMUM;

    if (core_vector_size(actors) < limit) {
        thorium_actor_send_range_loop(actor, actors, 0,
                       core_vector_size(actors) - 1, message);
        return;
    }

#ifdef THORIUM_ACTOR_DEBUG_BINOMIAL_TREE
    int name;

    name = thorium_actor_name(actor);
#endif

    first = 0;
    last = core_vector_size(actors) - 1;
    middle = first + (last - first) / 2;

#ifdef DEBUG_BINOMIAL_TREE
    printf("DEBUG_BINOMIAL_TREE %i thorium_actor_send_range_binomial_tree\n",
                    thorium_actor_name(actor));
    printf("DEBUG_BINOMIAL_TREE DEBUG %i first: %i last: %i middle: %i size %d\n",
                    thorium_actor_name(actor), first, last, middle,
                    (int)core_vector_size(actors));
#endif

    /*
     * Send the left part to an actor.
     */
    first1 = first;
    last1 = middle - 1;
    middle1 = first1 + (last1 - first1) / 2;
    left_actor = THORIUM_ACTOR_NOBODY;

    core_vector_init(&left_part, sizeof(int));
    core_vector_set_memory_pool(&left_part, ephemeral_memory);
    core_vector_copy_range(actors, first1, last1, &left_part);

    left_actor = *(int *)core_vector_at(&left_part, middle1);

    CORE_DEBUGGER_ASSERT(left_actor != THORIUM_ACTOR_NOBODY);

#ifdef DEBUG_BINOMIAL_TREE
    printf("DEBUG_BINOMIAL_TREE left_actor: %d left_part.size %d first1 %d last1 %d middle1 %d\n",
                    left_actor, (int)core_vector_size(&left_part),
                    first1, last1, middle1);
#endif

    thorium_actor_send_range_binomial_tree_part(actor, left_actor, &left_part, message);
    core_vector_destroy(&left_part);

    /*
     * Send the right part to an actor.
     */
    first2 = middle;
    last2 = last;
    middle2 = first2 + (last2 - first2) / 2;
    right_actor = THORIUM_ACTOR_NOBODY;

    core_vector_init(&right_part, sizeof(int));
    core_vector_set_memory_pool(&right_part, ephemeral_memory);
    core_vector_copy_range(actors, first2, last2, &right_part);

    /*
     * Substract the first offset to get a 0-based index.
     */
    right_actor = *(int *)core_vector_at(&right_part, middle2 - first2);

    CORE_DEBUGGER_ASSERT(right_actor != THORIUM_ACTOR_NOBODY);

#ifdef DEBUG_BINOMIAL_TREE
    printf("DEBUG_BINOMIAL_TREE right_actor: %d right_part.size %d first2 %d last2 %d middle2 %d\n",
                    right_actor, (int)core_vector_size(&right_part),
                    first2, last2, middle2);
#endif

    thorium_actor_send_range_binomial_tree_part(actor, right_actor, &right_part, message);
    core_vector_destroy(&right_part);
}

void thorium_actor_send_range_binomial_tree_part(struct thorium_actor *actor,
               int destination, struct core_vector *actors,
               struct thorium_message *message)
{
    int real_tag;
    int real_source;
    int real_count;
    char *real_buffer;
    char *new_buffer;
    int new_tag;
    int new_count;
    int offset;
    struct thorium_message new_message;

    real_tag = thorium_message_action(message);
    real_source = thorium_message_source(message);
    real_count = thorium_message_count(message);
    real_buffer = thorium_message_buffer(message);

    new_tag = ACTION_BINOMIAL_TREE_SEND;
    new_count = core_vector_pack_size(actors) + sizeof(real_tag) + sizeof(real_source) + sizeof(real_count) + real_count;
    new_buffer = thorium_actor_allocate(actor, new_count);

    offset = 0;
    offset += core_vector_pack(actors, new_buffer + offset);
    core_memory_copy(new_buffer + offset, &real_tag, sizeof(real_tag));
    offset += sizeof(real_tag);
    core_memory_copy(new_buffer + offset, &real_source, sizeof(real_source));
    offset += sizeof(real_source);
    core_memory_copy(new_buffer + offset, &real_count, sizeof(real_count));
    offset += sizeof(real_count);

    if (real_count > 0)
        core_memory_copy(new_buffer + offset, real_buffer, real_count);
    offset += real_count;

    CORE_DEBUGGER_ASSERT(offset == new_count);

    thorium_message_init(&new_message, new_tag, new_count, new_buffer);
    thorium_actor_send(actor, destination, &new_message);
    thorium_message_destroy(&new_message);
}


