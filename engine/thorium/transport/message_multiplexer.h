
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
 * Genomic graph traversal is characterized by:
 *
 * - a high ratio of consumer actors (visitors, walkers, and so on) to producer actors (graph stores)
 *   - small messages (example: ACTION_ASSEMBLY_GET_VERTEX)
 *
 *   Today, I implemented a multiplexer in thorium (< 500 lines).
 *
 *   With the current setting, it is giving a 25% decrease in running time on my small example.
 *   (parameters are 1 KiB for buffers and 512 us for maximum length of accumulation window
 *
 *
 *   It is very modular too.
 *
 *   I added 5 calls (around 10 lines)
 *   in thorium_node (in node.c, init, destroy, and 3 hooks: multiplex, demultiplex, test)
 */

/*
 * Size threshold.
 *
 * The current value is 1 KiB.
 */
#define THORIUM_MESSAGE_MULTIPLEXER_SIZE_THRESHOLD_IN_BYTES (1 * 1024)

/*
 * Time threshold in microseconds.
 *
 * The current value is 2000 us (2 ms).
 *
 * There are 1 000 ms in 1 second
 * There are 1 000 000 us in 1 second.
 * There are 1 000 000 000 ns in 1 second.
 */
#define THORIUM_MESSAGE_MULTIPLEXER_TIME_THRESHOLD_IN_NANOSECONDS (2 * 1000 * 1000)

/*
 * A message multiplerxser.
 * The 2 options are:
 *
 * threshold_buffer_size_in_bytes
 * threshold_time_in_nanoseconds
 */
struct thorium_message_multiplexer {
    struct bsal_timer timer;

    struct bsal_vector buffers;
    struct bsal_set buffers_with_content;

    struct thorium_node *node;
    char *big_buffer;

    uint32_t flags;

    /*
     * This is the maximum buffer size for a multiplexed message.
     * A multiplexed message contains smaller messages. These smaller
     * messages must be smaller than <threshold_buffer_size_in_bytes>
     */
    int threshold_buffer_size_in_bytes;

    /*
     * At some point (in thorium_message_multiplexer_test),
     * the multiplexer will flush messages even if they are below
     * <threshold_buffer_size_in_bytes>.
     * 
     * To do so, the maximum duration of <threshold_time_in_nanoseconds>
     * nanoseconds is utilized.
     */
    int threshold_time_in_nanoseconds;

    int original_message_count;
    int real_message_count;

    uint64_t last_flush;
};

void thorium_message_multiplexer_init(struct thorium_message_multiplexer *self,
                struct thorium_node *node, int threshold_buffer_size_in_bytes,
                int threshold_time_in_nanoseconds);
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
