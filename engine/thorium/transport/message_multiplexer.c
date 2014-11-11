
#include "message_multiplexer.h"
#include "multiplexed_buffer.h"

#include <engine/thorium/node.h>

#include <performance/tracepoints/tracepoints.h>

#include <core/helpers/bitmap.h>

#include <core/system/command.h>
#include <core/system/debugger.h>
#include <core/system/memory.h>

#include <core/structures/vector.h>
#include <core/helpers/set_helper.h>
#include <core/structures/set_iterator.h>

#include <stdlib.h>

#define FORCE_NO 0
#define FORCE_YES_SIZE 1
#define FORCE_YES_TIME 1

#define FLAG_DISABLED 0

/*
#define DEBUG_MULTIPLEXER
*/

#define MEMORY_MULTIPLEXER 0xb606aa9d

/*
 * Debug the _test() function.
 */
/*
#define DEBUG_MULTIPLEXER_TEST
#define DEBUG_MULTIPLEXER_FLUSH
*/

#define OPTION_DISABLE_MULTIPLEXER "-disable-multiplexer"

/*
 * Internal function for flushing stuff away.
 */
void thorium_message_multiplexer_flush(struct thorium_message_multiplexer *self, int index, int force);


void thorium_message_multiplexer_init(struct thorium_message_multiplexer *self,
                struct thorium_node *node, struct thorium_multiplexer_policy *policy)
{
    int size;
    int i;
    int bytes;
    int position;
    struct thorium_multiplexed_buffer *multiplexed_buffer;
    int argc;
    char **argv;

    self->policy = policy;
    self->original_message_count = 0;
    self->real_message_count = 0;

    CORE_BITMAP_CLEAR(self->flags);
    CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_DISABLED);

    core_set_init(&self->buffers_with_content, sizeof(int));

    core_timer_init(&self->timer);

    self->buffer_size_in_bytes = thorium_multiplexer_policy_size_threshold(self->policy);
    self->timeout_in_nanoseconds = thorium_multiplexer_policy_time_threshold(self->policy);

    self->node = node;

    core_vector_init(&self->buffers, sizeof(struct thorium_multiplexed_buffer));

    size = thorium_node_nodes(self->node);
    core_vector_resize(&self->buffers, size);

    bytes = size * self->buffer_size_in_bytes;

#ifdef DEBUG_MULTIPLEXER
    printf("DEBUG_MULTIPLEXER size %d bytes %d\n", size, bytes);
#endif

    self->big_buffer = core_memory_allocate(bytes, MEMORY_MULTIPLEXER);
    position = 0;

    for (i = 0; i < size; ++i) {
        multiplexed_buffer = core_vector_at(&self->buffers, i);

        multiplexed_buffer->buffer = self->big_buffer + position;
        position += self->buffer_size_in_bytes;

#ifdef DEBUG_MULTIPLEXER1
        printf("DEBUG_MULTIPLEXER thorium_message_multiplexer_init index %d buffer %p\n", i, buffer);
#endif

#ifdef DEBUG_MULTIPLEXER
        printf("DEBUG_MULTIPLEXER thorium_message_multiplexer_init (after) index %d buffer %p\n", i,
                        core_vector_at(&self->buffers, i));
#endif

        multiplexed_buffer->current_size = 0;
        multiplexed_buffer->message_count = 0;
        multiplexed_buffer->maximum_size = self->buffer_size_in_bytes;
    }

    if (thorium_multiplexer_policy_is_disabled(self->policy)) {
        CORE_BITMAP_SET_BIT(self->flags, FLAG_DISABLED);
    }

    if (thorium_node_nodes(self->node) < thorium_multiplexer_policy_minimum_node_count(self->policy)) {
        CORE_BITMAP_SET_BIT(self->flags, FLAG_DISABLED);
    }

    self->worker = NULL;

    core_vector_init(&self->to_flush, sizeof(int));

    argc = node->argc;
    argv = node->argv;

    /*
     * Aside from the policy, the end user can also disable the multiplexer code path
     */
    if (core_command_has_argument(argc, argv, OPTION_DISABLE_MULTIPLEXER)) {
        CORE_BITMAP_SET_BIT(self->flags, FLAG_DISABLED);
    }
}

void thorium_message_multiplexer_destroy(struct thorium_message_multiplexer *self)
{
    int i;
    int size;
    struct thorium_multiplexed_buffer *multiplexed_buffer;
    float ratio;

    ratio = 0.0;

    if (self->original_message_count != 0) {
        ratio = self->real_message_count / (0.0 + self->original_message_count);
    }
    printf("thorium_message_multiplexer: original_message_count %d real_message_count %d (%.4f)\n",
                    self->original_message_count, self->real_message_count, ratio);

#ifdef CORE_DEBUGGER_ASSERT_ENABLED
#endif
    size = core_vector_size(&self->buffers);

    /*
     * There can be no messages that are not flushed already.
     */
    CORE_DEBUGGER_ASSERT(core_set_empty(&self->buffers_with_content));

    core_set_destroy(&self->buffers_with_content);

    for (i = 0; i < size; ++i) {
        multiplexed_buffer = core_vector_at(&self->buffers, i);

        CORE_DEBUGGER_ASSERT(multiplexed_buffer->current_size == 0);

        multiplexed_buffer->buffer = 0;
    }

    core_vector_destroy(&self->buffers);

    self->node = NULL;

    self->buffer_size_in_bytes = -1;
    self->timeout_in_nanoseconds = -1;

    core_memory_free(self->big_buffer, MEMORY_MULTIPLEXER);
    self->big_buffer = NULL;

    core_timer_destroy(&self->timer);

    core_vector_destroy(&self->to_flush);
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
    int action;
    void *buffer;
    int destination_node;
    int destination_actor;
    int new_size;
    void *multiplexed_buffer;
    void *destination_in_buffer;
    int required_size;
    struct thorium_multiplexed_buffer *real_multiplexed_buffer;
    uint64_t time;

    ++self->original_message_count;

#ifdef DEBUG_MULTIPLEXER
    printf("multiplex\n");
    thorium_message_print(message);
#endif

    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_DISABLED)) {
        ++self->real_message_count;
        return 0;
    }

    action = thorium_message_action(message);

    CORE_DEBUGGER_ASSERT(action != ACTION_INVALID);

#ifdef THORIUM_MULTIPLEXER_USE_ACTIONS_TO_SKIP
    /*
     * Don't multiplex already-multiplexed messages.
     */
    if (thorium_multiplexer_policy_is_action_to_skip(self->policy, action)) {
        ++self->real_message_count;
        return 0;
    }
#endif

    count = thorium_message_count(message);
    required_size = sizeof(count) + count;
    buffer = thorium_message_buffer(message);
    destination_actor = thorium_message_destination(message);

#ifdef DEBUG_MULTIPLEXER
    printf("DEBUG multiplex count %d required_size %d action %x\n",
                    count, required_size, action);
#endif

    /*
     * Don't multiplex non-actor messages.
     */
    if (destination_actor == THORIUM_ACTOR_NOBODY) {
        return 0;
    }

    destination_node = thorium_message_destination_node(message);

    real_multiplexed_buffer = core_vector_at(&self->buffers, destination_node);

#ifdef CORE_DEBUGGER_ASSERT
    if (real_multiplexed_buffer == NULL) {
        printf("Error action %d destination_node %d destination_actor %d\n", action, destination_node,
                        destination_actor);
    }
#endif

    CORE_DEBUGGER_ASSERT(real_multiplexed_buffer != NULL);

    current_size = real_multiplexed_buffer->current_size;
    maximum_size = real_multiplexed_buffer->maximum_size;

    /*
     * Don't multiplex large messages.
     */
    if (required_size > maximum_size) {

#ifdef DEBUG_MULTIPLEXER
        printf("too large required_size %d maximum_size %d\n", required_size, maximum_size);
#endif
        return 0;
    }

    /*
    printf("MULTIPLEX_MESSAGE\n");
    */

    new_size = current_size + required_size;

    /*
     * Flush now if there is no space left for the <required_size> bytes
     */
    if (new_size > maximum_size) {

#ifdef DEBUG_MULTIPLEXER
        printf("thorium_message_multiplexer: must FLUSH thorium_message_multiplexer_multiplex required_size %d new_size %d maximum_size %d\n",
                    required_size, new_size, maximum_size);
#endif

        thorium_message_multiplexer_flush(self, destination_node, FORCE_YES_SIZE);
        current_size = real_multiplexed_buffer->current_size;

        CORE_DEBUGGER_ASSERT(current_size == 0);
    }

    multiplexed_buffer = real_multiplexed_buffer->buffer;
    destination_in_buffer = ((char *)multiplexed_buffer) + current_size;

    /*
     * Append <count><buffer> to the <multiplexed_buffer>
     */
    core_memory_copy(destination_in_buffer, &count, sizeof(count));
    core_memory_copy((char *)destination_in_buffer + sizeof(count),
                    buffer, count);

    /*
     * If the buffer is empty before adding the data, it means that it is not
     * in the list of buffers with content and it must be added.
     */
    if (current_size == 0) {

        time = core_timer_get_nanoseconds(&self->timer);
        real_multiplexed_buffer->timestamp = time;

        core_set_add(&self->buffers_with_content, &destination_node);
    }

    /*
     * Add the message.
     */
    current_size += required_size;
    CORE_DEBUGGER_ASSERT(current_size <= maximum_size);
    real_multiplexed_buffer->current_size = current_size;
    ++real_multiplexed_buffer->message_count;

    thorium_message_multiplexer_flush(self, destination_node, FORCE_NO);

    /*
     * Verify invariant.
     */
    CORE_DEBUGGER_ASSERT(real_multiplexed_buffer->current_size <= maximum_size);

    /*
     * Inject the buffer into the worker too.
     */
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
    struct core_memory_pool *pool;
    int messages;
    int tag;
    int source_node;
    int destination_node;

#ifdef DEBUG_MULTIPLEXER
    printf("demultiplex message\n");
    thorium_message_print(message);
#endif

    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_DISABLED)) {
        return 0;
    }

    tag = thorium_message_action(message);

    if (tag != ACTION_MULTIPLEXER_MESSAGE) {
        return 0;
    }

    /*
    printf("MULTIPLEXER demultiplex\n");
    */

    messages = 0;
    tracepoint(thorium_node, demultiplex_enter, self->node->name,
                    self->node->tick, messages);

    source_node = thorium_message_source_node(message);
    destination_node = thorium_message_destination_node(message);

    /*
     * Remove the metadata from the count.
     */
    thorium_message_remove_metadata_from_count(message);

    count = thorium_message_count(message);

    buffer = thorium_message_buffer(message);

    pool = thorium_worker_get_outbound_message_memory_pool(self->worker);

    position = 0;

    /*
     * Inject a message for each enclosed message.
     */
    while (position < count) {
        core_memory_copy(&new_count, buffer + position, sizeof(new_count));
        position += sizeof(new_count);

        new_buffer = core_memory_pool_allocate(pool, new_count);
        core_memory_copy(new_buffer, buffer + position, new_count);

        thorium_message_init_with_nodes(&new_message, new_count, new_buffer,
                        source_node, destination_node);

        thorium_node_prepare_received_message(self->node, &new_message);

        /*
         * Mark the message for recycling.
         */
        thorium_message_set_worker(&new_message, thorium_worker_name(self->worker));

#ifdef CORE_DEBUGGER_ASSERT_ENABLED
        if (thorium_message_action(&new_message) == ACTION_INVALID) {
            printf("Error invalid action DEMUL Multiplexer position %d count %d new_count %d\n",
                            position, count, new_count);
            thorium_message_print(&new_message);
        }
#endif

        CORE_DEBUGGER_ASSERT(thorium_message_action(&new_message) != ACTION_INVALID);

        /*
        printf("DEMULTIPLEX_MESSAGE\n");
        */

        /*
        printf("demultiplex, local delivery: \n");
        thorium_message_print(&new_message);
        */

        thorium_worker_send_local_delivery(self->worker, &new_message);

        /*
        thorium_message_destroy(&new_message);
        */

        position += new_count;
        ++messages;
    }

    CORE_DEBUGGER_ASSERT(messages > 0);

#ifdef DEBUG_MULTIPLEXER
    printf("thorium_message_multiplexer_demultiplex %d messages\n",
                    messages);
#endif

    tracepoint(thorium_node, demultiplex_exit, self->node->name,
                    self->node->tick, messages);

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
    struct core_set_iterator iterator;
    int duration;
    int index;
    uint64_t buffer_time;
    int i;
    int size;
    struct thorium_multiplexed_buffer *multiplexed_buffer;

    CORE_DEBUGGER_ASSERT(core_vector_empty(&self->to_flush));

    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_DISABLED)) {
        return;
    }

    /*
     * Nothing to do, there is nothing to flush
     * in the system.
     */
    if (core_set_empty(&self->buffers_with_content)) {
        return;
    }

    core_set_iterator_init(&iterator, &self->buffers_with_content);

    size = core_set_size(&self->buffers_with_content);

#ifdef DEBUG_MULTIPLEXER_TEST
    if (size >= 2)
        printf("DEBUG multiplexer_test buffers with content: %d\n",
                    size);
#endif

    /*
     * Flush only the buffers with a elapsed time that is greater or equal to the
     * timeout.
     */
    time = core_timer_get_nanoseconds(&self->timer);

    while (core_set_iterator_get_next_value(&iterator, &index)) {

        multiplexed_buffer = core_vector_at(&self->buffers, index);
        buffer_time = multiplexed_buffer->timestamp;

        duration = time - buffer_time;

#ifdef DEBUG_MULTIPLEXER_TEST
        if (size >= 2)
            printf("DEBUG ....... index %d elapsed %d ns\n", index, duration);
#endif

        /*
        printf("MULTIPLEXER FLUSH buffer %d\n", index);
        */
        if (duration >= self->timeout_in_nanoseconds) {
            core_vector_push_back(&self->to_flush, &index);
        }
    }

    core_set_iterator_destroy(&iterator);

    size = core_vector_size(&self->to_flush);

    /*
     * Flush entries
     */
    for (i = 0; i < size; ++i) {
        index = core_vector_at_as_int(&self->to_flush, i);

        thorium_message_multiplexer_flush(self, index, FORCE_YES_TIME);
    }

    core_vector_clear(&self->to_flush);
}

void thorium_message_multiplexer_flush(struct thorium_message_multiplexer *self, int index, int force)
{
    char *buffer;
    struct thorium_message message;
    int tag;
    int count;
    int current_size;
    int maximum_size;
    struct thorium_multiplexed_buffer *multiplexed_buffer;
    int destination_node;
    struct core_memory_pool *pool;

    if (CORE_BITMAP_GET_BIT(self->flags, FLAG_DISABLED)) {
        return;
    }

#ifdef CORE_DEBUGGER_ASSERT_ENABLED
    if (!(core_set_find(&self->buffers_with_content, &index))) {
        multiplexed_buffer = core_vector_at(&self->buffers, index);
        printf("index %d has no content\n", index);

        thorium_multiplexed_buffer_print(multiplexed_buffer);
    }
#endif

    CORE_DEBUGGER_ASSERT(core_set_find(&self->buffers_with_content, &index));

    multiplexed_buffer = core_vector_at(&self->buffers, index);
    current_size = multiplexed_buffer->current_size;
    maximum_size = multiplexed_buffer->maximum_size;

    if (force == FORCE_NO && current_size < maximum_size) {
        return;
    }

    CORE_DEBUGGER_ASSERT(current_size > 0);

    count = current_size + THORIUM_MESSAGE_METADATA_SIZE;
    pool = thorium_worker_get_outbound_message_memory_pool(self->worker);
    buffer = core_memory_pool_allocate(pool, count);

    tag = ACTION_MULTIPLEXER_MESSAGE;

    /*
     * This count does not include metadata for the final big message.
     *
     * TODO: Avoid this copy by using an array of pointers in the first place.
     */
    core_memory_copy(buffer, multiplexed_buffer->buffer, current_size);

    destination_node = index;

    thorium_message_init(&message, tag, count, buffer);
    thorium_message_set_destination(&message,
                    destination_node);
    thorium_message_set_source(&message,
            thorium_node_name(self->node));
    /*
     * Mark the message so that the buffer is eventually sent back here
     * for recycling.
     */
    thorium_message_set_worker(&message, thorium_worker_name(self->worker));

    thorium_message_write_metadata(&message);

#ifdef DEBUG_MULTIPLEXER_FLUSH
    printf("DEBUG_MULTIPLEXER thorium_message_multiplexer_flush index %d buffer %p force %d message_count %d current_size %d maximum_size %d"
                    " destination_node %d\n",
                    index, buffer, force, multiplexed_buffer->message_count,
                    current_size, maximum_size,
                    thorium_message_destination_node(&message));

    printf("message in flush\n");
    thorium_message_print(&message);
#endif

    CORE_DEBUGGER_ASSERT_NOT_NULL(self->worker);

    /*
     * Make a copy of the buffer because the multiplexer does not have communication buffers.
     */
    thorium_worker_enqueue_outbound_message(self->worker, &message);

    /*
    printf("MULTIPLEXER FLUSH\n");
    */

    ++self->real_message_count;
    thorium_message_destroy(&message);

    multiplexed_buffer->current_size = 0;
    multiplexed_buffer->message_count = 0;
    multiplexed_buffer->timestamp = 0;

    core_set_delete(&self->buffers_with_content, &index);
}

int thorium_message_multiplexer_is_disabled(struct thorium_message_multiplexer *self)
{
    return CORE_BITMAP_GET_BIT(self->flags, FLAG_DISABLED);
}

void thorium_message_multiplexer_set_worker(struct thorium_message_multiplexer *self,
                struct thorium_worker *worker)
{
    self->worker = worker;

    if (thorium_node_name(self->node) == 0 && thorium_worker_name(self->worker) == 0) {
        if (self->timeout_in_nanoseconds == THORIUM_DYNAMIC_TIMEOUT) {
            printf("thorium_message_multiplexer: disabled=%d buffer_size_in_bytes=%d timeout_in_nanoseconds=dynamic\n",
                            CORE_BITMAP_GET_BIT(self->flags, FLAG_DISABLED),
                        self->buffer_size_in_bytes);
        } else {
            printf("thorium_message_multiplexer: disabled=%d buffer_size_in_bytes=%d timeout_in_nanoseconds=%d\n",
                            CORE_BITMAP_GET_BIT(self->flags, FLAG_DISABLED),
                        self->buffer_size_in_bytes, self->timeout_in_nanoseconds);
        }
    }
}

int thorium_message_multiplexer_message_should_be_multiplexed(struct thorium_message_multiplexer *self,
                struct thorium_message *message)
{
    int buffer_size;
    int threshold;

    if (thorium_message_multiplexer_is_disabled(self))
        return 0;

    buffer_size = thorium_message_count(message);
    threshold = self->buffer_size_in_bytes / 2;

    return buffer_size <= threshold;
}
