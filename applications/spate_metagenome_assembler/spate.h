
#ifndef _SPATE_H_
#define _SPATE_H_

#include <biosal.h>

#define SPATE_SCRIPT 0xaefe198b

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

    /*
     * The inder of the initial actor to use
     * for spawning new colleagues in the company.
     */
    int spawner_index;

    /*
     * Children
     */
    int input_controller;
    int manager_for_sequence_stores;
    int assembly_graph_builder;
    int assembly_graph;
};

extern struct bsal_script spate_script;

void spate_init(struct bsal_actor *self);
void spate_destroy(struct bsal_actor *self);
void spate_receive(struct bsal_actor *self, struct bsal_message *message);

void spate_start(struct bsal_actor *self, struct bsal_message *message);
void spate_ask_to_stop(struct bsal_actor *self, struct bsal_message *message);
void spate_spawn_reply(struct bsal_actor *self, struct bsal_message *message);

int spate_get_spawner(struct bsal_actor *self);
void spate_set_consumers_reply(struct bsal_actor *self, struct bsal_message *message);
void spate_start_reply(struct bsal_actor *self, struct bsal_message *message);
void spate_set_script_reply(struct bsal_actor *self, struct bsal_message *message);

#endif
