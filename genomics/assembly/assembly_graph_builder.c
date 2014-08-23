
#include "assembly_graph_builder.h"

#include "assembly_graph_store.h"
#include "assembly_sliding_window.h"
#include "assembly_block_classifier.h"

#include "assembly_arc_kernel.h"
#include "assembly_arc_classifier.h"

#include <core/system/debugger.h>
#include <core/system/command.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

struct thorium_script bsal_assembly_graph_builder_script = {
    .identifier = SCRIPT_ASSEMBLY_GRAPH_BUILDER,
    .name = "bsal_assembly_graph_builder",
    .description = "This builder implements a distributed actor algorithm for building an assembly graph",
    .version = "Cool version",
    .author = "Sebastien Boisvert, Argonne National Laboratory",
    .init = bsal_assembly_graph_builder_init,
    .destroy = bsal_assembly_graph_builder_destroy,
    .receive = bsal_assembly_graph_builder_receive,
    .size = sizeof(struct bsal_assembly_graph_builder)
};

void bsal_assembly_graph_builder_init(struct thorium_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    bsal_timer_init(&concrete_self->timer);
    bsal_timer_init(&concrete_self->vertex_timer);
    bsal_timer_init(&concrete_self->arc_timer);

    concrete_self->completed_sliding_windows = 0;
    concrete_self->doing_arcs = 0;

    bsal_vector_init(&concrete_self->spawners, sizeof(int));
    bsal_vector_init(&concrete_self->sequence_stores, sizeof(int));
    bsal_vector_init(&concrete_self->block_classifiers, sizeof(int));

    thorium_actor_add_action(self, ACTION_START, bsal_assembly_graph_builder_start);
    thorium_actor_add_action(self, ACTION_SET_PRODUCERS, bsal_assembly_graph_builder_set_producers);
    thorium_actor_add_action(self, ACTION_SET_PRODUCER_REPLY, bsal_assembly_graph_builder_set_producer_reply);
    thorium_actor_add_action(self, ACTION_ASK_TO_STOP, bsal_assembly_graph_builder_ask_to_stop);
    thorium_actor_add_action(self, ACTION_SPAWN_REPLY, bsal_assembly_graph_builder_spawn_reply);
    thorium_actor_add_action(self, ACTION_SET_KMER_LENGTH_REPLY, bsal_assembly_graph_builder_set_kmer_reply);
    thorium_actor_add_action(self, ACTION_NOTIFY_REPLY, bsal_assembly_graph_builder_notify_reply);
    thorium_actor_add_action(self, ACTION_ASSEMBLY_GRAPH_BUILDER_CONTROL_COMPLEXITY,
                    bsal_assembly_graph_builder_control_complexity);

    thorium_actor_add_action(self, ACTION_SET_CONSUMER_REPLY, bsal_assembly_graph_builder_set_consumer_reply);
    thorium_actor_add_action(self, ACTION_SET_CONSUMERS_REPLY, bsal_assembly_graph_builder_set_consumers_reply);
    thorium_actor_add_action(self, ACTION_STORE_GET_ENTRY_COUNT_REPLY,
                    bsal_assembly_graph_builder_get_entry_count_reply);

    concrete_self->manager_for_graph_stores = THORIUM_ACTOR_NOBODY;
    bsal_vector_init(&concrete_self->graph_stores, sizeof(int));

    concrete_self->manager_for_classifiers = THORIUM_ACTOR_NOBODY;

    concrete_self->manager_for_windows = THORIUM_ACTOR_NOBODY;
    bsal_vector_init(&concrete_self->sliding_windows, sizeof(int));

    concrete_self->coverage_distribution = THORIUM_ACTOR_NOBODY;

    concrete_self->manager_for_arc_kernels = THORIUM_ACTOR_NOBODY;
    bsal_vector_init(&concrete_self->arc_kernels, sizeof(int));

    concrete_self->manager_for_arc_classifiers = THORIUM_ACTOR_NOBODY;
    bsal_vector_init(&concrete_self->arc_classifiers, sizeof(int));

    concrete_self->configured_graph_stores = 0;
    concrete_self->configured_sliding_windows = 0;
    concrete_self->configured_block_classifiers = 0;

    concrete_self->actors_with_kmer_length = 0;

    concrete_self->total_kmer_count = 0;
    concrete_self->notified_windows = 0;

    concrete_self->synchronized_graph_stores = 0;
    concrete_self->actual_kmer_count = 0;

    concrete_self->expected_arc_count = 0;

    concrete_self->vertex_count = 0;
    concrete_self->vertex_observation_count = 0;
    concrete_self->arc_count = 0;
}

void bsal_assembly_graph_builder_destroy(struct thorium_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    bsal_vector_destroy(&concrete_self->spawners);
    bsal_vector_destroy(&concrete_self->sequence_stores);

    concrete_self->manager_for_graph_stores = THORIUM_ACTOR_NOBODY;
    bsal_vector_destroy(&concrete_self->graph_stores);

    concrete_self->manager_for_classifiers = THORIUM_ACTOR_NOBODY;
    bsal_vector_destroy(&concrete_self->block_classifiers);

    concrete_self->manager_for_windows= THORIUM_ACTOR_NOBODY;
    bsal_vector_destroy(&concrete_self->sliding_windows);

    concrete_self->manager_for_arc_kernels = THORIUM_ACTOR_NOBODY;
    bsal_vector_destroy(&concrete_self->arc_kernels);

    concrete_self->manager_for_arc_classifiers = THORIUM_ACTOR_NOBODY;
    bsal_vector_destroy(&concrete_self->arc_classifiers);

    bsal_timer_destroy(&concrete_self->timer);
    bsal_timer_destroy(&concrete_self->vertex_timer);
    bsal_timer_destroy(&concrete_self->arc_timer);
}

void bsal_assembly_graph_builder_receive(struct thorium_actor *self, struct thorium_message *message)
{
    thorium_actor_take_action(self, message);
}

void bsal_assembly_graph_builder_ask_to_stop(struct thorium_actor *self, struct thorium_message *message)
{
    int name;
    struct bsal_assembly_graph_builder *concrete_self;

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

void bsal_assembly_graph_builder_start(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;
    int source;
    int spawner;

    source = thorium_message_source(message);
    concrete_self = thorium_actor_concrete_actor(self);

    if (source != concrete_self->source) {
        return;
    }

    bsal_timer_start(&concrete_self->timer);
    bsal_timer_start(&concrete_self->vertex_timer);

    buffer = thorium_message_buffer(message);

    bsal_vector_unpack(&concrete_self->spawners, buffer);

    printf("%s/%d received a urgent request from source %d to build an assembly graph using %d sequence stores (producers) and %d spawners\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    source,
                    (int)bsal_vector_size(&concrete_self->sequence_stores),
                    (int)bsal_vector_size(&concrete_self->spawners));

    spawner = thorium_actor_get_spawner(self, &concrete_self->spawners);

    /*
     * Spawn the manager for graph stores
     */
    concrete_self->manager_for_graph_stores = THORIUM_ACTOR_SPAWNING_IN_PROGRESS;

    thorium_actor_add_action_with_source_and_condition(self,
                    ACTION_SPAWN_REPLY,
                    bsal_assembly_graph_builder_spawn_reply_graph_store_manager,
                    spawner,
                    &(concrete_self->manager_for_graph_stores),
                    THORIUM_ACTOR_SPAWNING_IN_PROGRESS);

    thorium_actor_send_int(self, spawner, ACTION_SPAWN, SCRIPT_MANAGER);
}

void bsal_assembly_graph_builder_spawn_reply_graph_store_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    /*
     * Configure the manager for graph stores
     */

    thorium_message_unpack_int(message, 0, &concrete_self->manager_for_graph_stores);

    thorium_actor_add_action_with_source(self, ACTION_MANAGER_SET_SCRIPT_REPLY,
                    bsal_assembly_graph_builder_set_script_reply_store_manager,
                    concrete_self->manager_for_graph_stores);

    thorium_actor_add_action_with_source(self, ACTION_START_REPLY,
                    bsal_assembly_graph_builder_start_reply_store_manager,
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
void bsal_assembly_graph_builder_spawn_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    printf("DEBUG bsal_assembly_graph_builder_spawn_reply\n");

    /*
     * Configure the manager for block classifiers
     */

    if (concrete_self->manager_for_classifiers == THORIUM_ACTOR_NOBODY) {

        thorium_message_unpack_int(message, 0, &concrete_self->manager_for_classifiers);

        thorium_actor_add_action_with_source(self, ACTION_MANAGER_SET_SCRIPT_REPLY,
                        bsal_assembly_graph_builder_set_script_reply_classifier_manager,
                        concrete_self->manager_for_classifiers);

        thorium_actor_add_action_with_source(self, ACTION_START_REPLY,
                        bsal_assembly_graph_builder_start_reply_classifier_manager,
                        concrete_self->manager_for_classifiers);

        thorium_actor_send_int(self, concrete_self->manager_for_classifiers, ACTION_MANAGER_SET_SCRIPT,
                        SCRIPT_ASSEMBLY_BLOCK_CLASSIFIER);

    } else if (concrete_self->coverage_distribution == THORIUM_ACTOR_NOBODY) {

        thorium_message_unpack_int(message, 0, &concrete_self->coverage_distribution);

        thorium_actor_add_action_with_source(self, ACTION_SET_EXPECTED_MESSAGE_COUNT_REPLY,
                            bsal_assembly_graph_builder_set_expected_message_count_reply,
                        concrete_self->coverage_distribution);

        thorium_actor_add_action_with_source(self, ACTION_NOTIFY,
                        bsal_assembly_graph_builder_notify_from_distribution,
                        concrete_self->coverage_distribution);

        thorium_actor_send_int(self, concrete_self->coverage_distribution,
                        ACTION_SET_EXPECTED_MESSAGE_COUNT, (int)bsal_vector_size(&concrete_self->graph_stores));

    } else {

        printf("Warning: unknown state when receiveing ACTION_SPAWN_REPLY\n");
    }
}

void bsal_assembly_graph_builder_set_expected_message_count_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_actor_add_action_with_sources(self, ACTION_SET_CONSUMER_REPLY,
                    bsal_assembly_graph_builder_set_consumer_reply_graph_stores,
                    &concrete_self->graph_stores);

    /*
     * Link the graph stores to the coverage distribution
     */
    thorium_actor_send_range_int(self, &concrete_self->graph_stores,
                    ACTION_SET_CONSUMER,
                    concrete_self->coverage_distribution);
}

void bsal_assembly_graph_builder_set_script_reply_store_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_actor_send_reply_vector(self, ACTION_START, &concrete_self->spawners);
}

void bsal_assembly_graph_builder_start_reply_store_manager(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    int spawner;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    buffer = thorium_message_buffer(message);

    bsal_vector_unpack(&concrete_self->graph_stores, buffer);

    printf("%s/%d has %d graph stores for assembly\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->graph_stores));

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
                    bsal_assembly_graph_builder_spawn_reply_window_manager,
                    &concrete_self->manager_for_windows,
                    THORIUM_ACTOR_SPAWNING_IN_PROGRESS);

    thorium_actor_send_int(self, spawner, ACTION_SPAWN,
                    SCRIPT_MANAGER);

}

void bsal_assembly_graph_builder_spawn_reply_window_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

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
                    bsal_assembly_graph_builder_set_script_reply_window_manager,
                    concrete_self->manager_for_windows);

    thorium_actor_add_action_with_source(self, ACTION_START_REPLY,
                    bsal_assembly_graph_builder_start_reply_window_manager,
                    concrete_self->manager_for_windows);

    thorium_actor_send_int(self, concrete_self->manager_for_windows, ACTION_MANAGER_SET_SCRIPT,
                    SCRIPT_ASSEMBLY_SLIDING_WINDOW);

}

void bsal_assembly_graph_builder_set_producers(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;
    int source;

    source = thorium_message_source(message);


    concrete_self = thorium_actor_concrete_actor(self);
    concrete_self->source = source;

    buffer = thorium_message_buffer(message);

    printf("%s/%d received a %d sequence stores from source %d\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->sequence_stores),
                    concrete_self->source);

    bsal_vector_unpack(&concrete_self->sequence_stores, buffer);

    thorium_actor_send_reply_empty(self, ACTION_SET_PRODUCERS_REPLY);

}

void bsal_assembly_graph_builder_set_script_reply_window_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_actor_send_reply_vector(self, ACTION_START, &concrete_self->spawners);
}

void bsal_assembly_graph_builder_start_reply_window_manager(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;
    int spawner;

    concrete_self = thorium_actor_concrete_actor(self);

    buffer = thorium_message_buffer(message);

    /*
     * Unpack the actor names for the sliding
     * windows
     */
    bsal_vector_unpack(&concrete_self->sliding_windows, buffer);

    thorium_actor_add_action_with_sources(self, ACTION_SET_CONSUMER_REPLY,
                    bsal_assembly_graph_builder_set_consumer_reply_windows,
                    &concrete_self->sliding_windows);

    printf("%s/%d has %d sliding windows for assembly\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->sliding_windows));

    spawner = thorium_actor_get_spawner(self, &concrete_self->spawners);

    thorium_actor_send_int(self, spawner, ACTION_SPAWN,
                    SCRIPT_MANAGER);
}

void bsal_assembly_graph_builder_set_script_reply_classifier_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_actor_send_reply_vector(self, ACTION_START, &concrete_self->spawners);
}

void bsal_assembly_graph_builder_start_reply_classifier_manager(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    buffer = thorium_message_buffer(message);

    bsal_vector_unpack(&concrete_self->block_classifiers, buffer);

    printf("%s/%d has %d block classifiers for assembly\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->block_classifiers));

    /*
     * Configure the graph builder now.
     */
    bsal_assembly_graph_builder_configure(self);

}

void bsal_assembly_graph_builder_configure(struct thorium_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;
    int destination;
    int i;

    concrete_self = thorium_actor_concrete_actor(self);

    /*
     * Set the default kmer length
     */
    concrete_self->kmer_length = bsal_assembly_graph_builder_get_kmer_length(self);


    printf("%s/%d configures the kmer length (%d) for the actor computation\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    concrete_self->kmer_length);


    /*
     * set the kmer length for graph stores, sliding windows, and block classifiers
     */

    for (i = 0; i < bsal_vector_size(&concrete_self->graph_stores); i++) {

        destination = bsal_vector_at_as_int(&concrete_self->graph_stores, i);

        thorium_actor_send_int(self, destination, ACTION_SET_KMER_LENGTH,
                        concrete_self->kmer_length);
    }

    for (i = 0; i < bsal_vector_size(&concrete_self->sliding_windows); i++) {

        destination = bsal_vector_at_as_int(&concrete_self->sliding_windows, i);

        thorium_actor_send_int(self, destination, ACTION_SET_KMER_LENGTH,
                        concrete_self->kmer_length);
    }

    for (i = 0; i < bsal_vector_size(&concrete_self->block_classifiers); i++) {

        destination = bsal_vector_at_as_int(&concrete_self->block_classifiers, i);

        thorium_actor_send_int(self, destination, ACTION_SET_KMER_LENGTH,
                        concrete_self->kmer_length);
    }

}

void bsal_assembly_graph_builder_set_kmer_reply(struct thorium_actor *self, struct thorium_message *message)
{
    int expected;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    ++concrete_self->actors_with_kmer_length;

    expected = 0;

    expected += bsal_vector_size(&concrete_self->graph_stores);
    expected += bsal_vector_size(&concrete_self->sliding_windows);
    expected += bsal_vector_size(&concrete_self->block_classifiers);

    /*
     * Verify if all actors are properly configured
     */
    if (concrete_self->actors_with_kmer_length == expected) {

        printf("%s/%d configured (%d actors) the kmer length value for sliding windows, block classifiers and graph stores\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self),
                        expected);

        bsal_assembly_graph_builder_connect_actors(self);
    }
}

void bsal_assembly_graph_builder_connect_actors(struct thorium_actor *self)
{
    int i;
    int producer;
    int consumer;
    struct bsal_assembly_graph_builder *concrete_self;
    struct bsal_vector alternate_producers;
    struct bsal_memory_pool *ephemeral_memory;
    int period;
    int j;

    concrete_self = thorium_actor_concrete_actor(self);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    bsal_vector_init(&alternate_producers, sizeof(int));
    bsal_vector_set_memory_pool(&alternate_producers, ephemeral_memory);

    /*
     * sequence store ===> sliding window ===> block classifier ===> graph store
     */

    /*
     * TODO: instead of 8, the code could use the topology.
     */
    period = 8;

    printf("%s/%d connects actors\n",
            thorium_actor_script_name(self),
            thorium_actor_name(self));

    thorium_actor_add_action_with_sources(self, ACTION_SET_PRODUCERS_FOR_WORK_STEALING_REPLY,
                    bsal_assembly_graph_builder_set_consumer_reply_windows,
                    &concrete_self->sliding_windows);

    for (i = 0; i < bsal_vector_size(&concrete_self->sliding_windows); i++) {

        /*
         * Gather the list of alternate producers.
         */

        if (i % period == 0) {

            j = 0;

            bsal_vector_clear(&alternate_producers);

            /*
             * There is the same number of sliding_windows and sequence_stores.
             */
            while (j < period
                            && i + j < bsal_vector_size(&concrete_self->sequence_stores)) {

                producer = bsal_vector_at_as_int(&concrete_self->sequence_stores, i + j);

                bsal_vector_push_back(&alternate_producers, &producer);

                ++j;
            }
        }

        producer = bsal_vector_at_as_int(&concrete_self->sliding_windows, i);
        consumer = bsal_vector_at_as_int(&concrete_self->block_classifiers, i);

        /* set the consumer for sliding window
         */
        thorium_actor_send_int(self, producer, ACTION_SET_CONSUMER,
                        consumer);

        printf("DEBUG neural LINK %d -> %d\n",
                        producer, consumer);

        /*
         * Also configure a bunch of alternate producer for the
         * consumer.
         */

        thorium_actor_send_vector(self, producer, ACTION_SET_PRODUCERS_FOR_WORK_STEALING,
                        &alternate_producers);

        /*
         * set the consumers for each classifier
         */

        printf("DEBUG sending %d store names to classifier %d\n",
                        (int)bsal_vector_size(&concrete_self->graph_stores),
                        consumer);

        thorium_actor_send_vector(self, consumer, ACTION_SET_CONSUMERS,
                        &concrete_self->graph_stores);
    }

    bsal_vector_destroy(&alternate_producers);
}

void bsal_assembly_graph_builder_set_consumer_reply(struct thorium_actor *self, struct thorium_message *message)
{
}

void bsal_assembly_graph_builder_set_consumer_reply_graph_stores(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    ++concrete_self->configured_graph_stores;

    if (concrete_self->configured_graph_stores == bsal_vector_size(&concrete_self->graph_stores)) {

        /*
         * ABCD
         */

        thorium_actor_send_range_empty(self, &concrete_self->graph_stores,
                        ACTION_PUSH_DATA);
    }
}

void bsal_assembly_graph_builder_set_consumer_reply_windows(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    ++concrete_self->configured_sliding_windows;

#ifdef BSAL_ASSEMBLY_GRAPH_BUILDER_DEBUG
    printf("DEBUG windows UP %d\n",
        concrete_self->configured_sliding_windows);
#endif

    bsal_assembly_graph_builder_verify(self);
}

void bsal_assembly_graph_builder_verify(struct thorium_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;
    int i;
    int producer;
    int consumer;

    concrete_self = thorium_actor_concrete_actor(self);

    /* Verify if the system is ready to roll
     */
    if (concrete_self->configured_sliding_windows < 2 * bsal_vector_size(&concrete_self->sliding_windows)) {
        return;
    }

    if (concrete_self->configured_block_classifiers < bsal_vector_size(&concrete_self->block_classifiers)) {
        return;
    }

    printf("%s/%d is ready to build the graph\n",
            thorium_actor_script_name(self),
            thorium_actor_name(self));


    /* Set the producer for every sliding window.
     */

    for (i = 0; i < bsal_vector_size(&concrete_self->sliding_windows); i++) {

        producer = bsal_vector_at_as_int(&concrete_self->sequence_stores, i);
        consumer = bsal_vector_at_as_int(&concrete_self->sliding_windows, i);

        printf("CONFIGURE neural LINK %d -> %d\n",
                        producer, consumer);

        thorium_actor_send_int(self, consumer, ACTION_SET_PRODUCER,
                        producer);
    }

}

void bsal_assembly_graph_builder_set_consumers_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    ++concrete_self->configured_block_classifiers;

#ifdef BSAL_ASSEMBLY_GRAPH_BUILDER_DEBUG
    printf("DEBUG classifiers UP %d\n", concrete_self->configured_block_classifiers);
#endif

    bsal_assembly_graph_builder_verify(self);
}

void bsal_assembly_graph_builder_set_producer_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    ++concrete_self->completed_sliding_windows;

    /*
     * Tell the source that the graph is ready now.
     */
    if (concrete_self->completed_sliding_windows == bsal_vector_size(&concrete_self->sliding_windows)) {

        thorium_actor_send_range_empty(self, &concrete_self->sliding_windows, ACTION_NOTIFY);
    }
}

void bsal_assembly_graph_builder_tell_source(struct thorium_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;

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
    bsal_timer_stop(&concrete_self->arc_timer);
    bsal_timer_stop(&concrete_self->timer);
    bsal_timer_print_with_description(&concrete_self->arc_timer,
                    "Build assembly graph / Distribute arcs");
    bsal_timer_print_with_description(&concrete_self->timer,
                    "Build assembly graph");

    /*
     * Tell somebody about it
     */
    thorium_actor_send_vector(self, concrete_self->source, ACTION_START_REPLY,
                    &concrete_self->graph_stores);
}

int bsal_assembly_graph_builder_get_kmer_length(struct thorium_actor *self)
{
    int kmer_length;
    int argc;
    char **argv;

    argc = thorium_actor_argc(self);
    argv = thorium_actor_argv(self);

    kmer_length = BSAL_DEFAULT_KMER_LENGTH;
    kmer_length = bsal_command_get_kmer_length(argc, argv);

    return kmer_length;
}

void bsal_assembly_graph_builder_notify_reply(struct thorium_actor *self, struct thorium_message *message)
{
    uint64_t produced_kmer_count;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_message_unpack_uint64_t(message, 0, &produced_kmer_count);

    concrete_self->total_kmer_count += produced_kmer_count;

    ++concrete_self->notified_windows;

    if (concrete_self->notified_windows == bsal_vector_size(&concrete_self->sliding_windows)) {

        printf("%s/%d : %" PRIu64 " kmers were generated during the actor computation.\n",
            thorium_actor_script_name(self),
            thorium_actor_name(self),
            concrete_self->total_kmer_count);

        thorium_actor_send_to_self_empty(self, ACTION_ASSEMBLY_GRAPH_BUILDER_CONTROL_COMPLEXITY);
    }
}

void bsal_assembly_graph_builder_control_complexity(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    concrete_self->actual_kmer_count = 0;
    concrete_self->synchronized_graph_stores = 0;

    thorium_actor_send_range_empty(self, &concrete_self->graph_stores,
                    ACTION_STORE_GET_ENTRY_COUNT);
}

void bsal_assembly_graph_builder_get_entry_count_reply(struct thorium_actor *self, struct thorium_message *message)
{
    uint64_t kmer_count;
    struct bsal_assembly_graph_builder *concrete_self;
    int spawner;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_message_unpack_uint64_t(message, 0, &kmer_count);

    concrete_self->actual_kmer_count += kmer_count;

    ++concrete_self->synchronized_graph_stores;

    if (concrete_self->synchronized_graph_stores == bsal_vector_size(&concrete_self->graph_stores)) {

        if (concrete_self->actual_kmer_count == concrete_self->total_kmer_count) {

            printf("%s/%d synchronized with %d graph stores\n",
                            thorium_actor_script_name(self),
                            thorium_actor_name(self),
                            (int)bsal_vector_size(&concrete_self->graph_stores));

            /* Generate coverage distribution
             */

            spawner = thorium_actor_get_spawner(self, &concrete_self->spawners);

            thorium_actor_send_int(self, spawner, ACTION_SPAWN,
                            SCRIPT_COVERAGE_DISTRIBUTION);

        } else {
            /*
             * Otherwise, there are still some messages in transit.
             */

            thorium_actor_send_to_self_empty(self, ACTION_ASSEMBLY_GRAPH_BUILDER_CONTROL_COMPLEXITY);
        }
    }
}

void bsal_assembly_graph_builder_notify_from_distribution(struct thorium_actor *self, struct thorium_message *message)
{
    int spawner;
    struct bsal_assembly_graph_builder *concrete_self;

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

    bsal_timer_stop(&concrete_self->vertex_timer);

    bsal_timer_start(&concrete_self->arc_timer);
    bsal_timer_print_with_description(&concrete_self->vertex_timer,
                    "Build assembly graph / Distribute vertices");

    /*
     * Spawn manager for arc kernels
     */

    spawner = thorium_actor_get_spawner(self, &concrete_self->spawners);

    concrete_self->manager_for_arc_kernels = THORIUM_ACTOR_SPAWNING_IN_PROGRESS;

    thorium_actor_add_action_with_condition(self, ACTION_SPAWN_REPLY,
                    bsal_assembly_graph_builder_spawn_reply_arc_kernel_manager,
                    &concrete_self->manager_for_arc_kernels,
                    THORIUM_ACTOR_SPAWNING_IN_PROGRESS);

    thorium_actor_send_int(self, spawner, ACTION_SPAWN,
                    SCRIPT_MANAGER);
}

void bsal_assembly_graph_builder_spawn_reply_arc_kernel_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_message_unpack_int(message, 0, &concrete_self->manager_for_arc_kernels);

    thorium_actor_add_action_with_source(self, ACTION_MANAGER_SET_SCRIPT_REPLY,
                    bsal_assembly_graph_builder_set_script_reply_arc_kernel_manager,
                    concrete_self->manager_for_arc_kernels);

    thorium_actor_send_int(self, concrete_self->manager_for_arc_kernels, ACTION_MANAGER_SET_SCRIPT,
                    SCRIPT_ASSEMBLY_ARC_KERNEL);
}

void bsal_assembly_graph_builder_set_script_reply_arc_kernel_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_actor_add_action_with_source(self, ACTION_START_REPLY,
                    bsal_assembly_graph_builder_start_reply_arc_kernel_manager,
                    concrete_self->manager_for_arc_kernels);

    thorium_actor_send_reply_vector(self, ACTION_START, &concrete_self->spawners);
}

void bsal_assembly_graph_builder_start_reply_arc_kernel_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;
    void *buffer;
    int spawner;

    concrete_self = thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);

    bsal_vector_unpack(&concrete_self->arc_kernels, buffer);

    concrete_self->manager_for_arc_classifiers = THORIUM_ACTOR_SPAWNING_IN_PROGRESS;

    thorium_actor_add_action_with_condition(self, ACTION_SPAWN_REPLY,
                    bsal_assembly_graph_builder_spawn_reply_arc_classifier_manager,
                    &concrete_self->manager_for_arc_classifiers,
                    THORIUM_ACTOR_SPAWNING_IN_PROGRESS);

    spawner = thorium_actor_get_spawner(self, &concrete_self->spawners);

    thorium_actor_send_int(self, spawner, ACTION_SPAWN, SCRIPT_MANAGER);
}

void bsal_assembly_graph_builder_spawn_reply_arc_classifier_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_message_unpack_int(message, 0, &concrete_self->manager_for_arc_classifiers);

    /*
     * Set the script.
     */
    thorium_actor_add_action_with_source(self, ACTION_MANAGER_SET_SCRIPT_REPLY,
                    bsal_assembly_graph_builder_set_script_reply_arc_classifier_manager,
                    concrete_self->manager_for_arc_classifiers);

    thorium_actor_send_int(self, concrete_self->manager_for_arc_classifiers, ACTION_MANAGER_SET_SCRIPT,
                    SCRIPT_ASSEMBLY_ARC_CLASSIFIER);

}

void bsal_assembly_graph_builder_set_script_reply_arc_classifier_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_actor_add_action_with_source(self, ACTION_START_REPLY,
                    bsal_assembly_graph_builder_start_reply_arc_classifier_manager,
                    concrete_self->manager_for_arc_classifiers);

    thorium_actor_send_reply_vector(self, ACTION_START,
                    &concrete_self->spawners);
}

void bsal_assembly_graph_builder_start_reply_arc_classifier_manager(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    buffer = thorium_message_buffer(message);
    bsal_vector_unpack(&concrete_self->arc_classifiers, buffer);

    /*
     * Configure the kmer length.
     */
    concrete_self->doing_arcs = 1;

    thorium_actor_add_action_with_condition(self, ACTION_SET_KMER_LENGTH_REPLY,
                    bsal_assembly_graph_builder_set_kmer_reply_arcs,
                    &concrete_self->doing_arcs, 1);

    concrete_self->configured_actors_for_arcs = 0;

    thorium_actor_send_range_int(self, &concrete_self->arc_kernels, ACTION_SET_KMER_LENGTH,
                    concrete_self->kmer_length);
    thorium_actor_send_range_int(self, &concrete_self->arc_classifiers, ACTION_SET_KMER_LENGTH,
                    concrete_self->kmer_length);
}

void bsal_assembly_graph_builder_set_kmer_reply_arcs(struct thorium_actor *self, struct thorium_message *message)
{
    int expected;
    struct bsal_assembly_graph_builder *concrete_self;
    int i;
    int producer;
    int consumer;
    int size;

    concrete_self = thorium_actor_concrete_actor(self);

    ++concrete_self->configured_actors_for_arcs;

    expected = 0;
    expected += bsal_vector_size(&concrete_self->arc_kernels);
    expected += bsal_vector_size(&concrete_self->arc_classifiers);

    /*
     * All arc actors not have a kmer length configured.
     */
    if (concrete_self->configured_actors_for_arcs == expected) {

        printf("%s/%d configured the kmer length for arc actors\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self));

        concrete_self->configured_actors_for_arcs = 0;

        thorium_actor_add_action_with_condition(self, ACTION_SET_CONSUMER_REPLY,
                        bsal_assembly_graph_builder_configure_arc_actors,
                        &concrete_self->doing_arcs, 1);

        size = bsal_vector_size(&concrete_self->arc_kernels);

        /*
         * Design:
         *
         * sequence_store => arc_kernel => arc_classifier => graph_store/0 graph_store/1 graph_store/2
         */

        /*
         * Link kernels and classifiers
         */
        for (i = 0; i < size; i++) {

            producer = bsal_vector_at_as_int(&concrete_self->arc_kernels, i);
            consumer = bsal_vector_at_as_int(&concrete_self->arc_classifiers, i);

            thorium_actor_send_int(self, producer, ACTION_SET_CONSUMER,
                            consumer);
        }

        /*
         * Link classifiers and graph stores
         */

        thorium_actor_add_action_with_condition(self,
                        ACTION_SET_CONSUMERS_REPLY,
                        bsal_assembly_graph_builder_configure_arc_actors,
                        &concrete_self->doing_arcs, 1);

        thorium_actor_send_range_vector(self, &concrete_self->arc_classifiers,
                        ACTION_SET_CONSUMERS,
                        &concrete_self->graph_stores);

        /*
         * Reset the sequence stores now.
         */

        thorium_actor_add_action(self,
                        ACTION_RESET_REPLY,
                        bsal_assembly_graph_builder_configure_arc_actors);

        thorium_actor_send_range_empty(self, &concrete_self->sequence_stores,
                        ACTION_RESET);
    }
}

void bsal_assembly_graph_builder_configure_arc_actors(struct thorium_actor *self, struct thorium_message *message)
{
    int expected;
    int i;
    int size;
    int producer;
    int consumer;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    ++concrete_self->configured_actors_for_arcs;

    expected = 0;
    expected += bsal_vector_size(&concrete_self->arc_kernels);
    expected += bsal_vector_size(&concrete_self->arc_classifiers);
    expected += bsal_vector_size(&concrete_self->sequence_stores);

    printf("PROGRESS ARC configuration %d/%d %d %d %d\n",
                    concrete_self->configured_actors_for_arcs,
                    expected,
                    (int)bsal_vector_size(&concrete_self->arc_kernels),
                    (int)bsal_vector_size(&concrete_self->arc_classifiers),
                    (int)bsal_vector_size(&concrete_self->sequence_stores));

    if (concrete_self->configured_actors_for_arcs == expected) {

        printf("%s/%d configured the neural links for arc actors\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self));

        concrete_self->completed_arc_kernels = 0;

        /*
         * Start arc computation
         */

        size = bsal_vector_size(&concrete_self->arc_kernels);

        thorium_actor_add_action_with_condition(self, ACTION_SET_PRODUCER_REPLY,
                        bsal_assembly_graph_builder_verify_arc_kernels,
                        &concrete_self->doing_arcs, 1);

        for (i = 0; i < size; i++) {
            producer = bsal_vector_at_as_int(&concrete_self->sequence_stores, i);
            consumer = bsal_vector_at_as_int(&concrete_self->arc_kernels, i);

            thorium_actor_send_int(self, consumer, ACTION_SET_PRODUCER,
                            producer);
        }
    }
}

void bsal_assembly_graph_builder_verify_arc_kernels(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;
    int expected;

    concrete_self = thorium_actor_concrete_actor(self);

    ++concrete_self->completed_arc_kernels;

    expected = bsal_vector_size(&concrete_self->arc_kernels);

    if (concrete_self->completed_arc_kernels == expected) {

        concrete_self->completed_arc_kernels = 0;

        thorium_actor_add_action_with_condition(self,
                        ACTION_NOTIFY_REPLY,
                        bsal_assembly_graph_builder_notify_reply_arc_kernels,
                        &concrete_self->doing_arcs, 1);
        /*
         * Ask each kernel the amount of produced arcs
         */
        thorium_actor_send_range_empty(self, &concrete_self->arc_kernels,
                        ACTION_NOTIFY);
    }
}

void bsal_assembly_graph_builder_notify_reply_arc_kernels(struct thorium_actor *self, struct thorium_message *message)
{
    int expected;
    uint64_t arcs;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_message_unpack_uint64_t(message, 0, &arcs);

    concrete_self->expected_arc_count += arcs;

    ++concrete_self->completed_arc_kernels;

    expected = bsal_vector_size(&concrete_self->arc_kernels);

    if (concrete_self->completed_arc_kernels == expected) {

        printf("%s/%d %" PRIu64 " arcs were extracted from raw data by arc actors\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self),
                        concrete_self->expected_arc_count);

        thorium_actor_add_action(self, ACTION_VERIFY_ARCS,
                        bsal_assembly_graph_builder_verify_arcs);

        thorium_actor_send_to_self_empty(self, ACTION_VERIFY_ARCS);
    }
}

void bsal_assembly_graph_builder_verify_arcs(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    thorium_actor_add_action(self, ACTION_ASSEMBLY_GET_SUMMARY_REPLY,
                    bsal_assembly_graph_builder_get_summary_reply);

    concrete_self->ready_graph_store_count = 0;

    thorium_actor_send_range_empty(self, &concrete_self->graph_stores,
                    ACTION_ASSEMBLY_GET_SUMMARY);
}

void bsal_assembly_graph_builder_get_summary_reply(struct thorium_actor *self, struct thorium_message *message)
{
    int expected;
    struct bsal_assembly_graph_builder *concrete_self;
    struct bsal_vector vector;
    void *buffer;
    uint64_t vertex_count;
    uint64_t vertex_observation_count;
    uint64_t arc_count;
    int position;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    concrete_self = thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);

    bsal_vector_init(&vector, sizeof(int));
    bsal_vector_set_memory_pool(&vector, ephemeral_memory);

    bsal_vector_unpack(&vector, buffer);

    BSAL_DEBUGGER_ASSERT(bsal_vector_size(&vector) == 3);

    position = 0;
    vertex_count = bsal_vector_at_as_uint64_t(&vector, position);
    ++position;
    vertex_observation_count = bsal_vector_at_as_uint64_t(&vector, position);
    ++position;
    arc_count = bsal_vector_at_as_uint64_t(&vector, position);
    ++position;

    concrete_self->vertex_count += vertex_count;
    concrete_self->vertex_observation_count += vertex_observation_count;
    concrete_self->arc_count += arc_count;

    ++concrete_self->ready_graph_store_count;

    expected = bsal_vector_size(&concrete_self->graph_stores);

    printf("PROGRESS %d/%d\n", concrete_self->ready_graph_store_count,
                    expected);

    if (concrete_self->ready_graph_store_count == expected) {

        printf("GRAPH %s/%d ->",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self));

        printf(" %" PRIu64 " canonical vertices,",
                        concrete_self->vertex_count);
        printf(" %" PRIu64 " canonical vertex observations,",
                        concrete_self->vertex_observation_count);
        printf(" and %" PRIu64 " canonical arcs.",
                        concrete_self->arc_count);

        printf("\n");

        printf("GRAPH %s/%d ->",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self));


        printf(" %" PRIu64 " vertices,",
                        2 * concrete_self->vertex_count);
        printf(" %" PRIu64 " vertex observations,",
                        2 * concrete_self->vertex_observation_count);
        printf(" and %" PRIu64 " arcs.",
                        2 * concrete_self->arc_count);

        printf("\n");

        bsal_assembly_graph_builder_tell_source(self);
    }

    bsal_vector_destroy(&vector);
}
