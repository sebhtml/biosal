
#ifndef BSAL_ASSEMBLY_SLIDING_WINDOW_H
#define BSAL_ASSEMBLY_SLIDING_WINDOW_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>
#include <core/structures/vector.h>
#include <core/structures/dynamic_hash_table.h>

#include <core/system/memory_pool.h>

#define BSAL_ASSEMBLY_SLIDING_WINDOW_SCRIPT 0xa128805e

struct bsal_assembly_sliding_window {
    struct bsal_dna_codec codec;
    uint64_t expected;
    uint64_t actual;
    uint64_t last;

    uint64_t kmers;
    int blocks;
    int consumer;
    int producer;
    int kmer_length;

    int producer_source;

    int scaling_operations;

    int notified;
    int notification_source;

    int notified_children;
    uint64_t sum_of_kmers;

    struct bsal_vector kernels;

    int bytes_per_kmer;

    /*
     * Auto-scaling stuff
     */
    int auto_scaling_in_progress;
    int auto_scaling_clone;

    struct bsal_vector children;
};

extern struct bsal_script bsal_assembly_sliding_window_script;

void bsal_assembly_sliding_window_init(struct bsal_actor *actor);
void bsal_assembly_sliding_window_destroy(struct bsal_actor *actor);
void bsal_assembly_sliding_window_receive(struct bsal_actor *actor, struct bsal_message *message);

void bsal_assembly_sliding_window_verify(struct bsal_actor *actor, struct bsal_message *message);
void bsal_assembly_sliding_window_ask(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_sliding_window_do_auto_scaling(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_sliding_window_pack_message(struct bsal_actor *actor, struct bsal_message *message);
void bsal_assembly_sliding_window_unpack_message(struct bsal_actor *actor, struct bsal_message *message);
void bsal_assembly_sliding_window_clone_reply(struct bsal_actor *actor, struct bsal_message *message);

int bsal_assembly_sliding_window_pack(struct bsal_actor *actor, void *buffer);
int bsal_assembly_sliding_window_unpack(struct bsal_actor *actor, void *buffer);
int bsal_assembly_sliding_window_pack_size(struct bsal_actor *actor);
int bsal_assembly_sliding_window_pack_unpack(struct bsal_actor *actor, int operation, void *buffer);

void bsal_assembly_sliding_window_notify(struct bsal_actor *actor, struct bsal_message *message);
void bsal_assembly_sliding_window_notify_reply(struct bsal_actor *actor, struct bsal_message *message);
void bsal_assembly_sliding_window_push_sequence_data_block(struct bsal_actor *actor, struct bsal_message *message);

#endif
