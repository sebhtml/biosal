
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

    bsal_vector_init(&reader1->spawners, sizeof(int));
}

void reader_destroy(struct bsal_actor *actor)
{
    struct reader *reader1;

    reader1 = (struct reader *)bsal_actor_concrete_actor(actor);
    reader1->counted = 0;
    reader1->pulled = 0;

    bsal_vector_destroy(&reader1->spawners);
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
    void *buffer;
    int sequences;
    int script;
    int sequence_index;
    char *received_sequence;
    int error;

    reader1 = (struct reader *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);
    buffer = bsal_message_buffer(message);
    name = bsal_actor_name(actor);

    if (tag == BSAL_ACTOR_START) {

        bsal_vector_unpack(&reader1->spawners, buffer);
        bsal_actor_add_script(actor, BSAL_INPUT_STREAM_SCRIPT,
                    &bsal_input_stream_script);

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

        if (bsal_vector_index_of(&reader1->spawners, &name) != 0) {
            bsal_message_init(message, BSAL_ACTOR_STOP, 0, NULL);
            bsal_actor_send(actor, name, message);

            return;
        }

        if (argc == 1) {
            bsal_message_init(message, BSAL_ACTOR_STOP, 0, NULL);
            bsal_actor_send(actor, name, message);

            return;
        }

        reader1->sequence_reader = bsal_actor_spawn(actor, BSAL_INPUT_STREAM_SCRIPT);

        reader1->file = argv[argc - 1];

        bsal_message_init(message, BSAL_INPUT_OPEN, strlen(reader1->file) + 1,
                        reader1->file);

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

    } else if (tag == BSAL_INPUT_OPEN_REPLY && !reader1->counted) {

        bsal_message_unpack_int(message, 0, &error);

        if (error == BSAL_INPUT_ERROR_NO_ERROR) {
            printf("Successfully opened file.\n");
            bsal_actor_send_reply_empty(actor, BSAL_INPUT_COUNT);

        } else if (error == BSAL_INPUT_ERROR_FILE_NOT_FOUND) {

            printf("Error, file not found! \n");
            bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);

        } else if (error == BSAL_INPUT_ERROR_FORMAT_NOT_SUPPORTED) {

            printf("Error, format not supported! \n");
            bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);

        }
    } else if (tag == BSAL_INPUT_COUNT_REPLY) {

        count = *(int *)bsal_message_buffer(message);
        printf("actor %i: file has %i items\n", name, count);

        bsal_message_init(message, BSAL_INPUT_CLOSE, 0, NULL);
        bsal_actor_send(actor, source, message);

        reader1->counted = 1;

    } else if (tag == BSAL_INPUT_CLOSE_REPLY && !reader1->pulled) {

        /* not necessary, it is already dead. */
        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_ASK_TO_STOP);

        printf("actor %d received BSAL_INPUT_CLOSE_REPLY from actor %d, asking it to stop"
                        " with BSAL_ACTOR_ASK_TO_STOP\n", name, source);
            /*
        bsal_message_init(message, BSAL_ACTOR_STOP, 0, NULL);
        bsal_actor_send(actor, name, message);

        return;
        */

        script = BSAL_INPUT_STREAM_SCRIPT;

        bsal_message_init(message, BSAL_ACTOR_SPAWN, sizeof(script), &script);
        bsal_actor_send(actor, name, message);

    } else if (tag == BSAL_INPUT_CLOSE_REPLY && reader1->pulled) {

        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_ASK_TO_STOP);

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP_REPLY && reader1->pulled) {

        /* this tag will never arrive here */
        bsal_message_init(message, BSAL_ACTOR_STOP, 0, NULL);
        bsal_actor_send(actor, name, message);

    } else if (tag == BSAL_ACTOR_SPAWN_REPLY && source == name) {

        reader1->sequence_reader = *(int *)buffer;

        printf("actor %d tells actor %d to open %s to pull sequences from the file\n",
                        name, reader1->sequence_reader, reader1->file);

        bsal_message_init(message, BSAL_INPUT_OPEN,
                        strlen(reader1->file) + 1, reader1->file);
        bsal_actor_send(actor, reader1->sequence_reader, message);

    } else if (tag == BSAL_INPUT_OPEN_REPLY && reader1->counted) {

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

        reader1->pulled = 1;
    }
}
