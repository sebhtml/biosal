
#include "argonnite.h"

#include <structures/vector.h>
#include <structures/vector_iterator.h>

#include <kernels/kmer_counter_kernel.h>
#include <kernels/kernel_director.h>

#include <patterns/manager.h>
#include <patterns/aggregator.h>

#include <storage/kmer_store.h>

#include <system/memory.h>
#include <system/debugger.h>

#include <stdio.h>
#include <string.h>

/*
#define ARGONNITE_DEBUG
*/

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
    bsal_vector_init(&concrete_actor->directors, sizeof(int));
    bsal_vector_init(&concrete_actor->aggregators, sizeof(int));
    bsal_vector_init(&concrete_actor->stores, sizeof(int));

    bsal_actor_add_script(actor, BSAL_INPUT_CONTROLLER_SCRIPT,
                    &bsal_input_controller_script);
    bsal_actor_add_script(actor, BSAL_KMER_COUNTER_KERNEL_SCRIPT,
                    &bsal_kmer_counter_kernel_script);
    bsal_actor_add_script(actor, BSAL_MANAGER_SCRIPT,
                    &bsal_manager_script);
    bsal_actor_add_script(actor, BSAL_AGGREGATOR_SCRIPT,
                    &bsal_aggregator_script);
    bsal_actor_add_script(actor, BSAL_KERNEL_DIRECTOR_SCRIPT,
                    &bsal_kernel_director_script);
    bsal_actor_add_script(actor, BSAL_KMER_STORE_SCRIPT,
                    &bsal_kmer_store_script);

    concrete_actor->kmer_length = 41;
    concrete_actor->block_size = 512;

    concrete_actor->configured_actors = 0;
    concrete_actor->wired_directors = 0;
    concrete_actor->spawned_stores = 0;
}

void argonnite_destroy(struct bsal_actor *actor)
{
    struct argonnite *concrete_actor;

    concrete_actor = (struct argonnite *)bsal_actor_concrete_actor(actor);

    bsal_vector_destroy(&concrete_actor->initial_actors);
    bsal_vector_destroy(&concrete_actor->directors);
    bsal_vector_destroy(&concrete_actor->aggregators);
    bsal_vector_destroy(&concrete_actor->stores);
}

void argonnite_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    void *buffer;
    int total_actors;
    struct argonnite *concrete_actor;
    struct bsal_vector initial_actors;
    struct bsal_vector aggregators;
    struct bsal_vector directors;
    int *bucket;
    int controller;
    int manager_for_directors;
    int manager_for_aggregators;
    struct bsal_vector spawners;
    int other_name;
    struct bsal_vector customers;
    struct bsal_vector_iterator iterator;
    int argc;
    char **argv;
    int name;
    int source;
    int director;
    int director_index;
    int aggregator;
    int aggregator_index;
    int i;
    int manager_for_stores;
    struct bsal_vector stores;

    concrete_actor = (struct argonnite *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    argc = bsal_actor_argc(actor);
    argv = bsal_actor_argv(actor);
    name = bsal_actor_name(actor);
    source = bsal_message_source(message);

    if (tag == BSAL_ACTOR_START) {

        printf("argonnite actor/%d starts\n", name);

        for (i = 0; i < argc; i++) {
            if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {

                concrete_actor->kmer_length =atoi(argv[i + 1]);
            }
        }

#ifdef ARGONNITE_DEBUG1
        BSAL_DEBUG_MARKER("foo_marker");
#endif

        bsal_vector_unpack(&initial_actors, buffer);

        bsal_actor_helper_add_acquaintances(actor, &initial_actors, &concrete_actor->initial_actors);

        /*
         * Run only one argonnite actor
         */
        if (bsal_vector_helper_at_as_int(&initial_actors, 0) != name) {
            return;
        }

        controller = bsal_actor_spawn(actor, BSAL_INPUT_CONTROLLER_SCRIPT);
        concrete_actor->controller = bsal_actor_get_acquaintance_index(actor, controller);

        manager_for_directors = bsal_actor_spawn(actor, BSAL_MANAGER_SCRIPT);
        concrete_actor->manager_for_directors = bsal_actor_get_acquaintance_index(actor, manager_for_directors);

        bsal_actor_helper_send_int(actor, manager_for_directors, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_KERNEL_DIRECTOR_SCRIPT);

#ifdef ARGONNITE_DEBUG1
        BSAL_DEBUG_MARKER("after setting script");
        printf("manager %d, index %d\n", manager_for_directors, concrete_actor->manager_for_directors);
#endif

    } else if (tag == BSAL_MANAGER_SET_SCRIPT_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_directors)) {

#ifdef ARGONNITE_DEBUG1
        BSAL_DEBUG_MARKER("foo_marker_2");
#endif

        bsal_actor_helper_send_reply_int(actor, BSAL_MANAGER_SET_ACTORS_PER_SPAWNER, 1);

    } else if (tag == BSAL_MANAGER_SET_ACTORS_PER_SPAWNER_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_directors)) {

        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->initial_actors, &spawners);

        bsal_actor_helper_send_reply_vector(actor, BSAL_ACTOR_START, &spawners);

        /* ask the manager to spawn BSAL_KMER_COUNTER_KERNEL_SCRIPT actors,
         * these will be the customers of the controller
         */
        bsal_vector_destroy(&spawners);

    } else if (tag == BSAL_ACTOR_START_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_directors)) {

        /* make sure that customers are unpacking correctly
         */
        bsal_vector_unpack(&customers, buffer);

        controller = bsal_actor_get_acquaintance(actor, concrete_actor->controller);

        bsal_actor_helper_send_vector(actor, controller, BSAL_SET_CUSTOMERS,
                        &customers);

        /* save the directors
         */
        bsal_actor_helper_add_acquaintances(actor, &customers, &concrete_actor->directors);

        bsal_vector_destroy(&customers);

    } else if (tag == BSAL_SET_CUSTOMERS_REPLY
                    && source == bsal_actor_get_acquaintance(actor, concrete_actor->controller)) {

        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->initial_actors, &spawners);

        bsal_actor_helper_send_reply_vector(actor, BSAL_INPUT_CONTROLLER_START, &spawners);

        bsal_vector_destroy(&spawners);

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

            concrete_actor->manager_for_aggregators = bsal_actor_get_acquaintance_index(actor,
                            manager_for_aggregators);

            bsal_actor_helper_send_int(actor, manager_for_aggregators,
                            BSAL_MANAGER_SET_SCRIPT, BSAL_AGGREGATOR_SCRIPT);

#ifdef ARGONNITE_DEBUG2
            BSAL_DEBUG_MARKER("set aggregator script");
#endif

        }

    } else if (tag == BSAL_MANAGER_SET_SCRIPT_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_aggregators)) {

        manager_for_aggregators = bsal_actor_get_acquaintance(actor,
                        concrete_actor->manager_for_aggregators);

#ifdef ARGONNITE_DEBUG2
            BSAL_DEBUG_MARKER("set actors per spawner ");
#endif

        printf("argonnite actor/%d sets count per spawner to 1 for manager actor/%d\n",
                        bsal_actor_name(actor), manager_for_aggregators);

        bsal_actor_helper_send_reply_int(actor,
                            BSAL_MANAGER_SET_ACTORS_PER_SPAWNER, 1);

    } else if (tag == BSAL_MANAGER_SET_ACTORS_PER_SPAWNER_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_aggregators)) {

        manager_for_aggregators = bsal_actor_get_acquaintance(actor,
                        concrete_actor->manager_for_aggregators);

        /* send spawners to the aggregator manager
         */
        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->initial_actors, &spawners);

        bsal_actor_helper_send_reply_vector(actor, BSAL_ACTOR_START,
                        &spawners);

        printf("argonnite actor/%d ask manager actor/%d to spawn children for work\n",
                        bsal_actor_name(actor), manager_for_aggregators);

        bsal_vector_destroy(&spawners);

    } else if (tag == BSAL_ACTOR_START_REPLY &&
                    source == bsal_actor_get_acquaintance(actor, concrete_actor->manager_for_aggregators)) {

        concrete_actor->wired_directors = 0;

        bsal_vector_unpack(&aggregators, buffer);

        bsal_actor_helper_add_acquaintances(actor, &aggregators, &concrete_actor->aggregators);
        bsal_vector_destroy(&aggregators);

        /*
         * before distributing, wire together the directors and the aggregators.
         * It is like a brain, with some connections
         */

        printf("argonnite actor/%d wires the brain, %d directors, %d aggregators\n",
                        bsal_actor_name(actor),
                        (int)bsal_vector_size(&concrete_actor->directors),
                        (int)bsal_vector_size(&concrete_actor->aggregators));

        for (i = 0; i < bsal_vector_size(&concrete_actor->directors); i++) {

            director_index = bsal_vector_helper_at_as_int(&concrete_actor->directors, i);
            aggregator_index = bsal_vector_helper_at_as_int(&concrete_actor->aggregators, i);

            director = bsal_actor_get_acquaintance(actor, director_index);
            aggregator = bsal_actor_get_acquaintance(actor, aggregator_index);

            bsal_actor_helper_send_int(actor, director, BSAL_SET_CUSTOMER, aggregator);

#ifdef ARGONNITE_DEBUG3
            printf("wiring  director %d to aggregator %d (%d)\n",
                            director, aggregator, i);
#endif
        }

#ifdef ARGONNITE_DEBUG3
            BSAL_DEBUG_MARKER("after loop");
#endif

    } else if (tag == BSAL_SET_CUSTOMER_REPLY) {

        concrete_actor->wired_directors++;

        if (concrete_actor->wired_directors == (int)bsal_vector_size(&concrete_actor->directors)) {

            printf("argonnite actor/%d completed the wiring of the brain\n",
                bsal_actor_name(actor));

            concrete_actor->configured_actors = 0;

            bsal_actor_helper_get_acquaintances(actor, &concrete_actor->directors, &directors);
            bsal_actor_helper_get_acquaintances(actor, &concrete_actor->aggregators, &aggregators);

            bsal_actor_helper_send_range_int(actor, &directors, BSAL_SET_KMER_LENGTH, concrete_actor->kmer_length);
            bsal_actor_helper_send_range_int(actor, &aggregators, BSAL_SET_KMER_LENGTH, concrete_actor->kmer_length);

            bsal_vector_destroy(&directors);
            bsal_vector_destroy(&aggregators);
        }

    } else if (tag == BSAL_SET_KMER_LENGTH_REPLY
                    && concrete_actor->spawned_stores == 0) {

        concrete_actor->configured_actors++;

        total_actors = bsal_vector_size(&concrete_actor->aggregators) +
                                bsal_vector_size(&concrete_actor->directors);

        if (concrete_actor->configured_actors == total_actors) {

            bsal_actor_helper_send_int(actor, bsal_actor_get_acquaintance(actor,
                                    concrete_actor->controller), BSAL_SET_BLOCK_SIZE,
                            concrete_actor->block_size);

        }
    } else if (tag == BSAL_SET_BLOCK_SIZE_REPLY) {

        manager_for_stores = bsal_actor_spawn(actor, BSAL_MANAGER_SCRIPT);

        concrete_actor->manager_for_stores = bsal_actor_get_acquaintance_index(actor, manager_for_stores);

        printf("DEBUG manager_for_stores %d\n", concrete_actor->manager_for_stores);

        bsal_actor_helper_send_int(actor, manager_for_stores, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_KMER_STORE_SCRIPT);

    } else if (tag == BSAL_MANAGER_SET_SCRIPT_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_stores)) {

        bsal_actor_helper_send_reply_int(actor, BSAL_MANAGER_SET_ACTORS_PER_WORKER,
                        4);

    } else if (tag == BSAL_MANAGER_SET_ACTORS_PER_WORKER_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_stores)) {

        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->initial_actors,
                        &spawners);
        bsal_actor_helper_send_reply_vector(actor, BSAL_ACTOR_START, &spawners);

        bsal_vector_destroy(&spawners);

    } else if (tag == BSAL_ACTOR_START_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_stores)) {

        concrete_actor->spawned_stores = 1;

        bsal_vector_unpack(&stores, buffer);
        bsal_actor_helper_add_acquaintances(actor, &stores, &concrete_actor->stores);

        concrete_actor->configured_aggregators = 0;

        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->aggregators,
                    &aggregators);

        bsal_actor_helper_send_range_vector(actor, &aggregators, BSAL_SET_CUSTOMERS,
                &stores);

        bsal_vector_destroy(&aggregators);
        bsal_vector_destroy(&stores);

    } else if (tag == BSAL_SET_CUSTOMERS_REPLY) {
        /*
         * received a reply from one of the aggregators.
         */

        concrete_actor->configured_aggregators++;

        if (concrete_actor->configured_aggregators == bsal_vector_size(&concrete_actor->aggregators)) {

            concrete_actor->configured_actors = 0;

            bsal_actor_helper_get_acquaintances(actor, &concrete_actor->stores, &stores);

            bsal_actor_helper_send_range_int(actor, &stores, BSAL_SET_KMER_LENGTH,
                            concrete_actor->kmer_length);

            bsal_vector_destroy(&stores);
        }

    } else if (tag == BSAL_SET_KMER_LENGTH_REPLY
                    && concrete_actor->spawned_stores == 1) {

        concrete_actor->configured_actors++;

        if (concrete_actor->configured_actors == bsal_vector_size(&concrete_actor->stores)) {
            bsal_actor_helper_send_empty(actor, bsal_actor_get_acquaintance(actor,
                                    concrete_actor->controller), BSAL_INPUT_DISTRIBUTE);
        }

    } else if (tag == BSAL_INPUT_DISTRIBUTE_REPLY) {

#ifdef ARGONNITE_DEBUG
        printf("argonnite actor/%d receives BSAL_INPUT_DISTRIBUTE_REPLY\n",
                        name);
#endif

        controller = bsal_actor_get_acquaintance(actor, concrete_actor->controller);
        manager_for_directors = bsal_actor_get_acquaintance(actor, concrete_actor->manager_for_directors);
        manager_for_aggregators = bsal_actor_get_acquaintance(actor, concrete_actor->manager_for_aggregators);
        manager_for_stores = bsal_actor_get_acquaintance(actor, concrete_actor->manager_for_stores);

        printf("argonnite actor/%d stops controller actor/%d\n",
                        bsal_actor_name(actor), controller);
        bsal_actor_helper_send_empty(actor, controller, BSAL_ACTOR_ASK_TO_STOP);

        printf("argonnite actor/%d stops director manager actor/%d\n",
                        bsal_actor_name(actor), manager_for_directors);
        bsal_actor_helper_send_empty(actor, manager_for_directors, BSAL_ACTOR_ASK_TO_STOP);

        printf("argonnite actor/%d stops aggregator manager actor/%d\n",
                        bsal_actor_name(actor), manager_for_aggregators);
        bsal_actor_helper_send_empty(actor, manager_for_aggregators, BSAL_ACTOR_ASK_TO_STOP);

        printf("argonnite actor/%d stops store manager actor/%d\n",
                        bsal_actor_name(actor), manager_for_stores);
        bsal_actor_helper_send_empty(actor, manager_for_stores, BSAL_ACTOR_ASK_TO_STOP);

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
