
#ifndef _BSAL_INPUT_ACTOR_H
#define _BSAL_INPUT_ACTOR_H

#include <engine/actor.h>

struct bsal_input_actor {
    char *file_name;
    int supervisor;
};

enum {
    BSAL_INPUT_ACTOR_OPEN = BSAL_OFFSET_BSAL_INPUT_ACTOR,
    BSAL_INPUT_ACTOR_OPEN_OK,
    BSAL_INPUT_ACTOR_COUNT,
    BSAL_INPUT_ACTOR_COUNT_YIELD,
    BSAL_INPUT_ACTOR_COUNT_CONTINUE,
    BSAL_INPUT_ACTOR_COUNT_RESULT,
    BSAL_INPUT_ACTOR_CLOSE
};

struct bsal_actor_vtable bsal_input_actor_vtable;

void bsal_input_actor_init(struct bsal_actor *actor);
void bsal_input_actor_destroy(struct bsal_actor *actor);
void bsal_input_actor_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
