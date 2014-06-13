
#include "argonnite.h"

#include <structures/vector.h>
#include <structures/vector_iterator.h>

#include <kernels/kmer_counter_kernel.h>
#include <patterns/manager.h>

#include <system/memory.h>

#include <stdio.h>
#include <string.h>

struct bsal_script argonnite_script = {
    .name = ARGONNITE_SCRIPT,
    .init = argonnite_init,
    .destroy = argonnite_destroy,
    .receive = argonnite_receive,
    .size = sizeof(struct argonnite)
};

void argonnite_init(struct bsal_actor *actor)
{
    struct argonnite *concrete_actor;

    concrete_actor = (struct argonnite *)bsal_actor_concrete_actor(actor);
    bsal_vector_init(&concrete_actor->initial_actors, sizeof(int));

    bsal_actor_add_script(actor, BSAL_INPUT_CONTROLLER_SCRIPT,
                    &bsal_input_controller_script);
    bsal_actor_add_script(actor, BSAL_KMER_COUNTER_KERNEL_SCRIPT,
                    &bsal_kmer_counter_kernel_script);
    bsal_actor_add_script(actor, BSAL_MANAGER_SCRIPT,
                    &bsal_manager_script);
}

void argonnite_destroy(struct bsal_actor *actor)
{
    struct argonnite *concrete_actor;

    concrete_actor = (struct argonnite *)bsal_actor_concrete_actor(actor);
    bsal_vector_destroy(&concrete_actor->initial_actors);
}

void argonnite_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    void *buffer;
    struct argonnite *concrete_actor;
    struct bsal_vector initial_actors;
    int *bucket;
    int index;
    int controller;
    int manager;
    struct bsal_vector spawners;
    int other_name;
    int new_count;
    void *new_buffer;
    struct bsal_message new_message;
    struct bsal_vector customers;
    int count;
    struct bsal_vector_iterator iterator;
    int argc;
    int name;

    concrete_actor = (struct argonnite *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    count = bsal_message_count(message);
    argc = bsal_actor_argc(actor);
    name = bsal_actor_name(actor);

    if (tag == BSAL_ACTOR_START) {

        printf("argonnite actor%d starts\n", name);

        bsal_vector_unpack(&initial_actors, buffer);

        bsal_vector_iterator_init(&iterator, &initial_actors);

        while (bsal_vector_iterator_has_next(&iterator)) {
            bsal_vector_iterator_next(&iterator, (void **)&bucket);

            index = bsal_actor_add_acquaintance(actor, *bucket);

            bsal_vector_push_back(&concrete_actor->initial_actors,
                            &index);
        }

        bsal_vector_iterator_destroy(&iterator);

        /*
         * Run only one argonnite actor
         */
        if (bsal_vector_index_of(&initial_actors, &name) != 0) {
            return;
        }

        controller = bsal_actor_spawn(actor, BSAL_INPUT_CONTROLLER_SCRIPT);
        concrete_actor->controller = bsal_actor_add_acquaintance(actor, controller);

        manager = bsal_actor_spawn(actor, BSAL_MANAGER_SCRIPT);
        concrete_actor->manager = bsal_actor_add_acquaintance(actor, manager);

        bsal_actor_send_int(actor, manager, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_KMER_COUNTER_KERNEL_SCRIPT);

    } else if (tag == BSAL_MANAGER_SET_SCRIPT_REPLY) {

        bsal_vector_init(&spawners, sizeof(int));

        bsal_actor_get_acquaintances(actor, &concrete_actor->initial_actors, &spawners);

        new_count = bsal_vector_pack_size(&spawners);
        new_buffer = bsal_malloc(new_count);
        bsal_vector_pack(&spawners, new_buffer);

        bsal_message_init(&new_message, BSAL_ACTOR_START, new_count, new_buffer);

        manager = bsal_actor_get_acquaintance(actor, concrete_actor->manager);

        /* ask the manager to spawn BSAL_KMER_COUNTER_KERNEL_SCRIPT actors,
         * these will be the customers of the controller
         */
        bsal_actor_send(actor, manager, &new_message);

        bsal_vector_destroy(&spawners);
        bsal_message_destroy(&new_message);
        bsal_free(new_buffer);

    } else if (tag == BSAL_ACTOR_START_REPLY) {

        /* make sure that customers are unpacking correctly
         */
        bsal_vector_unpack(&customers, buffer);

        controller = bsal_actor_get_acquaintance(actor, concrete_actor->controller);

        bsal_message_init(&new_message, BSAL_INPUT_CONTROLLER_SET_CUSTOMERS,
                        count, buffer);
        bsal_actor_send(actor, controller, &new_message);

    } else if (tag == BSAL_INPUT_CONTROLLER_SET_CUSTOMERS_REPLY) {

        bsal_vector_init(&spawners, sizeof(int));
        bsal_actor_get_acquaintances(actor, &concrete_actor->initial_actors, &spawners);

        new_count = bsal_vector_pack_size(&spawners);
        new_buffer = bsal_malloc(new_count);
        bsal_vector_pack(&spawners, new_buffer);

        bsal_message_init(&new_message, BSAL_INPUT_CONTROLLER_START, new_count, new_buffer);

        controller = bsal_actor_get_acquaintance(actor, concrete_actor->controller);

        bsal_actor_send(actor, controller, &new_message);

        bsal_vector_destroy(&spawners);
        bsal_message_destroy(&new_message);
        bsal_free(new_buffer);

    } else if (tag == BSAL_INPUT_CONTROLLER_START_REPLY) {

        /* add files */
        concrete_actor->argument_iterator = 0;

        if (concrete_actor->argument_iterator < argc) {
            argonnite_add_file(actor, message);
        }
    } else if (tag == BSAL_ADD_FILE_REPLY) {

        if (concrete_actor->argument_iterator < argc) {
            argonnite_add_file(actor, message);
        } else {

            bsal_actor_send_reply_empty(actor, BSAL_INPUT_DISTRIBUTE);
        }
    } else if (tag == BSAL_INPUT_DISTRIBUTE_REPLY) {

        printf("argonnite actor/%d receives BSAL_INPUT_DISTRIBUTE_REPLY\n",
                        name);

        controller = bsal_actor_get_acquaintance(actor, concrete_actor->controller);
        manager = bsal_actor_get_acquaintance(actor, concrete_actor->manager);

        bsal_actor_send_empty(actor, controller, BSAL_ACTOR_ASK_TO_STOP);
        bsal_actor_send_empty(actor, manager, BSAL_ACTOR_ASK_TO_STOP);

        bsal_vector_init(&initial_actors, sizeof(int));
        bsal_actor_get_acquaintances(actor, &concrete_actor->initial_actors, &initial_actors);

        bsal_vector_iterator_init(&iterator, &initial_actors);

        while (bsal_vector_iterator_has_next(&iterator)) {
            bsal_vector_iterator_next(&iterator, (void **)&bucket);

            other_name = *bucket;

            printf("argonnite actor/%d stops argonnite actor/%d\n",
                            name, other_name);

            bsal_actor_send_empty(actor, other_name, BSAL_ACTOR_ASK_TO_STOP);
        }

        bsal_vector_destroy(&initial_actors);

        bsal_vector_iterator_destroy(&iterator);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        printf("argonnite actor/%d stops\n", name);

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}

void argonnite_add_file(struct bsal_actor *actor, struct bsal_message *message)
{
    int controller;
    char *file;
    int argc;
    char **argv;
    struct argonnite *concrete_actor;
    struct bsal_message new_message;

    argc = bsal_actor_argc(actor);
    concrete_actor = (struct argonnite *)bsal_actor_concrete_actor(actor);

    if (concrete_actor->argument_iterator >= argc) {
        return;
    }

    argv = bsal_actor_argv(actor);
    controller = bsal_actor_get_acquaintance(actor, concrete_actor->controller);

    file = argv[concrete_actor->argument_iterator++];

    bsal_message_init(&new_message, BSAL_ADD_FILE, strlen(file) + 1, file);

    bsal_actor_send(actor, controller, &new_message);

}
