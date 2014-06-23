
#include "argonnite.h"

#include <data/coverage_distribution.h>

#include <structures/vector.h>
#include <structures/vector_iterator.h>

#include <kernels/dna_kmer_counter_kernel.h>

#include <patterns/manager.h>
#include <patterns/aggregator.h>

#include <storage/kmer_store.h>

#include <system/memory.h>
#include <system/debugger.h>

#include <stdio.h>
#include <string.h>

#include <inttypes.h>

/*
#define ARGONNITE_DEBUG
*/

#define ARGONNITE_DEFAULT_KMER_LENGTH 41

struct bsal_script argonnite_script = {
    .name = ARGONNITE_SCRIPT,
    .init = argonnite_init,
    .destroy = argonnite_destroy,
    .receive = argonnite_receive,
    .size = sizeof(struct argonnite),
    .description = "argonnite"
};

void argonnite_init(struct bsal_actor *actor)
{
    struct argonnite *concrete_actor;

    concrete_actor = (struct argonnite *)bsal_actor_concrete_actor(actor);
    bsal_vector_init(&concrete_actor->initial_actors, sizeof(int));
    bsal_vector_init(&concrete_actor->kernels, sizeof(int));
    bsal_vector_init(&concrete_actor->aggregators, sizeof(int));
    bsal_vector_init(&concrete_actor->kmer_stores, sizeof(int));
    bsal_vector_init(&concrete_actor->sequence_stores, sizeof(int));
    bsal_vector_init(&concrete_actor->worker_counts, sizeof(int));

    bsal_actor_add_script(actor, BSAL_INPUT_CONTROLLER_SCRIPT,
                    &bsal_input_controller_script);
    bsal_actor_add_script(actor, BSAL_KMER_COUNTER_KERNEL_SCRIPT,
                    &bsal_dna_kmer_counter_kernel_script);
    bsal_actor_add_script(actor, BSAL_MANAGER_SCRIPT,
                    &bsal_manager_script);
    bsal_actor_add_script(actor, BSAL_AGGREGATOR_SCRIPT,
                    &bsal_aggregator_script);
    bsal_actor_add_script(actor, BSAL_KMER_STORE_SCRIPT,
                    &bsal_kmer_store_script);
    bsal_actor_add_script(actor, BSAL_COVERAGE_DISTRIBUTION_SCRIPT,
                    &bsal_coverage_distribution_script);

    concrete_actor->kmer_length = ARGONNITE_DEFAULT_KMER_LENGTH;

    /* the number of input sequences per block
     */
    concrete_actor->block_size = 512;
    /*concrete_actor->block_size = 2048;*/

    concrete_actor->configured_actors = 0;
    concrete_actor->wired_kernels = 0;
    concrete_actor->spawned_stores = 0;
    concrete_actor->wiring_distribution = 0;

    concrete_actor->ready_kernels = 0;
    concrete_actor->total_kmers = 0;
}

void argonnite_destroy(struct bsal_actor *actor)
{
    struct argonnite *concrete_actor;

    concrete_actor = (struct argonnite *)bsal_actor_concrete_actor(actor);

    bsal_vector_destroy(&concrete_actor->initial_actors);
    bsal_vector_destroy(&concrete_actor->kernels);
    bsal_vector_destroy(&concrete_actor->aggregators);
    bsal_vector_destroy(&concrete_actor->kmer_stores);
    bsal_vector_destroy(&concrete_actor->sequence_stores);
    bsal_vector_destroy(&concrete_actor->worker_counts);
}

void argonnite_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    void *buffer;
    int total_actors;
    struct argonnite *concrete_actor;
    struct bsal_vector initial_actors;
    struct bsal_vector aggregators;
    struct bsal_vector kernels;
    int *bucket;
    int controller;
    int manager_for_kernels;
    int manager_for_aggregators;
    int distribution;
    struct bsal_vector spawners;
    int other_name;
    struct bsal_vector customers;
    struct bsal_vector_iterator iterator;
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
    struct bsal_vector kmer_stores;
    int spawner;
    int is_boss;
    uint64_t produced;
    int workers;
    int kernel_index_index;
    int spawner_index;

    concrete_actor = (struct argonnite *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    argc = bsal_actor_argc(actor);
    argv = bsal_actor_argv(actor);
    name = bsal_actor_name(actor);
    source = bsal_message_source(message);

    if (tag == BSAL_ACTOR_START) {

        for (i = 0; i < argc; i++) {
            if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {

                concrete_actor->kmer_length =atoi(argv[i + 1]);
            }
        }

#ifdef ARGONNITE_DEBUG1
        BSAL_DEBUG_MARKER("foo_marker");
#endif

        bsal_vector_unpack(&initial_actors, buffer);

        is_boss = 0;

        if (bsal_vector_helper_at_as_int(&initial_actors, 0) == name) {
            is_boss = 1;
        }

        /* help page
         */
        if (argc == 1
                        || (argc == 2 && strstr(argv[1], "help") != NULL)) {

            if (is_boss) {
                argonnite_help(actor);
            }

            bsal_actor_helper_ask_to_stop(actor, message);
            return;
        }

        printf("argonnite actor/%d starts\n", name);

        bsal_actor_helper_add_acquaintances(actor, &initial_actors, &concrete_actor->initial_actors);

        /*
         * Run only one argonnite actor
         */
        if (!is_boss) {
            return;
        }

        controller = bsal_actor_spawn(actor, BSAL_INPUT_CONTROLLER_SCRIPT);
        concrete_actor->controller = bsal_actor_get_acquaintance_index(actor, controller);

        manager_for_kernels = bsal_actor_spawn(actor, BSAL_MANAGER_SCRIPT);
        concrete_actor->manager_for_kernels= bsal_actor_get_acquaintance_index(actor, manager_for_kernels);

#ifdef ARGONNITE_DEBUG1
        BSAL_DEBUG_MARKER("after setting script");
        printf("manager %d, index %d\n", manager_for_directors, concrete_actor->manager_for_directors);
#endif

        for (i = 0; i < bsal_vector_size(&initial_actors); i++) {
            bsal_vector_helper_push_back_int(&concrete_actor->worker_counts, 0);

            spawner = bsal_vector_helper_at_as_int(&initial_actors, i);

            bsal_actor_helper_send_empty(actor, spawner, BSAL_ACTOR_GET_NODE_WORKER_COUNT);
        }

        concrete_actor->configured_actors = 0;

        bsal_vector_destroy(&initial_actors);

    } else if (tag == BSAL_ACTOR_GET_NODE_WORKER_COUNT_REPLY) {

        bsal_message_helper_unpack_int(message, 0, &workers);

        concrete_actor->configured_actors++;

        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->initial_actors, &initial_actors);

        spawner_index = bsal_vector_index_of(&initial_actors, &source);

        bsal_vector_helper_set_int(&concrete_actor->worker_counts, spawner_index, workers);

        if (concrete_actor->configured_actors == bsal_vector_size(&concrete_actor->initial_actors)) {
            spawner = bsal_vector_helper_at_as_int(&initial_actors, bsal_vector_size(&initial_actors) / 2);
            bsal_actor_helper_send_int(actor, spawner, BSAL_ACTOR_SPAWN, BSAL_COVERAGE_DISTRIBUTION_SCRIPT);
        }

        bsal_vector_destroy(&initial_actors);

    } else if (tag == BSAL_ACTOR_SPAWN_REPLY) {

        bsal_message_helper_unpack_int(message, 0, &distribution);

        concrete_actor->distribution = bsal_actor_get_acquaintance_index(actor, distribution);

        manager_for_kernels = bsal_actor_get_acquaintance(actor, concrete_actor->manager_for_kernels);
        bsal_actor_helper_send_int(actor, manager_for_kernels, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_KMER_COUNTER_KERNEL_SCRIPT);

    } else if (tag == BSAL_MANAGER_SET_SCRIPT_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_kernels)) {

#ifdef ARGONNITE_DEBUG1
        BSAL_DEBUG_MARKER("foo_marker_2");
#endif

        bsal_actor_helper_send_reply_int(actor, BSAL_MANAGER_SET_ACTORS_PER_SPAWNER,
                        BSAL_ACTOR_NO_VALUE);

    } else if (tag == BSAL_MANAGER_SET_ACTORS_PER_SPAWNER_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_kernels)) {

        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->initial_actors, &spawners);

        bsal_actor_helper_send_reply_vector(actor, BSAL_ACTOR_START, &spawners);

        /* ask the manager to spawn BSAL_KMER_COUNTER_KERNEL_SCRIPT actors,
         * these will be the customers of the controller
         */
        bsal_vector_destroy(&spawners);

    } else if (tag == BSAL_ACTOR_START_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_kernels)) {

        /* make sure that customers are unpacking correctly
         */
        bsal_vector_unpack(&customers, buffer);

        controller = bsal_actor_get_acquaintance(actor, concrete_actor->controller);

        bsal_actor_helper_send_vector(actor, controller, BSAL_SET_CONSUMERS,
                        &customers);

        /* save the kernels
         */
        bsal_actor_helper_add_acquaintances(actor, &customers, &concrete_actor->kernels);

        bsal_vector_destroy(&customers);

    } else if (tag == BSAL_SET_CONSUMERS_REPLY
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

        concrete_actor->wired_kernels= 0;

        bsal_vector_unpack(&aggregators, buffer);

        bsal_actor_helper_add_acquaintances(actor, &aggregators, &concrete_actor->aggregators);
        bsal_vector_destroy(&aggregators);

        /*
         * before distributing, wire together the kernels and the aggregators.
         * It is like a brain, with some connections
         */

        printf("argonnite actor/%d wires the brain, %d kernels, %d aggregators\n",
                        bsal_actor_name(actor),
                        (int)bsal_vector_size(&concrete_actor->kernels),
                        (int)bsal_vector_size(&concrete_actor->aggregators));

        kernel_index_index = 0;

        for (spawner_index = 0; spawner_index < bsal_vector_size(&concrete_actor->initial_actors); spawner_index++) {

            workers = bsal_vector_helper_at_as_int(&concrete_actor->worker_counts, spawner_index);

#ifdef ARGONNITE_DEBUG_WIRING
            printf("Wiring %d, %d kernels\n", spawner_index, workers);
#endif

            while (workers--) {
                kernel_index = bsal_vector_helper_at_as_int(&concrete_actor->kernels, kernel_index_index);
                aggregator_index = bsal_vector_helper_at_as_int(&concrete_actor->aggregators, spawner_index);

                kernel = bsal_actor_get_acquaintance(actor, kernel_index);
                aggregator = bsal_actor_get_acquaintance(actor, aggregator_index);

#ifdef ARGONNITE_DEBUG_WIRING
                printf("wiring kernel %d to aggregator %d\n", kernel, aggregator);
#endif

                bsal_actor_helper_send_int(actor, kernel, BSAL_SET_CONSUMER, aggregator);

                kernel_index_index++;
            }
        }

    } else if (tag == BSAL_SET_CONSUMER_REPLY
                    && concrete_actor->wiring_distribution) {

        concrete_actor->configured_actors++;

        if (concrete_actor->configured_actors == bsal_vector_size(&concrete_actor->kmer_stores)) {
            bsal_actor_helper_send_empty(actor, bsal_actor_get_acquaintance(actor,
                                    concrete_actor->controller), BSAL_INPUT_DISTRIBUTE);

            concrete_actor->wiring_distribution = 0;
        }

    } else if (tag == BSAL_SET_CONSUMER_REPLY) {

        concrete_actor->wired_kernels++;

        if (concrete_actor->wired_kernels == (int)bsal_vector_size(&concrete_actor->kernels)) {

            printf("argonnite actor/%d completed the wiring of the brain\n",
                bsal_actor_name(actor));

            concrete_actor->configured_actors = 0;

            bsal_actor_helper_get_acquaintances(actor, &concrete_actor->kernels, &kernels);
            bsal_actor_helper_get_acquaintances(actor, &concrete_actor->aggregators, &aggregators);

            bsal_actor_helper_send_range_int(actor, &kernels, BSAL_SET_KMER_LENGTH, concrete_actor->kmer_length);
            bsal_actor_helper_send_range_int(actor, &aggregators, BSAL_SET_KMER_LENGTH, concrete_actor->kmer_length);

            bsal_vector_destroy(&kernels);
            bsal_vector_destroy(&aggregators);
        }

    } else if (tag == BSAL_SET_KMER_LENGTH_REPLY
                    && concrete_actor->spawned_stores == 0) {

        concrete_actor->configured_actors++;

        total_actors = bsal_vector_size(&concrete_actor->aggregators) +
                                bsal_vector_size(&concrete_actor->kernels);

        if (concrete_actor->configured_actors == total_actors) {

            bsal_actor_helper_send_int(actor, bsal_actor_get_acquaintance(actor,
                                    concrete_actor->controller), BSAL_SET_BLOCK_SIZE,
                            concrete_actor->block_size);

        }
    } else if (tag == BSAL_SET_BLOCK_SIZE_REPLY) {

        manager_for_kmer_stores = bsal_actor_spawn(actor, BSAL_MANAGER_SCRIPT);

        concrete_actor->manager_for_kmer_stores = bsal_actor_get_acquaintance_index(actor, manager_for_kmer_stores);

#ifdef ARGONNITE_DEBUG
        printf("DEBUG manager_for_kmer_stores %d\n", concrete_actor->manager_for_kmer_stores);
#endif

        bsal_actor_helper_send_int(actor, manager_for_kmer_stores, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_KMER_STORE_SCRIPT);

    } else if (tag == BSAL_MANAGER_SET_SCRIPT_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_kmer_stores)) {

        bsal_actor_helper_send_reply_int(actor, BSAL_MANAGER_SET_ACTORS_PER_WORKER,
                        1);

    } else if (tag == BSAL_MANAGER_SET_ACTORS_PER_WORKER_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_kmer_stores)) {

        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->initial_actors,
                        &spawners);
        bsal_actor_helper_send_reply_vector(actor, BSAL_ACTOR_START, &spawners);

        bsal_vector_destroy(&spawners);

    } else if (tag == BSAL_ACTOR_START_REPLY
                    && source == bsal_actor_get_acquaintance(actor,
                            concrete_actor->manager_for_kmer_stores)) {

        concrete_actor->spawned_stores = 1;

        bsal_vector_unpack(&kmer_stores, buffer);
        bsal_actor_helper_add_acquaintances(actor, &kmer_stores, &concrete_actor->kmer_stores);

        concrete_actor->configured_aggregators = 0;

        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->aggregators,
                    &aggregators);

        bsal_actor_helper_send_range_vector(actor, &aggregators, BSAL_SET_CONSUMERS,
                &kmer_stores);

        bsal_vector_destroy(&aggregators);
        bsal_vector_destroy(&kmer_stores);

    } else if (tag == BSAL_SET_CONSUMERS_REPLY) {
        /*
         * received a reply from one of the aggregators.
         */

        concrete_actor->configured_aggregators++;

        if (concrete_actor->configured_aggregators == bsal_vector_size(&concrete_actor->aggregators)) {

            concrete_actor->configured_actors = 0;

            bsal_actor_helper_get_acquaintances(actor, &concrete_actor->kmer_stores, &kmer_stores);

            bsal_actor_helper_send_range_int(actor, &kmer_stores, BSAL_SET_KMER_LENGTH,
                            concrete_actor->kmer_length);

            bsal_vector_destroy(&kmer_stores);
        }

    } else if (tag == BSAL_SET_KMER_LENGTH_REPLY
                    && concrete_actor->spawned_stores == 1) {

        concrete_actor->configured_actors++;

        if (concrete_actor->configured_actors == bsal_vector_size(&concrete_actor->kmer_stores)) {

            concrete_actor->configured_actors = 0;

            bsal_actor_helper_get_acquaintances(actor, &concrete_actor->kmer_stores, &kmer_stores);

            distribution = bsal_actor_get_acquaintance(actor, concrete_actor->distribution);

            concrete_actor->wiring_distribution = 1;

            bsal_actor_helper_send_range_int(actor, &kmer_stores, BSAL_SET_CONSUMER,
                            distribution);

            bsal_vector_destroy(&kmer_stores);
        }

    } else if (tag == BSAL_INPUT_DISTRIBUTE_REPLY) {

        printf("argonnite actor/%d receives BSAL_INPUT_DISTRIBUTE_REPLY\n",
                        name);
#ifdef ARGONNITE_DEBUG
#endif

        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->kernels, &kernels);
        bsal_actor_helper_send_range_empty(actor, &kernels, BSAL_KERNEL_NOTIFY);

    } else if (tag == BSAL_KERNEL_NOTIFY_REPLY) {

        bsal_message_helper_unpack_uint64_t(message, 0, &produced);

        printf("kernel/%d generated %" PRIu64 " kmers\n",
                        source, produced);

        concrete_actor->total_kmers += produced;

        concrete_actor->ready_kernels++;

        if (concrete_actor->ready_kernels == bsal_vector_size(&concrete_actor->kernels)) {

            bsal_actor_helper_send_to_self_empty(actor, ARGONNITE_PROBE_STORES);
        }

    } else if (tag == BSAL_STORE_GET_ENTRY_COUNT_REPLY) {

        concrete_actor->ready_stores++;
        bsal_message_helper_unpack_uint64_t(message, 0, &produced);
        concrete_actor->actual_kmers += produced;

        if (concrete_actor->ready_stores == bsal_vector_size(&concrete_actor->kmer_stores)) {

            if (concrete_actor->actual_kmers == concrete_actor->total_kmers) {

                printf("argonnite actor/%d: stores are ready, %" PRIu64 "/%" PRIu64 " kmers\n",
                                name, concrete_actor->actual_kmers, concrete_actor->total_kmers);

                distribution = bsal_actor_get_acquaintance(actor, concrete_actor->distribution);
                bsal_actor_helper_get_acquaintances(actor, &concrete_actor->kmer_stores, &kmer_stores);

                bsal_actor_helper_send_int(actor, distribution, BSAL_SET_EXPECTED_MESSAGES, bsal_vector_size(&kmer_stores));

                bsal_actor_helper_send_range_empty(actor, &kmer_stores, BSAL_PUSH_DATA);
                bsal_vector_destroy(&kmer_stores);

            } else {

                printf("argonnite actor/%d: stores are not ready, %" PRIu64 "/%" PRIu64 " kmers\n",
                                name, concrete_actor->actual_kmers, concrete_actor->total_kmers);

                bsal_actor_helper_send_to_self_empty(actor, ARGONNITE_PROBE_STORES);
            }
        }

    } else if (tag == ARGONNITE_PROBE_STORES) {

        /* tell aggregators to flush
         */

        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->aggregators, &aggregators);
        bsal_actor_helper_send_range_empty(actor, &aggregators, BSAL_AGGREGATOR_FLUSH);

        /* ask all stores how many kmers they have
         */
        bsal_actor_helper_get_acquaintances(actor, &concrete_actor->kmer_stores, &kmer_stores);

        bsal_actor_helper_send_range_empty(actor, &kmer_stores, BSAL_STORE_GET_ENTRY_COUNT);

        concrete_actor->ready_stores = 0;
        concrete_actor->actual_kmers = 0;
        bsal_vector_destroy(&kmer_stores);
        bsal_vector_destroy(&aggregators);

    } else if (tag == BSAL_SET_EXPECTED_MESSAGES_REPLY) {

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

        bsal_actor_helper_ask_to_stop(actor, message);


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

void argonnite_help(struct bsal_actor *actor)
{
    printf("argonnite - distributed kmer counter with actors\n");
    printf("\n");

    printf("Usage:\n");
    printf("mpiexec -n node_count argonnite -threads-per-node thread_count -k kmer_length -o output file1 file2 ...");
    printf("\n");

    printf("Options\n");
    printf("-thread-per-node thread_count       threads per biosal node\n");
    printf("-k kmer_length                      kmer length (default: %d, no limit, no compilation option)\n",
                    ARGONNITE_DEFAULT_KMER_LENGTH);
    printf("-o output                           output (default: %s)\n", BSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT);
    printf("-print-counters                     print node-level biosal counters\n");
    printf("-print-load                         print load\n");
    printf("-print-memory-usage                 print memory usage\n");
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
