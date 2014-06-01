
#ifndef _BSAL_H
#define _BSAL_H

#include "engine/actor.h"
#include "engine/message.h"
#include "engine/node.h"

#include "input/input_actor.h"

struct bsal_biosal {
    int version;
};

int bsal_biosal_init(struct bsal_biosal *biosal, int *argc, char ***argv);
int bsal_biosal_destroy(struct bsal_biosal *biosal);

#endif
