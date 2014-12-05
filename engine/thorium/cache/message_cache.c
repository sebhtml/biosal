
#include "message_cache.h"

#include "cache_tag.h"

#include <engine/thorium/message.h>
#include <core/structures/map.h>

void thorium_message_cache_init(struct thorium_message_cache *self)
{
    core_map_init(&self->entries, sizeof(struct thorium_cache_tag),
                    sizeof(struct thorium_message));

    core_set_init(&self->actions, sizeof(int));
}

void thorium_message_cache_destroy(struct thorium_message_cache *self)
{
    thorium_message_cache_clear(self);

    core_map_destroy(&self->entries);
    core_set_destroy(&self->actions);
}

void thorium_message_cache_set_memory_pool(struct thorium_message_cache *self,
                struct core_memory_pool *pool)
{
    self->pool = pool;
    core_map_set_memory_pool(&self->entries, pool);
    core_set_set_memory_pool(&self->actions, pool);
}

void thorium_message_cache_clear(struct thorium_message_cache *self)
{
    core_map_clear(&self->entries);
}

struct thorium_message *thorium_message_cache_get(struct thorium_message_cache *self,
                struct thorium_message *request_message)
{
    /*
     * Generate a tag and query it in the cache entries.
     */
    return NULL;
}

void thorium_message_cache_add(struct thorium_message_cache *self,
                struct thorium_message *request_message,
                struct thorium_message *reply_message)
{
    /*
     * Generate a cache tag and add the entry.
     */
}

void thorium_message_cache_enable(struct thorium_message_cache *self,
                int action)
{
    core_set_add(&self->actions, &action);
}

void thorium_message_cache_disable(struct thorium_message_cache *self,
                int action)
{
    core_set_delete(&self->actions, &action);
}
