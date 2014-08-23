
#ifndef BSAL_ASSEMBLY_DUMMY_WALKER_H
#define BSAL_ASSEMBLY_DUMMY_WALKER_H

#include <engine/thorium/actor.h>

#include <core/file_storage/output/buffered_file_writer.h>

#include <genomics/data/dna_codec.h>
#include <genomics/data/dna_kmer.h>

#include <genomics/assembly/assembly_vertex.h>

#include <core/structures/string.h>

#include <core/system/memory_pool.h>

#define SCRIPT_ASSEMBLY_DUMMY_WALKER 0x78390b2f

#define ACTION_ASSEMBLY_GET_VERTICES_AND_SELECT 0x0000122a
#define ACTION_ASSEMBLY_GET_VERTICES_AND_SELECT_REPLY 0x00000a0b

/*
 * A dummy walker to test the concept.
 */
struct bsal_assembly_dummy_walker {
    struct bsal_vector graph_stores;
    int kmer_length;
    struct bsal_dna_codec codec;
    struct bsal_memory_pool memory_pool;

    struct bsal_buffered_file_writer writer;
    struct bsal_string file_path;

    int has_starting_vertex;

    struct bsal_dna_kmer starting_kmer;
    struct bsal_vector path;

    struct bsal_dna_kmer current_kmer;
    struct bsal_assembly_vertex current_vertex;
    int current_child;

    struct bsal_vector child_kmers;
    struct bsal_vector child_vertices;
    int path_index;

    int key_length;
    struct bsal_set visited;
};

extern struct thorium_script bsal_assembly_dummy_walker_script;

void bsal_assembly_dummy_walker_init(struct thorium_actor *self);
void bsal_assembly_dummy_walker_destroy(struct thorium_actor *self);
void bsal_assembly_dummy_walker_receive(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_dummy_walker_get_starting_vertex_reply(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_dummy_walker_start(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_dummy_walker_get_vertex_reply(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_dummy_walker_get_vertices_and_select(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_dummy_walker_get_vertices_and_select_reply(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_dummy_walker_get_vertex_reply_starting_vertex(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_dummy_walker_clean_children(struct thorium_actor *self);
void bsal_assembly_dummy_walker_dump_path(struct thorium_actor *self);
void bsal_assembly_dummy_walker_begin(struct thorium_actor *self, struct thorium_message *message);

int bsal_assembly_dummy_walker_select(struct thorium_actor *self);
void bsal_assembly_dummy_walker_write(struct thorium_actor *self, char *sequence, int sequence_length);

#endif
