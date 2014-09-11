
#include "message_multiplexer.h"

#include <engine/thorium/node.h>

#include <core/helpers/bitmap.h>

#include <core/system/debugger.h>
#include <core/system/memory.h>

#include <core/structures/vector.h>
#include <core/structures/set_iterator.h>

#include <stdlib.h>

#define ACTION_MULTIPLEXER_MESSAGE 0x0024afc9

#define FORCE_NO 0
#define FORCE_YES 1

#define FLAG_DISABLE 0

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
    int size;
    int i;
    int bytes;
    char *buffer;
    int position;
    int buffer_size;
    int buffer_max_size;

    self->flags = 0;
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_DISABLE);

    bsal_set_init(&self->buffers_with_content, sizeof(int));

    bsal_timer_init(&self->timer);

    self->size_threshold_in_bytes = THORIUM_MESSAGE_MULTIPLEXER_SIZE_THRESHOLD_IN_BYTES;
    self->time_threshold_in_nanoseconds = THORIUM_MESSAGE_MULTIPLEXER_SIZE_THRESHOLD_IN_BYTES;

    self->node = node;

    bsal_vector_init(&self->buffers, sizeof(void *));
    bsal_vector_init(&self->buffer_current_sizes, sizeof(int));
    bsal_vector_init(&self->buffer_maximum_sizes, sizeof(int));

    size = thorium_node_nodes(self->node);

    bsal_vector_resize(&self->buffers, size);
    bsal_vector_resize(&self->buffer_current_sizes, size);
    bsal_vector_resize(&self->buffer_maximum_sizes, size);

    bytes = size * self->size_threshold_in_bytes;

    printf("DEBUG_MULTIPLEXER size %d bytes %d\n", size, bytes);

    self->big_buffer = bsal_memory_allocate(bytes);
    position = 0;

    for (i = 0; i < size; ++i) {
        buffer = self->big_buffer + position;
        position += self->size_threshold_in_bytes;

        printf("DEBUG_MULTIPLEXER thorium_message_multiplexer_init index %d buffer %p\n", i, buffer);
        bsal_vector_set(&self->buffers, i, &buffer);
        printf("DEBUG_MULTIPLEXER thorium_message_multiplexer_init (after) index %d buffer %p\n", i,
                        bsal_vector_at_as_void_pointer(&self->buffers, i));

        buffer_size = 0;
        bsal_vector_set(&self->buffer_current_sizes, i, &buffer_size);

        buffer_max_size = self->size_threshold_in_bytes;
        bsal_vector_set(&self->buffer_maximum_sizes, i, &buffer_max_size);
    }

    self->last_flush = bsal_timer_get_nanoseconds(&self->timer);

    bsal_bitmap_set_bit_uint32_t(&self->flags, FLAG_DISABLE);
}

void thorium_message_multiplexer_destroy(struct thorium_message_multiplexer *self)
{
    int i;
    int size;
#ifdef BSAL_DEBUGGER_ENABLE_ASSERT
#endif
    size = bsal_vector_size(&self->buffers);

    BSAL_DEBUGGER_ASSERT(bsal_set_empty(&self->buffers_with_content));

    bsal_set_destroy(&self->buffers_with_content);

    for (i = 0; i < size; ++i) {

        BSAL_DEBUGGER_ASSERT(bsal_vector_at_as_int(&self->buffer_current_sizes, i) == 0);
    }

    bsal_vector_destroy(&self->buffers);
    bsal_vector_destroy(&self->buffer_current_sizes);
    bsal_vector_destroy(&self->buffer_maximum_sizes);

    self->node = NULL;

    self->size_threshold_in_bytes = -1;
    self->time_threshold_in_nanoseconds = -1;

    bsal_memory_free(self->big_buffer);
    self->big_buffer = NULL;

    self->last_flush = 0;

    bsal_timer_destroy(&self->timer);
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

    int count;
    int current_size;
    int maximum_size;
    int tag;
    void *buffer;
    int destination_node;
    int new_size;
    void *multiplexed_buffer;
    void *destination_in_buffer;
    int required_size;

    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLE)) {
        return 0;
    }

    tag = thorium_message_tag(message);

    /*
     * Don't multiplex already-multiplexed messages.
     */
    if (tag == ACTION_MULTIPLEXER_MESSAGE) {
        return 0;
    }

    count = thorium_message_count(message);
    required_size = sizeof(count) + count;
    buffer = thorium_message_buffer(message);
    destination_node = thorium_message_destination_node(message);
    current_size = bsal_vector_at_as_int(&self->buffer_current_sizes, destination_node);
    maximum_size = bsal_vector_at_as_int(&self->buffer_maximum_sizes, destination_node);

    /*
     * Don't multiplex large messages.
     */
    if (required_size > maximum_size) {
        return 0;
    }

    printf("DEBUG_MULTIPLEXER thorium_message_multiplexer_multiplex required_size %d\n",
                    required_size);

    new_size = current_size + required_size;

    /*
     * Flush now if there is no space left for the <required_size> bytes
     */
    if (new_size > maximum_size) {
        thorium_message_multiplexer_flush(self, destination_node, FORCE_YES);
        current_size = bsal_vector_at_as_int(&self->buffer_current_sizes, destination_node);

        BSAL_DEBUGGER_ASSERT(current_size == 0);
    }

    multiplexed_buffer = bsal_vector_at_as_void_pointer(&self->buffers, destination_node);
    destination_in_buffer = ((char *)multiplexed_buffer) + current_size;

    /*
     * Append <count><buffer> to the <multiplexed_buffer>
     */
    bsal_memory_copy(destination_in_buffer, &count, sizeof(count));
    bsal_memory_copy((char *)destination_in_buffer + sizeof(count),
                    buffer, count);

    current_size += required_size;

    BSAL_DEBUGGER_ASSERT(current_size <= maximum_size);
    bsal_vector_set(&self->buffer_current_sizes, destination_node, &current_size);

    thorium_message_multiplexer_flush(self, destination_node, FORCE_NO);

    current_size = bsal_vector_at_as_int(&self->buffer_current_sizes, destination_node);

    BSAL_DEBUGGER_ASSERT(current_size <= maximum_size);

    /*
     * Add the key for this buffer with content.
     */
    if (current_size > 0) {
        bsal_set_add(&self->buffers_with_content, &destination_node);
    }

    return 1;
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
     *     for every enclosed message
     *         call thorium_node_prepare_received_message
     *         call thorium_node_dispatch_message()
     *     return 1
     *
     * return 0
     */

    int count;
    char *buffer;
    struct thorium_message new_message;
    int new_count;
    void *new_buffer;
    int position;
    struct bsal_memory_pool *pool;
    int messages;
    int tag;
    int source_node;
    int destination_node;

    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLE)) {
        return 0;
    }

    tag = thorium_message_tag(message);

    if (tag != ACTION_MULTIPLEXER_MESSAGE) {
        return 0;
    }

    source_node = thorium_message_source_node(message);
    destination_node = thorium_message_destination_node(message);
    count = thorium_message_count(message);
    buffer = thorium_message_buffer(message);
    pool = thorium_node_inbound_memory_pool(self->node);

    position = 0;
    messages = 0;

    /*
     * Inject a message for each enclosed message.
     */
    while (position < count) {
        bsal_memory_copy(&new_count, buffer + position, sizeof(new_count));
        position += sizeof(new_count);

        new_buffer = bsal_memory_pool_allocate(pool, new_count);
        bsal_memory_copy(new_buffer, buffer + position, new_count);

        thorium_message_init_with_nodes(&new_message, new_count, new_buffer,
                        source_node, destination_node);

        thorium_node_prepare_received_message(self->node, &new_message);
        thorium_node_dispatch_message(self->node, &new_message);

        thorium_message_destroy(&new_message);

        position += new_count;
        ++messages;
    }

    BSAL_DEBUGGER_ASSERT(messages > 0);

    printf("DEBUG_MULTIPLEXER thorium_message_multiplexer_demultiplex %d messages\n",
                    messages);

    return 1;
}

/*
 * This is O(n), that is if all thorium nodes have a buffer to flush.
 */
void thorium_message_multiplexer_test(struct thorium_message_multiplexer *self)
{
    /*
     * Check if the multiplexer has waited enough.
     */

    uint64_t time;
    struct bsal_set_iterator iterator;
    int duration;
    int index;

    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLE)) {
        return;
    }

    time = bsal_timer_get_nanoseconds(&self->timer);

    duration = time - self->last_flush;

    if (duration < self->time_threshold_in_nanoseconds) {
        return;
    }

    if (bsal_set_empty(&self->buffers_with_content)) {
        return;
    }

    bsal_set_iterator_init(&iterator, &self->buffers_with_content);

    while (bsal_set_iterator_get_next_value(&iterator, &index)) {

        thorium_message_multiplexer_flush(self, index, FORCE_YES);
    }

    bsal_set_iterator_destroy(&iterator);

    bsal_set_clear(&self->buffers_with_content);
}

void thorium_message_multiplexer_flush(struct thorium_message_multiplexer *self, int index, int force)
{
    char *buffer;
    struct thorium_message message;
    int tag;
    int count;
    int current_size;
    int maximum_size;

    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_DISABLE)) {
        return;
    }

    current_size = bsal_vector_at_as_int(&self->buffer_current_sizes, index);
    maximum_size = bsal_vector_at_as_int(&self->buffer_maximum_sizes, index);

    if (force == FORCE_NO && current_size < maximum_size) {
        return;
    }

    count = bsal_vector_at_as_int(&self->buffer_current_sizes, index);
    tag = ACTION_MULTIPLEXER_MESSAGE;
    buffer = bsal_vector_at_as_void_pointer(&self->buffers, index);

    printf("DEBUG_MULTIPLEXER thorium_message_multiplexer_flush index %d buffer %p force %d current_size %d maximum_size %d\n",
                    index, buffer, force, current_size, maximum_size);

    thorium_message_init(&message, tag, count, buffer);
    thorium_node_send_to_node(self->node, index, &message);
    thorium_message_destroy(&message);

    current_size = 0;

    bsal_vector_set(&self->buffer_current_sizes, index, &current_size);
}
