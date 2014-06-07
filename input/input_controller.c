
#include "input_controller.h"

#include "input_stream.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
*/

#define BSAL_INPUT_CONTROLLER_DEBUG

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
    controller->opened_streams = 0;
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
}

void bsal_input_controller_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int count;
    char *file;
    void *buffer;
    struct bsal_input_controller *controller;
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

    bsal_message_get_all(message, &tag, &count, &buffer, &source);

    name = bsal_actor_name(actor);
    controller = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);

    if (tag == BSAL_INPUT_CONTROLLER_START) {

        bsal_vector_unpack(&controller->spawners, buffer);
        bsal_actor_add_script(actor, BSAL_INPUT_STREAM_SCRIPT, &bsal_input_stream_script);

        bsal_actor_send_reply_empty(actor, BSAL_INPUT_CONTROLLER_START_REPLY);

    } else if (tag == BSAL_ADD_FILE) {

        file = (char *)buffer;

        local_file = malloc(strlen(file) + 1);
        strcpy(local_file, file);

        bsal_vector_push_back(&controller->files, &local_file);

        bucket = bsal_vector_at(&controller->files, bsal_vector_size(&controller->files) - 1);
        local_file = *(char **)bucket;

#ifdef BSAL_INPUT_CONTROLLER_DEBUG98
        printf("DEBUG11 BSAL_ADD_FILE %s %p bucket %p index %d\n",
                        local_file, local_file, (void *)bucket, bsal_vector_size(&controller->files) - 1);
#endif

        bsal_actor_send_reply_empty(actor, BSAL_ADD_FILE_REPLY);

    } else if (tag == BSAL_ACTOR_SPAWN_REPLY) {

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
            printf("DEBUG actor %d asks %d BSAL_INPUT_COUNT\n", name, stream);
            bsal_actor_send_empty(actor, stream, BSAL_INPUT_COUNT);
        } else {
            printf("DEBUG actor %d received error %d from %d\n", name, error, stream);
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

            for (i = 0; i < bsal_vector_size(&controller->files); i++) {
                entries = bsal_vector_at_as_int(&controller->counts, i);
                local_file = bsal_vector_at_as_char_pointer(&controller->files, i);

                printf("actor:%d, %d/%d %s %d\n",
                                name, i,
                                bsal_vector_size(&controller->files),
                                local_file,
                                entries);
            }

            bsal_actor_send_to_supervisor_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);
        }
    } else if (tag == BSAL_INPUT_DISTRIBUTE) {

        /* for each file, spawn a stream to count */

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG actor %d receives BSAL_INPUT_DISTRIBUTE\n", name);
#endif

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG send BSAL_INPUT_SPAWN to self\n");
#endif

        bsal_actor_send_to_self_empty(actor, BSAL_INPUT_SPAWN);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG12
        printf("DEBUG resizing counts to %d\n", bsal_vector_size(&controller->files));
#endif

        bsal_vector_resize(&controller->counts, bsal_vector_size(&controller->files));

        for (i = 0; i < bsal_vector_size(&controller->counts); i++) {
            bucket = (int *)bsal_vector_at(&controller->counts, i);
            *bucket = 0;
        }

    } else if (tag == BSAL_INPUT_SPAWN && source == name) {

        script = BSAL_INPUT_STREAM_SCRIPT;

        /* the next file name to send is the current number of streams */
        i = bsal_vector_size(&controller->streams);

        destination_index = i % bsal_vector_size(&controller->spawners);
        destination = *(int *)bsal_vector_at(&controller->spawners, destination_index);

        bsal_message_init(message, BSAL_ACTOR_SPAWN, sizeof(script), &script);
        bsal_actor_send(actor, destination, message);

        bucket = bsal_vector_at(&controller->files, i);
        local_file = *(char **)bsal_vector_at(&controller->files, i);

#ifdef BSAL_INPUT_CONTROLLER_DEBUG12
        printf("DEBUG890 local_file %p bucket %p index %d\n", local_file, (void *)bucket,
                        i);
#endif

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG actor %d spawns a stream for file %d/%d via spawner %d\n",
                        name, i, bsal_vector_size(&controller->files), destination);
#endif

        /* also, spawn 4 stores on each node */

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP && source == bsal_actor_supervisor(actor)) {

#ifdef BSAL_INPUT_CONTROLLER_DEBUG
        printf("DEBUG controller %d dies\n", name);
#endif

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_ASK_TO_STOP_REPLY);
    }
}
