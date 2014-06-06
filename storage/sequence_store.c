
#include "sequence_store.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_script bsal_sequence_store_script = {
    .name = BSAL_SEQUENCE_STORE_SCRIPT,
    .init = bsal_sequence_store_init,
    .destroy = bsal_sequence_store_destroy,
    .receive = bsal_sequence_store_receive,
    .size = sizeof(struct bsal_sequence_store)
};

void bsal_sequence_store_init(struct bsal_actor *actor)
{
    struct bsal_sequence_store *concrete_actor;

    concrete_actor = (struct bsal_sequence_store *)bsal_actor_concrete_actor(actor);
    bsal_vector_init(&concrete_actor->sequences, sizeof(int));
}

void bsal_sequence_store_destroy(struct bsal_actor *actor)
{
    struct bsal_sequence_store *concrete_actor;

    concrete_actor = (struct bsal_sequence_store *)bsal_actor_concrete_actor(actor);
    bsal_vector_destroy(&concrete_actor->sequences);
}

void bsal_sequence_store_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int amount;
    void *buffer;
    struct bsal_sequence_store *concrete_actor;

    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);

    concrete_actor = (struct bsal_sequence_store *)bsal_actor_concrete_actor(actor);

    if (tag == BSAL_STORE_SEQUENCES) {

        bsal_actor_send_reply_empty(actor, BSAL_STORE_SEQUENCES_REPLY);

    } else if (tag == BSAL_SEQUENCE_STORE_ALLOCATE) {

        amount = *(int *)buffer;

        bsal_vector_resize(&concrete_actor->sequences, amount);

        bsal_actor_send_reply_empty(actor, BSAL_SEQUENCE_STORE_ALLOCATE_REPLY);

    } else if (tag == BSAL_SEQUENCE_STORE_STOP) {

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}


