
#ifndef BSAL_KMER_COUNTER_KERNEL_H
#define BSAL_KMER_COUNTER_KERNEL_H

#include <engine/actor.h>

#include <structures/vector.h>
#include <structures/dynamic_hash_table.h>

#define BSAL_KMER_COUNTER_KERNEL_SCRIPT 0xed338fa2

struct bsal_kmer_counter_kernel {
    uint64_t expected;
    uint64_t actual;
    uint64_t last;

    uint64_t kmers;
    int blocks;
    int customer;
    int kmer_length;

    int notified;
    int notification_source;
    int bytes_per_kmer;
};

#define BSAL_SET_KMER_LENGTH 0x0000702b
#define BSAL_SET_KMER_LENGTH_REPLY 0x00005162
#define BSAL_KERNEL_NOTIFY 0x00005098
#define BSAL_KERNEL_NOTIFY_REPLY 0x00001d2b

extern struct bsal_script bsal_kmer_counter_kernel_script;

void bsal_kmer_counter_kernel_init(struct bsal_actor *actor);
void bsal_kmer_counter_kernel_destroy(struct bsal_actor *actor);
void bsal_kmer_counter_kernel_receive(struct bsal_actor *actor, struct bsal_message *message);

void bsal_kmer_counter_kernel_verify(struct bsal_actor *actor, struct bsal_message *message);

#endif
