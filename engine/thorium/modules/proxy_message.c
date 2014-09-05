
#include "proxy_message.h"

#include <engine/thorium/message.h>
#include <engine/thorium/actor.h>

#include <core/system/memory_pool.h>

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
    offset += sizeof(tag);

    /*thorium_message_init(message, tag, new_count, buffer);*/

    /*
     * Change the tag, source, and count.
     */
    thorium_message_set_source(message, source);
    thorium_message_set_tag(message, tag);
    thorium_message_set_count(message, new_count);

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
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    real_tag = thorium_message_tag(message);
    buffer = thorium_message_buffer(message);
    count = thorium_message_count(message);

    new_count = count + sizeof(real_source) + sizeof(real_tag);

    /* use slab allocator */
    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

#ifdef THORIUM_ACTOR_DEBUG
    printf("DEBUG12 bsal_memory_pool_allocate %p (pack proxy message)\n",
                    new_buffer);
#endif

    if (count > 0)
        bsal_memory_copy(new_buffer, buffer, count);

    offset = count;
    bsal_memory_copy((char *)new_buffer + offset, &real_source, sizeof(real_source));
    offset += sizeof(real_source);
    bsal_memory_copy((char *)new_buffer + offset, &real_tag, sizeof(real_tag));
    offset += sizeof(real_tag);

    thorium_message_init(message, ACTION_PROXY_MESSAGE, new_count, new_buffer);
    thorium_message_set_source(message, real_source);

#if 0
    /* free the old buffer
     */
    bsal_memory_free(buffer);
    buffer = NULL;
#endif
}

