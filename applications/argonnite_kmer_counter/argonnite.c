
#include "argonnite.h"

#include <structures/vector.h>
#include <structures/vector_iterator.h>

#include <kernels/kmer_counter_kernel.h>
#include <patterns/manager.h>
#include <patterns/aggregator.h>

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
    bsal_vector_init(&concrete_actor->kernels, sizeof(int));
    bsal_vector_init(&concrete_actor->aggregators, sizeof(int));

    bsal_actor_add_script(actor, BSAL_INPUT_CONTROLLER_SCRIPT,
                    &bsal_input_controller_script);
    bsal_actor_add_script(actor, BSAL_KMER_COUNTER_KERNEL_SCRIPT,
                    &bsal_kmer_counter_kernel_script);
    bsal_actor_add_script(actor, BSAL_MANAGER_SCRIPT,
                    &bsal_manager_script);
    bsal_actor_add_script(actor, BSAL_AGGREGATOR_SCRIPT,
                    &bsal_aggregator_script);
}

void argonnite_destroy(struct bsal_actor *actor)
{
    struct argonnite *concrete_actor;

    concrete_actor = (struct argonnite *)bsal_actor_concrete_actor(actor);

    bsal_vector_destroy(&concrete_actor->initial_actors);
    bsal_vector_destroy(&concrete_actor->kernels);
    bsal_vector_destroy(&concrete_actor->aggregators);
}

void argonnite_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    void *buffer;
    struct argonnite *concrete_actor;
    struct bsal_vector initial_actors;
    struct bsal_vector aggregators;
    int *bucket;
    int index;
    int controller;
    int manager_for_kernels;
    int manager_for_aggregators;
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
    int source;
    int kernel;
    int kernel_index;
    int kernel_index_index;
    int aggregator;
    int aggregator_index;
    int aggregator_index_index;
    int kernels_per_aggregator;

    concrete_actor = (struct argonnite *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    count = bsal_message_count(message);
    argc = bsal_actor_argc(actor);
    name = bsal_actor_name(actor);
    source = bsal_message_source(message);

    if (tag == BSAL_ACTOR_START) {

        printf("argonnite actor/%d starts\n", name);

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

        manager_for_kernels = bsal_actor_spawn(actor, BSAL_MANAGER_SCRIPT);
        concrete_actor->manager_for_kernels = bsal_actor_add_acquaintance(actor, manager_for_kernels);

        bsal_actor_helper_send_int(actor, manager_for_kernels, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_KMER_COUNTER_KERNEL_SCRIPT);

    } else if (tag == BSAL_MANAGER_SET_SCRIPT_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_kernels)) {

        bsal_vector_init(&spawners, sizeof(int));

        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->initial_actors, &spawners);

        new_count = bsal_vector_pack_size(&spawners);
        new_buffer = bsal_malloc(new_count);
        bsal_vector_pack(&spawners, new_buffer);

        bsal_message_init(&new_message, BSAL_ACTOR_START, new_count, new_buffer);

        manager_for_kernels = bsal_actor_get_acquaintance(actor, concrete_actor->manager_for_kernels);

        /* ask the manager to spawn BSAL_KMER_COUNTER_KERNEL_SCRIPT actors,
         * these will be the customers of the controller
         */
        bsal_actor_send(actor, manager_for_kernels, &new_message);

        bsal_vector_destroy(&spawners);
        bsal_message_destroy(&new_message);
        bsal_free(new_buffer);

    } else if (tag == BSAL_ACTOR_START_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_kernels)) {

        /* make sure that customers are unpacking correctly
         */
        bsal_vector_unpack(&customers, buffer);

        controller = bsal_actor_get_acquaintance(actor, concrete_actor->controller);

        bsal_message_init(&new_message, BSAL_INPUT_CONTROLLER_SET_CUSTOMERS,
                        count, buffer);
        bsal_actor_send(actor, controller, &new_message);

        /* add kernels to self
         */

        bsal_vector_iterator_init(&iterator, &customers);

        while (bsal_vector_iterator_has_next(&iterator)) {
            bsal_vector_iterator_next(&iterator, (void **)&bucket);
            kernel = *bucket;
            kernel_index = bsal_actor_add_acquaintance(actor, kernel);
            bsal_vector_push_back(&concrete_actor->kernels, &kernel_index);
        }

        bsal_vector_iterator_destroy(&iterator);

        bsal_vector_destroy(&customers);

    } else if (tag == BSAL_INPUT_CONTROLLER_SET_CUSTOMERS_REPLY) {

        bsal_vector_init(&spawners, sizeof(int));
        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->initial_actors, &spawners);

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

            /* spawn the manager for aggregators
             */
            manager_for_aggregators = bsal_actor_spawn(actor,
                            BSAL_MANAGER_SCRIPT);

            printf("argonnite actor/%d spawns manager for aggregators actor/%d\n",
                            bsal_actor_name(actor), manager_for_aggregators);

            concrete_actor->manager_for_aggregators = bsal_actor_add_acquaintance(actor,
                            manager_for_aggregators);

            bsal_actor_helper_send_int(actor, manager_for_aggregators,
                            BSAL_MANAGER_SET_SCRIPT, BSAL_AGGREGATOR_SCRIPT);
        }

    } else if (tag == BSAL_MANAGER_SET_SCRIPT_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_aggregators)) {

        manager_for_aggregators = bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_aggregators);

        printf("argonnite actor/%d sets count per spawner to 1 for manager actor/%d\n",
                        bsal_actor_name(actor), manager_for_aggregators);
        bsal_actor_helper_send_int(actor, manager_for_aggregators,
                            BSAL_MANAGER_SET_ACTORS_PER_SPAWNER, 1);

    } else if (tag == BSAL_MANAGER_SET_ACTORS_PER_SPAWNER_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_aggregators)) {

        /* send spawners to the aggregator manager
         */
        bsal_vector_init(&spawners, sizeof(int));
        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->initial_actors, &spawners);

        bsal_actor_helper_send_reply_vector(actor, BSAL_ACTOR_START,
                        &spawners);

        printf("argonnite actor/%d ask manager actor/%d to spawn children for work\n",
                        bsal_actor_name(actor), manager_for_aggregators);
        bsal_vector_destroy(&spawners);

    } else if (tag == BSAL_ACTOR_START_REPLY &&
                    source == bsal_actor_get_acquaintance(actor, concrete_actor->manager_for_aggregators)) {

        concrete_actor->wired_kernels = 0;

        bsal_vector_unpack(&aggregators, buffer);

        bsal_actor_helper_add_acquaintances(actor, &aggregators, &concrete_actor->aggregators);
        bsal_vector_destroy(&aggregators);

        /*
         * before distributing, wire together the kernels and the aggregators.
         * It is like a brain, with some connections
         */

        bsal_vector_iterator_init(&iterator, &concrete_actor->kernels);

        printf("argonnite actor/%d wires the brain, %d kernels, %d aggregators\n",
                        bsal_actor_name(actor),
                        (int)bsal_vector_size(&concrete_actor->kernels),
                        (int)bsal_vector_size(&concrete_actor->aggregators));

        bsal_vector_helper_print_int(&concrete_actor->aggregators);

        /* TODO, the wiring should use the number of
         * workers per node
         */
        kernels_per_aggregator = (bsal_vector_size(&concrete_actor->kernels) /
                            bsal_vector_size(&concrete_actor->aggregators));

        kernel_index_index = 0;
        while (bsal_vector_iterator_has_next(&iterator)) {
            bsal_vector_iterator_next(&iterator, (void **)&bucket);

            kernel_index = *bucket;
            kernel = bsal_actor_get_acquaintance(actor, kernel_index);

            aggregator_index_index = kernel_index_index / kernels_per_aggregator;

            /* avoid invalid index
             */
            if (aggregator_index_index >= (int)bsal_vector_size(&concrete_actor->aggregators)) {
                aggregator_index_index = (int)bsal_vector_size(&concrete_actor->aggregators) - 1;
            }

            aggregator_index = bsal_vector_helper_at_as_int(&concrete_actor->aggregators, aggregator_index_index);

            aggregator = bsal_actor_get_acquaintance(actor, aggregator_index);

#ifdef ARGONNITE_DEBUG
            printf("argonnite actor/%d set the customer of kernel actor/%d (%d) to aggregator actor/%d (%d)\n",
                            bsal_actor_name(actor), kernel,
                           kernel_index_index, aggregator, aggregator_index_index);
#endif

            bsal_actor_helper_send_int(actor, kernel, BSAL_SET_CUSTOMER, aggregator);

            kernel_index_index++;
        }

        bsal_vector_iterator_destroy(&iterator);

    } else if (tag == BSAL_SET_CUSTOMER_REPLY) {

        concrete_actor->wired_kernels++;

        if (concrete_actor->wired_kernels == (int)bsal_vector_size(&concrete_actor->kernels)) {

            printf("argonnite actor/%d completed the wiring of the brain\n",
                bsal_actor_name(actor));

            bsal_actor_helper_send_empty(actor, bsal_actor_get_acquaintance(actor,
                                    concrete_actor->controller), BSAL_INPUT_DISTRIBUTE);
        }

    } else if (tag == BSAL_INPUT_DISTRIBUTE_REPLY) {

        printf("argonnite actor/%d receives BSAL_INPUT_DISTRIBUTE_REPLY\n",
                        name);

        controller = bsal_actor_get_acquaintance(actor, concrete_actor->controller);
        manager_for_kernels = bsal_actor_get_acquaintance(actor, concrete_actor->manager_for_kernels);
        manager_for_aggregators = bsal_actor_get_acquaintance(actor, concrete_actor->manager_for_aggregators);

        printf("argonnite actor/%d stops controller actor/%d\n",
                        bsal_actor_name(actor), controller);
        bsal_actor_helper_send_empty(actor, controller, BSAL_ACTOR_ASK_TO_STOP);

        printf("argonnite actor/%d stops kernel manager actor/%d\n",
                        bsal_actor_name(actor), manager_for_kernels);
        bsal_actor_helper_send_empty(actor, manager_for_kernels, BSAL_ACTOR_ASK_TO_STOP);

        printf("argonnite actor/%d stops aggregator manager actor/%d\n",
                        bsal_actor_name(actor), manager_for_aggregators);
        bsal_actor_helper_send_empty(actor, manager_for_aggregators, BSAL_ACTOR_ASK_TO_STOP);

        bsal_vector_init(&initial_actors, sizeof(int));
        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->initial_actors, &initial_actors);

        bsal_vector_iterator_init(&iterator, &initial_actors);

        while (bsal_vector_iterator_has_next(&iterator)) {
            bsal_vector_iterator_next(&iterator, (void **)&bucket);

            other_name = *bucket;

            printf("argonnite actor/%d stops argonnite actor/%d\n",
                            name, other_name);

            bsal_actor_helper_send_empty(actor, other_name, BSAL_ACTOR_ASK_TO_STOP);
        }

        bsal_vector_destroy(&initial_actors);

        bsal_vector_iterator_destroy(&iterator);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        printf("argonnite actor/%d stops\n", name);

        bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);
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
