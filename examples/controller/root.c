
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
    root1->is_king = 0;
    root1->ready = 0;

    bsal_actor_add_script(actor, BSAL_INPUT_CONTROLLER_SCRIPT,
                        &bsal_input_controller_script);
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

    /*
    printf(">>root_receive source %d name %d tag %d BSAL_ACTOR_SYNCHRONIZED is %d\n", source, name, tag,
                    BSAL_ACTOR_SYNCHRONIZED);
*/
    if (tag == BSAL_ACTOR_START) {

        bsal_vector_unpack(&root1->spawners, buffer);


        king = *(int *)bsal_vector_at(&root1->spawners, 0);

        if (name == king) {
            root1->is_king = 1;
        }

        if (root1->is_king) {
/*
            printf("is king\n");
            */
            root1->controller = bsal_actor_spawn(actor, BSAL_INPUT_CONTROLLER_SCRIPT);
            printf("root actor/%d spawned controller actor/%d\n", name, root1->controller);
            bsal_actor_synchronize(actor, &root1->spawners);
            /*
            printf("actor %d synchronizes\n", name);
            */
        }

        root1->ready++;

        if (root1->ready == 2) {

            bsal_actor_send_empty(actor, king, BSAL_ACTOR_SYNCHRONIZE_REPLY);
        }

    } else if (tag == BSAL_ACTOR_SYNCHRONIZE) {

            /*
        printf("actor %d receives BSAL_ACTOR_SYNCHRONIZE\n", name);
*/
        bsal_actor_add_script(actor, BSAL_INPUT_CONTROLLER_SCRIPT,
                        &bsal_input_controller_script);

        /* TODO remove this, BSAL_INPUT_CONTROLLER_SCRIPT should pull
         * its dependencies...
         */
        bsal_actor_add_script(actor, BSAL_INPUT_STREAM_SCRIPT,
                        &bsal_input_stream_script);

        root1->ready++;

        if (root1->ready == 2) {

            king = *(int *)bsal_vector_at(&root1->spawners, 0);
            bsal_actor_send_empty(actor, king, BSAL_ACTOR_SYNCHRONIZE_REPLY);
        }

    } else if (tag == BSAL_ACTOR_SYNCHRONIZED) {

        if (root1->synchronized) {
            printf("Error already received BSAL_ACTOR_SYNCHRONIZED\n");
            return;
        }

        root1->synchronized = 1;
        /*
        printf("actor %d receives BSAL_ACTOR_SYNCHRONIZED, sending BSAL_ACTOR_YIELD\n", name);
        */
        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_YIELD);

    } else if (tag == BSAL_ACTOR_YIELD_REPLY) {

        bytes = bsal_vector_pack_size(&root1->spawners);
        buffer = malloc(bytes);
        bsal_vector_pack(&root1->spawners, buffer);

        bsal_message_init(message, BSAL_INPUT_CONTROLLER_START, bytes, buffer);
        bsal_actor_send(actor, root1->controller, message);
        free(buffer);
        buffer = NULL;

        printf("root actor/%d starts controller actor/%d\n", name,
                        root1->controller);

    } else if (tag == BSAL_INPUT_CONTROLLER_START_REPLY) {

        if (!root1->is_king) {
            printf("root actor/%d stops controller actor/%d\n", name, source);

            bsal_actor_send_reply_empty(actor, BSAL_ACTOR_ASK_TO_STOP);
            return;
        }
/*
        printf("actor %d is king\n", name);
*/
        argc = bsal_actor_argc(actor);
        argv = bsal_actor_argv(actor);

        for (i = 1; i < argc; i++) {
            file = argv[i];

            bsal_message_init(message, BSAL_ADD_FILE, strlen(file) + 1, file);
            bsal_actor_send_reply(actor, message);

            printf("root actor/%d add file %s to controller actor/%d\n", name,
                            file, source);

            root1->events++;
        }

        /* if there are no files,
         * simulate a mock file
         */
        if (root1->events == 0) {
            root1->events++;

            bsal_actor_send_to_self_empty(actor, BSAL_ADD_FILE_REPLY);
        }

        printf("root actor/%d has no more files to add\n", name);

    } else if (tag == BSAL_ADD_FILE_REPLY) {

        root1->events--;

        if (root1->events == 0) {

            printf("root actor/%d asks controller actor/%d to distribute data\n",
                            name, source);

            bsal_actor_send_empty(actor, root1->controller,
                            BSAL_INPUT_DISTRIBUTE);
        }
    } else if (tag == BSAL_INPUT_DISTRIBUTE_REPLY) {

        printf("root actor/%d is notified by controller actor/%d that the distribution is complete\n",
                        name, source);

        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_ASK_TO_STOP);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP_REPLY) {

        printf("DEBUG root receives BSAL_ACTOR_ASK_TO_STOP_REPLY\n");
        if (source == root1->controller) {

            printf("DEBUG root actor/%d sending to self ROOT_STOP_ALL\n",
                            bsal_actor_name(actor));

            bsal_actor_send_to_self_empty(actor, ROOT_STOP_ALL);
        }
    } else if (tag == ROOT_STOP_ALL) {

        printf("root actor/%d stops all other actors\n", name);

        bsal_actor_send_range_standard_empty(actor, &root1->spawners, BSAL_ACTOR_ASK_TO_STOP);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        printf("DEBUG stopping root actor/%d (source: %d)\n", bsal_actor_name(actor),
                        source);
        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}

