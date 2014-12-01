
#include "unitig_visitor.h"

#include <genomics/assembly/assembly_graph_store.h>
#include <genomics/assembly/assembly_arc.h>

#include <genomics/helpers/command.h>

#include <core/structures/vector.h>
#include <core/system/debugger.h>

#include <core/constants.h>

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
#define STEP_MARK_UNITIG 10

#define MEMORY_POOL_NAME_VISITOR 0x185945f7

void biosal_unitig_visitor_init(struct thorium_actor *self);
void biosal_unitig_visitor_destroy(struct thorium_actor *self);
void biosal_unitig_visitor_receive(struct thorium_actor *self, struct thorium_message *message);

void biosal_unitig_visitor_execute(struct thorium_actor *self);

void biosal_unitig_visitor_mark_vertex(struct thorium_actor *self, struct biosal_dna_kmer *kmer);

struct thorium_script biosal_unitig_visitor_script = {
    .identifier = SCRIPT_UNITIG_VISITOR,
    .name = "biosal_unitig_visitor",
    .init = biosal_unitig_visitor_init,
    .destroy = biosal_unitig_visitor_destroy,
    .receive = biosal_unitig_visitor_receive,
    .size = sizeof(struct biosal_unitig_visitor),
    .description = "A universe visitor for vertices."
};

void biosal_unitig_visitor_init(struct thorium_actor *self)
{
    struct biosal_unitig_visitor *concrete_self;

    concrete_self = (struct biosal_unitig_visitor *)thorium_actor_concrete_actor(self);
    core_memory_pool_init(&concrete_self->memory_pool, 131072,
                    MEMORY_POOL_NAME_VISITOR);

    core_vector_init(&concrete_self->graph_stores, sizeof(int));
    core_vector_set_memory_pool(&concrete_self->graph_stores, &concrete_self->memory_pool);
    concrete_self->manager = -1;
    concrete_self->completed = 0;
    concrete_self->graph_store_index = -1;
    concrete_self->kmer_length = -1;
    concrete_self->step = STEP_GET_KMER_LENGTH;

    biosal_dna_codec_init(&concrete_self->codec);

    if (biosal_dna_codec_must_use_two_bit_encoding(&concrete_self->codec,
                            thorium_actor_get_node_count(self))) {
        biosal_dna_codec_enable_two_bit_encoding(&concrete_self->codec);
    }

    concrete_self->visited_vertices = 0;

    concrete_self->last_second_count = thorium_actor_get_time_in_seconds(self);
    concrete_self->last_visited_count = concrete_self->visited_vertices;

    concrete_self->vertices_with_unitig_flag = 0;

    biosal_vertex_neighborhood_init_empty(&concrete_self->main_neighborhood);
    biosal_vertex_neighborhood_init_empty(&concrete_self->parent_neighborhood);
    biosal_vertex_neighborhood_init_empty(&concrete_self->child_neighborhood);

    biosal_unitig_heuristic_init(&concrete_self->heuristic,
                    biosal_command_get_minimum_coverage(thorium_actor_argc(self),
                            thorium_actor_argv(self)));

    biosal_dna_kmer_init_empty(&concrete_self->main_kmer);
    biosal_dna_kmer_init_empty(&concrete_self->parent_kmer);
    biosal_dna_kmer_init_empty(&concrete_self->child_kmer);

    concrete_self->verbose = FALSE;
}

void biosal_unitig_visitor_destroy(struct thorium_actor *self)
{
    struct biosal_unitig_visitor *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    if (concrete_self->verbose) {
        printf("%s/%d vertex_count: %d BIOSAL_VERTEX_FLAG_UNITIG: %d\n",
                    thorium_actor_script_name(self), thorium_actor_name(self),
                    concrete_self->visited_vertices, concrete_self->vertices_with_unitig_flag);
    }

    biosal_dna_codec_destroy(&concrete_self->codec);

    concrete_self->manager = -1;
    concrete_self->completed = 0;

    core_vector_destroy(&concrete_self->graph_stores);

    biosal_vertex_neighborhood_destroy(&concrete_self->main_neighborhood);
    biosal_vertex_neighborhood_destroy(&concrete_self->parent_neighborhood);
    biosal_vertex_neighborhood_destroy(&concrete_self->child_neighborhood);

    biosal_unitig_heuristic_destroy(&concrete_self->heuristic);
    core_memory_pool_destroy(&concrete_self->memory_pool);
}

void biosal_unitig_visitor_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    void *buffer;
    int source;
    struct biosal_unitig_visitor *concrete_self;
    int size;
    int count;
    int action;

    tag = thorium_message_action(message);
    count = thorium_message_count(message);
    buffer = thorium_message_buffer(message);
    concrete_self = thorium_actor_concrete_actor(self);
    source = thorium_message_source(message);
    action = tag;

    if (concrete_self->step == STEP_GET_MAIN_VERTEX_DATA) {
        if (biosal_vertex_neighborhood_receive(&concrete_self->main_neighborhood, message)) {

#if 0
            printf("VISITOR fetched main vertex data.\n");
#endif
            concrete_self->step = STEP_DECIDE_MAIN;
            biosal_unitig_visitor_execute(self);
        }
        return;
    } else if (concrete_self->step == STEP_GET_PARENT_VERTEX_DATA) {

        if (biosal_vertex_neighborhood_receive(&concrete_self->parent_neighborhood, message)) {

            concrete_self->step = STEP_DECIDE_PARENT;
            biosal_unitig_visitor_execute(self);
        }
        return;
    } else if (concrete_self->step == STEP_GET_CHILD_VERTEX_DATA) {

        if (biosal_vertex_neighborhood_receive(&concrete_self->child_neighborhood, message)) {
            concrete_self->step = STEP_DECIDE_CHILD;
            biosal_unitig_visitor_execute(self);
        }
        return;
    }

    if (tag == ACTION_START) {

        thorium_actor_send_to_self_empty(self, ACTION_ENABLE_MULTIPLEXER);

        concrete_self->manager = source;

        if (concrete_self->verbose) {
            printf("%s/%d is ready to visit places in the universe\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self));
        }

        core_vector_unpack(&concrete_self->graph_stores, buffer);
        size = core_vector_size(&concrete_self->graph_stores);
        concrete_self->graph_store_index = thorium_actor_get_random_number(self) % size;

        concrete_self->step = STEP_GET_KMER_LENGTH;
        biosal_unitig_visitor_execute(self);

    } else if (tag == ACTION_ASK_TO_STOP) {
        thorium_actor_send_to_self_empty(self, ACTION_STOP);

        thorium_actor_send_reply_empty(self, ACTION_ASK_TO_STOP_REPLY);

    } else if (action == ACTION_ENABLE_VERBOSE_MODE) {

        concrete_self->verbose = TRUE;
    } else if (action == ACTION_DISABLE_VERBOSE_MODE) {

        concrete_self->verbose = FALSE;
    } else if (tag == ACTION_SET_VERTEX_FLAG_REPLY) {

        concrete_self->step = STEP_DO_RESET;
        biosal_unitig_visitor_execute(self);

    } else if (tag == ACTION_ASSEMBLY_GET_STARTING_KMER_REPLY) {

        if (count == 0) {
            ++concrete_self->completed;

            /*
             * Restart somewhere else.
             */
            concrete_self->step = STEP_GET_MAIN_KMER;
            biosal_unitig_visitor_execute(self);

        } else {
            biosal_dna_kmer_unpack(&concrete_self->main_kmer, buffer, concrete_self->kmer_length,
                    &concrete_self->memory_pool, &concrete_self->codec);

            concrete_self->step = STEP_GET_MAIN_VERTEX_DATA;
            biosal_unitig_visitor_execute(self);
        }

    } else if (tag == ACTION_ASSEMBLY_GET_KMER_LENGTH_REPLY) {
        thorium_message_unpack_int(message, 0, &concrete_self->kmer_length);

        concrete_self->step = STEP_GET_MAIN_KMER;
        biosal_unitig_visitor_execute(self);

    } else if (tag == ACTION_ASSEMBLY_GET_VERTEX_REPLY) {

        if (concrete_self->step == STEP_GET_MAIN_VERTEX_DATA) {

            concrete_self->step = STEP_DECIDE_MAIN;
            biosal_unitig_visitor_execute(self);
        }

    } else if (tag == ACTION_YIELD_REPLY) {
        biosal_unitig_visitor_execute(self);
    }
}

void biosal_unitig_visitor_execute(struct thorium_actor *self)
{
    int graph_store_index;
    int graph_store;
    int size;
    struct core_vector coverages;
    int coverage;
    struct biosal_assembly_vertex *other_vertex;
    struct biosal_assembly_vertex *vertex;
    int parents;
    int children;
    int i;
    int other_coverage;
    struct core_memory_pool *ephemeral_memory;
    struct biosal_unitig_visitor *concrete_self;
    int code;
    int expected_code;
    float velocity;
    uint64_t current_time;
    int delta;
    char *new_buffer;
    int new_count;

    concrete_self = thorium_actor_concrete_actor(self);
    size = core_vector_size(&concrete_self->graph_stores);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

#if 0
    printf("biosal_unitig_visitor_execute step= %d\n",
                    concrete_self->step);
#endif

    if (concrete_self->completed == core_vector_size(&concrete_self->graph_stores)) {

        if (concrete_self->verbose) {
            printf("%s/%d : %d graph stores gave nothing !\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self),
                        concrete_self->completed);
        }

        new_count = sizeof(concrete_self->visited_vertices) + sizeof(concrete_self->vertices_with_unitig_flag);
        new_buffer = thorium_actor_allocate(self, new_count);

        core_memory_copy(new_buffer, &concrete_self->visited_vertices, sizeof(concrete_self->visited_vertices));
        core_memory_copy(new_buffer + sizeof(concrete_self->visited_vertices),
                        &concrete_self->vertices_with_unitig_flag,
                        sizeof(concrete_self->vertices_with_unitig_flag));

        thorium_actor_send_buffer(self, concrete_self->manager, ACTION_START_REPLY,
                        new_count, new_buffer);

#if 0
        printf("visitor Finished\n");
#endif
    } else if (concrete_self->step == STEP_GET_KMER_LENGTH) {
        graph_store_index = concrete_self->graph_store_index;
        graph_store = core_vector_at_as_int(&concrete_self->graph_stores, graph_store_index);

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

        /*
         * Reset the index.
         * Also, reset the number of graph stores that have nothing more to yield
         * to avoid a false ending.
         */
        if (concrete_self->graph_store_index == size) {
            concrete_self->graph_store_index = 0;
            concrete_self->completed = 0;
        }

        graph_store = core_vector_at_as_int(&concrete_self->graph_stores, graph_store_index);

        thorium_actor_send_empty(self, graph_store, ACTION_ASSEMBLY_GET_STARTING_KMER);

#if 0
        printf("visitor STEP_GET_MAIN_KMER\n");
#endif

    } else if (concrete_self->step == STEP_GET_MAIN_VERTEX_DATA) {

        /*
         * Fetch everything related to the main kmer.
         */
        biosal_vertex_neighborhood_init(&concrete_self->main_neighborhood,
                            &concrete_self->main_kmer, BIOSAL_ARC_TYPE_ANY, &concrete_self->graph_stores,
                            concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec, self);

        /*
         * Start first one.
         */
        biosal_vertex_neighborhood_receive(&concrete_self->main_neighborhood, NULL);

#if 0
        printf("visitor STEP_GET_MAIN_VERTEX_DATA\n");
#endif

    } else if (concrete_self->step == STEP_DECIDE_MAIN) {

        vertex = biosal_vertex_neighborhood_vertex(&concrete_self->main_neighborhood);
        coverage = biosal_assembly_vertex_coverage_depth(vertex);

        parents = biosal_assembly_vertex_parent_count(vertex);
        children = biosal_assembly_vertex_child_count(vertex);

        /*
         * Select a parent.
         */
        core_vector_init(&coverages, sizeof(int));
        core_vector_set_memory_pool(&coverages, ephemeral_memory);

        for (i = 0; i < parents; ++i) {

            vertex = biosal_vertex_neighborhood_parent(&concrete_self->main_neighborhood, i);
            other_coverage = biosal_assembly_vertex_coverage_depth(vertex);
            core_vector_push_back(&coverages, &other_coverage);
        }

        concrete_self->selected_parent = biosal_unitig_heuristic_select(&concrete_self->heuristic,
                        coverage, &coverages);

        core_vector_clear(&coverages);

        /*
         * Select a child
         */

        for (i = 0; i < children; ++i) {

            vertex = biosal_vertex_neighborhood_child(&concrete_self->main_neighborhood, i);
            other_coverage = biosal_assembly_vertex_coverage_depth(vertex);
            core_vector_push_back(&coverages, &other_coverage);
        }

        concrete_self->selected_child = biosal_unitig_heuristic_select(&concrete_self->heuristic,
                        coverage, &coverages);

        core_vector_destroy(&coverages);

#if 0
        printf("DEBUG selected_parent %d selected_child %d\n",
                        concrete_self->selected_parent, concrete_self->selected_child);
#endif

        if (concrete_self->selected_parent >= 0 && concrete_self->selected_child >= 0) {
            concrete_self->step = STEP_GET_PARENT_VERTEX_DATA;

        } else {
            concrete_self->step = STEP_DO_RESET;
        }
        biosal_unitig_visitor_execute(self);

    } else if (concrete_self->step == STEP_DO_RESET) {

        biosal_dna_kmer_destroy(&concrete_self->main_kmer, &concrete_self->memory_pool);
        biosal_dna_kmer_destroy(&concrete_self->parent_kmer, &concrete_self->memory_pool);
        biosal_dna_kmer_destroy(&concrete_self->child_kmer, &concrete_self->memory_pool);

        biosal_vertex_neighborhood_destroy(&concrete_self->main_neighborhood);
        biosal_vertex_neighborhood_init_empty(&concrete_self->main_neighborhood);

        biosal_vertex_neighborhood_destroy(&concrete_self->parent_neighborhood);
        biosal_vertex_neighborhood_init_empty(&concrete_self->parent_neighborhood);

        biosal_vertex_neighborhood_destroy(&concrete_self->child_neighborhood);
        biosal_vertex_neighborhood_init_empty(&concrete_self->child_neighborhood);


        concrete_self->step = STEP_GET_MAIN_KMER;

        if (concrete_self->verbose
                        && concrete_self->visited_vertices % 500 == 0) {

            current_time = thorium_actor_get_time_in_seconds(self);

            delta = current_time - concrete_self->last_second_count;

            /*
             * Wait at least 4 seconds.
             */
            if (delta >= 4) {

                velocity = (concrete_self->visited_vertices - concrete_self->last_visited_count + 0.0);

                velocity /= delta;

                printf("%s/%d visited %d vertices so far (velocity: %f vertices / s)\n",
                            thorium_actor_script_name(self), thorium_actor_name(self),
                            concrete_self->visited_vertices, velocity);

                concrete_self->last_second_count = current_time;
                concrete_self->last_visited_count = concrete_self->visited_vertices;
            }
        }

        ++concrete_self->visited_vertices;
        biosal_unitig_visitor_execute(self);

    } else if (concrete_self->step == STEP_GET_PARENT_VERTEX_DATA) {

        CORE_DEBUGGER_ASSERT(concrete_self->selected_parent >= 0);
        vertex = biosal_vertex_neighborhood_vertex(&concrete_self->main_neighborhood);
        code = biosal_assembly_vertex_get_parent(vertex,
                            concrete_self->selected_parent);

        biosal_dna_kmer_init_as_parent(&concrete_self->parent_kmer, &concrete_self->main_kmer,
                            code, concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec);

        biosal_vertex_neighborhood_init(&concrete_self->parent_neighborhood,
                            &concrete_self->parent_kmer, BIOSAL_ARC_TYPE_CHILD, &concrete_self->graph_stores,
                            concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec, self);

        /*
         * Start first one.
         */
        biosal_vertex_neighborhood_receive(&concrete_self->parent_neighborhood, NULL);

    } else if (concrete_self->step == STEP_DECIDE_PARENT) {

#if 0
        printf("DEBUG STEP_DECIDE_PARENT\n");
#endif
        /*
         * Verify if the child selected by the parent is the current vertex.
         */

        other_vertex = biosal_vertex_neighborhood_vertex(&concrete_self->parent_neighborhood);
        coverage = biosal_assembly_vertex_coverage_depth(other_vertex);
        children = biosal_assembly_vertex_child_count(other_vertex);

        core_vector_init(&coverages, sizeof(int));
        core_vector_set_memory_pool(&coverages, ephemeral_memory);

        for (i = 0; i < children; ++i) {

            vertex = biosal_vertex_neighborhood_child(&concrete_self->parent_neighborhood, i);
            other_coverage = biosal_assembly_vertex_coverage_depth(vertex);
            core_vector_push_back(&coverages, &other_coverage);
        }

        concrete_self->selected_parent_child = biosal_unitig_heuristic_select(&concrete_self->heuristic,
                        coverage, &coverages);
        core_vector_destroy(&coverages);

        if (concrete_self->selected_parent_child >= 0) {
            code = biosal_assembly_vertex_get_child(other_vertex, concrete_self->selected_parent_child);
            expected_code = biosal_dna_kmer_last_symbol(&concrete_self->main_kmer, concrete_self->kmer_length,
                            &concrete_self->codec);

            if (code == expected_code) {
                concrete_self->step = STEP_GET_CHILD_VERTEX_DATA;
            } else {
#if 0
                printf("Info: parent code mismatch !\n");
#endif
                concrete_self->step = STEP_DO_RESET;
            }
        } else {
            concrete_self->step = STEP_DO_RESET;
        }

        /* recursive call.
         *
         * Otherwise, we could use ACTION_YIELD to do an actor-like recursion.
         */
        biosal_unitig_visitor_execute(self);

    } else if (concrete_self->step == STEP_GET_CHILD_VERTEX_DATA) {

        CORE_DEBUGGER_ASSERT(concrete_self->selected_child >= 0);

        vertex = biosal_vertex_neighborhood_vertex(&concrete_self->main_neighborhood);
        code = biosal_assembly_vertex_get_child(vertex,
                            concrete_self->selected_child);

        biosal_dna_kmer_init_as_child(&concrete_self->child_kmer, &concrete_self->main_kmer,
                            code, concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec);

        biosal_vertex_neighborhood_init(&concrete_self->child_neighborhood,
                            &concrete_self->child_kmer, BIOSAL_ARC_TYPE_PARENT, &concrete_self->graph_stores,
                            concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec, self);

        /*
         * Start first one.
         */
        biosal_vertex_neighborhood_receive(&concrete_self->child_neighborhood, NULL);

    } else if (concrete_self->step == STEP_DECIDE_CHILD) {

#if 0
        printf("step = STEP_DECIDE_CHILD\n");
#endif

        /*
         * Check if the selected parent of the child is the current vertex.
         */
        other_vertex = biosal_vertex_neighborhood_vertex(&concrete_self->child_neighborhood);
        coverage = biosal_assembly_vertex_coverage_depth(other_vertex);
        parents = biosal_assembly_vertex_parent_count(other_vertex);

        core_vector_init(&coverages, sizeof(int));
        core_vector_set_memory_pool(&coverages, ephemeral_memory);

        for (i = 0; i < parents; ++i) {

            vertex = biosal_vertex_neighborhood_parent(&concrete_self->child_neighborhood, i);
            other_coverage = biosal_assembly_vertex_coverage_depth(vertex);
            core_vector_push_back(&coverages, &other_coverage);
        }

        concrete_self->selected_child_parent = biosal_unitig_heuristic_select(&concrete_self->heuristic,
                        coverage, &coverages);
        core_vector_destroy(&coverages);

        if (concrete_self->selected_child_parent >= 0) {
            code = biosal_assembly_vertex_get_parent(other_vertex, concrete_self->selected_child_parent);
            expected_code = biosal_dna_kmer_first_symbol(&concrete_self->main_kmer, concrete_self->kmer_length,
                            &concrete_self->codec);

            if (code == expected_code) {
                concrete_self->step = STEP_MARK_UNITIG;
            } else {
#if 0
                printf("Info: child code mismatch !\n");
#endif
                concrete_self->step = STEP_DO_RESET;
            }
        } else {
            concrete_self->step = STEP_DO_RESET;
        }

        /* recursive call.
         *
         * Otherwise, we could use ACTION_YIELD to do an actor-like recursion.
         */
        biosal_unitig_visitor_execute(self);

    } else if (concrete_self->step == STEP_MARK_UNITIG) {

        biosal_unitig_visitor_mark_vertex(self, &concrete_self->main_kmer);
    }
}

void biosal_unitig_visitor_mark_vertex(struct thorium_actor *self, struct biosal_dna_kmer *kmer)
{
    int flag;
    int new_count;
    char *new_buffer;
    int position;
    int store_index;
    int store;
    struct core_memory_pool *ephemeral_memory;
    struct biosal_unitig_visitor *concrete_self;
    int size;
    struct thorium_message new_message;

    concrete_self = thorium_actor_concrete_actor(self);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    ++concrete_self->vertices_with_unitig_flag;

    /*
     * Mark the vertex with the BIOSAL_VERTEX_FLAG_UNITIG
     */

    flag = BIOSAL_VERTEX_FLAG_UNITIG;

    new_count = biosal_dna_kmer_pack_size(kmer, concrete_self->kmer_length,
            &concrete_self->codec);
    new_count += sizeof(flag);
    new_buffer = thorium_actor_allocate(self, new_count);

    position = 0;
    position += biosal_dna_kmer_pack(kmer, new_buffer, concrete_self->kmer_length, &concrete_self->codec);
    core_memory_copy(new_buffer + position, &flag, sizeof(flag));
    position += sizeof(flag);

    CORE_DEBUGGER_ASSERT(position == new_count);

    size = core_vector_size(&concrete_self->graph_stores);
    store_index = biosal_dna_kmer_store_index(kmer, size, concrete_self->kmer_length,
            &concrete_self->codec, ephemeral_memory);
    store = core_vector_at_as_int(&concrete_self->graph_stores, store_index);

    thorium_message_init(&new_message, ACTION_SET_VERTEX_FLAG, new_count, new_buffer);
    thorium_actor_send(self, store, &new_message);
    thorium_message_destroy(&new_message);
}
