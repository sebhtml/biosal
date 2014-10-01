
#ifndef _SPATE_H_
#define _SPATE_H_

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

void spate_init(struct thorium_actor *self);
void spate_destroy(struct thorium_actor *self);
void spate_receive(struct thorium_actor *self, struct thorium_message *message);

void spate_start(struct thorium_actor *self, struct thorium_message *message);
void spate_ask_to_stop(struct thorium_actor *self, struct thorium_message *message);
void spate_spawn_reply(struct thorium_actor *self, struct thorium_message *message);

void spate_set_consumers_reply(struct thorium_actor *self, struct thorium_message *message);
void spate_start_reply(struct thorium_actor *self, struct thorium_message *message);
void spate_set_script_reply(struct thorium_actor *self, struct thorium_message *message);
void spate_start_reply_manager(struct thorium_actor *self, struct thorium_message *message);
void spate_start_reply_controller(struct thorium_actor *self, struct thorium_message *message);
void spate_distribute_reply(struct thorium_actor *self, struct thorium_message *message);
void spate_set_block_size_reply(struct thorium_actor *self, struct thorium_message *message);

void spate_add_files(struct thorium_actor *self, struct thorium_message *message);
void spate_add_files_reply(struct thorium_actor *self, struct thorium_message *message);

int spate_add_file(struct thorium_actor *self);
void spate_add_file_reply(struct thorium_actor *self, struct thorium_message *message);
void spate_start_reply_builder(struct thorium_actor *self, struct thorium_message *message);
void spate_set_producers_reply(struct thorium_actor *self, struct thorium_message *message);

void spate_help(struct thorium_actor *self);
void spate_stop(struct thorium_actor *self);
int spate_must_print_help(struct thorium_actor *self);

void spate_spawn_reply_unitig_manager(struct thorium_actor *self, struct thorium_message *message);
void spate_start_reply_unitig_manager(struct thorium_actor *self, struct thorium_message *message);
void spate_set_producers_reply_reply_unitig_manager(struct thorium_actor *self, struct thorium_message *message);

#endif
