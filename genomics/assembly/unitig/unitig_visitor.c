
#include "unitig_visitor.h"

#include <genomics/assembly/assembly_graph_store.h>
#include <genomics/assembly/assembly_arc.h>

#include <core/structures/vector.h>

#include <stdio.h>

#define STEP_GET_KMER_LENGTH 0

#define STEP_GET_MAIN_KMER 1
#define STEP_GET_MAIN_VERTEX_DATA 2

#define STEP_DECIDE 1000
#define STEP_ABORT 9999

struct thorium_script bsal_unitig_visitor_script = {
    .identifier = SCRIPT_UNITIG_VISITOR,
    .name = "bsal_unitig_visitor",
    .init = bsal_unitig_visitor_init,
    .destroy = bsal_unitig_visitor_destroy,
    .receive = bsal_unitig_visitor_receive,
    .size = sizeof(struct bsal_unitig_visitor),
    .description = "A universe visitor for vertices."
};

void bsal_unitig_visitor_init(struct thorium_actor *self)
{
    struct bsal_unitig_visitor *concrete_self;

    concrete_self = (struct bsal_unitig_visitor *)thorium_actor_concrete_actor(self);
    bsal_memory_pool_init(&concrete_self->memory_pool, 131072);

    bsal_vector_init(&concrete_self->graph_stores, sizeof(int));
    bsal_vector_set_memory_pool(&concrete_self->graph_stores, &concrete_self->memory_pool);
    concrete_self->manager = -1;
    concrete_self->completed = 0;
    concrete_self->graph_store_index = -1;
    concrete_self->kmer_length = -1;
    concrete_self->step = STEP_GET_KMER_LENGTH;

    bsal_dna_codec_init(&concrete_self->codec);

    if (bsal_dna_codec_must_use_two_bit_encoding(&concrete_self->codec,
                            thorium_actor_get_node_count(self))) {
        bsal_dna_codec_enable_two_bit_encoding(&concrete_self->codec);
    }

    concrete_self->visited = 0;

    /*
    bsal_vertex_neighborhood_init_empty(&concrete_self->main_neighborhood);
    bsal_vertex_neighborhood_init_empty(&concrete_self->left_neighborhood);
    bsal_vertex_neighborhood_init_empty(&concrete_self->right_neighborhood);
    */
}

void bsal_unitig_visitor_destroy(struct thorium_actor *self)
{
    struct bsal_unitig_visitor *concrete_self;
    concrete_self = (struct bsal_unitig_visitor *)thorium_actor_concrete_actor(self);
    concrete_self->manager = -1;
    concrete_self->completed = 0;

    bsal_vector_destroy(&concrete_self->graph_stores);

    /*
    bsal_vertex_neighborhood_destroy(&concrete_self->main_neighborhood);
    bsal_vertex_neighborhood_destroy(&concrete_self->left_neighborhood);
    bsal_vertex_neighborhood_destroy(&concrete_self->right_neighborhood);
    */

    bsal_memory_pool_destroy(&concrete_self->memory_pool);
}

void bsal_unitig_visitor_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    void *buffer;
    int source;
    struct bsal_unitig_visitor *concrete_self;
    int size;
    int count;

    tag = thorium_message_tag(message);
    count = thorium_message_count(message);
    buffer = thorium_message_buffer(message);
    concrete_self = thorium_actor_concrete_actor(self);
    source = thorium_message_source(message);

    if (concrete_self->step == STEP_GET_MAIN_VERTEX_DATA) {
        if (bsal_vertex_neighborhood_receive(&concrete_self->main_neighborhood, message)) {

#if 0
            printf("VISITOR fetched main vertex data.\n");
#endif
            concrete_self->step = STEP_DECIDE;
            bsal_unitig_visitor_run(self);
        }
        return;
    }

    if (tag == ACTION_START) {

        concrete_self->manager = source;

        printf("%s/%d is ready to visit places in the universe\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self));

        bsal_vector_unpack(&concrete_self->graph_stores, buffer);
        size = bsal_vector_size(&concrete_self->graph_stores);
        concrete_self->graph_store_index = rand() % size;

        concrete_self->step = STEP_GET_KMER_LENGTH;
        bsal_unitig_visitor_run(self);

    } else if (tag == ACTION_ASK_TO_STOP) {
        thorium_actor_send_to_self_empty(self, ACTION_STOP);

        thorium_actor_send_reply_empty(self, ACTION_ASK_TO_STOP_REPLY);

    } else if (tag == ACTION_ASSEMBLY_GET_STARTING_KMER_REPLY) {

        if (count == 0) {
            ++concrete_self->completed;

            /*
             * Restart somewhere else.
             */
            concrete_self->step = STEP_GET_MAIN_KMER;
            bsal_unitig_visitor_run(self);

        } else {
            bsal_dna_kmer_init_empty(&concrete_self->main_kmer);
            bsal_dna_kmer_unpack(&concrete_self->main_kmer, buffer, concrete_self->kmer_length,
                    &concrete_self->memory_pool, &concrete_self->codec);

            concrete_self->step = STEP_GET_MAIN_VERTEX_DATA;
            bsal_unitig_visitor_run(self);
        }

    } else if (tag == ACTION_ASSEMBLY_GET_KMER_LENGTH_REPLY) {
        thorium_message_unpack_int(message, 0, &concrete_self->kmer_length);

        concrete_self->step = STEP_GET_MAIN_KMER;
        bsal_unitig_visitor_run(self);

    } else if (tag == ACTION_ASSEMBLY_GET_VERTEX_REPLY) {

        if (concrete_self->step == STEP_GET_MAIN_VERTEX_DATA) {

            concrete_self->step = STEP_DECIDE;
            bsal_unitig_visitor_run(self);
        }

    } else if (tag == ACTION_YIELD_REPLY) {
        bsal_unitig_visitor_run(self);
    }
}

void bsal_unitig_visitor_run(struct thorium_actor *self)
{
    struct bsal_unitig_visitor *concrete_self;
    int graph_store_index;
    int graph_store;
    int size;

    concrete_self = thorium_actor_concrete_actor(self);
    size = bsal_vector_size(&concrete_self->graph_stores);

#if 0
    printf("bsal_unitig_visitor_run step= %d\n",
                    concrete_self->step);
#endif

    if (concrete_self->completed == bsal_vector_size(&concrete_self->graph_stores)) {
        thorium_actor_send_empty(self, concrete_self->manager, ACTION_START_REPLY);

#if 0
        printf("visitor Finished\n");
#endif
    } else if (concrete_self->step == STEP_GET_KMER_LENGTH) {
        graph_store_index = concrete_self->graph_store_index;
        graph_store = bsal_vector_at_as_int(&concrete_self->graph_stores, graph_store_index);

        thorium_actor_send_empty(self, graph_store, ACTION_ASSEMBLY_GET_KMER_LENGTH);

#if 0
        printf("visitor STEP_GET_KMER_LENGTH\n");
#endif

    } else if (concrete_self->step == STEP_GET_MAIN_KMER) {

        /*
         * Get a starting kmer from one of the graph stores.
         */
        graph_store_index = concrete_self->graph_store_index;
        ++concrete_self->graph_store_index;
        concrete_self->graph_store_index %= size;
        graph_store = bsal_vector_at_as_int(&concrete_self->graph_stores, graph_store_index);

        thorium_actor_send_empty(self, graph_store, ACTION_ASSEMBLY_GET_STARTING_KMER);

#if 0
        printf("visitor STEP_GET_MAIN_KMER\n");
#endif

    } else if (concrete_self->step == STEP_GET_MAIN_VERTEX_DATA) {

        /*
         * Fetch everything related to the main kmer.
         */
        bsal_vertex_neighborhood_init(&concrete_self->main_neighborhood,
                            &concrete_self->main_kmer, BSAL_ARC_TYPE_ANY, &concrete_self->graph_stores,
                            concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec, self);

        /*
         * Start first one.
         */
        bsal_vertex_neighborhood_receive(&concrete_self->main_neighborhood, NULL);

#if 0
        printf("visitor STEP_GET_MAIN_VERTEX_DATA\n");
#endif

    } else if (concrete_self->step == STEP_DECIDE) {

        bsal_dna_kmer_destroy(&concrete_self->main_kmer, &concrete_self->memory_pool);
        bsal_vertex_neighborhood_destroy(&concrete_self->main_neighborhood);

        concrete_self->step = STEP_GET_MAIN_KMER;

        if (concrete_self->visited % 5000 == 0) {
            printf("%s/%d visited %d vertices so far\n",
                            thorium_actor_script_name(self), thorium_actor_name(self),
                            concrete_self->visited);
        }
        ++concrete_self->visited;

        thorium_actor_send_to_self_empty(self, ACTION_YIELD);
    }
}


