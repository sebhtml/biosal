
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
    int tag;
    char *file;
    void *buffer;

    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);

    if (tag == BSAL_INPUT_CONTROLLER_START) {

        bsal_actor_send_reply_empty(actor, BSAL_INPUT_CONTROLLER_START_REPLY);
    } else if (tag == BSAL_ADD_FILE) {

        file = (char *)buffer;
        file[0]=0;

        bsal_actor_send_reply_empty(actor, BSAL_ADD_FILE_REPLY);

    } else if (tag == BSAL_INPUT_DISTRIBUTE) {

        /* for each file, spawn a stream to count */
        /* also, spawn 4 stores on each node */

        bsal_actor_send_reply_empty(actor, BSAL_INPUT_DISTRIBUTE_REPLY);

    } else if (tag == BSAL_INPUT_STOP) {

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}
