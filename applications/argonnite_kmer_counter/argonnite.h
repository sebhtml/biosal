
#ifndef ARGONNITE_H
#define ARGONNITE_H

#include <biosal.h>

#define ARGONNITE_SCRIPT 0x97e07af7

struct argonnite {
    struct bsal_vector initial_actors;
    int controller;
    int manager;
    int argument_iterator;
};

extern struct bsal_script argonnite_script;

void argonnite_init(struct bsal_actor *actor);
void argonnite_destroy(struct bsal_actor *actor);
void argonnite_receive(struct bsal_actor *actor, struct bsal_message *message);

void argonnite_add_file(struct bsal_actor *actor, struct bsal_message *message);

#endif
