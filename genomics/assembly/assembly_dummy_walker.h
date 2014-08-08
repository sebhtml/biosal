
#ifndef BSAL_ASSEMBLY_DUMMY_WALKER_H
#define BSAL_ASSEMBLY_DUMMY_WALKER_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>

#define BSAL_ASSEMBLY_DUMMY_WALKER_SCRIPT 0x78390b2f

/*
 * A dummy walker to test the concept.
 */
struct bsal_assembly_dummy_walker {
    struct bsal_vector graph_stores;
    int kmer_length;
    struct bsal_dna_codec codec;
};

extern struct bsal_script bsal_assembly_dummy_walker_script;

void bsal_assembly_dummy_walker_init(struct bsal_actor *self);
void bsal_assembly_dummy_walker_destroy(struct bsal_actor *self);
void bsal_assembly_dummy_walker_receive(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_dummy_walker_get_starting_vertex_reply(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_dummy_walker_start(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_dummy_walker_get_vertex_reply(struct bsal_actor *self, struct bsal_message *message);

#endif
