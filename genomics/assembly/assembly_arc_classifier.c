
#include "assembly_arc_classifier.h"

#include "assembly_arc_kernel.h"
#include "assembly_arc_block.h"

#include <genomics/kernels/dna_kmer_counter_kernel.h>

#include <core/system/memory_cache.h>

#include <core/system/debugger.h>

#include <biosal.h>

#include <stdio.h>

#define MEMORY_NAME_ARC_CLASSIFIER 0x1d4f2792

#define ARC_CLASSIFIER_CACHE 0x93dd7074
#define ARC_CLASSIFIER_CACHE_CHUNK_SIZE 8388608

void biosal_assembly_arc_classifier_init(struct thorium_actor *self);
void biosal_assembly_arc_classifier_destroy(struct thorium_actor *self);
void biosal_assembly_arc_classifier_receive(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_arc_classifier_set_kmer_length(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_arc_classifier_push_arc_block(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_arc_classifier_verify_counters(struct thorium_actor *self);

void biosal_assembly_arc_classifier_set_consumers(struct thorium_actor *self,
                struct thorium_message *message);

void biosal_assembly_arc_classifier_flush_all(struct thorium_actor *self,
                struct thorium_message *message, int force);

struct thorium_script biosal_assembly_arc_classifier_script = {
    .identifier = SCRIPT_ASSEMBLY_ARC_CLASSIFIER,
    .name = "biosal_assembly_arc_classifier",
    .init = biosal_assembly_arc_classifier_init,
    .destroy = biosal_assembly_arc_classifier_destroy,
    .receive = biosal_assembly_arc_classifier_receive,
    .size = sizeof(struct biosal_assembly_arc_classifier),
    .author = "Sebastien Boisvert",
    .description = "Arc classifier for metagenomics"
};

void biosal_assembly_arc_classifier_init(struct thorium_actor *self)
{
    struct biosal_assembly_arc_classifier *concrete_self;

    concrete_self = (struct biosal_assembly_arc_classifier *)thorium_actor_concrete_actor(self);

    core_memory_pool_init(&concrete_self->persistent_memory, 16 * 1048576, MEMORY_NAME_ARC_CLASSIFIER);
    /*
    core_memory_pool_enable_tracepoints(&concrete_self->persistent_memory);
    */

    concrete_self->kmer_length = -1;

    thorium_actor_add_action(self, ACTION_ASK_TO_STOP,
                    thorium_actor_ask_to_stop);

    thorium_actor_add_action(self, ACTION_SET_KMER_LENGTH,
                    biosal_assembly_arc_classifier_set_kmer_length);

    /*
     *
     * Configure the codec.
     */

    biosal_dna_codec_init(&concrete_self->codec);

    if (biosal_dna_codec_must_use_two_bit_encoding(&concrete_self->codec,
                            thorium_actor_get_node_count(self))) {
        biosal_dna_codec_enable_two_bit_encoding(&concrete_self->codec);
    }

    core_vector_init(&concrete_self->consumers, sizeof(int));

    thorium_actor_add_action(self, ACTION_ASSEMBLY_PUSH_ARC_BLOCK,
                    biosal_assembly_arc_classifier_push_arc_block);

    concrete_self->received_blocks = 0;

    core_vector_init(&concrete_self->pending_requests, sizeof(int));
    concrete_self->active_requests = 0;

    concrete_self->producer_is_waiting = 0;

    /*
     * The number of active messages provided by
     * thorium_actor_active_message_limit is typically 1.
     *
     * Here, adding arcs in the graph basically just turn on some bits.
     * Because of that, we need to allow more in-flight active messages,
     * or we need to use larger messages.
     * In general, using larger messages is a better idea than having
     * more in-flight messages because the same amount of work is produced
     * in both case, but using messages that are 2 times larger has less
     * overhead than using 2 times more in-flight messages.
     */
    concrete_self->maximum_pending_request_count = thorium_actor_active_message_limit(self);
    /*
    */

    /*
     * Add some bonus points to keep the pipeline busy.
     *
     * This is a total of 4.
     */
    /*
    concrete_self->maximum_pending_request_count += 3;
    */

    concrete_self->consumer_count_with_maximum = 0;

    thorium_actor_log(self, "%s/%d is now active, ACTIVE_MESSAGE_LIMIT %d\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    concrete_self->maximum_pending_request_count);
/*
    concrete_self->maximum_pending_requests = 0;
    */

    core_memory_cache_init(&concrete_self->memory_cache, ARC_CLASSIFIER_CACHE, 0, 0);
}

void biosal_assembly_arc_classifier_destroy(struct thorium_actor *self)
{
    struct biosal_assembly_arc_classifier *concrete_self;
    int i;
    int consumer_count;
    struct biosal_assembly_arc_block *output_block;

    concrete_self = (struct biosal_assembly_arc_classifier *)thorium_actor_concrete_actor(self);

    consumer_count = core_vector_size(&concrete_self->consumers);

    for (i = 0; i < consumer_count; i++) {

        output_block = core_vector_at(&concrete_self->output_blocks, i);
        biosal_assembly_arc_block_destroy(output_block);
    }

    core_vector_destroy(&concrete_self->output_blocks);

    concrete_self->kmer_length = -1;

    core_vector_destroy(&concrete_self->consumers);

    biosal_dna_codec_destroy(&concrete_self->codec);

    core_vector_destroy(&concrete_self->pending_requests);

    core_vector_destroy(&concrete_self->output_blocks);

    /*
     * Make sure that there is no memory leak.
     */

#ifdef SHOW_MEMORY_POOL_STATUS
    core_memory_pool_examine(&concrete_self->persistent_memory);
#endif

    core_memory_pool_destroy(&concrete_self->persistent_memory);

#if 0
    thorium_actor_log(self, "ISSUE-819 biosal_assembly_arc_classifier dies\n");
#endif

    core_memory_cache_destroy(&concrete_self->memory_cache);
}

void biosal_assembly_arc_classifier_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    /*
    void *buffer;
    */
    struct biosal_assembly_arc_classifier *concrete_self;
    int *bucket;
    int source;
    int source_index;

    if (thorium_actor_take_action(self, message)) {
        return;
    }

    concrete_self = (struct biosal_assembly_arc_classifier *)thorium_actor_concrete_actor(self);
    tag = thorium_message_action(message);
    /*
    buffer = thorium_message_buffer(message);
    */
    source = thorium_message_source(message);

    if (tag == ACTION_SET_CONSUMERS) {

        biosal_assembly_arc_classifier_set_consumers(self, message);

    } else if (tag == ACTION_ASSEMBLY_PUSH_ARC_BLOCK_REPLY){

        /*
         * Decrease counter now.
         */
        source_index = core_vector_index_of(&concrete_self->consumers, &source);
        bucket = core_vector_at(&concrete_self->pending_requests, source_index);
        /*
        thorium_actor_log(self, "active_requests %d\n", concrete_self->active_requests);
        */

        /*
         * The previous value was maximum_pending_request_count + 1
         */
        if (*bucket == concrete_self->maximum_pending_request_count) {

            --concrete_self->consumer_count_with_maximum;

            /*
             * Find the new maximum pending request count
             */
        }

        --(*bucket);
        --concrete_self->active_requests;

        biosal_assembly_arc_classifier_verify_counters(self);

    } else if (tag == ACTION_AGGREGATOR_FLUSH) {

        biosal_assembly_arc_classifier_flush_all(self, message, 1);

        thorium_actor_send_reply_empty(self, ACTION_AGGREGATOR_FLUSH_REPLY);
    }
}

void biosal_assembly_arc_classifier_set_kmer_length(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_arc_classifier *concrete_self;
    int encoded_length;

    concrete_self = (struct biosal_assembly_arc_classifier *)thorium_actor_concrete_actor(self);

    thorium_message_unpack_int(message, 0, &concrete_self->kmer_length);

    encoded_length = biosal_dna_codec_encoded_length(&concrete_self->codec,
                    concrete_self->kmer_length);

    core_memory_cache_destroy(&concrete_self->memory_cache);
    core_memory_cache_init(&concrete_self->memory_cache, ARC_CLASSIFIER_CACHE, encoded_length,
                    ARC_CLASSIFIER_CACHE_CHUNK_SIZE);

    thorium_actor_send_reply_empty(self, ACTION_SET_KMER_LENGTH_REPLY);
}

void biosal_assembly_arc_classifier_push_arc_block(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_arc_classifier *concrete_self;
    int source;
    struct biosal_assembly_arc_block input_block;
    struct biosal_assembly_arc_block *output_block;
    int consumer_count;
    struct core_vector *input_arcs;
    int size;
    int i;
    struct biosal_assembly_arc *arc;
    void *buffer;
    int count;
    struct biosal_dna_kmer *kmer;
    int consumer_index;
    struct core_memory_pool *ephemeral_memory;

    count = thorium_message_count(message);
    buffer = thorium_message_buffer(message);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    if (count == 0) {
        thorium_actor_log(self, "Error, count is 0 (classifier_push_arc_block)\n");
        return;
    }

    concrete_self = (struct biosal_assembly_arc_classifier *)thorium_actor_concrete_actor(self);
    source = thorium_message_source(message);
    consumer_count = core_vector_size(&concrete_self->consumers);

    CORE_DEBUGGER_LEAK_DETECTION_BEGIN(ephemeral_memory, classify_arcs);

    biosal_assembly_arc_block_init(&input_block, ephemeral_memory, concrete_self->kmer_length,
                    &concrete_self->codec);

#ifdef BIOSAL_ASSEMBLY_ARC_CLASSIFIER_DEBUG
    thorium_actor_log(self, "UNPACKING\n");
#endif

    biosal_assembly_arc_block_unpack(&input_block, buffer, concrete_self->kmer_length,
                    &concrete_self->codec, ephemeral_memory);

#ifdef BIOSAL_ASSEMBLY_ARC_CLASSIFIER_DEBUG
    thorium_actor_log(self, "OK\n");
#endif

    input_arcs = biosal_assembly_arc_block_get_arcs(&input_block);

    /*
     * Classify every arc in the input block
     * and put them in output blocks.
     */

    size = core_vector_size(input_arcs);

#ifdef BIOSAL_ASSEMBLY_ARC_CLASSIFIER_DEBUG
    thorium_actor_log(self, "ClassifyArcs arc_count= %d\n", size);

#endif

    CORE_DEBUGGER_ASSERT(!core_memory_pool_has_double_free(ephemeral_memory));

    for (i = 0; i < size; i++) {

        arc = core_vector_at(input_arcs, i);

        kmer = biosal_assembly_arc_source(arc);

        consumer_index = biosal_dna_kmer_store_index(kmer, consumer_count,
                        concrete_self->kmer_length, &concrete_self->codec,
                        ephemeral_memory);

        output_block = core_vector_at(&concrete_self->output_blocks, consumer_index);

        /*
         * Make a copy of the arc and copy it.
         * It will be freed
         */

        biosal_assembly_arc_block_add_arc_copy(output_block, arc,
                        concrete_self->kmer_length, &concrete_self->codec);
    }

    /*
     * Input arcs are not needed anymore.
     */
    biosal_assembly_arc_block_destroy(&input_block);

    CORE_DEBUGGER_ASSERT(!core_memory_pool_has_double_free(ephemeral_memory));

    /*
     * Finally, send these output blocks to consumers.
     */

    concrete_self->producer_is_waiting = 1;

    biosal_assembly_arc_classifier_flush_all(self, message, 0);

    CORE_DEBUGGER_ASSERT(!core_memory_pool_has_double_free(ephemeral_memory));

    CORE_DEBUGGER_LEAK_CHECK_DOUBLE_FREE(ephemeral_memory);

    /*
     * Check if a response must be sent now.
     */

    ++concrete_self->received_blocks;
    concrete_self->source = source;

#if 0
    /*
     * Only send a direct reply if there is enough memory.
     *
     * As long as maximum_pending_requests is lower than maximum_pending_request_count,
     * there is still space for at least one additional request.
     */
    if (concrete_self->maximum_pending_requests < concrete_self->maximum_pending_request_count) {
            /*&& core_memory_has_enough_bytes()) {*/

        thorium_actor_send_empty(self, concrete_self->source,
                    ACTION_ASSEMBLY_PUSH_ARC_BLOCK_REPLY);
    } else {

        concrete_self->producer_is_waiting = 1;
    }
#endif

    CORE_DEBUGGER_LEAK_DETECTION_END(ephemeral_memory, classify_arcs);

    biosal_assembly_arc_classifier_verify_counters(self);
}

void biosal_assembly_arc_classifier_verify_counters(struct thorium_actor *self)
{
    struct biosal_assembly_arc_classifier *concrete_self;

    concrete_self = (struct biosal_assembly_arc_classifier *)thorium_actor_concrete_actor(self);

    /*
    thorium_actor_log(self, "verify_counters consumer_count_with_maximum %d\n",
                    concrete_self->consumer_count_with_maximum);
                    */

#if 0
    size = core_vector_size(&concrete_self->consumers);
#endif

    /*
     * Don't do anything if the producer is not waiting anyway.
     */
    if (!concrete_self->producer_is_waiting) {
        return;
    }

    if (concrete_self->consumer_count_with_maximum > 0) {
        return;
    }

    /*
     * Make sure that we have enough memory available.
     * This verification is not performed if there are 0 active
     * requests.
     */
    /*
     * The code here is to make sure that there is enough memory.
     */

#if 0
    if (concrete_self->active_requests > 0
            && !core_memory_has_enough_bytes()) {
        return;
    }
#endif

#if 0
    /*
     * Abort if at least one counter is above the threshold.
     */
    for (i = 0; i < size; i++) {
        bucket = core_vector_at(&concrete_self->pending_requests, i);
        active_count = *bucket;

        if (active_count > concrete_self->maximum_pending_request_count) {

            return;
        }
    }
#endif

    /*
     * Trigger an actor event now.
     */
    thorium_actor_send_empty(self, concrete_self->source,
             ACTION_ASSEMBLY_PUSH_ARC_BLOCK_REPLY);

    /*
    thorium_actor_log(self, "send ACTION_ASSEMBLY_PUSH_ARC_BLOCK_REPLY to %d\n",
                    concrete_self->source);
                    */

    concrete_self->producer_is_waiting = 0;
}

void biosal_assembly_arc_classifier_set_consumers(struct thorium_actor *self,
                struct thorium_message *message)
{
    struct biosal_assembly_arc_classifier *concrete_self;
    void *buffer;
    int size;
    int reservation;
    int consumer_count;
    int i;
    int arc_count;
    struct biosal_assembly_arc_block *output_block;

    buffer = thorium_message_buffer(message);

    concrete_self = thorium_actor_concrete_actor(self);
    core_vector_unpack(&concrete_self->consumers, buffer);

    size = core_vector_size(&concrete_self->consumers);
    consumer_count = size;
    core_vector_resize(&concrete_self->pending_requests, size);

    for (i = 0; i < size; i++) {
        core_vector_set_int(&concrete_self->pending_requests, i, 0);
    }

    core_vector_init(&concrete_self->output_blocks, sizeof(struct biosal_assembly_arc_block));
    core_vector_set_memory_pool(&concrete_self->output_blocks,
                    &concrete_self->persistent_memory);

    /*
     * Configure the bin memory reservation.
     */
    arc_count = consumer_count;
    reservation = (arc_count / consumer_count) * 2;

    core_vector_resize(&concrete_self->output_blocks, consumer_count);

    /*
    CORE_DEBUGGER_ASSERT(!core_memory_pool_has_double_free(ephemeral_memory));
    */

    if (concrete_self->kmer_length < 0)
        thorium_actor_log(self, "Error kmer_length not set\n");

    /*
     * Initialize output blocks.
     * There is one for each destination.
     */
    for (i = 0; i < consumer_count; i++) {

        output_block = core_vector_at(&concrete_self->output_blocks, i);

        biosal_assembly_arc_block_init(output_block, &concrete_self->persistent_memory,
                        concrete_self->kmer_length, &concrete_self->codec);

        biosal_assembly_arc_block_reserve(output_block, reservation);
    }

    thorium_actor_send_reply_empty(self, ACTION_SET_CONSUMERS_REPLY);
}

void biosal_assembly_arc_classifier_flush_all(struct thorium_actor *self,
                struct thorium_message *message, int force)
{
    int maximum_buffer_length;
    int new_count;
    void *new_buffer;
    int consumer;
    struct thorium_message new_message;
    int *bucket;
    int arc_count;
    struct core_vector *output_arcs;
    int i;
    struct biosal_assembly_arc_classifier *concrete_self;
    int consumer_count;
    struct core_memory_pool *ephemeral_memory;
    struct biosal_assembly_arc_block *output_block;
    int threshold;

    threshold = thorium_actor_suggested_buffer_size(self);

    /*
     * Use a buffer size of 8 KiB instead of 4 KiB since
     * adding edges is less time-consuming than
     * adding vertices.
     *
     * The idea is that there is a cost for doing an actor
     * context switch (the "gap" between two consecutive receive
     * calls for any given actor.
     */
    /*
    threshold *= 2;
    */

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    concrete_self = thorium_actor_concrete_actor(self);

    consumer_count = core_vector_size(&concrete_self->output_blocks);
    maximum_buffer_length = 0;

    /*
     * Figure out the maximum buffer length tor
     * messages.
     */
    for (i = 0; i < consumer_count; i++) {

        output_block = core_vector_at(&concrete_self->output_blocks, i);
        new_count = biosal_assembly_arc_block_pack_size(output_block, concrete_self->kmer_length,
                    &concrete_self->codec);

        if (new_count > maximum_buffer_length) {
            maximum_buffer_length = new_count;
        }
    }

#if 0
    thorium_actor_log(self, "POOL_BALANCE %d\n",
                    core_memory_pool_profile_balance_count(ephemeral_memory));
#endif

    for (i = 0; i < consumer_count; i++) {

        output_block = core_vector_at(&concrete_self->output_blocks, i);
        output_arcs = biosal_assembly_arc_block_get_arcs(output_block);
        arc_count = core_vector_size(output_arcs);

        /*
         * Don't send an empty message.
         */
        if (arc_count > 0) {

            /*
             * Allocation is not required because new_count <= maximum_buffer_length
             */
            new_count = biosal_assembly_arc_block_pack_size(output_block, concrete_self->kmer_length,
                    &concrete_self->codec);

            if (!force && new_count < threshold)
                continue;

            new_buffer = thorium_actor_allocate(self, new_count);

            CORE_DEBUGGER_ASSERT(new_count <= maximum_buffer_length);

            biosal_assembly_arc_block_pack(output_block, new_buffer, concrete_self->kmer_length,
                    &concrete_self->codec);

            thorium_message_init(&new_message, ACTION_ASSEMBLY_PUSH_ARC_BLOCK,
                    new_count, new_buffer);

            /*
            thorium_actor_log(self, "ACTION_ASSEMBLY_PUSH_ARC_BLOCK...\n");
*/
            consumer = core_vector_at_as_int(&concrete_self->consumers, i);

            /*
             * Send the message.
             */
            thorium_actor_send(self, consumer, &new_message);
            thorium_message_destroy(&new_message);

            /* update event counters for control.
             */
            bucket = core_vector_at(&concrete_self->pending_requests, i);
            ++(*bucket);
            ++concrete_self->active_requests;

            /*
            if (*bucket > concrete_self->maximum_pending_requests) {
                concrete_self->maximum_pending_requests = *bucket;
            }
*/
            if (*bucket == concrete_self->maximum_pending_request_count) {
                ++concrete_self->consumer_count_with_maximum;
            }

#if 0
            thorium_actor_log(self, "Before biosal_assembly_arc_block_clear\n");
            core_memory_pool_examine(&concrete_self->persistent_memory);
#endif

            /*
             * Destroy output block.
             */
            biosal_assembly_arc_block_clear(output_block);

#if 0
            thorium_actor_log(self, "After biosal_assembly_arc_block_clear\n");
            core_memory_pool_examine(&concrete_self->persistent_memory);
#endif
        }

        CORE_DEBUGGER_ASSERT(!core_memory_pool_has_double_free(ephemeral_memory));

#if 0
        thorium_actor_log(self, "i = %d\n", i);
#endif

        CORE_DEBUGGER_LEAK_CHECK_DOUBLE_FREE(ephemeral_memory);
        CORE_DEBUGGER_ASSERT(!core_memory_pool_has_double_free(ephemeral_memory));
    }
}
