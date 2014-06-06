
#include "process.h"

#include <stdio.h>

struct bsal_script process_script = {
    .name = PROCESS_SCRIPT,
    .init = process_init,
    .destroy = process_destroy,
    .receive = process_receive,
    .size = sizeof(struct process)
};

void process_init(struct bsal_actor *actor)
{
    struct process *process1;

    process1 = (struct process *)bsal_actor_concrete_actor(actor);
    bsal_vector_init(&process1->initial_processes, sizeof(int));
}

void process_destroy(struct bsal_actor *actor)
{
    struct process *process1;

    process1 = (struct process *)bsal_actor_concrete_actor(actor);

    bsal_vector_destroy(&process1->initial_processes);
}

void process_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int name;
    void *buffer;
    struct process *process1;
    int i;

    process1 = (struct process *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    name = bsal_actor_name(actor);
    buffer = bsal_message_buffer(message);

    if (tag == BSAL_ACTOR_START) {

        bsal_vector_unpack(&process1->initial_processes, buffer);

        printf("Hello world ! my name is actor:%d and I have %d acquaintances:",
                        name, bsal_vector_size(&process1->initial_processes));

        for (i = 0; i < bsal_vector_size(&process1->initial_processes); i++) {
            printf(" actor:%d", bsal_vector_at_as_int(&process1->initial_processes, i));
        }
        printf("\n");

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}
