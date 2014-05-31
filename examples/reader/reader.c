
#include "reader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_actor_vtable reader_vtable = {
    .receive = reader_receive
};

void reader_init(struct bsal_actor *actor)
{
}

void reader_destroy(struct bsal_actor *actor)
{
    bsal_actor_die(actor);
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
    char *file;
    int count;
    int nodes;
    void *buffer;
    int sequences;

    reader1 = (struct reader *)bsal_actor_pointer(actor);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);
    nodes = bsal_actor_nodes(actor);
    buffer = bsal_message_buffer(message);

    if (tag == BSAL_ACTOR_START) {
        reader_init(actor);

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
            reader_destroy(actor);
            return;
        }

        if (argc == 1) {
            reader_destroy(actor);
            return;
        }

        reader1->sequence_reader = bsal_actor_spawn(actor, &reader1->input_actor,
                        &bsal_input_actor_vtable);

        file = argv[argc - 1];

        bsal_message_set_tag(message, BSAL_INPUT_ACTOR_OPEN);
        bsal_message_set_buffer(message, file);
        bsal_message_set_count(message, strlen(file));

        printf("actor %i: asking actor %i to count entries in %s\n",
                        name, reader1->sequence_reader, file);

        bsal_actor_send(actor, reader1->sequence_reader, message);

    } else if (tag == BSAL_INPUT_ACTOR_COUNT_PROGRESS) {

        sequences = *(int *)buffer;

        if (sequences < reader1->last_report + 10000000) {

            return;
        }

        printf("Actor %i received a progress report from actor %i: %i\n",
                        name, source, sequences);
        reader1->last_report = sequences;

    } else if (tag == BSAL_INPUT_ACTOR_OPEN_NOT_FOUND) {

        printf("Error, file not found! \n");
        reader_destroy(actor);

    } else if (tag == BSAL_INPUT_ACTOR_OPEN_NOT_SUPPORTED) {

        printf("Error, format not supported! \n");
        reader_destroy(actor);

    } else if (tag == BSAL_INPUT_ACTOR_OPEN_OK) {
        bsal_message_set_tag(message, BSAL_INPUT_ACTOR_COUNT);
        bsal_actor_send(actor, source, message);

    } else if (tag == BSAL_INPUT_ACTOR_COUNT_RESULT) {

        count = *(int *)bsal_message_buffer(message);
        printf("actor %i: file has %i items\n", name, count);

        bsal_message_set_tag(message, BSAL_INPUT_ACTOR_CLOSE);
        bsal_actor_send(actor, source, message);

        reader_destroy(actor);
    }
}
