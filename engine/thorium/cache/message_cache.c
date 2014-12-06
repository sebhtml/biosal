
#include "message_cache.h"

#include "cache_tag.h"

#include <engine/thorium/message.h>
#include <core/structures/map.h>

#include <core/system/debugger.h>

#include <stdio.h>

void thorium_message_cache_init(struct thorium_message_cache *self)
{
    core_map_init(&self->entries, sizeof(struct thorium_cache_tag),
                    sizeof(struct thorium_message));

    core_set_init(&self->actions, sizeof(int));

    thorium_cache_tag_init(&self->last_tag);

    self->pool = NULL;
}

void thorium_message_cache_destroy(struct thorium_message_cache *self)
{
    thorium_message_cache_clear(self);

    core_map_destroy(&self->entries);
    core_set_destroy(&self->actions);

    thorium_cache_tag_destroy(&self->last_tag);
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

void thorium_message_cache_save_reply_message(struct thorium_message_cache *self,
                struct thorium_message *message)
{
    /*
     * This is not a reply to a request that needs to use
     * caching.
     */
    if (thorium_cache_tag_action(&self->last_tag) == ACTION_INVALID) {
        return;
    }

    /*
    printf("DEBUG thorium_message_cache_save_reply_message action %d\n",
                    thorium_message_action(message));
    */
    /*
     * Use the cache tag in self->last_tag to add the reply
     * message in an entry.
     */
    /* TODO */

    CORE_DEBUGGER_ASSERT(self->pool != NULL);

    /*
     * After that, free/reset the request message.
     */
    thorium_cache_tag_reset(&self->last_tag);
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

void thorium_message_cache_save_request_message(struct thorium_message_cache *self,
                struct thorium_message *message)
{
    int action;

    action = thorium_message_action(message);

    if (!core_set_find(&self->actions, &action))
        return;

    /*
     * Save the message cache tag for later.
     */
    thorium_cache_tag_set(&self->last_tag, message);

    /*
    thorium_cache_tag_print(&self->last_tag);
    */
}

int thorium_message_cache_action_count(struct thorium_message_cache *self)
{
    return core_set_size(&self->actions);
}
