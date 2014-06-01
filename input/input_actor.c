
#include "input_actor.h"

#include <data/dna_sequence.h>

#include <stdio.h>

/*#define BSAL_INPUT_ACTOR_DEBUG*/

struct bsal_actor_vtable bsal_input_actor_vtable = {
    .receive = bsal_input_actor_receive
};

void bsal_input_actor_init(struct bsal_actor *actor)
{
    struct bsal_input_actor *input;

    input = (struct bsal_input_actor *)bsal_actor_pointer(actor);
    input->proxy_ready = 0;
}

void bsal_input_actor_destroy(struct bsal_actor *actor)
{
    struct bsal_input_actor *input;

    input = (struct bsal_input_actor *)bsal_actor_pointer(actor);

    if (input->proxy_ready) {
        bsal_input_proxy_destroy(&input->proxy);
    }

    bsal_actor_die(actor);
}

void bsal_input_actor_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int name;
    int count;
    struct bsal_input_actor *input;
    struct bsal_dna_sequence sequence;
    char *buffer;
    int i;
    int has_sequence;
    int sequences;

    input = (struct bsal_input_actor *)bsal_actor_pointer(actor);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);
    name = bsal_actor_name(actor);
    buffer = (char *)bsal_message_buffer(message);

    /* Do nothing if there is an error.
     * has_error returns the error to the source.
     */
    if (bsal_input_actor_has_error(actor, message, 0)) {
        return;
    }

    if (tag == BSAL_INPUT_ACTOR_OPEN) {

        bsal_input_actor_init(actor);

#ifdef BSAL_INPUT_ACTOR_DEBUG
        printf("DEBUG bsal_input_actor_receive open %s\n",
                        buffer);
#endif

        bsal_input_proxy_init(&input->proxy, buffer);
        input->proxy_ready = 1;

        /* Die if there is an error...
         */
        if (bsal_input_actor_has_error(actor, message, 1)) {
            bsal_input_actor_destroy(actor);

            return;
        }

        bsal_message_set_tag(message, BSAL_INPUT_ACTOR_OPEN_OK);
        bsal_actor_send(actor, source, message);

    } else if (tag == BSAL_INPUT_ACTOR_COUNT) {
        /* count a little bit and yield the worker */

        i = 0;
        /* continue counting ... */
        has_sequence = 1;
        while (i < 1000 && has_sequence) {
            has_sequence = bsal_input_proxy_get_sequence(&input->proxy,
                            &sequence);
            i++;
        }

        if (has_sequence) {

            /*printf("DEBUG yield\n");*/
            bsal_message_set_tag(message, BSAL_INPUT_ACTOR_COUNT_YIELD);
            bsal_actor_send(actor, name, message);

            /* notify the supervisor of our progress...
             */

            sequences = bsal_input_proxy_size(&input->proxy);
            bsal_message_init(message, BSAL_INPUT_ACTOR_COUNT_PROGRESS,
                            sizeof(sequences), &sequences);
            bsal_actor_send(actor, bsal_actor_supervisor(actor), message);
            bsal_message_destroy(message);
        } else {
            bsal_message_set_tag(message, BSAL_INPUT_ACTOR_COUNT_READY);
            bsal_actor_send(actor, name, message);
        }

    } else if (tag == BSAL_INPUT_ACTOR_COUNT_YIELD) {
        bsal_message_set_tag(message, BSAL_INPUT_ACTOR_COUNT);
        bsal_actor_send(actor, source, message);

    } else if (tag == BSAL_INPUT_ACTOR_COUNT_READY) {

        count = bsal_input_proxy_size(&input->proxy);
        bsal_message_set_buffer(message, &count);
        bsal_message_set_count(message, sizeof(count));
        bsal_message_set_tag(message, BSAL_INPUT_ACTOR_COUNT_RESULT);
        bsal_actor_send(actor, bsal_actor_supervisor(actor), message);

    } else if (tag == BSAL_INPUT_ACTOR_CLOSE) {

#ifdef BSAL_INPUT_ACTOR_DEBUG
        printf("DEBUG destroy proxy\n");
#endif

        bsal_input_actor_destroy(actor);
    }
}

int bsal_input_actor_has_error(struct bsal_actor *actor,
                struct bsal_message *message, int force)
{
    int source;
    struct bsal_input_actor *input;
    int error;

    if (bsal_actor_received_messages(actor) == 1 && !force) {
        return 0;
    }

    input = (struct bsal_input_actor *)bsal_actor_pointer(actor);

    if (!input->proxy_ready) {
        return 0;
    }

    error = bsal_input_proxy_error(&input->proxy);

    if (error == BSAL_INPUT_ERROR_NOT_FOUND) {

#ifdef BSAL_INPUT_ACTOR_DEBUG
        printf("DEBUG bsal_input_actor_has_error BSAL_INPUT_ERROR_NOT_FOUND\n");
#endif

        source = bsal_message_source(message);

        bsal_message_set_tag(message, BSAL_INPUT_ACTOR_OPEN_NOT_FOUND);
        bsal_actor_send(actor, source, message);
        return 1;

    } else if (error == BSAL_INPUT_ERROR_NOT_SUPPORTED) {
        source = bsal_message_source(message);

        bsal_message_set_tag(message, BSAL_INPUT_ACTOR_OPEN_NOT_SUPPORTED);
        bsal_actor_send(actor, source, message);
        return 1;
    }

    return 0;
}
