
#ifndef BSAL_HELPER_H
#define BSAL_HELPER_H

#include <engine/actor.h>
#include <structures/vector.h>

struct bsal_helper {
    int mock;
};

void bsal_helper_init(struct bsal_helper *self);
void bsal_helper_destroy(struct bsal_helper *self);

void bsal_helper_send_empty(struct bsal_actor *actor, int destination, int tag);
void bsal_helper_send_int(struct bsal_actor *actor, int destination, int tag, int value);

void bsal_helper_send_reply_empty(struct bsal_actor *actor, int tag);
void bsal_helper_send_reply_int(struct bsal_actor *actor, int tag, int error);
void bsal_helper_send_reply_vector(struct bsal_actor *actor, int tag, struct bsal_vector *vector);

void bsal_helper_send_to_self_empty(struct bsal_actor *actor, int tag);
void bsal_helper_send_to_self_int(struct bsal_actor *actor, int tag, int value);

void bsal_helper_send_to_supervisor_empty(struct bsal_actor *actor, int tag);
void bsal_helper_send_to_supervisor_int(struct bsal_actor *actor, int tag, int value);

#endif
