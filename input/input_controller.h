
#ifndef _BSAL_INPUT_CONTROLLER_H
#define _BSAL_INPUT_CONTROLLER_H

#include <engine/actor.h>

#define BSAL_INPUT_CONTROLLER_SCRIPT 0x985607aa

struct bsal_input_controller {
    int foo;
};

extern struct bsal_script bsal_input_controller_script;

void bsal_input_controller_init(struct bsal_actor *actor);
void bsal_input_controller_destroy(struct bsal_actor *actor);
void bsal_input_controller_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
