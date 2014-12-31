
#include "cache_tag.h"

#include <engine/thorium/message.h>

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#include <string.h>

void thorium_cache_tag_init(struct thorium_cache_tag *self)
{
    thorium_cache_tag_reset(self);
}

void thorium_cache_tag_destroy(struct thorium_cache_tag *self)
{
    thorium_cache_tag_reset(self);
}

void thorium_cache_tag_set(struct thorium_cache_tag *self,
                struct thorium_message *message)
{
    thorium_cache_tag_reset(self);

    self->action = thorium_message_action(message);
    self->destination = thorium_message_destination(message);
    self->count = thorium_message_count(message);
    self->signature = thorium_message_signature(message);
}

int thorium_cache_tag_action(struct thorium_cache_tag *self)
{
    return self->action;
}

void thorium_cache_tag_reset(struct thorium_cache_tag *self)
{
    /*
     * Clear the padding to have a deterministic cache
     * store.
     */
    memset(self, 0, sizeof(struct thorium_cache_tag));

    self->action = ACTION_INVALID;
    self->signature = 0;
    self->destination = 0;
    self->count = 0;
}

void thorium_cache_tag_print(struct thorium_cache_tag *self)
{
    printf("REGRESSION thorium_cache_tag_print action %d destination %d"
                   " count %d signature %" PRIx64 "\n",
                   self->action, self->destination, self->count,
                   self->signature);
}

int thorium_cache_tag_destination(struct thorium_cache_tag *self)
{
    return self->destination;
}
