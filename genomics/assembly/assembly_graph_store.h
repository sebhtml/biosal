
#ifndef _BIOSAL_ASSEMBLY_GRAPH_STORE_H_
#define _BIOSAL_ASSEMBLY_GRAPH_STORE_H_

#include <biosal.h>

#define BSAL_ASSEMBLY_GRAPH_STORE_SCRIPT 0xc81a1596

struct bsal_assembly_graph_store {
    struct bsal_map map;
};

#define BSAL_VERTEX 0x000027bf
#define BSAL_VERTEX_REPLY 0x0000733a

extern struct bsal_script bsal_assembly_graph_store_script;

void bsal_assembly_graph_store_init(struct bsal_actor *self);
void bsal_assembly_graph_store_destroy(struct bsal_actor *self);
void bsal_assembly_graph_store_receive(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_graph_store_ask_to_stop(struct bsal_actor *self, struct bsal_message *message);

#endif
