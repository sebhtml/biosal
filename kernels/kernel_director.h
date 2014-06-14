
#ifndef BSAL_KERNEL_DIRECTOR_H
#define BSAL_KERNEL_DIRECTOR_H

#include <engine/actor.h>

#include <structures/queue.h>
#include <structures/dynamic_hash_table.h>
#include <structures/vector.h>

#define BSAL_KERNEL_DIRECTOR_SCRIPT 0x7e627e1c

struct bsal_kernel_director {
    struct bsal_queue available_kernels;
    struct bsal_vector kernels;
    int aggregator;
    int kmer_length;
    int maximum_kernels;
    int received;
};

#define BSAL_SPAWN_KERNEL 0x0000125c
#define BSAL_SPAWN_KERNEL_REPLY 0x00002c77

extern struct bsal_script bsal_kernel_director_script;

void bsal_kernel_director_init(struct bsal_actor *actor);
void bsal_kernel_director_destroy(struct bsal_actor *actor);
void bsal_kernel_director_receive(struct bsal_actor *actor, struct bsal_message *message);

void bsal_kernel_director_try_kernel(struct bsal_actor *actor, int kernel);

#endif
