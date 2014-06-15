
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

    int controller;
    int manager_for_directors;
    int manager_for_aggregators;
    int manager_for_stores;
    int argument_iterator;

    int block_size;
    int kmer_length;
    int configured_actors;
    int configured_aggregators;
};

extern struct bsal_script argonnite_script;

void argonnite_init(struct bsal_actor *actor);
void argonnite_destroy(struct bsal_actor *actor);
void argonnite_receive(struct bsal_actor *actor, struct bsal_message *message);

void argonnite_add_file(struct bsal_actor *actor, struct bsal_message *message);

#endif
