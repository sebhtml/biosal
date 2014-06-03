
#ifndef _BSAL_INPUT_CONTROLLER_H
#define _BSAL_INPUT_CONTROLLER_H

#include <engine/actor.h>

#define BSAL_INPUT_CONTROLLER_SCRIPT 0x985607aa

struct bsal_input_controller {
    int foo;
};

enum {
    BSAL_INPUT_DISTRIBUTE = BSAL_TAG_OFFSET_INPUT_CONTROLLER,
    BSAL_INPUT_STOP,
    BSAL_INPUT_CONTROLLER_START,
    BSAL_INPUT_CONTROLLER_START_REPLY,
    BSAL_ADD_FILE,
    BSAL_ADD_FILE_REPLY,
    BSAL_INPUT_DISTRIBUTE_REPLY
};

extern struct bsal_script bsal_input_controller_script;

void bsal_input_controller_init(struct bsal_actor *actor);
void bsal_input_controller_destroy(struct bsal_actor *actor);
void bsal_input_controller_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
