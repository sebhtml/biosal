
#include "input_controller.h"

#include "input_stream.h"

#include <storage/sequence_store.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h> /* for PRIu64 */

/*
#define BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
#define BSAL_INPUT_CONTROLLER_DEBUG_10355
*/

#define BSAL_INPUT_CONTROLLER_DEBUG

#define BSAL_INPUT_CONTROLLER_STATE_NONE 0
#define BSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS 1
#define BSAL_INPUT_CONTROLLER_STATE_SPAWN_STORES 2
#define BSAL_INPUT_CONTROLLER_STATE_SPAWN_STREAMS 42

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
    bsal_vector_init(&controller->files, sizeof(char *));
    bsal_vector_init(&controller->spawners, sizeof(int));
    bsal_vector_init(&controller->counts, sizeof(int));
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
    int entries;
    int *bucket;
    int store;
    int spawner;

    bsal_message_get_all(message, &tag, &count, &buffer, &source);

    name = bsal_actor_name(actor);
    controller = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);
    concrete_actor = controller;

    if (tag == BSAL_INPUT_CONTROLLER_START) {

        bsal_vector_unpack(&controller->spawners, buffer);

        bsal_vector_resize(&controller->stores_per_spawner,
                        bsal_vector_size(&controller->spawners));

        for (i = 0; i < bsal_vector_size(&controller->spawners); i++) {
            bucket = (int *)bsal_vector_at(&controller->stores_per_spawner, i);
            *bucket = -1;

            spawner = bsal_vector_at_as_int(&controller->spawners, i);

            bsal_queue_enqueue(&concrete_actor->unprepared_spawners, &spawner);
        }

        controller->state = BSAL_INPUT_CONTROLLER_PREPARE_SPAWNERS;

        bsal_actor_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_PREPARE_SPAWNERS);

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

        bsal_actor_send_reply_empty(actor, BSAL_ADD_FILE_REPLY);

    } else if (tag == BSAL_ACTOR_SPAWN_REPLY) {

        if (controller->state == BSAL_INPUT_CONTROLLER_STATE_SPAWN_STORES) {

            bsal_input_controller_add_store(actor, message);
            return;

        } else if (controller->state == BSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS) {

            bsal_message_unpack_int(message, 0, &name);
            bsal_actor_send_empty(actor, name, BSAL_ACTOR_ASK_TO_STOP);
            bsal_actor_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_PREPARE_SPAWNERS);
            return;
        }

        stream = *(int *)buffer;

        local_file = *(char **)bsal_vector_at(&controller->files, bsal_vector_size(&controller->streams));

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG actor %d receives stream %d from spawner %d for file %s\n",
                        name, stream, source,
                        local_file);
#endif

        bsal_vector_push_back(&controller->streams, &stream);

        bsal_message_init(&new_message, BSAL_INPUT_OPEN, strlen(local_file) + 1, local_file);
        bsal_actor_send(actor, stream, &new_message);

        if (bsal_vector_size(&controller->streams) != bsal_vector_size(&controller->files)) {

            bsal_actor_send_to_self_empty(actor, BSAL_INPUT_SPAWN);

        }

    } else if (tag == BSAL_INPUT_OPEN_REPLY) {

        controller->opened_streams++;

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
            controller->counted++;
        }

	/* if all streams failed, notice supervisor */
        if (controller->counted == bsal_vector_size(&controller->files)) {

            bsal_actor_send_to_supervisor_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);
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
        local_file = bsal_vector_at_as_char_pointer(&controller->files, stream_index);
        bsal_message_unpack_int(message, 0, &entries);

        bucket = (int *)bsal_vector_at(&controller->counts, stream_index);

        if (entries > *bucket + 10000000) {
            printf("DEBUG actor:%d receives from actor:%d, file %s, %d entries so far\n",
                        name, source, local_file, entries);
            *bucket = entries;
        }

    } else if (tag == BSAL_INPUT_COUNT_REPLY) {

        stream_index = bsal_vector_index_of(&controller->streams, &source);
        local_file = bsal_vector_at_as_char_pointer(&controller->files, stream_index);
        bsal_message_unpack_int(message, 0, &entries);

        bucket = (int *)bsal_vector_at(&controller->counts, stream_index);
        *bucket = entries;

        printf("DEBUG Actor %d received from actor %d for file %s: %d entries\n",
                        name, source, local_file, entries);

        controller->counted++;

        bsal_actor_send_reply_empty(actor, BSAL_INPUT_CLOSE);

        /* continue work here, tell supervisor about it */
        if (controller->counted == bsal_vector_size(&controller->files)) {
            bsal_actor_send_to_self_empty(actor, BSAL_INPUT_CONTROLLER_CREATE_STORES);
        }
    } else if (tag == BSAL_INPUT_DISTRIBUTE) {

        /* for each file, spawn a stream to count */

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG actor %d receives BSAL_INPUT_DISTRIBUTE\n", name);
#endif

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG send BSAL_INPUT_SPAWN to self\n");
#endif

        bsal_actor_send_to_self_empty(actor, BSAL_INPUT_SPAWN);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG resizing counts to %d\n", bsal_vector_size(&controller->files));
#endif

        bsal_vector_resize(&controller->counts, bsal_vector_size(&controller->files));

        for (i = 0; i < bsal_vector_size(&controller->counts); i++) {
            bucket = (int *)bsal_vector_at(&controller->counts, i);
            *bucket = 0;
        }

    } else if (tag == BSAL_INPUT_SPAWN && source == name) {

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

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG actor %d spawns a stream for file %d/%d via spawner %d\n",
                        name, i, bsal_vector_size(&controller->files), destination);
#endif

        /* also, spawn 4 stores on each node */

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP && source == bsal_actor_supervisor(actor)) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_LEVEL_2
        printf("DEBUG controller %d dies\n", name);
#endif

        /* stop data stores
         */
        for (i = 0; i < bsal_vector_size(&concrete_actor->stores); i++) {
            store = bsal_vector_at_as_int(&concrete_actor->stores, i);

            bsal_actor_send_empty(actor, store, BSAL_ACTOR_ASK_TO_STOP);
        }

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_ASK_TO_STOP_REPLY);

    }
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
    int entries;
    char *local_file;
    int name;
    int stores;

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

    printf("DEBUG sequence stores are ready (%d)\n",
                    bsal_vector_size(&concrete_actor->stores));

    for (i = 0; i < bsal_vector_size(&concrete_actor->stores); i++) {
        value = bsal_vector_at_as_int(&concrete_actor->stores, i);

        printf("DEBUG sequence store %i is %d\n", i, value);
    }

    printf("DEBUG sequence files\n");

    total = 0;
    block_size = 4096;

    for (i = 0; i < bsal_vector_size(&concrete_actor->files); i++) {
        entries = bsal_vector_at_as_int(&concrete_actor->counts, i);
        local_file = bsal_vector_at_as_char_pointer(&concrete_actor->files, i);

        printf("actor:%d, %d/%d %s %d\n",
                        name, i,
                        bsal_vector_size(&concrete_actor->files),
                        local_file,
                        entries);
        total += entries;
    }

    blocks = total / block_size;

    if (total % block_size != 0) {
        blocks++;
    }

    stores = bsal_vector_size(&concrete_actor->stores);
    printf("DEBUG Partition Total: %" PRIu64 ", block_size: %d, blocks: %d\n",
                    total, block_size, blocks);
    printf("DEBUG Stores: %d, Blocks per store: %d, entries per store: %d\n",
                          stores, blocks / stores, (int)(total / stores));

#ifdef BSAL_INPUT_CONTROLLER_DEBUG_10355
    printf("DEBUG send BSAL_INPUT_CONTROLLER_CREATE_STORES to self %d\n",
                            bsal_actor_name(actor));
#endif


    bsal_actor_send_to_supervisor_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);

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

    printf("DEBUG spawner %d is on node %d\n", spawner, node);

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
    *bucket = worker_count;

    printf("DEBUG spawner %d (%d) is on a node that has %d workers\n", spawner,
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


    *bucket = (*bucket - 1);
    bsal_vector_push_back(&concrete_actor->stores, &store);

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

    printf("DEBUG bsal_input_controller_prepare_spawners \n");

    /* spawn an actor of the same script on every spawner to load required
     * scripts on all nodes
     */
    if (bsal_queue_dequeue(&concrete_actor->unprepared_spawners, &spawner)) {

        bsal_actor_send_int(actor, spawner, BSAL_ACTOR_SPAWN, bsal_actor_script(actor));
        concrete_actor->state = BSAL_INPUT_CONTROLLER_STATE_PREPARE_SPAWNERS;
    } else {

        printf("DEBUG all spawners are prepared\n");
        bsal_actor_send_to_supervisor_empty(actor, BSAL_INPUT_CONTROLLER_START_REPLY);
    }
}
