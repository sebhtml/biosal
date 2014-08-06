
#ifndef BSAL_ASSEMBLY_ARC_KERNEL_H
#define BSAL_ASSEMBLY_ARC_KERNEL_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>
#include <core/structures/vector.h>

#include <core/system/memory_pool.h>

#define BSAL_ASSEMBLY_ARC_KERNEL_SCRIPT 0xe4c41672

/*
 * Arc generator for the assembly graph.
 */
struct bsal_assembly_arc_kernel {
    int kmer_length;

    int producer;

    int consumer;

    struct bsal_dna_codec codec;

    uint64_t produced_arcs;

    int source;

    int received_blocks;
};

extern struct bsal_script bsal_assembly_arc_kernel_script;

void bsal_assembly_arc_kernel_init(struct bsal_actor *self);
void bsal_assembly_arc_kernel_destroy(struct bsal_actor *self);
void bsal_assembly_arc_kernel_receive(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_arc_kernel_set_kmer_length(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_arc_kernel_push_sequence_data_block(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_arc_kernel_ask(struct bsal_actor *self, struct bsal_message *message);

#endif
