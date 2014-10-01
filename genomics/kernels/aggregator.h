
#ifndef BIOSAL_AGGREGATOR_H
#define BIOSAL_AGGREGATOR_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>

#include <core/structures/vector.h>
#include <core/structures/fast_queue.h>

#include <core/system/memory_pool.h>

#include <stdint.h>

#define SCRIPT_AGGREGATOR 0x82673850

struct biosal_aggregator {
    uint64_t received;
    uint64_t last;
    int kmer_length;

    struct biosal_vector consumers;

    struct biosal_fast_queue stalled_producers;

    int maximum_active_messages;
    struct biosal_vector active_messages;

    int customer_block_size;
    int flushed;

    int forced;

    struct biosal_dna_codec codec;
};

/* message tags
 */
#define ACTION_AGGREGATE_KERNEL_OUTPUT 0x0000225f
#define ACTION_AGGREGATE_KERNEL_OUTPUT_REPLY 0x00005cf2
#define ACTION_AGGREGATOR_FLUSH 0x00007305
#define ACTION_AGGREGATOR_FLUSH_REPLY 0x000029fe

extern struct thorium_script biosal_aggregator_script;

void biosal_aggregator_init(struct thorium_actor *actor);
void biosal_aggregator_destroy(struct thorium_actor *actor);
void biosal_aggregator_receive(struct thorium_actor *actor, struct thorium_message *message);

void biosal_aggregator_flush(struct thorium_actor *self, int customer_index, struct biosal_vector *buffers,
                int force);
void biosal_aggregator_verify(struct thorium_actor *self, struct thorium_message *message);
void biosal_aggregator_aggregate_kernel_output(struct thorium_actor *self, struct thorium_message *message);

void biosal_aggregator_unpack_message(struct thorium_actor *actor, struct thorium_message *message);
void biosal_aggregator_pack_message(struct thorium_actor *actor, struct thorium_message *message);
int biosal_aggregator_set_consumers(struct thorium_actor *actor, void *buffer);

int biosal_aggregator_pack_unpack(struct thorium_actor *actor, int operation, void *buffer);
int biosal_aggregator_pack(struct thorium_actor *actor, void *buffer);
int biosal_aggregator_unpack(struct thorium_actor *actor, void *buffer);
int biosal_aggregator_pack_size(struct thorium_actor *actor);

#endif
