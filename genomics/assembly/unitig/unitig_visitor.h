
#ifndef BSAL_UNITIG_VISITOR_H
#define BSAL_UNITIG_VISITOR_H

#include <engine/thorium/actor.h>

#include <core/system/timer.h>

#define SCRIPT_UNITIG_VISITOR 0xeb3b39fd

/*
 * A visitor for unitigs.
 */
struct bsal_unitig_visitor {
    struct bsal_vector spawners;
    struct bsal_vector graph_stores;
    struct bsal_vector walkers;
    int completed;
    int visitor;
    struct bsal_timer timer;
};

extern struct thorium_script bsal_unitig_visitor_script;

void bsal_unitig_visitor_init(struct thorium_actor *self);
void bsal_unitig_visitor_destroy(struct thorium_actor *self);
void bsal_unitig_visitor_receive(struct thorium_actor *self, struct thorium_message *message);

#endif
