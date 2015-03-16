
#ifndef BIOSAL_TIP_DETECTOR_H
#define BIOSAL_TIP_DETECTOR_H

#define SCRIPT_TIP_DETECTOR 0x7d51e0f5

/*
 * Detect tips and discard them.
 *
 * ACTION_START - contains the name of the graph manager.
 */
struct biosal_tip_detector {
    int foo;
    int graph_manager_name;
};

extern struct thorium_script biosal_tip_detector_script;

#endif
