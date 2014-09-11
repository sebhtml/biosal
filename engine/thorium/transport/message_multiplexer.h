
#ifndef THORIUM_MESSAGE_MULTIPLEXER_H
#define THORIUM_MESSAGE_MULTIPLEXER_H

struct thorium_node;
struct thorium_message;

#include <core/structures/vector.h>
#include <core/structures/set.h>
#include <core/structures/set_iterator.h>

#include <core/system/timer.h>

#include <stdint.h>

/*
 * A message multiplerxser.
 * The 2 options are:
 *
 * size_threshold_in_bytes
 * time_threshold_in_nanoseconds
 */
struct thorium_message_multiplexer {
    struct bsal_timer timer;

    struct bsal_vector buffers;
    struct bsal_set buffers_with_content;

    struct thorium_node *node;
    char *big_buffer;

    uint32_t flags;

    int size_threshold_in_bytes;
    int time_threshold_in_nanoseconds;

    int original_message_count;
    int real_message_count;

    uint64_t last_flush;
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

/*
 * Internal function for flushing stuff away.
 */
void thorium_message_multiplexer_flush(struct thorium_message_multiplexer *self, int index, int force);

#endif
