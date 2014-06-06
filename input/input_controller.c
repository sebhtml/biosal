
#include "input_controller.h"

#include "input_stream.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
}

void bsal_input_controller_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
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

    name = bsal_actor_name(actor);
    controller = (struct bsal_input_controller *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);
    buffer = bsal_message_buffer(message);

    if (tag == BSAL_INPUT_CONTROLLER_START) {

        bsal_vector_unpack(&controller->spawners, buffer);
        bsal_actor_add_script(actor, BSAL_INPUT_STREAM_SCRIPT, &bsal_input_stream_script);

        bsal_actor_send_reply_empty(actor, BSAL_INPUT_CONTROLLER_START_REPLY);

    } else if (tag == BSAL_ADD_FILE) {

        file = (char *)buffer;

        local_file = malloc(strlen(file) + 1);
        strcpy(local_file, file);

        bsal_vector_push_back(&controller->files, &local_file);

        bsal_actor_send_reply_empty(actor, BSAL_ADD_FILE_REPLY);

    } else if (tag == BSAL_ACTOR_SPAWN_REPLY) {

        stream = *(int *)buffer;

        local_file = *(char **)bsal_vector_at(&controller->files, bsal_vector_size(&controller->streams));

        printf("DEBUG actor %d receives stream %d from spawner %d for file %s\n",
                        name, stream, source,
                        local_file);

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
        /* TODO continue work here */

        if (error == BSAL_INPUT_ERROR_NO_ERROR) {
            /*bsal_actor_send_empty(actor, stream, BSAL_INPUT_CLOSE);*/
            bsal_actor_send_empty(actor, stream, BSAL_ACTOR_ASK_TO_STOP);
        }

        if (controller->opened_streams == bsal_vector_size(&controller->files)) {

            printf("DEBUG controller %d sends BSAL_INPUT_DISTRIBUTE_REPLY to supervisor %d [%d/%d]\n",
                            name, bsal_actor_supervisor(actor),
                            controller->opened_streams, bsal_vector_size(&controller->files));

            bsal_actor_send_to_supervisor_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);
        }

    } else if (tag == BSAL_INPUT_DISTRIBUTE) {

        /* for each file, spawn a stream to count */

        printf("DEBUG actor %d receives BSAL_INPUT_DISTRIBUTE\n", name);

        bsal_actor_send_to_self_empty(actor, BSAL_INPUT_SPAWN);

    } else if (tag == BSAL_INPUT_SPAWN && source == name) {

        script = BSAL_INPUT_STREAM_SCRIPT;

        i = bsal_vector_size(&controller->streams);

        destination_index = i % bsal_vector_size(&controller->spawners);
        destination = *(int *)bsal_vector_at(&controller->spawners, destination_index);

        bsal_message_init(message, BSAL_ACTOR_SPAWN, sizeof(script), &script);
        bsal_actor_send(actor, destination, message);

        local_file = *(char **)bsal_vector_at(&controller->files, i);
        printf("DEBUG actor %d spawns a stream for %s via spawner %d\n",
                        name, local_file, destination);

        /* also, spawn 4 stores on each node */

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP && source == bsal_actor_supervisor(actor)) {

        printf("DEBUG controller %d dies\n", name);

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_ASK_TO_STOP_REPLY);
    }
}
