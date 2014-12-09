
#ifndef THORIUM_CACHE_TAG_
#define THORIUM_CACHE_TAG_

#include <stdint.h>

struct thorium_message;

/**
 * A Thorium cache tag (for request message)
 * for message cache.
 */
struct thorium_cache_tag {
    int action;
    int destination;
    int count;
    uint64_t signature;
};

void thorium_cache_tag_init(struct thorium_cache_tag *self);
void thorium_cache_tag_destroy(struct thorium_cache_tag *self);

/**
 * Generate values and assign them given a message.
 */
void thorium_cache_tag_set(struct thorium_cache_tag *self,
                struct thorium_message *message);
int thorium_cache_tag_action(struct thorium_cache_tag *self);
int thorium_cache_tag_destination(struct thorium_cache_tag *self);

/**
 * Invalidate the cache tag attribute values.
 * This sets the action to ACTION_INVALID.
 */
void thorium_cache_tag_reset(struct thorium_cache_tag *self);
void thorium_cache_tag_print(struct thorium_cache_tag *self);

#endif
