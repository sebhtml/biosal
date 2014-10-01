
#ifndef BIOSAL_UNITIG_MANAGER_H
#define BIOSAL_UNITIG_MANAGER_H

#include <engine/thorium/actor.h>

#include <core/system/timer.h>

#define SCRIPT_UNITIG_MANAGER 0x3bf29ca1

/*
 * A manager for unitig walkers
 */
struct biosal_unitig_manager {
    struct biosal_vector spawners;
    struct biosal_vector graph_stores;
    struct biosal_vector visitors;
    struct biosal_vector walkers;

    int completed;
    int manager;

    struct biosal_timer timer;
    int state;

    int writer_process;
};

extern struct thorium_script biosal_unitig_manager_script;

void biosal_unitig_manager_init(struct thorium_actor *self);
void biosal_unitig_manager_destroy(struct thorium_actor *self);
void biosal_unitig_manager_receive(struct thorium_actor *self, struct thorium_message *message);

#endif
