
#ifndef THORIUM_MESSAGE_BLOCK_H
#define THORIUM_MESSAGE_BLOCK_H

#include <engine/thorium/message.h>

#define THORIUM_MESSAGE_BLOCK_MAXIMUM_SIZE 8

/*
 * A message block. This is used for grouping messages
 * in the worker-to-node pipe.
 */
struct thorium_message_block {
    int size;
    struct thorium_message messages[THORIUM_MESSAGE_BLOCK_MAXIMUM_SIZE];
};

void thorium_message_block_init(struct thorium_message_block *self);
void thorium_message_block_destroy(struct thorium_message_block *self);

int thorium_message_block_add_message(struct thorium_message_block *self,
                struct thorium_message *message);
struct thorium_message *thorium_message_block_get_message(struct thorium_message_block *self, int i);

int thorium_message_block_size(struct thorium_message_block *self);
int thorium_message_block_full(struct thorium_message_block *self);
int thorium_message_block_empty(struct thorium_message_block *self);
void thorium_message_block_clear(struct thorium_message_block *self);

#endif
