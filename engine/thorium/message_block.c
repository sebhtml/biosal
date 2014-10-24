
#include "message_block.h"

#include <stdlib.h>

void thorium_message_block_init(struct thorium_message_block *self)
{
    self->count = 0;
}

void thorium_message_block_destroy(struct thorium_message_block *self)
{
    self->count = 0;
}

int thorium_message_block_add_message(struct thorium_message_block *self,
                struct thorium_message *message)
{
    if (self->count == THORIUM_MESSAGE_BLOCK_MAXIMUM_SIZE)
        return 0;

    self->messages[self->count] = *message;

    ++self->count;

    return 1;
}

struct thorium_message *thorium_message_block_get_message(struct thorium_message_block *self, int i)
{
    if (i >= 0 && i < self->count)
        return self->messages + i;

    return NULL;
}

int thorium_message_block_count(struct thorium_message_block *self)
{
    return self->count;
}
