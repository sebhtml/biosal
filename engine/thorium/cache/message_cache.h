
#ifndef THORIUM_MESSAGE_CACHE_H_
#define THORIUM_MESSAGE_CACHE_H_

#include <core/structures/map.h>

#define THORIUM_CACHE_BASE  -20000
#define ACTION_ENABLE_MESSAGE_CACHE (THORIUM_CACHE_BASE + 1)
#define ACTION_DISABLE_MESSAGE_CACHE (THORIUM_CACHE_BASE + 2)
#define ACTION_CLEAR_MESSAGE_CACHE (THORIUM_CACHE_BASE + 3)

struct thorium_message;
struct core_memory_pool;

/**
 * An implementation of a message cache.
 */
struct thorium_message_cache {
    int size;
    struct core_map entries;
    struct core_memory_pool *pool;
};

void thorium_message_cache_init(struct thorium_message_cache *self);
void thorium_message_cache_destroy(struct thorium_message_cache *self);

void thorium_message_cache_set_memory_pool(struct thorium_message_cache *self,
                struct core_memory_pool *pool);
void thorium_message_cache_set_size(struct thorium_message_cache *self, int size);
void thorium_message_cache_clear(struct thorium_message_cache *self);

struct thorium_message *thorium_message_cache_get(struct thorium_message_cache *self,
                struct thorium_message *request_message);

void thorium_message_cache_add(struct thorium_message_cache *self,
                struct thorium_message *request_message,
                struct thorium_message *reply_message);

#endif
