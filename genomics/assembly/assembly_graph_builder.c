
#include "assembly_graph_builder.h"

#include "assembly_graph_store.h"
#include "assembly_sliding_window.h"
#include "assembly_block_classifier.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

struct bsal_script bsal_assembly_graph_builder_script = {
    .identifier = BSAL_ASSEMBLY_GRAPH_BUILDER_SCRIPT,
    .name = BSAL_ASSEMBLY_GRAPH_BUILDER_NAME,
    .description = BSAL_ASSEMBLY_GRAPH_BUILDER_DESCRIPTION,
    .version = BSAL_ASSEMBLY_GRAPH_BUILDER_VERSION,
    .author = BSAL_ASSEMBLY_GRAPH_BUILDER_AUTHOR,
    .init = bsal_assembly_graph_builder_init,
    .destroy = bsal_assembly_graph_builder_destroy,
    .receive = bsal_assembly_graph_builder_receive,
    .size = sizeof(struct bsal_assembly_graph_builder)
};

void bsal_assembly_graph_builder_init(struct bsal_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_timer_init(&concrete_self->timer);

    concrete_self->completed_sliding_windows = 0;

    bsal_vector_init(&concrete_self->spawners, sizeof(int));
    bsal_vector_init(&concrete_self->sequence_stores, sizeof(int));
    bsal_vector_init(&concrete_self->block_classifiers, sizeof(int));

    bsal_actor_add_route(self, BSAL_ACTOR_START, bsal_assembly_graph_builder_start);
    bsal_actor_add_route(self, BSAL_ACTOR_SET_PRODUCERS, bsal_assembly_graph_builder_set_producers);
    bsal_actor_add_route(self, BSAL_ACTOR_SET_PRODUCER_REPLY, bsal_assembly_graph_builder_set_producer_reply);
    bsal_actor_add_route(self, BSAL_ACTOR_ASK_TO_STOP, bsal_assembly_graph_builder_ask_to_stop);
    bsal_actor_add_route(self, BSAL_ACTOR_SPAWN_REPLY, bsal_assembly_graph_builder_spawn_reply);
    bsal_actor_add_route(self, BSAL_SET_KMER_LENGTH_REPLY, bsal_assembly_graph_builder_set_kmer_reply);
    bsal_actor_add_route(self, BSAL_ACTOR_NOTIFY_REPLY, bsal_assembly_graph_builder_notify_reply);
    bsal_actor_add_route(self, BSAL_ASSEMBLY_GRAPH_BUILDER_CONTROL_COMPLEXITY,
                    bsal_assembly_graph_builder_control_complexity);

    bsal_actor_add_route(self, BSAL_ACTOR_SET_CONSUMER_REPLY, bsal_assembly_graph_builder_set_consumer_reply);
    bsal_actor_add_route(self, BSAL_ACTOR_SET_CONSUMERS_REPLY, bsal_assembly_graph_builder_set_consumers_reply);
    bsal_actor_add_route(self, BSAL_STORE_GET_ENTRY_COUNT_REPLY,
                    bsal_assembly_graph_builder_get_entry_count_reply);

    concrete_self->manager_for_graph_stores = BSAL_ACTOR_NOBODY;
    bsal_vector_init(&concrete_self->graph_stores, sizeof(int));

    concrete_self->manager_for_classifiers = BSAL_ACTOR_NOBODY;

    concrete_self->manager_for_windows = BSAL_ACTOR_NOBODY;
    bsal_vector_init(&concrete_self->sliding_windows, sizeof(int));

    concrete_self->coverage_distribution = BSAL_ACTOR_NOBODY;

    concrete_self->configured_graph_stores = 0;
    concrete_self->configured_sliding_windows = 0;
    concrete_self->configured_block_classifiers = 0;

    concrete_self->actors_with_kmer_length = 0;

    concrete_self->total_kmer_count = 0;
    concrete_self->notified_windows = 0;

    concrete_self->synchronized_graph_stores = 0;
    concrete_self->actual_kmer_count = 0;
}

void bsal_assembly_graph_builder_destroy(struct bsal_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_vector_destroy(&concrete_self->spawners);
    bsal_vector_destroy(&concrete_self->sequence_stores);

    concrete_self->manager_for_graph_stores = BSAL_ACTOR_NOBODY;
    bsal_vector_destroy(&concrete_self->graph_stores);

    concrete_self->manager_for_classifiers = BSAL_ACTOR_NOBODY;
    bsal_vector_destroy(&concrete_self->block_classifiers);

    concrete_self->manager_for_windows= BSAL_ACTOR_NOBODY;
    bsal_vector_destroy(&concrete_self->sliding_windows);

    bsal_timer_destroy(&concrete_self->timer);
}

void bsal_assembly_graph_builder_receive(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_call_handler(self, message);
}

void bsal_assembly_graph_builder_ask_to_stop(struct bsal_actor *self, struct bsal_message *message)
{
    int name;

    name = bsal_actor_name(self);

    printf("builder/%d dies\n", name);

    bsal_actor_ask_to_stop(self, message);
}

void bsal_assembly_graph_builder_start(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;
    int source;
    int spawner;

    source = bsal_message_source(message);
    concrete_self = bsal_actor_concrete_actor(self);

    if (source != concrete_self->source) {
        return;
    }

    bsal_timer_start(&concrete_self->timer);

    buffer = bsal_message_buffer(message);

    bsal_vector_unpack(&concrete_self->spawners, buffer);

    printf("%s/%d received a urgent request from source %d to build an assembly graph using %d sequence stores (producers) and %d spawners\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    source,
                    (int)bsal_vector_size(&concrete_self->sequence_stores),
                    (int)bsal_vector_size(&concrete_self->spawners));

    spawner = bsal_actor_get_spawner(self, &concrete_self->spawners);

    /*
     * Spawn the manager for graph stores
     */
    concrete_self->manager_for_graph_stores = BSAL_ACTOR_SPAWNING_IN_PROGRESS;

    bsal_actor_add_route_with_source_and_condition(self,
                    BSAL_ACTOR_SPAWN_REPLY,
                    bsal_assembly_graph_builder_spawn_reply_graph_store_manager,
                    spawner,
                    &(concrete_self->manager_for_graph_stores),
                    BSAL_ACTOR_SPAWNING_IN_PROGRESS);

    bsal_actor_send_int(self, spawner, BSAL_ACTOR_SPAWN, BSAL_MANAGER_SCRIPT);
}

void bsal_assembly_graph_builder_spawn_reply_graph_store_manager(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    /*
     * Configure the manager for graph stores
     */

    bsal_message_unpack_int(message, 0, &concrete_self->manager_for_graph_stores);

    bsal_actor_add_route_with_source(self, BSAL_MANAGER_SET_SCRIPT_REPLY,
                    bsal_assembly_graph_builder_set_script_reply_store_manager,
                    concrete_self->manager_for_graph_stores);

    bsal_actor_add_route_with_source(self, BSAL_ACTOR_START_REPLY,
                    bsal_assembly_graph_builder_start_reply_store_manager,
                    concrete_self->manager_for_graph_stores);

    bsal_actor_send_int(self, concrete_self->manager_for_graph_stores, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_ASSEMBLY_GRAPH_STORE_SCRIPT);
}


/*
 * find some sort of way to reduce the number of lines here
 * using a new Thorium API call.
 *
 * Solutions:
 *
 * bsal_actor_add_route_with_condition
 * bsal_actor_add_route_with_source_and_condition
 */
void bsal_assembly_graph_builder_spawn_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    printf("DEBUG bsal_assembly_graph_builder_spawn_reply\n");

    /*
     * Configure the manager for block classifiers
     */

    if (concrete_self->manager_for_classifiers == BSAL_ACTOR_NOBODY) {

        bsal_message_unpack_int(message, 0, &concrete_self->manager_for_classifiers);

        bsal_actor_add_route_with_source(self, BSAL_MANAGER_SET_SCRIPT_REPLY,
                        bsal_assembly_graph_builder_set_script_reply_classifier_manager,
                        concrete_self->manager_for_classifiers);

        bsal_actor_add_route_with_source(self, BSAL_ACTOR_START_REPLY,
                        bsal_assembly_graph_builder_start_reply_classifier_manager,
                        concrete_self->manager_for_classifiers);

        bsal_actor_send_int(self, concrete_self->manager_for_classifiers, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_ASSEMBLY_BLOCK_CLASSIFIER_SCRIPT);

    } else if (concrete_self->coverage_distribution == BSAL_ACTOR_NOBODY) {

        bsal_message_unpack_int(message, 0, &concrete_self->coverage_distribution);

        bsal_actor_add_route_with_source(self, BSAL_SET_EXPECTED_MESSAGE_COUNT_REPLY,
                            bsal_assembly_graph_builder_set_expected_message_count_reply,
                        concrete_self->coverage_distribution);

        bsal_actor_add_route_with_source(self, BSAL_ACTOR_NOTIFY,
                        bsal_assembly_graph_builder_notify_from_distribution,
                        concrete_self->coverage_distribution);

        bsal_actor_send_int(self, concrete_self->coverage_distribution,
                        BSAL_SET_EXPECTED_MESSAGE_COUNT, (int)bsal_vector_size(&concrete_self->graph_stores));

    } else {

        printf("Warning: unknown state when receiveing BSAL_ACTOR_SPAWN_REPLY\n");
    }
}

void bsal_assembly_graph_builder_set_expected_message_count_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_actor_add_route_with_sources(self, BSAL_ACTOR_SET_CONSUMER_REPLY,
                    bsal_assembly_graph_builder_set_consumer_reply_graph_stores,
                    &concrete_self->graph_stores);

    /*
     * Link the graph stores to the coverage distribution
     */
    bsal_actor_send_range_int(self, &concrete_self->graph_stores,
                    BSAL_ACTOR_SET_CONSUMER,
                    concrete_self->coverage_distribution);
}

void bsal_assembly_graph_builder_set_script_reply_store_manager(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_actor_send_reply_vector(self, BSAL_ACTOR_START, &concrete_self->spawners);
}

void bsal_assembly_graph_builder_start_reply_store_manager(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    int spawner;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    buffer = bsal_message_buffer(message);

    bsal_vector_unpack(&concrete_self->graph_stores, buffer);

    printf("%s/%d has %d graph stores for assembly\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->graph_stores));

    /* Spawn the manager for the sliding windows manager
     */

    spawner = bsal_actor_get_spawner(self, &concrete_self->spawners);

    /*
     * Register event to happen before
     * sending message (which generates an event)
     */
    concrete_self->manager_for_windows = BSAL_ACTOR_SPAWNING_IN_PROGRESS;

    bsal_actor_add_route_with_condition(self,
                    BSAL_ACTOR_SPAWN_REPLY,
                    bsal_assembly_graph_builder_spawn_reply_window_manager,
                    &concrete_self->manager_for_windows,
                    BSAL_ACTOR_SPAWNING_IN_PROGRESS);

    bsal_actor_send_int(self, spawner, BSAL_ACTOR_SPAWN,
                    BSAL_MANAGER_SCRIPT);

}

void bsal_assembly_graph_builder_spawn_reply_window_manager(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    /*
     * Configure the manager
     * for sliding windows
     */

    bsal_message_unpack_int(message, 0, &concrete_self->manager_for_windows);

    printf("%s/%d says that the manager for windows is %d\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    concrete_self->manager_for_windows);

    bsal_actor_add_route_with_source(self, BSAL_MANAGER_SET_SCRIPT_REPLY,
                    bsal_assembly_graph_builder_set_script_reply_window_manager,
                    concrete_self->manager_for_windows);

    bsal_actor_add_route_with_source(self, BSAL_ACTOR_START_REPLY,
                    bsal_assembly_graph_builder_start_reply_window_manager,
                    concrete_self->manager_for_windows);

    bsal_actor_send_int(self, concrete_self->manager_for_windows, BSAL_MANAGER_SET_SCRIPT,
                    BSAL_ASSEMBLY_SLIDING_WINDOW_SCRIPT);

}

void bsal_assembly_graph_builder_set_producers(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;
    int source;

    source = bsal_message_source(message);


    concrete_self = bsal_actor_concrete_actor(self);
    concrete_self->source = source;

    buffer = bsal_message_buffer(message);

    printf("%s/%d received a %d sequence stores from source %d\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->sequence_stores),
                    concrete_self->source);

    bsal_vector_unpack(&concrete_self->sequence_stores, buffer);

    bsal_actor_send_reply_empty(self, BSAL_ACTOR_SET_PRODUCERS_REPLY);

}

void bsal_assembly_graph_builder_set_script_reply_window_manager(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_actor_send_reply_vector(self, BSAL_ACTOR_START, &concrete_self->spawners);
}

void bsal_assembly_graph_builder_start_reply_window_manager(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;
    int spawner;

    concrete_self = bsal_actor_concrete_actor(self);

    buffer = bsal_message_buffer(message);

    /*
     * Unpack the actor names for the sliding
     * windows
     */
    bsal_vector_unpack(&concrete_self->sliding_windows, buffer);

    bsal_actor_add_route_with_sources(self, BSAL_ACTOR_SET_CONSUMER_REPLY,
                    bsal_assembly_graph_builder_set_consumer_reply_windows,
                    &concrete_self->sliding_windows);

    printf("%s/%d has %d sliding windows for assembly\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->sliding_windows));

    spawner = bsal_actor_get_spawner(self, &concrete_self->spawners);

    bsal_actor_send_int(self, spawner, BSAL_ACTOR_SPAWN,
                    BSAL_MANAGER_SCRIPT);
}

void bsal_assembly_graph_builder_set_script_reply_classifier_manager(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_actor_send_reply_vector(self, BSAL_ACTOR_START, &concrete_self->spawners);
}

void bsal_assembly_graph_builder_start_reply_classifier_manager(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    buffer = bsal_message_buffer(message);

    bsal_vector_unpack(&concrete_self->block_classifiers, buffer);

    printf("%s/%d has %d block classifiers for assembly\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->block_classifiers));

    /*
     * Configure the graph builder now.
     */
    bsal_assembly_graph_builder_configure(self);

}

void bsal_assembly_graph_builder_configure(struct bsal_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;
    int destination;
    int i;

    concrete_self = bsal_actor_concrete_actor(self);

    /*
     * Set the default kmer length
     */
    concrete_self->kmer_length = bsal_assembly_graph_builder_get_kmer_length(self);


    printf("%s/%d configures the kmer length (%d) for the actor computation\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    concrete_self->kmer_length);


    /*
     * set the kmer length for graph stores, sliding windows, and block classifiers
     */

    for (i = 0; i < bsal_vector_size(&concrete_self->graph_stores); i++) {

        destination = bsal_vector_at_as_int(&concrete_self->graph_stores, i);

        bsal_actor_send_int(self, destination, BSAL_SET_KMER_LENGTH,
                        concrete_self->kmer_length);
    }

    for (i = 0; i < bsal_vector_size(&concrete_self->sliding_windows); i++) {

        destination = bsal_vector_at_as_int(&concrete_self->sliding_windows, i);

        bsal_actor_send_int(self, destination, BSAL_SET_KMER_LENGTH,
                        concrete_self->kmer_length);
    }

    for (i = 0; i < bsal_vector_size(&concrete_self->block_classifiers); i++) {

        destination = bsal_vector_at_as_int(&concrete_self->block_classifiers, i);

        bsal_actor_send_int(self, destination, BSAL_SET_KMER_LENGTH,
                        concrete_self->kmer_length);
    }

}

void bsal_assembly_graph_builder_set_kmer_reply(struct bsal_actor *self, struct bsal_message *message)
{
    int expected;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

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
                        bsal_actor_script_name(self),
                        bsal_actor_name(self),
                        expected);

        bsal_assembly_graph_builder_connect_actors(self);
    }
}

void bsal_assembly_graph_builder_connect_actors(struct bsal_actor *self)
{
    int i;
    int producer;
    int consumer;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    /*
     * sequence store ===> sliding window ===> block classifier ===> graph store
     */

    printf("%s/%d connects actors\n",
            bsal_actor_script_name(self),
            bsal_actor_name(self));


    for (i = 0; i < bsal_vector_size(&concrete_self->sliding_windows); i++) {

        producer = bsal_vector_at_as_int(&concrete_self->sliding_windows, i);
        consumer = bsal_vector_at_as_int(&concrete_self->block_classifiers, i);

        /* set the consumer for sliding window
         */
        bsal_actor_send_int(self, producer, BSAL_ACTOR_SET_CONSUMER,
                        consumer);

        printf("DEBUG neural LINK %d -> %d\n",
                        producer, consumer);

        /*
         * set the consumers for each classifier
         */

        printf("DEBUG sending %d store names to classifier %d\n",
                        (int)bsal_vector_size(&concrete_self->graph_stores),
                        consumer);

        bsal_actor_send_vector(self, consumer, BSAL_ACTOR_SET_CONSUMERS,
                        &concrete_self->graph_stores);
    }
}

void bsal_assembly_graph_builder_set_consumer_reply(struct bsal_actor *self, struct bsal_message *message)
{

}

void bsal_assembly_graph_builder_set_consumer_reply_graph_stores(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    ++concrete_self->configured_graph_stores;

    if (concrete_self->configured_graph_stores == bsal_vector_size(&concrete_self->graph_stores)) {

        /*
         * ABCD
         */

        bsal_actor_send_range_empty(self, &concrete_self->graph_stores,
                        BSAL_PUSH_DATA);
    }
}

void bsal_assembly_graph_builder_set_consumer_reply_windows(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    ++concrete_self->configured_sliding_windows;

#ifdef BSAL_ASSEMBLY_GRAPH_BUILDER_DEBUG
    printf("DEBUG windows UP %d\n",
        concrete_self->configured_sliding_windows);
#endif

    bsal_assembly_graph_builder_verify(self);
}

void bsal_assembly_graph_builder_verify(struct bsal_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;
    int i;
    int producer;
    int consumer;

    concrete_self = bsal_actor_concrete_actor(self);

    /* Verify if the system is ready to roll
     */
    if (concrete_self->configured_sliding_windows < bsal_vector_size(&concrete_self->sliding_windows)) {
        return;
    }

    if (concrete_self->configured_block_classifiers < bsal_vector_size(&concrete_self->block_classifiers)) {
        return;
    }

    printf("%s/%d is ready to build the graph\n",
            bsal_actor_script_name(self),
            bsal_actor_name(self));


    /* Set the producer for every sliding window.
     */

#if 0
    bsal_assembly_graph_builder_tell_source(self);

    return;
#endif

    for (i = 0; i < bsal_vector_size(&concrete_self->sliding_windows); i++) {

        producer = bsal_vector_at_as_int(&concrete_self->sequence_stores, i);
        consumer = bsal_vector_at_as_int(&concrete_self->sliding_windows, i);

        printf("CONFIGURE neural LINK %d -> %d\n",
                        producer, consumer);

        bsal_actor_send_int(self, consumer, BSAL_ACTOR_SET_PRODUCER,
                        producer);
    }

}

void bsal_assembly_graph_builder_set_consumers_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    ++concrete_self->configured_block_classifiers;

#ifdef BSAL_ASSEMBLY_GRAPH_BUILDER_DEBUG
    printf("DEBUG classifiers UP %d\n", concrete_self->configured_block_classifiers);
#endif

    bsal_assembly_graph_builder_verify(self);
}

void bsal_assembly_graph_builder_set_producer_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    ++concrete_self->completed_sliding_windows;

    /*
     * Tell the source that the graph is ready now.
     */
    if (concrete_self->completed_sliding_windows == bsal_vector_size(&concrete_self->sliding_windows)) {

        bsal_actor_send_range_empty(self, &concrete_self->sliding_windows, BSAL_ACTOR_NOTIFY);
    }
}

void bsal_assembly_graph_builder_tell_source(struct bsal_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_timer_stop(&concrete_self->timer);
    bsal_timer_print_with_description(&concrete_self->timer, "Build assembly graph");
    bsal_actor_send_empty(self, concrete_self->source, BSAL_ACTOR_START_REPLY);
}

int bsal_assembly_graph_builder_get_kmer_length(struct bsal_actor *self)
{
    int kmer_length;
    int i;
    int argc;
    char **argv;

    argc = bsal_actor_argc(self);
    argv = bsal_actor_argv(self);

    kmer_length = BSAL_ASSEMBLY_GRAPH_BUILDER_DEFAULT_KMER_LENGTH;

    /* get kmer length
     */
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            kmer_length = atoi(argv[i + 1]);
            break;
        }
    }

    /*
     * Use a odd kmer length
     */
    if (kmer_length % 2 == 0) {
        ++kmer_length;
    }

    return kmer_length;
}

void bsal_assembly_graph_builder_notify_reply(struct bsal_actor *self, struct bsal_message *message)
{
    uint64_t produced_kmer_count;
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_message_unpack_uint64_t(message, 0, &produced_kmer_count);

    concrete_self->total_kmer_count += produced_kmer_count;

    ++concrete_self->notified_windows;

    if (concrete_self->notified_windows == bsal_vector_size(&concrete_self->sliding_windows)) {

        printf("%s/%d : %" PRIu64 " kmers were generated during the actor computation.\n",
            bsal_actor_script_name(self),
            bsal_actor_name(self),
            concrete_self->total_kmer_count);

        bsal_actor_send_to_self_empty(self, BSAL_ASSEMBLY_GRAPH_BUILDER_CONTROL_COMPLEXITY);
    }
}

void bsal_assembly_graph_builder_control_complexity(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    concrete_self->actual_kmer_count = 0;
    concrete_self->synchronized_graph_stores = 0;

    bsal_actor_send_range_empty(self, &concrete_self->graph_stores,
                    BSAL_STORE_GET_ENTRY_COUNT);
}

void bsal_assembly_graph_builder_get_entry_count_reply(struct bsal_actor *self, struct bsal_message *message)
{
    uint64_t kmer_count;
    struct bsal_assembly_graph_builder *concrete_self;
    int spawner;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_message_unpack_uint64_t(message, 0, &kmer_count);

    concrete_self->actual_kmer_count += kmer_count;

    ++concrete_self->synchronized_graph_stores;

    if (concrete_self->synchronized_graph_stores == bsal_vector_size(&concrete_self->graph_stores)) {

        if (concrete_self->actual_kmer_count == concrete_self->total_kmer_count) {

            printf("%s/%d synchronized with %d graph stores\n",
                            bsal_actor_script_name(self),
                            bsal_actor_name(self),
                            (int)bsal_vector_size(&concrete_self->graph_stores));

            /* Generate coverage distribution
             */

            spawner = bsal_actor_get_spawner(self, &concrete_self->spawners);

            bsal_actor_send_int(self, spawner, BSAL_ACTOR_SPAWN,
                            BSAL_COVERAGE_DISTRIBUTION_SCRIPT);

        } else {
            /*
             * Otherwise, there are still some messages in transit.
             */

            bsal_actor_send_to_self_empty(self, BSAL_ASSEMBLY_GRAPH_BUILDER_CONTROL_COMPLEXITY);
        }
    }
}

void bsal_assembly_graph_builder_notify_from_distribution(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_assembly_graph_builder_tell_source(self);
}
