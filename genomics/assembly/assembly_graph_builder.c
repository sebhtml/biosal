
#include "assembly_graph_builder.h"

#include "assembly_graph_store.h"
#include "assembly_sliding_window.h"
#include "assembly_block_classifier.h"

#include <core/helpers/actor_helper.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_script bsal_assembly_graph_builder_script = {
    .identifier = BSAL_ASSEMBLY_GRAPH_BUILDER_SCRIPT,
    .name = "assembly_graph_builder",
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

    bsal_actor_register_handler(self, BSAL_ACTOR_START, bsal_assembly_graph_builder_start);
    bsal_actor_register_handler(self, BSAL_ACTOR_SET_PRODUCERS, bsal_assembly_graph_builder_set_producers);
    bsal_actor_register_handler(self, BSAL_ACTOR_SET_PRODUCER_REPLY, bsal_assembly_graph_builder_set_producer_reply);
    bsal_actor_register_handler(self, BSAL_ACTOR_ASK_TO_STOP, bsal_assembly_graph_builder_ask_to_stop);
    bsal_actor_register_handler(self, BSAL_ACTOR_SPAWN_REPLY, bsal_assembly_graph_builder_spawn_reply);
    bsal_actor_register_handler(self, BSAL_SET_KMER_LENGTH_REPLY, bsal_assembly_graph_builder_set_kmer_reply);

    bsal_actor_register_handler(self, BSAL_ACTOR_SET_CONSUMER_REPLY, bsal_assembly_graph_builder_set_consumer_reply);
    bsal_actor_register_handler(self, BSAL_ACTOR_SET_CONSUMERS_REPLY, bsal_assembly_graph_builder_set_consumers_reply);

    concrete_self->manager_for_graph_stores = BSAL_ACTOR_NOBODY;
    bsal_vector_init(&concrete_self->graph_stores, sizeof(int));

    concrete_self->manager_for_classifiers = BSAL_ACTOR_NOBODY;

    concrete_self->manager_for_windows = BSAL_ACTOR_NOBODY;
    bsal_vector_init(&concrete_self->sliding_windows, sizeof(int));

    concrete_self->configured_graph_stores = 0;
    concrete_self->configured_sliding_windows = 0;
    concrete_self->configured_block_classifiers = 0;

    concrete_self->actors_with_kmer_length = 0;
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

    bsal_actor_helper_ask_to_stop(self, message);
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

    bsal_actor_helper_send_int(self, spawner, BSAL_ACTOR_SPAWN, BSAL_MANAGER_SCRIPT);
}

void bsal_assembly_graph_builder_spawn_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    /*
     * Configure the manager for graph stores
     */
    if (concrete_self->manager_for_graph_stores == BSAL_ACTOR_NOBODY) {
        bsal_message_helper_unpack_int(message, 0, &concrete_self->manager_for_graph_stores);

        bsal_actor_register_handler_with_source(self, BSAL_MANAGER_SET_SCRIPT_REPLY,
                        concrete_self->manager_for_graph_stores,
                        bsal_assembly_graph_builder_set_script_reply_store_manager);

        bsal_actor_register_handler_with_source(self, BSAL_ACTOR_START_REPLY,
                        concrete_self->manager_for_graph_stores,
                        bsal_assembly_graph_builder_start_reply_store_manager);

        bsal_actor_helper_send_int(self, concrete_self->manager_for_graph_stores, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_ASSEMBLY_GRAPH_STORE_SCRIPT);

    /*
     * Configure the manager
     * for sliding windows
     */
    } else if (concrete_self->manager_for_windows == BSAL_ACTOR_NOBODY) {

        bsal_message_helper_unpack_int(message, 0, &concrete_self->manager_for_windows);

        bsal_actor_register_handler_with_source(self, BSAL_MANAGER_SET_SCRIPT_REPLY,
                        concrete_self->manager_for_windows,
                        bsal_assembly_graph_builder_set_script_reply_window_manager);

        bsal_actor_register_handler_with_source(self, BSAL_ACTOR_START_REPLY,
                        concrete_self->manager_for_windows,
                        bsal_assembly_graph_builder_start_reply_window_manager);

        bsal_actor_helper_send_int(self, concrete_self->manager_for_windows, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_ASSEMBLY_SLIDING_WINDOW_SCRIPT);

    /*
     * Configure the manager for block classifiers
     */
    } else if (concrete_self->manager_for_classifiers == BSAL_ACTOR_NOBODY) {

        bsal_message_helper_unpack_int(message, 0, &concrete_self->manager_for_classifiers);

        bsal_actor_register_handler_with_source(self, BSAL_MANAGER_SET_SCRIPT_REPLY,
                        concrete_self->manager_for_classifiers,
                        bsal_assembly_graph_builder_set_script_reply_classifier_manager);

        bsal_actor_register_handler_with_source(self, BSAL_ACTOR_START_REPLY,
                        concrete_self->manager_for_classifiers,
                        bsal_assembly_graph_builder_start_reply_classifier_manager);

        bsal_actor_helper_send_int(self, concrete_self->manager_for_classifiers, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_ASSEMBLY_BLOCK_CLASSIFIER_SCRIPT);
    }
}

void bsal_assembly_graph_builder_set_script_reply_store_manager(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_actor_helper_send_reply_vector(self, BSAL_ACTOR_START, &concrete_self->spawners);
}

void bsal_assembly_graph_builder_start_reply_store_manager(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_graph_builder *concrete_self;
    int spawner;

    concrete_self = bsal_actor_concrete_actor(self);

    buffer = bsal_message_buffer(message);

    bsal_vector_unpack(&concrete_self->graph_stores, buffer);

    printf("%s/%d has %d graph stores for assembly\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->graph_stores));

    spawner = bsal_actor_get_spawner(self, &concrete_self->spawners);

    bsal_actor_helper_send_int(self, spawner, BSAL_ACTOR_SPAWN,
                    BSAL_MANAGER_SCRIPT);

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

    bsal_actor_helper_send_reply_empty(self, BSAL_ACTOR_SET_PRODUCERS_REPLY);

}

void bsal_assembly_graph_builder_set_script_reply_window_manager(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_actor_helper_send_reply_vector(self, BSAL_ACTOR_START, &concrete_self->spawners);
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

    printf("%s/%d has %d sliding windows for assembly\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->sliding_windows));

    spawner = bsal_actor_get_spawner(self, &concrete_self->spawners);

    bsal_actor_helper_send_int(self, spawner, BSAL_ACTOR_SPAWN,
                    BSAL_MANAGER_SCRIPT);
}

void bsal_assembly_graph_builder_set_script_reply_classifier_manager(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_actor_helper_send_reply_vector(self, BSAL_ACTOR_START, &concrete_self->spawners);
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
    int argc;
    int i;
    char **argv;
    int destination;

    concrete_self = bsal_actor_concrete_actor(self);
    argc = bsal_actor_argc(self);
    argv = bsal_actor_argv(self);

    /* get kmer length
     */
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0) {
            concrete_self->kmer_length = atoi(argv[i]);
            break;
        }
    }

    /*
     * set the kmer length for graph stores, sliding windows, and block classifiers
     */

    for (i = 0; i < bsal_vector_size(&concrete_self->graph_stores); i++) {

        destination = bsal_vector_helper_at_as_int(&concrete_self->graph_stores, i);

        bsal_actor_helper_send_int(self, destination, BSAL_SET_KMER_LENGTH,
                        concrete_self->kmer_length);
    }

    for (i = 0; i < bsal_vector_size(&concrete_self->sliding_windows); i++) {

        destination = bsal_vector_helper_at_as_int(&concrete_self->sliding_windows, i);

        bsal_actor_helper_send_int(self, destination, BSAL_SET_KMER_LENGTH,
                        concrete_self->kmer_length);
    }

    for (i = 0; i < bsal_vector_size(&concrete_self->block_classifiers); i++) {

        destination = bsal_vector_helper_at_as_int(&concrete_self->block_classifiers, i);

        bsal_actor_helper_send_int(self, destination, BSAL_SET_KMER_LENGTH,
                        concrete_self->kmer_length);
    }

    printf("%s/%d configures the kmer length (%d) for the actor computation\n",
                    bsal_actor_script_name(self),
                    concrete_self->kmer_length,
                    bsal_actor_name(self));

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

        printf("%s/%d configured the kmer length value for sliding windows, block classifiers and graph stores\n",
                        bsal_actor_script_name(self),
                        bsal_actor_name(self));

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

        producer = bsal_vector_helper_at_as_int(&concrete_self->sliding_windows, i);
        consumer = bsal_vector_helper_at_as_int(&concrete_self->block_classifiers, i);

        /* set the consumer for sliding window
         */
        bsal_actor_helper_send_int(self, producer, BSAL_ACTOR_SET_CONSUMER,
                        consumer);

        printf("DEBUG neural LINK %d -> %d\n",
                        producer, consumer);

        /*
         * set the consumers for each classifier
         */

        printf("DEBUG sending %d store names to classifier %d\n",
                        (int)bsal_vector_size(&concrete_self->graph_stores),
                        consumer);

        bsal_actor_helper_send_vector(self, consumer, BSAL_ACTOR_SET_CONSUMERS,
                        &concrete_self->graph_stores);
    }
}

void bsal_assembly_graph_builder_set_consumer_reply(struct bsal_actor *self, struct bsal_message *message)
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

    bsal_assembly_graph_builder_tell_source(self);

    return;

    for (i = 0; i < bsal_vector_size(&concrete_self->sliding_windows); i++) {

        producer = bsal_vector_helper_at_as_int(&concrete_self->sequence_stores, i);
        consumer = bsal_vector_helper_at_as_int(&concrete_self->sliding_windows, i);

        bsal_actor_helper_send_int(self, consumer, BSAL_ACTOR_SET_PRODUCER,
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
        bsal_assembly_graph_builder_tell_source(self);
    }
}

void bsal_assembly_graph_builder_tell_source(struct bsal_actor *self)
{
    struct bsal_assembly_graph_builder *concrete_self;

    concrete_self = bsal_actor_concrete_actor(self);

    bsal_timer_stop(&concrete_self->timer);
    bsal_timer_print_with_description(&concrete_self->timer, "Build assembly graph");
    bsal_actor_helper_send_empty(self, concrete_self->source, BSAL_ACTOR_START_REPLY);
}
