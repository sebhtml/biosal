
#include "message_cache.h"

#include "cache_tag.h"

#include <engine/thorium/message.h>

#include <core/structures/map.h>
#include <core/structures/map_iterator.h>

#include <core/system/debugger.h>
#include <core/system/memory_pool.h>

/*
#define TRACK_REGRESSION_2014_12_30
*/

#ifdef TRACK_REGRESSION_2014_12_30
#include <genomics/assembly/assembly_graph_store.h>
#endif

#include <stdio.h>

#ifdef TRACK_REGRESSION_2014_12_30
uint64_t TRACK_SIGNATURE = 0x70127736b0c54cf2;

/*
*/
#define TRACK_REGRESSION_2014_12_30_BUG

#endif

void thorium_message_cache_init(struct thorium_message_cache *self)
{
    core_map_init(&self->entries, sizeof(struct thorium_cache_tag),
                    sizeof(struct thorium_message));

    core_map_init(&self->actions, sizeof(int), sizeof(int));

    thorium_cache_tag_init(&self->saved_reply_message_cache_tag);

    self->pool = NULL;

    self->profile_cache_miss_count = 0;
    self->profile_cache_hit_count = 0;
}

void thorium_message_cache_destroy(struct thorium_message_cache *self)
{
    thorium_message_cache_clear(self);

    core_map_destroy(&self->entries);
    core_map_destroy(&self->actions);

    thorium_cache_tag_destroy(&self->saved_reply_message_cache_tag);
}

void thorium_message_cache_set_memory_pool(struct thorium_message_cache *self,
                struct core_memory_pool *pool)
{
    self->pool = pool;
    core_map_set_memory_pool(&self->entries, pool);
    core_map_set_memory_pool(&self->actions, pool);
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

    if (core_map_get(&self->actions, &action) == NULL) {

            /*
        printf("DEBUG request action %d is not supported !\n",
                        action);
                        */
        return NULL;
    }

    /*
     * Generate a cache tag and perform a lookup.
     * In other words, generate a tag and query it in the cache entries.
     */
    thorium_cache_tag_set(&cache_tag, request_message);
    reply_message = core_map_get(&self->entries, &cache_tag);

#ifdef TRACK_REGRESSION_2014_12_30_BUG
    if (action == ACTION_ASSEMBLY_GET_VERTEX
                    && thorium_cache_tag_signature(&cache_tag) == TRACK_SIGNATURE) {

        if (reply_message == NULL) {
            reply_message = core_map_get(&self->entries, &cache_tag);
            reply_message = core_map_get(&self->entries, &cache_tag);
            reply_message = core_map_get(&self->entries, &cache_tag);
            reply_message = core_map_get(&self->entries, &cache_tag);
            reply_message = core_map_get(&self->entries, &cache_tag);
            reply_message = core_map_get(&self->entries, &cache_tag);

            if (reply_message != NULL) {
                printf("WHAT ! not NULL anymore...\n");
            }
        }
        printf("REGRESSION get_reply_message for ACTION_ASSEMBLY_GET_VERTEX... -> %p\n",
                        (void *)reply_message);
        printf("REGRESSION entries in cache: %d\n",
                        (int)core_map_size(&self->entries));

        thorium_cache_tag_print(&cache_tag);
    }
#endif

    /*
     * If the key is NULL, save the request message for later use.
     * This basically sets the attribute saved_reply_message_cache_tag.
     */
    if (reply_message == NULL) {

        /*
         * Save the request message if necessary.
         */
        thorium_message_cache_save_request_message(self, request_message);

    }

    if (reply_message == NULL) {
        /*
         * This is a cache miss.
         */
        ++self->profile_cache_miss_count;
    } else {
        /*
         * This is a cache hit.
         */
        ++self->profile_cache_hit_count;
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
    int expected_source;
    int actual_source;
    int expected_request_action;
    int *bucket;
    int expected_reply_action;
    int actual_reply_action;

    CORE_DEBUGGER_ASSERT_NOT_NULL(self);
    CORE_DEBUGGER_ASSERT_NOT_NULL(message);

    /*
     * This is not a reply to a request that needs to use
     * caching.
     */
    if (thorium_cache_tag_action(&self->saved_reply_message_cache_tag) == ACTION_INVALID) {
        return;
    }

    actual_source = thorium_message_source(message);
    expected_source = thorium_cache_tag_destination(&self->saved_reply_message_cache_tag);

    /*
     * This is not the good message.
     */
    if (actual_source != expected_source) {
        return;
    }

    expected_request_action = thorium_cache_tag_action(&self->saved_reply_message_cache_tag);
    bucket = core_map_get(&self->actions, &expected_request_action);

    /*
     * Not supported.
     */
    if (bucket == NULL) {
        return;
    }

    expected_reply_action = *bucket;
    actual_reply_action = thorium_message_action(message);

    /*
     * This is not the message we are looking for.
     */
    if (actual_reply_action != expected_reply_action) {
        return;
    }

    /*
     * The saved_reply_message_cache_tag is already in the cache entries.
     * This code path should not actually happen...
     */
    if (core_map_get(&self->entries, &self->saved_reply_message_cache_tag) != NULL) {
        thorium_cache_tag_reset(&self->saved_reply_message_cache_tag);
        return;
    }

#ifdef TRACK_REGRESSION_2014_12_30
    if (thorium_message_action(message) == ACTION_ASSEMBLY_GET_VERTEX_REPLY
            && thorium_cache_tag_signature(&self->saved_reply_message_cache_tag) ==
            TRACK_SIGNATURE) {
        printf("REGRESSION thorium_message_cache_save_reply_message action ACTION_ASSEMBLY_GET_VERTEX_REPLY\n");
        thorium_cache_tag_print(&self->saved_reply_message_cache_tag);
    }
#endif

    /*
     * Use the cache tag in self->saved_reply_message_cache_tag to add the reply
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

    core_map_add_value(&self->entries, &self->saved_reply_message_cache_tag, &stored_message);

    /*
     * After that, free/reset the request message.
     */
    thorium_cache_tag_reset(&self->saved_reply_message_cache_tag);
}

void thorium_message_cache_enable(struct thorium_message_cache *self,
                int request_action, int reply_action)
{
    core_map_add_value(&self->actions, &request_action, &reply_action);
}

void thorium_message_cache_disable(struct thorium_message_cache *self,
                int request_action)
{
    core_map_delete(&self->actions, &request_action);
}

void thorium_message_cache_save_request_message(struct thorium_message_cache *self,
                struct thorium_message *message)
{
    int action;

    action = thorium_message_action(message);

    if (core_map_get(&self->actions, &action) == NULL) {
            /*
        printf("DEBUG request action %d not managed by cache\n",
                        action);
                        */
        return;
    }

    /*
     * At this point, the saved_reply_message_cache_tag may be already set.
     * In such a case, it is just overwritten anyway.
     */
    /*
     * Save the message cache tag for later.
     */
    thorium_cache_tag_set(&self->saved_reply_message_cache_tag, message);

#ifdef TRACK_REGRESSION_2014_12_30
    if (action == ACTION_ASSEMBLY_GET_VERTEX
              && thorium_cache_tag_signature(&self->saved_reply_message_cache_tag) == TRACK_SIGNATURE) {
        printf("REGRESSION saved request message ACTION_ASSEMBLY_GET_VERTEX\n");
        thorium_cache_tag_print(&self->saved_reply_message_cache_tag);
    }
#endif
}

int thorium_message_cache_action_count(struct thorium_message_cache *self)
{
    return core_map_size(&self->actions);
}

void thorium_message_cache_get_profile(struct thorium_message_cache *self,
                int *cache_miss_count, int *cache_hit_count)
{
    *cache_miss_count = self->profile_cache_miss_count;
    *cache_hit_count = self->profile_cache_hit_count;
}
