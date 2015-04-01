
#ifndef BIOSAL_TIP_MANAGER_H
#define BIOSAL_TIP_MANAGER_H

#define SCRIPT_TIP_MANAGER 0xa2661578

struct biosal_tip_manager {
    int graph_manager_name;

    int __supervisor;
    int done;
};

extern struct thorium_script biosal_tip_manager_script;

#endif
