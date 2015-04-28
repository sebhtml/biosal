
#ifndef BIOSAL_TIP_MANAGER_H
#define BIOSAL_TIP_MANAGER_H

#include <core/structures/vector.h>

#define SCRIPT_TIP_MANAGER 0xa2661578

/**
 * A tip manager is responsible for removing tips in the graph.
 */
struct biosal_tip_manager {
    int graph_manager_name;

    int __supervisor;
    int origin_message;

    int done;

    int tip_detector_manager;
    struct core_vector tip_detectors;
    struct core_vector graph_stores;

    struct core_vector spawners;
};

extern struct thorium_script biosal_tip_manager_script;

#endif
