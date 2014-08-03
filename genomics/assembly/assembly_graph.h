
#ifndef _BIOSAL_ASSEMBLY_GRAPH_H_
#define _BIOSAL_ASSEMBLY_GRAPH_H_

#include <biosal.h>

#define BSAL_ASSEMBLY_GRAPH_SCRIPT 0x8272a301

/*
 * This actor controls the assembly graph.
 */
struct bsal_assembly_graph {
    int mock;
};

extern struct bsal_script bsal_assembly_graph_script;

void bsal_assembly_graph_init(struct bsal_actor *self);
void bsal_assembly_graph_destroy(struct bsal_actor *self);
void bsal_assembly_graph_receive(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_graph_ask_to_stop(struct bsal_actor *self, struct bsal_message *message);

#endif
