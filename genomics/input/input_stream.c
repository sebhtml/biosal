
#include "input_stream.h"

#include "input_command.h"
#include "mega_block.h"

#include <genomics/data/dna_sequence.h>
#include <genomics/storage/sequence_store.h>

#include <core/file_storage/file.h>

#include <core/helpers/message_helper.h>

#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h>

/*
#define BSAL_INPUT_STREAM_DEBUG
*/

/*
 * Enable fast parallel count.
 */
#define ENABLE_PARALLEL_COUNT

struct thorium_script bsal_input_stream_script = {
    .identifier = ACTION_INPUT_SCRIPT_STREAM,
    .init = bsal_input_stream_init,
    .destroy = bsal_input_stream_destroy,
    .receive = bsal_input_stream_receive,
    .size = sizeof(struct bsal_input_stream),
    .name = "bsal_input_stream"
};

void bsal_input_stream_init(struct thorium_actor *actor)
{
    struct bsal_input_stream *input;
    struct bsal_input_stream *concrete_self;

    input = (struct bsal_input_stream *)thorium_actor_concrete_actor(actor);
    concrete_self = input;
    concrete_self->proxy_ready = 0;
    concrete_self->buffer_for_sequence = NULL;
    concrete_self->maximum_sequence_length = 0;
    concrete_self->open = 0;
    concrete_self->error = BSAL_INPUT_ERROR_NO_ERROR;

    concrete_self->file_name = NULL;

    bsal_dna_codec_init(&concrete_self->codec);

    if (bsal_dna_codec_must_use_two_bit_encoding(&concrete_self->codec,
                            thorium_actor_get_node_count(actor))) {
        bsal_dna_codec_enable_two_bit_encoding(&concrete_self->codec);
    }

    /*concrete_self->mega_block_size = 2097152*/

    /*
     * This is the mega block size in number of sequences.
     */
    concrete_self->mega_block_size = 2097152;
    concrete_self->granularity = 1024;

    concrete_self->last_offset = 0;
    concrete_self->last_entries = 0;

    concrete_self->starting_offset = 0;

    /*
     * Use a large ending offset.
     */
    concrete_self->ending_offset = 0;
    --concrete_self->ending_offset;

    bsal_vector_init(&concrete_self->mega_blocks, sizeof(struct bsal_mega_block));

    thorium_actor_add_action(actor, ACTION_INPUT_STREAM_SET_START_OFFSET, bsal_input_stream_set_start_offset);
    thorium_actor_add_action(actor, ACTION_INPUT_STREAM_SET_END_OFFSET, bsal_input_stream_set_end_offset);

#ifdef ENABLE_PARALLEL_COUNT
    thorium_actor_add_action(actor, ACTION_INPUT_COUNT_IN_PARALLEL, bsal_input_stream_count_in_parallel);
    thorium_actor_add_action(actor, ACTION_INPUT_COUNT_REPLY, bsal_input_stream_count_reply);

#else
    thorium_actor_add_action(actor, ACTION_INPUT_COUNT_IN_PARALLEL, bsal_input_stream_count_in_parallel_mock);
    thorium_actor_add_action(actor, ACTION_INPUT_COUNT_REPLY, bsal_input_stream_count_reply_mock);
#endif

    concrete_self->count_customer = THORIUM_ACTOR_NOBODY;

#if 0
    bsal_string_init(&concrete_self->file_for_parallel_counting, NULL);
#endif

    /*
     * Parallel counting.
     */

    concrete_self->total_entries = 0;

    /*
     * Disable parallel counting.
     */

    concrete_self->finished_parallel_stream_count = 0;

    bsal_vector_init(&concrete_self->spawners, sizeof(int));
    bsal_vector_init(&concrete_self->parallel_streams, sizeof(int));
    bsal_vector_init(&concrete_self->start_offsets, sizeof(uint64_t));
    bsal_vector_init(&concrete_self->end_offsets, sizeof(uint64_t));

    bsal_vector_init(&concrete_self->parallel_mega_blocks, sizeof(struct bsal_vector));

    printf("%s/%d is now online on node %d\n",
                    thorium_actor_script_name(actor),
                    thorium_actor_name(actor),
                    thorium_actor_node_name(actor));
}

void bsal_input_stream_destroy(struct thorium_actor *actor)
{
    struct bsal_input_stream *concrete_self;

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(actor);

#if 0
    bsal_string_destroy(&concrete_selffile_for_parallel_counting);
#endif

    if (concrete_self->buffer_for_sequence != NULL) {
        bsal_memory_free(concrete_self->buffer_for_sequence);
        concrete_self->buffer_for_sequence = NULL;
        concrete_self->maximum_sequence_length = 0;
    }

    concrete_self->open = 0;

    if (concrete_self->proxy_ready) {
        bsal_input_proxy_destroy(&concrete_self->proxy);
        concrete_self->proxy_ready = 0;
    }

    if (concrete_self->file_name != NULL) {
        bsal_memory_free(concrete_self->file_name);
        concrete_self->file_name = NULL;
    }

    concrete_self->file_name = NULL;
    bsal_dna_codec_destroy(&concrete_self->codec);

    bsal_vector_destroy(&concrete_self->mega_blocks);

    concrete_self->total_entries = 0;
    concrete_self->finished_parallel_stream_count = 0;

    bsal_vector_destroy(&concrete_self->spawners);
    bsal_vector_destroy(&concrete_self->parallel_streams);
    bsal_vector_destroy(&concrete_self->start_offsets);
    bsal_vector_destroy(&concrete_self->end_offsets);
    bsal_vector_destroy(&concrete_self->parallel_mega_blocks);
}

void bsal_input_stream_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int source;
    uint64_t count;
    struct bsal_input_stream *concrete_self;
    int i;
    int has_sequence;
    int sequences;
    int sequence_index;
    int buffer_size;
    char *buffer;
    char *read_buffer;
    struct bsal_mega_block mega_block;
    char *file_name_in_buffer;

    if (thorium_actor_take_action(actor, message)) {
        return;
    }

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_tag(message);
    source = thorium_message_source(message);
    buffer = (char *)thorium_message_buffer(message);

    /* Do nothing if there is an error.
     * has_error returns the error to the source.
     */
    /*
    if (bsal_input_stream_has_error(actor, message)) {
        return;
    }

    */

    if (tag == ACTION_INPUT_OPEN) {

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("DEBUG ACTION_INPUT_OPEN\n");
#endif

        if (concrete_self->open) {

            concrete_self->error = BSAL_INPUT_ERROR_ALREADY_OPEN;
            thorium_actor_send_reply_int(actor, ACTION_INPUT_OPEN_REPLY, concrete_self->error);
            thorium_actor_send_to_self_empty(actor, ACTION_ASK_TO_STOP);

            return;
        }

        concrete_self->open = 1;

        /* TODO: find out the maximum read length in some way */
        concrete_self->maximum_sequence_length = BSAL_INPUT_MAXIMUM_SEQUENCE_LENGTH;

        concrete_self->buffer_for_sequence = (char *)bsal_memory_allocate(concrete_self->maximum_sequence_length);

        /*bsal_input_stream_init(actor);*/

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("DEBUG bsal_input_stream_receive open %s\n",
                        buffer);
#endif

        /*memcpy(&concrete_self->file_index, buffer, sizeof(concrete_self->file_index));*/
        file_name_in_buffer = buffer;

        printf("stream/%d (node/%d) opens file %s offset %" PRIu64 "\n", thorium_actor_name(actor),
                        thorium_actor_node_name(actor), file_name_in_buffer,
                        concrete_self->starting_offset);

#ifdef DEBUG_ISSUE_594
        thorium_message_print(message);

        printf("Buffer %s\n", buffer);
#endif

        concrete_self->file_name = bsal_memory_allocate(strlen(file_name_in_buffer) + 1);
        strcpy(concrete_self->file_name, file_name_in_buffer);

        bsal_input_proxy_init(&concrete_self->proxy, concrete_self->file_name,
                        concrete_self->starting_offset, concrete_self->ending_offset);

        concrete_self->proxy_ready = 1;

        /* Die if there is an error...
         */
        if (bsal_input_stream_has_error(actor, message)) {

#ifdef BSAL_INPUT_STREAM_DEBUG
            printf("DEBUG has error\n");
#endif

            thorium_actor_send_reply_int(actor, ACTION_INPUT_OPEN_REPLY, concrete_self->error);
            thorium_actor_send_to_self_empty(actor, ACTION_ASK_TO_STOP);

            return;
        }

        concrete_self->controller = source;

        /* no error here... */
        thorium_actor_send_reply_int(actor, ACTION_INPUT_OPEN_REPLY, concrete_self->error);

    } else if (tag == ACTION_INPUT_COUNT) {
        /* count a little bit and yield the worker */

        if (concrete_self->count_customer == THORIUM_ACTOR_NOBODY) {
            concrete_self->count_customer = source;
        }

        if (bsal_input_stream_check_open_error(actor, message)) {

            thorium_actor_send_reply_int64_t(actor, ACTION_INPUT_COUNT_REPLY, concrete_self->error);
            thorium_actor_send_to_self_empty(actor, ACTION_ASK_TO_STOP);

            return;
        }

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("DEBUG ACTION_INPUT_COUNT received...\n");
#endif

        i = 0;
        /* continue counting ... */
        has_sequence = 1;

        while (i < concrete_self->granularity && has_sequence) {
            has_sequence = bsal_input_proxy_get_sequence(&concrete_self->proxy,
                            concrete_self->buffer_for_sequence);

#if 0
            printf("Sequence= %s\n", concrete_self->buffer_for_sequence);
#endif
            i++;
        }

        sequences = bsal_input_proxy_size(&concrete_self->proxy);

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("DEBUG ACTION_INPUT_COUNT sequences %d...\n", sequences);
#endif

        if (!has_sequence || sequences % concrete_self->mega_block_size == 0) {

            bsal_mega_block_init(&mega_block, -1, concrete_self->last_offset,
                            sequences - concrete_self->last_entries, sequences);

            bsal_vector_push_back(&concrete_self->mega_blocks, &mega_block);

            concrete_self->last_entries = sequences;
            concrete_self->last_offset = bsal_input_proxy_offset(&concrete_self->proxy);

            thorium_actor_send_int64_t(actor, concrete_self->controller, ACTION_INPUT_COUNT_PROGRESS, sequences);
        }

        if (has_sequence) {

            /*printf("DEBUG yield\n");*/

            thorium_actor_send_to_self_empty(actor, ACTION_YIELD);

            /* notify the controller of our progress...
             */

        } else {

            thorium_actor_send_to_self_empty(actor, ACTION_INPUT_COUNT_READY);
        }

    } else if (tag == ACTION_YIELD_REPLY) {

        if (bsal_input_stream_check_open_error(actor, message)) {
                /*
                 * it is not clear that there can be an error when receiving YIELD.
            error = concrete_self->error;
            thorium_actor_send_reply_int(actor, ACTION_INPUT_COUNT_REPLY, error);
            thorium_actor_send_to_self(actor, ACTION_ASK_TO_STOP);

                 */
            return;
        }

        thorium_actor_send_to_self_empty(actor, ACTION_INPUT_COUNT);

    } else if (tag == ACTION_INPUT_COUNT_READY) {

        if (bsal_input_stream_check_open_error(actor, message)) {
            return;
        }

        count = bsal_input_proxy_size(&concrete_self->proxy);

        thorium_actor_send_vector(actor, concrete_self->count_customer, ACTION_INPUT_COUNT_REPLY,
                        &concrete_self->mega_blocks);

        printf("input_stream/%d on node/%d counted entries in %s, %" PRIu64 "\n",
                        thorium_actor_name(actor), thorium_actor_node_name(actor),
                        concrete_self->file_name, count);

    } else if (tag == ACTION_INPUT_CLOSE) {

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("DEBUG destroy proxy\n");
#endif
        concrete_self->error = BSAL_INPUT_ERROR_NO_ERROR;

        if (bsal_input_stream_check_open_error(actor, message)) {
            concrete_self->error = BSAL_INPUT_ERROR_FILE_NOT_OPEN;

            thorium_message_init(message, ACTION_INPUT_CLOSE_REPLY, sizeof(concrete_self->error),
                &concrete_self->error);
            thorium_actor_send(actor, source, message);

            thorium_actor_send_to_self_empty(actor, ACTION_ASK_TO_STOP);
            return;
        }

        thorium_message_init(message, ACTION_INPUT_CLOSE_REPLY, sizeof(concrete_self->error),
                &concrete_self->error);
        thorium_actor_send(actor, source, message);

        thorium_actor_send_to_self_empty(actor, ACTION_ASK_TO_STOP);

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("actor %d sending ACTION_INPUT_CLOSE_REPLY to %d\n",
                        thorium_actor_name(actor), source);
#endif

    } else if (tag == ACTION_INPUT_GET_SEQUENCE) {

        if (bsal_input_stream_check_open_error(actor, message)) {

            /* the error management could be better. */
            concrete_self->error = BSAL_INPUT_ERROR_FILE_NOT_OPEN;
            thorium_message_init(message, ACTION_INPUT_GET_SEQUENCE_REPLY, sizeof(concrete_self->error),
                            &concrete_self->error);
            thorium_actor_send(actor, source, message);

            return;
        }

        sequence_index = bsal_input_proxy_size(&concrete_self->proxy);

        /* TODO it would be clearer to use a struct to pack a int and a char []
         * then to use the code below.
         */
        read_buffer = concrete_self->buffer_for_sequence + sizeof(sequence_index);
        has_sequence = bsal_input_proxy_get_sequence(&concrete_self->proxy,
                            read_buffer);

        if (!has_sequence) {
            thorium_message_init(message, ACTION_INPUT_GET_SEQUENCE_END, 0, NULL);
            thorium_actor_send(actor, source, message);

            return;
        }

        buffer_size = sizeof(sequence_index) + strlen(read_buffer) + 1;

        memcpy(concrete_self->buffer_for_sequence, &sequence_index, sizeof(sequence_index));

        thorium_message_init(message, ACTION_INPUT_GET_SEQUENCE_REPLY,
                        buffer_size, concrete_self->buffer_for_sequence);
        thorium_actor_send(actor, source, message);

    } else if (tag == ACTION_INPUT_PUSH_SEQUENCES) {

        bsal_input_stream_push_sequences(actor, message);

    } else if (tag == ACTION_ASK_TO_STOP) {

        thorium_actor_send_to_self_empty(actor, ACTION_STOP);

        thorium_actor_send_range_empty(actor, &concrete_self->parallel_streams,
                        ACTION_ASK_TO_STOP);

        thorium_actor_send_reply_empty(actor, ACTION_ASK_TO_STOP_REPLY);

    } else if (tag == ACTION_INPUT_STREAM_RESET) {

        /* fail silently
         */
        if (!concrete_self->open) {
            thorium_actor_send_reply_empty(actor, ACTION_INPUT_STREAM_RESET_REPLY);
            return;
        }

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("DEBUG ACTION_INPUT_STREAM_RESET\n");
#endif

        bsal_input_proxy_destroy(&concrete_self->proxy);
        bsal_input_proxy_init(&concrete_self->proxy, concrete_self->file_name,
                        concrete_self->starting_offset,
                        concrete_self->ending_offset);

        thorium_actor_send_reply_empty(actor, ACTION_INPUT_STREAM_RESET_REPLY);

    } else if (tag == ACTION_PUSH_SEQUENCE_DATA_BLOCK_REPLY) {

        thorium_actor_send_to_supervisor_int(actor, ACTION_INPUT_PUSH_SEQUENCES_REPLY,
                        source);
    }
}

int bsal_input_stream_has_error(struct thorium_actor *actor,
                struct thorium_message *message)
{
    struct bsal_input_stream *concrete_self;

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(actor);

    if (!concrete_self->proxy_ready) {
        return 0;
    }

    concrete_self->error = bsal_input_proxy_error(&concrete_self->proxy);

    if (concrete_self->error == BSAL_INPUT_ERROR_FILE_NOT_FOUND) {

        return 1;

    } else if (concrete_self->error == BSAL_INPUT_ERROR_FORMAT_NOT_SUPPORTED) {
        return 1;
    }

    return 0;
}

int bsal_input_stream_check_open_error(struct thorium_actor *actor,
                struct thorium_message *message)
{
    struct bsal_input_stream *concrete_self;

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(actor);

    if (!concrete_self->open) {

        concrete_self->error = BSAL_INPUT_ERROR_FILE_NOT_OPEN;
        return 1;
    }

    return 0;
}

void bsal_input_stream_push_sequences(struct thorium_actor *actor,
                struct thorium_message *message)
{
    struct bsal_input_command command;
    void *buffer;
    int store_name;
    uint64_t store_first;
    uint64_t store_last;
    int sequences_to_read;
    void *new_buffer;
    int new_count;
    struct thorium_message new_message;
    void *buffer_for_sequence;
    struct bsal_dna_sequence dna_sequence;
    /*struct bsal_vector *command_entries;*/
    int has_sequence;
    int i;
    struct bsal_input_stream *concrete_self;
    struct bsal_memory_pool *ephemeral_memory;

#ifdef BSAL_INPUT_STREAM_DEBUG
    int count;
    int actual_count;
#endif

    /* answer immediately
     */
    thorium_actor_send_reply_empty(actor, ACTION_INPUT_PUSH_SEQUENCES_READY);
    ephemeral_memory = (struct bsal_memory_pool *)thorium_actor_get_ephemeral_memory(actor);
    has_sequence = 1;

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG stream/%d bsal_input_stream_push_sequences entering...\n",
                    thorium_actor_name(actor));
#endif


    buffer = thorium_message_buffer(message);

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(actor);

    bsal_input_command_init_empty(&command);
    bsal_input_command_unpack(&command, buffer, thorium_actor_get_ephemeral_memory(actor),
                    &concrete_self->codec);

#ifdef BSAL_INPUT_STREAM_DEBUG
    count = thorium_message_count(message);
    printf("DEBUG bsal_input_stream_push_sequences after unpack, count %d:\n",
                    count);

    bsal_input_command_print(&command);
#endif

    store_name = bsal_input_command_store_name(&command);
    store_first = bsal_input_command_store_first(&command);
    store_last = bsal_input_command_store_last(&command);

    sequences_to_read = store_last - store_first + 1;

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG bsal_input_stream_push_sequences received ACTION_INPUT_PUSH_SEQUENCES\n");
    printf("DEBUG Command before sending it\n");

    bsal_input_command_print(&command);
#endif

    /*
     * add sequences to the input command
     */

    buffer_for_sequence = concrete_self->buffer_for_sequence;
    /*command_entries = bsal_input_command_entries(&command);*/

    i = 0;
    /* TODO: actually load something
     */
    while (sequences_to_read-- && has_sequence) {
        has_sequence = bsal_input_proxy_get_sequence(&concrete_self->proxy,
                            buffer_for_sequence);

        bsal_dna_sequence_init(&dna_sequence, buffer_for_sequence,
                        &concrete_self->codec, thorium_actor_get_ephemeral_memory(actor));

        bsal_input_command_add_entry(&command, &dna_sequence, &concrete_self->codec,
                        thorium_actor_get_ephemeral_memory(actor));

        bsal_dna_sequence_destroy(&dna_sequence, thorium_actor_get_ephemeral_memory(actor));

        i++;
    }

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG prepared %d sequences for command\n",
                    i);
    bsal_input_command_print(&command);
#endif

    new_count = bsal_input_command_pack_size(&command,
                    &concrete_self->codec);

    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

    bsal_input_command_pack(&command, new_buffer, &concrete_self->codec);

#ifdef BSAL_INPUT_STREAM_DEBUG
    actual_count = bsal_input_command_pack(&command, new_buffer);
    printf("DEBUG123 new_count %d actual_count %d\n", new_count,
                    actual_count);
#endif

    thorium_message_init(&new_message, ACTION_PUSH_SEQUENCE_DATA_BLOCK,
                    new_count, new_buffer);

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG bsal_input_stream_push_sequences sending ACTION_PUSH_SEQUENCE_DATA_BLOCK to %d\n",
                    store_name);
#endif

    thorium_actor_send(actor, store_name, &new_message);

    /*
     * send sequences to store.
     * The required information is the input command
     */
    /*
    thorium_actor_send_reply_empty(actor, ACTION_INPUT_PUSH_SEQUENCES_REPLY);
    */

    /* free memory
     */
    bsal_memory_pool_free(ephemeral_memory, new_buffer);

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG freeing %d entries\n",
                    (int)bsal_vector_size(command_entries));
#endif

    bsal_input_command_destroy(&command, thorium_actor_get_ephemeral_memory(actor));

#if 0
    for (i = 0; i < bsal_vector_size(command_entries); i++) {
        a_sequence = (struct bsal_dna_sequence *)bsal_vector_at(command_entries, i);

        bsal_dna_sequence_destroy(a_sequence, thorium_actor_get_ephemeral_memory(actor));
    }

    bsal_vector_destroy(command_entries);
#endif

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG bsal_input_stream_push_sequences EXIT\n");
#endif

}

void bsal_input_stream_set_start_offset(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_input_stream *concrete_self;

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(self);
    thorium_message_unpack_uint64_t(message, 0, &concrete_self->starting_offset);
    thorium_actor_send_reply_empty(self, ACTION_INPUT_STREAM_SET_START_OFFSET_REPLY);

#ifdef DEBUG_ISSUE_594
    printf("DEBUG %d bsal_input_stream_set_start_offset\n",
                    thorium_actor_name(self));
#endif

    concrete_self->last_offset = concrete_self->starting_offset;
}

void bsal_input_stream_set_end_offset(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_input_stream *concrete_self;

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(self);

#ifdef DEBUG_ISSUE_594
    printf("DEBUG %d bsal_input_stream_set_end_offset\n",
                    thorium_actor_name(self));
#endif

    thorium_message_unpack_uint64_t(message, 0, &concrete_self->ending_offset);
    thorium_actor_send_reply_empty(self, ACTION_INPUT_STREAM_SET_END_OFFSET_REPLY);
}

void bsal_input_stream_count_in_parallel(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_input_stream *concrete_self;
    char *file;
    uint64_t file_size;
    uint64_t start_offset;
    uint64_t end_offset;
    void *buffer;
    int spawner;
    int i;
    int size;
    uint64_t parallel_block_size;

    /*
     * The block size for deciding when to spawn new actors for
     * I/O.
     */
    parallel_block_size = 536870912;

    /*
    parallel_block_size = 0;
    --parallel_block_size;
     */

    /*
     * Approach:
     *
     * 1. Check number of bytes.
     * 2. Determine the number of streams.
     * 3. Spawn streams using a manager.
     * 4. Set the offsets for each stream
     * 5. Set ask them to count
     */
    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);

    bsal_vector_unpack(&concrete_self->spawners, buffer);

    file = concrete_self->file_name;

    file_size = bsal_file_get_size(file);

    printf("COUNT_IN_PARALLEL %s %" PRIu64 "\n",
                    file, file_size);

    start_offset = 0;

    i = 0;
    while (start_offset < file_size) {

        end_offset = start_offset + parallel_block_size - 1;

        if (end_offset > file_size - 1 || end_offset < start_offset) {
            end_offset = file_size - 1;
        }


        bsal_vector_push_back_uint64_t(&concrete_self->start_offsets, start_offset);
        bsal_vector_push_back_uint64_t(&concrete_self->end_offsets, end_offset);

        printf("DEBUG PARALLEL BLOCK %s %i %" PRIu64 " %" PRIu64 "\n",
                        file,
                        i,
                        start_offset,
                        end_offset);

        start_offset = end_offset + 1;
        ++i;
    }

    size = bsal_vector_size(&concrete_self->start_offsets);

    thorium_actor_add_action(self, ACTION_SPAWN_REPLY, bsal_input_stream_spawn_reply);

    printf("DEBUG stream/%d spawns %d streams for counting\n",
                    thorium_actor_name(self),
                    size);

    for (i = 0; i < size; i++) {

        spawner = thorium_actor_get_random_spawner(self, &concrete_self->spawners);

        thorium_actor_send_int(self, spawner, ACTION_SPAWN,
                ACTION_INPUT_SCRIPT_STREAM);
    }
}

void bsal_input_stream_spawn_reply(struct thorium_actor *self, struct thorium_message *message)
{
    int stream;
    struct bsal_input_stream *concrete_self;
    int i;
    int size;
    uint64_t start_offset;
    uint64_t end_offset;

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(self);

    thorium_message_unpack_int(message, 0, &stream);

    bsal_vector_push_back_int(&concrete_self->parallel_streams, stream);

#ifdef DEBUG_ISSUE_594
    printf("DEBUG bsal_input_stream_spawn_reply %d/%d\n",
            (int)bsal_vector_size(&concrete_self->parallel_streams),
            (int)bsal_vector_size(&concrete_self->start_offsets));
#endif

    if (bsal_vector_size(&concrete_self->parallel_streams) ==
                    bsal_vector_size(&concrete_self->start_offsets)) {

        /* Set offsets
         */

        thorium_actor_add_action(self, ACTION_INPUT_STREAM_SET_START_OFFSET_REPLY,
                            bsal_input_stream_set_offset_reply);
        thorium_actor_add_action(self, ACTION_INPUT_STREAM_SET_END_OFFSET_REPLY,
                            bsal_input_stream_set_offset_reply);

        concrete_self->finished_parallel_stream_count = 0;

        size = bsal_vector_size(&concrete_self->parallel_streams);

        for (i = 0; i < size; i++) {

            start_offset = bsal_vector_at_as_uint64_t(&concrete_self->start_offsets, i);
            end_offset = bsal_vector_at_as_uint64_t(&concrete_self->end_offsets, i);
            stream = bsal_vector_at_as_int(&concrete_self->parallel_streams, i);

#ifdef DEBUG_ISSUE_594
            printf("actor %d send ACTION_INPUT_STREAM_SET_START_OFFSET to %d\n",
                            name, stream);

            printf("actor %d send ACTION_INPUT_STREAM_SET_END_OFFSET to %d\n",
                            name, stream);
#endif

            thorium_actor_send_uint64_t(self, stream, ACTION_INPUT_STREAM_SET_START_OFFSET,
                            start_offset);
            thorium_actor_send_uint64_t(self, stream, ACTION_INPUT_STREAM_SET_END_OFFSET,
                            end_offset);
        }
    }
}

void bsal_input_stream_open_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_input_stream *concrete_self;
    int i;
    int size;
    struct bsal_vector *vector;

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(self);

    ++concrete_self->finished_parallel_stream_count;

#if 0
    printf("DEBUG open_reply\n");
#endif

    if (concrete_self->finished_parallel_stream_count ==
                    bsal_vector_size(&concrete_self->parallel_streams)) {

        concrete_self->finished_parallel_stream_count = 0;

        size = bsal_vector_size(&concrete_self->parallel_streams);

        bsal_vector_resize(&concrete_self->parallel_mega_blocks,
                       size);

        for (i = 0; i < size; i++) {

            vector = bsal_vector_at(&concrete_self->parallel_mega_blocks, i);

            bsal_vector_init(vector, sizeof(struct bsal_mega_block));
        }

        thorium_actor_send_range_empty(self, &concrete_self->parallel_streams,
                        ACTION_INPUT_COUNT);
    }
}

void bsal_input_stream_count_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_input_stream *concrete_self;
    void *buffer;
    uint64_t result;
    struct bsal_mega_block *block;
    int i;
    int size;
    struct bsal_vector *vector;
    int source_index;
    int source;
    int j;
    uint64_t total;

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(self);

    buffer = thorium_message_buffer(message);

    source = thorium_message_source(message);
    source_index = bsal_vector_index_of(&concrete_self->parallel_streams, &source);
    vector = bsal_vector_at(&concrete_self->parallel_mega_blocks, source_index);
    bsal_vector_unpack(vector, buffer);

    block = bsal_vector_at_last(vector);

    result = bsal_mega_block_get_entries(block);

    concrete_self->total_entries += result;
    ++concrete_self->finished_parallel_stream_count;

    printf("DEBUG count_reply %d/%d\n",
                    concrete_self->finished_parallel_stream_count,
                    (int)bsal_vector_size(&concrete_self->parallel_streams));

    /*
     * Send back an array of mega blocks when it is done.
     */
    if (concrete_self->finished_parallel_stream_count ==
                    bsal_vector_size(&concrete_self->parallel_streams)) {

        /*
         * Transfer mega blocks
         * to main vector.
         */

        size = bsal_vector_size(&concrete_self->parallel_streams);

        for (i = 0; i < size; i++) {

            /*
             * This is easy to do because they are already sorted.
             *
             * With one parallel stream, there is nothing else to do.
             *
             * Otherwise, mega blocks with correct entries_from_start
             * need to be created (the entries fields are only for
             * the stuff between 2 offsets so they are already
             * correct.
             */
            vector = bsal_vector_at(&concrete_self->parallel_mega_blocks,
                            i);

#if 0
            printf("ParallelStream %d\n", i);
#endif

            for (j = 0; j < bsal_vector_size(vector); j++) {

                block = bsal_vector_at(vector, j);
                bsal_mega_block_print(block);
            }

            bsal_vector_push_back_vector(&concrete_self->mega_blocks,
                            vector);
        }

        /*
         * Update total
         */

        total = 0;

        for (i = 0; i < bsal_vector_size(&concrete_self->mega_blocks); i++) {

            block = bsal_vector_at(&concrete_self->mega_blocks, i);

            total += bsal_mega_block_get_entries(block);

            bsal_mega_block_set_entries_from_start(block, total);
        }

        /*
         * Destroy mega block vectors.
         */

        for (i = 0; i < size; i++) {

            vector = bsal_vector_at(&concrete_self->parallel_mega_blocks,
                            i);

            bsal_vector_destroy(vector);
        }

        bsal_vector_resize(&concrete_self->parallel_mega_blocks, 0);

        printf("DEBUG send ACTION_INPUT_COUNT_IN_PARALLEL_REPLY to %d\n",
                        concrete_self->controller);

        thorium_actor_send_vector(self, concrete_self->controller,
                    ACTION_INPUT_COUNT_IN_PARALLEL_REPLY,
                    &concrete_self->mega_blocks);
    }
}

void bsal_input_stream_count_in_parallel_mock(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_input_stream *concrete_self;
    void *buffer;
    int count;
    char *file;

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(self);

    buffer = thorium_message_buffer(message);
    count = thorium_message_count(message);

    file = concrete_self->file_name;

    printf("%s/%d receives ACTION_INPUT_COUNT_IN_PARALLEL file %s\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    file);

    thorium_actor_send_to_self_buffer(self, ACTION_INPUT_COUNT, count, buffer);
}

void bsal_input_stream_count_reply_mock(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_input_stream *concrete_self;
    void *buffer;
    int count;
    struct bsal_vector mega_blocks;
    char *file;
    struct bsal_memory_pool *ephemeral_memory;
    uint64_t result;
    struct bsal_mega_block *block;

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);
    count = thorium_message_count(message);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    bsal_vector_init(&mega_blocks, 0);
    bsal_vector_set_memory_pool(&mega_blocks, ephemeral_memory);
    bsal_vector_unpack(&mega_blocks, buffer);

    block = bsal_vector_at_last(&mega_blocks);

    result = bsal_mega_block_get_entries(block);

#if 0
    file = bsal_string_get(&concrete_self->file_for_parallel_counting);
#endif

    file = concrete_self->file_name;

    printf("%s/%d COUNT_IN_PARALLEL result for %s is %" PRIu64 "\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    file,
                    result);

    bsal_vector_destroy(&mega_blocks);

    thorium_actor_send_buffer(self, concrete_self->controller,
                    ACTION_INPUT_COUNT_IN_PARALLEL_REPLY, count, buffer);
}

void bsal_input_stream_set_offset_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_input_stream *concrete_self;

    concrete_self = (struct bsal_input_stream *)thorium_actor_concrete_actor(self);
    ++concrete_self->finished_parallel_stream_count;

#ifdef DEBUG_ISSUE_594
    printf("DEBUG bsal_input_stream_set_offset_reply %d/%d\n",
                    concrete_self->finished_parallel_stream_count,
                    2 * bsal_vector_size(&concrete_self->parallel_streams));
#endif

    if (concrete_self->finished_parallel_stream_count ==
                    2 * bsal_vector_size(&concrete_self->parallel_streams)) {
        /*
         * Assign files to input streams
         */

        thorium_actor_add_action(self, ACTION_INPUT_OPEN_REPLY,
                        bsal_input_stream_open_reply);

        concrete_self->finished_parallel_stream_count = 0;

        thorium_actor_send_range_buffer(self,
                        &concrete_self->parallel_streams,
                        ACTION_INPUT_OPEN,
                        strlen(concrete_self->file_name) + 1,
                        concrete_self->file_name);

        printf("DEBUG SEND ACTION_INPUT_OPEN to %d actors with file %s\n",
                        (int)bsal_vector_size(&concrete_self->parallel_streams),
                        concrete_self->file_name);
    }
}
