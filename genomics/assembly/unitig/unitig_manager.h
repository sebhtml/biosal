
#ifndef BIOSAL_UNITIG_MANAGER_H
#define BIOSAL_UNITIG_MANAGER_H

#include <engine/thorium/actor.h>

#include <core/system/timer.h>

#include <stdint.h>

#define SCRIPT_UNITIG_MANAGER 0x3bf29ca1

/*
 * A manager for unitig walkers
 */
struct biosal_unitig_manager {
    struct core_vector spawners;
    struct core_vector graph_stores;
    struct core_vector visitors;
    struct core_vector walkers;

    int completed;
    int manager;

    struct core_timer timer;
    int state;

    int writer_process;

    uint64_t processed_vertices;
    uint64_t vertices_with_unitig_flag;
};

extern struct thorium_script biosal_unitig_manager_script;

#endif
