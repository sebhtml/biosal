
#ifndef _BSAL_INPUT_CONTROLLER_H
#define _BSAL_INPUT_CONTROLLER_H

#include <engine/actor.h>
#include <structures/vector.h>

#define BSAL_INPUT_CONTROLLER_SCRIPT 0x985607aa

struct bsal_input_controller {
    struct bsal_vector files;
    struct bsal_vector streams;
};

#define BSAL_INPUT_DISTRIBUTE 0x00003cbe
#define BSAL_INPUT_STOP 0x00004107
#define BSAL_INPUT_CONTROLLER_START 0x000000d0
#define BSAL_INPUT_CONTROLLER_START_REPLY 0x00004e66
#define BSAL_ADD_FILE 0x00007deb
#define BSAL_ADD_FILE_REPLY 0x00007036
#define BSAL_INPUT_DISTRIBUTE_REPLY 0x00001ab0
#define BSAL_INPUT_SPAWN 0x00000a5d

extern struct bsal_script bsal_input_controller_script;

void bsal_input_controller_init(struct bsal_actor *actor);
void bsal_input_controller_destroy(struct bsal_actor *actor);
void bsal_input_controller_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
