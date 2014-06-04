
#include "root.h"

#include <biosal.h>

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
}

void root_destroy(struct bsal_actor *actor)
{
}

void root_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int nodes;
    int name;
    int king;
    int is_king;
    int argc;
    char **argv;
    int i;
    char *file;
    struct root *root1;

    root1 = (struct root *)bsal_actor_concrete_actor(actor);
    source = bsal_message_source(message);
    tag = bsal_message_tag(message);
    nodes = bsal_actor_nodes(actor);
    name = bsal_actor_name(actor);

    king = nodes / 2;

    is_king = 0;

    if (name == king) {
        is_king = 1;
    }

    if (tag == BSAL_ACTOR_START) {

        bsal_actor_add_script(actor, BSAL_INPUT_CONTROLLER_SCRIPT,
                        &bsal_input_controller_script);

        if (is_king) {
            root1->controller = bsal_actor_spawn(actor, BSAL_INPUT_CONTROLLER_SCRIPT);
            printf("actor %d spawned controller %d\n", name, root1->controller);
            bsal_actor_synchronize(actor, 0, nodes - 1);
            printf("actor %d synchronizes\n", name);
        }

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE) {

        bsal_actor_add_script(actor, BSAL_INPUT_CONTROLLER_SCRIPT,
                        &bsal_input_controller_script);

        /* TODO remove this, BSAL_INPUT_CONTROLLER_SCRIPT should pull
         * its dependencies...
         */
        bsal_actor_add_script(actor, BSAL_INPUT_STREAM_SCRIPT,
                        &bsal_input_stream_script);

        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_SYNCHRONIZE_REPLY);

    } else if (tag == BSAL_ACTOR_SYNCHRONIZED) {

        printf("actor %d synchronized\n", name);
        bsal_actor_send_to_self_empty(actor, ROOT_CONTINUE);

    } else if (tag == ROOT_CONTINUE) {

        bsal_actor_send_empty(actor, root1->controller, BSAL_INPUT_CONTROLLER_START);

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

        bsal_actor_send_range_standard_empty(actor, 0, nodes - 1, ROOT_DIE);

    } else if (tag == ROOT_DIE) {

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}

