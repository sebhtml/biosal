
#ifndef BSAL_GRAPH_STORE_H
#define BSAL_GRAPH_STORE_H

#include <engine/actor.h>

#define BSAL_GRAPH_STORE_SCRIPT 0xc81a1596

struct bsal_graph_store {
    int foo;
};

#define BSAL_VERTEX 0x000027bf
#define BSAL_VERTEX_REPLY 0x0000733a
#define BSAL_GRAPH_STORE_STOP 0x00006b59

extern struct bsal_script bsal_graph_store_script;

void bsal_graph_store_init(struct bsal_actor *actor);
void bsal_graph_store_destroy(struct bsal_actor *actor);
void bsal_graph_store_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
