
#ifndef BSAL_UNITIG_MANAGER_H
#define BSAL_UNITIG_MANAGER_H

#include <engine/thorium/actor.h>

#include <core/system/timer.h>

#define SCRIPT_UNITIG_MANAGER 0x3bf29ca1

/*
 * A manager for unitig walkers
 */
struct bsal_unitig_manager {
    struct bsal_vector spawners;
    struct bsal_vector graph_stores;
    struct bsal_vector visitors;
    struct bsal_vector walkers;

    int completed;
    int manager;

    struct bsal_timer timer;
    int state;

    int writer_process;
};

extern struct thorium_script bsal_unitig_manager_script;

void bsal_unitig_manager_init(struct thorium_actor *self);
void bsal_unitig_manager_destroy(struct thorium_actor *self);
void bsal_unitig_manager_receive(struct thorium_actor *self, struct thorium_message *message);

#endif
