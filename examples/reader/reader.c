
#include "reader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_script reader_script = {
    .name = READER_SCRIPT,
    .init = reader_init,
    .destroy = reader_destroy,
    .receive = reader_receive,
    .size = sizeof(struct reader)
};

void reader_init(struct bsal_actor *actor)
{
    struct reader *reader1;

    reader1 = (struct reader *)bsal_actor_concrete_actor(actor);
    reader1->counted = 0;
    reader1->pulled = 0;
}

void reader_destroy(struct bsal_actor *actor)
{
    struct reader *reader1;

    reader1 = (struct reader *)bsal_actor_concrete_actor(actor);
    reader1->counted = 0;
    reader1->pulled = 0;
}

void reader_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int argc;
    char **argv;
    /*int i;*/
    int name;
    struct reader *reader1;
    int source;
    int count;
    int nodes;
    void *buffer;
    int sequences;
    int script;
    int sequence_index;
    char *received_sequence;

    reader1 = (struct reader *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);
    nodes = bsal_actor_nodes(actor);
    buffer = bsal_message_buffer(message);
    name = bsal_actor_name(actor);

    if (tag == BSAL_ACTOR_START) {

        bsal_actor_add_script(actor, BSAL_INPUT_SCRIPT,
                    &bsal_input_script);

        argc = bsal_actor_argc(actor);
        argv = bsal_actor_argv(actor);
        name = bsal_actor_name(actor);
        reader1->last_report = 0;

        /*
        printf("actor %i received %i arguments\n", name, argc);

        for (i = 0; i < argc ;i++) {
            printf("   argument %i : %s\n", i, argv[i]);
        }
        */

        if (name % nodes != 0) {
            bsal_message_set_tag(message, BSAL_ACTOR_STOP);
            bsal_actor_send(actor, name, message);

            return;
        }

        if (argc == 1) {
            bsal_message_set_tag(message, BSAL_ACTOR_STOP);
            bsal_actor_send(actor, name, message);

            return;
        }

        reader1->sequence_reader = bsal_actor_spawn(actor, BSAL_INPUT_SCRIPT);

        reader1->file = argv[argc - 1];

        bsal_message_set_tag(message, BSAL_INPUT_OPEN);
        bsal_message_set_buffer(message, reader1->file);
        bsal_message_set_count(message, strlen(reader1->file) + 1);

        printf("actor %i: asking actor %i to count entries in %s\n",
                        name, reader1->sequence_reader, reader1->file);

        bsal_actor_send(actor, reader1->sequence_reader, message);

    } else if (tag == BSAL_INPUT_COUNT_PROGRESS) {

        sequences = *(int *)buffer;

        if (sequences < reader1->last_report + 10000000) {

            return;
        }

        printf("Actor %i received a progress report from actor %i: %i\n",
                        name, source, sequences);
        reader1->last_report = sequences;

    } else if (tag == BSAL_INPUT_ERROR_FILE_NOT_FOUND) {

        printf("Error, file not found! \n");
        bsal_message_set_tag(message, BSAL_ACTOR_STOP);
        bsal_actor_send(actor, name, message);

    } else if (tag == BSAL_INPUT_ERROR_FORMAT_NOT_SUPPORTED) {

        printf("Error, format not supported! \n");

        bsal_message_set_tag(message, BSAL_ACTOR_STOP);
        bsal_actor_send(actor, name, message);

    } else if (tag == BSAL_INPUT_OPEN_OK && !reader1->counted) {
        bsal_message_set_tag(message, BSAL_INPUT_COUNT);
        bsal_actor_send(actor, source, message);

    } else if (tag == BSAL_INPUT_COUNT_RESULT) {

        count = *(int *)bsal_message_buffer(message);
        printf("actor %i: file has %i items\n", name, count);

        bsal_message_set_tag(message, BSAL_INPUT_CLOSE);
        bsal_actor_send(actor, source, message);

        reader1->counted = 1;
    } else if (tag == BSAL_INPUT_CLOSE_OK && !reader1->pulled) {

            /*
        bsal_message_set_tag(message, BSAL_ACTOR_STOP);
        bsal_actor_send(actor, name, message);

        return;
        */

        script = BSAL_INPUT_SCRIPT;

        bsal_message_init(message, BSAL_ACTOR_SPAWN, sizeof(script), &script);
        bsal_actor_send(actor, name, message);

    } else if (tag == BSAL_INPUT_CLOSE_OK && reader1->pulled) {

        bsal_message_init(message, BSAL_ACTOR_STOP, 0, NULL);
        bsal_actor_send(actor, name, message);

    } else if (tag == BSAL_ACTOR_SPAWN_REPLY) {

        reader1->sequence_reader = *(int *)buffer;

        printf("actor %d tells actor %d to open %s\n",
                        name, reader1->sequence_reader, reader1->file);

        bsal_message_init(message, BSAL_INPUT_OPEN,
                        strlen(reader1->file) + 1, reader1->file);
        bsal_actor_send(actor, reader1->sequence_reader, message);

    } else if (tag == BSAL_INPUT_OPEN_OK && reader1->counted) {

        bsal_message_init(message, BSAL_INPUT_GET_SEQUENCE, 0, NULL);
        bsal_actor_send(actor, source, message);

    } else if (tag == BSAL_INPUT_GET_SEQUENCE_REPLY) {

        sequence_index = *(int *)buffer;
        received_sequence = (char*)buffer + sizeof(sequence_index);

        /*
        printf("DEBUG %d %s\n", sequence_index, received_sequence);
*/
        if (sequence_index == 123456) {
            printf("actor %d says that sequence %d is %s.\n",
                            name, sequence_index, received_sequence);
        } else if (sequence_index % 100000 == 0) {
            printf("actor %d is pulling sequences from fellow local actor %d,"
                            " %d sequences pulled so far !\n",
                            name, reader1->sequence_reader, sequence_index);
        }

        if (sequence_index < 1000000) {
            bsal_message_init(message, BSAL_INPUT_GET_SEQUENCE, 0, NULL);
            bsal_actor_send(actor, source, message);
        } else {
            bsal_message_init(message, BSAL_INPUT_CLOSE, 0, NULL);
            bsal_actor_send(actor, source, message);
            reader1->pulled = 1;
        }

    } else if (tag == BSAL_INPUT_GET_SEQUENCE_END) {
        printf("actor %d: reached the end...\n", name);

        bsal_message_init(message, BSAL_INPUT_CLOSE, 0, NULL);
        bsal_actor_send(actor, source, message);

        bsal_message_set_tag(message, BSAL_ACTOR_STOP);
        bsal_actor_send(actor, name, message);
    }
}
