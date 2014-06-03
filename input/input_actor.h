
#ifndef _BSAL_INPUT_ACTOR_H
#define _BSAL_INPUT_ACTOR_H

#include "input_proxy.h"

#include <engine/actor.h>

#define BSAL_INPUT_ACTOR_SCRIPT 0xeb2fe16a

struct bsal_input_actor {
    struct bsal_input_proxy proxy;
    int proxy_ready;
    char *buffer_for_sequence;
    int maximum_sequence_length;
    int open;
};

enum {
    BSAL_INPUT_ACTOR_OPEN = BSAL_TAG_OFFSET_INPUT_ACTOR, /* +0 */
    BSAL_INPUT_ACTOR_OPEN_OK,
    BSAL_INPUT_ACTOR_ERROR,
    BSAL_INPUT_ACTOR_ERROR_FILE_NOT_FOUND,
    BSAL_INPUT_ACTOR_ERROR_FORMAT_NOT_SUPPORTED,
    BSAL_INPUT_ACTOR_ERROR_ALREADY_OPEN,
    BSAL_INPUT_ACTOR_ERROR_FILE_NOT_OPEN,
    BSAL_INPUT_ACTOR_COUNT,
    BSAL_INPUT_ACTOR_COUNT_YIELD,
    BSAL_INPUT_ACTOR_COUNT_PROGRESS, /* +9 */
    BSAL_INPUT_ACTOR_COUNT_READY,
    BSAL_INPUT_ACTOR_COUNT_RESULT,
    BSAL_INPUT_ACTOR_CLOSE,
    BSAL_INPUT_ACTOR_CLOSE_OK,
    BSAL_INPUT_ACTOR_GET_SEQUENCE,
    BSAL_INPUT_ACTOR_GET_SEQUENCE_END,
    BSAL_INPUT_ACTOR_GET_SEQUENCE_REPLY /* +16 */
};

extern struct bsal_script bsal_input_script;

void bsal_input_actor_init(struct bsal_actor *actor);
void bsal_input_actor_destroy(struct bsal_actor *actor);
void bsal_input_actor_receive(struct bsal_actor *actor, struct bsal_message *message);

int bsal_input_actor_has_error(struct bsal_actor *actor,
                struct bsal_message *message);

int bsal_input_actor_check_open_error(struct bsal_actor *actor,
                struct bsal_message *message);
#endif
