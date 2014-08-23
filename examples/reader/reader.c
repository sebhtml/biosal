
#include "reader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h>

struct thorium_script reader_script = {
    .identifier = SCRIPT_READER,
    .init = reader_init,
    .destroy = reader_destroy,
    .receive = reader_receive,
    .size = sizeof(struct reader),
    .name = "reader"
};

void reader_init(struct thorium_actor *actor)
{
    struct reader *reader1;

    reader1 = (struct reader *)thorium_actor_concrete_actor(actor);
    reader1->counted = 0;
    reader1->pulled = 0;

    bsal_vector_init(&reader1->spawners, sizeof(int));
    thorium_actor_add_script(actor, BSAL_INPUT_SCRIPT_STREAM,
                    &bsal_input_stream_script);
}

void reader_destroy(struct thorium_actor *actor)
{
    struct reader *reader1;

    reader1 = (struct reader *)thorium_actor_concrete_actor(actor);
    reader1->counted = 0;
    reader1->pulled = 0;

    bsal_vector_destroy(&reader1->spawners);
}

void reader_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int argc;
    char **argv;
    /*int i;*/
    int name;
    struct reader *reader1;
    int source;
    uint64_t count;
    void *buffer;
    int sequences;
    int script;
    int sequence_index;
    char *received_sequence;
    int error;

    reader1 = (struct reader *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_tag(message);
    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);
    name = thorium_actor_name(actor);

    if (tag == THORIUM_ACTOR_START) {

        bsal_vector_init(&reader1->spawners, 0);
        bsal_vector_unpack(&reader1->spawners, buffer);

        argc = thorium_actor_argc(actor);
        argv = thorium_actor_argv(actor);
        name = thorium_actor_name(actor);
        reader1->last_report = 0;

        /*
        printf("actor %i received %i arguments\n", name, argc);

        for (i = 0; i < argc ;i++) {
            printf("   argument %i : %s\n", i, argv[i]);
        }
        */

        if (bsal_vector_index_of(&reader1->spawners, &name) != 0) {
            thorium_message_init(message, THORIUM_ACTOR_STOP, 0, NULL);
            thorium_actor_send(actor, name, message);

            return;
        }

        if (argc == 1) {
            thorium_message_init(message, THORIUM_ACTOR_STOP, 0, NULL);
            thorium_actor_send(actor, name, message);

            return;
        }

        reader1->sequence_reader = thorium_actor_spawn(actor, BSAL_INPUT_SCRIPT_STREAM);

        reader1->file = argv[argc - 1];

        thorium_message_init(message, BSAL_INPUT_OPEN, strlen(reader1->file) + 1,
                        reader1->file);

        printf("actor %i: asking actor %i to count entries in %s\n",
                        name, reader1->sequence_reader, reader1->file);

        thorium_actor_send(actor, reader1->sequence_reader, message);

    } else if (tag == BSAL_INPUT_COUNT_PROGRESS) {

        sequences = *(int64_t *)buffer;

        if (sequences < reader1->last_report + 10000000) {

            return;
        }

        printf("Actor %i received a progress report from actor %i: %i\n",
                        name, source, sequences);
        reader1->last_report = sequences;

    } else if (tag == BSAL_INPUT_OPEN_REPLY && !reader1->counted) {

        thorium_message_unpack_int(message, 0, &error);

        if (error == BSAL_INPUT_ERROR_NO_ERROR) {
            printf("Successfully opened file.\n");
            thorium_actor_send_reply_empty(actor, BSAL_INPUT_COUNT);

        } else if (error == BSAL_INPUT_ERROR_FILE_NOT_FOUND) {

            printf("Error, file not found! \n");
            thorium_actor_send_to_self_empty(actor, THORIUM_ACTOR_STOP);

        } else if (error == BSAL_INPUT_ERROR_FORMAT_NOT_SUPPORTED) {

            printf("Error, format not supported! \n");
            thorium_actor_send_to_self_empty(actor, THORIUM_ACTOR_STOP);

        }
    } else if (tag == BSAL_INPUT_COUNT_REPLY) {

        count = *(int64_t*)thorium_message_buffer(message);
        printf("actor %i: file has %" PRIu64 " items\n", name, count);

        thorium_message_init(message, BSAL_INPUT_CLOSE, 0, NULL);
        thorium_actor_send(actor, source, message);

        reader1->counted = 1;

    } else if (tag == BSAL_INPUT_CLOSE_REPLY && !reader1->pulled) {

        /* not necessary, it is already dead. */
        thorium_actor_send_reply_empty(actor, THORIUM_ACTOR_ASK_TO_STOP);

        printf("actor %d received BSAL_INPUT_CLOSE_REPLY from actor %d, asking it to stop"
                        " with THORIUM_ACTOR_ASK_TO_STOP\n", name, source);
            /*
        thorium_message_init(message, THORIUM_ACTOR_STOP, 0, NULL);
        thorium_actor_send(actor, name, message);

        return;
        */

        script = BSAL_INPUT_SCRIPT_STREAM;

        thorium_message_init(message, THORIUM_ACTOR_SPAWN, sizeof(script), &script);
        thorium_actor_send(actor, name, message);

    } else if (tag == BSAL_INPUT_CLOSE_REPLY && reader1->pulled) {

        thorium_actor_send_reply_empty(actor, THORIUM_ACTOR_ASK_TO_STOP);

        thorium_actor_send_to_self_empty(actor, THORIUM_ACTOR_STOP);

    } else if (tag == THORIUM_ACTOR_ASK_TO_STOP_REPLY && reader1->pulled) {

        /* this tag will never arrive here */
        thorium_message_init(message, THORIUM_ACTOR_STOP, 0, NULL);
        thorium_actor_send(actor, name, message);

    } else if (tag == THORIUM_ACTOR_SPAWN_REPLY && source == name) {

        reader1->sequence_reader = *(int *)buffer;

        printf("actor %d tells actor %d to open %s to pull sequences from the file\n",
                        name, reader1->sequence_reader, reader1->file);

        thorium_message_init(message, BSAL_INPUT_OPEN,
                        strlen(reader1->file) + 1, reader1->file);
        thorium_actor_send(actor, reader1->sequence_reader, message);

    } else if (tag == BSAL_INPUT_OPEN_REPLY && reader1->counted) {

        thorium_message_init(message, BSAL_INPUT_GET_SEQUENCE, 0, NULL);
        thorium_actor_send(actor, source, message);

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
            thorium_message_init(message, BSAL_INPUT_GET_SEQUENCE, 0, NULL);
            thorium_actor_send(actor, source, message);
        } else {
            thorium_message_init(message, BSAL_INPUT_CLOSE, 0, NULL);
            thorium_actor_send(actor, source, message);
            reader1->pulled = 1;
        }

    } else if (tag == BSAL_INPUT_GET_SEQUENCE_END) {
        printf("actor %d: reached the end...\n", name);

        thorium_message_init(message, BSAL_INPUT_CLOSE, 0, NULL);
        thorium_actor_send(actor, source, message);

        reader1->pulled = 1;
    }
}
