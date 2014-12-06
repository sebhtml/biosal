
#include "message_cache.h"

#include "cache_tag.h"

#include <engine/thorium/message.h>

#include <core/structures/map.h>
#include <core/structures/map_iterator.h>

#include <core/system/debugger.h>
#include <core/system/memory_pool.h>

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
    struct core_map_iterator iterator;
    struct thorium_cache_tag *request_tag;
    struct thorium_message *reply_message;
    void *buffer;

#ifdef DEBUG_MESSAGE_CACHE_CLEAR
    printf("DEBUG thorium_message_cache_clear entries: %d\n",
                    (int)core_map_size(&self->entries));
#endif

    core_map_iterator_init(&iterator, &self->entries);

    while (core_map_iterator_next(&iterator, (void **)&request_tag,
                            (void **)&reply_message)) {

        thorium_cache_tag_destroy(request_tag);

        buffer = thorium_message_buffer(reply_message);

        if (buffer != NULL) {
            core_memory_pool_free(self->pool, buffer);
        }

        thorium_message_destroy(reply_message);
    }

    core_map_iterator_destroy(&iterator);

    core_map_clear(&self->entries);
}

struct thorium_message *thorium_message_cache_get_reply_message(struct thorium_message_cache *self,
                struct thorium_message *request_message)
{
    struct thorium_cache_tag cache_tag;
    int action;
    struct thorium_message *reply_message;

    /*
     * Verify if the action is supported.
     */
    action = thorium_message_action(request_message);

    if (!core_set_find(&self->actions, &action)) {
        return NULL;
    }

    /*
     * Generate a cache tag and perform a lookup.
     * In other words, generate a tag and query it in the cache entries.
     */
    thorium_cache_tag_set(&cache_tag, request_message);
    reply_message = core_map_get(&self->entries, &cache_tag);

    /*
     * If the key is NULL, save the request message for later use.
     * This basically sets the attribute last_tag.
     */
    if (reply_message == NULL) {

        /*
         * Save the request message if necessary.
         */
        thorium_message_cache_save_request_message(self, request_message);
    }

    /*
     * Return the reply message.
     * This can be NULL.
     */
    return reply_message;
}

void thorium_message_cache_save_reply_message(struct thorium_message_cache *self,
                struct thorium_message *message)
{
    struct thorium_message stored_message;
    void *buffer;

    CORE_DEBUGGER_ASSERT_NOT_NULL(self);
    CORE_DEBUGGER_ASSERT_NOT_NULL(message);

    /*
     * This is not a reply to a request that needs to use
     * caching.
     */
    if (thorium_cache_tag_action(&self->last_tag) == ACTION_INVALID) {
        return;
    }

    /*
     * The last_tag is already in the cache entries.
     * This code path should not actually happen...
     */
    if (core_map_get(&self->entries, &self->last_tag) != NULL) {
        thorium_cache_tag_reset(&self->last_tag);
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

    CORE_DEBUGGER_ASSERT(self->pool != NULL);

    core_memory_copy(&stored_message, message, sizeof(stored_message));

    /*
     * Copy the buffer if it is not NULL.
     */
    if (thorium_message_buffer(message) != NULL) {
        buffer = core_memory_pool_allocate(self->pool,
                    thorium_message_count(&stored_message));
        thorium_message_set_buffer(&stored_message, buffer);
        core_memory_copy(thorium_message_buffer(&stored_message),
                        thorium_message_buffer(message),
                        thorium_message_count(message));
    }

    core_map_add_value(&self->entries, &self->last_tag, &stored_message);

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
     * At this point, the last_tag may be already set.
     * In such a case, it is just overwritten anyway.
     */
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
