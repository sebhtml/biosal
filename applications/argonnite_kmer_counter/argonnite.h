
#ifndef ARGONNITE_H
#define ARGONNITE_H

#include <biosal.h>

#define ARGONNITE_SCRIPT 0x97e07af7

struct argonnite {
    struct bsal_vector initial_actors;
    struct bsal_vector aggregators;
    struct bsal_vector directors;
    struct bsal_vector stores;

    int wired_directors;
    int ready_directors;
    int ready_stores;

    int controller;
    int manager_for_directors;
    int manager_for_aggregators;
    int manager_for_stores;
    int argument_iterator;

    int block_size;
    int kmer_length;
    int configured_actors;
    int configured_aggregators;

    int spawned_stores;

    int distribution;
    int wiring_distribution;

    uint64_t total_kmers;
    uint64_t actual_kmers;
};

extern struct bsal_script argonnite_script;

#define ARGONNITE_PROBE_STORES 0x00001ba7

void argonnite_init(struct bsal_actor *actor);
void argonnite_destroy(struct bsal_actor *actor);
void argonnite_receive(struct bsal_actor *actor, struct bsal_message *message);

void argonnite_add_file(struct bsal_actor *actor, struct bsal_message *message);
void argonnite_help(struct bsal_actor *actor);

#endif
