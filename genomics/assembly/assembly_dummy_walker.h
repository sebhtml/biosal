
#ifndef BSAL_ASSEMBLY_DUMMY_WALKER_H
#define BSAL_ASSEMBLY_DUMMY_WALKER_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>
#include <genomics/data/dna_kmer.h>

#include <genomics/assembly/assembly_vertex.h>

#include <core/system/memory_pool.h>

#define BSAL_ASSEMBLY_DUMMY_WALKER_SCRIPT 0x78390b2f

#define BSAL_ASSEMBLY_PUSH_AND_FETCH 0x0000122a
#define BSAL_ASSEMBLY_PUSH_AND_FETCH_REPLY 0x00000a0b

/*
 * A dummy walker to test the concept.
 */
struct bsal_assembly_dummy_walker {
    struct bsal_vector graph_stores;
    int kmer_length;
    struct bsal_dna_codec codec;
    struct bsal_memory_pool memory_pool;

    struct bsal_dna_kmer first_kmer;
    struct bsal_vector path;

    struct bsal_dna_kmer current_kmer;
    struct bsal_assembly_vertex current_vertex;
    int current_child;

    struct bsal_vector arc_kmers;
    struct bsal_vector arc_vertices;
};

extern struct bsal_script bsal_assembly_dummy_walker_script;

void bsal_assembly_dummy_walker_init(struct bsal_actor *self);
void bsal_assembly_dummy_walker_destroy(struct bsal_actor *self);
void bsal_assembly_dummy_walker_receive(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_dummy_walker_get_starting_vertex_reply(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_dummy_walker_start(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_dummy_walker_get_vertex_reply(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_dummy_walker_push_and_fetch_reply(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_dummy_walker_push_and_fetch(struct bsal_actor *self, struct bsal_message *message);

#endif
