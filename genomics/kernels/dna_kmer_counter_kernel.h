
#ifndef BIOSAL_DNA_KMER_COUNTER_KERNEL_H
#define BIOSAL_DNA_KMER_COUNTER_KERNEL_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>
#include <core/structures/vector.h>
#include <core/structures/map.h>

#include <core/system/memory_pool.h>

#define SCRIPT_DNA_KMER_COUNTER_KERNEL 0xed338fa2

struct biosal_dna_kmer_counter_kernel {
    struct biosal_dna_codec codec;
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

    struct core_vector kernels;

    int bytes_per_kmer;

    /*
     * Auto-scaling stuff
     */
    int auto_scaling_in_progress;
    int auto_scaling_clone;

    struct core_vector children;
};

#define ACTION_SET_KMER_LENGTH 0x0000702b
#define ACTION_SET_KMER_LENGTH_REPLY 0x00005162

extern struct thorium_script biosal_dna_kmer_counter_kernel_script;

void biosal_dna_kmer_counter_kernel_init(struct thorium_actor *actor);
void biosal_dna_kmer_counter_kernel_destroy(struct thorium_actor *actor);
void biosal_dna_kmer_counter_kernel_receive(struct thorium_actor *actor, struct thorium_message *message);

void biosal_dna_kmer_counter_kernel_verify(struct thorium_actor *actor, struct thorium_message *message);
void biosal_dna_kmer_counter_kernel_ask(struct thorium_actor *self, struct thorium_message *message);

void biosal_dna_kmer_counter_kernel_do_auto_scaling(struct thorium_actor *self, struct thorium_message *message);

void biosal_dna_kmer_counter_kernel_pack_message(struct thorium_actor *actor, struct thorium_message *message);
void biosal_dna_kmer_counter_kernel_unpack_message(struct thorium_actor *actor, struct thorium_message *message);
void biosal_dna_kmer_counter_kernel_clone_reply(struct thorium_actor *actor, struct thorium_message *message);

int biosal_dna_kmer_counter_kernel_pack(struct thorium_actor *actor, void *buffer);
int biosal_dna_kmer_counter_kernel_unpack(struct thorium_actor *actor, void *buffer);
int biosal_dna_kmer_counter_kernel_pack_size(struct thorium_actor *actor);
int biosal_dna_kmer_counter_kernel_pack_unpack(struct thorium_actor *actor, int operation, void *buffer);

void biosal_dna_kmer_counter_kernel_notify(struct thorium_actor *actor, struct thorium_message *message);
void biosal_dna_kmer_counter_kernel_notify_reply(struct thorium_actor *actor, struct thorium_message *message);
void biosal_dna_kmer_counter_kernel_push_sequence_data_block(struct thorium_actor *actor, struct thorium_message *message);

#endif
