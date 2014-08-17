
#ifndef BSAL_ASSEMBLY_BLOCK_CLASSIFIER_H
#define BSAL_ASSEMBLY_BLOCK_CLASSIFIER_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>
#include <genomics/kernels/aggregator.h>

#include <core/structures/vector.h>
#include <core/structures/ring_queue.h>

#include <core/system/memory_pool.h>

#include <stdint.h>

#define BSAL_ASSEMBLY_BLOCK_CLASSIFIER_SCRIPT 0x8f6f8767

/*
 * This is a classifier that dispatch a vertex block to the
 * graph stores.
 */
struct bsal_assembly_block_classifier {
    uint64_t received;
    uint64_t last;
    int kmer_length;

    struct bsal_vector consumers;

    struct bsal_ring_queue stalled_producers;

    int maximum_active_messages;
    struct bsal_vector active_messages;

    int customer_block_size;
    int flushed;

    int forced;

    struct bsal_dna_codec codec;

    int consumer_count_above_threshold;
};


extern struct thorium_script bsal_assembly_block_classifier_script;

void bsal_assembly_block_classifier_init(struct thorium_actor *actor);
void bsal_assembly_block_classifier_destroy(struct thorium_actor *actor);
void bsal_assembly_block_classifier_receive(struct thorium_actor *actor, struct thorium_message *message);

void bsal_assembly_block_classifier_flush(struct thorium_actor *self, int customer_index, struct bsal_vector *buffers,
                int force);
void bsal_assembly_block_classifier_verify(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_block_classifier_aggregate_kernel_output(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_block_classifier_unpack_message(struct thorium_actor *actor, struct thorium_message *message);
void bsal_assembly_block_classifier_pack_message(struct thorium_actor *actor, struct thorium_message *message);
int bsal_assembly_block_classifier_set_consumers(struct thorium_actor *actor, void *buffer);

int bsal_assembly_block_classifier_pack_unpack(struct thorium_actor *actor, int operation, void *buffer);
int bsal_assembly_block_classifier_pack(struct thorium_actor *actor, void *buffer);
int bsal_assembly_block_classifier_unpack(struct thorium_actor *actor, void *buffer);
int bsal_assembly_block_classifier_pack_size(struct thorium_actor *actor);

#endif
