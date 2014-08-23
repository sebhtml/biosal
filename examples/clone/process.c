
#include "process.h"

#include <stdio.h>

struct thorium_script process_script = {
    .identifier = SCRIPT_PROCESS,
    .init = process_init,
    .destroy = process_destroy,
    .receive = process_receive,
    .size = sizeof(struct process),
    .name = "process"
};

void process_init(struct thorium_actor *actor)
{
    struct process *process1;

    process1 = (struct process *)thorium_actor_concrete_actor(actor);
    process1->clone = -1;
    process1->value = 42;
    process1->ready = 0;
    process1->cloned = 0;
    thorium_actor_send_to_self_empty(actor, THORIUM_ACTOR_PACK_ENABLE);
}

void process_destroy(struct thorium_actor *actor)
{
    struct process *process1;

    process1 = (struct process *)thorium_actor_concrete_actor(actor);
    process1->clone = -1;
}

void process_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int name;
    void *buffer;
    struct process *process1;
    int i;
    int other;
    struct thorium_message new_message;

    process1 = (struct process *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_tag(message);
    name = thorium_actor_name(actor);
    buffer = thorium_message_buffer(message);

    if (tag == THORIUM_ACTOR_START) {

        bsal_vector_init(&process1->initial_processes, 0);
        bsal_vector_unpack(&process1->initial_processes, buffer);

        for (i = 0; i < bsal_vector_size(&process1->initial_processes); i++) {

            other = bsal_vector_at_as_int(&process1->initial_processes, i);

            if (other != name) {
                break;
            }
        }

        process1->value = 89 * thorium_actor_name(actor);
        printf("Hi, I am actor:%d and my value is %d. I will clone myself using %d as the spawner\n",
                        name, process1->value, other);

        thorium_message_init(&new_message, THORIUM_ACTOR_CLONE, sizeof(other), &other);

        /* create 2 clones
         * to test the capacity of the runtime to queue messages
         * during cloning
         */
        thorium_actor_send_to_self(actor, &new_message);
        thorium_actor_send_to_self(actor, &new_message);

    } else if (tag == THORIUM_ACTOR_CLONE_REPLY) {

        process1->clone = *(int *)buffer;

        printf("New clone is actor:%d\n", process1->clone);

        thorium_actor_send_empty(actor, process1->clone, THORIUM_ACTOR_ASK_TO_STOP);

        process1->cloned++;

        /* wait for the second clone
         */
        if (process1->cloned != 2) {
            return;
        }

        if (bsal_vector_at_as_int(&process1->initial_processes, 0) == name) {
            thorium_actor_synchronize(actor, &process1->initial_processes);
        }

        if (process1->ready) {
            thorium_actor_send_empty(actor, bsal_vector_at_as_int(&process1->initial_processes, 0),
                            THORIUM_ACTOR_SYNCHRONIZE_REPLY);
        }
        process1->ready = 1;

    } else if (tag == THORIUM_ACTOR_SYNCHRONIZE) {

        if (process1->ready) {
            thorium_actor_send_empty(actor, bsal_vector_at_as_int(&process1->initial_processes, 0),
                            THORIUM_ACTOR_SYNCHRONIZE_REPLY);
        }
        process1->ready = 1;

    } else if (tag == THORIUM_ACTOR_SYNCHRONIZED) {

        thorium_actor_send_range_empty(actor, &process1->initial_processes, THORIUM_ACTOR_ASK_TO_STOP);

    } else if (tag == THORIUM_ACTOR_PACK) {

        thorium_message_init(&new_message, THORIUM_ACTOR_PACK_REPLY, sizeof(process1->value), &process1->value);
        thorium_actor_send_reply(actor, &new_message);

    } else if (tag == THORIUM_ACTOR_UNPACK) {

        process1->value = *(int*)buffer;
        thorium_actor_send_reply_empty(actor, THORIUM_ACTOR_UNPACK_REPLY);

    } else if (tag == THORIUM_ACTOR_ASK_TO_STOP) {

        if (process1->clone == -1) {
            printf("Hi !, my name is %d and my value is %d. I am a clone.\n", name, process1->value);
        } else {

            printf("Hello. my name is %d and my value is %d. I am an original.\n", name, process1->value);
        }

        thorium_actor_send_to_self_empty(actor, THORIUM_ACTOR_STOP);
    }
}
