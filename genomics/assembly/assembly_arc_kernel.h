
#ifndef BIOSAL_ASSEMBLY_ARC_KERNEL_H
#define BIOSAL_ASSEMBLY_ARC_KERNEL_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>

#include <core/structures/vector.h>
#include <core/structures/fast_queue.h>

#include <core/system/memory_pool.h>

#define SCRIPT_ASSEMBLY_ARC_KERNEL 0xe4c41672

#define ACTION_ASSEMBLY_PUSH_ARC_BLOCK 0x0000688b
#define ACTION_ASSEMBLY_PUSH_ARC_BLOCK_REPLY 0x00004c03

/*
 * Arc generator for the assembly graph.
 */
struct biosal_assembly_arc_kernel {
    int kmer_length;

    int producer;
    struct biosal_fast_queue producers_for_work_stealing;

    int consumer;

    struct biosal_dna_codec codec;

    uint64_t produced_arcs;

    int source;

    int received_blocks;
    int flushed_messages;
};

extern struct thorium_script biosal_assembly_arc_kernel_script;

void biosal_assembly_arc_kernel_init(struct thorium_actor *self);
void biosal_assembly_arc_kernel_destroy(struct thorium_actor *self);
void biosal_assembly_arc_kernel_receive(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_arc_kernel_set_kmer_length(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_arc_kernel_push_sequence_data_block(struct thorium_actor *self, struct thorium_message *message);
void biosal_assembly_arc_kernel_ask(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_arc_kernel_set_producers_for_work_stealing(struct thorium_actor *self, struct thorium_message *message);

#endif
