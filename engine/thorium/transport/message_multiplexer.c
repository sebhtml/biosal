
#include "message_multiplexer.h"
#include "multiplexed_buffer.h"

#include <engine/thorium/node.h>

#include <tracepoints/tracepoints.h>

#include <core/helpers/bitmap.h>

#include <core/system/command.h>
#include <core/system/debugger.h>
#include <core/system/memory.h>

#include <core/structures/vector.h>
#include <core/helpers/set_helper.h>

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#define FORCE_NO            0
#define FORCE_YES_SIZE      1
#define FORCE_YES_TIME      2
#define FORCE_YES_DOA       3

#define FLAG_DISABLED CORE_BITMAP_MAKE_FLAG(0)

/*
#define DEBUG_MULTIPLEXER
*/

#define MEMORY_MULTIPLEXER 0xb606aa9d

/*
 * Debug the _test() function.
 */
/*
#define DEBUG_MULTIPLEXER_TEST
#define DEBUG_MINIMUM_COUNT 3
#define DEBUG_MULTIPLEXER_FLUSH
*/

#define OPTION_DISABLE_MULTIPLEXER "-disable-multiplexer"

#define MULTIPLEXER_IS_VERBOSE
/*
*/

/*
 * Internal function for flushing stuff away.
 */
void thorium_message_multiplexer_flush(struct thorium_message_multiplexer *self, int index, int force);

void thorium_message_multiplexer_print_traffic_reduction(struct thorium_message_multiplexer *self);

void thorium_message_multiplexer_update_timeout(struct thorium_message_multiplexer *self);

void thorium_message_multiplexer_init(struct thorium_message_multiplexer *self,
                struct thorium_node *node, struct thorium_multiplexer_policy *policy)
{
    int size;
    int i;
    /*
    int bytes;
    */
    int position;
    struct thorium_multiplexed_buffer *multiplexed_buffer;
    int argc;
    char **argv;

    thorium_decision_maker_init(&self->decision_maker);

    self->policy = policy;
    self->original_message_count = 0;
    self->real_message_count = 0;

    CORE_BITMAP_CLEAR_FLAGS(self->flags);
    CORE_BITMAP_CLEAR_FLAG(self->flags, FLAG_DISABLED);

#ifdef THORIUM_MULTIPLEXER_TRACK_BUFFERS_WITH_CONTENT
    core_set_init(&self->buffers_with_content, sizeof(int));
#endif

    core_timer_init(&self->timer);

    self->buffer_size_in_bytes = thorium_multiplexer_policy_size_threshold(self->policy);

#ifdef CONFIG_MULTIPLEXER_USE_DECISION_MAKER
    self->timeout_in_nanoseconds = thorium_decision_maker_get_best_timeout(&self->decision_maker,
                    THORIUM_TIMEOUT_NO_VALUE);
#else
    self->timeout_in_nanoseconds = self->policy->threshold_time_in_nanoseconds;
#endif

    CORE_DEBUGGER_ASSERT(self->timeout_in_nanoseconds >= 0);

    self->node = node;

    core_vector_init(&self->buffers, sizeof(struct thorium_multiplexed_buffer));

    size = thorium_node_nodes(self->node);
    core_vector_resize(&self->buffers, size);

    /*
    bytes = size * self->buffer_size_in_bytes;
    */

#ifdef DEBUG_MULTIPLEXER
    printf("DEBUG_MULTIPLEXER size %d bytes %d\n", size, bytes);
#endif

    position = 0;

    for (i = 0; i < size; ++i) {
        multiplexed_buffer = core_vector_at(&self->buffers, i);

        CORE_DEBUGGER_ASSERT(multiplexed_buffer != NULL);

        /*
         * Initially, these multiplexed buffers have a NULL buffer.
         * It is only allocated when needed because each worker is an exporter
         * of small messages for a subset of all the destination nodes.
         */
        thorium_multiplexed_buffer_init(multiplexed_buffer, self->buffer_size_in_bytes,
                        self->timeout_in_nanoseconds);

        position += self->buffer_size_in_bytes;

#ifdef DEBUG_MULTIPLEXER1
        printf("DEBUG_MULTIPLEXER thorium_message_multiplexer_init index %d buffer %p\n", i, buffer);
#endif

#ifdef DEBUG_MULTIPLEXER
        printf("DEBUG_MULTIPLEXER thorium_message_multiplexer_init (after) index %d buffer %p\n", i,
                        core_vector_at(&self->buffers, i));
#endif
    }

    if (thorium_multiplexer_policy_is_disabled(self->policy)) {
        CORE_BITMAP_SET_FLAG(self->flags, FLAG_DISABLED);
    }

    if (thorium_node_nodes(self->node) < thorium_multiplexer_policy_minimum_node_count(self->policy)) {
        CORE_BITMAP_SET_FLAG(self->flags, FLAG_DISABLED);
    }

    self->worker = NULL;

    argc = node->argc;
    argv = node->argv;

    /*
     * Aside from the policy, the end user can also disable the multiplexer code path
     */
    if (core_command_has_argument(argc, argv, OPTION_DISABLE_MULTIPLEXER)) {
        CORE_BITMAP_SET_FLAG(self->flags, FLAG_DISABLED);
    }

    self->last_send_event_count = 0;
    self->last_time = core_timer_get_nanoseconds(&self->timer);
    self->last_update_time = time(NULL);

    self->degree_of_aggregation_limit = self->policy->degree_of_aggregation_limit;
}

void thorium_message_multiplexer_destroy(struct thorium_message_multiplexer *self)
{
    int i;
    int size;
    struct thorium_multiplexed_buffer *multiplexed_buffer;
    float ratio;

    if (thorium_node_must_print_data(self->node)) {

        ratio = 0.0;

        if (self->original_message_count != 0) {
            ratio = self->real_message_count / (0.0 + self->original_message_count);

            /*
             * Jack M. Nilles
             * "Traffic reduction by telecommuting: A status review and selected bibliography"
             * Transportation Research Part A: General
             * Volume 22, Issue 4, July 1988, Pages 301–317.
             *
             * @see http://www.sciencedirect.com/science/article/pii/0191260788900088
             * @see http://ww2.cityofpasadena.net/councilagendas/2007%20agendas/feb_26_07/pasadena%20traffic%20reduction%20strategies%2011-20-06%20draft.pdf
             */
            printf("[thorium] node %d worker %d message_multiplexer:"
                            " original_message_count %d real_message_count %d (traffic reduction: %.2f%%)\n",
                            thorium_node_name(self->node), thorium_worker_name(self->worker),
                    self->original_message_count, self->real_message_count, (1.0 - ratio) * 100);

            thorium_message_multiplexer_print_traffic_reduction(self);
        }
    }

#ifdef CORE_DEBUGGER_ASSERT_ENABLED
#endif
    size = core_vector_size(&self->buffers);

#ifdef THORIUM_MULTIPLEXER_TRACK_BUFFERS_WITH_CONTENT
    /*
     * There can be no messages that are not flushed already.
     */
    CORE_DEBUGGER_ASSERT(core_set_empty(&self->buffers_with_content));

    core_set_destroy(&self->buffers_with_content);
#endif

    for (i = 0; i < size; ++i) {
        multiplexed_buffer = core_vector_at(&self->buffers, i);

        CORE_DEBUGGER_ASSERT(thorium_multiplexed_buffer_current_size(multiplexed_buffer) == 0);

        thorium_multiplexed_buffer_destroy(multiplexed_buffer);
    }

    core_vector_destroy(&self->buffers);

    self->node = NULL;

    self->buffer_size_in_bytes = -1;
    self->timeout_in_nanoseconds = -1;

    core_timer_destroy(&self->timer);

#ifdef THORIUM_MULTIPLEXER_USE_TREE
    core_red_black_tree_destroy(&self->timeline);
#elif defined(THORIUM_MULTIPLEXER_USE_HEAP)
    core_binary_heap_destroy(&self->timeline);
#elif defined(THORIUM_MULTIPLEXER_USE_QUEUE)
    core_queue_destroy(&self->timeline);

#endif

    thorium_decision_maker_destroy(&self->decision_maker);
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
    struct core_memory_pool *pool;
    void *new_buffer;
    int new_count;
    void *buffer;
    int destination_node;
    int destination_actor;
    int new_size;
    int required_size;
    struct thorium_multiplexed_buffer *real_multiplexed_buffer;
    uint64_t time;

#ifdef DEBUG_MULTIPLEXER
    printf("multiplex\n");
    thorium_message_print(message);
#endif

    if (CORE_BITMAP_GET_FLAG(self->flags, FLAG_DISABLED)) {
        return 0;
    }

    action = thorium_message_action(message);

    CORE_DEBUGGER_ASSERT(action != ACTION_INVALID);

#ifdef THORIUM_MULTIPLEXER_USE_ACTIONS_TO_SKIP
    /*
     * Don't multiplex already-multiplexed messages.
     */
    if (thorium_multiplexer_policy_is_action_to_skip(self->policy, action)) {
        return 0;
    }
#endif

#ifdef CONFIG_MULTIPLEXER_USE_DECISION_MAKER
    thorium_message_multiplexer_update_timeout(self);
#endif

    ++self->original_message_count;

    count = thorium_message_count(message);

    destination_node = thorium_message_destination_node(message);

    real_multiplexed_buffer = core_vector_at(&self->buffers, destination_node);

    CORE_DEBUGGER_ASSERT(real_multiplexed_buffer != NULL);

    required_size = thorium_multiplexed_buffer_required_size(real_multiplexed_buffer, count);

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

#ifdef CORE_DEBUGGER_ASSERT
    if (real_multiplexed_buffer == NULL) {
        printf("Error action %d destination_node %d destination_actor %d\n", action, destination_node,
                        destination_actor);
    }
#endif

    current_size = thorium_multiplexed_buffer_current_size(real_multiplexed_buffer);
    maximum_size = thorium_multiplexed_buffer_maximum_size(real_multiplexed_buffer);

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
        current_size = thorium_multiplexed_buffer_current_size(real_multiplexed_buffer);

        CORE_DEBUGGER_ASSERT(current_size == 0);
    }

    time = core_timer_get_nanoseconds(&self->timer);

    /*
     * If the buffer is empty before adding the data, it means that it is not
     * in the list of buffers with content and it must be added.
     */
    if (current_size == 0) {

        thorium_multiplexed_buffer_set_time(real_multiplexed_buffer, time);

#ifdef THORIUM_MULTIPLEXER_TRACK_BUFFERS_WITH_CONTENT
        core_set_add(&self->buffers_with_content, &destination_node);
#endif

        /*
         * Add it to the timeline.
         */

#ifdef THORIUM_MULTIPLEXER_USE_TREE
        core_red_black_tree_add_key_and_value(&self->timeline, &time, &destination_node);

#elif defined(THORIUM_MULTIPLEXER_USE_HEAP)
        core_binary_heap_insert(&self->timeline, &time, &destination_node);
#elif defined(THORIUM_MULTIPLEXER_USE_QUEUE)
        core_queue_enqueue(&self->timeline, &destination_node);
#endif

    }

    /*
     * The allocation of buffer is lazy.
     * The current worker is an exporter of small message for the destination
     * "destination_node".
     */
    if (thorium_multiplexed_buffer_buffer(real_multiplexed_buffer) == NULL) {
        pool = thorium_worker_get_outbound_message_memory_pool(self->worker);

        new_count = self->buffer_size_in_bytes + THORIUM_MESSAGE_METADATA_SIZE;
        new_buffer = core_memory_pool_allocate(pool, new_count);

        thorium_multiplexed_buffer_set_buffer(real_multiplexed_buffer, new_buffer);
    }

    /*
    printf("DEBUG worker_latency %d ns\n",
                    thorium_worker_latency(self->worker));
                    */

    thorium_multiplexed_buffer_append(real_multiplexed_buffer, count, buffer, time);

    /*
     * Try to flush. This only flushes something if the buffer is full.
     */
    /*
    thorium_message_multiplexer_flush(self, destination_node, FORCE_YES_DOA);
    thorium_message_multiplexer_flush(self, destination_node, FORCE_YES_TIME);
    */

    /*
     * Verify invariant.
     */
    CORE_DEBUGGER_ASSERT(thorium_multiplexed_buffer_current_size(real_multiplexed_buffer)<= maximum_size);

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

    if (CORE_BITMAP_GET_FLAG(self->flags, FLAG_DISABLED)) {
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
    int duration;
    int index;
    uint64_t buffer_time;
    struct thorium_multiplexed_buffer *multiplexed_buffer;

#if defined(THORIUM_MULTIPLEXER_USE_TREE) || defined(THORIUM_MULTIPLEXER_USE_HEAP)
    uint64_t *lowest_key;
    int *index_bucket;
#endif

    int timeout;
#ifdef CHECK_PREDICTED_TRAFFIC_REDUCTION
    double acceptable_traffic_reduction;
    double traffic_reduction;
#endif
    int message_count;

    if (CORE_BITMAP_GET_FLAG(self->flags, FLAG_DISABLED)) {
        return;
    }

#ifdef THORIUM_MULTIPLEXER_TRACK_BUFFERS_WITH_CONTENT
    /*
     * Nothing to do, there is nothing to flush
     * in the system.
     */
    if (core_set_empty(&self->buffers_with_content)) {
        return;
    }
#endif

#if 1
    /*
     * Don't flush anything if the outbound ring is full,
     * which means there is congestion.
     *
     * This means that the throughput is at its maximum value. In that case,
     * it is better to generate even larger messages.
     */
    if (thorium_worker_has_outbound_traffic_congestion(self->worker)) {
        return;
    }
#endif

#ifdef DEBUG_MULTIPLEXER_TEST
    if (size >= DEBUG_MINIMUM_COUNT)
        printf("DEBUG multiplexer_test buffers with content: %d\n",
                    size);
#endif

#ifdef CHECK_PREDICTED_TRAFFIC_REDUCTION
    /*
     * 0.95 corresponds to 20 actor messages in 1 network message.
     * 0.90 corresponds to 10 actor messages in 1 network message.
     */
    acceptable_traffic_reduction = 0.90;
#endif

    /*
     * Get the destination with the oldest buffer.
     * If this one has not waited enough, then any other more recent
     * buffer has not waited enough neither.
     */
    while (1) {
#ifdef THORIUM_MULTIPLEXER_USE_TREE
        lowest_key = core_red_black_tree_get_lowest_key(&self->timeline);
        /*
        * The timeline is empty.
        */
        if (lowest_key == NULL)
            return;

#elif defined(THORIUM_MULTIPLEXER_USE_HEAP)
        lowest_key = NULL;
        core_binary_heap_get_root(&self->timeline, (void **)&lowest_key,
                            (void **)&index_bucket);
        /*
        * The timeline is empty.
        */
        if (lowest_key == NULL)
            return;

#elif defined(THORIUM_MULTIPLEXER_USE_QUEUE)

        if (!core_queue_dequeue(&self->timeline, &index)) {
            return;
        }

#endif

        /*
        * Get the index.
        * The index is the destination.
        */
#ifdef THORIUM_MULTIPLEXER_USE_TREE
        index_bucket = core_red_black_tree_get(&self->timeline, lowest_key);
        index = *index_bucket;
#endif

        multiplexed_buffer = core_vector_at(&self->buffers, index);
        buffer_time = thorium_multiplexed_buffer_time(multiplexed_buffer);
        message_count = multiplexed_buffer->message_count_;
    
        /*
        * Flush only the buffers with a elapsed time that is greater or equal to the
        * timeout.
        */
        time = core_timer_get_nanoseconds(&self->timer);
    
        /*
        * Get the current time. This current time will be compared
        * with the virtual time of each item in the timeline.
        */
        duration = time - buffer_time;

        timeout = self->timeout_in_nanoseconds;

#ifdef CHECK_PREDICTED_TRAFFIC_REDUCTION
        traffic_reduction = thorium_multiplexed_buffer_get_traffic_reduction(multiplexed_buffer);
#endif

        /*
        * The oldest item is too recent.
        * Therefore, all the others are too recent too
        * because the timeline is ordered.
        */
        if (timeout != 0
                    && duration < timeout
                    && message_count < self->degree_of_aggregation_limit
#ifdef CHECK_PREDICTED_TRAFFIC_REDUCTION
                   traffic_reduction < acceptable_traffic_reduction
#endif
        ) {
#ifdef THORIUM_MULTIPLEXER_USE_QUEUE
            core_queue_enqueue(&self->timeline, &index);
#endif
            return;
        }

#ifdef CHECK_OUTBOUND_THROUGHPUT
        /*
        * Don't flush now since the transport layer has already reached its maximum
        * throughput.
        */
        if (thorium_worker_has_reached_maximum_outbound_throughput(self->worker)
                      && traffic_reduction < acceptable_traffic_reduction) {
#ifdef THORIUM_MULTIPLEXER_USE_QUEUE
            core_queue_enqueue(&self->timeline, &index);
#endif
            return;
        }
#endif

    /*
     * Remove the object from the timeline.
     */

#ifdef THORIUM_MULTIPLEXER_USE_TREE
        core_red_black_tree_delete(&self->timeline, lowest_key);
#elif defined(THORIUM_MULTIPLEXER_USE_HEAP)
        core_binary_heap_delete_root(&self->timeline);
#endif

#ifdef VERIFY_TARGET
        /*
        * Verify if the buffer can grow more.
        */
        if (!thorium_multiplexed_buffer_has_reached_target(multiplexed_buffer)) {
            thorium_multiplexed_buffer_set_time(multiplexed_buffer, time);
    
            core_red_black_tree_add_key_and_value(&self->timeline, &time, &index);
    
            return;
        }
#endif

        /*
        * The item won't have content in the case were _flush()
        * was called elsewhere with FORCE_YES_SIZE.
        */
#ifdef THORIUM_MULTIPLEXER_TRACK_BUFFERS_WITH_CONTENT
        if (core_set_find(&self->buffers_with_content, &index)) {
#endif
            thorium_message_multiplexer_flush(self, index, FORCE_YES_TIME);

#ifdef THORIUM_MULTIPLEXER_TRACK_BUFFERS_WITH_CONTENT
        }
#endif

        /*
         * Otherwise, keep flushing stuff.
         * This will eventually end anyway.
         */
    }
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
    /*
    int elapsed;
    int message_count;
    */

    if (CORE_BITMAP_GET_FLAG(self->flags, FLAG_DISABLED)) {
        return;
    }

#ifdef THORIUM_MULTIPLEXER_TRACK_BUFFERS_WITH_CONTENT
#ifdef CORE_DEBUGGER_ASSERT_ENABLED
    if (!(core_set_find(&self->buffers_with_content, &index))) {
        multiplexed_buffer = core_vector_at(&self->buffers, index);
        printf("index %d has no content\n", index);

        thorium_multiplexed_buffer_print(multiplexed_buffer);
    }
#endif

    CORE_DEBUGGER_ASSERT(core_set_find(&self->buffers_with_content, &index));
#endif

    multiplexed_buffer = core_vector_at(&self->buffers, index);
    current_size = thorium_multiplexed_buffer_current_size(multiplexed_buffer);
    maximum_size = thorium_multiplexed_buffer_maximum_size(multiplexed_buffer);

    /*
     * The buffer was still in the timeline, but it was flushed elsewhere.
     */
    if (current_size == 0) {
        return;

        /*
    if (force == FORCE_NO && current_size < maximum_size) {
        return;

    } else if (force == FORCE_YES_TIME) {
        elapsed = core_timer_get_nanoseconds(&self->timer) - multiplexed_buffer->timestamp_;

        if (elapsed < self->timeout_in_nanoseconds) {
            return;
        }
    } else if (force == FORCE_YES_DOA) {
        message_count = multiplexed_buffer->message_count_;

        if (message_count < self->degree_of_aggregation_limit) {
            return;
        }
        */
    }

#ifdef CORE_DEBUGGER_ASSERT_ENABLED
    if (current_size <= 0)
        printf("current_size %d maximum_size %d\n", current_size, maximum_size);
#endif

    CORE_DEBUGGER_ASSERT(current_size > 0);

    buffer = thorium_multiplexed_buffer_buffer(multiplexed_buffer);

    count = current_size + THORIUM_MESSAGE_METADATA_SIZE;

    tag = ACTION_MULTIPLEXER_MESSAGE;

    /*
     * This count does not include metadata for the final big message.
     *
     * Avoid this copy by using an array of pointers in the first place.
     */

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
    printf("DEBUG_MULTIPLEXER thorium_message_multiplexer_flush index %d buffer %p force %d current_size %d maximum_size %d"
                    " destination_node %d\n",
                    index, buffer, force,
                    current_size, maximum_size,
                    thorium_message_destination_node(&message));

    printf("message in flush\n");
    thorium_multiplexed_buffer_print(multiplexed_buffer);
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

    thorium_multiplexed_buffer_reset(multiplexed_buffer);

#ifdef THORIUM_MULTIPLEXER_TRACK_BUFFERS_WITH_CONTENT
    core_set_delete(&self->buffers_with_content, &index);
#endif
}

int thorium_message_multiplexer_is_disabled(struct thorium_message_multiplexer *self)
{
    return CORE_BITMAP_GET_FLAG(self->flags, FLAG_DISABLED);
}

void thorium_message_multiplexer_set_worker(struct thorium_message_multiplexer *self,
                struct thorium_worker *worker)
{
    struct core_memory_pool *pool;

    self->worker = worker;
    pool = thorium_worker_get_memory_pool(self->worker,
                            MEMORY_POOL_NAME_WORKER_PERSISTENT);

#ifdef THORIUM_MULTIPLEXER_USE_TREE
    core_red_black_tree_init(&self->timeline, sizeof(uint64_t), sizeof(int),
                    pool);
    core_red_black_tree_use_uint64_t_keys(&self->timeline);

#elif defined(THORIUM_MULTIPLEXER_USE_HEAP)
    core_binary_heap_init(&self->timeline, sizeof(uint64_t), sizeof(int),
                    CORE_BINARY_HEAP_MIN | CORE_BINARY_HEAP_UINT64_T_KEYS);
    core_binary_heap_set_memory_pool(&self->timeline, pool);

#elif defined(THORIUM_MULTIPLEXER_USE_QUEUE)

    core_queue_init(&self->timeline, sizeof(int));

#endif

    if (thorium_node_name(self->node) == 0 && thorium_worker_name(self->worker) == 0
                    && thorium_node_must_print_data(self->node)) {
        printf("[thorium] message_multiplexer: disabled=%d buffer_size_in_bytes=%d timeout_in_nanoseconds=%d\n",
                            CORE_BITMAP_GET_FLAG(self->flags, FLAG_DISABLED),
                        self->buffer_size_in_bytes, self->timeout_in_nanoseconds);
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

void thorium_message_multiplexer_print_traffic_reduction(struct thorium_message_multiplexer *self)
{
    char buffer[1024];
    int position;
    int i;
    int size;
    struct thorium_multiplexed_buffer *multiplexed_buffer;
    int original_message_count;
    int real_message_count;
    float reduction;

    position = 0;

    position += sprintf(buffer + position, "[thorium] node %d worker %d multiplexer channels",
                    thorium_node_name(self->node), thorium_worker_name(self->worker));

    size = core_vector_size(&self->buffers);
    for (i = 0; i < size; ++i) {
        multiplexed_buffer = core_vector_at(&self->buffers, i);

        original_message_count = thorium_multiplexed_buffer_original_message_count(multiplexed_buffer);
        real_message_count = thorium_multiplexed_buffer_real_message_count(multiplexed_buffer);

        if (original_message_count == 0)
            continue;

        reduction = (0.0 + original_message_count - real_message_count) / original_message_count;
        reduction *= 100.0;

        position += sprintf(buffer + position, " [%d: %d %d %.2f%%]",
                        i, original_message_count, real_message_count,
                        reduction);
    }

    position += sprintf(buffer + position, "\n");

    printf("%s", buffer);
}

int thorium_message_multiplexer_get_original_message_count(struct thorium_message_multiplexer *self)
{
    return self->original_message_count;
}

int thorium_message_multiplexer_get_real_message_count(struct thorium_message_multiplexer *self)
{
    return self->real_message_count;
}

void thorium_message_multiplexer_update_timeout(struct thorium_message_multiplexer *self)
{
    time_t now;
    uint64_t elapsed;
    int period;
    uint64_t throughput;
    int timeout;
    uint64_t now_nanoseconds;
    uint64_t event_count;
    int new_timeout;

#ifdef MULTIPLEXER_IS_VERBOSE
    int print = 0;
    if (thorium_node_must_print_data(self->worker->node)
        && self->worker->node->name == 0 && self->worker->name == 1)
        print = 1;
#endif

    period = 1;
    now = time(NULL);

    elapsed = now - self->last_update_time;

    if ((int)elapsed < period)
        return;

    timeout = self->timeout_in_nanoseconds;

    CORE_DEBUGGER_ASSERT(timeout >= 0);
    now_nanoseconds = core_timer_get_nanoseconds(&self->timer);

    elapsed = now_nanoseconds - self->last_time;
    event_count = thorium_worker_get_event_counter(self->worker,
                    THORIUM_EVENT_ACTOR_SEND);

    throughput = event_count;
    throughput -= self->last_send_event_count;
    throughput *= (1000 * 1000 * 1000);
    throughput /= elapsed;

#ifdef MULTIPLEXER_IS_VERBOSE
    if (print)
        printf("event_count %" PRIu64 " last %" PRIu64 " elapsed %" PRIu64 " = %" PRIu64 " MPS\n",
                    event_count, self->last_send_event_count, elapsed, throughput);
#endif

    thorium_decision_maker_add_data_point(&self->decision_maker, timeout,
                    (int)throughput);

#ifdef MULTIPLEXER_IS_VERBOSE
    if (print)
        thorium_decision_maker_print(&self->decision_maker, timeout);
#endif

    new_timeout = thorium_decision_maker_get_best_timeout(&self->decision_maker,
                    self->timeout_in_nanoseconds);

    if (new_timeout != THORIUM_TIMEOUT_NO_VALUE) {
#ifdef MULTIPLEXER_IS_VERBOSE
        if (print)
            printf("TIMEOUT -> old value %d new value %d\n",
                            self->timeout_in_nanoseconds,
                            new_timeout);
#endif

        self->timeout_in_nanoseconds = new_timeout;
    }

    self->last_update_time = now;
    self->last_time = now_nanoseconds;
    self->last_send_event_count = event_count;
}
