
#include "unitig_visitor.h"

#include <genomics/assembly/assembly_graph_store.h>
#include <genomics/assembly/assembly_arc.h>

#include <core/structures/vector.h>
#include <core/system/debugger.h>

#include <stdio.h>

#define STEP_GET_KMER_LENGTH 0
#define STEP_GET_MAIN_KMER 1
#define STEP_GET_MAIN_VERTEX_DATA 2
#define STEP_DECIDE_MAIN 3
#define STEP_DO_RESET 4
#define STEP_ABORT 5
#define STEP_GET_PARENT_VERTEX_DATA 6
#define STEP_DECIDE_PARENT 7
#define STEP_GET_CHILD_VERTEX_DATA 8
#define STEP_DECIDE_CHILD 9

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

    bsal_vertex_neighborhood_init_empty(&concrete_self->main_neighborhood);
    bsal_vertex_neighborhood_init_empty(&concrete_self->parent_neighborhood);
    bsal_vertex_neighborhood_init_empty(&concrete_self->child_neighborhood);

    bsal_unitig_heuristic_init(&concrete_self->heuristic);

    bsal_dna_kmer_init_empty(&concrete_self->main_kmer);
    bsal_dna_kmer_init_empty(&concrete_self->parent_kmer);
    bsal_dna_kmer_init_empty(&concrete_self->child_kmer);
}

void bsal_unitig_visitor_destroy(struct thorium_actor *self)
{
    struct bsal_unitig_visitor *concrete_self;
    concrete_self = (struct bsal_unitig_visitor *)thorium_actor_concrete_actor(self);
    concrete_self->manager = -1;
    concrete_self->completed = 0;

    bsal_vector_destroy(&concrete_self->graph_stores);

    bsal_vertex_neighborhood_destroy(&concrete_self->main_neighborhood);
    bsal_vertex_neighborhood_destroy(&concrete_self->parent_neighborhood);
    bsal_vertex_neighborhood_destroy(&concrete_self->child_neighborhood);

    bsal_unitig_heuristic_destroy(&concrete_self->heuristic);
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
            concrete_self->step = STEP_DECIDE_MAIN;
            bsal_unitig_visitor_execute(self);
        }
        return;
    } else if (concrete_self->step == STEP_GET_PARENT_VERTEX_DATA) {

        if (bsal_vertex_neighborhood_receive(&concrete_self->parent_neighborhood, message)) {

            concrete_self->step = STEP_DECIDE_PARENT;
            bsal_unitig_visitor_execute(self);
        }
        return;
    } else if (concrete_self->step == STEP_GET_CHILD_VERTEX_DATA) {

        if (bsal_vertex_neighborhood_receive(&concrete_self->child_neighborhood, message)) {
            concrete_self->step = STEP_DECIDE_CHILD;
            bsal_unitig_visitor_execute(self);
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
        bsal_unitig_visitor_execute(self);

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
            bsal_unitig_visitor_execute(self);

        } else {
            bsal_dna_kmer_unpack(&concrete_self->main_kmer, buffer, concrete_self->kmer_length,
                    &concrete_self->memory_pool, &concrete_self->codec);

            concrete_self->step = STEP_GET_MAIN_VERTEX_DATA;
            bsal_unitig_visitor_execute(self);
        }

    } else if (tag == ACTION_ASSEMBLY_GET_KMER_LENGTH_REPLY) {
        thorium_message_unpack_int(message, 0, &concrete_self->kmer_length);

        concrete_self->step = STEP_GET_MAIN_KMER;
        bsal_unitig_visitor_execute(self);

    } else if (tag == ACTION_ASSEMBLY_GET_VERTEX_REPLY) {

        if (concrete_self->step == STEP_GET_MAIN_VERTEX_DATA) {

            concrete_self->step = STEP_DECIDE_MAIN;
            bsal_unitig_visitor_execute(self);
        }

    } else if (tag == ACTION_YIELD_REPLY) {
        bsal_unitig_visitor_execute(self);
    }
}

void bsal_unitig_visitor_execute(struct thorium_actor *self)
{
    struct bsal_unitig_visitor *concrete_self;
    int graph_store_index;
    int graph_store;
    int size;
    struct bsal_vector coverages;
    int coverage;
    struct bsal_assembly_vertex *other_vertex;
    struct bsal_assembly_vertex *vertex;
    int parents;
    int children;
    int i;
    int other_coverage;
    struct bsal_memory_pool *ephemeral_memory;
    int code;
    int expected_code;

    concrete_self = thorium_actor_concrete_actor(self);
    size = bsal_vector_size(&concrete_self->graph_stores);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

#if 0
    printf("bsal_unitig_visitor_execute step= %d\n",
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

    } else if (concrete_self->step == STEP_DECIDE_MAIN) {

        vertex = bsal_vertex_neighborhood_vertex(&concrete_self->main_neighborhood);
        coverage = bsal_assembly_vertex_coverage_depth(vertex);

        parents = bsal_assembly_vertex_parent_count(vertex);
        children = bsal_assembly_vertex_child_count(vertex);

        /*
         * Select a parent.
         */
        bsal_vector_init(&coverages, sizeof(int));
        bsal_vector_set_memory_pool(&coverages, ephemeral_memory);

        for (i = 0; i < parents; ++i) {

            vertex = bsal_vertex_neighborhood_parent(&concrete_self->main_neighborhood, i);
            other_coverage = bsal_assembly_vertex_coverage_depth(vertex);
            bsal_vector_push_back(&coverages, &other_coverage);
        }

        concrete_self->selected_parent = bsal_unitig_heuristic_select(&concrete_self->heuristic,
                        coverage, &coverages);

        bsal_vector_clear(&coverages);

        /*
         * Select a child
         */

        for (i = 0; i < children; ++i) {

            vertex = bsal_vertex_neighborhood_child(&concrete_self->main_neighborhood, i);
            other_coverage = bsal_assembly_vertex_coverage_depth(vertex);
            bsal_vector_push_back(&coverages, &other_coverage);
        }

        concrete_self->selected_child = bsal_unitig_heuristic_select(&concrete_self->heuristic,
                        coverage, &coverages);

        bsal_vector_destroy(&coverages);

#if 0
        printf("DEBUG selected_parent %d selected_child %d\n",
                        concrete_self->selected_parent, concrete_self->selected_child);
#endif

        if (concrete_self->selected_parent >= 0 && concrete_self->selected_child >= 0) {
            concrete_self->step = STEP_GET_PARENT_VERTEX_DATA;

        } else {
            concrete_self->step = STEP_DO_RESET;
        }
        bsal_unitig_visitor_execute(self);

    } else if (concrete_self->step == STEP_DO_RESET) {

        bsal_dna_kmer_destroy(&concrete_self->main_kmer, &concrete_self->memory_pool);
        bsal_dna_kmer_destroy(&concrete_self->parent_kmer, &concrete_self->memory_pool);
        bsal_dna_kmer_destroy(&concrete_self->child_kmer, &concrete_self->memory_pool);

        bsal_vertex_neighborhood_destroy(&concrete_self->main_neighborhood);
        bsal_vertex_neighborhood_init_empty(&concrete_self->main_neighborhood);

        bsal_vertex_neighborhood_destroy(&concrete_self->parent_neighborhood);
        bsal_vertex_neighborhood_init_empty(&concrete_self->parent_neighborhood);

        bsal_vertex_neighborhood_destroy(&concrete_self->child_neighborhood);
        bsal_vertex_neighborhood_init_empty(&concrete_self->child_neighborhood);


        concrete_self->step = STEP_GET_MAIN_KMER;

        if (concrete_self->visited % 5000 == 0) {
            printf("%s/%d visited %d vertices so far\n",
                            thorium_actor_script_name(self), thorium_actor_name(self),
                            concrete_self->visited);
        }

        ++concrete_self->visited;
        bsal_unitig_visitor_execute(self);

    } else if (concrete_self->step == STEP_GET_PARENT_VERTEX_DATA) {

        BSAL_DEBUGGER_ASSERT(concrete_self->selected_parent >= 0);
        vertex = bsal_vertex_neighborhood_vertex(&concrete_self->main_neighborhood);
        code = bsal_assembly_vertex_get_parent(vertex,
                            concrete_self->selected_parent);

        bsal_dna_kmer_init_as_parent(&concrete_self->parent_kmer, &concrete_self->main_kmer,
                            code, concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec);

        bsal_vertex_neighborhood_init(&concrete_self->parent_neighborhood,
                            &concrete_self->parent_kmer, BSAL_ARC_TYPE_CHILD, &concrete_self->graph_stores,
                            concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec, self);

        /*
         * Start first one.
         */
        bsal_vertex_neighborhood_receive(&concrete_self->parent_neighborhood, NULL);

    } else if (concrete_self->step == STEP_DECIDE_PARENT) {

#if 0
        printf("DEBUG STEP_DECIDE_PARENT\n");
#endif
        other_vertex = bsal_vertex_neighborhood_vertex(&concrete_self->parent_neighborhood);
        coverage = bsal_assembly_vertex_coverage_depth(other_vertex);
        children = bsal_assembly_vertex_child_count(other_vertex);

        bsal_vector_init(&coverages, sizeof(int));
        bsal_vector_set_memory_pool(&coverages, ephemeral_memory);

        for (i = 0; i < children; ++i) {

            vertex = bsal_vertex_neighborhood_child(&concrete_self->parent_neighborhood, i);
            other_coverage = bsal_assembly_vertex_coverage_depth(vertex);
            bsal_vector_push_back(&coverages, &other_coverage);
        }

        concrete_self->selected_parent_child = bsal_unitig_heuristic_select(&concrete_self->heuristic,
                        coverage, &coverages);
        bsal_vector_destroy(&coverages);

        if (concrete_self->selected_parent_child >= 0) {
            code = bsal_assembly_vertex_get_child(other_vertex, concrete_self->selected_parent_child);
            expected_code = bsal_dna_kmer_last_symbol(&concrete_self->main_kmer, concrete_self->kmer_length,
                            &concrete_self->codec);

            if (code == expected_code) {
                concrete_self->step = STEP_GET_CHILD_VERTEX_DATA;
            } else {
                printf("Info: parent code mismatch !\n");
                concrete_self->step = STEP_DO_RESET;
            }
        } else {
            concrete_self->step = STEP_DO_RESET;
        }

        /* recursive call.
         *
         * Otherwise, we could use ACTION_YIELD to do an actor-like recursion.
         */
        bsal_unitig_visitor_execute(self);

    } else if (concrete_self->step == STEP_GET_CHILD_VERTEX_DATA) {

        BSAL_DEBUGGER_ASSERT(concrete_self->selected_child >= 0);

        vertex = bsal_vertex_neighborhood_vertex(&concrete_self->main_neighborhood);
        code = bsal_assembly_vertex_get_child(vertex,
                            concrete_self->selected_child);

        bsal_dna_kmer_init_as_child(&concrete_self->child_kmer, &concrete_self->main_kmer,
                            code, concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec);

        bsal_vertex_neighborhood_init(&concrete_self->child_neighborhood,
                            &concrete_self->child_kmer, BSAL_ARC_TYPE_PARENT, &concrete_self->graph_stores,
                            concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec, self);

        /*
         * Start first one.
         */
        bsal_vertex_neighborhood_receive(&concrete_self->child_neighborhood, NULL);

    } else if (concrete_self->step == STEP_DECIDE_CHILD) {

        concrete_self->step = STEP_DO_RESET;

        bsal_unitig_visitor_execute(self);
    }
}


