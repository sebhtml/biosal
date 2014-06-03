
#ifndef _BSAL_INPUT_STREAM_H
#define _BSAL_INPUT_STREAM_H

#include <engine/actor.h>

#define BSAL_SEQUENCE_STORE_SCRIPT 0x47e2e424

struct bsal_sequence_store {
    int foo;
};

enum {
    BSAL_SEQUENCE_STORE_ALLOCATE = BSAL_TAG_OFFSET_SEQUENCE_STORE, /* +0 */
    BSAL_SEQUENCE_STORE_ALLOCATE_REPLY,
    BSAL_STORE_SEQUENCES,
    BSAL_STORE_SEQUENCES_REPLY,
    BSAL_SEQUENCE_STORE_STOP
};

extern struct bsal_script bsal_input_script;

void bsal_sequence_store_init(struct bsal_actor *actor);
void bsal_sequence_store_destroy(struct bsal_actor *actor);
void bsal_sequence_store_receive(struct bsal_actor *actor, struct bsal_message *message);

int bsal_sequence_store_has_error(struct bsal_actor *actor,
                struct bsal_message *message);

int bsal_sequence_store_check_open_error(struct bsal_actor *actor,
                struct bsal_message *message);
#endif
