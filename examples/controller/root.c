
#include "root.h"

#include <biosal.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void root_init(struct thorium_actor *self);
void root_destroy(struct thorium_actor *self);
void root_receive(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script root_script = {
    .identifier = SCRIPT_ROOT,
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
    core_vector_init(&concrete_self->spawners, sizeof(int));
    concrete_self->is_king = 0;
    concrete_self->ready = 0;

    thorium_actor_add_script(actor, SCRIPT_INPUT_CONTROLLER,
                        &biosal_input_controller_script);
    thorium_actor_add_script(actor, SCRIPT_MANAGER,
                        &core_manager_script);

    /* TODO: do this one inside the manager script.
     */
    thorium_actor_add_script(actor, SCRIPT_SEQUENCE_STORE,
                        &biosal_sequence_store_script);
    thorium_actor_add_script(actor, SCRIPT_INPUT_CONTROLLER,
                        &biosal_input_controller_script);

    /* TODO remove this, SCRIPT_INPUT_CONTROLLER should pull
     * its dependencies...
     */
    thorium_actor_add_script(actor, SCRIPT_INPUT_STREAM,
                        &biosal_input_stream_script);
}

void root_destroy(struct thorium_actor *actor)
{
    struct root *concrete_self;

    concrete_self = (struct root *)thorium_actor_concrete_actor(actor);
    core_vector_destroy(&concrete_self->spawners);
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
    struct core_vector spawners;
    int new_count;
    void *new_buffer;
    struct thorium_message new_message;
    struct core_vector stores;
    int count;

    concrete_self = (struct root *)thorium_actor_concrete_actor(actor);
    concrete_actor = concrete_self;
    source = thorium_message_source(message);
    tag = thorium_message_action(message);
    buffer = thorium_message_buffer(message);
    name = thorium_actor_name(actor);
    count = thorium_message_count(message);

    /*
    printf(">>root_receive source %d name %d tag %d ACTION_SYNCHRONIZED is %d\n", source, name, tag,
                    ACTION_SYNCHRONIZED);
*/
    if (tag == ACTION_START) {

        core_vector_init(&concrete_self->spawners, 0);
        core_vector_unpack(&concrete_self->spawners, buffer);


        king = *(int *)core_vector_at(&concrete_self->spawners, 0);

        if (name == king) {
            concrete_self->is_king = 1;
        }

        if (concrete_self->is_king) {
/*
            printf("is king\n");
            */
            concrete_self->controller = thorium_actor_spawn(actor, SCRIPT_INPUT_CONTROLLER);
            printf("root actor/%d spawned controller actor/%d\n", name, concrete_self->controller);
            thorium_actor_synchronize(actor, &concrete_self->spawners);
            /*
            printf("actor %d synchronizes\n", name);
            */
        }

        concrete_self->ready++;

        if (concrete_self->ready == 2) {

            thorium_actor_send_empty(actor, king, ACTION_SYNCHRONIZE_REPLY);
        }

    } else if (tag == ACTION_SYNCHRONIZE) {

            /*
        printf("actor %d receives ACTION_SYNCHRONIZE\n", name);
*/
        concrete_self->ready++;

        if (concrete_self->ready == 2) {

            king = *(int *)core_vector_at(&concrete_self->spawners, 0);
            thorium_actor_send_empty(actor, king, ACTION_SYNCHRONIZE_REPLY);
        }

    } else if (tag == ACTION_SYNCHRONIZED) {

        if (concrete_self->synchronized) {
            printf("Error already received ACTION_SYNCHRONIZED\n");
            return;
        }

        concrete_self->synchronized = 1;
        /*
        printf("actor %d receives ACTION_SYNCHRONIZED, sending ACTION_YIELD\n", name);
        */
        thorium_actor_send_to_self_empty(actor, ACTION_YIELD);

    } else if (tag == ACTION_YIELD_REPLY) {

        manager = thorium_actor_spawn(actor, SCRIPT_MANAGER);

        concrete_actor->manager = manager;

        thorium_actor_send_int(actor, manager, ACTION_MANAGER_SET_SCRIPT, SCRIPT_SEQUENCE_STORE);

        printf("DEBUG root actor/%d spawned manager actor/%d\n",
                        thorium_actor_name(actor), manager);

    } else if (tag == ACTION_MANAGER_SET_SCRIPT_REPLY) {

        manager = source;

        core_vector_init(&spawners, sizeof(int));

        core_vector_push_back_vector(&spawners, &concrete_actor->spawners);

        new_count = core_vector_pack_size(&spawners);
        new_buffer = thorium_actor_allocate(actor, new_count);
        core_vector_pack(&spawners, new_buffer);

        thorium_message_init(&new_message, ACTION_START, new_count, new_buffer);
        thorium_actor_send(actor, manager, &new_message);

        core_vector_destroy(&spawners);

    } else if (tag == ACTION_START_REPLY
                    && source == concrete_actor->manager) {

        core_vector_init(&stores, 0);
        core_vector_unpack(&stores, buffer);

        printf("DEBUG root actor/%d received stores from manager actor/%d\n",
                        thorium_actor_name(actor),
                        source);

        thorium_message_init(&new_message, ACTION_SET_CONSUMERS, count, buffer);
        thorium_actor_send(actor, concrete_self->controller, &new_message);

    } else if (tag == ACTION_SET_CONSUMERS_REPLY) {

        bytes = core_vector_pack_size(&concrete_self->spawners);
        buffer = thorium_actor_allocate(actor, bytes);
        core_vector_pack(&concrete_self->spawners, buffer);

        thorium_message_init(message, ACTION_START, bytes, buffer);
        thorium_actor_send(actor, concrete_self->controller, message);
        buffer = NULL;

        printf("root actor/%d starts controller actor/%d\n", name,
                        concrete_self->controller);

    } else if (tag == ACTION_START_REPLY) {

        if (!concrete_self->is_king) {
            printf("root actor/%d stops controller actor/%d\n", name, source);

            thorium_actor_send_reply_empty(actor, ACTION_ASK_TO_STOP);
            return;
        }
/*
        printf("actor %d is king\n", name);
*/
        argc = thorium_actor_argc(actor);
        argv = thorium_actor_argv(actor);

        for (i = 1; i < argc; i++) {
            file = argv[i];

            thorium_message_init(&new_message, ACTION_ADD_FILE, strlen(file) + 1, file);
            thorium_actor_send_reply(actor, &new_message);
            thorium_message_destroy(&new_message);

            printf("root actor/%d add file %s to controller actor/%d\n", name,
                            file, source);

            concrete_self->events++;
        }

        /* if there are no files,
         * simulate a mock file
         */
        if (concrete_self->events == 0) {
            concrete_self->events++;

            thorium_actor_send_to_self_empty(actor, ACTION_ADD_FILE_REPLY);
        }

        printf("root actor/%d has no more files to add (events %d)\n", name,
                        concrete_self->events);

    } else if (tag == ACTION_ADD_FILE_REPLY) {

        concrete_self->events--;

        printf("DEBUG receives ACTION_ADD_FILE_REPLY\n");

        if (concrete_self->events == 0) {

            printf("root actor/%d asks controller actor/%d to distribute data\n",
                            name, source);

            thorium_actor_send_empty(actor, concrete_self->controller,
                            ACTION_INPUT_DISTRIBUTE);
        }
    } else if (tag == ACTION_INPUT_DISTRIBUTE_REPLY) {

        printf("root actor/%d is notified by controller actor/%d that the distribution is complete\n",
                        name, source);

        thorium_actor_send_reply_empty(actor, ACTION_ASK_TO_STOP);

    } else if (tag == ACTION_ASK_TO_STOP_REPLY) {

        printf("DEBUG root receives ACTION_ASK_TO_STOP_REPLY\n");
        if (source == concrete_self->controller) {

            printf("DEBUG root actor/%d sending to self ACTION_ROOT_STOP_ALL\n",
                            thorium_actor_name(actor));

            thorium_actor_send_to_self_empty(actor, ACTION_ROOT_STOP_ALL);

            /* Stop manager and controller
             */

            thorium_actor_send_empty(actor, concrete_self->manager,
                            ACTION_ASK_TO_STOP);
            thorium_actor_send_empty(actor, concrete_self->controller,
                            ACTION_ASK_TO_STOP);

        }
    } else if (tag == ACTION_ROOT_STOP_ALL) {

        printf("root actor/%d stops all other actors\n", name);

        thorium_actor_send_range_empty(actor, &concrete_self->spawners, ACTION_ASK_TO_STOP);

    } else if (tag == ACTION_ASK_TO_STOP) {

        thorium_actor_send_empty(actor, concrete_actor->manager, ACTION_ASK_TO_STOP);

        printf("DEBUG stopping root actor/%d (source: %d)\n", thorium_actor_name(actor),
                        source);
        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
}

