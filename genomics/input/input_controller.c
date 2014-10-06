
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
#define BIOSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
#define BIOSAL_INPUT_CONTROLLER_DEBUG_10355
#define BIOSAL_INPUT_CONTROLLER_DEBUG
#define BIOSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
#define BIOSAL_INPUT_CONTROLLER_DEBUG_CONSUMERS
*/

/* states of this actor
 */
#define BIOSAL_INPUT_CONTROLLER_STATE_NONE 0
#define BIOSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS 1
#define BIOSAL_INPUT_CONTROLLER_STATE_SPAWN_STREAMS 2
#define BIOSAL_INPUT_CONTROLLER_STATE_SPAWN_STORES 3
#define BIOSAL_INPUT_CONTROLLER_STATE_SPAWN_PARTITIONER 4
#define BIOSAL_INPUT_CONTROLLER_STATE_SPAWN_READING_STREAMS 5

void biosal_input_controller_init(struct thorium_actor *actor);
void biosal_input_controller_destroy(struct thorium_actor *actor);
void biosal_input_controller_receive(struct thorium_actor *actor, struct thorium_message *message);

void biosal_input_controller_create_stores(struct thorium_actor *actor, struct thorium_message *message);
void biosal_input_controller_get_node_name_reply(struct thorium_actor *actor, struct thorium_message *message);
void biosal_input_controller_get_node_worker_count_reply(struct thorium_actor *actor, struct thorium_message *message);
void biosal_input_controller_add_store(struct thorium_actor *actor, struct thorium_message *message);
void biosal_input_controller_prepare_spawners(struct thorium_actor *actor, struct thorium_message *message);
void biosal_input_controller_receive_store_entry_counts(struct thorium_actor *actor, struct thorium_message *message);
void biosal_input_controller_receive_command(struct thorium_actor *actor, struct thorium_message *message);

void biosal_input_controller_spawn_streams(struct thorium_actor *actor, struct thorium_message *message);
void biosal_input_controller_set_offset_reply(struct thorium_actor *self, struct thorium_message *message);
void biosal_input_controller_verify_requests(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script biosal_input_controller_script = {
    .identifier = SCRIPT_INPUT_CONTROLLER,
    .init = biosal_input_controller_init,
    .destroy = biosal_input_controller_destroy,
    .receive = biosal_input_controller_receive,
    .size = sizeof(struct biosal_input_controller),
    .name = "biosal_input_controller"
};

#define MEMORY_CONTROLLER SCRIPT_INPUT_CONTROLLER

void biosal_input_controller_init(struct thorium_actor *actor)
{
    struct biosal_input_controller *concrete_actor;

    concrete_actor = (struct biosal_input_controller *)thorium_actor_concrete_actor(actor);

    core_map_init(&concrete_actor->mega_blocks, sizeof(int), sizeof(struct core_vector));
    core_map_init(&concrete_actor->assigned_blocks, sizeof(int), sizeof(int));
    core_vector_init(&concrete_actor->mega_block_vector, sizeof(struct biosal_mega_block));

    core_vector_init(&concrete_actor->counting_streams, sizeof(int));
    core_vector_init(&concrete_actor->reading_streams, sizeof(int));
    core_vector_init(&concrete_actor->partition_commands, sizeof(int));
    core_vector_init(&concrete_actor->stream_consumers, sizeof(int));
    core_vector_init(&concrete_actor->consumer_active_requests, sizeof(int));
    core_vector_init(&concrete_actor->files, sizeof(char *));
    core_vector_init(&concrete_actor->spawners, sizeof(int));
    core_vector_init(&concrete_actor->counts, sizeof(int64_t));
    core_vector_init(&concrete_actor->consumers, sizeof(int));
    core_vector_init(&concrete_actor->stores_per_spawner, sizeof(int));

    core_timer_init(&concrete_actor->input_timer);
    core_timer_init(&concrete_actor->counting_timer);
    core_timer_init(&concrete_actor->distribution_timer);

    biosal_dna_codec_init(&concrete_actor->codec);

    if (biosal_dna_codec_must_use_two_bit_encoding(&concrete_actor->codec,
                            thorium_actor_get_node_count(actor))) {
        biosal_dna_codec_enable_two_bit_encoding(&concrete_actor->codec);
    }

    core_queue_init(&concrete_actor->unprepared_spawners, sizeof(int));

    concrete_actor->opened_streams = 0;
    concrete_actor->state = BIOSAL_INPUT_CONTROLLER_STATE_NONE;

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_10355
    printf("DEBUG actor %d register ACTION_INPUT_CONTROLLER_CREATE_STORES\n",
                    thorium_actor_name(actor));
#endif

    thorium_actor_add_action(actor, ACTION_INPUT_CONTROLLER_CREATE_STORES,
                    biosal_input_controller_create_stores);
    thorium_actor_add_action(actor, ACTION_GET_NODE_NAME_REPLY,
                    biosal_input_controller_get_node_name_reply);
    thorium_actor_add_action(actor, ACTION_GET_NODE_WORKER_COUNT_REPLY,
                    biosal_input_controller_get_node_worker_count_reply);

    thorium_actor_add_action(actor, ACTION_INPUT_CONTROLLER_PREPARE_SPAWNERS,
                    biosal_input_controller_prepare_spawners);
    thorium_actor_add_action(actor, ACTION_INPUT_CONTROLLER_SPAWN_READING_STREAMS,
                    biosal_input_controller_spawn_streams);

    thorium_actor_add_action(actor, ACTION_INPUT_STREAM_SET_START_OFFSET_REPLY,
                    biosal_input_controller_set_offset_reply);
    thorium_actor_add_script(actor, SCRIPT_INPUT_STREAM, &biosal_input_stream_script);
    thorium_actor_add_script(actor, SCRIPT_SEQUENCE_STORE, &biosal_sequence_store_script);
    thorium_actor_add_script(actor, SCRIPT_SEQUENCE_PARTITIONER,
                    &biosal_sequence_partitioner_script);

    /* configuration for the input controller
     * other values for block size: 512, 1024, 2048, 4096, 8192 * /
     */
    concrete_actor->block_size = 4096;
    concrete_actor->stores_per_worker_per_spawner = 0;

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG %d init controller\n",
                    thorium_actor_name(actor));
#endif

    concrete_actor->ready_spawners = 0;
    concrete_actor->ready_consumers = 0;
    concrete_actor->partitioner = THORIUM_ACTOR_NOBODY;
    concrete_actor->filled_consumers = 0;

    concrete_actor->counted = 0;
}

void biosal_input_controller_destroy(struct thorium_actor *actor)
{
    struct biosal_input_controller *concrete_actor;
    int i;
    char *pointer;
    struct core_map_iterator iterator;
    struct core_vector *vector;

    concrete_actor = (struct biosal_input_controller *)thorium_actor_concrete_actor(actor);

    core_timer_destroy(&concrete_actor->input_timer);
    core_timer_destroy(&concrete_actor->counting_timer);
    core_timer_destroy(&concrete_actor->distribution_timer);

    biosal_dna_codec_destroy(&concrete_actor->codec);

    for (i = 0; i < core_vector_size(&concrete_actor->files); i++) {
        pointer = *(char **)core_vector_at(&concrete_actor->files, i);
        core_memory_free(pointer, MEMORY_CONTROLLER);
    }

    core_vector_destroy(&concrete_actor->mega_block_vector);
    core_vector_destroy(&concrete_actor->counting_streams);
    core_vector_destroy(&concrete_actor->reading_streams);
    core_vector_destroy(&concrete_actor->partition_commands);
    core_vector_destroy(&concrete_actor->consumer_active_requests);
    core_vector_destroy(&concrete_actor->stream_consumers);
    core_vector_destroy(&concrete_actor->files);
    core_vector_destroy(&concrete_actor->spawners);
    core_vector_destroy(&concrete_actor->counts);
    core_vector_destroy(&concrete_actor->consumers);
    core_vector_destroy(&concrete_actor->stores_per_spawner);
    core_queue_destroy(&concrete_actor->unprepared_spawners);

    core_map_iterator_init(&iterator, &concrete_actor->mega_blocks);
    while (core_map_iterator_has_next(&iterator)) {
        core_map_iterator_next(&iterator, NULL, (void **)&vector);

        core_vector_destroy(vector);
    }
    core_map_iterator_destroy(&iterator);
    core_map_destroy(&concrete_actor->mega_blocks);
    core_map_destroy(&concrete_actor->assigned_blocks);
}

void biosal_input_controller_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int count;
    char *file;
    void *buffer;
    struct biosal_input_controller *controller;
    struct biosal_input_controller *concrete_actor;
    int destination;
    int script;
    int stream;
    char *local_file;
    int i;
    int name;
    int source;
    int destination_index;
    struct thorium_message new_message;
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
    struct core_vector mega_blocks;
    struct core_vector_iterator vector_iterator;
    struct biosal_mega_block *mega_block;
    struct core_vector *vector_bucket;
    struct core_vector block_counts;
    uint64_t block_entries;
    int mega_block_index;
    uint64_t offset;
    struct biosal_mega_block *block;
    int acquaintance_index;

    if (thorium_actor_take_action(actor, message)) {
        return;
    }

    thorium_message_get_all(message, &tag, &count, &buffer, &source);

    name = thorium_actor_name(actor);
    controller = (struct biosal_input_controller *)thorium_actor_concrete_actor(actor);
    concrete_actor = controller;

    if (tag == ACTION_START) {

        core_vector_init(&concrete_actor->spawners, 0);
        core_vector_unpack(&concrete_actor->spawners, buffer);

        core_vector_resize(&concrete_actor->stores_per_spawner,
                        core_vector_size(&concrete_actor->spawners));

        for (i = 0; i < core_vector_size(&concrete_actor->spawners); i++) {
            int_bucket = (int *)core_vector_at(&concrete_actor->stores_per_spawner, i);
            *int_bucket = 0;

            spawner = core_vector_at_as_int(&concrete_actor->spawners, i);

            core_queue_enqueue(&concrete_actor->unprepared_spawners, &spawner);
        }

        concrete_actor->state = BIOSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS;

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_LEVEL
        printf("DEBUG preparing first spawner\n");
#endif

        thorium_actor_send_to_self_empty(actor, ACTION_INPUT_CONTROLLER_PREPARE_SPAWNERS);

        /*
        thorium_dispatcher_print(thorium_actor_dispatcher(actor));
        */

    } else if (tag == ACTION_ADD_FILE) {

        file = (char *)buffer;

        local_file = core_memory_allocate(strlen(file) + 1, MEMORY_CONTROLLER);
        strcpy(local_file, file);

        printf("controller %d ACTION_ADD_FILE %s\n",
                        thorium_actor_name(actor),
                        local_file);

        core_vector_push_back(&concrete_actor->files, &local_file);

        bucket = core_vector_at(&concrete_actor->files, core_vector_size(&concrete_actor->files) - 1);
        local_file = *(char **)bucket;

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG11 ACTION_ADD_FILE %s %p bucket %p index %d\n",
                        local_file, local_file, (void *)bucket, core_vector_size(&concrete_actor->files) - 1);
#endif

        thorium_actor_send_reply_empty(actor, ACTION_ADD_FILE_REPLY);

    } else if (tag == ACTION_SPAWN_REPLY) {

        if (concrete_actor->state == BIOSAL_INPUT_CONTROLLER_STATE_SPAWN_STORES) {

            biosal_input_controller_add_store(actor, message);
            return;

        } else if (concrete_actor->state == BIOSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS) {

            concrete_actor->ready_spawners++;
            thorium_message_unpack_int(message, 0, &name);
            thorium_actor_send_empty(actor, name, ACTION_ASK_TO_STOP);
            thorium_actor_send_to_self_empty(actor, ACTION_INPUT_CONTROLLER_PREPARE_SPAWNERS);

            if (concrete_actor->ready_spawners == (int)core_vector_size(&concrete_actor->spawners)) {

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
                printf("DEBUG all spawners are prepared\n");
#endif
                thorium_actor_send_to_supervisor_empty(actor, ACTION_START_REPLY);
            }

            return;

        } else if (concrete_actor->state == BIOSAL_INPUT_CONTROLLER_STATE_SPAWN_PARTITIONER) {

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
            printf("DEBUG received spawn reply, state is spawn_partitioner\n");
#endif

            thorium_message_unpack_int(message, 0, &concrete_actor->partitioner);

            /* configure the partitioner
             */
            destination = concrete_actor->partitioner;
            thorium_actor_send_int(actor, destination,
                            ACTION_SEQUENCE_PARTITIONER_SET_BLOCK_SIZE,
                            concrete_actor->block_size);
            thorium_actor_send_int(actor, destination,
                            ACTION_SEQUENCE_PARTITIONER_SET_ACTOR_COUNT,
                            core_vector_size(&concrete_actor->consumers));

            core_vector_init(&block_counts, sizeof(uint64_t));

            for (i = 0; i < core_vector_size(&concrete_actor->mega_block_vector); i++) {

                block = (struct biosal_mega_block *)core_vector_at(&concrete_actor->mega_block_vector, i);
                block_entries = biosal_mega_block_get_entries(block);

                core_vector_push_back_uint64_t(&block_counts, block_entries);
            }

            new_count = core_vector_pack_size(&block_counts);
            new_buffer = thorium_actor_allocate(actor, new_count);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
            printf("DEBUG packed counts, %d bytes\n", count);
#endif

            core_vector_pack(&block_counts, new_buffer);
            thorium_message_init(&new_message, ACTION_SEQUENCE_PARTITIONER_SET_ENTRY_VECTOR,
                            new_count, new_buffer);
            thorium_actor_send(actor, destination, &new_message);
            core_vector_destroy(&block_counts);

            return;

        } else if (concrete_actor->state == BIOSAL_INPUT_CONTROLLER_STATE_SPAWN_READING_STREAMS) {

            thorium_message_unpack_int(message, 0, &stream);

            stream_index = stream;

            mega_block_index = core_vector_size(&concrete_actor->reading_streams);

            core_vector_push_back_int(&concrete_actor->reading_streams, stream_index);
            core_vector_push_back_int(&concrete_actor->partition_commands, -1);
            core_vector_push_back_int(&concrete_actor->stream_consumers, -1);

            stream_index = core_vector_size(&concrete_actor->reading_streams) - 1;
            mega_block = (struct biosal_mega_block *)core_vector_at(&concrete_actor->mega_block_vector,
                            mega_block_index);

            offset = biosal_mega_block_get_offset(mega_block);

            core_map_add_value(&concrete_actor->assigned_blocks,
                            &stream_index, &mega_block_index);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
            printf("DEBUG setting offset to %" PRIu64 " for stream/%d\n",
                            offset, stream);
#endif

            thorium_actor_send_uint64_t(actor, stream, ACTION_INPUT_STREAM_SET_START_OFFSET, offset);

            return;
        }

        stream = *(int *)buffer;

        file_index = core_vector_size(&concrete_actor->counting_streams);
        local_file = *(char **)core_vector_at(&concrete_actor->files, file_index);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
        printf("DEBUG actor %d receives stream %d from spawner %d for file %s\n",
                        name, stream, source,
                        local_file);
#endif

        core_vector_push_back(&concrete_actor->counting_streams, &stream);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
        printf("asking stream/%d to open %s\n", stream, local_file);
#endif
        thorium_message_init(&new_message, ACTION_INPUT_OPEN, strlen(local_file) + 1, local_file);

#ifdef DEBUG_ISSUE_594
        thorium_message_print(&new_message);
        printf("SEND Buffer %s\n", local_file);
#endif

        thorium_actor_send(actor, stream, &new_message);
        thorium_message_destroy(&new_message);

        if (core_vector_size(&concrete_actor->counting_streams) != core_vector_size(&concrete_actor->files)) {

            thorium_actor_send_to_self_empty(actor, ACTION_INPUT_SPAWN);

        }

#ifdef DEBUG_ISSUE_594
        printf("EXIT Buffer %s\n", local_file);
#endif

    } else if (tag == ACTION_INPUT_OPEN_REPLY) {

        if (concrete_actor->state == BIOSAL_INPUT_CONTROLLER_STATE_SPAWN_READING_STREAMS) {

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
            printf("DEBUG receives open.reply for reading stream/%d\n", source);
#endif
            concrete_actor->opened_streams++;

            if (concrete_actor->opened_streams == core_vector_size(&concrete_actor->mega_block_vector)) {
                thorium_actor_send_to_self_empty(actor, ACTION_INPUT_CONTROLLER_CREATE_STORES);
            }

            return;
        }

        concrete_actor->opened_streams++;

        stream = source;
        thorium_message_unpack_int(message, 0, &error);

        if (error == BIOSAL_INPUT_ERROR_NO_ERROR) {

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
            printf("DEBUG actor %d asks %d ACTION_INPUT_COUNT_IN_PARALLEL\n", name, stream);
#endif

            thorium_actor_send_vector(actor, stream, ACTION_INPUT_COUNT_IN_PARALLEL,
                            &concrete_actor->spawners);
        } else {

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
            printf("DEBUG actor %d received error %d from %d\n", name, error, stream);
#endif
            concrete_actor->counted++;
        }

	/* if all streams failed, notice supervisor */
        if (concrete_actor->counted == core_vector_size(&concrete_actor->files)) {

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
#endif
            printf("DEBUG %d: Error all streams failed.\n",
                            thorium_actor_name(actor));
            thorium_actor_send_to_supervisor_empty(actor, ACTION_INPUT_DISTRIBUTE_REPLY);
        }

/*
        if (concrete_actor->opened_streams == core_vector_size(&concrete_actor->files)) {

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
            printf("DEBUG controller %d sends ACTION_INPUT_DISTRIBUTE_REPLY to supervisor %d [%d/%d]\n",
                            name, thorium_actor_supervisor(actor),
                            concrete_actor->opened_streams, core_vector_size(&concrete_actor->files));
#endif

        }
*/

    } else if (tag == ACTION_INPUT_COUNT_PROGRESS) {

        stream_index = core_vector_index_of(&concrete_actor->counting_streams, &source);
        local_file = core_vector_at_as_char_pointer(&concrete_actor->files, stream_index);
        thorium_message_unpack_int64_t(message, 0, &entries);

        bucket = (int64_t *)core_vector_at(&concrete_actor->counts, stream_index);

        printf("controller/%d receives progress from stream/%d file %s %" PRIu64 " entries so far\n",
                        name, source, local_file, entries);
        *bucket = entries;

    } else if (tag == ACTION_INPUT_COUNT_IN_PARALLEL_REPLY) {

        stream_index = core_vector_index_of(&concrete_actor->counting_streams, &source);
        local_file = core_vector_at_as_char_pointer(&concrete_actor->files, stream_index);

        core_vector_init(&mega_blocks, 0);
        core_vector_unpack(&mega_blocks, buffer);

        printf("DEBUG receive mega blocks from %d\n", source);
        /*
         * Update the file index for every mega block.
         */
        core_vector_iterator_init(&vector_iterator, &mega_blocks);

        bucket = (int64_t*)core_vector_at(&concrete_actor->counts, stream_index);
        (*bucket) = 0;

        while (core_vector_iterator_has_next(&vector_iterator)) {
            core_vector_iterator_next(&vector_iterator, (void **)&mega_block);

            printf("SETTING setting file to %d for mega block\n", stream_index);
            biosal_mega_block_set_file(mega_block, stream_index);

            entries = biosal_mega_block_get_entries_from_start(mega_block);

            printf("Cataloging %d ENTRIES\n", (int)entries);

            (*bucket) = entries;

            biosal_mega_block_print(mega_block);
        }

        core_vector_iterator_destroy(&vector_iterator);

        vector_bucket = (struct core_vector *)core_map_add(&concrete_actor->mega_blocks, &stream_index);
        core_vector_init_copy(vector_bucket, &mega_blocks);

        core_vector_destroy(&mega_blocks);

        concrete_actor->counted++;

        printf("controller/%d received from stream/%d for file %s %" PRIu64 " entries (final) %d/%d\n",
                        name, source, local_file, entries,
                        concrete_actor->counted, (int)core_vector_size(&concrete_actor->files));

        thorium_actor_send_reply_empty(actor, ACTION_INPUT_CLOSE);

        /* continue work here, tell supervisor about it */
        if (concrete_actor->counted == core_vector_size(&concrete_actor->files)) {
            thorium_actor_send_to_self_empty(actor, ACTION_INPUT_CONTROLLER_SPAWN_READING_STREAMS);
        }


    } else if (tag == ACTION_INPUT_DISTRIBUTE) {

        core_timer_start(&concrete_actor->input_timer);
        core_timer_start(&concrete_actor->counting_timer);

        /* for each file, spawn a stream to count */

        /* no files, return immediately
         */
        if (core_vector_size(&concrete_actor->files) == 0) {

            printf("Error: no file to distribute...\n");
            thorium_actor_send_reply_empty(actor, ACTION_INPUT_DISTRIBUTE_REPLY);
            return;
        }

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG actor %d receives ACTION_INPUT_DISTRIBUTE\n", name);
#endif

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG send ACTION_INPUT_SPAWN to self\n");
#endif

        thorium_actor_send_to_self_empty(actor, ACTION_INPUT_SPAWN);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG resizing counts to %d\n", core_vector_size(&concrete_actor->files));
#endif

        core_vector_resize(&concrete_actor->counts, core_vector_size(&concrete_actor->files));

        for (i = 0; i < core_vector_size(&concrete_actor->counts); i++) {
            bucket = (int64_t*)core_vector_at(&concrete_actor->counts, i);
            *bucket = 0;
        }

    } else if (tag == ACTION_INPUT_SPAWN && source == name) {

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG ACTION_INPUT_SPAWN\n");
#endif

        script = SCRIPT_INPUT_STREAM;

        concrete_actor->state = BIOSAL_INPUT_CONTROLLER_STATE_SPAWN_STREAMS;

        /* the next file name to send is the current number of streams */
        i = core_vector_size(&concrete_actor->counting_streams);

        destination_index = i % core_vector_size(&concrete_actor->spawners);
        destination = *(int *)core_vector_at(&concrete_actor->spawners, destination_index);

        thorium_message_init(message, ACTION_SPAWN, sizeof(script), &script);
        thorium_actor_send(actor, destination, message);

        bucket = core_vector_at(&concrete_actor->files, i);
        local_file = *(char **)core_vector_at(&concrete_actor->files, i);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG890 local_file %p bucket %p index %d\n", local_file, (void *)bucket,
                        i);
#endif

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG actor %d spawns a stream for file %d/%d via spawner %d\n",
                        name, i, core_vector_size(&concrete_actor->files), destination);
#endif

        /* also, spawn 4 stores on each node */

    } else if (tag == ACTION_ASK_TO_STOP && ( source == thorium_actor_supervisor(actor)
                            || source == thorium_actor_name(actor))) {

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
#endif

        /* stop streams
         */
        for (i = 0; i < core_vector_size(&concrete_actor->counting_streams); i++) {
            stream = *(int *)core_vector_at(&concrete_actor->counting_streams, i);

            thorium_actor_send_empty(actor, stream, ACTION_ASK_TO_STOP);
        }
        for (i = 0; i < core_vector_size(&concrete_actor->reading_streams); i++) {
            stream = *(int *)core_vector_at(&concrete_actor->reading_streams, i);

            thorium_actor_send_empty(actor, stream, ACTION_ASK_TO_STOP);
        }



#if 0
        /* stop data stores
         */
        for (i = 0; i < core_vector_size(&concrete_actor->consumers); i++) {
            store = core_vector_at_as_int(&concrete_actor->consumers, i);

            thorium_actor_send_empty(actor, store, ACTION_ASK_TO_STOP);
        }
#endif
        /* stop partitioner
         */

        if (concrete_actor->partitioner != THORIUM_ACTOR_NOBODY) {
            thorium_actor_send_empty(actor,
                                concrete_actor->partitioner,
                        ACTION_ASK_TO_STOP);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
            printf("DEBUG controller %d sends ACTION_ASK_TO_STOP_REPLY to %d\n",
                        thorium_actor_name(actor),
                        thorium_message_source(message));
#endif

        }

        thorium_actor_send_reply_empty(actor, ACTION_ASK_TO_STOP_REPLY);

        /* stop self
         */
        thorium_actor_send_to_self_empty(actor, ACTION_STOP);

        thorium_actor_ask_to_stop(actor, message);

        printf("DEBUG controller %d dies\n", name);
#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
#endif

    } else if (tag == ACTION_INPUT_CONTROLLER_CREATE_PARTITION && source == name) {

        spawner = *(int *)core_vector_at(&concrete_actor->spawners,
                        core_vector_size(&concrete_actor->spawners) / 2);

        thorium_actor_send_int(actor, spawner, ACTION_SPAWN,
                        SCRIPT_SEQUENCE_PARTITIONER);
        concrete_actor->state = BIOSAL_INPUT_CONTROLLER_STATE_SPAWN_PARTITIONER;

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG input controller %d spawns a partitioner via spawner %d\n",
                        name,  spawner);
#endif

    } else if (tag == ACTION_SEQUENCE_PARTITIONER_COMMAND_IS_READY) {

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG controller receives ACTION_SEQUENCE_PARTITIONER_COMMAND_IS_READY, asks for command\n");
#endif

        thorium_actor_send_reply_empty(actor, ACTION_SEQUENCE_PARTITIONER_GET_COMMAND);

    } else if (tag == ACTION_SEQUENCE_PARTITIONER_GET_COMMAND_REPLY) {

        biosal_input_controller_receive_command(actor, message);

    } else if (tag == ACTION_SEQUENCE_PARTITIONER_FINISHED) {

        thorium_actor_send_empty(actor,
                                concrete_actor->partitioner,
                        ACTION_ASK_TO_STOP);

        biosal_input_controller_verify_requests(actor, message);

    } else if (tag == ACTION_SEQUENCE_PARTITIONER_PROVIDE_STORE_ENTRY_COUNTS) {

        biosal_input_controller_receive_store_entry_counts(actor, message);

    } else if (tag == ACTION_RESERVE_REPLY) {

        concrete_actor->ready_consumers++;

        printf("DEBUG marker ACTION_RESERVE_REPLY %d/%d\n",
                        concrete_actor->ready_consumers,
                        (int)core_vector_size(&concrete_actor->consumers));

        if (concrete_actor->ready_consumers == core_vector_size(&concrete_actor->consumers)) {

            concrete_actor->ready_consumers = 0;
            printf("DEBUG all consumers are ready\n");
            thorium_actor_send_empty(actor,
                            concrete_actor->partitioner,
                            ACTION_SEQUENCE_PARTITIONER_PROVIDE_STORE_ENTRY_COUNTS_REPLY);
        }

    } else if (tag == ACTION_INPUT_PUSH_SEQUENCES_READY) {

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG biosal_input_controller_receive received ACTION_INPUT_PUSH_SEQUENCES_READY\n");
#endif

        stream_name = source;

        acquaintance_index = stream_name;
        stream_index = core_vector_index_of(&concrete_actor->reading_streams, &acquaintance_index);
        command_name = *(int *)core_vector_at(&concrete_actor->partition_commands,
                        stream_index);

        thorium_actor_send_int(actor,
                                concrete_actor->partitioner,
                        ACTION_SEQUENCE_PARTITIONER_GET_COMMAND_REPLY_REPLY,
                        command_name);

    } else if (tag == ACTION_INPUT_PUSH_SEQUENCES_REPLY) {

        stream_name = source;

        thorium_message_unpack_int(message, 0, &consumer);

        consumer_index = core_vector_index_of(&concrete_actor->consumers,
                        &consumer);

        bucket_for_requests = (int *)core_vector_at(&concrete_actor->consumer_active_requests, consumer_index);

        (*bucket_for_requests)--;

        biosal_input_controller_verify_requests(actor, message);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_CONSUMERS
        printf("DEBUG consumer # %d has %d active requests\n",
                        consumer_index, *bucket_for_requests);
#endif


    } else if (tag == ACTION_SET_CONSUMERS) {

        core_vector_init(&concrete_actor->consumers, 0);
        core_vector_unpack(&concrete_actor->consumers, buffer);

        printf("controller %d receives %d consumers\n",
                        thorium_actor_name(actor),
                        (int)core_vector_size(&concrete_actor->consumers));

        for (i = 0; i < core_vector_size(&concrete_actor->consumers); i++) {
            core_vector_push_back_int(&concrete_actor->consumer_active_requests, 0);
        }

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
        core_vector_print_int(&concrete_actor->consumers);
        printf("\n");
#endif
        thorium_actor_send_reply_empty(actor, ACTION_SET_CONSUMERS_REPLY);

    } else if (tag == ACTION_SET_BLOCK_SIZE) {

        thorium_message_unpack_int(message, 0, &concrete_actor->block_size);
        thorium_actor_send_reply_empty(actor, ACTION_SET_BLOCK_SIZE_REPLY);

    } else if (tag == ACTION_SEQUENCE_STORE_READY) {

        concrete_actor->filled_consumers++;

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG ACTION_SEQUENCE_STORE_READY %d/%d\n", concrete_actor->filled_consumers,
                        (int)core_vector_size(&concrete_actor->consumers));
#endif

        if (concrete_actor->filled_consumers == core_vector_size(&concrete_actor->consumers)) {
            concrete_actor->filled_consumers = 0;

            printf("DEBUG: all consumers are filled,  sending ACTION_INPUT_DISTRIBUTE_REPLY\n");

            core_timer_stop(&concrete_actor->input_timer);
            core_timer_stop(&concrete_actor->distribution_timer);

            core_timer_print_with_description(&concrete_actor->distribution_timer,
                            "Load input / Distribute input data");

            core_timer_print_with_description(&concrete_actor->input_timer,
                            "Load input");

            thorium_actor_send_to_supervisor_empty(actor, ACTION_INPUT_DISTRIBUTE_REPLY);
        }
    }
}

void biosal_input_controller_receive_store_entry_counts(struct thorium_actor *actor, struct thorium_message *message)
{
    struct biosal_input_controller *concrete_actor;
    struct core_vector store_entries;
    void *buffer;
    int i;
    int store;
    uint64_t entries;
    struct thorium_message new_message;
    int name;

    core_vector_init(&store_entries, sizeof(uint64_t));

    concrete_actor = (struct biosal_input_controller *)thorium_actor_concrete_actor(actor);
    buffer = thorium_message_buffer(message);
    name = thorium_actor_name(actor);
    concrete_actor->ready_consumers = 0;

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG biosal_input_controller_receive_store_entry_counts unpacking entries\n");
#endif

    core_vector_init(&store_entries, 0);
    core_vector_unpack(&store_entries, buffer);

    for (i = 0; i < core_vector_size(&store_entries); i++) {
        store = *(int *)core_vector_at(&concrete_actor->consumers, i);
        entries = *(uint64_t *)core_vector_at(&store_entries, i);

        printf("DEBUG controller/%d tells consumer/%d to reserve %" PRIu64 " buckets\n",
                        name, store, entries);

        thorium_message_init(&new_message, ACTION_RESERVE,
                        sizeof(entries), &entries);
        thorium_actor_send(actor, store, &new_message);
    }

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG biosal_input_controller_receive_store_entry_counts will wait for replies\n");
#endif

    core_vector_destroy(&store_entries);
}

void biosal_input_controller_create_stores(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int source;
    void *buffer;
    int count;
    int i;
    struct biosal_input_controller *concrete_actor;
    int value;
    int spawner;
    uint64_t total;
    int block_size;
    int blocks;
    uint64_t entries;
    char *local_file;
    int name;

    concrete_actor = (struct biosal_input_controller *)thorium_actor_concrete_actor(actor);

    thorium_message_get_all(message, &tag, &count, &buffer, &source);
/*
    printf("DEBUG biosal_input_controller_create_stores\n");
    */

    for (i = 0; i < core_vector_size(&concrete_actor->stores_per_spawner); i++) {
        value = core_vector_at_as_int(&concrete_actor->stores_per_spawner, i);

        if (value == -1) {

                /*
            printf("DEBUG need more information about spawner at %i\n",
                            i);
                            */

            spawner = core_vector_at_as_int(&concrete_actor->spawners, i);

            thorium_actor_send_empty(actor, spawner, ACTION_GET_NODE_NAME);
            return;
        }
    }

    concrete_actor->state = BIOSAL_INPUT_CONTROLLER_STATE_SPAWN_STORES;

    /* at this point, we know the worker count of every node corresponding
     * to each spawner
     */

    for (i = 0; i < core_vector_size(&concrete_actor->stores_per_spawner); i++) {
            /*
        printf("DEBUG polling spawner %i/%d\n", i,
                        core_vector_size(&concrete_actor->stores_per_spawner));
*/
        value = core_vector_at_as_int(&concrete_actor->stores_per_spawner, i);

        if (value != 0) {

            spawner = core_vector_at_as_int(&concrete_actor->spawners, i);
/*
            printf("DEBUG spawner %d is %d\n", i, spawner);
*/
            thorium_actor_send_int(actor, spawner, ACTION_SPAWN, SCRIPT_SEQUENCE_STORE);

            return;
        }
/*
        printf("DEBUG spawner %i spawned all its stores\n", i);
        */
    }

    printf("DEBUG controller %d: consumers are ready (%d)\n",
                    thorium_actor_name(actor),
                    (int)core_vector_size(&concrete_actor->consumers));

    for (i = 0; i < core_vector_size(&concrete_actor->consumers); i++) {
        value = core_vector_at_as_int(&concrete_actor->consumers, i);

        printf("DEBUG controller %d: consumer %i is %d\n",
                        thorium_actor_name(actor), i, value);
    }

    printf("DEBUG controller %d: streams are\n",
                    thorium_actor_name(actor));

    total = 0;
    block_size = concrete_actor->block_size;

    for (i = 0; i < core_vector_size(&concrete_actor->files); i++) {
        entries = *(uint64_t*)core_vector_at(&concrete_actor->counts, i);
        local_file = core_vector_at_as_char_pointer(&concrete_actor->files, i);
        name = *(int *)core_vector_at(&concrete_actor->counting_streams, i);

        printf("stream %d, %d/%d %s %" PRIu64 "\n",
                        name, i,
                        (int)core_vector_size(&concrete_actor->files),
                        local_file,
                        entries);
        total += entries;
    }

    blocks = total / block_size;

    if (total % block_size != 0) {
        blocks++;
    }

    core_timer_stop(&concrete_actor->counting_timer);
    core_timer_start(&concrete_actor->distribution_timer);

    core_timer_print_with_description(&concrete_actor->counting_timer,
                    "Load input / Count input data");

    printf("DEBUG controller %d: Partition Total: %" PRIu64 ", block_size: %d, blocks: %d\n",
                    thorium_actor_name(actor),
                    total, block_size, blocks);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_10355
    printf("DEBUG send ACTION_INPUT_CONTROLLER_CREATE_STORES to self %d\n",
                            thorium_actor_name(actor));
#endif

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG biosal_input_controller_create_stores send ACTION_INPUT_CONTROLLER_CREATE_PARTITION\n");
#endif

    /* no sequences at all !
     */
    if (total == 0) {
        printf("Error, total is 0, can not distribute\n");
        thorium_actor_send_to_supervisor_empty(actor, ACTION_INPUT_DISTRIBUTE_REPLY);
        return;
    } else {
        thorium_actor_send_to_self_empty(actor, ACTION_INPUT_CONTROLLER_CREATE_PARTITION);
    }

    /*
    thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    */
}

void biosal_input_controller_get_node_name_reply(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int source;
    void *buffer;
    int count;
    int spawner;
    int node;

    thorium_message_get_all(message, &tag, &count, &buffer, &source);
    spawner = source;
    thorium_message_unpack_int(message, 0, &node);

    printf("DEBUG spawner %d is on node node/%d\n", spawner, node);

    thorium_actor_send_reply_empty(actor, ACTION_GET_NODE_WORKER_COUNT);
}

void biosal_input_controller_get_node_worker_count_reply(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int source;
    void *buffer;
    int count;
    int index;
    struct biosal_input_controller *concrete_actor;
    int spawner;
    int worker_count;
    int *bucket;

    concrete_actor = (struct biosal_input_controller *)thorium_actor_concrete_actor(actor);

    thorium_message_get_all(message, &tag, &count, &buffer, &source);
    spawner = source;
    thorium_message_unpack_int(message, 0, &worker_count);

    index = core_vector_index_of(&concrete_actor->spawners, &spawner);
    bucket = core_vector_at(&concrete_actor->stores_per_spawner, index);
    *bucket = worker_count * concrete_actor->stores_per_worker_per_spawner;

    printf("DEBUG spawner %d (node %d) is on a node that has %d workers\n", spawner,
                    index, worker_count);

    thorium_actor_send_to_self_empty(actor, ACTION_INPUT_CONTROLLER_CREATE_STORES);
}

void biosal_input_controller_add_store(struct thorium_actor *actor, struct thorium_message *message)
{
    int source;
    int store;
    int index;
    struct biosal_input_controller *concrete_actor;
    int *bucket;

    /*
    printf("DEBUG biosal_input_controller_add_store\n");
    */

    concrete_actor = (struct biosal_input_controller *)thorium_actor_concrete_actor(actor);
    source = thorium_message_source(message);
    thorium_message_unpack_int(message, 0, &store);

    index = core_vector_index_of(&concrete_actor->spawners, &source);

    /*
    printf("DEBUG biosal_input_controller_add_store index %d\n", index);
    */

    bucket = core_vector_at(&concrete_actor->stores_per_spawner, index);

    /* the content of the bucket is initially the total number of
     * stores that are desired for this spawner.
     */
    *bucket = (*bucket - 1);
    core_vector_push_back(&concrete_actor->consumers, &store);

    thorium_actor_send_to_self_empty(actor, ACTION_INPUT_CONTROLLER_CREATE_STORES);

    /*
    printf("DEBUG remaining to spawn: %d (before returning)\n", *bucket);
    */
}

void biosal_input_controller_prepare_spawners(struct thorium_actor *actor, struct thorium_message *message)
{
    int spawner;
    struct biosal_input_controller *concrete_actor;

    concrete_actor = (struct biosal_input_controller *)thorium_actor_concrete_actor(actor);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG biosal_input_controller_prepare_spawners \n");
#endif

    /* spawn an actor of the same script on every spawner to load required
     * scripts on all nodes
     */
    if (core_queue_dequeue(&concrete_actor->unprepared_spawners, &spawner)) {

        thorium_actor_send_int(actor, spawner, ACTION_SPAWN, thorium_actor_script(actor));
        concrete_actor->state = BIOSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS;
    }
}

void biosal_input_controller_receive_command(struct thorium_actor *actor, struct thorium_message *message)
{
    struct biosal_partition_command command;
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
    struct biosal_input_command input_command;
    void *new_buffer;
    struct thorium_message new_message;
    struct biosal_input_controller *concrete_actor;
    int *bucket;
    int *bucket_for_consumer;
    int consumer_index;
    struct core_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    concrete_actor = (struct biosal_input_controller *)thorium_actor_concrete_actor(actor);
    buffer = thorium_message_buffer(message);
    biosal_partition_command_unpack(&command, buffer);
    stream_index = biosal_partition_command_stream_index(&command);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
    printf("DEBUG biosal_input_controller_receive_command controller receives command for stream %d\n", stream_index);
    biosal_partition_command_print(&command);
#endif

    store_index = biosal_partition_command_store_index(&command);
    bucket_for_command_name = (int *)core_vector_at(&concrete_actor->partition_commands,
                    stream_index);
    bucket_for_consumer = (int *)core_vector_at(&concrete_actor->stream_consumers,
                    stream_index);

    stream_name = core_vector_at_as_int(&concrete_actor->reading_streams,
                    stream_index);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
    printf("DEBUG stream_index %d stream_name %d\n", stream_index, stream_name);
#endif

    store_name = *(int *)core_vector_at(&concrete_actor->consumers, store_index);
    store_first = biosal_partition_command_store_first(&command);
    store_last = biosal_partition_command_store_last(&command);

    biosal_input_command_init(&input_command, store_name, store_first, store_last,
                    ephemeral_memory);

    bytes = biosal_input_command_pack_size(&input_command,
                    &concrete_actor->codec);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
    printf("DEBUG input command\n");
    biosal_input_command_print(&input_command);

    printf("DEBUG biosal_input_controller_receive_command bytes %d\n",
                    bytes);
#endif

    new_buffer = thorium_actor_allocate(actor, bytes);
    biosal_input_command_pack(&input_command, new_buffer,
                    &concrete_actor->codec);

    thorium_message_init(&new_message, ACTION_INPUT_PUSH_SEQUENCES, bytes,
                    new_buffer);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
    printf("DEBUG biosal_input_controller_receive_command sending ACTION_INPUT_PUSH_SEQUENCES to %d (index %d)\n",
                    stream_name, stream_index);
    biosal_input_command_print(&input_command);

    printf("SENDING COMMAND TO stream/%d\n", stream_name);
#endif

    thorium_actor_send(actor, stream_name, &new_message);

    command_name = biosal_partition_command_name(&command);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
    printf("controller/%d processed input command %d %p\n", thorium_actor_name(actor), command_name,
                    (void *)bucket_for_command_name);
#endif

    *bucket_for_command_name = command_name;

    consumer_index = store_index;
    *bucket_for_consumer = consumer_index;

    bucket = (int *)core_vector_at(&concrete_actor->consumer_active_requests, consumer_index);

    (*bucket)++;

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_CONSUMERS
    printf("DEBUG consumer # %d has %d active requests\n",
                        consumer_index, *bucket);
#endif
}

void biosal_input_controller_spawn_streams(struct thorium_actor *actor, struct thorium_message *message)
{
    int spawner;
    struct biosal_input_controller *concrete_actor;
    struct core_vector_iterator iterator;
    int i;
    int j;
    int block_index;
    struct core_vector *vector;
    struct biosal_mega_block *block;

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
    printf("DEBUG biosal_input_controller_spawn_streams\n");
#endif

    concrete_actor = (struct biosal_input_controller *)thorium_actor_concrete_actor(actor);
    concrete_actor->opened_streams = 0;
    concrete_actor->spawner = 0;

    /* gather mega blocks
     */

    block_index = 0;
    printf("DEBUG received MEGA BLOCKS\n");
    for (i = 0; i < core_vector_size(&concrete_actor->files); i++) {

        vector = (struct core_vector *)core_map_get(&concrete_actor->mega_blocks, &i);

        if (vector == NULL) {
            continue;
        }

        for (j = 0; j < core_vector_size(vector); j++) {
            block = (struct biosal_mega_block *)core_vector_at(vector, j);

            printf("BLOCK # %d ", block_index);
            block_index++;
            biosal_mega_block_print(block);

            core_vector_push_back(&concrete_actor->mega_block_vector, block);
        }
    }
    printf("DEBUG MEGA BLOCKS (total: %d)\n", block_index);

    core_vector_iterator_init(&iterator, &concrete_actor->mega_block_vector);

    while (core_vector_iterator_has_next(&iterator)) {
        core_vector_iterator_next(&iterator, NULL);
        spawner = core_vector_at_as_int(&concrete_actor->spawners, concrete_actor->spawner);

        concrete_actor->spawner++;
        concrete_actor->spawner %= core_vector_size(&concrete_actor->spawners);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
        printf("DEBUG asking %d to spawn script %d\n", spawner, SCRIPT_INPUT_STREAM);
#endif

        thorium_actor_send_int(actor, spawner, ACTION_SPAWN, SCRIPT_INPUT_STREAM);
    }

    core_vector_iterator_destroy(&iterator);

    concrete_actor->state = BIOSAL_INPUT_CONTROLLER_STATE_SPAWN_READING_STREAMS;
}

void biosal_input_controller_set_offset_reply(struct thorium_actor *self, struct thorium_message *message)
{
    int stream_index;
    int acquaintance_index;
    int source;
    int block_index;
    struct biosal_mega_block *block;
    struct biosal_input_controller *concrete_actor;
    int file_index;
    char *file_name;
    struct thorium_message new_message;

    source = thorium_message_source(message);
    acquaintance_index = source;
    concrete_actor = (struct biosal_input_controller *)thorium_actor_concrete_actor(self);
    stream_index = core_vector_index_of(&concrete_actor->reading_streams, &acquaintance_index);

    block_index = core_map_get_int(&concrete_actor->assigned_blocks, &stream_index);

#ifdef BIOSAL_INPUT_CONTROLLER_DEBUG_READING_STREAMS
    printf("DEBUG got reply from stream/%d for offset, stream_index %d block_index %d\n", source,
                    stream_index, block_index);
#endif

    block = (struct biosal_mega_block *)core_vector_at(&concrete_actor->mega_block_vector,
                    block_index);

    file_index = biosal_mega_block_get_file(block);
    file_name = *(char **)core_vector_at(&concrete_actor->files, file_index);

    printf("DEBUG send ACTION_INPUT_OPEN %s\n", file_name);

    thorium_message_init(&new_message, ACTION_INPUT_OPEN, strlen(file_name) + 1, file_name);

    thorium_actor_send_reply(self, &new_message);

    thorium_message_destroy(&new_message);
}

void biosal_input_controller_verify_requests(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_input_controller *concrete_actor;
    int i;
    int active;

    active = 0;
    concrete_actor = (struct biosal_input_controller *)thorium_actor_concrete_actor(self);

    for (i = 0; i < core_vector_size(&concrete_actor->consumer_active_requests); i++) {
        if (core_vector_at_as_int(&concrete_actor->consumer_active_requests, i) != 0) {
            active++;
        }
    }

    if (active == 0) {
    }
}
