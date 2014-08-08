
#ifndef _SPATE_H_
#define _SPATE_H_

#include <biosal.h>

#define SPATE_SCRIPT 0xaefe198b

#define SPATE_ADD_FILES 0x00004eff
#define SPATE_ADD_FILES_REPLY 0x00001b3a

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
    struct bsal_vector initial_actors;

    int is_leader;
    struct bsal_timer timer;

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

    struct bsal_vector sequence_stores;
};

extern struct bsal_script spate_script;

void spate_init(struct bsal_actor *self);
void spate_destroy(struct bsal_actor *self);
void spate_receive(struct bsal_actor *self, struct bsal_message *message);

void spate_start(struct bsal_actor *self, struct bsal_message *message);
void spate_ask_to_stop(struct bsal_actor *self, struct bsal_message *message);
void spate_spawn_reply(struct bsal_actor *self, struct bsal_message *message);

void spate_set_consumers_reply(struct bsal_actor *self, struct bsal_message *message);
void spate_start_reply(struct bsal_actor *self, struct bsal_message *message);
void spate_set_script_reply(struct bsal_actor *self, struct bsal_message *message);
void spate_start_reply_manager(struct bsal_actor *self, struct bsal_message *message);
void spate_start_reply_controller(struct bsal_actor *self, struct bsal_message *message);
void spate_distribute_reply(struct bsal_actor *self, struct bsal_message *message);
void spate_set_block_size_reply(struct bsal_actor *self, struct bsal_message *message);

void spate_add_files(struct bsal_actor *self, struct bsal_message *message);
void spate_add_files_reply(struct bsal_actor *self, struct bsal_message *message);

int spate_add_file(struct bsal_actor *self);
void spate_add_file_reply(struct bsal_actor *self, struct bsal_message *message);
void spate_start_reply_builder(struct bsal_actor *self, struct bsal_message *message);
void spate_set_producers_reply(struct bsal_actor *self, struct bsal_message *message);

void spate_help(struct bsal_actor *self);
void spate_stop(struct bsal_actor *self);
int spate_must_print_help(struct bsal_actor *self);

#endif
