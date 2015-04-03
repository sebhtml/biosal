
#include "proxy_message.h"

#include "binomial_tree_message.h" /* for DEBUG_BINOMIAL_TREE */

#include <engine/thorium/message.h>
#include <engine/thorium/actor.h>

#include <core/system/memory_pool.h>
#include <core/system/debugger.h>

#include <string.h>

void thorium_actor_unpack_proxy_message(struct thorium_actor *self,
                struct thorium_message *message)
{
    int new_count;
    int tag;
    int source;
    void *buffer;
    int offset;

    buffer = thorium_message_buffer(message);
    new_count = thorium_message_count(message);
    new_count -= sizeof(source);
    new_count -= sizeof(tag);

    offset = new_count;

    source = *(int *)((char *)buffer + offset);
    offset += sizeof(source);
    tag = *(int *)((char *)buffer + offset);

#ifdef DEBUG_BINOMIAL_TREE
    printf("DEBUG_BINOMIAL_TREE unpack_proxy_message real_count %d real_source %d real_action %x\n",
                    new_count, source, tag);
#endif

    /*
     * The action can not be ACTION_INVALID because it is invalid
     * by convention.
     */
    CORE_DEBUGGER_ASSERT(tag != ACTION_INVALID);

    offset += sizeof(tag);

    /*thorium_message_init(message, tag, new_count, buffer);*/

    /*
     * Change the tag, source, and count.
     */
    thorium_message_set_source(message, source);
    thorium_message_set_action(message, tag);
    thorium_message_set_count(message, new_count);

    thorium_message_set_identifier(message, -1);
    thorium_message_set_parent_actor(message, -1);
    thorium_message_set_parent_identifier(message, -1);

#if 0
    printf("DEBUG unpack_proxy_message... source %d tag %d count %d\n",
                    source, tag, new_count);
#endif
}

void thorium_actor_pack_proxy_message(struct thorium_actor *self, struct thorium_message *message,
                int real_source)
{
    int real_tag;
    int count;
    int new_count;
    void *buffer;
    void *new_buffer;
    int offset;
    struct core_memory_pool *ephemeral_memory;

    /*
     * pack data in this order:
     *
     * - data (count bytes)
     * - real_source
     * - real_tag
     */
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    real_tag = thorium_message_action(message);
    buffer = thorium_message_buffer(message);
    count = thorium_message_count(message);

#ifdef DEBUG_BINOMIAL_TREE
    printf("DEBUG_BINOMIAL_TREE pack_proxy_message count %d source %d action %x\n", count,
                    thorium_actor_name(self),
                    real_tag);
    thorium_message_print(message);

#endif

    new_count = count + sizeof(real_source) + sizeof(real_tag);

    /* use slab allocator */
    new_buffer = core_memory_pool_allocate(ephemeral_memory, new_count);

#ifdef THORIUM_ACTOR_DEBUG
    printf("DEBUG12 core_memory_pool_allocate %p (pack proxy message)\n",
                    new_buffer);
#endif

    if (count > 0)
        core_memory_copy(new_buffer, buffer, count);

    offset = count;
    core_memory_copy((char *)new_buffer + offset, &real_source, sizeof(real_source));
    offset += sizeof(real_source);
    core_memory_copy((char *)new_buffer + offset, &real_tag, sizeof(real_tag));
    offset += sizeof(real_tag);

    thorium_message_init(message, ACTION_PROXY_MESSAGE, new_count, new_buffer);
    thorium_message_set_source(message, real_source);

#if 0
    /* free the old buffer
     */
    core_memory_free(buffer);
    buffer = NULL;
#endif
}

