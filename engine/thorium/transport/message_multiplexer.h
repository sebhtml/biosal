
#ifndef THORIUM_MESSAGE_MULTIPLEXER_H
#define THORIUM_MESSAGE_MULTIPLEXER_H

struct thorium_node;
struct thorium_message;

#include <core/structures/vector.h>

/*
 * A message multiplerxser.
 * The 2 options are:
 *
 * size_threshold_in_bytes
 * time_threshold_in_nanoseconds
 */
struct thorium_message_multiplexer {
    int size_threshold_in_bytes;
    int time_threshold_in_nanoseconds;

    struct bsal_vector buffers;
    struct bsal_vector buffer_maximum_sizes;
    struct bsal_vector buffer_current_sizes;

    struct thorium_node *node;
};

void thorium_message_multiplexer_init(struct thorium_message_multiplexer *self,
                struct thorium_node *node);
void thorium_message_multiplexer_destroy(struct thorium_message_multiplexer *self);

/*
 * Returns 1 if the message was multiplexed.
 */
int thorium_message_multiplexer_multiplex(struct thorium_message_multiplexer *self,
                struct thorium_message *message);
/*
 * Returns 1 if the message was demultiplexed.
 */
int thorium_message_multiplexer_demultiplex(struct thorium_message_multiplexer *self,
                struct thorium_message *message);

/*
 * Test the multiplexer. This is used to flush messages that have been waiting for too long.
 */
void thorium_message_multiplexer_test(struct thorium_message_multiplexer *self);

#endif
