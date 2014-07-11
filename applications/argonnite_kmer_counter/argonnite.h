
#ifndef ARGONNITE_H
#define ARGONNITE_H

#include <biosal.h>

#define ARGONNITE_SCRIPT 0x97e07af7

struct argonnite {
    struct bsal_vector initial_actors;
    struct bsal_vector aggregators;
    struct bsal_vector kernels;
    struct bsal_vector kmer_stores;
    struct bsal_vector sequence_stores;

    struct bsal_map plentiful_stores;

    struct bsal_vector worker_counts;

    int wired_kernels;
    int ready_kernels;
    int ready_stores;
    int finished_kernels;

    int controller;
    int manager_for_kernels;
    int manager_for_aggregators;
    int manager_for_kmer_stores;
    int manager_for_sequence_stores;
    int argument_iterator;

    int block_size;
    int kmer_length;
    int configured_actors;
    int configured_aggregators;

    int spawned_stores;

    int distribution;
    int wiring_distribution;

    int state;
    uint64_t total_kmers;
    uint64_t actual_kmers;

    int not_ready_warnings;
};

extern struct bsal_script argonnite_script;

#define ARGONNITE_PROBE_KMER_STORES 0x00001ba7
#define ARGONNITE_PREPARE_SEQUENCE_STORES 0x00003264

void argonnite_init(struct bsal_actor *actor);
void argonnite_destroy(struct bsal_actor *actor);
void argonnite_receive(struct bsal_actor *actor, struct bsal_message *message);

void argonnite_add_file(struct bsal_actor *actor, struct bsal_message *message);
void argonnite_help(struct bsal_actor *actor);

void argonnite_prepare_sequence_stores_reply(struct bsal_actor *self, struct bsal_message *message);
void argonnite_prepare_sequence_stores(struct bsal_actor *self, struct bsal_message *message);
void argonnite_connect_kernels_with_stores(struct bsal_actor *self, struct bsal_message *message);
void argonnite_request_progress_reply(struct bsal_actor *actor, struct bsal_message *message);

#endif
