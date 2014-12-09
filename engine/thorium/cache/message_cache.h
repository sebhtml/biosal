
#ifndef THORIUM_MESSAGE_CACHE_H_
#define THORIUM_MESSAGE_CACHE_H_

#include "cache_tag.h"

#include <core/structures/map.h>
#include <core/structures/set.h>

struct thorium_message;
struct core_memory_pool;

/**
 * An implementation of a message cache.
 */
struct thorium_message_cache {
    struct core_map actions;
    struct core_map entries;
    struct core_memory_pool *pool;

    struct thorium_cache_tag saved_reply_message_cache_tag;

    int profile_cache_miss_count;
    int profile_cache_hit_count;
};

void thorium_message_cache_init(struct thorium_message_cache *self);
void thorium_message_cache_destroy(struct thorium_message_cache *self);

void thorium_message_cache_set_memory_pool(struct thorium_message_cache *self,
                struct core_memory_pool *pool);
void thorium_message_cache_clear(struct thorium_message_cache *self);

/**
 * Obtain a reply message associated to a request message (from the message
 * cache).
 */
struct thorium_message *thorium_message_cache_get_reply_message(struct thorium_message_cache *self,
                struct thorium_message *request_message);

/**
 * This is called in the ACTION_ENABLE_MESSAGE_CACHE code path.
 */
void thorium_message_cache_enable(struct thorium_message_cache *self,
                int request_action, int reply_action);

/**
 * This is called in the ACTION_DISABLE_MESSAGE_CACHE code path.
 */
void thorium_message_cache_disable(struct thorium_message_cache *self,
                int action);

void thorium_message_cache_save_request_message(struct thorium_message_cache *self,
                struct thorium_message *message);
void thorium_message_cache_save_reply_message(struct thorium_message_cache *self,
                struct thorium_message *message);
int thorium_message_cache_action_count(struct thorium_message_cache *self);
void thorium_message_cache_get_profile(struct thorium_message_cache *self,
                int *cache_miss_count, int *cache_hit_count);
#endif
