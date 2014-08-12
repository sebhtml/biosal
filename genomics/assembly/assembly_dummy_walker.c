
#include "assembly_dummy_walker.h"

#include "assembly_graph_store.h"
#include "assembly_vertex.h"

#include <genomics/data/coverage_distribution.h>
#include <genomics/data/dna_kmer.h>
#include <genomics/data/dna_codec.h>

#include <stdio.h>
#include <string.h>

#include <inttypes.h>
#include <stdint.h>

/*
 * Debug the walker.
 */
/*
#define BSAL_ASSEMBLY_DUMMY_WALKER_DEBUG
*/

struct bsal_script bsal_assembly_dummy_walker_script = {
    .identifier = BSAL_ASSEMBLY_DUMMY_WALKER_SCRIPT,
    .name = "bsal_assembly_dummy_walker",
    .init = bsal_assembly_dummy_walker_init,
    .destroy = bsal_assembly_dummy_walker_destroy,
    .receive = bsal_assembly_dummy_walker_receive,
    .size = sizeof(struct bsal_assembly_dummy_walker)
};

void bsal_assembly_dummy_walker_init(struct bsal_actor *self)
{
    struct bsal_assembly_dummy_walker *concrete_self;
    char *directory_name;
    char *path;
    int argc;
    char **argv;

    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);

    argc = bsal_actor_argc(self);
    argv = bsal_actor_argv(self);

    bsal_vector_init(&concrete_self->graph_stores, sizeof(int));

    bsal_actor_add_route(self, BSAL_ASSEMBLY_GET_STARTING_VERTEX_REPLY,
        bsal_assembly_dummy_walker_get_starting_vertex_reply);

    bsal_actor_add_route(self, BSAL_ACTOR_START,
                    bsal_assembly_dummy_walker_start);

    bsal_actor_add_route(self, BSAL_ASSEMBLY_GET_VERTEX_REPLY,
                    bsal_assembly_dummy_walker_get_vertex_reply);

    bsal_actor_add_route(self, BSAL_ASSEMBLY_GET_VERTICES_AND_SELECT,
                    bsal_assembly_dummy_walker_get_vertices_and_select);

    bsal_actor_add_route(self, BSAL_ASSEMBLY_GET_VERTICES_AND_SELECT_REPLY,
                    bsal_assembly_dummy_walker_get_vertices_and_select_reply);

    bsal_actor_add_route(self, BSAL_ACTOR_BEGIN,
                    bsal_assembly_dummy_walker_begin);

    bsal_actor_add_route_with_condition(self, BSAL_ASSEMBLY_GET_VERTEX_REPLY,
                    bsal_assembly_dummy_walker_get_vertex_reply_starting_vertex,
                    &concrete_self->has_starting_vertex, 0);

    bsal_memory_pool_init(&concrete_self->memory_pool, 32768);

    bsal_vector_init(&concrete_self->path, sizeof(int));

    bsal_vector_init(&concrete_self->child_vertices, sizeof(struct bsal_assembly_vertex));
    bsal_vector_init(&concrete_self->child_kmers, sizeof(struct bsal_dna_kmer));

    /*
     * Configure the codec.
     */

    bsal_dna_codec_init(&concrete_self->codec);

    if (bsal_actor_get_node_count(self) >= BSAL_DNA_CODEC_MINIMUM_NODE_COUNT_FOR_TWO_BIT) {
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_TRANSPORT
        bsal_dna_codec_enable_two_bit_encoding(&concrete_self->codec);
#endif
    }

    bsal_dna_kmer_init_empty(&concrete_self->current_kmer);
    bsal_assembly_vertex_init(&concrete_self->current_vertex);

    concrete_self->path_index = 0;

    directory_name = bsal_get_output_directory(argc, argv);

    bsal_string_init(&concrete_self->file_path, directory_name);
    bsal_string_append(&concrete_self->file_path, "/dummy_walker.fasta");

    path = bsal_string_get(&concrete_self->file_path);

    bsal_buffered_file_writer_init(&concrete_self->writer, path);
}

void bsal_assembly_dummy_walker_destroy(struct bsal_actor *self)
{
    struct bsal_assembly_dummy_walker *concrete_self;

    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);

    bsal_string_destroy(&concrete_self->file_path);

    bsal_buffered_file_writer_destroy(&concrete_self->writer);

    bsal_set_destroy(&concrete_self->visited);

    bsal_vector_destroy(&concrete_self->graph_stores);

    bsal_assembly_dummy_walker_clean_children(self);

    bsal_vector_destroy(&concrete_self->path);
    bsal_vector_destroy(&concrete_self->child_kmers);
    bsal_vector_destroy(&concrete_self->child_vertices);

    bsal_dna_kmer_destroy(&concrete_self->current_kmer, &concrete_self->memory_pool);
    bsal_assembly_vertex_destroy(&concrete_self->current_vertex);
}

void bsal_assembly_dummy_walker_receive(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    struct bsal_assembly_dummy_walker *concrete_self;
    struct bsal_dna_kmer kmer;
    struct bsal_memory_pool *ephemeral_memory;

    if (bsal_actor_use_route(self, message)) {
        return;
    }

    tag = bsal_message_tag(message);
    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    if (tag == BSAL_ACTOR_START) {

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_STOP);

    } else if (tag == BSAL_ASSEMBLY_GET_KMER_LENGTH_REPLY) {

        bsal_message_unpack_int(message, 0, &concrete_self->kmer_length);

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

        printf("KeyLength %d\n", concrete_self->key_length);

        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_BEGIN);
    }
}

void bsal_assembly_dummy_walker_begin(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_dummy_walker *concrete_self;
    int store_index;
    int store;

    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);

    store_index = 0;
    store = bsal_vector_at_as_int(&concrete_self->graph_stores, store_index);

    bsal_actor_send_empty(self, store, BSAL_ASSEMBLY_GET_STARTING_VERTEX);
}

void bsal_assembly_dummy_walker_start(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_dummy_walker *concrete_self;
    int graph;

    buffer = bsal_message_buffer(message);
    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);

    printf("%s/%d is ready to surf the graph !\n",
                        bsal_actor_script_name(self),
                        bsal_actor_name(self));

    bsal_vector_unpack(&concrete_self->graph_stores, buffer);

    graph = bsal_vector_at_as_int(&concrete_self->graph_stores, 0);

    bsal_actor_send_empty(self, graph, BSAL_ASSEMBLY_GET_KMER_LENGTH);
}

void bsal_assembly_dummy_walker_get_starting_vertex_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_dummy_walker *concrete_self;
    void *buffer;
    struct bsal_memory_pool *ephemeral_memory;
    int count;
    struct bsal_message new_message;

    count = bsal_message_count(message);

    /*
     * No more vertices to consume.
     */
    if (count == 0) {

        bsal_actor_send_to_supervisor_empty(self, BSAL_ACTOR_START_REPLY);
        return;
    }

    buffer = bsal_message_buffer(message);
    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    bsal_dna_kmer_init_empty(&concrete_self->current_kmer);
    bsal_dna_kmer_unpack(&concrete_self->current_kmer, buffer, concrete_self->kmer_length,
                    &concrete_self->memory_pool,
                    &concrete_self->codec);

#ifdef BSAL_ASSEMBLY_DUMMY_WALKER_DEBUG
    printf("%s/%d received starting vertex (%d bytes) from source %d hash %" PRIu64 "\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    count,
                    bsal_message_source(message),
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

    bsal_message_init(&new_message, BSAL_ASSEMBLY_GET_VERTEX, count, buffer);
    bsal_actor_send_reply(self, &new_message);
    bsal_message_destroy(&new_message);
}

void bsal_assembly_dummy_walker_get_vertex_reply_starting_vertex(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_dummy_walker *concrete_self;
    struct bsal_memory_pool *ephemeral_memory;

    buffer = bsal_message_buffer(message);
    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    bsal_assembly_vertex_init(&concrete_self->current_vertex);
    bsal_assembly_vertex_unpack(&concrete_self->current_vertex, buffer);

#ifdef BSAL_ASSEMBLY_DUMMY_WALKER_DEBUG
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
    bsal_assembly_dummy_walker_clean_children(self);

    bsal_actor_send_to_self_empty(self, BSAL_ASSEMBLY_GET_VERTICES_AND_SELECT);
}

void bsal_assembly_dummy_walker_get_vertices_and_select(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_dummy_walker *concrete_self;
    struct bsal_message new_message;
    int new_count;
    void *new_buffer;
    struct bsal_memory_pool *ephemeral_memory;
    int store_index;
    int store;
    int i;
    int size;
    int code;
    struct bsal_dna_kmer child_kmer;
    struct bsal_dna_kmer *child_kmer_to_fetch;
    struct bsal_dna_kmer *kmer;
    struct bsal_assembly_vertex *vertex;
    int choice;
    void *key;

    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

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
    size = bsal_assembly_vertex_child_count(&concrete_self->current_vertex);

    if (bsal_vector_size(&concrete_self->child_kmers) !=
                    size) {

#ifdef BSAL_ASSEMBLY_DUMMY_WALKER_DEBUG
        printf("Generate child kmers\n");
#endif
        for (i = 0; i < size; i++) {

            code = bsal_assembly_vertex_get_child(&concrete_self->current_vertex, i);

            bsal_dna_kmer_init_as_child(&child_kmer, &concrete_self->current_kmer,
                            code, concrete_self->kmer_length, &concrete_self->memory_pool,
                            &concrete_self->codec);

            bsal_vector_push_back(&concrete_self->child_kmers,
                            &child_kmer);
        }
    }

    /* Fetch a child vertex.
     */
    if (concrete_self->current_child < size) {

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

        bsal_message_init(&new_message, BSAL_ASSEMBLY_GET_VERTEX, new_count, new_buffer);
        bsal_actor_send(self, store, &new_message);
        bsal_message_destroy(&new_message);

        bsal_memory_pool_free(ephemeral_memory, new_buffer);

    } else {

        /*
         * - select
         * - append code
         * - set current kmer
         * - set current vertex
         * - send message.
         */

#ifdef BSAL_ASSEMBLY_DUMMY_WALKER_DEBUG
        printf("Got all child vertices (%d)\n",
                        size);
#endif

        choice = bsal_assembly_dummy_walker_select(self);

        /*
         * Proceed with the choice.
         */
        if (choice >= 0) {

            code = bsal_assembly_vertex_get_child(&concrete_self->current_vertex, choice);
            bsal_vector_push_back(&concrete_self->path, &code);

#ifdef BSAL_ASSEMBLY_DUMMY_WALKER_DEBUG
            printf("Path now has %d arcs\n", (int)bsal_vector_size(&concrete_self->path));
#endif

            kmer = bsal_vector_at(&concrete_self->child_kmers, choice);
            vertex = bsal_vector_at(&concrete_self->child_vertices, choice);

            key = bsal_memory_pool_allocate(ephemeral_memory, concrete_self->key_length);
            bsal_dna_kmer_pack(kmer, key, concrete_self->kmer_length, &concrete_self->codec);

            bsal_set_add(&concrete_self->visited, key);

#ifdef BSAL_ASSEMBLY_DUMMY_WALKER_DEBUG
            printf("Added (%d)\n",
                            (int)bsal_set_size(&concrete_self->visited));

            bsal_dna_kmer_print(kmer, concrete_self->kmer_length, &concrete_self->codec,
                            ephemeral_memory);
#endif

            bsal_memory_pool_free(ephemeral_memory, key);

            bsal_dna_kmer_destroy(&concrete_self->current_kmer, &concrete_self->memory_pool);

            bsal_dna_kmer_init_copy(&concrete_self->current_kmer, kmer,
                    concrete_self->kmer_length, &concrete_self->memory_pool,
                    &concrete_self->codec);

            bsal_assembly_vertex_destroy(&concrete_self->current_vertex);
            bsal_assembly_vertex_init_copy(&concrete_self->current_vertex, vertex);

            bsal_assembly_dummy_walker_clean_children(self);

            bsal_actor_send_to_self_empty(self, BSAL_ASSEMBLY_GET_VERTICES_AND_SELECT);

        /* Finish
         */
        } else {
            bsal_dna_kmer_destroy(&concrete_self->current_kmer, &concrete_self->memory_pool);
            bsal_assembly_vertex_destroy(&concrete_self->current_vertex);

            bsal_assembly_dummy_walker_clean_children(self);

            bsal_actor_send_to_self_empty(self, BSAL_ASSEMBLY_GET_VERTICES_AND_SELECT_REPLY);
        }
    }
}

void bsal_assembly_dummy_walker_get_vertices_and_select_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_dummy_walker *concrete_self;

    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);

    bsal_assembly_dummy_walker_dump_path(self);

    if (concrete_self->path_index < 100) {

        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_BEGIN);

    } else {
        bsal_actor_send_to_supervisor_empty(self, BSAL_ACTOR_START_REPLY);
    }
}

void bsal_assembly_dummy_walker_get_vertex_reply(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_dummy_walker *concrete_self;
    struct bsal_memory_pool *ephemeral_memory;
    struct bsal_assembly_vertex vertex;

    buffer = bsal_message_buffer(message);
    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    bsal_assembly_vertex_init(&vertex);
    bsal_assembly_vertex_unpack(&vertex, buffer);

#ifdef BSAL_ASSEMBLY_DUMMY_WALKER_DEBUG
    printf("Connectivity for vertex: \n");

    bsal_assembly_vertex_print(&vertex);
#endif

    bsal_vector_push_back(&concrete_self->child_vertices, &vertex);

    ++concrete_self->current_child;

    bsal_actor_send_to_self_empty(self, BSAL_ASSEMBLY_GET_VERTICES_AND_SELECT);
}

void bsal_assembly_dummy_walker_clean_children(struct bsal_actor *self)
{
    struct bsal_assembly_dummy_walker *concrete_self;
    int size;
    int i;
    struct bsal_dna_kmer *kmer;
    struct bsal_assembly_vertex *vertex;

    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);

    size = bsal_vector_size(&concrete_self->child_kmers);

    for (i = 0; i < size; i++) {

        kmer = bsal_vector_at(&concrete_self->child_kmers, i);
        vertex = bsal_vector_at(&concrete_self->child_vertices, i);

        bsal_dna_kmer_destroy(kmer, &concrete_self->memory_pool);
        bsal_assembly_vertex_destroy(vertex);
    }

    bsal_vector_clear(&concrete_self->child_kmers);
    bsal_vector_clear(&concrete_self->child_vertices);

    concrete_self->current_child = 0;
}

void bsal_assembly_dummy_walker_dump_path(struct bsal_actor *self)
{
    struct bsal_assembly_dummy_walker *concrete_self;
    char *sequence;
    int path_arcs;
    int sequence_length;
    struct bsal_memory_pool *ephemeral_memory;
    int position;
    int i;
    char nucleotide;
    int code;

    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    path_arcs = bsal_vector_size(&concrete_self->path);
    sequence_length = 0;

    sequence_length += concrete_self->kmer_length;

    sequence_length += path_arcs;

    sequence = bsal_memory_pool_allocate(ephemeral_memory, sequence_length + 1);

    position = 0;
    bsal_dna_kmer_get_sequence(&concrete_self->starting_kmer, sequence, concrete_self->kmer_length,
                    &concrete_self->codec);
    position += concrete_self->kmer_length;

    for (i = 0 ; i < path_arcs; i++) {

        code = bsal_vector_at_as_int(&concrete_self->path, i);
        nucleotide = bsal_dna_codec_get_nucleotide_from_code(code);

        sequence[position] = nucleotide;
        ++position;
    }

    sequence[sequence_length] = '\0';

    bsal_assembly_dummy_walker_write(self, sequence, sequence_length);

    bsal_memory_pool_free(ephemeral_memory, sequence);

    bsal_vector_resize(&concrete_self->path, 0);
    bsal_dna_kmer_destroy(&concrete_self->starting_kmer, &concrete_self->memory_pool);

    bsal_set_clear(&concrete_self->visited);

    ++concrete_self->path_index;
}

int bsal_assembly_dummy_walker_select(struct bsal_actor *self)
{
    int choice;
    struct bsal_assembly_dummy_walker *concrete_self;
    int best_coverage;
    int current_coverage;
    int best_difference;
    int difference;
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

    /*
     * This code is just a test, it uses the closest coverage, always.
     */
    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    key = bsal_memory_pool_allocate(ephemeral_memory, concrete_self->key_length);

    choice = -1;

    size = bsal_vector_size(&concrete_self->child_vertices);

    current_coverage = bsal_assembly_vertex_coverage_depth(&concrete_self->current_vertex);

    best_coverage = -1;

    for (i = 0; i < size; i++) {
        code = bsal_assembly_vertex_get_child(&concrete_self->current_vertex, i);

        nucleotide = bsal_dna_codec_get_nucleotide_from_code(code);
        vertex = bsal_vector_at(&concrete_self->child_vertices, i);
        kmer = bsal_vector_at(&concrete_self->child_kmers, i);

        bsal_dna_kmer_pack(kmer, key, concrete_self->kmer_length,
                        &concrete_self->codec);

        found = bsal_set_find(&concrete_self->visited, key);

#ifdef BSAL_ASSEMBLY_DUMMY_WALKER_DEBUG
        printf("Find result %d\n", found);

        bsal_dna_kmer_print(kmer, concrete_self->kmer_length, &concrete_self->codec,
                        ephemeral_memory);
#endif

        if (found) {
            continue;
        }

        coverage = bsal_assembly_vertex_coverage_depth(vertex);

#ifdef BSAL_ASSEMBLY_DUMMY_WALKER_DEBUG
        printf("#%d is %c with coverage %d\n", i, nucleotide,
                        coverage);
#endif

        if (best_coverage < 0) {
            choice = i;
            best_coverage = coverage;
        }

        best_difference = best_coverage - current_coverage;
        if (best_difference < 0) {
            best_difference = -best_difference;
        }

        difference = coverage - current_coverage;

        if (difference < 0) {
            difference = -difference;
        }

        if (difference < best_difference) {
            choice = i;
            best_coverage = coverage;
        }
    }

#ifdef BSAL_ASSEMBLY_DUMMY_WALKER_DEBUG
    printf("Choice is %d\n", choice);
#endif

    /*
     * Avoid an infinite loop since this select function is just
     * a test.
     */
    if (bsal_vector_size(&concrete_self->path) >= 100000) {

        choice = -1;
    }

    bsal_memory_pool_free(ephemeral_memory, key);

    return choice;
}

void bsal_assembly_dummy_walker_write(struct bsal_actor *self, char *sequence,
                int sequence_length)
{
    struct bsal_assembly_dummy_walker *concrete_self;
    int column_width;
    char *buffer;
    struct bsal_memory_pool *ephemeral_memory;
    int block_length;
    int i;

    /*
     * \see http://en.wikipedia.org/wiki/FASTA_format
     */

    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    column_width = 80;
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    buffer = bsal_memory_pool_allocate(ephemeral_memory, column_width + 1);

    i = 0;

#if 0
    printf("LENGTH=%d\n", sequence_length);
#endif

    bsal_buffered_file_writer_printf(&concrete_self->writer,
                    ">%s-%d-%d length=%d\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    concrete_self->path_index,
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
