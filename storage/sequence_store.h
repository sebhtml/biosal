
#ifndef BSAL_SEQUENCE_STORE_H
#define BSAL_SEQUENCE_STORE_H

#include <engine/actor.h>

#include <structures/vector.h>

#define BSAL_SEQUENCE_STORE_SCRIPT 0x47e2e424

struct bsal_sequence_store {
    struct bsal_vector sequences;
    uint64_t received;
};

#define BSAL_RESERVE 0x00000d3c
#define BSAL_RESERVE_REPLY 0x00002ca8
#define BSAL_PUSH_SEQUENCE_DATA_BLOCK 0x00001160
#define BSAL_PUSH_SEQUENCE_DATA_BLOCK_REPLY 0x00004d02

extern struct bsal_script bsal_sequence_store_script;

void bsal_sequence_store_init(struct bsal_actor *actor);
void bsal_sequence_store_destroy(struct bsal_actor *actor);
void bsal_sequence_store_receive(struct bsal_actor *actor, struct bsal_message *message);

int bsal_sequence_store_has_error(struct bsal_actor *actor,
                struct bsal_message *message);

int bsal_sequence_store_check_open_error(struct bsal_actor *actor,
                struct bsal_message *message);
void bsal_sequence_store_store_sequences(struct bsal_actor *actor, struct bsal_message *message);
void bsal_sequence_store_reserve(struct bsal_actor *actor, struct bsal_message *message);
void bsal_sequence_store_show_progress(struct bsal_actor *actor, struct bsal_message *message);

#endif
