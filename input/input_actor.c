
#include "input_actor.h"

struct bsal_actor_vtable bsal_input_actor_vtable = {
    .init = bsal_input_actor_init,
    .destroy = bsal_input_actor_destroy,
    .receive = bsal_input_actor_receive
};

void bsal_input_actor_init(struct bsal_actor *actor)
{
    struct bsal_input_actor *input;

    input = (struct bsal_input_actor *)bsal_actor_actor(actor);
    input->file_name = NULL;
}

void bsal_input_actor_destroy(struct bsal_actor *actor)
{
    struct bsal_input_actor *input;

    input = (struct bsal_input_actor *)bsal_actor_actor(actor);

    if (input->file_name != NULL) {
        /* free memory */
    }

    input->file_name = NULL;
}

void bsal_input_actor_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int name;
    int count;
    struct bsal_input_actor *input;

    input = (struct bsal_input_actor *)bsal_actor_actor(actor);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);
    name = bsal_actor_name(actor);

    if (tag == BSAL_INPUT_ACTOR_OPEN) {

        bsal_message_set_tag(message, BSAL_INPUT_ACTOR_OPEN_OK);
        bsal_actor_send(actor, source, message);
        input->supervisor = source;

    } else if (tag == BSAL_INPUT_ACTOR_COUNT) {
        /* count a little bit and yield the worker */
        bsal_message_set_tag(message, BSAL_INPUT_ACTOR_COUNT_YIELD);
        bsal_actor_send(actor, name, message);

    } else if (tag == BSAL_INPUT_ACTOR_COUNT_YIELD) {
        bsal_message_set_tag(message, BSAL_INPUT_ACTOR_COUNT_CONTINUE);
        bsal_actor_send(actor, source, message);

    } else if (tag == BSAL_INPUT_ACTOR_COUNT_CONTINUE) {
        /* continue counting ... */

        count = 42;
        bsal_message_set_buffer(message, &count);
        bsal_message_set_count(message, sizeof(count));
        bsal_message_set_tag(message, BSAL_INPUT_ACTOR_COUNT_RESULT);
        bsal_actor_send(actor, input->supervisor, message);

    } else if (tag == BSAL_INPUT_ACTOR_CLOSE) {
        bsal_actor_die(actor);
    }
}
