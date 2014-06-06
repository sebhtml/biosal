
#ifndef _BSAL_INPUT_STREAM_H
#define _BSAL_INPUT_STREAM_H

#include "input_proxy.h"

#include <engine/actor.h>

#define BSAL_INPUT_STREAM_SCRIPT 0xeb2fe16a

struct bsal_input_stream {
    struct bsal_input_proxy proxy;
    int proxy_ready;
    char *buffer_for_sequence;
    int maximum_sequence_length;
    int open;
};

#define BSAL_INPUT_OPEN 0x000075fa
#define BSAL_INPUT_OPEN_REPLY 0x00000f68
#define BSAL_INPUT_COUNT 0x00004943
#define BSAL_INPUT_COUNT_YIELD 0x00000173
#define BSAL_INPUT_COUNT_PROGRESS 0x00001ce0
#define BSAL_INPUT_COUNT_READY 0x0000710e
#define BSAL_INPUT_COUNT_REPLY 0x000018a9
#define BSAL_INPUT_CLOSE 0x00007646
#define BSAL_INPUT_CLOSE_REPLY 0x00004329
#define BSAL_INPUT_GET_SEQUENCE 0x00001333
#define BSAL_INPUT_GET_SEQUENCE_END 0x00006d55
#define BSAL_INPUT_GET_SEQUENCE_REPLY 0x00005295

extern struct bsal_script bsal_input_stream_script;

void bsal_input_stream_init(struct bsal_actor *actor);
void bsal_input_stream_destroy(struct bsal_actor *actor);
void bsal_input_stream_receive(struct bsal_actor *actor, struct bsal_message *message);

int bsal_input_stream_has_error(struct bsal_actor *actor,
                struct bsal_message *message);

int bsal_input_stream_check_open_error(struct bsal_actor *actor,
                struct bsal_message *message);
#endif
