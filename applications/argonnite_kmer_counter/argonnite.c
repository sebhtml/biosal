
#include "argonnite.h"

#include <genomics/kernels/dna_kmer_counter_kernel.h>
#include <genomics/kernels/aggregator.h>

#include <genomics/data/coverage_distribution.h>
#include <genomics/storage/kmer_store.h>

#include <core/structures/vector.h>
#include <core/structures/vector_iterator.h>

#include <core/helpers/set_helper.h>
#include <core/patterns/manager.h>

#include <core/system/memory.h>
#include <core/system/command.h>
#include <core/system/debugger.h>

#include <stdio.h>
#include <string.h>

#include <inttypes.h>

/*
#define ARGONNITE_DEBUG
*/

#define ARGONNITE_DEFAULT_KMER_LENGTH 41

/*
#define ARGONNITE_WORKERS_PER_AGGREGATOR 4
*/

#define ARGONNITE_KMER_STORES_PER_WORKER 1

#define ARGONNITE_STATE_NONE 0
#define ARGONNITE_STATE_PREPARE_SEQUENCE_STORES 1

struct thorium_script argonnite_script = {
    .identifier = SCRIPT_ARGONNITE,
    .init = argonnite_init,
    .destroy = argonnite_destroy,
    .receive = argonnite_receive,
    .size = sizeof(struct argonnite),
    .name = "argonnite"
};

void argonnite_init(struct thorium_actor *actor)
{
    struct argonnite *concrete_actor;

    concrete_actor = (struct argonnite *)thorium_actor_concrete_actor(actor);
    core_vector_init(&concrete_actor->initial_actors, sizeof(int));
    core_vector_init(&concrete_actor->kernels, sizeof(int));
    core_vector_init(&concrete_actor->aggregators, sizeof(int));
    core_vector_init(&concrete_actor->kmer_stores, sizeof(int));
    core_vector_init(&concrete_actor->sequence_stores, sizeof(int));
    core_vector_init(&concrete_actor->worker_counts, sizeof(int));

    core_timer_init(&concrete_actor->timer);
    core_timer_init(&concrete_actor->timer_for_kmers);
    core_map_init(&concrete_actor->plentiful_stores, sizeof(int), sizeof(int));

    thorium_actor_add_script(actor, SCRIPT_INPUT_CONTROLLER,
                    &biosal_input_controller_script);
    thorium_actor_add_script(actor, SCRIPT_DNA_KMER_COUNTER_KERNEL,
                    &biosal_dna_kmer_counter_kernel_script);
    thorium_actor_add_script(actor, SCRIPT_MANAGER,
                    &core_manager_script);
    thorium_actor_add_script(actor, SCRIPT_AGGREGATOR,
                    &biosal_aggregator_script);
    thorium_actor_add_script(actor, SCRIPT_KMER_STORE,
                    &biosal_kmer_store_script);
    thorium_actor_add_script(actor, SCRIPT_SEQUENCE_STORE,
                    &biosal_sequence_store_script);
    thorium_actor_add_script(actor, SCRIPT_COVERAGE_DISTRIBUTION,
                    &biosal_coverage_distribution_script);

    concrete_actor->kmer_length = ARGONNITE_DEFAULT_KMER_LENGTH;
    concrete_actor->not_ready_warnings = 0;

    /* The number of input sequences per I/O block.
     *
     *
     */
    concrete_actor->block_size = 16 * 4096;
    /*concrete_actor->block_size = 4096;*/

    /*concrete_actor->block_size = 2048;*/

    concrete_actor->configured_actors = 0;
    concrete_actor->wired_kernels = 0;
    concrete_actor->spawned_stores = 0;
    concrete_actor->wiring_distribution = 0;

    concrete_actor->ready_kernels = 0;
    concrete_actor->finished_kernels = 0;
    concrete_actor->total_kmers = 0;

    thorium_actor_add_action(actor, ACTION_ARGONNITE_PREPARE_SEQUENCE_STORES,
                    argonnite_prepare_sequence_stores);
    thorium_actor_add_action(actor, ACTION_INPUT_DISTRIBUTE_REPLY,
                    argonnite_connect_kernels_with_stores);
    thorium_actor_add_action(actor, ACTION_SEQUENCE_STORE_REQUEST_PROGRESS_REPLY,
                    argonnite_request_progress_reply);

    concrete_actor->state = ARGONNITE_STATE_NONE;

}

void argonnite_destroy(struct thorium_actor *actor)
{
    struct argonnite *concrete_actor;

    concrete_actor = (struct argonnite *)thorium_actor_concrete_actor(actor);

    core_vector_destroy(&concrete_actor->initial_actors);
    core_vector_destroy(&concrete_actor->kernels);
    core_vector_destroy(&concrete_actor->aggregators);
    core_vector_destroy(&concrete_actor->kmer_stores);
    core_vector_destroy(&concrete_actor->sequence_stores);
    core_vector_destroy(&concrete_actor->worker_counts);

    core_timer_destroy(&concrete_actor->timer);
    core_timer_destroy(&concrete_actor->timer_for_kmers);
    core_map_destroy(&concrete_actor->plentiful_stores);
}

void argonnite_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    void *buffer;
    int total_actors;
    struct argonnite *concrete_actor;
    int *bucket;
    int controller;
    int manager_for_kernels;
    int manager_for_aggregators;
    int distribution;
    int other_name;
    struct core_vector_iterator iterator;
    int argc;
    char **argv;
    int name;
    int source;
    int kernel;
    int kernel_index;
    int aggregator;
    int aggregator_index;
    int i;
    int manager_for_kmer_stores;
    struct core_vector kmer_stores;
    int spawner;
    uint64_t produced;
    int workers;
    int kernel_index_index;
    int spawner_index;
    int sequence_store_index;
    int sequence_store_index_index;
    int sequence_store;
    int other_kernel;
    int print_stuff;
    int aggregator_index_index;
    int enable_work_stealing;

    if (thorium_actor_take_action(actor, message)) {
        return;
    }

    concrete_actor = (struct argonnite *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_action(message);
    buffer = thorium_message_buffer(message);
    argc = thorium_actor_argc(actor);
    argv = thorium_actor_argv(actor);
    name = thorium_actor_name(actor);
    source = thorium_message_source(message);

    if (tag == ACTION_START) {

            /* Don't register for auto scaling here.
             * Kernels do this instead.
             */
            /*
        thorium_actor_send_to_self_empty(actor,
                        ACTION_ENABLE_AUTO_SCALING);
                        */

        concrete_actor->kmer_length = core_command_get_kmer_length(argc, argv);

#ifdef ARGONNITE_DEBUG1
        BIOSAL_DEBUG_MARKER("foo_marker");
#endif

        core_vector_unpack(&concrete_actor->initial_actors, buffer);

        concrete_actor->is_boss = 0;

        if (core_vector_at_as_int(&concrete_actor->initial_actors, 0) == name) {
            concrete_actor->is_boss = 1;
        }

        /* help page
         */
        if (argc == 1
                        || (argc == 2 && strstr(argv[1], "help") != NULL)) {

            if (concrete_actor->is_boss) {
                argonnite_help(actor);
            }

            thorium_actor_ask_to_stop(actor, message);
            return;
        }

        printf("argonnite %d starts\n", name);

        /*
         * Run only one argonnite actor
         */
        if (!concrete_actor->is_boss) {
            return;
        }

        core_timer_start(&concrete_actor->timer);

        controller = thorium_actor_spawn(actor, SCRIPT_INPUT_CONTROLLER);
        concrete_actor->controller = controller;

        manager_for_kernels = thorium_actor_spawn(actor, SCRIPT_MANAGER);
        concrete_actor->manager_for_kernels = manager_for_kernels;

#ifdef ARGONNITE_DEBUG1
        BIOSAL_DEBUG_MARKER("after setting script");
        printf("manager %d, index %d\n", manager_for_directors, concrete_actor->manager_for_directors);
#endif

        for (i = 0; i < core_vector_size(&concrete_actor->initial_actors); i++) {
            core_vector_push_back_int(&concrete_actor->worker_counts, 0);

            spawner = core_vector_at_as_int(&concrete_actor->initial_actors, i);

            thorium_actor_send_empty(actor, spawner, ACTION_GET_NODE_WORKER_COUNT);
        }

        concrete_actor->configured_actors = 0;


    } else if (tag == ACTION_GET_NODE_WORKER_COUNT_REPLY) {

        thorium_message_unpack_int(message, 0, &workers);

        concrete_actor->configured_actors++;

        spawner_index = core_vector_index_of(&concrete_actor->initial_actors, &source);

        printf("MANY_AGGREGATORS argonnite %d: spawner %d is on a node with %d workers\n",
                        name, source, workers);

        core_vector_set_int(&concrete_actor->worker_counts, spawner_index, workers);

        if (concrete_actor->configured_actors == core_vector_size(&concrete_actor->initial_actors)) {
            spawner = core_vector_at_as_int(&concrete_actor->initial_actors, core_vector_size(&concrete_actor->initial_actors) / 2);
            thorium_actor_send_int(actor, spawner, ACTION_SPAWN, SCRIPT_COVERAGE_DISTRIBUTION);
        }


    } else if (tag == ACTION_SPAWN_REPLY) {

        if (concrete_actor->state == ARGONNITE_STATE_PREPARE_SEQUENCE_STORES) {
            argonnite_prepare_sequence_stores(actor, message);
            return;
        }

        thorium_message_unpack_int(message, 0, &distribution);

        concrete_actor->distribution = distribution;

        manager_for_kernels = concrete_actor->manager_for_kernels;
        thorium_actor_send_int(actor, manager_for_kernels, ACTION_MANAGER_SET_SCRIPT,
                        SCRIPT_DNA_KMER_COUNTER_KERNEL);

    } else if (tag == ACTION_MANAGER_SET_SCRIPT_REPLY
                    && source == concrete_actor->manager_for_sequence_stores) {

        argonnite_prepare_sequence_stores(actor, message);

    } else if (tag == ACTION_MANAGER_SET_SCRIPT_REPLY
                    && source == concrete_actor->manager_for_kernels) {

#ifdef ARGONNITE_DEBUG1
        BIOSAL_DEBUG_MARKER("foo_marker_2");
#endif

        thorium_actor_send_reply_int(actor, ACTION_MANAGER_SET_ACTORS_PER_SPAWNER,
                        THORIUM_ACTOR_NO_VALUE);

    } else if (tag == ACTION_MANAGER_SET_ACTORS_PER_SPAWNER_REPLY
                    && source == concrete_actor->manager_for_kernels) {

        thorium_actor_send_reply_vector(actor, ACTION_START, &concrete_actor->initial_actors);

        /* ask the manager to spawn BIOSAL_KMER_COUNTER_KERNEL_SCRIPT actors,
         * these will be the customers of the controller
         */

    } else if (tag == ACTION_START_REPLY
                    && source == concrete_actor->manager_for_sequence_stores) {

        argonnite_prepare_sequence_stores(actor, message);

    } else if (tag == ACTION_START_REPLY
                    && source == concrete_actor->manager_for_kernels) {

        /* make sure that customers are unpacking correctly
         */
        core_vector_unpack(&concrete_actor->kernels, buffer);

        controller = concrete_actor->controller;

        printf("DEBUG Y got kernels !\n");

        thorium_actor_send_to_self_empty(actor, ACTION_ARGONNITE_PREPARE_SEQUENCE_STORES);

        /* save the kernels
         */


    } else if (tag == ACTION_SET_CONSUMERS_REPLY
                    && source == concrete_actor->controller) {


        thorium_actor_send_reply_vector(actor, ACTION_START, &concrete_actor->initial_actors);


    } else if (tag == ACTION_START_REPLY
                    && source == concrete_actor->controller) {

        /* add files */
        concrete_actor->argument_iterator = 0;

        if (concrete_actor->argument_iterator < argc) {
            argonnite_add_file(actor, message);
        }

    } else if (tag == ACTION_ADD_FILE_REPLY) {

        if (concrete_actor->argument_iterator < argc) {
            argonnite_add_file(actor, message);
        } else {

            /* spawn the manager for aggregators
             */
            manager_for_aggregators = thorium_actor_spawn(actor,
                            SCRIPT_MANAGER);

            printf("argonnite %d spawns manager %d for aggregators\n",
                            thorium_actor_name(actor), manager_for_aggregators);

            concrete_actor->manager_for_aggregators = manager_for_aggregators;

            thorium_actor_send_int(actor, manager_for_aggregators,
                            ACTION_MANAGER_SET_SCRIPT, SCRIPT_AGGREGATOR);

#ifdef ARGONNITE_DEBUG2
            BIOSAL_DEBUG_MARKER("set aggregator script");
#endif

        }

    } else if (tag == ACTION_MANAGER_SET_SCRIPT_REPLY
                    && source == concrete_actor->manager_for_aggregators) {

        manager_for_aggregators = concrete_actor->manager_for_aggregators;

#ifdef ARGONNITE_DEBUG2
            BIOSAL_DEBUG_MARKER("set actors per spawner ");
#endif

        /*
        workers_per_aggregator = ARGONNITE_WORKERS_PER_AGGREGATOR;
        printf("MANY_AGGREGATORS argonnite %d sets count per spawner to %d for aggregator manager %d\n",
                        thorium_actor_name(actor),
                        workers_per_aggregator, manager_for_aggregators);

        thorium_actor_send_reply_int(actor,
                            ACTION_MANAGER_SET_WORKERS_PER_ACTOR, workers_per_aggregator);
*/
        thorium_actor_send_reply_int(actor,
                            ACTION_MANAGER_SET_ACTORS_PER_WORKER, 1);

    } else if (tag == ACTION_MANAGER_SET_ACTORS_PER_WORKER_REPLY
                    && source == concrete_actor->manager_for_aggregators) {

        manager_for_aggregators = concrete_actor->manager_for_aggregators;

        /* send spawners to the aggregator manager
         */

        thorium_actor_send_reply_vector(actor, ACTION_START,
                        &concrete_actor->initial_actors);

        printf("argonnite %d ask manager %d to spawn children for work\n",
                        thorium_actor_name(actor), manager_for_aggregators);


    } else if (tag == ACTION_START_REPLY &&
                    source == concrete_actor->manager_for_aggregators) {

        concrete_actor->wired_kernels= 0;

        core_vector_unpack(&concrete_actor->aggregators, buffer);


        /*
         * before distributing, wire together the kernels and the aggregators.
         * It is like a brain, with some connections
         */

        printf("argonnite %d wires the brain, %d kernels, %d aggregators\n",
                        thorium_actor_name(actor),
                        (int)core_vector_size(&concrete_actor->kernels),
                        (int)core_vector_size(&concrete_actor->aggregators));

        kernel_index_index = 0;
        aggregator_index_index = 0;

        while (kernel_index_index < core_vector_size(&concrete_actor->kernels)) {
            kernel_index = core_vector_at_as_int(&concrete_actor->kernels, kernel_index_index);
            aggregator_index = core_vector_at_as_int(&concrete_actor->aggregators, aggregator_index_index);
            kernel = kernel_index;
            aggregator = aggregator_index;

            thorium_actor_send_int(actor, kernel, ACTION_SET_CONSUMER, aggregator);

            ++kernel_index_index;
            ++aggregator_index_index;
        }

#if 0
        for (spawner_index = 0; spawner_index < core_vector_size(&concrete_actor->initial_actors); spawner_index++) {

            workers = core_vector_at_as_int(&concrete_actor->worker_counts, spawner_index);
            /*
            workers_per_aggregator = ARGONNITE_WORKERS_PER_AGGREGATOR;
            */

#ifdef ARGONNITE_DEBUG_WIRING
            printf("Wiring %d, %d kernels\n", spawner_index, workers);
#endif

            while (workers > 0) {
                kernel_index = core_vector_at_as_int(&concrete_actor->kernels, kernel_index_index);
                aggregator_index = core_vector_at_as_int(&concrete_actor->aggregators, aggregator_index_index);

                kernel = kernel_index;
                aggregator = aggregator_index;

                printf("wiring kernel %d to aggregator %d\n", kernel, aggregator);
#ifdef ARGONNITE_DEBUG_WIRING
#endif

                thorium_actor_send_int(actor, kernel, ACTION_SET_CONSUMER, aggregator);

                kernel_index_index++;
                aggregator
                --workers;
                /*
                --workers_per_aggregator;

                if (workers_per_aggregator == 0) {
                    workers_per_aggregator = ARGONNITE_WORKERS_PER_AGGREGATOR;
                    aggregator_index_index++;
                }
                */
            }

            workers = core_vector_at_as_int(&concrete_actor->worker_counts, spawner_index);
            workers_per_aggregator = ARGONNITE_WORKERS_PER_AGGREGATOR;

            /* increment the aggregator if the number of workers is not a multiple of
             * the number of workers per aggregator (for this spawner).
             * This is required to avoid between-node transfers.
             */
            if (workers % workers_per_aggregator != 0) {
                ++aggregator_index_index;
            }
        }
#endif

    } else if (tag == ACTION_SET_CONSUMER_REPLY
                    && concrete_actor->wiring_distribution) {

        concrete_actor->configured_actors++;

        if (concrete_actor->configured_actors == core_vector_size(&concrete_actor->kmer_stores)) {

            printf("DEBUG all kmer stores are wired.\n");

            thorium_actor_send_empty(actor, concrete_actor->controller, ACTION_INPUT_DISTRIBUTE);

            concrete_actor->wiring_distribution = 0;
        }


    } else if (tag == ACTION_SET_CONSUMER_REPLY) {

        concrete_actor->wired_kernels++;

        if (concrete_actor->wired_kernels == (int)core_vector_size(&concrete_actor->kernels)) {

            printf("argonnite %d completed the wiring of the brain\n",
                thorium_actor_name(actor));

            concrete_actor->configured_actors = 0;



            thorium_actor_send_range_int(actor, &concrete_actor->kernels, ACTION_SET_KMER_LENGTH, concrete_actor->kmer_length);
            thorium_actor_send_range_int(actor, &concrete_actor->aggregators, ACTION_SET_KMER_LENGTH, concrete_actor->kmer_length);
        }

    } else if (tag == ACTION_SET_KMER_LENGTH_REPLY
                    && concrete_actor->spawned_stores == 0) {

        concrete_actor->configured_actors++;

        total_actors = core_vector_size(&concrete_actor->aggregators) +
                                core_vector_size(&concrete_actor->kernels);

        if (concrete_actor->configured_actors == total_actors) {

            thorium_actor_send_int(actor, concrete_actor->controller, ACTION_SET_BLOCK_SIZE,
                            concrete_actor->block_size);

        }
    } else if (tag == ACTION_SET_BLOCK_SIZE_REPLY) {

        manager_for_kmer_stores = thorium_actor_spawn(actor, SCRIPT_MANAGER);

        concrete_actor->manager_for_kmer_stores = manager_for_kmer_stores;

#ifdef ARGONNITE_DEBUG
        printf("DEBUG manager_for_kmer_stores %d\n", concrete_actor->manager_for_kmer_stores);
#endif

        thorium_actor_send_int(actor, manager_for_kmer_stores, ACTION_MANAGER_SET_SCRIPT,
                        SCRIPT_KMER_STORE);

    } else if (tag == ACTION_MANAGER_SET_SCRIPT_REPLY
                    && source == concrete_actor->manager_for_kmer_stores) {

        thorium_actor_send_reply_int(actor, ACTION_MANAGER_SET_ACTORS_PER_WORKER,
                        ARGONNITE_KMER_STORES_PER_WORKER);

    } else if (tag == ACTION_MANAGER_SET_ACTORS_PER_WORKER_REPLY
                    && source == concrete_actor->manager_for_kmer_stores) {

        thorium_actor_send_reply_vector(actor, ACTION_START, &concrete_actor->initial_actors);


    } else if (tag == ACTION_START_REPLY
                    && source == concrete_actor->manager_for_kmer_stores) {

        printf("DEBUG kmer stores READY\n");
        concrete_actor->spawned_stores = 1;

        core_vector_unpack(&concrete_actor->kmer_stores, buffer);

        concrete_actor->configured_aggregators = 0;


        thorium_actor_send_range_vector(actor, &concrete_actor->aggregators, ACTION_SET_CONSUMERS,
                &concrete_actor->kmer_stores);


    } else if (tag == ACTION_SET_CONSUMERS_REPLY) {
        /*
         * received a reply from one of the aggregators.
         */

        concrete_actor->configured_aggregators++;

        if (concrete_actor->configured_aggregators == core_vector_size(&concrete_actor->aggregators)) {

            printf("DEBUG all aggregator configured...\n");

            concrete_actor->configured_actors = 0;


            thorium_actor_send_range_int(actor, &concrete_actor->kmer_stores, ACTION_SET_KMER_LENGTH,
                            concrete_actor->kmer_length);

        }

    } else if (tag == ACTION_SET_KMER_LENGTH_REPLY
                    && concrete_actor->spawned_stores == 1) {

        concrete_actor->configured_actors++;

        if (concrete_actor->configured_actors == core_vector_size(&concrete_actor->kmer_stores)) {

            printf("DEBUG all kmer store have kmer length\n");
            concrete_actor->configured_actors = 0;


            distribution = concrete_actor->distribution;

            concrete_actor->wiring_distribution = 1;

            thorium_actor_send_range_int(actor, &concrete_actor->kmer_stores, ACTION_SET_CONSUMER,
                            distribution);

        }

    } else if (tag == ACTION_SET_PRODUCER_REPLY) {

        /* give it a new producer
         */

        kernel_index = source;
        kernel_index_index = core_vector_index_of(&concrete_actor->kernels, &kernel_index);

        CORE_DEBUGGER_ASSERT(kernel_index >= 0);
        CORE_DEBUGGER_ASSERT(kernel_index_index >= 0);

        sequence_store_index = core_vector_at_as_int(&concrete_actor->sequence_stores, kernel_index_index);

        bucket = core_map_get(&concrete_actor->plentiful_stores, &kernel_index_index);

        core_map_delete(&concrete_actor->plentiful_stores, &kernel_index_index);

        concrete_actor->finished_kernels++;

        print_stuff = 0;

        /* The work stealing code does not work very well on the IBM Blue Gene/Q
         * because the victim selection is not good enough.
         */
        enable_work_stealing = 0;

        if (bucket != NULL) {

            print_stuff = 1;
            printf("DEBUG finished kernels %d/%d, plentiful stores %d/%d\n", concrete_actor->finished_kernels,
                        (int)core_vector_size(&concrete_actor->kernels),
                        (int)core_map_size(&concrete_actor->plentiful_stores),
                        (int)core_vector_size(&concrete_actor->sequence_stores));
        }

        /* CONSTRUCTION SITE */

        /* If there are still stores with data
         */
        if (enable_work_stealing && !core_map_empty(&concrete_actor->plentiful_stores)) {

            /* Find a store now with data.
             * There is at least one.
             */
            sequence_store_index_index = kernel_index_index;

            bucket = core_map_get(&concrete_actor->plentiful_stores, &sequence_store_index_index);

            while (bucket == NULL) {

                sequence_store_index_index++;
                sequence_store_index_index %= (int)core_vector_size(&concrete_actor->sequence_stores);

                bucket = core_map_get(&concrete_actor->plentiful_stores, &sequence_store_index_index);
            }

            /* Make sure the bucket is not NULL...
             */
            CORE_DEBUGGER_ASSERT(bucket != NULL);

            (*bucket)++;

            sequence_store_index = core_vector_at_as_int(&concrete_actor->sequence_stores,
                            sequence_store_index_index);
            sequence_store = sequence_store_index;
            other_kernel = core_vector_at_as_int(&concrete_actor->kernels,
                            sequence_store_index_index);
            kernel = source;

            thorium_actor_send_int(actor, kernel, ACTION_SET_PRODUCER, sequence_store);

            if (print_stuff) {
                printf("argonnite %d tells kernel %d to steal work from kernel %d (%d), producer is sequence store %d\n",
                            thorium_actor_name(actor),
                            kernel,
                            other_kernel,
                            sequence_store_index_index,
                            sequence_store);
                printf("any further thefts by kernel %d will not be reported\n",
                            kernel);
            }

            concrete_actor->finished_kernels--;
        }

        if (concrete_actor->finished_kernels == core_vector_size(&concrete_actor->kernels)) {


            printf("sending ACTION_NOTIFY\n");
            thorium_actor_send_range_empty(actor, &concrete_actor->kernels, ACTION_NOTIFY);

            concrete_actor->finished_kernels = 0;
        }

    } else if (tag == ACTION_NOTIFY_REPLY) {

        thorium_message_unpack_uint64_t(message, 0, &produced);

        printf("kernel/%d generated %" PRIu64 " kmers\n",
                        source, produced);

        concrete_actor->total_kmers += produced;

        concrete_actor->ready_kernels++;

        if (concrete_actor->ready_kernels == core_vector_size(&concrete_actor->kernels)) {

            printf("DEBUG probing kmer stores\n");
            thorium_actor_send_to_self_empty(actor, ACTION_ARGONNITE_PROBE_KMER_STORES);
        }

    } else if (tag == ACTION_STORE_GET_ENTRY_COUNT_REPLY) {

        concrete_actor->ready_stores++;
        thorium_message_unpack_uint64_t(message, 0, &produced);
        concrete_actor->actual_kmers += produced;

        if (concrete_actor->ready_stores == core_vector_size(&concrete_actor->kmer_stores)) {

            if (concrete_actor->actual_kmers == concrete_actor->total_kmers) {

                printf("argonnite %d: stores are ready, %" PRIu64 "/%" PRIu64 " kmers\n",
                                name, concrete_actor->actual_kmers, concrete_actor->total_kmers);

                distribution = concrete_actor->distribution;



                thorium_actor_send_int(actor, distribution, ACTION_SET_EXPECTED_MESSAGE_COUNT,
                                core_vector_size(&concrete_actor->kmer_stores));

                printf("ISSUE_481 argonnite %d sends ACTION_PUSH_DATA to %d stores\n",
                                thorium_actor_name(actor),
                                (int)core_vector_size(&kmer_stores));

                thorium_actor_send_range_empty(actor, &concrete_actor->kmer_stores, ACTION_PUSH_DATA);

            } else {

                if (concrete_actor->not_ready_warnings % 100 == 0) {
                    printf("argonnite %d: stores are not ready, %" PRIu64 "/%" PRIu64 " kmers\n",
                                name, concrete_actor->actual_kmers, concrete_actor->total_kmers);
                }

                concrete_actor->not_ready_warnings++;
                thorium_actor_send_to_self_empty(actor, ACTION_ARGONNITE_PROBE_KMER_STORES);
            }
        }

    } else if (tag == ACTION_ARGONNITE_PROBE_KMER_STORES) {

        /* tell aggregators to flush
         */

        thorium_actor_send_range_empty(actor, &concrete_actor->aggregators, ACTION_AGGREGATOR_FLUSH);

        /* ask all stores how many kmers they have
         */

        thorium_actor_send_range_empty(actor, &concrete_actor->kmer_stores, ACTION_STORE_GET_ENTRY_COUNT);

        concrete_actor->ready_stores = 0;
        concrete_actor->actual_kmers = 0;


    } else if (tag == ACTION_NOTIFY && source == concrete_actor->distribution) {


        core_vector_iterator_init(&iterator, &concrete_actor->initial_actors);

        while (core_vector_iterator_has_next(&iterator)) {
            core_vector_iterator_next(&iterator, (void **)&bucket);

            other_name = *bucket;

            printf("argonnite %d stops argonnite %d\n",
                            name, other_name);

            thorium_actor_send_empty(actor, other_name, ACTION_ASK_TO_STOP);
        }


        core_vector_iterator_destroy(&iterator);

    } else if (tag == ACTION_ASK_TO_STOP) {

        if (concrete_actor->is_boss) {
            core_timer_stop(&concrete_actor->timer_for_kmers);
            core_timer_print_with_description(&concrete_actor->timer_for_kmers, "Input streaming and classification");

            core_timer_stop(&concrete_actor->timer);
            core_timer_print_with_description(&concrete_actor->timer, "Actor computation");
        }

        printf("argonnite %d stops\n", name);

        /*
         * STOP everything
         */

        thorium_actor_send_empty(actor, concrete_actor->manager_for_sequence_stores,
                        ACTION_ASK_TO_STOP);
        thorium_actor_send_empty(actor, concrete_actor->manager_for_kmer_stores,
                        ACTION_ASK_TO_STOP);
        thorium_actor_send_empty(actor, concrete_actor->manager_for_kernels,
                        ACTION_ASK_TO_STOP);
        thorium_actor_send_empty(actor, concrete_actor->manager_for_aggregators,
                        ACTION_ASK_TO_STOP);
        thorium_actor_send_empty(actor, concrete_actor->distribution,
                        ACTION_ASK_TO_STOP);

        thorium_actor_ask_to_stop(actor, message);

        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
}

void argonnite_add_file(struct thorium_actor *actor, struct thorium_message *message)
{
    int controller;
    char *file;
    int argc;
    char **argv;
    struct argonnite *concrete_actor;
    struct thorium_message new_message;

    argc = thorium_actor_argc(actor);
    concrete_actor = (struct argonnite *)thorium_actor_concrete_actor(actor);

    if (concrete_actor->argument_iterator >= argc) {
        return;
    }

    argv = thorium_actor_argv(actor);
    controller = concrete_actor->controller;

    file = argv[concrete_actor->argument_iterator++];

    thorium_message_init(&new_message, ACTION_ADD_FILE, strlen(file) + 1, file);

    thorium_actor_send(actor, controller, &new_message);

}

void argonnite_help(struct thorium_actor *actor)
{
    printf("argonnite - distributed kmer counter with actors\n");
    printf("\n");

    printf("Usage:\n");
    printf("mpiexec -n node_count argonnite -threads-per-node thread_count -k kmer_length -o output file1 file2 ...");
    printf("\n");

    printf("Options\n");
    printf("-threads-per-node thread_count       threads per biosal node\n");
    printf("-k kmer_length                      kmer length (default: %d, no limit, no compilation option)\n",
                    ARGONNITE_DEFAULT_KMER_LENGTH);
    printf("-o output                           output directory (default: %s)\n",
                    CORE_DEFAULT_OUTPUT);
    printf("-print-load                         display load, memory usage, actor count, active requests\n");
    printf("-print-counters                     print node-level biosal counters\n");
    printf("\n");

    printf("Output\n");
    printf(" %s\n", BIOSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT_FILE);
    printf(" %s\n", BIOSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT_FILE_CANONICAL);
    printf("\n");

    printf("Example\n");
    printf("\n");
    printf("mpiexec -n 64 argonnite -threads-per-node 32 -k 47 -o output \\\n");
    printf(" GPIC.1424-1.1371_1.fastq \\\n");
    printf(" GPIC.1424-1.1371_2.fastq \\\n");
    printf(" GPIC.1424-2.1371_1.fastq \\\n");
    printf(" GPIC.1424-2.1371_2.fastq \\\n");
    printf(" GPIC.1424-3.1371_1.fastq \\\n");
    printf(" GPIC.1424-3.1371_2.fastq \\\n");
    printf(" GPIC.1424-4.1371_1.fastq \\\n");
    printf(" GPIC.1424-4.1371_2.fastq \\\n");
    printf(" GPIC.1424-5.1371_1.fastq \\\n");
    printf(" GPIC.1424-5.1371_2.fastq \\\n");
    printf(" GPIC.1424-6.1371_1.fastq \\\n");
    printf(" GPIC.1424-6.1371_2.fastq \\\n");
    printf(" GPIC.1424-7.1371_1.fastq \\\n");
    printf(" GPIC.1424-7.1371_2.fastq\n");
}

void argonnite_prepare_sequence_stores(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    struct argonnite *concrete_actor;
    int spawner;
    int manager_for_sequence_stores;
    void *buffer;
    int controller;
    int i;
    int *bucket;

    concrete_actor = (struct argonnite *)thorium_actor_concrete_actor(self);

    concrete_actor->state = ARGONNITE_STATE_PREPARE_SEQUENCE_STORES;

    tag = thorium_message_action(message);
    buffer = thorium_message_buffer(message);

    if (tag == ACTION_ARGONNITE_PREPARE_SEQUENCE_STORES) {

        printf("DEBUGY spawn manager for stores\n");
        spawner = core_vector_at_as_int(&concrete_actor->initial_actors,
                        core_vector_size(&concrete_actor->initial_actors) - 1);
        thorium_actor_send_int(self, spawner, ACTION_SPAWN, SCRIPT_MANAGER);

    } else if (tag == ACTION_SPAWN_REPLY) {

        printf("DEBUGY got manager for stores\n");
        thorium_message_unpack_int(message, 0, &manager_for_sequence_stores);
        concrete_actor->manager_for_sequence_stores = manager_for_sequence_stores;

        thorium_actor_send_int(self, manager_for_sequence_stores, ACTION_MANAGER_SET_SCRIPT,
                        SCRIPT_SEQUENCE_STORE);

    } else if (tag == ACTION_MANAGER_SET_SCRIPT_REPLY) {

        printf("DEBUG script set for manager for sequence stores\n");


        thorium_actor_send_reply_vector(self, ACTION_START, &concrete_actor->initial_actors);

    } else if (tag == ACTION_START_REPLY) {

        printf("DEBUGY got sequence stores !\n");
        core_vector_unpack(&concrete_actor->sequence_stores, buffer);

        controller = concrete_actor->controller;

        thorium_actor_send_vector(self, controller, ACTION_SET_CONSUMERS,
                        &concrete_actor->sequence_stores);


        /* add stores in the plentiful stores
         */

        for (i = 0; i < core_vector_size(&concrete_actor->sequence_stores); i++) {
                /*
            store_index = core_vector_at_as_int(&concrete_actor->sequence_stores, i);
            */

            bucket = core_map_add(&concrete_actor->plentiful_stores, &i);
            *bucket = 1;

            /*
             * Subscribe for these notification
             * about progress
             */
#if 1
            thorium_actor_send_empty(self, core_vector_at_as_int(&concrete_actor->sequence_stores, i),
                            ACTION_SEQUENCE_STORE_REQUEST_PROGRESS);
#endif
        }

    }
}


void argonnite_connect_kernels_with_stores(struct thorium_actor *self, struct thorium_message *message)
{
    struct argonnite *concrete_actor;
    int name;
    int kernel;
    int sequence_store;
    int i;

    /* kill controller now !
     */

    thorium_actor_send_reply_empty(self, ACTION_ASK_TO_STOP);

    name = thorium_actor_name(self);
    concrete_actor = (struct argonnite *)thorium_actor_concrete_actor(self);

    core_timer_start(&concrete_actor->timer_for_kmers);

    printf("argonnite %d receives ACTION_INPUT_DISTRIBUTE_REPLY\n",
                        name);
#ifdef ARGONNITE_DEBUG
#endif

    concrete_actor->state = ARGONNITE_STATE_NONE;

    concrete_actor->ready_kernels = 0;

    /* tell the kernels to fetch data and compute
     */


    for (i = 0; i < core_vector_size(&concrete_actor->kernels); i++) {
        sequence_store = core_vector_at_as_int(&concrete_actor->sequence_stores, i);
        kernel = core_vector_at_as_int(&concrete_actor->kernels, i);

        thorium_actor_send_int(self, kernel, ACTION_SET_PRODUCER, sequence_store);

        /* Enable auto-scaling for initial kernels
         */
        thorium_actor_send_empty(self, kernel, ACTION_ENABLE_AUTO_SCALING);
    }


}

void argonnite_request_progress_reply(struct thorium_actor *actor, struct thorium_message *message)
{
    double value;
    int source;
    int store_index;
    int store_index_index;
    int kmer_store;
    struct argonnite *concrete_actor;

    concrete_actor = (struct argonnite *)thorium_actor_concrete_actor(actor);
    thorium_message_unpack_double(message, 0, &value);
    source = thorium_message_source(message);

    store_index = source;
    store_index_index = core_vector_index_of(&concrete_actor->sequence_stores, &store_index);
    kmer_store = core_vector_at_as_int(&concrete_actor->kmer_stores, store_index_index);

    printf("sequence store %d has a completion of %f, sending notice to kmer store %d\n",
                    source, value, kmer_store);

    if (thorium_actor_get_node_count(actor) == 1) {
        thorium_actor_send_double(actor, kmer_store, ACTION_SEQUENCE_STORE_REQUEST_PROGRESS_REPLY,
                    value);
    }
}

