
#include "root.h"

#include <biosal.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct thorium_script root_script = {
    .identifier = ROOT_SCRIPT,
    .init = root_init,
    .destroy = root_destroy,
    .receive = root_receive,
    .size = sizeof(struct root),
    .name = "root"
};

void root_init(struct thorium_actor *actor)
{
    struct root *concrete_self;

    concrete_self = (struct root *)thorium_actor_concrete_actor(actor);
    concrete_self->controller = -1;
    concrete_self->events = 0;
    concrete_self->synchronized = 0;
    bsal_vector_init(&concrete_self->spawners, sizeof(int));
    concrete_self->is_king = 0;
    concrete_self->ready = 0;

    thorium_actor_add_script(actor, BSAL_INPUT_CONTROLLER_SCRIPT,
                        &bsal_input_controller_script);
    thorium_actor_add_script(actor, BSAL_MANAGER_SCRIPT,
                        &bsal_manager_script);

    /* TODO: do this one inside the manager script.
*/
thorium_actor_add_script(actor, BSAL_SEQUENCE_STORE_SCRIPT,
                        &bsal_sequence_store_script);
}

void root_destroy(struct thorium_actor *actor)
{
    struct root *concrete_self;

    concrete_self = (struct root *)thorium_actor_concrete_actor(actor);
    bsal_vector_destroy(&concrete_self->spawners);
}

void root_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int source;
    int name;
    int king;
    int argc;
    char **argv;
    int i;
    char *file;
    struct root *concrete_self;
    struct root *concrete_actor;
    char *buffer;
    int bytes;
    int manager;
    struct bsal_vector spawners;
    int new_count;
    void *new_buffer;
    struct thorium_message new_message;
    struct bsal_vector stores;
    int count;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    concrete_self = (struct root *)thorium_actor_concrete_actor(actor);
    concrete_actor = concrete_self;
    source = thorium_message_source(message);
    tag = thorium_message_tag(message);
    buffer = thorium_message_buffer(message);
    name = thorium_actor_name(actor);
    count = thorium_message_count(message);

    /*
    printf(">>root_receive source %d name %d tag %d THORIUM_ACTOR_SYNCHRONIZED is %d\n", source, name, tag,
                    THORIUM_ACTOR_SYNCHRONIZED);
*/
    if (tag == THORIUM_ACTOR_START) {

        bsal_vector_init(&concrete_self->spawners, 0);
        bsal_vector_unpack(&concrete_self->spawners, buffer);


        king = *(int *)bsal_vector_at(&concrete_self->spawners, 0);

        if (name == king) {
            concrete_self->is_king = 1;
        }

        if (concrete_self->is_king) {
/*
            printf("is king\n");
            */
            concrete_self->controller = thorium_actor_spawn(actor, BSAL_INPUT_CONTROLLER_SCRIPT);
            printf("root actor/%d spawned controller actor/%d\n", name, concrete_self->controller);
            thorium_actor_synchronize(actor, &concrete_self->spawners);
            /*
            printf("actor %d synchronizes\n", name);
            */
        }

        concrete_self->ready++;

        if (concrete_self->ready == 2) {

            thorium_actor_send_empty(actor, king, THORIUM_ACTOR_SYNCHRONIZE_REPLY);
        }

    } else if (tag == THORIUM_ACTOR_SYNCHRONIZE) {

            /*
        printf("actor %d receives THORIUM_ACTOR_SYNCHRONIZE\n", name);
*/
        thorium_actor_add_script(actor, BSAL_INPUT_CONTROLLER_SCRIPT,
                        &bsal_input_controller_script);

        /* TODO remove this, BSAL_INPUT_CONTROLLER_SCRIPT should pull
         * its dependencies...
         */
        thorium_actor_add_script(actor, BSAL_INPUT_STREAM_SCRIPT,
                        &bsal_input_stream_script);

        concrete_self->ready++;

        if (concrete_self->ready == 2) {

            king = *(int *)bsal_vector_at(&concrete_self->spawners, 0);
            thorium_actor_send_empty(actor, king, THORIUM_ACTOR_SYNCHRONIZE_REPLY);
        }

    } else if (tag == THORIUM_ACTOR_SYNCHRONIZED) {

        if (concrete_self->synchronized) {
            printf("Error already received THORIUM_ACTOR_SYNCHRONIZED\n");
            return;
        }

        concrete_self->synchronized = 1;
        /*
        printf("actor %d receives THORIUM_ACTOR_SYNCHRONIZED, sending THORIUM_ACTOR_YIELD\n", name);
        */
        thorium_actor_send_to_self_empty(actor, THORIUM_ACTOR_YIELD);

    } else if (tag == THORIUM_ACTOR_YIELD_REPLY) {

        manager = thorium_actor_spawn(actor, BSAL_MANAGER_SCRIPT);

        concrete_actor->manager = manager;

        thorium_actor_send_int(actor, manager, BSAL_MANAGER_SET_SCRIPT, BSAL_SEQUENCE_STORE_SCRIPT);

        printf("DEBUG root actor/%d spawned manager actor/%d\n",
                        thorium_actor_name(actor), manager);

    } else if (tag == BSAL_MANAGER_SET_SCRIPT_REPLY) {

        manager = source;

        bsal_vector_init(&spawners, sizeof(int));

        bsal_vector_push_back_vector(&spawners, &concrete_actor->spawners);

        new_count = bsal_vector_pack_size(&spawners);
        new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);
        bsal_vector_pack(&spawners, new_buffer);

        thorium_message_init(&new_message, THORIUM_ACTOR_START, new_count, new_buffer);
        thorium_actor_send(actor, manager, &new_message);

        bsal_memory_pool_free(ephemeral_memory, new_buffer);

        bsal_vector_destroy(&spawners);

    } else if (tag == THORIUM_ACTOR_START_REPLY
                    && source == concrete_actor->manager) {

        bsal_vector_init(&stores, 0);
        bsal_vector_unpack(&stores, buffer);

        printf("DEBUG root actor/%d received stores from manager actor/%d\n",
                        thorium_actor_name(actor),
                        source);

        thorium_message_init(&new_message, THORIUM_ACTOR_SET_CONSUMERS, count, buffer);
        thorium_actor_send(actor, concrete_self->controller, &new_message);

    } else if (tag == THORIUM_ACTOR_SET_CONSUMERS_REPLY) {

        bytes = bsal_vector_pack_size(&concrete_self->spawners);
        buffer = bsal_memory_pool_allocate(ephemeral_memory, bytes);
        bsal_vector_pack(&concrete_self->spawners, buffer);

        thorium_message_init(message, THORIUM_ACTOR_START, bytes, buffer);
        thorium_actor_send(actor, concrete_self->controller, message);
        bsal_memory_pool_free(ephemeral_memory, buffer);
        buffer = NULL;

        printf("root actor/%d starts controller actor/%d\n", name,
                        concrete_self->controller);

    } else if (tag == THORIUM_ACTOR_START_REPLY) {

        if (!concrete_self->is_king) {
            printf("root actor/%d stops controller actor/%d\n", name, source);

            thorium_actor_send_reply_empty(actor, THORIUM_ACTOR_ASK_TO_STOP);
            return;
        }
/*
        printf("actor %d is king\n", name);
*/
        argc = thorium_actor_argc(actor);
        argv = thorium_actor_argv(actor);

        for (i = 1; i < argc; i++) {
            file = argv[i];

            thorium_message_init(message, BSAL_ADD_FILE, strlen(file) + 1, file);
            thorium_actor_send_reply(actor, message);

            printf("root actor/%d add file %s to controller actor/%d\n", name,
                            file, source);

            concrete_self->events++;
        }

        /* if there are no files,
         * simulate a mock file
         */
        if (concrete_self->events == 0) {
            concrete_self->events++;

            thorium_actor_send_to_self_empty(actor, BSAL_ADD_FILE_REPLY);
        }

        printf("root actor/%d has no more files to add\n", name);

    } else if (tag == BSAL_ADD_FILE_REPLY) {

        concrete_self->events--;

        if (concrete_self->events == 0) {

            printf("root actor/%d asks controller actor/%d to distribute data\n",
                            name, source);

            thorium_actor_send_empty(actor, concrete_self->controller,
                            BSAL_INPUT_DISTRIBUTE);
        }
    } else if (tag == BSAL_INPUT_DISTRIBUTE_REPLY) {

        printf("root actor/%d is notified by controller actor/%d that the distribution is complete\n",
                        name, source);

        thorium_actor_send_reply_empty(actor, THORIUM_ACTOR_ASK_TO_STOP);

    } else if (tag == THORIUM_ACTOR_ASK_TO_STOP_REPLY) {

        printf("DEBUG root receives THORIUM_ACTOR_ASK_TO_STOP_REPLY\n");
        if (source == concrete_self->controller) {

            printf("DEBUG root actor/%d sending to self ROOT_STOP_ALL\n",
                            thorium_actor_name(actor));

            thorium_actor_send_to_self_empty(actor, ROOT_STOP_ALL);

            /* Stop manager and controller
             */

            thorium_actor_send_empty(actor, concrete_self->manager,
                            THORIUM_ACTOR_ASK_TO_STOP);
            thorium_actor_send_empty(actor, concrete_self->controller,
                            THORIUM_ACTOR_ASK_TO_STOP);

        }
    } else if (tag == ROOT_STOP_ALL) {

        printf("root actor/%d stops all other actors\n", name);

        thorium_actor_send_range_empty(actor, &concrete_self->spawners, THORIUM_ACTOR_ASK_TO_STOP);

    } else if (tag == THORIUM_ACTOR_ASK_TO_STOP) {

        thorium_actor_send_empty(actor, concrete_actor->manager, THORIUM_ACTOR_ASK_TO_STOP);

        printf("DEBUG stopping root actor/%d (source: %d)\n", thorium_actor_name(actor),
                        source);
        thorium_actor_send_to_self_empty(actor, THORIUM_ACTOR_STOP);
    }
}

