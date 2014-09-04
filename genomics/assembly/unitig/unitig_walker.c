
#include "unitig_walker.h"

#include "../assembly_graph_store.h"
#include "../assembly_vertex.h"

#include <genomics/data/coverage_distribution.h>
#include <genomics/data/dna_kmer.h>
#include <genomics/data/dna_codec.h>

#include <core/system/command.h>
#include <core/system/debugger.h>

#include <stdio.h>
#include <string.h>

#include <inttypes.h>
#include <stdint.h>

/*
 * Debug the walker.
 */
/*
#define BSAL_UNITIG_WALKER_DEBUG
*/

#define OPERATION_FETCH_FIRST 0
#define OPERATION_FETCH_PARENTS 1
#define OPERATION_FETCH_CHILDREN 2

#define OPERATION_SELECT_CHILD 8
#define OPERATION_SELECT_PARENT 9

struct thorium_script bsal_unitig_walker_script = {
    .identifier = SCRIPT_UNITIG_WALKER,
    .name = "bsal_unitig_walker",
    .init = bsal_unitig_walker_init,
    .destroy = bsal_unitig_walker_destroy,
    .receive = bsal_unitig_walker_receive,
    .size = sizeof(struct bsal_unitig_walker),
    .description = "Testbed for testing ideas."
};

void bsal_unitig_walker_init(struct thorium_actor *self)
{
    struct bsal_unitig_walker *concrete_self;
    char *directory_name;
    char *path;
    int argc;
    char **argv;
    char name_as_string[64];
    int name;

    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);

    concrete_self->dried_stores = 0;

    argc = thorium_actor_argc(self);
    argv = thorium_actor_argv(self);

    bsal_vector_init(&concrete_self->graph_stores, sizeof(int));

    thorium_actor_add_action(self, ACTION_ASSEMBLY_GET_STARTING_VERTEX_REPLY,
        bsal_unitig_walker_get_starting_vertex_reply);

    thorium_actor_add_action(self, ACTION_START,
                    bsal_unitig_walker_start);

    thorium_actor_add_action(self, ACTION_ASSEMBLY_GET_VERTEX_REPLY,
                    bsal_unitig_walker_get_vertex_reply);

    thorium_actor_add_action(self, ACTION_ASSEMBLY_GET_VERTICES_AND_SELECT,
                    bsal_unitig_walker_get_vertices_and_select);

    thorium_actor_add_action(self, ACTION_ASSEMBLY_GET_VERTICES_AND_SELECT_REPLY,
                    bsal_unitig_walker_get_vertices_and_select_reply);

    thorium_actor_add_action(self, ACTION_BEGIN,
                    bsal_unitig_walker_begin);

    thorium_actor_add_action_with_condition(self, ACTION_ASSEMBLY_GET_VERTEX_REPLY,
                    bsal_unitig_walker_get_vertex_reply_starting_vertex,
                    &concrete_self->has_starting_vertex, 0);

    bsal_memory_pool_init(&concrete_self->memory_pool, 1048576);

    bsal_vector_init(&concrete_self->left_path, sizeof(int));
    bsal_vector_init(&concrete_self->right_path, sizeof(int));

    bsal_vector_init(&concrete_self->child_vertices, sizeof(struct bsal_assembly_vertex));
    bsal_vector_init(&concrete_self->child_kmers, sizeof(struct bsal_dna_kmer));

    bsal_vector_init(&concrete_self->parent_vertices, sizeof(struct bsal_assembly_vertex));
    bsal_vector_init(&concrete_self->parent_kmers, sizeof(struct bsal_dna_kmer));

    /*
     * Configure the codec.
     */

    bsal_dna_codec_init(&concrete_self->codec);

    if (bsal_dna_codec_must_use_two_bit_encoding(&concrete_self->codec,
                            thorium_actor_get_node_count(self))) {
        bsal_dna_codec_enable_two_bit_encoding(&concrete_self->codec);
    }

    bsal_dna_kmer_init_empty(&concrete_self->current_kmer);
    bsal_assembly_vertex_init(&concrete_self->current_vertex);

    concrete_self->path_index = 0;

    directory_name = bsal_command_get_output_directory(argc, argv);

    name = thorium_actor_name(self);
    sprintf(name_as_string, "%d", name);
    bsal_string_init(&concrete_self->file_path, directory_name);
    bsal_string_append(&concrete_self->file_path, "/dummy_walker_");
    bsal_string_append(&concrete_self->file_path, name_as_string);
    bsal_string_append(&concrete_self->file_path, ".fasta");

    path = bsal_string_get(&concrete_self->file_path);

    bsal_buffered_file_writer_init(&concrete_self->writer, path);

    concrete_self->fetch_operation = OPERATION_FETCH_FIRST;
    concrete_self->select_operation = OPERATION_SELECT_CHILD;
}

void bsal_unitig_walker_destroy(struct thorium_actor *self)
{
    struct bsal_unitig_walker *concrete_self;

    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);

    bsal_memory_pool_destroy(&concrete_self->memory_pool);
    bsal_dna_codec_destroy(&concrete_self->codec);

    bsal_string_destroy(&concrete_self->file_path);

    bsal_buffered_file_writer_destroy(&concrete_self->writer);

    bsal_set_destroy(&concrete_self->visited);

    bsal_vector_destroy(&concrete_self->graph_stores);

    bsal_unitig_walker_clear(self);

    bsal_vector_destroy(&concrete_self->left_path);
    bsal_vector_destroy(&concrete_self->right_path);

    bsal_vector_destroy(&concrete_self->child_kmers);
    bsal_vector_destroy(&concrete_self->child_vertices);
    bsal_vector_destroy(&concrete_self->parent_kmers);
    bsal_vector_destroy(&concrete_self->parent_vertices);

    bsal_dna_kmer_destroy(&concrete_self->current_kmer, &concrete_self->memory_pool);
    bsal_assembly_vertex_destroy(&concrete_self->current_vertex);
}

void bsal_unitig_walker_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    struct bsal_unitig_walker *concrete_self;
    struct bsal_dna_kmer kmer;
    struct bsal_memory_pool *ephemeral_memory;

    if (thorium_actor_take_action(self, message)) {
        return;
    }

    tag = thorium_message_tag(message);
    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    if (tag == ACTION_START) {

    } else if (tag == ACTION_ASK_TO_STOP) {

        thorium_actor_send_to_self_empty(self, ACTION_STOP);

    } else if (tag == ACTION_ASSEMBLY_GET_KMER_LENGTH_REPLY) {

        thorium_message_unpack_int(message, 0, &concrete_self->kmer_length);

        bsal_dna_kmer_init_mock(&kmer, concrete_self->kmer_length, &concrete_self->codec,
                        ephemeral_memory);

        concrete_self->key_length = bsal_dna_kmer_pack_size(&kmer, concrete_self->kmer_length,
                        &concrete_self->codec);

        /*
         * This uses 2-bit encoding to store the visited kmers
         * when there are at least 2 nodes.
         */
        bsal_set_init(&concrete_self->visited, concrete_self->key_length);

        bsal_dna_kmer_destroy(&kmer, ephemeral_memory);

#if 0
        printf("KeyLength %d\n", concrete_self->key_length);
#endif

        thorium_actor_send_to_self_empty(self, ACTION_BEGIN);
    }
}

void bsal_unitig_walker_begin(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_unitig_walker *concrete_self;
    int store_index;
    int store;
    int size;

    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);

    size = bsal_vector_size(&concrete_self->graph_stores);
    store_index = concrete_self->store_index;
    ++concrete_self->store_index;
    concrete_self->store_index %= size;

    store = bsal_vector_at_as_int(&concrete_self->graph_stores, store_index);

    thorium_actor_send_empty(self, store, ACTION_ASSEMBLY_GET_STARTING_VERTEX);
}

void bsal_unitig_walker_start(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct bsal_unitig_walker *concrete_self;
    int graph;
    int source;
    int size;

    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);
    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);
    concrete_self->source = source;

    printf("%s/%d is ready to surf the graph !\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self));

    bsal_vector_unpack(&concrete_self->graph_stores, buffer);
    size = bsal_vector_size(&concrete_self->graph_stores);

    concrete_self->store_index = rand() % size;

    graph = bsal_vector_at_as_int(&concrete_self->graph_stores, concrete_self->store_index);

    thorium_actor_send_empty(self, graph, ACTION_ASSEMBLY_GET_KMER_LENGTH);
}

void bsal_unitig_walker_get_starting_vertex_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_unitig_walker *concrete_self;
    void *buffer;
    int count;
    struct thorium_message new_message;

    count = thorium_message_count(message);

    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);

    /*
     * No more vertices to consume.
     */
    if (count == 0) {

        ++concrete_self->dried_stores;

        if (concrete_self->dried_stores == bsal_vector_size(&concrete_self->graph_stores)) {
            thorium_actor_send_empty(self, concrete_self->source, ACTION_START_REPLY);
        } else {
            thorium_actor_send_to_self_empty(self, ACTION_BEGIN);
        }

        return;
    }

    buffer = thorium_message_buffer(message);

    bsal_dna_kmer_init_empty(&concrete_self->current_kmer);
    bsal_dna_kmer_unpack(&concrete_self->current_kmer, buffer, concrete_self->kmer_length,
                    &concrete_self->memory_pool,
                    &concrete_self->codec);

#ifdef BSAL_UNITIG_WALKER_DEBUG
    printf("%s/%d received starting vertex (%d bytes) from source %d hash %" PRIu64 "\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    count,
                    thorium_message_source(message),
                    bsal_dna_kmer_hash(&concrete_self->current_kmer, concrete_self->kmer_length,
                            &concrete_self->codec));

    bsal_dna_kmer_print(&concrete_self->current_kmer, concrete_self->kmer_length, &concrete_self->codec,
                    ephemeral_memory);
#endif

    /*
     * First steps:
     *
     * - get starting kmer
     * - get starting vertex
     * - set starting vertex
     * - set current kmer
     * - set current vertex
     */
    bsal_dna_kmer_init_copy(&concrete_self->starting_kmer, &concrete_self->current_kmer,
                    concrete_self->kmer_length, &concrete_self->memory_pool,
                    &concrete_self->codec);

    concrete_self->has_starting_vertex = 0;

    concrete_self->fetch_operation = OPERATION_FETCH_FIRST;

    thorium_message_init(&new_message, ACTION_ASSEMBLY_GET_VERTEX, count, buffer);
    thorium_actor_send_reply(self, &new_message);
    thorium_message_destroy(&new_message);
}

void bsal_unitig_walker_get_vertex_reply_starting_vertex(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct bsal_unitig_walker *concrete_self;

    buffer = thorium_message_buffer(message);
    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);

    bsal_assembly_vertex_init(&concrete_self->current_vertex);
    bsal_assembly_vertex_unpack(&concrete_self->current_vertex, buffer);

    bsal_assembly_vertex_init_copy(&concrete_self->starting_vertex,
                    &concrete_self->current_vertex);

#ifdef BSAL_UNITIG_WALKER_DEBUG
    printf("Connectivity for starting vertex: \n");

    bsal_assembly_vertex_print(&concrete_self->current_vertex);
#endif

    /*
     * At this point, this information is fetched:
     *
     * - current_kmer
     * - current_vertex
     *
     * From these, the children kmers can be generated.
     */

    concrete_self->has_starting_vertex = 1;
    bsal_unitig_walker_clear(self);

    thorium_actor_send_to_self_empty(self, ACTION_ASSEMBLY_GET_VERTICES_AND_SELECT);
}

void bsal_unitig_walker_get_vertices_and_select(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_unitig_walker *concrete_self;
    struct thorium_message new_message;
    int new_count;
    void *new_buffer;
    struct bsal_memory_pool *ephemeral_memory;
    int store_index;
    int store;
    int i;
    int child_count;
    int parent_count;
    int code;
    struct bsal_dna_kmer child_kmer;
    struct bsal_dna_kmer *child_kmer_to_fetch;
    struct bsal_dna_kmer parent_kmer;
    struct bsal_dna_kmer *parent_kmer_to_fetch;

    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    /*
     * - generate child kmers
     * - get child vertices
     * - select
     * - append code
     * - set current kmer
     * - set current vertex
     */

    /*
     * Generate child kmers.
     */
    child_count = bsal_assembly_vertex_child_count(&concrete_self->current_vertex);
    parent_count = bsal_assembly_vertex_parent_count(&concrete_self->current_vertex);

    if (bsal_vector_size(&concrete_self->child_kmers) !=
                    child_count) {

#ifdef BSAL_UNITIG_WALKER_DEBUG
        printf("Generate child kmers\n");
#endif
        for (i = 0; i < child_count; i++) {

            code = bsal_assembly_vertex_get_child(&concrete_self->current_vertex, i);

            bsal_dna_kmer_init_as_child(&child_kmer, &concrete_self->current_kmer,
                            code, concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec);

            bsal_vector_push_back(&concrete_self->child_kmers,
                            &child_kmer);
        }
    }

    if (bsal_vector_size(&concrete_self->parent_kmers) !=
                    parent_count) {

        for (i = 0; i < parent_count; i++) {

            code = bsal_assembly_vertex_get_parent(&concrete_self->current_vertex, i);
            bsal_dna_kmer_init_as_parent(&parent_kmer, &concrete_self->current_kmer,
                            code, concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec);

            bsal_vector_push_back(&concrete_self->parent_kmers,
                            &parent_kmer);
        }
    }

    /* Fetch a child vertex.
     */
    if (concrete_self->current_child < child_count) {

        child_kmer_to_fetch = bsal_vector_at(&concrete_self->child_kmers,
                        concrete_self->current_child);

        new_count = bsal_dna_kmer_pack_size(child_kmer_to_fetch, concrete_self->kmer_length,
                    &concrete_self->codec);
        new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

        bsal_dna_kmer_pack(child_kmer_to_fetch, new_buffer,
                    concrete_self->kmer_length,
                    &concrete_self->codec);

        store_index = bsal_dna_kmer_store_index(child_kmer_to_fetch,
                    bsal_vector_size(&concrete_self->graph_stores),
                    concrete_self->kmer_length,
                    &concrete_self->codec, ephemeral_memory);

        store = bsal_vector_at_as_int(&concrete_self->graph_stores, store_index);

        concrete_self->fetch_operation = OPERATION_FETCH_CHILDREN;

        thorium_message_init(&new_message, ACTION_ASSEMBLY_GET_VERTEX, new_count, new_buffer);
        thorium_actor_send(self, store, &new_message);
        thorium_message_destroy(&new_message);

        bsal_memory_pool_free(ephemeral_memory, new_buffer);

    /* Fetch a parent vertex.
     */
    } else if (concrete_self->current_parent < parent_count) {

        parent_kmer_to_fetch = bsal_vector_at(&concrete_self->parent_kmers,
                        concrete_self->current_parent);

        new_count = bsal_dna_kmer_pack_size(parent_kmer_to_fetch, concrete_self->kmer_length,
                    &concrete_self->codec);
        new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

        bsal_dna_kmer_pack(parent_kmer_to_fetch, new_buffer,
                    concrete_self->kmer_length,
                    &concrete_self->codec);

        store_index = bsal_dna_kmer_store_index(parent_kmer_to_fetch,
                    bsal_vector_size(&concrete_self->graph_stores),
                    concrete_self->kmer_length,
                    &concrete_self->codec, ephemeral_memory);

        store = bsal_vector_at_as_int(&concrete_self->graph_stores, store_index);

        concrete_self->fetch_operation = OPERATION_FETCH_PARENTS;

#if 0
        printf("DEBUG send %d/%d ACTION_ASSEMBLY_GET_VERTEX Parent ",
                        concrete_self->current_parent, parent_count);
        bsal_dna_kmer_print(parent_kmer_to_fetch, concrete_self->kmer_length, &concrete_self->codec,
                ephemeral_memory);
        printf("Current: ");
        bsal_dna_kmer_print(&concrete_self->current_kmer, concrete_self->kmer_length, &concrete_self->codec,
                    ephemeral_memory);
#endif

        thorium_message_init(&new_message, ACTION_ASSEMBLY_GET_VERTEX, new_count, new_buffer);
        thorium_actor_send(self, store, &new_message);
        thorium_message_destroy(&new_message);

        bsal_memory_pool_free(ephemeral_memory, new_buffer);

    } else {

        /*
         * - select
         * - append code
         * - set current kmer
         * - set current vertex
         * - send message.
         */

#ifdef BSAL_UNITIG_WALKER_DEBUG
        printf("Got all child vertices (%d)\n",
                        size);
#endif

        bsal_unitig_walker_make_decision(self);
    }
}

void bsal_unitig_walker_get_vertices_and_select_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_unitig_walker *concrete_self;

    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);

    bsal_unitig_walker_dump_path(self);

    printf("path_index is %d\n", concrete_self->path_index);
    thorium_actor_send_to_self_empty(self, ACTION_BEGIN);
}

void bsal_unitig_walker_get_vertex_reply(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct bsal_unitig_walker *concrete_self;
    struct bsal_assembly_vertex vertex;

    buffer = thorium_message_buffer(message);
    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);

    bsal_assembly_vertex_init(&vertex);
    bsal_assembly_vertex_unpack(&vertex, buffer);

#ifdef BSAL_UNITIG_WALKER_DEBUG
    printf("Connectivity for vertex: \n");

    bsal_assembly_vertex_print(&vertex);
#endif

    /*
     * OPERATION_FETCH_FIRST uses a different code path.
     */
    BSAL_DEBUGGER_ASSERT(concrete_self->fetch_operation != OPERATION_FETCH_FIRST);

    if (concrete_self->fetch_operation == OPERATION_FETCH_CHILDREN) {
        bsal_vector_push_back(&concrete_self->child_vertices, &vertex);
        ++concrete_self->current_child;

    } else if (concrete_self->fetch_operation == OPERATION_FETCH_PARENTS) {

        bsal_vector_push_back(&concrete_self->parent_vertices, &vertex);
        ++concrete_self->current_parent;
    }

    thorium_actor_send_to_self_empty(self, ACTION_ASSEMBLY_GET_VERTICES_AND_SELECT);
}

void bsal_unitig_walker_clear(struct thorium_actor *self)
{
    struct bsal_unitig_walker *concrete_self;
    int parent_count;
    int child_count;
    int i;
    struct bsal_dna_kmer *kmer;
    struct bsal_assembly_vertex *vertex;

    concrete_self = thorium_actor_concrete_actor(self);

    /*
     * Clear children
     */
    child_count = bsal_vector_size(&concrete_self->child_kmers);

    for (i = 0; i < child_count; i++) {

        kmer = bsal_vector_at(&concrete_self->child_kmers, i);
        vertex = bsal_vector_at(&concrete_self->child_vertices, i);

        bsal_dna_kmer_destroy(kmer, &concrete_self->memory_pool);
        bsal_assembly_vertex_destroy(vertex);
    }

    concrete_self->current_child = 0;
    bsal_vector_clear(&concrete_self->child_kmers);
    bsal_vector_clear(&concrete_self->child_vertices);

    /*
     * Clear parents
     */
    parent_count = bsal_vector_size(&concrete_self->parent_kmers);

    for (i = 0; i < parent_count; i++) {

        kmer = bsal_vector_at(&concrete_self->parent_kmers, i);
        vertex = bsal_vector_at(&concrete_self->parent_vertices, i);

        bsal_dna_kmer_destroy(kmer, &concrete_self->memory_pool);
        bsal_assembly_vertex_destroy(vertex);
    }

    concrete_self->current_parent =0;
    bsal_vector_clear(&concrete_self->parent_kmers);
    bsal_vector_clear(&concrete_self->parent_vertices);
}

void bsal_unitig_walker_dump_path(struct thorium_actor *self)
{
    struct bsal_unitig_walker *concrete_self;
    char *sequence;
    int left_path_arcs;
    int right_path_arcs;
    int sequence_length;
    struct bsal_memory_pool *ephemeral_memory;
    int position;
    int i;
    char nucleotide;
    int code;

    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    left_path_arcs = bsal_vector_size(&concrete_self->left_path);
    right_path_arcs = bsal_vector_size(&concrete_self->right_path);

    sequence_length = 0;

    printf("DEBUG dump_path() kmer_length %d left_path_arcs %d right_path_arcs %d\n",
                    concrete_self->kmer_length,
                    left_path_arcs, right_path_arcs);

    sequence_length += concrete_self->kmer_length;
    sequence_length += left_path_arcs;
    sequence_length += right_path_arcs;

    sequence = bsal_memory_pool_allocate(ephemeral_memory, sequence_length + 1);

    position = 0;

    /*
     * On the left
     */
    for (i = left_path_arcs - 1 ; i >= 0; --i) {

        code = bsal_vector_at_as_int(&concrete_self->left_path, i);
        nucleotide = bsal_dna_codec_get_nucleotide_from_code(code);
        sequence[position] = nucleotide;
        ++position;
    }

    /*
     * Starting kmer.
     */
    bsal_dna_kmer_get_sequence(&concrete_self->starting_kmer, sequence + position,
                    concrete_self->kmer_length,
                    &concrete_self->codec);
    position += concrete_self->kmer_length;

    /*
     * And on the right.
     */
    for (i = 0 ; i < right_path_arcs; i++) {

        code = bsal_vector_at_as_int(&concrete_self->right_path, i);
        nucleotide = bsal_dna_codec_get_nucleotide_from_code(code);
        sequence[position] = nucleotide;
        ++position;
    }

    BSAL_DEBUGGER_ASSERT(position == sequence_length);

    sequence[sequence_length] = '\0';

    bsal_unitig_walker_write(self, concrete_self->hash_value,
                    sequence, sequence_length);

    bsal_memory_pool_free(ephemeral_memory, sequence);

    bsal_vector_resize(&concrete_self->left_path, 0);
    bsal_vector_resize(&concrete_self->right_path, 0);
    bsal_dna_kmer_destroy(&concrete_self->starting_kmer, &concrete_self->memory_pool);
    bsal_assembly_vertex_destroy(&concrete_self->starting_vertex);

    bsal_set_clear(&concrete_self->visited);

    ++concrete_self->path_index;
}

int bsal_unitig_walker_select(struct thorium_actor *self)
{
    int choice;
    struct bsal_unitig_walker *concrete_self;
    int current_coverage;
    int size;
    int i;
    int code;
    char nucleotide;
    struct bsal_assembly_vertex *vertex;
    struct bsal_dna_kmer *kmer;
    int coverage;
    void *key;
    struct bsal_memory_pool *ephemeral_memory;
    int found;
    struct bsal_vector *selected_vertices;
    struct bsal_vector *selected_kmers;
    struct bsal_assembly_vertex *other_vertex;
    int threshold;
    int threshold_weak;
    int j;
    int other_coverage;
    int is_strong;

    /*
     * This code select the best edge for a unitig.
     */
    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    key = bsal_memory_pool_allocate(ephemeral_memory, concrete_self->key_length);

    choice = -1;

    size = 0;
    found = 0;

    if (concrete_self->select_operation == OPERATION_SELECT_CHILD) {
        selected_vertices = &concrete_self->child_vertices;
        selected_kmers = &concrete_self->child_kmers;
    } else /*if (concrete_self->select_operation == OPERATION_SELECT_PARENT) */{
        selected_vertices = &concrete_self->parent_vertices;
        selected_kmers = &concrete_self->parent_kmers;
    }

    size = bsal_vector_size(selected_vertices);
    current_coverage = bsal_assembly_vertex_coverage_depth(&concrete_self->current_vertex);
    threshold = current_coverage;
    threshold_weak = current_coverage / 4;

    if (size >= 2)
        threshold /= size;

    for (i = 0; i < size; i++) {

        vertex = bsal_vector_at(selected_vertices, i);

        /*
         * Verify if it was visited already.
         */
        kmer = bsal_vector_at(selected_kmers, i);

        bsal_dna_kmer_pack(kmer, key, concrete_self->kmer_length,
                        &concrete_self->codec);

        found = bsal_set_find(&concrete_self->visited, key);

#ifdef BSAL_UNITIG_WALKER_DEBUG
        printf("Find result %d\n", found);

        bsal_dna_kmer_print(kmer, concrete_self->kmer_length, &concrete_self->codec,
                        ephemeral_memory);
#endif

        if (found) {
            printf("This one was already visited:\n");

            bsal_dna_kmer_print(kmer, concrete_self->kmer_length, &concrete_self->codec,
                        ephemeral_memory);
            continue;
        }

        /*
         * Otherwise, if there is only one choice, return it.
         */

        if (size == 1) {
            choice = i;
            break;
        }

        coverage = bsal_assembly_vertex_coverage_depth(vertex);

        /*
         * This change is too big to be OK.
         */
        if (coverage >= 2 * current_coverage) {
            continue;
        }

        /*
         * This is a unitig, so it must be
         * regular. Otherwise there could be DNA misassemblies
         * at this stage.
         */
        if (!(coverage >= threshold)) {
            continue;
        }

        is_strong = 1;

        /*
         * Check out the others too and make sure that they are all weak.
         */
        for (j = 0; j < size; ++j) {

            other_vertex = bsal_vector_at(selected_vertices, j);
            other_coverage = bsal_assembly_vertex_coverage_depth(vertex);

            if (other_coverage >= threshold_weak) {
                is_strong = 0;
                break;
            }
        }

        if (!is_strong) {
            continue;
        }

        /*
         * At this point, the edge satisfies everything.
         */

        choice = i;
        break;
    }

#ifdef BSAL_UNITIG_WALKER_DEBUG
    printf("Choice is %d\n", choice);
#endif

#if 0
    /*
     * Avoid an infinite loop since this select function is just
     * a test.
     */
    if (bsal_vector_size(&concrete_self->left_path) >= 100000) {

        choice = -1;
    }
#endif

    bsal_memory_pool_free(ephemeral_memory, key);

    if (choice < 0) {
        printf("Notice: can not select, current_coverage %d, %d arcs: ", current_coverage, size);
        for (i = 0; i < size; i++) {

            if (concrete_self->select_operation == OPERATION_SELECT_CHILD) {
                code = bsal_assembly_vertex_get_child(&concrete_self->current_vertex, i);
            } else /*if (concrete_self->select_operation == OPERATION_SELECT_CHILD) */ {
                code = bsal_assembly_vertex_get_parent(&concrete_self->current_vertex, i);
            }

            nucleotide = bsal_dna_codec_get_nucleotide_from_code(code);
            vertex = bsal_vector_at(selected_vertices, i);
            kmer = bsal_vector_at(selected_kmers, i);

            coverage = bsal_assembly_vertex_coverage_depth(vertex);

            printf(" (%c %d)", nucleotide, coverage);
        }
        printf("\n");
        printf("Most likely reason (select_operation= ");

        if (concrete_self->select_operation == OPERATION_SELECT_CHILD)
            printf("OPERATION_SELECT_CHILD): ");
        else
            printf("OPERATION_SELECT_PARENT): ");

        if (size == 0) {
            printf("0 choice, dead end\n");
        } else if (size == 1 && found) {
            printf("1 choice, already in use\n");
        } else {
            printf("Unknown\n");
        }
    }

    return choice;
}

void bsal_unitig_walker_write(struct thorium_actor *self, uint64_t name,
                char *sequence,
                int sequence_length)
{
    struct bsal_unitig_walker *concrete_self;
    int column_width;
    char *buffer;
    struct bsal_memory_pool *ephemeral_memory;
    int block_length;
    int i;

    /*
     * \see http://en.wikipedia.org/wiki/FASTA_format
     */

    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);
    column_width = 80;
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    buffer = bsal_memory_pool_allocate(ephemeral_memory, column_width + 1);

    i = 0;

#if 0
    printf("LENGTH=%d\n", sequence_length);
#endif

    bsal_buffered_file_writer_printf(&concrete_self->writer,
                    ">path_%" PRIu64 " length=%d\n",
                    name,
                    sequence_length);

    while (i < sequence_length) {

        block_length = sequence_length - i;

        if (column_width < block_length) {
            block_length = column_width;
        }

        memcpy(buffer, sequence + i, block_length);
        buffer[block_length] = '\0';

#if 0
        printf("BLOCK %d <<%s>>\n",
                        block_length, buffer);
#endif

        bsal_buffered_file_writer_printf(&concrete_self->writer,
                        "%s\n", buffer);

        i += block_length;
    }

    bsal_memory_pool_free(ephemeral_memory, buffer);
}

void bsal_unitig_walker_make_decision(struct thorium_actor *self)
{
    struct bsal_dna_kmer *kmer;
    struct bsal_assembly_vertex *vertex;
    int choice;
    void *key;
    int coverage;
    struct bsal_unitig_walker *concrete_self;
    struct bsal_vector *selected_vertices;
    struct bsal_vector *selected_kmers;
    struct bsal_vector *selected_output_path;
    struct bsal_memory_pool *ephemeral_memory;
    int code;
    uint64_t hash_value;

    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    /*
     * Select a choice and carry on...
     */
    choice = bsal_unitig_walker_select(self);

    if (concrete_self->select_operation == OPERATION_SELECT_CHILD) {
        selected_vertices = &concrete_self->child_vertices;
        selected_kmers = &concrete_self->child_kmers;
        selected_output_path = &concrete_self->right_path;
    } else /* if (concrete_self->select_operation == OPERATION_SELECT_PARENT)*/ {
        selected_vertices = &concrete_self->parent_vertices;
        selected_kmers = &concrete_self->parent_kmers;
        selected_output_path = &concrete_self->left_path;
    }

    /*
     * Proceed with the choice
     * (this is either for OPERATION_SELECT_CHILD or OPERATION_SELECT_PARENT).
     */
    if (choice >= 0) {

        coverage = bsal_assembly_vertex_coverage_depth(&concrete_self->current_vertex);

        if (concrete_self->select_operation == OPERATION_SELECT_CHILD) {
            code = bsal_assembly_vertex_get_child(&concrete_self->current_vertex, choice);

        } else if (concrete_self->select_operation == OPERATION_SELECT_PARENT) {
            code = bsal_assembly_vertex_get_parent(&concrete_self->current_vertex, choice);
        }

        kmer = bsal_vector_at(selected_kmers, choice);
        vertex = bsal_vector_at(selected_vertices, choice);
        bsal_vector_push_back(selected_output_path, &code);

        key = bsal_memory_pool_allocate(ephemeral_memory, concrete_self->key_length);
        bsal_dna_kmer_pack(kmer, key, concrete_self->kmer_length, &concrete_self->codec);

        bsal_set_add(&concrete_self->visited, key);

        #ifdef BSAL_UNITIG_WALKER_DEBUG
        printf("Added (%d)\n",
                        (int)bsal_set_size(&concrete_self->visited));

        bsal_dna_kmer_print(kmer, concrete_self->kmer_length, &concrete_self->codec,
                        ephemeral_memory);
        #endif
        #ifdef BSAL_UNITIG_WALKER_DEBUG
        printf("coverage= %d, path now has %d arcs\n", coverage,
                        (int)bsal_vector_size(&concrete_self->path));
        #endif

        bsal_memory_pool_free(ephemeral_memory, key);

        bsal_unitig_walker_set_current(self, kmer, vertex);

        bsal_unitig_walker_clear(self);

        thorium_actor_send_to_self_empty(self, ACTION_ASSEMBLY_GET_VERTICES_AND_SELECT);

    } else if (concrete_self->select_operation == OPERATION_SELECT_CHILD) {

        /*
         * Switch now to OPERATION_SELECT_PARENT
         */
        concrete_self->select_operation = OPERATION_SELECT_PARENT;

        hash_value = bsal_dna_kmer_canonical_hash(&concrete_self->current_kmer,
                        concrete_self->kmer_length, &concrete_self->codec,
                       ephemeral_memory);

        concrete_self->hash_value = hash_value;

        /*
         * Change the current vertex.
         */
        bsal_unitig_walker_set_current(self, &concrete_self->starting_kmer,
                        &concrete_self->starting_vertex);

        bsal_unitig_walker_clear(self);

        thorium_actor_send_to_self_empty(self, ACTION_ASSEMBLY_GET_VERTICES_AND_SELECT);

    /* Finish
     */
    } else if (concrete_self->select_operation == OPERATION_SELECT_PARENT) {

        /*
         * Get the hash value
         */
        hash_value = bsal_dna_kmer_canonical_hash(&concrete_self->current_kmer,
                        concrete_self->kmer_length, &concrete_self->codec,
                        ephemeral_memory);

        if (hash_value < concrete_self->hash_value) {
            concrete_self->hash_value = hash_value;
        }

        bsal_dna_kmer_destroy(&concrete_self->current_kmer, &concrete_self->memory_pool);
        bsal_assembly_vertex_destroy(&concrete_self->current_vertex);

        bsal_unitig_walker_clear(self);

        concrete_self->select_operation = OPERATION_SELECT_CHILD;

        thorium_actor_send_to_self_empty(self, ACTION_ASSEMBLY_GET_VERTICES_AND_SELECT_REPLY);
    }
}

void bsal_unitig_walker_set_current(struct thorium_actor *self,
                struct bsal_dna_kmer *kmer, struct bsal_assembly_vertex *vertex)
{
    struct bsal_unitig_walker *concrete_self;

    concrete_self = (struct bsal_unitig_walker *)thorium_actor_concrete_actor(self);

    bsal_dna_kmer_destroy(&concrete_self->current_kmer, &concrete_self->memory_pool);

    bsal_dna_kmer_init_copy(&concrete_self->current_kmer, kmer,
            concrete_self->kmer_length, &concrete_self->memory_pool,
            &concrete_self->codec);

    bsal_assembly_vertex_destroy(&concrete_self->current_vertex);
    bsal_assembly_vertex_init_copy(&concrete_self->current_vertex, vertex);
}
