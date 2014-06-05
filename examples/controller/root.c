
#include "root.h"

#include <biosal.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_script root_script = {
    .name = ROOT_SCRIPT,
    .init = root_init,
    .destroy = root_destroy,
    .receive = root_receive,
    .size = sizeof(struct root)
};

void root_init(struct bsal_actor *actor)
{
    struct root *root1;

    root1 = (struct root *)bsal_actor_concrete_actor(actor);
    root1->controller = -1;
    root1->events = 0;
    root1->synchronized = 0;
    bsal_vector_init(&root1->spawners, sizeof(int));
}

void root_destroy(struct bsal_actor *actor)
{
    struct root *root1;

    root1 = (struct root *)bsal_actor_concrete_actor(actor);
    bsal_vector_destroy(&root1->spawners);
}

void root_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int name;
    int king;
    int is_king;
    int argc;
    char **argv;
    int i;
    char *file;
    struct root *root1;
    char *buffer;
    int bytes;

    root1 = (struct root *)bsal_actor_concrete_actor(actor);
    source = bsal_message_source(message);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    name = bsal_actor_name(actor);

    king = 0;

    is_king = 0;

    if (name == king) {
        is_king = 1;
    }

    if (tag == BSAL_ACTOR_START) {

        bsal_vector_unpack(&root1->spawners, buffer);

        bsal_actor_add_script(actor, BSAL_INPUT_CONTROLLER_SCRIPT,
                        &bsal_input_controller_script);

        if (is_king) {
            printf("is king\n");
            root1->controller = bsal_actor_spawn(actor, BSAL_INPUT_CONTROLLER_SCRIPT);
            printf("actor %d spawned controller %d\n", name, root1->controller);
            bsal_actor_synchronize(actor, 0, bsal_vector_size(&root1->spawners) - 1);
            printf("actor %d synchronizes\n", name);
        }

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE) {

        printf("actor %d receives BSAL_ACTOR_SYNCHRONIZE\n", name);

        bsal_actor_add_script(actor, BSAL_INPUT_CONTROLLER_SCRIPT,
                        &bsal_input_controller_script);

        /* TODO remove this, BSAL_INPUT_CONTROLLER_SCRIPT should pull
         * its dependencies...
         */
        bsal_actor_add_script(actor, BSAL_INPUT_STREAM_SCRIPT,
                        &bsal_input_stream_script);

        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_SYNCHRONIZE_REPLY);

    } else if (tag == BSAL_ACTOR_SYNCHRONIZED) {

        if (root1->synchronized) {
            printf("Error already received BSAL_ACTOR_SYNCHRONIZED\n");
            return;
        }

        root1->synchronized = 1;
        printf("actor %d receives BSAL_ACTOR_SYNCHRONIZED, sending ROOT_CONTINUE\n", name);
        bsal_actor_send_to_self_empty(actor, ROOT_CONTINUE);

    } else if (tag == ROOT_CONTINUE) {

        bytes = bsal_vector_pack_size(&root1->spawners);
        buffer = malloc(bytes);
        bsal_vector_pack(&root1->spawners, buffer);

        bsal_message_init(message, BSAL_INPUT_CONTROLLER_START, bytes, buffer);
        bsal_actor_send(actor, root1->controller, message);
        free(buffer);
        buffer = NULL;

        printf("actor %d ROOT_CONTINUE, starting controller %d\n", name,
                        root1->controller);

    } else if (tag == BSAL_INPUT_CONTROLLER_START_REPLY) {

        if (!is_king) {
            printf("actor %d stops controller %d\n", name, source);

            bsal_actor_send_reply_empty(actor, BSAL_INPUT_STOP);
            return;
        }

        printf("actor %d is king\n", name);

        argc = bsal_actor_argc(actor);
        argv = bsal_actor_argv(actor);

        if (argc == 1) {

            bsal_actor_send_to_self_empty(actor, ROOT_STOP_ALL);
            bsal_actor_send_reply_empty(actor, BSAL_INPUT_STOP);
        }

        for (i = 1; i < argc; i++) {
            file = argv[i];

            bsal_message_init(message, BSAL_ADD_FILE, strlen(file) + 1, file);
            bsal_actor_send_reply(actor, message);

            printf("actor %d add file %s to actor %d\n", name,
                            file, source);

            root1->events++;
        }

        printf("actor %i no more files to add\n", name);

    } else if (tag == BSAL_ADD_FILE_REPLY) {

        root1->events--;

        if (root1->events == 0) {

            bsal_actor_send_reply_empty(actor, BSAL_INPUT_DISTRIBUTE);

            printf("actor %d asks actor %d to distribute data\n",
                            name, source);
        }
    } else if (tag == BSAL_INPUT_DISTRIBUTE_REPLY) {

        printf("Actor %d is notified by actor %d that the distribution is complete\n",
                        name, source);

        bsal_actor_send_to_self_empty(actor, ROOT_STOP_ALL);
        bsal_actor_send_reply_empty(actor, BSAL_INPUT_STOP);

    } else if (tag == ROOT_STOP_ALL) {

        printf("actor %d stops all other actors\n", name);

        bsal_actor_send_range_standard_empty(actor, 0, bsal_vector_size(&root1->spawners) - 1, ROOT_DIE);

    } else if (tag == ROOT_DIE) {

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}

