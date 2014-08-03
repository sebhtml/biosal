
#include "spate.h"

#include <stdio.h>

struct bsal_script spate_script = {
    .identifier = SPATE_SCRIPT,
    .init = spate_init,
    .destroy = spate_destroy,
    .receive = spate_receive,
    .size = sizeof(struct spate),
    .name = "spate",
    .version = "Project Thor",
    .author = "Sébastien Boisvert",
    .description = "Exact, convenient, and scalable metagenome assembly and genome isolation for everyone"
};

void spate_init(struct bsal_actor *self)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);
    bsal_vector_init(&concrete_self->initial_actors, sizeof(int));

    bsal_actor_register(self, BSAL_ACTOR_START, spate_start);
}

void spate_destroy(struct bsal_actor *self)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);

    bsal_vector_destroy(&concrete_self->initial_actors);
}

void spate_receive(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_dispatch(self, message);
}

void spate_start(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    int name;
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);
    buffer = bsal_message_buffer(message);
    name = bsal_actor_name(self);

    bsal_vector_unpack(&concrete_self->initial_actors, buffer);

    printf("spate/%d starts\n", name);

    bsal_actor_helper_send_to_self_empty(self, BSAL_ACTOR_STOP);
}
