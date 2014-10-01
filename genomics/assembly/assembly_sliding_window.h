
#ifndef BIOSAL_ASSEMBLY_SLIDING_WINDOW_H
#define BIOSAL_ASSEMBLY_SLIDING_WINDOW_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>

#include <core/structures/vector.h>
#include <core/structures/fast_queue.h>
#include <core/structures/map.h>

#include <core/system/memory_pool.h>

#define SCRIPT_ASSEMBLY_SLIDING_WINDOW 0xa128805e

/*
 * A kernel for computing vertices from sequence reads.
 */
struct biosal_assembly_sliding_window {
    struct biosal_dna_codec codec;
    uint64_t expected;
    uint64_t actual;
    uint64_t last;

    uint64_t kmers;
    int blocks;
    int consumer;
    int producer;
    struct biosal_fast_queue producers_for_work_stealing;
    int kmer_length;

    int producer_source;

    int scaling_operations;

    int notified;
    int notification_source;

    int notified_children;
    uint64_t sum_of_kmers;

    struct biosal_vector kernels;

    int bytes_per_kmer;

    /*
     * Auto-scaling stuff
     */
    int auto_scaling_in_progress;
    int auto_scaling_clone;

    struct biosal_vector children;

    int flushed_payloads;
};

extern struct thorium_script biosal_assembly_sliding_window_script;

void biosal_assembly_sliding_window_init(struct thorium_actor *actor);
void biosal_assembly_sliding_window_destroy(struct thorium_actor *actor);
void biosal_assembly_sliding_window_receive(struct thorium_actor *actor, struct thorium_message *message);

void biosal_assembly_sliding_window_verify(struct thorium_actor *actor, struct thorium_message *message);
void biosal_assembly_sliding_window_ask(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_sliding_window_do_auto_scaling(struct thorium_actor *self, struct thorium_message *message);

void biosal_assembly_sliding_window_pack_message(struct thorium_actor *actor, struct thorium_message *message);
void biosal_assembly_sliding_window_unpack_message(struct thorium_actor *actor, struct thorium_message *message);
void biosal_assembly_sliding_window_clone_reply(struct thorium_actor *actor, struct thorium_message *message);

int biosal_assembly_sliding_window_pack(struct thorium_actor *actor, void *buffer);
int biosal_assembly_sliding_window_unpack(struct thorium_actor *actor, void *buffer);
int biosal_assembly_sliding_window_pack_size(struct thorium_actor *actor);
int biosal_assembly_sliding_window_pack_unpack(struct thorium_actor *actor, int operation, void *buffer);

void biosal_assembly_sliding_window_notify(struct thorium_actor *actor, struct thorium_message *message);
void biosal_assembly_sliding_window_notify_reply(struct thorium_actor *actor, struct thorium_message *message);
void biosal_assembly_sliding_window_push_sequence_data_block(struct thorium_actor *actor, struct thorium_message *message);

void biosal_assembly_sliding_window_set_producers_for_work_stealing(struct thorium_actor *self, struct thorium_message *message);

#endif
