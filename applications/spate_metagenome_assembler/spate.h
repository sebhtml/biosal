
#ifndef BIOSAL_SPATE_H_
#define BIOSAL_SPATE_H_

#include <biosal.h>

#define SCRIPT_SPATE 0xaefe198b

#define ACTION_SPATE_ADD_FILES 0x00004eff
#define ACTION_SPATE_ADD_FILES_REPLY 0x00001b3a

/*
 * A really cool metagenome assembler.
 *
 * The design have mainly 4 goals:
 *
 * 1. Accuracy
 * 2. Scalability
 * 3. Convenience
 * 4. Clean source code
 *
 * All this is to enable nice science.
 */
struct spate {
    struct core_vector initial_actors;
    struct core_vector graph_stores;

    int unitig_manager;

    int is_leader;
    struct core_timer timer;

    /*
     * Children
     */
    int input_controller;
    int manager_for_sequence_stores;
    int assembly_graph_builder;
    int assembly_graph;

    /*
     * Block size for distribution
     */
    int block_size;

    int file_index;

    struct core_vector sequence_stores;
};

extern struct thorium_script spate_script;

#endif
