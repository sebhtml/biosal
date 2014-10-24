
#include "message_block.h"

#include <stdlib.h>

void thorium_message_block_init(struct thorium_message_block *self)
{
    self->size = 0;
}

void thorium_message_block_destroy(struct thorium_message_block *self)
{
    self->size = 0;
}

int thorium_message_block_add_message(struct thorium_message_block *self,
                struct thorium_message *message)
{
    if (self->size == THORIUM_MESSAGE_BLOCK_MAXIMUM_SIZE)
        return 0;

    self->messages[self->size] = *message;

    ++self->size;

    return 1;
}

struct thorium_message *thorium_message_block_get_message(struct thorium_message_block *self, int i)
{
    if (i >= 0 && i < self->size)
        return self->messages + i;

    return NULL;
}

int thorium_message_block_size(struct thorium_message_block *self)
{
    return self->size;
}

int thorium_message_block_full(struct thorium_message_block *self)
{
    return self->size == THORIUM_MESSAGE_BLOCK_MAXIMUM_SIZE;
}

void thorium_message_block_clear(struct thorium_message_block *self)
{
    self->size = 0;
}

int thorium_message_block_empty(struct thorium_message_block *self)
{
    return self->size == 0;
}
