
#ifndef BSAL_HELPER_H
#define BSAL_HELPER_H

#include <engine/actor.h>
#include <structures/vector.h>

struct bsal_helper {
    int mock;
};

void bsal_helper_init(struct bsal_helper *self);
void bsal_helper_destroy(struct bsal_helper *self);
void bsal_helper_send_reply_vector(struct bsal_helper *self, struct bsal_actor *actor, int tag, struct bsal_vector *vector);

#endif
