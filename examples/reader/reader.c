
#include "reader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bsal_actor_vtable reader_vtable = {
    .init = reader_init,
    .destroy = reader_destroy,
    .receive = reader_receive
};

void reader_init(struct bsal_actor *actor)
{
}

void reader_destroy(struct bsal_actor *actor)
{
}

void reader_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int argc;
    char **argv;
    int i;
    int name;

    tag = bsal_message_tag(message);

    if (tag == BSAL_START) {
        argc = bsal_actor_argc(actor);
        argv = bsal_actor_argv(actor);
        name = bsal_actor_name(actor);
        printf("actor %i received %i arguments\n", name, argc);

        for (i = 0; i < argc ;i++) {
            printf("   argument %i : %s\n", i, argv[i]);
        }

        bsal_actor_die(actor);
    }
}
