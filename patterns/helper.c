
#include "helper.h"

#include <engine/message.h>

#include <stdlib.h>

void bsal_helper_init(struct bsal_helper *self)
{
    self->mock = 0;
}

void bsal_helper_destroy(struct bsal_helper *self)
{
    self->mock = 0;
}

void bsal_helper_send_reply_vector(struct bsal_helper *self, struct bsal_actor *actor, int tag, struct bsal_vector *vector)
{
    int count;
    struct bsal_message message;
    void *buffer;

    count = bsal_vector_pack_size(vector);
    buffer = malloc(count);
    bsal_vector_pack(vector, buffer);

    bsal_message_init(&message, tag, count, buffer);

    bsal_actor_send_reply(actor, &message);

    free(buffer);

    bsal_message_destroy(&message);
}


