
#ifndef ARGONNITE_H
#define ARGONNITE_H

#include <biosal.h>

#define SCRIPT_ARGONNITE 0x97e07af7

struct argonnite {
    struct core_vector initial_actors;
    struct core_vector aggregators;
    struct core_vector kernels;
    struct core_vector kmer_stores;
    struct core_vector sequence_stores;

    struct core_timer timer;
    struct core_timer timer_for_kmers;
    struct core_map plentiful_stores;

    struct core_vector worker_counts;

    int is_boss;
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

extern struct thorium_script argonnite_script;

#define ACTION_ARGONNITE_PROBE_KMER_STORES 0x00001ba7
#define ACTION_ARGONNITE_PREPARE_SEQUENCE_STORES 0x00003264

void argonnite_init(struct thorium_actor *actor);
void argonnite_destroy(struct thorium_actor *actor);
void argonnite_receive(struct thorium_actor *actor, struct thorium_message *message);

void argonnite_add_file(struct thorium_actor *actor, struct thorium_message *message);
void argonnite_help(struct thorium_actor *actor);

void argonnite_prepare_sequence_stores_reply(struct thorium_actor *self, struct thorium_message *message);
void argonnite_prepare_sequence_stores(struct thorium_actor *self, struct thorium_message *message);
void argonnite_connect_kernels_with_stores(struct thorium_actor *self, struct thorium_message *message);
void argonnite_request_progress_reply(struct thorium_actor *actor, struct thorium_message *message);

#endif
