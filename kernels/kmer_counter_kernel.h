
#ifndef BSAL_KMER_COUNTER_KERNEL_H
#define BSAL_KMER_COUNTER_KERNEL_H

#include <engine/actor.h>

#include <structures/vector.h>

#define BSAL_KMER_COUNTER_KERNEL_SCRIPT 0xed338fa2

struct bsal_kmer_counter_kernel {
    struct bsal_vector customers;
};

extern struct bsal_script bsal_kmer_counter_kernel_script;

void bsal_kmer_counter_kernel_init(struct bsal_actor *actor);
void bsal_kmer_counter_kernel_destroy(struct bsal_actor *actor);
void bsal_kmer_counter_kernel_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
