
#include "engine/bsal_actor.h"
#include "engine/bsal_message.h"
#include "engine/bsal_node.h"

struct bsal_biosal {
    int version;
};

int bsal_biosal_init(struct bsal_biosal *biosal, int nodes, int cores_per_node);
int bsal_biosal_destroy(struct bsal_biosal *biosal);

