
#include "assembly_graph_builder.h"

#include "assembly_graph_store.h"
#include "assembly_sliding_window.h"
#include "assembly_block_classifier.h"

#include "assembly_arc_kernel.h"
#include "assembly_arc_classifier.h"

#include <genomics/helpers/command.h>

#include <core/system/debugger.h>
#include <core/system/command.h>

#include <core/structures/string.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

/*
 * Constructor and destructor
 */
void biosal_assembly_graph_builder_init(struct thorium_actor *self);
void biosal_assembly_graph_builder_destroy(struct thorium_actor *self);

/*
 * The main handler
 */
void biosal_assembly_graph_builder_receive(struct thorium_actor *self, struct thorium_message *message);

/*
 * Other handlers
 */
void biosal_assembly_graph_builder_ask_to_stop(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_graph_builder_start(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_set_producers(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_spawn_reply(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_spawn_reply_graph_store_manager(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_spawn_reply_window_manager(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_graph_builder_set_script_reply_store_manager(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_start_reply_store_manager(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_graph_builder_set_script_reply_window_manager(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_start_reply_window_manager(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_graph_builder_set_script_reply_classifier_manager(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_start_reply_classifier_manager(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_graph_builder_configure(struct thorium_actor *self);
void biosal_assembly_graph_builder_set_kmer_reply(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_graph_builder_connect_actors(struct thorium_actor *self);

void biosal_assembly_graph_builder_set_consumers_reply(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_verify(struct thorium_actor *self);
void biosal_assembly_graph_builder_set_consumer_reply(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_graph_builder_tell_source(struct thorium_actor *self);
void biosal_assembly_graph_builder_set_producer_reply(struct thorium_actor *self, struct thorium_message *message);
int biosal_assembly_graph_builder_get_kmer_length(struct thorium_actor *self);

void biosal_assembly_graph_builder_notify_reply(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_control_complexity(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_get_entry_count_reply(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_graph_builder_set_consumer_reply_graph_stores(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_set_consumer_reply_windows(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_set_expected_message_count_reply(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_notify_from_distribution(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_graph_builder_spawn_reply_arc_kernel_manager(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_start_reply_arc_kernel_manager(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_set_script_reply_arc_kernel_manager(struct thorium_actor *self, struct thorium_message *message);

/*
 * For the manager of arc classifiers.
 */
void biosal_assembly_graph_builder_start_reply_arc_classifier_manager(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_set_script_reply_arc_classifier_manager(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_spawn_reply_arc_classifier_manager(struct thorium_actor *self, struct thorium_message *message);

/*
 * Configure kmer length for arcs.
 */

void biosal_assembly_graph_builder_set_kmer_reply_arcs(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_configure_arc_actors(struct thorium_actor *self, struct thorium_message *message);

/*
 * Functions to verify arcs for high quality.
 */
void biosal_assembly_graph_builder_verify_arcs(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_verify_arc_kernels(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_notify_reply_arc_kernels(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_graph_builder_get_summary_reply(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_graph_builder_get_producers_for_work_stealing(struct thorium_actor *self, struct core_vector *producers_for_work_stealing,
                int current_index);
void biosal_assembly_graph_builder_set_actors_reply_store_manager(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script biosal_assembly_graph_builder_script = {
    .identifier = SCRIPT_ASSEMBLY_GRAPH_BUILDER,
    .name = "biosal_assembly_graph_builder",
    .description = "This builder implements a distributed actor algorithm for building an assembly graph",
    .version = "Cool version",
    .author = "Sebastien Boisvert, Argonne National Laboratory",
    .init = biosal_assembly_graph_builder_init,
    .destroy = biosal_assembly_graph_builder_destroy,
    .receive = biosal_assembly_graph_builder_receive,
    .size = sizeof(struct biosal_assembly_graph_builder)
};

void biosal_assembly_graph_builder_init(struct thorium_actor *self)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    core_timer_init(&concrete_self->timer);
    core_timer_init(&concrete_self->vertex_timer);
    core_timer_init(&concrete_self->arc_timer);

    concrete_self->completed_sliding_windows = 0;
    concrete_self->doing_arcs = 0;

    core_vector_init(&concrete_self->spawners, sizeof(int));
    core_vector_init(&concrete_self->sequence_stores, sizeof(int));
    core_vector_init(&concrete_self->block_classifiers, sizeof(int));

    thorium_actor_add_action(self, ACTION_START, biosal_assembly_graph_builder_start);
    thorium_actor_add_action(self, ACTION_SET_PRODUCERS, biosal_assembly_graph_builder_set_producers);
    thorium_actor_add_action(self, ACTION_SET_PRODUCER_REPLY, biosal_assembly_graph_builder_set_producer_reply);
    thorium_actor_add_action(self, ACTION_ASK_TO_STOP, biosal_assembly_graph_builder_ask_to_stop);
    thorium_actor_add_action(self, ACTION_SPAWN_REPLY, biosal_assembly_graph_builder_spawn_reply);
    thorium_actor_add_action(self, ACTION_SET_KMER_LENGTH_REPLY, biosal_assembly_graph_builder_set_kmer_reply);
    thorium_actor_add_action(self, ACTION_NOTIFY_REPLY, biosal_assembly_graph_builder_notify_reply);
    thorium_actor_add_action(self, ACTION_ASSEMBLY_GRAPH_BUILDER_CONTROL_COMPLEXITY,
                    biosal_assembly_graph_builder_control_complexity);

    thorium_actor_add_action(self, ACTION_SET_CONSUMER_REPLY, biosal_assembly_graph_builder_set_consumer_reply);
    thorium_actor_add_action(self, ACTION_SET_CONSUMERS_REPLY, biosal_assembly_graph_builder_set_consumers_reply);
    thorium_actor_add_action(self, ACTION_STORE_GET_ENTRY_COUNT_REPLY,
                    biosal_assembly_graph_builder_get_entry_count_reply);

    concrete_self->manager_for_graph_stores = THORIUM_ACTOR_NOBODY;
    core_vector_init(&concrete_self->graph_stores, sizeof(int));

    concrete_self->manager_for_classifiers = THORIUM_ACTOR_NOBODY;

    concrete_self->manager_for_windows = THORIUM_ACTOR_NOBODY;
    core_vector_init(&concrete_self->sliding_windows, sizeof(int));

    concrete_self->coverage_distribution = THORIUM_ACTOR_NOBODY;

    concrete_self->manager_for_arc_kernels = THORIUM_ACTOR_NOBODY;
    core_vector_init(&concrete_self->arc_kernels, sizeof(int));

    concrete_self->manager_for_arc_classifiers = THORIUM_ACTOR_NOBODY;
    core_vector_init(&concrete_self->arc_classifiers, sizeof(int));

    concrete_self->configured_graph_stores = 0;
    concrete_self->configured_sliding_windows = 0;
    concrete_self->configured_block_classifiers = 0;

    concrete_self->actors_with_kmer_length = 0;

    concrete_self->total_kmer_count = 0;
    concrete_self->notified_windows = 0;

    concrete_self->synchronized_graph_stores = 0;
    concrete_self->actual_kmer_count = 0;

    concrete_self->expected_arc_count = 0;

    biosal_assembly_graph_summary_init(&concrete_self->graph_summary);
}

void biosal_assembly_graph_builder_destroy(struct thorium_actor *self)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    core_vector_destroy(&concrete_self->spawners);
    core_vector_destroy(&concrete_self->sequence_stores);

    concrete_self->manager_for_graph_stores = THORIUM_ACTOR_NOBODY;
    core_vector_destroy(&concrete_self->graph_stores);

    concrete_self->manager_for_classifiers = THORIUM_ACTOR_NOBODY;
    core_vector_destroy(&concrete_self->block_classifiers);

    concrete_self->manager_for_windows= THORIUM_ACTOR_NOBODY;
    core_vector_destroy(&concrete_self->sliding_windows);

    concrete_self->manager_for_arc_kernels = THORIUM_ACTOR_NOBODY;
    core_vector_destroy(&concrete_self->arc_kernels);

    concrete_self->manager_for_arc_classifiers = THORIUM_ACTOR_NOBODY;
    core_vector_destroy(&concrete_self->arc_classifiers);

    core_timer_destroy(&concrete_self->timer);
    core_timer_destroy(&concrete_self->vertex_timer);
    core_timer_destroy(&concrete_self->arc_timer);

    biosal_assembly_graph_summary_destroy(&concrete_self->graph_summary);
}

void biosal_assembly_graph_builder_receive(struct thorium_actor *self, struct thorium_message *message)
{
    thorium_actor_take_action(self, message);
}

void biosal_assembly_graph_builder_ask_to_stop(struct thorium_actor *self, struct thorium_message *message)
{
    int name;
    struct biosal_assembly_graph_builder *concrete_self;

    name = thorium_actor_name(self);
    concrete_self = thorium_actor_concrete_actor(self);

    printf("builder/%d dies\n", name);

    thorium_actor_ask_to_stop(self, message);

    /*
     * Stop graph stores
     */

    thorium_actor_send_empty(self, concrete_self->manager_for_graph_stores,
                    ACTION_ASK_TO_STOP);
}

void biosal_assembly_graph_builder_start(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct biosal_assembly_graph_builder *concrete_self;
    int source;
    int spawner;

    source = thorium_message_source(message);
    concrete_self = thorium_actor_concrete_actor(self);

    if (source != concrete_self->source) {
        return;
    }

    core_timer_start(&concrete_self->timer);
    core_timer_start(&concrete_self->vertex_timer);

    buffer = thorium_message_buffer(message);

    core_vector_unpack(&concrete_self->spawners, buffer);

    printf("%s/%d received a urgent request from source %d to build an assembly graph using %d sequence stores (producers) and %d spawners\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    source,
                    (int)core_vector_size(&concrete_self->sequence_stores),
                    (int)core_vector_size(&concrete_self->spawners));

    spawner = thorium_actor_get_spawner(self, &concrete_self->spawners);

    /*
     * Spawn the manager for graph stores
     */
    concrete_self->manager_for_graph_stores = THORIUM_ACTOR_SPAWNING_IN_PROGRESS;

    thorium_actor_add_action_with_source_and_condition(self,
                    ACTION_SPAWN_REPLY,
                    biosal_assembly_graph_builder_spawn_reply_graph_store_manager,
                    spawner,
                    &(concrete_self->manager_for_graph_stores),
                    THORIUM_ACTOR_SPAWNING_IN_PROGRESS);

    thorium_actor_send_int(self, spawner, ACTION_SPAWN, SCRIPT_MANAGER);
}

void biosal_assembly_graph_builder_spawn_reply_graph_store_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    /*
     * Configure the manager for graph stores
     */

    thorium_message_unpack_int(message, 0, &concrete_self->manager_for_graph_stores);

    thorium_actor_add_action_with_source(self, ACTION_MANAGER_SET_SCRIPT_REPLY,
                    biosal_assembly_graph_builder_set_script_reply_store_manager,
                    concrete_self->manager_for_graph_stores);

    thorium_actor_add_action_with_source(self, ACTION_START_REPLY,
                    biosal_assembly_graph_builder_start_reply_store_manager,
                    concrete_self->manager_for_graph_stores);

    thorium_actor_send_int(self, concrete_self->manager_for_graph_stores, ACTION_MANAGER_SET_SCRIPT,
                        SCRIPT_ASSEMBLY_GRAPH_STORE);
}

/*
 * find some sort of way to reduce the number of lines here
 * using a new Thorium API call.
 *
 * Solutions:
 *
 * thorium_actor_add_action_with_condition
 * thorium_actor_add_action_with_source_and_condition
 */
void biosal_assembly_graph_builder_spawn_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);
    printf("DEBUG biosal_assembly_graph_builder_spawn_reply\n");

    /*
     * Configure the manager for block classifiers
     */
    if (concrete_self->manager_for_classifiers == THORIUM_ACTOR_NOBODY) {

        thorium_message_unpack_int(message, 0, &concrete_self->manager_for_classifiers);

        thorium_actor_add_action_with_source(self, ACTION_MANAGER_SET_SCRIPT_REPLY,
                        biosal_assembly_graph_builder_set_script_reply_classifier_manager,
                        concrete_self->manager_for_classifiers);

        thorium_actor_add_action_with_source(self, ACTION_START_REPLY,
                        biosal_assembly_graph_builder_start_reply_classifier_manager,
                        concrete_self->manager_for_classifiers);

        thorium_actor_send_int(self, concrete_self->manager_for_classifiers, ACTION_MANAGER_SET_SCRIPT,
                        SCRIPT_ASSEMBLY_BLOCK_CLASSIFIER);

    } else if (concrete_self->coverage_distribution == THORIUM_ACTOR_NOBODY) {

        thorium_message_unpack_int(message, 0, &concrete_self->coverage_distribution);

        thorium_actor_add_action_with_source(self, ACTION_SET_EXPECTED_MESSAGE_COUNT_REPLY,
                            biosal_assembly_graph_builder_set_expected_message_count_reply,
                        concrete_self->coverage_distribution);

        thorium_actor_add_action_with_source(self, ACTION_NOTIFY,
                        biosal_assembly_graph_builder_notify_from_distribution,
                        concrete_self->coverage_distribution);

        thorium_actor_send_int(self, concrete_self->coverage_distribution,
                        ACTION_SET_EXPECTED_MESSAGE_COUNT, (int)core_vector_size(&concrete_self->graph_stores));

    } else {

        printf("Warning: unknown state when receiveing ACTION_SPAWN_REPLY\n");
    }
}

void biosal_assembly_graph_builder_set_expected_message_count_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_actor_add_action_with_sources(self, ACTION_SET_CONSUMER_REPLY,
                    biosal_assembly_graph_builder_set_consumer_reply_graph_stores,
                    &concrete_self->graph_stores);

    /*
     * Link the graph stores to the coverage distribution
     */
    thorium_actor_send_range_int(self, &concrete_self->graph_stores,
                    ACTION_SET_CONSUMER,
                    concrete_self->coverage_distribution);
}

void biosal_assembly_graph_builder_set_script_reply_store_manager(struct thorium_actor *self, struct thorium_message *message)
{
        /*
    struct biosal_assembly_graph_builder *concrete_self;
    */

#if 0
    int actor_count;
#endif
    int actors_per_spawner;
    int source;

    /*
     * Verify is the number of actors is too high.
     */
    actors_per_spawner = biosal_assembly_graph_store_get_store_count_per_node(self);
    /*
    actor_count = node_count * actors_per_spawner;
    */
    source = thorium_message_source(message);

    /*
    concrete_self = thorium_actor_concrete_actor(self);
    */

    thorium_actor_add_action_with_source(self, ACTION_MANAGER_SET_ACTORS_PER_SPAWNER_REPLY,
                    biosal_assembly_graph_builder_set_actors_reply_store_manager,
                    source);

    thorium_actor_send_reply_int(self, ACTION_MANAGER_SET_ACTORS_PER_SPAWNER,
                    actors_per_spawner);
}

void biosal_assembly_graph_builder_set_actors_reply_store_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);
    thorium_actor_send_reply_vector(self, ACTION_START, &concrete_self->spawners);
}

void biosal_assembly_graph_builder_start_reply_store_manager(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    int spawner;
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    buffer = thorium_message_buffer(message);

    core_vector_unpack(&concrete_self->graph_stores, buffer);

    printf("%s/%d has %d graph stores for assembly\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    (int)core_vector_size(&concrete_self->graph_stores));

    /* Spawn the manager for the sliding windows manager
     */

    spawner = thorium_actor_get_spawner(self, &concrete_self->spawners);

    /*
     * Register event to happen before
     * sending message (which generates an event)
     */
    concrete_self->manager_for_windows = THORIUM_ACTOR_SPAWNING_IN_PROGRESS;

    thorium_actor_add_action_with_condition(self,
                    ACTION_SPAWN_REPLY,
                    biosal_assembly_graph_builder_spawn_reply_window_manager,
                    &concrete_self->manager_for_windows,
                    THORIUM_ACTOR_SPAWNING_IN_PROGRESS);

    thorium_actor_send_int(self, spawner, ACTION_SPAWN,
                    SCRIPT_MANAGER);
}

void biosal_assembly_graph_builder_spawn_reply_window_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    /*
     * Configure the manager
     * for sliding windows
     */

    thorium_message_unpack_int(message, 0, &concrete_self->manager_for_windows);

    printf("%s/%d says that the manager for windows is %d\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    concrete_self->manager_for_windows);

    thorium_actor_add_action_with_source(self, ACTION_MANAGER_SET_SCRIPT_REPLY,
                    biosal_assembly_graph_builder_set_script_reply_window_manager,
                    concrete_self->manager_for_windows);

    thorium_actor_add_action_with_source(self, ACTION_START_REPLY,
                    biosal_assembly_graph_builder_start_reply_window_manager,
                    concrete_self->manager_for_windows);

    thorium_actor_send_int(self, concrete_self->manager_for_windows, ACTION_MANAGER_SET_SCRIPT,
                    SCRIPT_ASSEMBLY_SLIDING_WINDOW);
}

void biosal_assembly_graph_builder_set_producers(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct biosal_assembly_graph_builder *concrete_self;
    int source;

    source = thorium_message_source(message);

    concrete_self = thorium_actor_concrete_actor(self);
    concrete_self->source = source;

    buffer = thorium_message_buffer(message);

    printf("%s/%d received a %d sequence stores from source %d\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    (int)core_vector_size(&concrete_self->sequence_stores),
                    concrete_self->source);

    /*
     * Unpack sequence stores in the concrete actor.
     */
    core_vector_unpack(&concrete_self->sequence_stores, buffer);

    thorium_actor_send_reply_empty(self, ACTION_SET_PRODUCERS_REPLY);
}

void biosal_assembly_graph_builder_set_script_reply_window_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);
    thorium_actor_send_reply_vector(self, ACTION_START, &concrete_self->spawners);
}

void biosal_assembly_graph_builder_start_reply_window_manager(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct biosal_assembly_graph_builder *concrete_self;
    int spawner;

    concrete_self = thorium_actor_concrete_actor(self);

    buffer = thorium_message_buffer(message);

    /*
     * Unpack the actor names for the sliding
     * windows
     */
    core_vector_unpack(&concrete_self->sliding_windows, buffer);

    thorium_actor_add_action_with_sources(self, ACTION_SET_CONSUMER_REPLY,
                    biosal_assembly_graph_builder_set_consumer_reply_windows,
                    &concrete_self->sliding_windows);

    printf("%s/%d has %d sliding windows for assembly\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    (int)core_vector_size(&concrete_self->sliding_windows));

    spawner = thorium_actor_get_spawner(self, &concrete_self->spawners);

    thorium_actor_send_int(self, spawner, ACTION_SPAWN,
                    SCRIPT_MANAGER);
}

void biosal_assembly_graph_builder_set_script_reply_classifier_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);
    thorium_actor_send_reply_vector(self, ACTION_START, &concrete_self->spawners);
}

void biosal_assembly_graph_builder_start_reply_classifier_manager(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);
    core_vector_unpack(&concrete_self->block_classifiers, buffer);

    printf("%s/%d has %d block classifiers for assembly\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    (int)core_vector_size(&concrete_self->block_classifiers));

    /*
     * Configure the graph builder now.
     */
    biosal_assembly_graph_builder_configure(self);
}

void biosal_assembly_graph_builder_configure(struct thorium_actor *self)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    /*
     * Set the default kmer length
     */
    concrete_self->kmer_length = biosal_assembly_graph_builder_get_kmer_length(self);

#if 0
    printf("EXAMINE: before configuring kmer length\n");
    thorium_actor_print(self);
#endif

    printf("%s/%d configures the kmer length (%d) for the actor computation\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    concrete_self->kmer_length);
    /*
     * set the kmer length for graph stores, sliding windows, and block classifiers
     */

    thorium_actor_send_range_int(self, &concrete_self->graph_stores, ACTION_SET_KMER_LENGTH,
                        concrete_self->kmer_length);

    thorium_actor_send_range_int(self, &concrete_self->sliding_windows, ACTION_SET_KMER_LENGTH,
                        concrete_self->kmer_length);

    thorium_actor_send_range_int(self, &concrete_self->block_classifiers, ACTION_SET_KMER_LENGTH,
                        concrete_self->kmer_length);

#if 0
    /*
     * There will be a response for this.
     */
    printf("EXAMINE: after configuring kmer length\n");
    thorium_actor_print(self);
#endif
}

void biosal_assembly_graph_builder_set_kmer_reply(struct thorium_actor *self, struct thorium_message *message)
{
    int expected;
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);
    ++concrete_self->actors_with_kmer_length;

    expected = 0;
    expected += core_vector_size(&concrete_self->graph_stores);
    expected += core_vector_size(&concrete_self->sliding_windows);
    expected += core_vector_size(&concrete_self->block_classifiers);

    if (concrete_self->actors_with_kmer_length % 100 == 0) {

        printf("EXAMINE: progress SET_KMER_LENGTH %d/%d\n",
                        concrete_self->actors_with_kmer_length,
                        expected);
        thorium_actor_print(self);
    }

    /*
     * Verify if all actors are properly configured
     */
    if (concrete_self->actors_with_kmer_length == expected) {

        printf("EXAMINE: configured kmer length\n");
        thorium_actor_print(self);

        printf("%s/%d configured (%d actors) the kmer length value for sliding windows, block classifiers and graph stores\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self),
                        expected);

        biosal_assembly_graph_builder_connect_actors(self);
    }
}

void biosal_assembly_graph_builder_connect_actors(struct thorium_actor *self)
{
    int i;
    int producer;
    int consumer;
    struct biosal_assembly_graph_builder *concrete_self;
    struct core_vector producers_for_work_stealing;
    struct core_memory_pool *ephemeral_memory;

    concrete_self = thorium_actor_concrete_actor(self);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    core_vector_init(&producers_for_work_stealing, sizeof(int));
    core_vector_set_memory_pool(&producers_for_work_stealing, ephemeral_memory);

    /*
     * sequence store ===> sliding window ===> block classifier ===> graph store
     */

    printf("%s/%d connects actors\n",
            thorium_actor_script_name(self),
            thorium_actor_name(self));

    printf("EXAMINE: connecting actors\n");
    thorium_actor_print(self);

    thorium_actor_add_action_with_sources(self, ACTION_SET_PRODUCERS_FOR_WORK_STEALING_REPLY,
                    biosal_assembly_graph_builder_set_consumer_reply_windows,
                    &concrete_self->sliding_windows);

    for (i = 0; i < core_vector_size(&concrete_self->sliding_windows); ++i) {

        biosal_assembly_graph_builder_get_producers_for_work_stealing(self, &producers_for_work_stealing, i);

        producer = core_vector_at_as_int(&concrete_self->sliding_windows, i);
        thorium_actor_send_vector(self, producer, ACTION_SET_PRODUCERS_FOR_WORK_STEALING,
                        &producers_for_work_stealing);
    }

    printf("EXAMINE: after sending producers for work stealing\n");
    thorium_actor_print(self);

    for (i = 0; i < core_vector_size(&concrete_self->sliding_windows); i++) {

        producer = core_vector_at_as_int(&concrete_self->sliding_windows, i);
        consumer = core_vector_at_as_int(&concrete_self->block_classifiers, i);

        /*
         * Also configure a bunch of alternate producer for the
         * consumer.
         */

        /* set the consumer for sliding window
         */
        thorium_actor_send_int(self, producer, ACTION_SET_CONSUMER,
                        consumer);

        printf("DEBUG neural LINK %d -> %d\n",
                        producer, consumer);

        /*
         * set the consumers for each classifier
         */

        printf("DEBUG sending %d store names to classifier %d\n",
                        (int)core_vector_size(&concrete_self->graph_stores),
                        consumer);
    }

    printf("EXAMINE: before SET_CONSUMERS\n");
    thorium_actor_print(self);

    thorium_actor_send_range_vector(self, &concrete_self->block_classifiers,
                    ACTION_SET_CONSUMERS,
                        &concrete_self->graph_stores);

    core_vector_destroy(&producers_for_work_stealing);

    printf("EXAMINE: after connecting actors\n");
    thorium_actor_print(self);
}

void biosal_assembly_graph_builder_set_consumer_reply(struct thorium_actor *self, struct thorium_message *message)
{
}

void biosal_assembly_graph_builder_set_consumer_reply_graph_stores(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);
    ++concrete_self->configured_graph_stores;

    if (concrete_self->configured_graph_stores == core_vector_size(&concrete_self->graph_stores)) {

        /*
         * ABCD
         */

        thorium_actor_send_range_empty(self, &concrete_self->graph_stores,
                        ACTION_PUSH_DATA);
    }
}

void biosal_assembly_graph_builder_set_consumer_reply_windows(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);
    ++concrete_self->configured_sliding_windows;

#ifdef BIOSAL_ASSEMBLY_GRAPH_BUILDER_DEBUG
    printf("DEBUG windows UP %d\n",
        concrete_self->configured_sliding_windows);
#endif

    biosal_assembly_graph_builder_verify(self);
}

void biosal_assembly_graph_builder_verify(struct thorium_actor *self)
{
    struct biosal_assembly_graph_builder *concrete_self;
    int i;
    int producer;
    int consumer;

    concrete_self = thorium_actor_concrete_actor(self);

    /* Verify if the system is ready to roll
     */
    if (concrete_self->configured_sliding_windows < 2 * core_vector_size(&concrete_self->sliding_windows)) {
        return;
    }

    if (concrete_self->configured_block_classifiers < core_vector_size(&concrete_self->block_classifiers)) {
        return;
    }

    printf("%s/%d is ready to build the graph\n",
            thorium_actor_script_name(self),
            thorium_actor_name(self));

    printf("EXAMINE: ready to build\n");
    thorium_actor_print(self);

    /* Set the producer for every sliding window.
     */

    for (i = 0; i < core_vector_size(&concrete_self->sliding_windows); i++) {

        producer = core_vector_at_as_int(&concrete_self->sequence_stores, i);
        consumer = core_vector_at_as_int(&concrete_self->sliding_windows, i);

        printf("CONFIGURE neural LINK store %d -> window %d\n",
                        producer, consumer);

        thorium_actor_send_int(self, consumer, ACTION_SET_PRODUCER,
                        producer);
    }
}

void biosal_assembly_graph_builder_set_consumers_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);
    ++concrete_self->configured_block_classifiers;

#ifdef BIOSAL_ASSEMBLY_GRAPH_BUILDER_DEBUG
    printf("DEBUG classifiers UP %d\n", concrete_self->configured_block_classifiers);
#endif

    biosal_assembly_graph_builder_verify(self);
}

void biosal_assembly_graph_builder_set_producer_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);
    ++concrete_self->completed_sliding_windows;

    /*
     * Tell the source that the graph is ready now.
     */
    if (concrete_self->completed_sliding_windows == core_vector_size(&concrete_self->sliding_windows)) {

        thorium_actor_send_range_empty(self, &concrete_self->sliding_windows, ACTION_NOTIFY);
    }
}

void biosal_assembly_graph_builder_tell_source(struct thorium_actor *self)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    /*
     * Stop actors
     */
    /*
    thorium_actor_send_range_empty(self, &concrete_self->arc_kernels, ACTION_ASK_TO_STOP);
    */
    /*
     * We need to stop the arc supervisor to stop
     * arc kernels.
     * The current actor can not stop arc kernels directly.
     */

    thorium_actor_send_empty(self, concrete_self->manager_for_arc_kernels, ACTION_ASK_TO_STOP);
    thorium_actor_send_empty(self, concrete_self->manager_for_arc_classifiers, ACTION_ASK_TO_STOP);

    /*
     * Show timer
     */
    core_timer_stop(&concrete_self->arc_timer);
    core_timer_stop(&concrete_self->timer);
    core_timer_print_with_description(&concrete_self->arc_timer,
                    "Build assembly graph / Distribute arcs");
    core_timer_print_with_description(&concrete_self->timer,
                    "Build assembly graph");

    /*
     * Tell somebody about it
     */
    thorium_actor_send_vector(self, concrete_self->source, ACTION_START_REPLY,
                    &concrete_self->graph_stores);
}

int biosal_assembly_graph_builder_get_kmer_length(struct thorium_actor *self)
{
    int kmer_length;
    int argc;
    char **argv;

    argc = thorium_actor_argc(self);
    argv = thorium_actor_argv(self);

    kmer_length = biosal_command_get_kmer_length(argc, argv);

    return kmer_length;
}

void biosal_assembly_graph_builder_notify_reply(struct thorium_actor *self, struct thorium_message *message)
{
    uint64_t produced_kmer_count;
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_message_unpack_uint64_t(message, 0, &produced_kmer_count);

    concrete_self->total_kmer_count += produced_kmer_count;
    ++concrete_self->notified_windows;

    /*
     * All sliding window terminated their job of extracting kmers.
     */
    if (concrete_self->notified_windows == core_vector_size(&concrete_self->sliding_windows)) {

        printf("%s/%d : %" PRIu64 " kmers were generated during the actor computation.\n",
            thorium_actor_script_name(self),
            thorium_actor_name(self),
            concrete_self->total_kmer_count);

        /*
         * Tell all classifiers to flush now.
         */

        thorium_actor_send_range_empty(self, &concrete_self->block_classifiers,
                        ACTION_AGGREGATOR_FLUSH);

        /*
         * Use message passing for control.
         *
         * \see http://dspace.mit.edu/handle/1721.1/6272
         */
        thorium_actor_send_to_self_empty(self, ACTION_ASSEMBLY_GRAPH_BUILDER_CONTROL_COMPLEXITY);
    }
}

void biosal_assembly_graph_builder_control_complexity(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    concrete_self->actual_kmer_count = 0;
    concrete_self->synchronized_graph_stores = 0;

    thorium_actor_send_range_empty(self, &concrete_self->graph_stores,
                    ACTION_STORE_GET_ENTRY_COUNT);
}

void biosal_assembly_graph_builder_get_entry_count_reply(struct thorium_actor *self, struct thorium_message *message)
{
    uint64_t kmer_count;
    struct biosal_assembly_graph_builder *concrete_self;
    int spawner;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_message_unpack_uint64_t(message, 0, &kmer_count);

    concrete_self->actual_kmer_count += kmer_count;

    ++concrete_self->synchronized_graph_stores;

    if (concrete_self->synchronized_graph_stores == core_vector_size(&concrete_self->graph_stores)) {

        printf("graph store synchronization: actual_kmer_count %" PRIu64 " total_kmer_count %" PRIu64 "\n",
                        concrete_self->actual_kmer_count,
                        concrete_self->total_kmer_count);
        if (concrete_self->actual_kmer_count == concrete_self->total_kmer_count) {

            printf("%s/%d synchronized with %d graph stores\n",
                            thorium_actor_script_name(self),
                            thorium_actor_name(self),
                            (int)core_vector_size(&concrete_self->graph_stores));

            /* Generate coverage distribution
             */

            spawner = thorium_actor_get_spawner(self, &concrete_self->spawners);

            thorium_actor_send_int(self, spawner, ACTION_SPAWN,
                            SCRIPT_COVERAGE_DISTRIBUTION);

        } else {
            /*
             * Otherwise, there are still some messages in transit.
             */

            printf("got mismatch, will try again\n");

            thorium_actor_send_to_self_empty(self, ACTION_ASSEMBLY_GRAPH_BUILDER_CONTROL_COMPLEXITY);
        }
    }
}

void biosal_assembly_graph_builder_notify_from_distribution(struct thorium_actor *self, struct thorium_message *message)
{
    int spawner;
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    /*
     * Kill windows and classifiers
     */

    thorium_actor_send_empty(self, concrete_self->manager_for_windows,
                    ACTION_ASK_TO_STOP);

    thorium_actor_send_empty(self, concrete_self->manager_for_classifiers,
                    ACTION_ASK_TO_STOP);

    thorium_actor_send_empty(self, concrete_self->coverage_distribution,
                    ACTION_ASK_TO_STOP);

    core_timer_stop(&concrete_self->vertex_timer);

    core_timer_start(&concrete_self->arc_timer);
    core_timer_print_with_description(&concrete_self->vertex_timer,
                    "Build assembly graph / Distribute vertices");

    /*
     * Spawn manager for arc kernels
     */

    spawner = thorium_actor_get_spawner(self, &concrete_self->spawners);

    concrete_self->manager_for_arc_kernels = THORIUM_ACTOR_SPAWNING_IN_PROGRESS;

    thorium_actor_add_action_with_condition(self, ACTION_SPAWN_REPLY,
                    biosal_assembly_graph_builder_spawn_reply_arc_kernel_manager,
                    &concrete_self->manager_for_arc_kernels,
                    THORIUM_ACTOR_SPAWNING_IN_PROGRESS);

    thorium_actor_send_int(self, spawner, ACTION_SPAWN,
                    SCRIPT_MANAGER);
}

void biosal_assembly_graph_builder_spawn_reply_arc_kernel_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_message_unpack_int(message, 0, &concrete_self->manager_for_arc_kernels);

    thorium_actor_add_action_with_source(self, ACTION_MANAGER_SET_SCRIPT_REPLY,
                    biosal_assembly_graph_builder_set_script_reply_arc_kernel_manager,
                    concrete_self->manager_for_arc_kernels);

    thorium_actor_send_int(self, concrete_self->manager_for_arc_kernels, ACTION_MANAGER_SET_SCRIPT,
                    SCRIPT_ASSEMBLY_ARC_KERNEL);
}

void biosal_assembly_graph_builder_set_script_reply_arc_kernel_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_actor_add_action_with_source(self, ACTION_START_REPLY,
                    biosal_assembly_graph_builder_start_reply_arc_kernel_manager,
                    concrete_self->manager_for_arc_kernels);

    thorium_actor_send_reply_vector(self, ACTION_START, &concrete_self->spawners);
}

void biosal_assembly_graph_builder_start_reply_arc_kernel_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;
    void *buffer;
    int spawner;

    concrete_self = thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);

    core_vector_unpack(&concrete_self->arc_kernels, buffer);

    concrete_self->manager_for_arc_classifiers = THORIUM_ACTOR_SPAWNING_IN_PROGRESS;

    thorium_actor_add_action_with_condition(self, ACTION_SPAWN_REPLY,
                    biosal_assembly_graph_builder_spawn_reply_arc_classifier_manager,
                    &concrete_self->manager_for_arc_classifiers,
                    THORIUM_ACTOR_SPAWNING_IN_PROGRESS);

    spawner = thorium_actor_get_spawner(self, &concrete_self->spawners);

    thorium_actor_send_int(self, spawner, ACTION_SPAWN, SCRIPT_MANAGER);
}

void biosal_assembly_graph_builder_spawn_reply_arc_classifier_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_message_unpack_int(message, 0, &concrete_self->manager_for_arc_classifiers);

    /*
     * Set the script.
     */
    thorium_actor_add_action_with_source(self, ACTION_MANAGER_SET_SCRIPT_REPLY,
                    biosal_assembly_graph_builder_set_script_reply_arc_classifier_manager,
                    concrete_self->manager_for_arc_classifiers);

    thorium_actor_send_int(self, concrete_self->manager_for_arc_classifiers, ACTION_MANAGER_SET_SCRIPT,
                    SCRIPT_ASSEMBLY_ARC_CLASSIFIER);

}

void biosal_assembly_graph_builder_set_script_reply_arc_classifier_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_actor_add_action_with_source(self, ACTION_START_REPLY,
                    biosal_assembly_graph_builder_start_reply_arc_classifier_manager,
                    concrete_self->manager_for_arc_classifiers);

    thorium_actor_send_reply_vector(self, ACTION_START,
                    &concrete_self->spawners);
}

void biosal_assembly_graph_builder_start_reply_arc_classifier_manager(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    buffer = thorium_message_buffer(message);
    core_vector_unpack(&concrete_self->arc_classifiers, buffer);

    /*
     * Configure the kmer length.
     */
    concrete_self->doing_arcs = 1;

    thorium_actor_add_action_with_condition(self, ACTION_SET_KMER_LENGTH_REPLY,
                    biosal_assembly_graph_builder_set_kmer_reply_arcs,
                    &concrete_self->doing_arcs, 1);

    concrete_self->configured_actors_for_arcs = 0;

    thorium_actor_send_range_int(self, &concrete_self->arc_kernels, ACTION_SET_KMER_LENGTH,
                    concrete_self->kmer_length);
    thorium_actor_send_range_int(self, &concrete_self->arc_classifiers, ACTION_SET_KMER_LENGTH,
                    concrete_self->kmer_length);
}

void biosal_assembly_graph_builder_set_kmer_reply_arcs(struct thorium_actor *self, struct thorium_message *message)
{
    int expected;
    struct biosal_assembly_graph_builder *concrete_self;
    int i;
    int producer;
    int consumer;
    int size;
    struct core_vector producers_for_work_stealing;
    struct core_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    concrete_self = thorium_actor_concrete_actor(self);

    ++concrete_self->configured_actors_for_arcs;

    expected = 0;
    expected += core_vector_size(&concrete_self->arc_kernels);
    expected += core_vector_size(&concrete_self->arc_classifiers);

    /*
     * All arc actors have a kmer length configured.
     */
    if (concrete_self->configured_actors_for_arcs < expected) {

        return;
    }

    core_vector_init(&producers_for_work_stealing, sizeof(int));
    core_vector_set_memory_pool(&producers_for_work_stealing,
                ephemeral_memory);

    printf("%s/%d configured the kmer length for arc actors\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self));

    concrete_self->configured_actors_for_arcs = 0;

    thorium_actor_add_action_with_condition(self, ACTION_SET_CONSUMER_REPLY,
                    biosal_assembly_graph_builder_configure_arc_actors,
                    &concrete_self->doing_arcs, 1);

    size = core_vector_size(&concrete_self->arc_kernels);

    /*
     * Design:
     *
     * sequence_store => arc_kernel => arc_classifier => graph_store/0 graph_store/1 graph_store/2
     */

    /*
     * Configure action for reply to the message that sets
     * work stealing producers.
     */
    thorium_actor_add_action_with_sources(self, ACTION_SET_PRODUCERS_FOR_WORK_STEALING_REPLY,
                    biosal_assembly_graph_builder_configure_arc_actors,
                    &concrete_self->arc_kernels);

    for (i = 0; i < size; ++i) {

        biosal_assembly_graph_builder_get_producers_for_work_stealing(self, &producers_for_work_stealing, i);

        producer = core_vector_at_as_int(&concrete_self->arc_kernels, i);

        /*
         * Also set producers for work stealing too.
         */
        thorium_actor_send_vector(self, producer, ACTION_SET_PRODUCERS_FOR_WORK_STEALING,
                        &producers_for_work_stealing);
    }

    /*
     * Link kernels and classifiers
     */
    for (i = 0; i < size; i++) {

        producer = core_vector_at_as_int(&concrete_self->arc_kernels, i);
        consumer = core_vector_at_as_int(&concrete_self->arc_classifiers, i);

        thorium_actor_send_int(self, producer, ACTION_SET_CONSUMER,
                        consumer);
    }

    /*
     * Link classifiers and graph stores
     */

    thorium_actor_add_action_with_condition(self,
                    ACTION_SET_CONSUMERS_REPLY,
                    biosal_assembly_graph_builder_configure_arc_actors,
                    &concrete_self->doing_arcs, 1);

    thorium_actor_send_range_vector(self, &concrete_self->arc_classifiers,
                    ACTION_SET_CONSUMERS,
                    &concrete_self->graph_stores);

    /*
     * Reset the sequence stores now.
     */

    thorium_actor_add_action(self,
                    ACTION_RESET_REPLY,
                    biosal_assembly_graph_builder_configure_arc_actors);

    thorium_actor_send_range_empty(self, &concrete_self->sequence_stores,
                    ACTION_RESET);

    core_vector_destroy(&producers_for_work_stealing);
}

void biosal_assembly_graph_builder_configure_arc_actors(struct thorium_actor *self, struct thorium_message *message)
{
    int expected;
    int i;
    int size;
    int producer;
    int consumer;
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);
    ++concrete_self->configured_actors_for_arcs;

    expected = 0;
    /* For ACTION_SET_CONSUMER_REPLY */
    expected += core_vector_size(&concrete_self->arc_kernels);
    /* For ACTION_SET_PRODUCERS_FOR_WORK_STEALING_REPLY */
    expected += core_vector_size(&concrete_self->arc_kernels);
    /* For ACTION_SET_CONSUMERS_REPLY */
    expected += core_vector_size(&concrete_self->arc_classifiers);
    /* For ACTION_RESET_REPLY */
    expected += core_vector_size(&concrete_self->sequence_stores);

    if (concrete_self->configured_actors_for_arcs % 1000 == 0
                    || concrete_self->configured_actors_for_arcs == expected) {
        printf("PROGRESS ARC configuration %d/%d %d %d %d\n",
                    concrete_self->configured_actors_for_arcs,
                    expected,
                    (int)core_vector_size(&concrete_self->arc_kernels),
                    (int)core_vector_size(&concrete_self->arc_classifiers),
                    (int)core_vector_size(&concrete_self->sequence_stores));
    }

    if (concrete_self->configured_actors_for_arcs < expected) {
        return;
    }

    printf("%s/%d configured the neural links for arc actors\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self));

    concrete_self->completed_arc_kernels = 0;

    /*
     * Start arc computation
     */

    size = core_vector_size(&concrete_self->arc_kernels);

    thorium_actor_add_action_with_condition(self, ACTION_SET_PRODUCER_REPLY,
                    biosal_assembly_graph_builder_verify_arc_kernels,
                    &concrete_self->doing_arcs, 1);

    for (i = 0; i < size; i++) {
        producer = core_vector_at_as_int(&concrete_self->sequence_stores, i);
        consumer = core_vector_at_as_int(&concrete_self->arc_kernels, i);

        thorium_actor_send_int(self, consumer, ACTION_SET_PRODUCER,
                        producer);
    }
}

void biosal_assembly_graph_builder_verify_arc_kernels(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;
    int expected;

    concrete_self = thorium_actor_concrete_actor(self);

    ++concrete_self->completed_arc_kernels;

    expected = core_vector_size(&concrete_self->arc_kernels);

    if (concrete_self->completed_arc_kernels == expected) {

        concrete_self->completed_arc_kernels = 0;

        thorium_actor_add_action_with_condition(self,
                        ACTION_NOTIFY_REPLY,
                        biosal_assembly_graph_builder_notify_reply_arc_kernels,
                        &concrete_self->doing_arcs, 1);
        /*
         * Ask each kernel the amount of produced arcs
         */
        thorium_actor_send_range_empty(self, &concrete_self->arc_kernels,
                        ACTION_NOTIFY);
    }
}

void biosal_assembly_graph_builder_notify_reply_arc_kernels(struct thorium_actor *self, struct thorium_message *message)
{
    int expected;
    uint64_t arcs;
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_message_unpack_uint64_t(message, 0, &arcs);

    concrete_self->expected_arc_count += arcs;

    ++concrete_self->completed_arc_kernels;

    expected = core_vector_size(&concrete_self->arc_kernels);

    if (concrete_self->completed_arc_kernels == expected) {

        printf("%s/%d %" PRIu64 " arcs were extracted from raw data by arc actors\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self),
                        concrete_self->expected_arc_count);

        thorium_actor_add_action(self, ACTION_VERIFY_ARCS,
                        biosal_assembly_graph_builder_verify_arcs);

        thorium_actor_send_to_self_empty(self, ACTION_VERIFY_ARCS);
    }
}

void biosal_assembly_graph_builder_verify_arcs(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_actor_add_action(self, ACTION_ASSEMBLY_GET_SUMMARY_REPLY,
                    biosal_assembly_graph_builder_get_summary_reply);

    concrete_self->ready_graph_store_count = 0;

    thorium_actor_send_range_empty(self, &concrete_self->graph_stores,
                    ACTION_ASSEMBLY_GET_SUMMARY);
}

void biosal_assembly_graph_builder_get_summary_reply(struct thorium_actor *self, struct thorium_message *message)
{
    int expected;
    struct biosal_assembly_graph_builder *concrete_self;
    void *buffer;
    struct biosal_assembly_graph_summary partial_summary;
    char *file_name;
    struct core_string file_name_string;
    char *output_directory;
    int argc;
    char **argv;

    concrete_self = thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);

    biosal_assembly_graph_summary_init(&partial_summary);
    biosal_assembly_graph_summary_unpack(&partial_summary, buffer);

    biosal_assembly_graph_summary_merge(&concrete_self->graph_summary, &partial_summary);
    biosal_assembly_graph_summary_destroy(&partial_summary);

    ++concrete_self->ready_graph_store_count;

    expected = core_vector_size(&concrete_self->graph_stores);

    printf("PROGRESS %d/%d\n", concrete_self->ready_graph_store_count,
                    expected);

    if (concrete_self->ready_graph_store_count == expected) {

        biosal_assembly_graph_summary_print(&concrete_self->graph_summary);

        /*
         * Write summary file too in XML
         */

        argc = thorium_actor_argc(self);
        argv = thorium_actor_argv(self);
        output_directory = biosal_command_get_output_directory(argc, argv);
        core_string_init(&file_name_string, output_directory);
        core_string_append(&file_name_string, "/assembly_graph_summary.xml");
        file_name = core_string_get(&file_name_string);

        biosal_assembly_graph_summary_write_summary(&concrete_self->graph_summary,
                        file_name, concrete_self->kmer_length);

        core_string_destroy(&file_name_string);

        biosal_assembly_graph_builder_tell_source(self);
    }
}

void biosal_assembly_graph_builder_get_producers_for_work_stealing(struct thorium_actor *self, struct core_vector *producers_for_work_stealing,
                int current_index)
{
    struct biosal_assembly_graph_builder *concrete_self;
    int index;
    int producer;
    int store_count;
    int victim_count;
    int i;
    double probability;
    int nodes;
    int workers;
    int actors;
    double probability_of_having_an_alone_actor;
    int steps;

    /*
     * Given N actors,
     * the probability of being picked up as a victim by an actor (1 event)
     * is 1 / N.
     *
     * The probability of not being picked up is 1 - 1/N.
     *
     * If all actors picks up 1 victim, the probability of not being the victim
     * of any actor is
     *
     * (1 - 1/N) ^ ( actors * 1)
     *
     * Some examples:
     */
    /*
     * Now, given N actors, we don't want any of them to be alone.
     *
     * An actor has probability N of being selected.
     *
     *
     */
    nodes = thorium_actor_get_node_count(self);
    workers = thorium_actor_node_worker_count(self);
    actors = nodes * workers;

    /*
     * We don't want any actor alone in the process. Each actor, hopefully,
     * should be qable to get its stuff stolen by a thief.
     * But obviously that's not how life works. Everything is
     * a probability. So let's set that probability to a very small
     * value.
     *
     * 0.0001 is 0.01%.
     *
     * So the probability that at least one of the actors not being selected
     * by any other actor is very low.
     */
    probability_of_having_an_alone_actor = 0.0001;

    /* probability = 1 - pow(M_E, log(1.0 - probability_of_having_an_alone_actor) / actors);*/
    /* Otherwise, M_E can be obtained with:
     * double e = exp(1.0);
     */

    /*
     * Suppose that p is the probability that 1 actor be the victim of no one.
     * (1 - p) is then the probability of being a victim.
     *
     * (1 - p) ^ N, where N is the number of actors, is the probability that every actor
     * is a victim.
     *
     * q is the probability that at least 1 actor is not the victim of anyone.
     * To compute q, we need the probability that every actor is the victim of at
     * least an actor.
     *
     * In the relationship is:
     *
     * probability_of_having_an_alone_actor = 1.0 - pow(1.0 - probability, actors)
     *
     * "pow(1.0 - probability, actors)" represents the probability of having all
     * actors being the victim of at least 1 actor.
     *
     * The complement event is "at least one actor is not the victime of anyone.
     *
     * q = 1 - (1 - p) ^ N
     *
     * Let's isolate p.
     *
     * (1 - p) ^ N = 1 - q
     * (1 - p) = (1 - q) ^ (1 / N)
     * 1 - p = (1 - q) ^ (1 / N)
     * 1 - (1 - q) ^ (1 / N) = p
     * p = 1 - (1 - q) ^ (1 / N) ****
     */

    probability = 1 - pow(1.0 - probability_of_having_an_alone_actor, 1.0 / actors);

#ifdef DISPLAY_WORK_STEALING_STATISTICS
    printf("PROBABILITY p of having an actor not selected by anyone %f, given %d actors\n",
                    probability_of_having_an_alone_actor, actors);

    printf("PROBABILITY p for an actor to not being selected by anyone: %f\n",
                    probability);
#endif

    /*
     * irb(main):011:0> victims = Math.log(1.0/(5*actors))/(actors*Math.log(1 - 1.0/actors))
     */

    victim_count = log(probability) / (actors * log(1 - (1.0 / actors)));

#ifdef DISPLAY_WORK_STEALING_STATISTICS
#endif
    printf("WORK_STEALING DEBUG victim count is %d\n", victim_count);

    /*
     * Algorithm:
     * each actor picks a constant number of peers.
     */

    concrete_self = thorium_actor_concrete_actor(self);

    store_count = core_vector_size(&concrete_self->sequence_stores);

    /*
     * Gather the list of alternate producers.
     */
    core_vector_clear(producers_for_work_stealing);

#ifdef __bDISABLEgq__
    /*
     * Disable this right now
     * because sequence stores are generating very big messages.
     *
     * There is not enough memory on Blue Gene/Q per node.
     */
    victim_count = 0;
#endif

    /*
     * There is the same number of sliding_windows and sequence_stores.
     */
    for (i = 0; i < victim_count; ++i) {
        index = current_index;

        /*
         * Avoid infinite loops if random numbers are very bad.
         */
        steps = 512;

        while (index == current_index
                        && --steps) {
            index = rand() % store_count;
        }

        producer = core_vector_at_as_int(&concrete_self->sequence_stores, index);
        core_vector_push_back(producers_for_work_stealing, &producer);
    }
}
