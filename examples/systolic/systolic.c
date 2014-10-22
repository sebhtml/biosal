#include "systolic.h"

/* This is mostly the Hello, World program being used as a template for
 * starting work on the systolic computation.
 */

#include <stdio.h>

void systolic_init(struct thorium_actor *self);
void systolic_destroy(struct thorium_actor *self);
void systolic_receive(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script systolic_script = {
    .identifier = SCRIPT_SYSTOLIC,
    .init = systolic_init,
    .destroy = systolic_destroy,
    .receive = systolic_receive,
    .size = sizeof(struct systolic),
    .name = "systolic"
};

void systolic_init(struct thorium_actor *actor)
{
    struct systolic *systolic1;

    systolic1 = (struct systolic *)thorium_actor_concrete_actor(actor);

    // TODO: Replace this with actual data needed for systolic array
    core_vector_init(&systolic1->initial_data, sizeof(int));
}

void systolic_destroy(struct thorium_actor *actor)
{
    struct systolic *systolic1;

    systolic1 = (struct systolic *)thorium_actor_concrete_actor(actor);

    core_vector_destroy(&systolic1->initial_data);
}

void systolic_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int name;
    void *buffer;
    struct systolic *systolic1;
    int i;

    systolic1 = (struct systolic *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_action(message);
    name = thorium_actor_name(actor);
    buffer = thorium_message_buffer(message);

    if (tag == ACTION_START) {

        core_vector_unpack(&systolic1->initial_data, buffer);

        printf("Hello world ! my name is actor:%d and I have %d acquaintances:",
                        name, (int)core_vector_size(&systolic1->initial_data));

        for (i = 0; i < core_vector_size(&systolic1->initial_data); i++) {
            printf(" actor:%d", core_vector_at_as_int(&systolic1->initial_data, i));
        }
        printf("\n");

        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
}
