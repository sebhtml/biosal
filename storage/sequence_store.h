
#ifndef _BSAL_INPUT_STREAM_H
#define _BSAL_INPUT_STREAM_H

#include <engine/actor.h>

#include <structures/vector.h>

#define BSAL_SEQUENCE_STORE_SCRIPT 0x47e2e424

struct bsal_sequence_store {
    struct bsal_vector sequences;
};

#define BSAL_SEQUENCE_STORE_ALLOCATE 0x00000d3c
#define BSAL_SEQUENCE_STORE_ALLOCATE_REPLY 0x00002ca8
#define BSAL_STORE_SEQUENCES 0x00001160
#define BSAL_STORE_SEQUENCES_REPLY 0x00004d02
#define BSAL_SEQUENCE_STORE_STOP 0x0000237b

extern struct bsal_script bsal_input_script;

void bsal_sequence_store_init(struct bsal_actor *actor);
void bsal_sequence_store_destroy(struct bsal_actor *actor);
void bsal_sequence_store_receive(struct bsal_actor *actor, struct bsal_message *message);

int bsal_sequence_store_has_error(struct bsal_actor *actor,
                struct bsal_message *message);

int bsal_sequence_store_check_open_error(struct bsal_actor *actor,
                struct bsal_message *message);
#endif
