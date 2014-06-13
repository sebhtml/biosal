
#include "aggregator.h"

#include <helpers/actor_helper.h>

#include <stdio.h>
#include <inttypes.h>

struct bsal_script bsal_aggregator_script = {
    .name = BSAL_AGGREGATOR_SCRIPT,
    .init = bsal_aggregator_init,
    .destroy = bsal_aggregator_destroy,
    .receive = bsal_aggregator_receive,
    .size = sizeof(struct bsal_aggregator)
};

void bsal_aggregator_init(struct bsal_actor *actor)
{
    struct bsal_aggregator *concrete_actor;

    concrete_actor = (struct bsal_aggregator *)bsal_actor_concrete_actor(actor);
    concrete_actor->received = 0;
    concrete_actor->last = 0;
}

void bsal_aggregator_destroy(struct bsal_actor *actor)
{
    struct bsal_aggregator *concrete_actor;

    concrete_actor = (struct bsal_aggregator *)bsal_actor_concrete_actor(actor);
    concrete_actor->received = 0;
    concrete_actor->last = 0;
}

void bsal_aggregator_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    struct bsal_aggregator *concrete_actor;
    int content;
    void *buffer;
    int source;

    concrete_actor = (struct bsal_aggregator *)bsal_actor_concrete_actor(actor);
    buffer = bsal_message_buffer(message);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);

    if (tag == BSAL_AGGREGATE_KERNEL_OUTPUT) {

        concrete_actor->received++;
        content = *(int *)buffer;

        if (concrete_actor->last == 0
                        || concrete_actor->received > concrete_actor->last + 1000) {

            printf("aggregator actor/%d received %" PRIu64 " kernel outputs so far.\n",
                            bsal_actor_name(actor),
                            concrete_actor->received);

            concrete_actor->last = concrete_actor->received;
        }

        bsal_actor_helper_send_reply_int(actor, BSAL_AGGREGATE_KERNEL_OUTPUT_REPLY,
                        content);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP
                    && source == bsal_actor_supervisor(actor)) {

        bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}
