
#include "message_multiplexer.h"

#include <core/system/debugger.h>
#include <core/structures/vector.h>

#include <stdlib.h>

/*
 * 4 KiB
 */
#define THORIUM_MESSAGE_MULTIPLEXER_SIZE_THRESHOLD_IN_BYTES (1 * 1024)

/*
 * 10 microseconds.
 */
#define THORIUM_MESSAGE_MULTIPLEXER_TIME_THRESHOLD_IN_NANOSECONDS (10 * 1000)

void thorium_message_multiplexer_init(struct thorium_message_multiplexer *self,
                struct thorium_node *node)
{
    self->size_threshold_in_bytes = THORIUM_MESSAGE_MULTIPLEXER_SIZE_THRESHOLD_IN_BYTES;
    self->time_threshold_in_nanoseconds = THORIUM_MESSAGE_MULTIPLEXER_SIZE_THRESHOLD_IN_BYTES;

    self->node = node;

    bsal_vector_init(&self->buffers, sizeof(void *));
    bsal_vector_init(&self->buffer_current_sizes, sizeof(int));
    bsal_vector_init(&self->buffer_maximum_sizes, sizeof(int));
}

void thorium_message_multiplexer_destroy(struct thorium_message_multiplexer *self)
{
    int i;
    int size;

#ifdef BSAL_DEBUGGER_ENABLE_ASSERT
#endif
    size = bsal_vector_size(&self->buffers);

    for (i = 0; i < size; ++i) {

        BSAL_DEBUGGER_ASSERT(bsal_vector_at_as_int(&self->buffer_current_sizes, i) == 0);

        bsal_vector_destroy(&self->buffers);
        bsal_vector_destroy(&self->buffer_current_sizes);
        bsal_vector_destroy(&self->buffer_maximum_sizes);
    }

    self->node = NULL;

    self->size_threshold_in_bytes = -1;
    self->time_threshold_in_nanoseconds = -1;
}

/*
 * Returns 1 if the message was multiplexed.
 *
 * This is O(1) in regard to the number of thorium nodes.
 */
int thorium_message_multiplexer_multiplex(struct thorium_message_multiplexer *self,
                struct thorium_message *message)
{
    /*
     * If buffer is full, use thorium_node_send_with_transport
     *
     * get count
     *
     * if count is below or equal to the threshold
     *      multiplex the message.
     *      return 1
     *
     * return 0
     */
    return 0;
}

/*
 * Returns 1 if the message was demultiplexed.
 *
 * This is O(1) in regard to the number of thorium nodes.
 */
int thorium_message_multiplexer_demultiplex(struct thorium_message_multiplexer *self,
                struct thorium_message *message)
{
    /*
     * Algorithm:
     *
     * get tag.
     * if tag is ACTION_MULTIPLEXER_MESSAGE
     *   call thorium_node_dispatch_message() for every enclosed message
     *   return 1
     *
     * return 0
     */
    return 0;
}

/*
 * This is O(n), that is if all thorium nodes have a buffer to flush.
 */
void thorium_message_multiplexer_test(struct thorium_message_multiplexer *self)
{
    /*
     * Check if the multiplexer has waited enough.
     */
}
