
#include "gc_ratio_calculator.h"

#include <stdio.h>

struct bsal_script gc_ratio_calculator_script = {
    .name = GC_RATIO_CALCULATOR_SCRIPT,
    .init = gc_ratio_calculator_init,
    .destroy = gc_ratio_calculator_destroy,
    .receive = gc_ratio_calculator_receive,
    .size = sizeof(struct gc_ratio_calculator),
    .description = "gc_ratio_calculator"
};

void gc_ratio_calculator_init(struct bsal_actor *actor)
{
}

void gc_ratio_calculator_destroy(struct bsal_actor *actor)
{
}

void gc_ratio_calculator_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    struct bsal_message new_message;
    int name;

    name = bsal_actor_get_name(actor);

    bsal_message_init(&new_message, BSAL_ACTOR_STOP, 0, NULL);
    bsal_actor_send(actor, name, &new_message);
    bsal_message_destroy(&new_message);
}
