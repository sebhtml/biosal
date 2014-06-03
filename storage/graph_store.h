
#ifndef _BSAL_GRAPH_STORE_H
#define _BSAL_GRAPH_STORE_H

#include <engine/actor.h>

#define BSAL_GRAPH_STORE_SCRIPT 0xc81a1596

struct bsal_graph_store {
    int foo;
};

enum {
    BSAL_VERTEX = BSAL_TAG_OFFSET_GRAPH_STORE, /* +0 */
    BSAL_VERTEX_REPLY,
    BSAL_GRAPH_STORE_STOP
};

extern struct bsal_script bsal_graph_store_script;

void bsal_graph_store_init(struct bsal_actor *actor);
void bsal_graph_store_destroy(struct bsal_actor *actor);
void bsal_graph_store_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
