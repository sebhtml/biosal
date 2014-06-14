
#include "input_controller.h"

#include "input_stream.h"

#include <storage/sequence_store.h>
#include <storage/sequence_partitioner.h>
#include <storage/partition_command.h>
#include <input/input_command.h>

#include <helpers/vector_helper.h>
#include <helpers/actor_helper.h>
#include <helpers/message_helper.h>

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
*/

/* states of this actor
 */
#define BSAL_INPUT_CONTROLLER_STATE_NONE 0
#define BSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS 1
#define BSAL_INPUT_CONTROLLER_STATE_SPAWN_STREAMS 2
#define BSAL_INPUT_CONTROLLER_STATE_SPAWN_STORES 3
#define BSAL_INPUT_CONTROLLER_STATE_SPAWN_PARTITIONER 4

struct bsal_script bsal_input_controller_script = {
    .name = BSAL_INPUT_CONTROLLER_SCRIPT,
    .init = bsal_input_controller_init,
    .destroy = bsal_input_controller_destroy,
    .receive = bsal_input_controller_receive,
    .size = sizeof(struct bsal_input_controller)
};

void bsal_input_controller_init(struct bsal_actor *actor)
{
    struct bsal_input_controller *controller;

    controller = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);

    bsal_vector_init(&controller->streams, sizeof(int));
    bsal_vector_init(&controller->partition_commands, sizeof(int));
    bsal_vector_init(&controller->files, sizeof(char *));
    bsal_vector_init(&controller->spawners, sizeof(int));
    bsal_vector_init(&controller->counts, sizeof(uint64_t));
    bsal_vector_init(&controller->stores, sizeof(int));
    bsal_vector_init(&controller->stores_per_spawner, sizeof(int));

    bsal_queue_init(&controller->unprepared_spawners, sizeof(int));

    controller->opened_streams = 0;
    controller->state = BSAL_INPUT_CONTROLLER_STATE_NONE;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_10355
    printf("DEBUG actor %d register BSAL_INPUT_CONTROLLER_CREATE_STORES\n",
                    bsal_actor_name(actor));
#endif

    bsal_actor_register(actor, BSAL_INPUT_CONTROLLER_CREATE_STORES,
                    bsal_input_controller_create_stores);
    bsal_actor_register(actor, BSAL_ACTOR_GET_NODE_NAME_REPLY,
                    bsal_input_controller_get_node_name_reply);
    bsal_actor_register(actor, BSAL_ACTOR_GET_NODE_WORKER_COUNT_REPLY,
                    bsal_input_controller_get_node_worker_count_reply);

    bsal_actor_register(actor, BSAL_INPUT_CONTROLLER_PREPARE_SPAWNERS,
                    bsal_input_controller_prepare_spawners);

    bsal_actor_add_script(actor, BSAL_INPUT_STREAM_SCRIPT, &bsal_input_stream_script);
    bsal_actor_add_script(actor, BSAL_SEQUENCE_STORE_SCRIPT, &bsal_sequence_store_script);
    bsal_actor_add_script(actor, BSAL_SEQUENCE_PARTITIONER_SCRIPT,
                    &bsal_sequence_partitioner_script);

    /* configuration for the input controller
     * other values for block size: 512, 1024, 2048, 4096, 8192 * /
     */
    controller->block_size = 8192;
    controller->stores_per_worker_per_spawner = 0;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG actor/%d init controller\n",
                    bsal_actor_name(actor));
#endif

    controller->ready_spawners = 0;
    controller->ready_stores = 0;
    controller->partitioner = -1;
}

void bsal_input_controller_destroy(struct bsal_actor *actor)
{
    struct bsal_input_controller *controller;
    int i;
    char *pointer;

    controller = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);

    for (i = 0; i < bsal_vector_size(&controller->files); i++) {
        pointer = *(char **)bsal_vector_at(&controller->files, i);
        free(pointer);
    }

    bsal_vector_destroy(&controller->streams);
    bsal_vector_destroy(&controller->partition_commands);
    bsal_vector_destroy(&controller->files);
    bsal_vector_destroy(&controller->spawners);
    bsal_vector_destroy(&controller->counts);
    bsal_vector_destroy(&controller->stores);
    bsal_vector_destroy(&controller->stores_per_spawner);
    bsal_queue_destroy(&controller->unprepared_spawners);
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
    uint64_t entries;
    uint64_t *bucket;
    int *int_bucket;
    int store;
    int spawner;
    int command_name;
    int stream_name;

    bsal_message_helper_get_all(message, &tag, &count, &buffer, &source);

    name = bsal_actor_name(actor);
    controller = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);
    concrete_actor = controller;

    if (tag == BSAL_INPUT_CONTROLLER_START) {

        bsal_vector_unpack(&controller->spawners, buffer);

        bsal_vector_resize(&controller->stores_per_spawner,
                        bsal_vector_size(&controller->spawners));

        for (i = 0; i < bsal_vector_size(&controller->spawners); i++) {
            int_bucket = (int *)bsal_vector_at(&controller->stores_per_spawner, i);
            *int_bucket = 0;

            spawner = bsal_vector_helper_at_as_int(&controller->spawners, i);

            bsal_queue_enqueue(&concrete_actor->unprepared_spawners, &spawner);
        }

        controller->state = BSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS;

        printf("DEBUG preparing first spawner\n");

        bsal_actor_helper_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_PREPARE_SPAWNERS);

        /*
        bsal_dispatcher_print(bsal_actor_dispatcher(actor));
        */

    } else if (tag == BSAL_ADD_FILE) {

        file = (char *)buffer;

        local_file = malloc(strlen(file) + 1);
        strcpy(local_file, file);

        bsal_vector_push_back(&controller->files, &local_file);

        bucket = bsal_vector_at(&controller->files, bsal_vector_size(&controller->files) - 1);
        local_file = *(char **)bucket;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG11 BSAL_ADD_FILE %s %p bucket %p index %d\n",
                        local_file, local_file, (void *)bucket, bsal_vector_size(&controller->files) - 1);
#endif

        bsal_actor_helper_send_reply_empty(actor, BSAL_ADD_FILE_REPLY);

    } else if (tag == BSAL_ACTOR_SPAWN_REPLY) {

        if (controller->state == BSAL_INPUT_CONTROLLER_STATE_SPAWN_STORES) {

            bsal_input_controller_add_store(actor, message);
            return;

        } else if (controller->state == BSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS) {

            concrete_actor->ready_spawners++;
            bsal_message_helper_unpack_int(message, 0, &name);
            bsal_actor_helper_send_empty(actor, name, BSAL_ACTOR_ASK_TO_STOP);
            bsal_actor_helper_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_PREPARE_SPAWNERS);

            if (concrete_actor->ready_spawners == (int)bsal_vector_size(&concrete_actor->spawners)) {

                printf("DEBUG all spawners are prepared\n");
#ifdef BSAL_INPUT_CONTROLLER_DEBUG
#endif
                bsal_actor_helper_send_to_supervisor_empty(actor, BSAL_INPUT_CONTROLLER_START_REPLY);
            }

            return;

        } else if (concrete_actor->state == BSAL_INPUT_CONTROLLER_STATE_SPAWN_PARTITIONER) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
            printf("DEBUG received spawn reply, state is spawn_partitioner\n");
#endif

            bsal_message_helper_unpack_int(message, 0, &concrete_actor->partitioner);
            concrete_actor->partitioner = bsal_actor_add_acquaintance(actor,
                            concrete_actor->partitioner);

            /* configure the partitioner
             */
            destination = bsal_actor_get_acquaintance(actor,
                                    concrete_actor->partitioner);
            bsal_actor_helper_send_int(actor, destination,
                            BSAL_SEQUENCE_PARTITIONER_SET_BLOCK_SIZE,
                            concrete_actor->block_size);
            bsal_actor_helper_send_int(actor, destination,
                            BSAL_SEQUENCE_PARTITIONER_SET_ACTOR_COUNT,
                            bsal_vector_size(&concrete_actor->stores));

            count = bsal_vector_pack_size(&concrete_actor->counts);
            buffer = malloc(count);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
            printf("DEBUG packed counts, %d bytes\n", count);
#endif

            bsal_vector_pack(&concrete_actor->counts, buffer);
            bsal_message_init(&new_message, BSAL_SEQUENCE_PARTITIONER_SET_ENTRY_VECTOR,
                            count, buffer);
            bsal_actor_send(actor, destination, &new_message);
            free(buffer);
            return;
        }

        stream = *(int *)buffer;

        local_file = *(char **)bsal_vector_at(&controller->files, bsal_vector_size(&controller->streams));

        printf("DEBUG actor %d receives stream %d from spawner %d for file %s\n",
                        name, stream, source,
                        local_file);
#ifdef BSAL_INPUT_CONTROLLER_DEBUG
#endif

        bsal_vector_push_back(&controller->streams, &stream);
        bsal_vector_push_back(&controller->partition_commands, &stream);
        *(int *)bsal_vector_at(&controller->partition_commands,
                        bsal_vector_size(&controller->partition_commands) -1) = -1;

        bsal_message_init(&new_message, BSAL_INPUT_OPEN, strlen(local_file) + 1, local_file);
        bsal_actor_send(actor, stream, &new_message);

        if (bsal_vector_size(&controller->streams) != bsal_vector_size(&controller->files)) {

            bsal_actor_helper_send_to_self_empty(actor, BSAL_INPUT_SPAWN);

        }

    } else if (tag == BSAL_INPUT_OPEN_REPLY) {

        controller->opened_streams++;

        stream = source;
        bsal_message_helper_unpack_int(message, 0, &error);

        if (error == BSAL_INPUT_ERROR_NO_ERROR) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
            printf("DEBUG actor %d asks %d BSAL_INPUT_COUNT\n", name, stream);
#endif

            bsal_actor_helper_send_empty(actor, stream, BSAL_INPUT_COUNT);
        } else {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
            printf("DEBUG actor %d received error %d from %d\n", name, error, stream);
#endif
            controller->counted++;
        }

	/* if all streams failed, notice supervisor */
        if (controller->counted == bsal_vector_size(&controller->files)) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
#endif
            printf("DEBUG actor/%d: all streams failed.\n",
                            bsal_actor_name(actor));
            bsal_actor_helper_send_to_supervisor_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);
        }

/*
        if (controller->opened_streams == bsal_vector_size(&controller->files)) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
            printf("DEBUG controller %d sends BSAL_INPUT_DISTRIBUTE_REPLY to supervisor %d [%d/%d]\n",
                            name, bsal_actor_supervisor(actor),
                            controller->opened_streams, bsal_vector_size(&controller->files));
#endif

        }
*/

    } else if (tag == BSAL_INPUT_COUNT_PROGRESS) {

        stream_index = bsal_vector_index_of(&controller->streams, &source);
        local_file = bsal_vector_helper_at_as_char_pointer(&controller->files, stream_index);
        bsal_message_helper_unpack_uint64_t(message, 0, &entries);

        bucket = (uint64_t *)bsal_vector_at(&controller->counts, stream_index);

        if (entries > *bucket + 10000000) {
            printf("controller actor/%d receives from stream actor/%d: file %s, %" PRIu64 " entries so far\n",
                        name, source, local_file, entries);
            *bucket = entries;
        }

    } else if (tag == BSAL_INPUT_COUNT_REPLY) {

        stream_index = bsal_vector_index_of(&controller->streams, &source);
        local_file = bsal_vector_helper_at_as_char_pointer(&controller->files, stream_index);
        bsal_message_helper_unpack_uint64_t(message, 0, &entries);

        bucket = (uint64_t*)bsal_vector_at(&controller->counts, stream_index);
        *bucket = entries;

        printf("controller actor/%d received from stream actor/%d for file %s: %" PRIu64 " entries (final)\n",
                        name, source, local_file, entries);

        bsal_actor_helper_send_reply_empty(actor, BSAL_INPUT_STREAM_RESET);

    } else if (tag == BSAL_INPUT_STREAM_RESET_REPLY) {

        controller->counted++;

        /* continue work here, tell supervisor about it */
        if (controller->counted == bsal_vector_size(&controller->files)) {
            bsal_actor_helper_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_CREATE_STORES);
        }
    } else if (tag == BSAL_INPUT_DISTRIBUTE) {

        /* for each file, spawn a stream to count */

        /* no files, return immediately
         */
        if (bsal_vector_size(&concrete_actor->files) == 0) {
            bsal_actor_helper_send_reply_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);
            return;
        }

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG actor %d receives BSAL_INPUT_DISTRIBUTE\n", name);
#endif

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG send BSAL_INPUT_SPAWN to self\n");
#endif

        bsal_actor_helper_send_to_self_empty(actor, BSAL_INPUT_SPAWN);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG resizing counts to %d\n", bsal_vector_size(&controller->files));
#endif

        bsal_vector_resize(&controller->counts, bsal_vector_size(&controller->files));

        for (i = 0; i < bsal_vector_size(&controller->counts); i++) {
            bucket = (uint64_t*)bsal_vector_at(&controller->counts, i);
            *bucket = 0;
        }

    } else if (tag == BSAL_INPUT_SPAWN && source == name) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG BSAL_INPUT_SPAWN\n");
#endif

        script = BSAL_INPUT_STREAM_SCRIPT;

        concrete_actor->state = BSAL_INPUT_CONTROLLER_STATE_SPAWN_STREAMS;

        /* the next file name to send is the current number of streams */
        i = bsal_vector_size(&controller->streams);

        destination_index = i % bsal_vector_size(&controller->spawners);
        destination = *(int *)bsal_vector_at(&controller->spawners, destination_index);

        bsal_message_init(message, BSAL_ACTOR_SPAWN, sizeof(script), &script);
        bsal_actor_send(actor, destination, message);

        bucket = bsal_vector_at(&controller->files, i);
        local_file = *(char **)bsal_vector_at(&controller->files, i);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG890 local_file %p bucket %p index %d\n", local_file, (void *)bucket,
                        i);
#endif

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG actor %d spawns a stream for file %d/%d via spawner %d\n",
                        name, i, bsal_vector_size(&controller->files), destination);
#endif

        /* also, spawn 4 stores on each node */

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP && ( source == bsal_actor_supervisor(actor)
                            || source == bsal_actor_name(actor))) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
#endif
        /* stop streams
         */
        for (i = 0; i < bsal_vector_size(&concrete_actor->streams); i++) {
            stream = *(int *)bsal_vector_at(&concrete_actor->streams, i);

            bsal_actor_helper_send_empty(actor, stream, BSAL_ACTOR_ASK_TO_STOP);
        }

        /* stop data stores
         */
        for (i = 0; i < bsal_vector_size(&concrete_actor->stores); i++) {
            store = bsal_vector_helper_at_as_int(&concrete_actor->stores, i);

            bsal_actor_helper_send_empty(actor, store, BSAL_ACTOR_ASK_TO_STOP);
        }

        /* stop partitioner
         */

        if (concrete_actor->partitioner > 0) {
            bsal_actor_helper_send_empty(actor, bsal_actor_get_acquaintance(actor,
                                concrete_actor->partitioner),
                        BSAL_ACTOR_ASK_TO_STOP);

            printf("DEBUG controller %d sends BSAL_ACTOR_ASK_TO_STOP_REPLY to %d\n",
                        bsal_actor_name(actor),
                        bsal_message_source(message));

        }

        bsal_actor_helper_send_reply_empty(actor, BSAL_ACTOR_ASK_TO_STOP_REPLY);

        /* stop self
         */
        bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);

        printf("DEBUG controller actor/%d dies\n", name);

    } else if (tag == BSAL_INPUT_CONTROLLER_CREATE_PARTITION && source == name) {

        spawner = *(int *)bsal_vector_at(&concrete_actor->spawners,
                        bsal_vector_size(&concrete_actor->spawners) / 2);

        bsal_actor_helper_send_int(actor, spawner, BSAL_ACTOR_SPAWN,
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

        bsal_actor_helper_send_reply_empty(actor, BSAL_SEQUENCE_PARTITIONER_GET_COMMAND);

    } else if (tag == BSAL_SEQUENCE_PARTITIONER_GET_COMMAND_REPLY) {

        bsal_input_controller_receive_command(actor, message);

    } else if (tag == BSAL_SEQUENCE_PARTITIONER_FINISHED) {

        bsal_actor_helper_send_empty(actor, bsal_actor_get_acquaintance(actor,
                                concrete_actor->partitioner),
                        BSAL_ACTOR_ASK_TO_STOP);

        bsal_actor_helper_send_to_supervisor_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);

    } else if (tag == BSAL_SEQUENCE_PARTITIONER_PROVIDE_STORE_ENTRY_COUNTS) {

        bsal_input_controller_receive_store_entry_counts(actor, message);

    } else if (tag == BSAL_SEQUENCE_STORE_RESERVE_REPLY) {
        concrete_actor->ready_stores++;

        if (concrete_actor->ready_stores == bsal_vector_size(&concrete_actor->stores)) {

            printf("DEBUG all stores are ready\n");
            bsal_actor_helper_send_empty(actor,
                            bsal_actor_get_acquaintance(actor, concrete_actor->partitioner),
                            BSAL_SEQUENCE_PARTITIONER_PROVIDE_STORE_ENTRY_COUNTS_REPLY);
        }

    } else if (tag == BSAL_INPUT_PUSH_SEQUENCES_REPLY) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG bsal_input_controller_receive received BSAL_INPUT_PUSH_SEQUENCES_REPLY\n");
#endif

        stream_name = source;

        stream_index = bsal_vector_index_of(&concrete_actor->streams, &stream_name);
        command_name = *(int *)bsal_vector_at(&concrete_actor->partition_commands,
                        stream_index);

        bsal_actor_helper_send_int(actor, bsal_actor_get_acquaintance(actor,
                                concrete_actor->partitioner),
                        BSAL_SEQUENCE_PARTITIONER_GET_COMMAND_REPLY_REPLY,
                        command_name);

    } else if (tag == BSAL_INPUT_CONTROLLER_SET_CUSTOMERS) {

        bsal_vector_unpack(&concrete_actor->stores, buffer);
        printf("controller actor/%d receives %d customers\n",
                        bsal_actor_name(actor),
                        (int)bsal_vector_size(&concrete_actor->stores));

        bsal_vector_helper_print_int(&concrete_actor->stores);
        printf("\n");

        bsal_actor_helper_send_reply_empty(actor, BSAL_INPUT_CONTROLLER_SET_CUSTOMERS_REPLY);
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
    concrete_actor->ready_stores = 0;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG bsal_input_controller_receive_store_entry_counts unpacking entries\n");
#endif

    bsal_vector_unpack(&store_entries, buffer);

    for (i = 0; i < bsal_vector_size(&store_entries); i++) {
        store = *(int *)bsal_vector_at(&concrete_actor->stores, i);
        entries = *(uint64_t *)bsal_vector_at(&store_entries, i);

        printf("DEBUG controller actor/%d tells store actor/%d to reserve %" PRIu64 " buckets\n",
                        name, store, entries);

        bsal_message_init(&new_message, BSAL_SEQUENCE_STORE_RESERVE,
                        sizeof(entries), &entries);
        bsal_actor_send(actor, store, &new_message);
    }

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
    printf("DEBUG bsal_input_controller_receive_store_entry_counts will wait for replies\n");
#endif
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

    bsal_message_helper_get_all(message, &tag, &count, &buffer, &source);
/*
    printf("DEBUG bsal_input_controller_create_stores\n");
    */

    for (i = 0; i < bsal_vector_size(&concrete_actor->stores_per_spawner); i++) {
        value = bsal_vector_helper_at_as_int(&concrete_actor->stores_per_spawner, i);

        if (value == -1) {

                /*
            printf("DEBUG need more information about spawner at %i\n",
                            i);
                            */

            spawner = bsal_vector_helper_at_as_int(&concrete_actor->spawners, i);

            bsal_actor_helper_send_empty(actor, spawner, BSAL_ACTOR_GET_NODE_NAME);
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
        value = bsal_vector_helper_at_as_int(&concrete_actor->stores_per_spawner, i);

        if (value != 0) {

            spawner = bsal_vector_helper_at_as_int(&concrete_actor->spawners, i);
/*
            printf("DEBUG spawner %d is %d\n", i, spawner);
*/
            bsal_actor_helper_send_int(actor, spawner, BSAL_ACTOR_SPAWN, BSAL_SEQUENCE_STORE_SCRIPT);

            return;
        }
/*
        printf("DEBUG spawner %i spawned all its stores\n", i);
        */
    }

    printf("DEBUG controller actor/%d: sequence stores are ready (%d)\n",
                    bsal_actor_name(actor),
                    (int)bsal_vector_size(&concrete_actor->stores));

    for (i = 0; i < bsal_vector_size(&concrete_actor->stores); i++) {
        value = bsal_vector_helper_at_as_int(&concrete_actor->stores, i);

        printf("DEBUG controller actor/%d: sequence store %i is actor/%d\n",
                        bsal_actor_name(actor), i, value);
    }

    printf("DEBUG controller actor/%d: stream actors are\n",
                    bsal_actor_name(actor));

    total = 0;
    block_size = concrete_actor->block_size;

    for (i = 0; i < bsal_vector_size(&concrete_actor->files); i++) {
        entries = *(uint64_t*)bsal_vector_at(&concrete_actor->counts, i);
        local_file = bsal_vector_helper_at_as_char_pointer(&concrete_actor->files, i);
        name = *(int *)bsal_vector_at(&concrete_actor->streams, i);

        printf("stream actor/%d, %d/%d %s %" PRIu64 "\n",
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

    printf("DEBUG controller actor/%d: Partition Total: %" PRIu64 ", block_size: %d, blocks: %d\n",
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
        bsal_actor_helper_send_to_supervisor_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);
        return;
    } else {
        bsal_actor_helper_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_CREATE_PARTITION);
    }

    /*
    bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);
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

    bsal_message_helper_get_all(message, &tag, &count, &buffer, &source);
    spawner = source;
    bsal_message_helper_unpack_int(message, 0, &node);

    printf("DEBUG spawner actor/%d is on node node/%d\n", spawner, node);

    bsal_actor_helper_send_reply_empty(actor, BSAL_ACTOR_GET_NODE_WORKER_COUNT);
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

    bsal_message_helper_get_all(message, &tag, &count, &buffer, &source);
    spawner = source;
    bsal_message_helper_unpack_int(message, 0, &worker_count);

    index = bsal_vector_index_of(&concrete_actor->spawners, &spawner);
    bucket = bsal_vector_at(&concrete_actor->stores_per_spawner, index);
    *bucket = worker_count * concrete_actor->stores_per_worker_per_spawner;

    printf("DEBUG spawner actor/%d (node/%d) is on a node that has %d workers\n", spawner,
                    index, worker_count);

    bsal_actor_helper_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_CREATE_STORES);
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
    bsal_message_helper_unpack_int(message, 0, &store);

    index = bsal_vector_index_of(&concrete_actor->spawners, &source);

    /*
    printf("DEBUG bsal_input_controller_add_store index %d\n", index);
    */

    bucket = bsal_vector_at(&concrete_actor->stores_per_spawner, index);

    /* the content of the bucket is initially the total number of
     * stores that are desired for this spawner.
     */
    *bucket = (*bucket - 1);
    bsal_vector_push_back(&concrete_actor->stores, &store);

    bsal_actor_helper_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_CREATE_STORES);

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

        bsal_actor_helper_send_int(actor, spawner, BSAL_ACTOR_SPAWN, bsal_actor_script(actor));
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

    stream_name = *(int *)bsal_vector_at(&concrete_actor->streams,
                    stream_index);

    store_name = *(int *)bsal_vector_at(&concrete_actor->stores, store_index);
    store_first = bsal_partition_command_store_first(&command);
    store_last = bsal_partition_command_store_last(&command);

    bsal_input_command_init(&input_command, store_name, store_first, store_last);

    bytes = bsal_input_command_pack_size(&input_command);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
    printf("DEBUG input command\n");
    bsal_input_command_print(&input_command);

    printf("DEBUG bsal_input_controller_receive_command bytes %d\n",
                    bytes);
#endif

    new_buffer = malloc(bytes);
    bsal_input_command_pack(&input_command, new_buffer);

    bsal_message_init(&new_message, BSAL_INPUT_PUSH_SEQUENCES, bytes,
                    new_buffer);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_COMMANDS
    printf("DEBUG bsal_input_controller_receive_command sending BSAL_INPUT_PUSH_SEQUENCES to %d (index %d)\n",
                    stream_name, stream_index);
    bsal_input_command_print(&input_command);
#endif

    bsal_actor_send(actor, stream_name, &new_message);

    free(new_buffer);

    command_name = bsal_partition_command_name(&command);

    *bucket_for_command_name = command_name;
}
