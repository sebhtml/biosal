
#include "input_controller.h"

#include "input_stream.h"
#include "mega_block.h"

#include <genomics/storage/sequence_store.h>
#include <genomics/storage/sequence_partitioner.h>
#include <genomics/storage/partition_command.h>
#include <genomics/input/input_command.h>

#include <core/structures/vector.h>
#include <core/structures/vector_iterator.h>
#include <core/structures/map_iterator.h>

#include <core/helpers/vector_helper.h>
#include <core/helpers/map_helper.h>
#include <core/helpers/message_helper.h>
#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h> /* for PRIu64 */

/*
 * The list below contains debugging options
 */
/*
#define BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
#define BSAL_INPUT_CONTROLLER_DEBUG_10355
#define BSAL_INPUT_CONTROLLER_DEBUG
#define BSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
#define BSAL_INPUT_CONTROLLER_DEBUG_CONSUMERS
*/


/* states of this actor
 */
#define BSAL_INPUT_CONTROLLER_STATE_NONE 0
#define BSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS 1
#define BSAL_INPUT_CONTROLLER_STATE_SPAWN_STREAMS 2
#define BSAL_INPUT_CONTROLLER_STATE_SPAWN_STORES 3
#define BSAL_INPUT_CONTROLLER_STATE_SPAWN_PARTITIONER 4
#define BSAL_INPUT_CONTROLLER_STATE_SPAWN_READING_STREAMS 5

struct bsal_script bsal_input_controller_script = {
    .identifier = BSAL_INPUT_CONTROLLER_SCRIPT,
    .init = bsal_input_controller_init,
    .destroy = bsal_input_controller_destroy,
    .receive = bsal_input_controller_receive,
    .size = sizeof(struct bsal_input_controller),
    .name = "input_controller"
};

void bsal_input_controller_init(struct bsal_actor *actor)
{
    struct bsal_input_controller *concrete_actor;

    concrete_actor = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);

    bsal_map_init(&concrete_actor->mega_blocks, sizeof(int), sizeof(struct bsal_vector));
    bsal_map_init(&concrete_actor->assigned_blocks, sizeof(int), sizeof(int));
    bsal_vector_init(&concrete_actor->mega_block_vector, sizeof(struct bsal_mega_block));

    bsal_vector_init(&concrete_actor->counting_streams, sizeof(int));
    bsal_vector_init(&concrete_actor->reading_streams, sizeof(int));
    bsal_vector_init(&concrete_actor->partition_commands, sizeof(int));
    bsal_vector_init(&concrete_actor->stream_consumers, sizeof(int));
    bsal_vector_init(&concrete_actor->consumer_active_requests, sizeof(int));
    bsal_vector_init(&concrete_actor->files, sizeof(char *));
    bsal_vector_init(&concrete_actor->spawners, sizeof(int));
    bsal_vector_init(&concrete_actor->counts, sizeof(int64_t));
    bsal_vector_init(&concrete_actor->consumers, sizeof(int));
    bsal_vector_init(&concrete_actor->stores_per_spawner, sizeof(int));

    bsal_timer_init(&concrete_actor->input_timer);
    bsal_timer_init(&concrete_actor->counting_timer);
    bsal_timer_init(&concrete_actor->distribution_timer);

    bsal_dna_codec_init(&concrete_actor->codec);

    if (bsal_actor_get_node_count(actor) >= BSAL_DNA_CODEC_MINIMUM_NODE_COUNT_FOR_TWO_BIT) {
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_TRANSPORT
        bsal_dna_codec_enable_two_bit_encoding(&concrete_actor->codec);
#endif
    }

    bsal_queue_init(&concrete_actor->unprepared_spawners, sizeof(int));

    concrete_actor->opened_streams = 0;
    concrete_actor->state = BSAL_INPUT_CONTROLLER_STATE_NONE;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_10355
    printf("DEBUG actor %d register BSAL_INPUT_CONTROLLER_CREATE_STORES\n",
                    bsal_actor_name(actor));
#endif

    bsal_actor_add_route(actor, BSAL_INPUT_CONTROLLER_CREATE_STORES,
                    bsal_input_controller_create_stores);
    bsal_actor_add_route(actor, BSAL_ACTOR_GET_NODE_NAME_REPLY,
                    bsal_input_controller_get_node_name_reply);
    bsal_actor_add_route(actor, BSAL_ACTOR_GET_NODE_WORKER_COUNT_REPLY,
                    bsal_input_controller_get_node_worker_count_reply);

    bsal_actor_add_route(actor, BSAL_INPUT_CONTROLLER_PREPARE_SPAWNERS,
                    bsal_input_controller_prepare_spawners);
    bsal_actor_add_route(actor, BSAL_INPUT_CONTROLLER_SPAWN_READING_STREAMS,
                    bsal_input_controller_spawn_streams);

    bsal_actor_add_route(actor, BSAL_INPUT_STREAM_SET_OFFSET_REPLY,
                    bsal_input_controller_set_offset_reply);
    bsal_actor_add_script(actor, BSAL_INPUT_STREAM_SCRIPT, &bsal_input_stream_script);
    bsal_actor_add_script(actor, BSAL_SEQUENCE_STORE_SCRIPT, &bsal_sequence_store_script);
    bsal_actor_add_script(actor, BSAL_SEQUENCE_PARTITIONER_SCRIPT,
                    &bsal_sequence_partitioner_script);

    /* configuration for the input controller
     * other values for block size: 512, 1024, 2048, 4096, 8192 * /
     */
    concrete_actor->block_size = 4096;
    concrete_actor->stores_per_worker_per_spawner = 0;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG %d init controller\n",
                    bsal_actor_name(actor));
#endif

    concrete_actor->ready_spawners = 0;
    concrete_actor->ready_consumers = 0;
    concrete_actor->partitioner = BSAL_ACTOR_NOBODY;
    concrete_actor->filled_consumers = 0;

    concrete_actor->counted = 0;
}

void bsal_input_controller_destroy(struct bsal_actor *actor)
{
    struct bsal_input_controller *concrete_actor;
    int i;
    char *pointer;
    struct bsal_map_iterator iterator;
    struct bsal_vector *vector;

    concrete_actor = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);

    bsal_timer_destroy(&concrete_actor->input_timer);
    bsal_timer_destroy(&concrete_actor->counting_timer);
    bsal_timer_destroy(&concrete_actor->distribution_timer);

    bsal_dna_codec_destroy(&concrete_actor->codec);

    for (i = 0; i < bsal_vector_size(&concrete_actor->files); i++) {
        pointer = *(char **)bsal_vector_at(&concrete_actor->files, i);
        bsal_memory_free(pointer);
    }

    bsal_vector_destroy(&concrete_actor->mega_block_vector);
    bsal_vector_destroy(&concrete_actor->counting_streams);
    bsal_vector_destroy(&concrete_actor->reading_streams);
    bsal_vector_destroy(&concrete_actor->partition_commands);
    bsal_vector_destroy(&concrete_actor->consumer_active_requests);
    bsal_vector_destroy(&concrete_actor->stream_consumers);
    bsal_vector_destroy(&concrete_actor->files);
    bsal_vector_destroy(&concrete_actor->spawners);
    bsal_vector_destroy(&concrete_actor->counts);
    bsal_vector_destroy(&concrete_actor->consumers);
    bsal_vector_destroy(&concrete_actor->stores_per_spawner);
    bsal_queue_destroy(&concrete_actor->unprepared_spawners);

    bsal_map_iterator_init(&iterator, &concrete_actor->mega_blocks);
    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, NULL, (void **)&vector);

        bsal_vector_destroy(vector);
    }
    bsal_map_iterator_destroy(&iterator);
    bsal_map_destroy(&concrete_actor->mega_blocks);
    bsal_map_destroy(&concrete_actor->assigned_blocks);
}

void bsal_input_controller_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int count;
    char *file;
    void *buffer;
    struct bsal_input_controller *controller;
    struct bsal_input_controller *concrete_actor;
    int destination;
    int script;
    int stream;
    char *local_file;
    int i;
    int name;
    int source;
    int destination_index;
    struct bsal_message new_message;
    int error;
    int stream_index;
    int64_t entries;
    int64_t *bucket;
    int *int_bucket;
    int spawner;
    int command_name;
    int stream_name;
    int consumer;
    int consumer_index;
    int *bucket_for_requests;
    char *new_buffer;
    int new_count;
    int file_index;
    struct bsal_vector mega_blocks;
    struct bsal_vector_iterator vector_iterator;
    struct bsal_mega_block *mega_block;
    struct bsal_vector *vector_bucket;
    struct bsal_vector block_counts;
    uint64_t block_entries;
    int mega_block_index;
    uint64_t offset;
    struct bsal_mega_block *block;
    int acquaintance_index;
    struct bsal_memory_pool *ephemeral_memory;

    if (bsal_actor_use_route(actor, message)) {
        return;
    }

    bsal_message_get_all(message, &tag, &count, &buffer, &source);

    ephemeral_memory = (struct bsal_memory_pool *)bsal_actor_get_ephemeral_memory(actor);
    name = bsal_actor_name(actor);
    controller = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);
    concrete_actor = controller;

    if (tag == BSAL_ACTOR_START) {

        bsal_vector_init(&concrete_actor->spawners, 0);
        bsal_vector_unpack(&concrete_actor->spawners, buffer);

        bsal_vector_resize(&concrete_actor->stores_per_spawner,
                        bsal_vector_size(&concrete_actor->spawners));

        for (i = 0; i < bsal_vector_size(&concrete_actor->spawners); i++) {
            int_bucket = (int *)bsal_vector_at(&concrete_actor->stores_per_spawner, i);
            *int_bucket = 0;

            spawner = bsal_vector_at_as_int(&concrete_actor->spawners, i);

            bsal_queue_enqueue(&concrete_actor->unprepared_spawners, &spawner);
        }

        concrete_actor->state = BSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL
        printf("DEBUG preparing first spawner\n");
#endif

        bsal_actor_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_PREPARE_SPAWNERS);

        /*
        bsal_dispatcher_print(bsal_actor_dispatcher(actor));
        */

    } else if (tag == BSAL_ADD_FILE) {

        file = (char *)buffer;

        local_file = bsal_memory_allocate(strlen(file) + 1);
        strcpy(local_file, file);

        bsal_vector_push_back(&concrete_actor->files, &local_file);

        bucket = bsal_vector_at(&concrete_actor->files, bsal_vector_size(&concrete_actor->files) - 1);
        local_file = *(char **)bucket;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG11 BSAL_ADD_FILE %s %p bucket %p index %d\n",
                        local_file, local_file, (void *)bucket, bsal_vector_size(&concrete_actor->files) - 1);
#endif

        bsal_actor_send_reply_empty(actor, BSAL_ADD_FILE_REPLY);

    } else if (tag == BSAL_ACTOR_SPAWN_REPLY) {

        if (concrete_actor->state == BSAL_INPUT_CONTROLLER_STATE_SPAWN_STORES) {

            bsal_input_controller_add_store(actor, message);
            return;

        } else if (concrete_actor->state == BSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS) {

            concrete_actor->ready_spawners++;
            bsal_message_unpack_int(message, 0, &name);
            bsal_actor_send_empty(actor, name, BSAL_ACTOR_ASK_TO_STOP);
            bsal_actor_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_PREPARE_SPAWNERS);

            if (concrete_actor->ready_spawners == (int)bsal_vector_size(&concrete_actor->spawners)) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
                printf("DEBUG all spawners are prepared\n");
#endif
                bsal_actor_send_to_supervisor_empty(actor, BSAL_ACTOR_START_REPLY);
            }

            return;

        } else if (concrete_actor->state == BSAL_INPUT_CONTROLLER_STATE_SPAWN_PARTITIONER) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
            printf("DEBUG received spawn reply, state is spawn_partitioner\n");
#endif

            bsal_message_unpack_int(message, 0, &concrete_actor->partitioner);

            /* configure the partitioner
             */
            destination = concrete_actor->partitioner;
            bsal_actor_send_int(actor, destination,
                            BSAL_SEQUENCE_PARTITIONER_SET_BLOCK_SIZE,
                            concrete_actor->block_size);
            bsal_actor_send_int(actor, destination,
                            BSAL_SEQUENCE_PARTITIONER_SET_ACTOR_COUNT,
                            bsal_vector_size(&concrete_actor->consumers));

            bsal_vector_init(&block_counts, sizeof(uint64_t));

            for (i = 0; i < bsal_vector_size(&concrete_actor->mega_block_vector); i++) {

                block = (struct bsal_mega_block *)bsal_vector_at(&concrete_actor->mega_block_vector, i);
                block_entries = bsal_mega_block_get_entries(block);

                bsal_vector_push_back_uint64_t(&block_counts, block_entries);
            }

            new_count = bsal_vector_pack_size(&block_counts);
            new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
            printf("DEBUG packed counts, %d bytes\n", count);
#endif

            bsal_vector_pack(&block_counts, new_buffer);
            bsal_message_init(&new_message, BSAL_SEQUENCE_PARTITIONER_SET_ENTRY_VECTOR,
                            new_count, new_buffer);
            bsal_actor_send(actor, destination, &new_message);
            bsal_memory_pool_free(ephemeral_memory, new_buffer);
            bsal_vector_destroy(&block_counts);

            return;

        } else if (concrete_actor->state == BSAL_INPUT_CONTROLLER_STATE_SPAWN_READING_STREAMS) {

            bsal_message_unpack_int(message, 0, &stream);

            stream_index = stream;

            mega_block_index = bsal_vector_size(&concrete_actor->reading_streams);

            bsal_vector_push_back_int(&concrete_actor->reading_streams, stream_index);
            bsal_vector_push_back_int(&concrete_actor->partition_commands, -1);
            bsal_vector_push_back_int(&concrete_actor->stream_consumers, -1);

            stream_index = bsal_vector_size(&concrete_actor->reading_streams) - 1;
            mega_block = (struct bsal_mega_block *)bsal_vector_at(&concrete_actor->mega_block_vector,
                            mega_block_index);

            offset = bsal_mega_block_get_offset(mega_block);

            bsal_map_add_value(&concrete_actor->assigned_blocks,
                            &stream_index, &mega_block_index);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
            printf("DEBUG setting offset to %" PRIu64 " for stream/%d\n",
                            offset, stream);
#endif

            bsal_actor_send_uint64_t(actor, stream, BSAL_INPUT_STREAM_SET_OFFSET, offset);

            return;
        }

        stream = *(int *)buffer;

        file_index = bsal_vector_size(&concrete_actor->counting_streams);
        local_file = *(char **)bsal_vector_at(&concrete_actor->files, file_index);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
        printf("DEBUG actor %d receives stream %d from spawner %d for file %s\n",
                        name, stream, source,
                        local_file);
#endif

        bsal_vector_push_back(&concrete_actor->counting_streams, &stream);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
        printf("asking stream/%d to open %s\n", stream, local_file);
#endif
        bsal_message_init(&new_message, BSAL_INPUT_OPEN, strlen(local_file) + 1, local_file);
        bsal_actor_send(actor, stream, &new_message);
        bsal_message_destroy(&new_message);

        if (bsal_vector_size(&concrete_actor->counting_streams) != bsal_vector_size(&concrete_actor->files)) {

            bsal_actor_send_to_self_empty(actor, BSAL_INPUT_SPAWN);

        }

    } else if (tag == BSAL_INPUT_OPEN_REPLY) {

        if (concrete_actor->state == BSAL_INPUT_CONTROLLER_STATE_SPAWN_READING_STREAMS) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
            printf("DEBUG receives open.reply for reading stream/%d\n", source);
#endif
            concrete_actor->opened_streams++;

            if (concrete_actor->opened_streams == bsal_vector_size(&concrete_actor->mega_block_vector)) {
                bsal_actor_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_CREATE_STORES);
            }

            return;
        }

        concrete_actor->opened_streams++;

        stream = source;
        bsal_message_unpack_int(message, 0, &error);

        if (error == BSAL_INPUT_ERROR_NO_ERROR) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
            printf("DEBUG actor %d asks %d BSAL_INPUT_COUNT\n", name, stream);
#endif

            bsal_actor_send_empty(actor, stream, BSAL_INPUT_COUNT);
        } else {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
            printf("DEBUG actor %d received error %d from %d\n", name, error, stream);
#endif
            concrete_actor->counted++;
        }

	/* if all streams failed, notice supervisor */
        if (concrete_actor->counted == bsal_vector_size(&concrete_actor->files)) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
#endif
            printf("DEBUG %d: Error all streams failed.\n",
                            bsal_actor_name(actor));
            bsal_actor_send_to_supervisor_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);
        }

/*
        if (concrete_actor->opened_streams == bsal_vector_size(&concrete_actor->files)) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
            printf("DEBUG controller %d sends BSAL_INPUT_DISTRIBUTE_REPLY to supervisor %d [%d/%d]\n",
                            name, bsal_actor_supervisor(actor),
                            concrete_actor->opened_streams, bsal_vector_size(&concrete_actor->files));
#endif

        }
*/

    } else if (tag == BSAL_INPUT_COUNT_PROGRESS) {

        stream_index = bsal_vector_index_of(&concrete_actor->counting_streams, &source);
        local_file = bsal_vector_at_as_char_pointer(&concrete_actor->files, stream_index);
        bsal_message_unpack_int64_t(message, 0, &entries);

        bucket = (int64_t *)bsal_vector_at(&concrete_actor->counts, stream_index);

        printf("controller/%d receives progress from stream/%d file %s %" PRIu64 " entries so far\n",
                        name, source, local_file, entries);
        *bucket = entries;

    } else if (tag == BSAL_INPUT_COUNT_REPLY) {

        stream_index = bsal_vector_index_of(&concrete_actor->counting_streams, &source);
        local_file = bsal_vector_at_as_char_pointer(&concrete_actor->files, stream_index);

        bsal_vector_init(&mega_blocks, 0);
        bsal_vector_unpack(&mega_blocks, buffer);

        printf("DEBUG receive mega blocks from %d\n", source);
        /*
         * Update the file index for every mega block.
         */
        bsal_vector_iterator_init(&vector_iterator, &mega_blocks);

        bucket = (int64_t*)bsal_vector_at(&concrete_actor->counts, stream_index);
        (*bucket) = 0;

        while (bsal_vector_iterator_has_next(&vector_iterator)) {
            bsal_vector_iterator_next(&vector_iterator, (void **)&mega_block);

            printf("SETTING setting file to %d for mega block\n", stream_index);
            bsal_mega_block_set_file(mega_block, stream_index);

            entries = bsal_mega_block_get_entries_from_start(mega_block);

            printf("Cataloging %d ENTRIES\n", (int)entries);

            (*bucket) = entries;

            bsal_mega_block_print(mega_block);
        }

        bsal_vector_iterator_destroy(&vector_iterator);

        vector_bucket = (struct bsal_vector *)bsal_map_add(&concrete_actor->mega_blocks, &stream_index);
        bsal_vector_init_copy(vector_bucket, &mega_blocks);

        bsal_vector_destroy(&mega_blocks);

        concrete_actor->counted++;

        printf("controller/%d received from stream/%d for file %s %" PRIu64 " entries (final) %d/%d\n",
                        name, source, local_file, entries,
                        concrete_actor->counted, (int)bsal_vector_size(&concrete_actor->files));

        bsal_actor_send_reply_empty(actor, BSAL_INPUT_CLOSE);

        /* continue work here, tell supervisor about it */
        if (concrete_actor->counted == bsal_vector_size(&concrete_actor->files)) {
            bsal_actor_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_SPAWN_READING_STREAMS);
        }


    } else if (tag == BSAL_INPUT_DISTRIBUTE) {

        bsal_timer_start(&concrete_actor->input_timer);
        bsal_timer_start(&concrete_actor->counting_timer);

        /* for each file, spawn a stream to count */

        /* no files, return immediately
         */
        if (bsal_vector_size(&concrete_actor->files) == 0) {

            printf("Error: no file to distribute...\n");
            bsal_actor_send_reply_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);
            return;
        }

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG actor %d receives BSAL_INPUT_DISTRIBUTE\n", name);
#endif

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG send BSAL_INPUT_SPAWN to self\n");
#endif

        bsal_actor_send_to_self_empty(actor, BSAL_INPUT_SPAWN);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG resizing counts to %d\n", bsal_vector_size(&concrete_actor->files));
#endif

        bsal_vector_resize(&concrete_actor->counts, bsal_vector_size(&concrete_actor->files));

        for (i = 0; i < bsal_vector_size(&concrete_actor->counts); i++) {
            bucket = (int64_t*)bsal_vector_at(&concrete_actor->counts, i);
            *bucket = 0;
        }

    } else if (tag == BSAL_INPUT_SPAWN && source == name) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG BSAL_INPUT_SPAWN\n");
#endif

        script = BSAL_INPUT_STREAM_SCRIPT;

        concrete_actor->state = BSAL_INPUT_CONTROLLER_STATE_SPAWN_STREAMS;

        /* the next file name to send is the current number of streams */
        i = bsal_vector_size(&concrete_actor->counting_streams);

        destination_index = i % bsal_vector_size(&concrete_actor->spawners);
        destination = *(int *)bsal_vector_at(&concrete_actor->spawners, destination_index);

        bsal_message_init(message, BSAL_ACTOR_SPAWN, sizeof(script), &script);
        bsal_actor_send(actor, destination, message);

        bucket = bsal_vector_at(&concrete_actor->files, i);
        local_file = *(char **)bsal_vector_at(&concrete_actor->files, i);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG890 local_file %p bucket %p index %d\n", local_file, (void *)bucket,
                        i);
#endif

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG actor %d spawns a stream for file %d/%d via spawner %d\n",
                        name, i, bsal_vector_size(&concrete_actor->files), destination);
#endif

        /* also, spawn 4 stores on each node */

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP && ( source == bsal_actor_supervisor(actor)
                            || source == bsal_actor_name(actor))) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
#endif

        /* stop streams
         */
        for (i = 0; i < bsal_vector_size(&concrete_actor->counting_streams); i++) {
            stream = *(int *)bsal_vector_at(&concrete_actor->counting_streams, i);

            bsal_actor_send_empty(actor, stream, BSAL_ACTOR_ASK_TO_STOP);
        }
        for (i = 0; i < bsal_vector_size(&concrete_actor->reading_streams); i++) {
            stream = *(int *)bsal_vector_at(&concrete_actor->reading_streams, i);

            bsal_actor_send_empty(actor, stream, BSAL_ACTOR_ASK_TO_STOP);
        }



#if 0
        /* stop data stores
         */
        for (i = 0; i < bsal_vector_size(&concrete_actor->consumers); i++) {
            store = bsal_vector_at_as_int(&concrete_actor->consumers, i);

            bsal_actor_send_empty(actor, store, BSAL_ACTOR_ASK_TO_STOP);
        }
#endif
        /* stop partitioner
         */

        if (concrete_actor->partitioner != BSAL_ACTOR_NOBODY) {
            bsal_actor_send_empty(actor,
                                concrete_actor->partitioner,
                        BSAL_ACTOR_ASK_TO_STOP);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
            printf("DEBUG controller %d sends BSAL_ACTOR_ASK_TO_STOP_REPLY to %d\n",
                        bsal_actor_name(actor),
                        bsal_message_source(message));
#endif

        }

        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_ASK_TO_STOP_REPLY);

        /* stop self
         */
        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);

        bsal_actor_ask_to_stop(actor, message);

        printf("DEBUG controller %d dies\n", name);
#ifdef BSAL_INPUT_CONTROLLER_DEBUG
#endif

    } else if (tag == BSAL_INPUT_CONTROLLER_CREATE_PARTITION && source == name) {

        spawner = *(int *)bsal_vector_at(&concrete_actor->spawners,
                        bsal_vector_size(&concrete_actor->spawners) / 2);

        bsal_actor_send_int(actor, spawner, BSAL_ACTOR_SPAWN,
                        BSAL_SEQUENCE_PARTITIONER_SCRIPT);
        concrete_actor->state = BSAL_INPUT_CONTROLLER_STATE_SPAWN_PARTITIONER;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG input controller %d spawns a partitioner via spawner %d\n",
                        name,  spawner);
#endif

    } else if (tag == BSAL_SEQUENCE_PARTITIONER_COMMAND_IS_READY) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG controller receives BSAL_SEQUENCE_PARTITIONER_COMMAND_IS_READY, asks for command\n");
#endif

        bsal_actor_send_reply_empty(actor, BSAL_SEQUENCE_PARTITIONER_GET_COMMAND);

    } else if (tag == BSAL_SEQUENCE_PARTITIONER_GET_COMMAND_REPLY) {

        bsal_input_controller_receive_command(actor, message);

    } else if (tag == BSAL_SEQUENCE_PARTITIONER_FINISHED) {

        bsal_actor_send_empty(actor,
                                concrete_actor->partitioner,
                        BSAL_ACTOR_ASK_TO_STOP);

        bsal_input_controller_verify_requests(actor, message);

    } else if (tag == BSAL_SEQUENCE_PARTITIONER_PROVIDE_STORE_ENTRY_COUNTS) {

        bsal_input_controller_receive_store_entry_counts(actor, message);

    } else if (tag == BSAL_RESERVE_REPLY) {

        concrete_actor->ready_consumers++;

        printf("DEBUG marker BSAL_RESERVE_REPLY %d/%d\n",
                        concrete_actor->ready_consumers,
                        (int)bsal_vector_size(&concrete_actor->consumers));

        if (concrete_actor->ready_consumers == bsal_vector_size(&concrete_actor->consumers)) {

            concrete_actor->ready_consumers = 0;
            printf("DEBUG all consumers are ready\n");
            bsal_actor_send_empty(actor,
                            concrete_actor->partitioner,
                            BSAL_SEQUENCE_PARTITIONER_PROVIDE_STORE_ENTRY_COUNTS_REPLY);
        }

    } else if (tag == BSAL_INPUT_PUSH_SEQUENCES_READY) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG bsal_input_controller_receive received BSAL_INPUT_PUSH_SEQUENCES_READY\n");
#endif

        stream_name = source;

        acquaintance_index = stream_name;
        stream_index = bsal_vector_index_of(&concrete_actor->reading_streams, &acquaintance_index);
        command_name = *(int *)bsal_vector_at(&concrete_actor->partition_commands,
                        stream_index);

        bsal_actor_send_int(actor,
                                concrete_actor->partitioner,
                        BSAL_SEQUENCE_PARTITIONER_GET_COMMAND_REPLY_REPLY,
                        command_name);

    } else if (tag == BSAL_INPUT_PUSH_SEQUENCES_REPLY) {

        stream_name = source;

        bsal_message_unpack_int(message, 0, &consumer);

        consumer_index = bsal_vector_index_of(&concrete_actor->consumers,
                        &consumer);

        bucket_for_requests = (int *)bsal_vector_at(&concrete_actor->consumer_active_requests, consumer_index);

        (*bucket_for_requests)--;

        bsal_input_controller_verify_requests(actor, message);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_CONSUMERS
        printf("DEBUG consumer # %d has %d active requests\n",
                        consumer_index, *bucket_for_requests);
#endif


    } else if (tag == BSAL_ACTOR_SET_CONSUMERS) {

        bsal_vector_init(&concrete_actor->consumers, 0);
        bsal_vector_unpack(&concrete_actor->consumers, buffer);

        printf("controller %d receives %d consumers\n",
                        bsal_actor_name(actor),
                        (int)bsal_vector_size(&concrete_actor->consumers));

        for (i = 0; i < bsal_vector_size(&concrete_actor->consumers); i++) {
            bsal_vector_push_back_int(&concrete_actor->consumer_active_requests, 0);
        }

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        bsal_vector_print_int(&concrete_actor->consumers);
        printf("\n");
#endif
        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_SET_CONSUMERS_REPLY);

    } else if (tag == BSAL_SET_BLOCK_SIZE) {

        bsal_message_unpack_int(message, 0, &concrete_actor->block_size);
        bsal_actor_send_reply_empty(actor, BSAL_SET_BLOCK_SIZE_REPLY);

    } else if (tag == BSAL_SEQUENCE_STORE_READY) {

        concrete_actor->filled_consumers++;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG BSAL_SEQUENCE_STORE_READY %d/%d\n", concrete_actor->filled_consumers,
                        (int)bsal_vector_size(&concrete_actor->consumers));
#endif

        if (concrete_actor->filled_consumers == bsal_vector_size(&concrete_actor->consumers)) {
            concrete_actor->filled_consumers = 0;

            printf("DEBUG: all consumers are filled,  sending BSAL_INPUT_DISTRIBUTE_REPLY\n");

            bsal_timer_stop(&concrete_actor->input_timer);
            bsal_timer_stop(&concrete_actor->distribution_timer);

            bsal_timer_print_with_description(&concrete_actor->distribution_timer,
                            "Input / Distributing input data");

            bsal_timer_print_with_description(&concrete_actor->input_timer,
                            "Input");

            bsal_actor_send_to_supervisor_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);
        }
    }
}

void bsal_input_controller_receive_store_entry_counts(struct bsal_actor *actor, struct bsal_message *message)
{
    struct bsal_input_controller *concrete_actor;
    struct bsal_vector store_entries;
    void *buffer;
    int i;
    int store;
    uint64_t entries;
    struct bsal_message new_message;
    int name;

    bsal_vector_init(&store_entries, sizeof(uint64_t));

    concrete_actor = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);
    buffer = bsal_message_buffer(message);
    name = bsal_actor_name(actor);
    concrete_actor->ready_consumers = 0;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG bsal_input_controller_receive_store_entry_counts unpacking entries\n");
#endif

    bsal_vector_init(&store_entries, 0);
    bsal_vector_unpack(&store_entries, buffer);

    for (i = 0; i < bsal_vector_size(&store_entries); i++) {
        store = *(int *)bsal_vector_at(&concrete_actor->consumers, i);
        entries = *(uint64_t *)bsal_vector_at(&store_entries, i);

        printf("DEBUG controller/%d tells consumer/%d to reserve %" PRIu64 " buckets\n",
                        name, store, entries);

        bsal_message_init(&new_message, BSAL_RESERVE,
                        sizeof(entries), &entries);
        bsal_actor_send(actor, store, &new_message);
    }

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG bsal_input_controller_receive_store_entry_counts will wait for replies\n");
#endif

    bsal_vector_destroy(&store_entries);
}

void bsal_input_controller_create_stores(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    void *buffer;
    int count;
    int i;
    struct bsal_input_controller *concrete_actor;
    int value;
    int spawner;
    uint64_t total;
    int block_size;
    int blocks;
    uint64_t entries;
    char *local_file;
    int name;

    concrete_actor = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);

    bsal_message_get_all(message, &tag, &count, &buffer, &source);
/*
    printf("DEBUG bsal_input_controller_create_stores\n");
    */

    for (i = 0; i < bsal_vector_size(&concrete_actor->stores_per_spawner); i++) {
        value = bsal_vector_at_as_int(&concrete_actor->stores_per_spawner, i);

        if (value == -1) {

                /*
            printf("DEBUG need more information about spawner at %i\n",
                            i);
                            */

            spawner = bsal_vector_at_as_int(&concrete_actor->spawners, i);

            bsal_actor_send_empty(actor, spawner, BSAL_ACTOR_GET_NODE_NAME);
            return;
        }
    }

    concrete_actor->state = BSAL_INPUT_CONTROLLER_STATE_SPAWN_STORES;

    /* at this point, we know the worker count of every node corresponding
     * to each spawner
     */

    for (i = 0; i < bsal_vector_size(&concrete_actor->stores_per_spawner); i++) {
            /*
        printf("DEBUG polling spawner %i/%d\n", i,
                        bsal_vector_size(&concrete_actor->stores_per_spawner));
*/
        value = bsal_vector_at_as_int(&concrete_actor->stores_per_spawner, i);

        if (value != 0) {

            spawner = bsal_vector_at_as_int(&concrete_actor->spawners, i);
/*
            printf("DEBUG spawner %d is %d\n", i, spawner);
*/
            bsal_actor_send_int(actor, spawner, BSAL_ACTOR_SPAWN, BSAL_SEQUENCE_STORE_SCRIPT);

            return;
        }
/*
        printf("DEBUG spawner %i spawned all its stores\n", i);
        */
    }

    printf("DEBUG controller %d: consumers are ready (%d)\n",
                    bsal_actor_name(actor),
                    (int)bsal_vector_size(&concrete_actor->consumers));

    for (i = 0; i < bsal_vector_size(&concrete_actor->consumers); i++) {
        value = bsal_vector_at_as_int(&concrete_actor->consumers, i);

        printf("DEBUG controller %d: consumer %i is %d\n",
                        bsal_actor_name(actor), i, value);
    }

    printf("DEBUG controller %d: streams are\n",
                    bsal_actor_name(actor));

    total = 0;
    block_size = concrete_actor->block_size;

    for (i = 0; i < bsal_vector_size(&concrete_actor->files); i++) {
        entries = *(uint64_t*)bsal_vector_at(&concrete_actor->counts, i);
        local_file = bsal_vector_at_as_char_pointer(&concrete_actor->files, i);
        name = *(int *)bsal_vector_at(&concrete_actor->counting_streams, i);

        printf("stream %d, %d/%d %s %" PRIu64 "\n",
                        name, i,
                        (int)bsal_vector_size(&concrete_actor->files),
                        local_file,
                        entries);
        total += entries;
    }

    blocks = total / block_size;

    if (total % block_size != 0) {
        blocks++;
    }

    bsal_timer_stop(&concrete_actor->counting_timer);
    bsal_timer_start(&concrete_actor->distribution_timer);

    bsal_timer_print_with_description(&concrete_actor->counting_timer,
                    "Input / Counting input data");

    printf("DEBUG controller %d: Partition Total: %" PRIu64 ", block_size: %d, blocks: %d\n",
                    bsal_actor_name(actor),
                    total, block_size, blocks);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_10355
    printf("DEBUG send BSAL_INPUT_CONTROLLER_CREATE_STORES to self %d\n",
                            bsal_actor_name(actor));
#endif

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG bsal_input_controller_create_stores send BSAL_INPUT_CONTROLLER_CREATE_PARTITION\n");
#endif

    /* no sequences at all !
     */
    if (total == 0) {
        printf("Error, total is 0, can not distribute\n");
        bsal_actor_send_to_supervisor_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);
        return;
    } else {
        bsal_actor_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_CREATE_PARTITION);
    }

    /*
    bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    */
}

void bsal_input_controller_get_node_name_reply(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    void *buffer;
    int count;
    int spawner;
    int node;

    bsal_message_get_all(message, &tag, &count, &buffer, &source);
    spawner = source;
    bsal_message_unpack_int(message, 0, &node);

    printf("DEBUG spawner %d is on node node/%d\n", spawner, node);

    bsal_actor_send_reply_empty(actor, BSAL_ACTOR_GET_NODE_WORKER_COUNT);
}

void bsal_input_controller_get_node_worker_count_reply(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    void *buffer;
    int count;
    int index;
    struct bsal_input_controller *concrete_actor;
    int spawner;
    int worker_count;
    int *bucket;

    concrete_actor = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);

    bsal_message_get_all(message, &tag, &count, &buffer, &source);
    spawner = source;
    bsal_message_unpack_int(message, 0, &worker_count);

    index = bsal_vector_index_of(&concrete_actor->spawners, &spawner);
    bucket = bsal_vector_at(&concrete_actor->stores_per_spawner, index);
    *bucket = worker_count * concrete_actor->stores_per_worker_per_spawner;

    printf("DEBUG spawner %d (node %d) is on a node that has %d workers\n", spawner,
                    index, worker_count);

    bsal_actor_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_CREATE_STORES);
}

void bsal_input_controller_add_store(struct bsal_actor *actor, struct bsal_message *message)
{
    int source;
    int store;
    int index;
    struct bsal_input_controller *concrete_actor;
    int *bucket;

    /*
    printf("DEBUG bsal_input_controller_add_store\n");
    */

    concrete_actor = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);
    source = bsal_message_source(message);
    bsal_message_unpack_int(message, 0, &store);

    index = bsal_vector_index_of(&concrete_actor->spawners, &source);

    /*
    printf("DEBUG bsal_input_controller_add_store index %d\n", index);
    */

    bucket = bsal_vector_at(&concrete_actor->stores_per_spawner, index);

    /* the content of the bucket is initially the total number of
     * stores that are desired for this spawner.
     */
    *bucket = (*bucket - 1);
    bsal_vector_push_back(&concrete_actor->consumers, &store);

    bsal_actor_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_CREATE_STORES);

    /*
    printf("DEBUG remaining to spawn: %d (before returning)\n", *bucket);
    */
}

void bsal_input_controller_prepare_spawners(struct bsal_actor *actor, struct bsal_message *message)
{
    int spawner;
    struct bsal_input_controller *concrete_actor;

    concrete_actor = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG bsal_input_controller_prepare_spawners \n");
#endif

    /* spawn an actor of the same script on every spawner to load required
     * scripts on all nodes
     */
    if (bsal_queue_dequeue(&concrete_actor->unprepared_spawners, &spawner)) {

        bsal_actor_send_int(actor, spawner, BSAL_ACTOR_SPAWN, bsal_actor_script(actor));
        concrete_actor->state = BSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS;
    }
}

void bsal_input_controller_receive_command(struct bsal_actor *actor, struct bsal_message *message)
{
    struct bsal_partition_command command;
    void *buffer;
    int stream_index;
    int store_index;
    int store_name;
    uint64_t store_first;
    uint64_t store_last;
    int command_name;
    int stream_name;
    int *bucket_for_command_name;
    int bytes;
    struct bsal_input_command input_command;
    void *new_buffer;
    struct bsal_message new_message;
    struct bsal_input_controller *concrete_actor;
    int *bucket;
    int *bucket_for_consumer;
    int consumer_index;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(actor);
    concrete_actor = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);
    buffer = bsal_message_buffer(message);
    bsal_partition_command_unpack(&command, buffer);
    stream_index = bsal_partition_command_stream_index(&command);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
    printf("DEBUG bsal_input_controller_receive_command controller receives command for stream %d\n", stream_index);
    bsal_partition_command_print(&command);
#endif

    store_index = bsal_partition_command_store_index(&command);
    bucket_for_command_name = (int *)bsal_vector_at(&concrete_actor->partition_commands,
                    stream_index);
    bucket_for_consumer = (int *)bsal_vector_at(&concrete_actor->stream_consumers,
                    stream_index);

    stream_name = bsal_vector_at_as_int(&concrete_actor->reading_streams,
                    stream_index);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
    printf("DEBUG stream_index %d stream_name %d\n", stream_index, stream_name);
#endif

    store_name = *(int *)bsal_vector_at(&concrete_actor->consumers, store_index);
    store_first = bsal_partition_command_store_first(&command);
    store_last = bsal_partition_command_store_last(&command);

    bsal_input_command_init(&input_command, store_name, store_first, store_last);

    bytes = bsal_input_command_pack_size(&input_command,
                    &concrete_actor->codec);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
    printf("DEBUG input command\n");
    bsal_input_command_print(&input_command);

    printf("DEBUG bsal_input_controller_receive_command bytes %d\n",
                    bytes);
#endif

    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, bytes);
    bsal_input_command_pack(&input_command, new_buffer,
                    &concrete_actor->codec);

    bsal_message_init(&new_message, BSAL_INPUT_PUSH_SEQUENCES, bytes,
                    new_buffer);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
    printf("DEBUG bsal_input_controller_receive_command sending BSAL_INPUT_PUSH_SEQUENCES to %d (index %d)\n",
                    stream_name, stream_index);
    bsal_input_command_print(&input_command);

    printf("SENDING COMMAND TO stream/%d\n", stream_name);
#endif

    bsal_actor_send(actor, stream_name, &new_message);

    bsal_memory_pool_free(ephemeral_memory, new_buffer);

    command_name = bsal_partition_command_name(&command);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
    printf("controller/%d processed input command %d %p\n", bsal_actor_name(actor), command_name,
                    (void *)bucket_for_command_name);
#endif

    *bucket_for_command_name = command_name;

    consumer_index = store_index;
    *bucket_for_consumer = consumer_index;

    bucket = (int *)bsal_vector_at(&concrete_actor->consumer_active_requests, consumer_index);

    (*bucket)++;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_CONSUMERS
    printf("DEBUG consumer # %d has %d active requests\n",
                        consumer_index, *bucket);
#endif
}

void bsal_input_controller_spawn_streams(struct bsal_actor *actor, struct bsal_message *message)
{
    int spawner;
    struct bsal_input_controller *concrete_actor;
    struct bsal_vector_iterator iterator;
    int i;
    int j;
    int block_index;
    struct bsal_vector *vector;
    struct bsal_mega_block *block;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
    printf("DEBUG bsal_input_controller_spawn_streams\n");
#endif

    concrete_actor = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);
    concrete_actor->opened_streams = 0;
    concrete_actor->spawner = 0;

    /* gather mega blocks
     */

    block_index = 0;
    printf("DEBUG received MEGA BLOCKS\n");
    for (i = 0; i < bsal_vector_size(&concrete_actor->files); i++) {

        vector = (struct bsal_vector *)bsal_map_get(&concrete_actor->mega_blocks, &i);

        if (vector == NULL) {
            continue;
        }

        for (j = 0; j < bsal_vector_size(vector); j++) {
            block = (struct bsal_mega_block *)bsal_vector_at(vector, j);

            printf("BLOCK # %d ", block_index);
            block_index++;
            bsal_mega_block_print(block);

            bsal_vector_push_back(&concrete_actor->mega_block_vector, block);
        }
    }
    printf("DEBUG MEGA BLOCKS (total: %d)\n", block_index);

    bsal_vector_iterator_init(&iterator, &concrete_actor->mega_block_vector);

    while (bsal_vector_iterator_has_next(&iterator)) {
        bsal_vector_iterator_next(&iterator, NULL);
        spawner = bsal_vector_at_as_int(&concrete_actor->spawners, concrete_actor->spawner);

        concrete_actor->spawner++;
        concrete_actor->spawner %= bsal_vector_size(&concrete_actor->spawners);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
        printf("DEBUG asking %d to spawn script %d\n", spawner, BSAL_INPUT_STREAM_SCRIPT);
#endif

        bsal_actor_send_int(actor, spawner, BSAL_ACTOR_SPAWN, BSAL_INPUT_STREAM_SCRIPT);
    }

    bsal_vector_iterator_destroy(&iterator);

    concrete_actor->state = BSAL_INPUT_CONTROLLER_STATE_SPAWN_READING_STREAMS;
}

void bsal_input_controller_set_offset_reply(struct bsal_actor *self, struct bsal_message *message)
{
    int stream_index;
    int acquaintance_index;
    int source;
    int block_index;
    struct bsal_mega_block *block;
    struct bsal_input_controller *concrete_actor;
    int file_index;
    char *file_name;
    struct bsal_message new_message;

    source = bsal_message_source(message);
    acquaintance_index = source;
    concrete_actor = (struct bsal_input_controller *)bsal_actor_concrete_actor(self);
    stream_index = bsal_vector_index_of(&concrete_actor->reading_streams, &acquaintance_index);

    block_index = bsal_map_get_int(&concrete_actor->assigned_blocks, &stream_index);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
    printf("DEBUG got reply from stream/%d for offset, stream_index %d block_index %d\n", source,
                    stream_index, block_index);
#endif

    block = (struct bsal_mega_block *)bsal_vector_at(&concrete_actor->mega_block_vector,
                    block_index);

    file_index = bsal_mega_block_get_file(block);
    file_name = *(char **)bsal_vector_at(&concrete_actor->files, file_index);

    bsal_message_init(&new_message, BSAL_INPUT_OPEN, strlen(file_name) + 1, file_name);

    bsal_actor_send_reply(self, &new_message);

    bsal_message_destroy(&new_message);
}

void bsal_input_controller_verify_requests(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_input_controller *concrete_actor;
    int i;
    int active;

    active = 0;
    concrete_actor = (struct bsal_input_controller *)bsal_actor_concrete_actor(self);

    for (i = 0; i < bsal_vector_size(&concrete_actor->consumer_active_requests); i++) {
        if (bsal_vector_at_as_int(&concrete_actor->consumer_active_requests, i) != 0) {
            active++;
        }
    }

    if (active == 0) {
    }
}
