
#ifndef THORIUM_CACHE_TAG_
#define THORIUM_CACHE_TAG_

#include <stdint.h>

struct thorium_message;

/*
 * A Thorium cache tag for message cache
 */
struct thorium_cache_tag {
    int action;
    int destination;
    int count;
    uint64_t signature;
};

void thorium_cache_tag_init(struct thorium_cache_tag *self);
void thorium_cache_tag_destroy(struct thorium_cache_tag *self);

void thorium_cache_tag_set(struct thorium_cache_tag *self,
                struct thorium_message *message);
int thorium_cache_tag_action(struct thorium_cache_tag *self);
void thorium_cache_tag_reset(struct thorium_cache_tag *self);
void thorium_cache_tag_print(struct thorium_cache_tag *self);

#endif
