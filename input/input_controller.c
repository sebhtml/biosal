
#include "input_controller.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_script bsal_input_controller_script = {
    .name = BSAL_INPUT_CONTROLLER_SCRIPT,
    .init = bsal_input_controller_init,
    .destroy = bsal_input_controller_destroy,
    .receive = bsal_input_controller_receive,
    .size = sizeof(struct bsal_input_controller)
};

void bsal_input_controller_init(struct bsal_actor *actor)
{
}

void bsal_input_controller_destroy(struct bsal_actor *actor)
{
}

void bsal_input_controller_receive(struct bsal_actor *actor, struct bsal_message *message)
{
}
