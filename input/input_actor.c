
#include "input_actor.h"

#include <data/dna_sequence.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*#define BSAL_INPUT_DEBUG*/

struct bsal_script bsal_input_script = {
    .name = BSAL_INPUT_SCRIPT,
    .init = bsal_input_actor_init,
    .destroy = bsal_input_actor_destroy,
    .receive = bsal_input_actor_receive,
    .size = sizeof(struct bsal_input_actor)
};

void bsal_input_actor_init(struct bsal_actor *actor)
{
    struct bsal_input_actor *input;

    input = (struct bsal_input_actor *)bsal_actor_concrete_actor(actor);
    input->proxy_ready = 0;
    input->buffer_for_sequence = NULL;
    input->maximum_sequence_length = 0;
    input->open = 0;
}

void bsal_input_actor_destroy(struct bsal_actor *actor)
{
    struct bsal_input_actor *input;

    input = (struct bsal_input_actor *)bsal_actor_concrete_actor(actor);

    if (input->buffer_for_sequence != NULL) {
        free(input->buffer_for_sequence);
        input->buffer_for_sequence = NULL;
        input->maximum_sequence_length = 0;
    }

    input->open = 0;

    if (input->proxy_ready) {
        bsal_input_proxy_destroy(&input->proxy);
        input->proxy_ready = 0;
    }
}

void bsal_input_actor_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int name;
    int count;
    struct bsal_input_actor *input;
    int i;
    int has_sequence;
    int sequences;
    int sequence_index;
    int buffer_size;
    char *buffer;
    char *read_buffer;

    input = (struct bsal_input_actor *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);
    name = bsal_actor_name(actor);
    buffer = (char *)bsal_message_buffer(message);

    /* Do nothing if there is an error.
     * has_error returns the error to the source.
     */
    if (bsal_input_actor_has_error(actor, message)) {
        return;
    }

    if (tag == BSAL_INPUT_OPEN) {

        if (input->open) {
            bsal_message_set_tag(message, BSAL_INPUT_ERROR_ALREADY_OPEN);
            bsal_actor_send(actor, source, message);

            bsal_message_set_tag(message, BSAL_ACTOR_STOP);
            bsal_actor_send(actor, name, message);
            return;
        }

        input->open = 1;

        /* TODO: find out the maximum read length in some way */
        input->maximum_sequence_length = BSAL_INPUT_MAXIMUM_SEQUENCE_LENGTH;

        input->buffer_for_sequence = (char *)malloc(input->maximum_sequence_length);

        /*bsal_input_actor_init(actor);*/

#ifdef BSAL_INPUT_DEBUG
        printf("DEBUG bsal_input_actor_receive open %s\n",
                        buffer);
#endif

        bsal_input_proxy_init(&input->proxy, buffer);
        input->proxy_ready = 1;

        /* Die if there is an error...
         */
        if (bsal_input_actor_has_error(actor, message)) {
            bsal_message_set_tag(message, BSAL_ACTOR_STOP);
            bsal_actor_send(actor, name, message);

            return;
        }

        bsal_message_set_tag(message, BSAL_INPUT_OPEN_OK);
        bsal_actor_send(actor, source, message);

    } else if (tag == BSAL_INPUT_COUNT) {
        /* count a little bit and yield the worker */

        if (bsal_input_actor_check_open_error(actor, message)) {
            return;
        }

        i = 0;
        /* continue counting ... */
        has_sequence = 1;
        while (i < 1000 && has_sequence) {
            has_sequence = bsal_input_proxy_get_sequence(&input->proxy,
                            input->buffer_for_sequence);
            i++;
        }

        if (has_sequence) {

            /*printf("DEBUG yield\n");*/
            bsal_message_set_tag(message, BSAL_INPUT_COUNT_YIELD);
            bsal_actor_send(actor, name, message);

            /* notify the supervisor of our progress...
             */

            sequences = bsal_input_proxy_size(&input->proxy);
            bsal_message_init(message, BSAL_INPUT_COUNT_PROGRESS,
                            sizeof(sequences), &sequences);
            bsal_actor_send(actor, bsal_actor_supervisor(actor), message);
            bsal_message_destroy(message);
        } else {
            bsal_message_set_tag(message, BSAL_INPUT_COUNT_READY);
            bsal_actor_send(actor, name, message);
        }

    } else if (tag == BSAL_INPUT_COUNT_YIELD) {

        if (bsal_input_actor_check_open_error(actor, message)) {
            return;
        }

        bsal_message_set_tag(message, BSAL_INPUT_COUNT);
        bsal_actor_send(actor, source, message);

    } else if (tag == BSAL_INPUT_COUNT_READY) {

        if (bsal_input_actor_check_open_error(actor, message)) {
            return;
        }

        count = bsal_input_proxy_size(&input->proxy);

        bsal_message_init(message, BSAL_INPUT_COUNT_RESULT, sizeof(count),
                        &count);
        bsal_actor_send(actor, bsal_actor_supervisor(actor), message);

    } else if (tag == BSAL_INPUT_CLOSE) {

#ifdef BSAL_INPUT_DEBUG
        printf("DEBUG destroy proxy\n");
#endif

        if (bsal_input_actor_check_open_error(actor, message)) {
            return;
        }

        bsal_message_set_tag(message, BSAL_INPUT_CLOSE_OK);
        bsal_actor_send(actor, source, message);

        bsal_message_set_tag(message, BSAL_ACTOR_STOP);
        bsal_actor_send(actor, name, message);

    } else if (tag == BSAL_INPUT_GET_SEQUENCE) {

        if (bsal_input_actor_check_open_error(actor, message)) {
            return;
        }

        sequence_index = bsal_input_proxy_size(&input->proxy);

        /* TODO it would be clearer to use a struct to pack a int and a char []
         * then to use the code below.
         */
        read_buffer = input->buffer_for_sequence + sizeof(sequence_index);
        has_sequence = bsal_input_proxy_get_sequence(&input->proxy,
                            read_buffer);

        if (!has_sequence) {
            bsal_message_set_tag(message, BSAL_INPUT_GET_SEQUENCE_END);
            bsal_actor_send(actor, source, message);

            return;
        }

        buffer_size = sizeof(sequence_index) + strlen(read_buffer) + 1;

        memcpy(input->buffer_for_sequence, &sequence_index, sizeof(sequence_index));

        bsal_message_init(message, BSAL_INPUT_GET_SEQUENCE_REPLY,
                        buffer_size, input->buffer_for_sequence);
        bsal_actor_send(actor, source, message);
    }
}

int bsal_input_actor_has_error(struct bsal_actor *actor,
                struct bsal_message *message)
{
    int source;
    struct bsal_input_actor *input;
    int error;

    input = (struct bsal_input_actor *)bsal_actor_concrete_actor(actor);

    if (!input->proxy_ready) {
        return 0;
    }

    error = bsal_input_proxy_error(&input->proxy);

    if (error == BSAL_INPUT_ERROR_NOT_FOUND) {

#ifdef BSAL_INPUT_DEBUG
        printf("DEBUG bsal_input_actor_has_error BSAL_INPUT_ERROR_NOT_FOUND\n");
#endif

        source = bsal_message_source(message);

        bsal_message_set_tag(message, BSAL_INPUT_ERROR_FILE_NOT_FOUND);
        bsal_actor_send(actor, source, message);
        return 1;

    } else if (error == BSAL_INPUT_ERROR_NOT_SUPPORTED) {
        source = bsal_message_source(message);

        bsal_message_set_tag(message, BSAL_INPUT_ERROR_FORMAT_NOT_SUPPORTED);
        bsal_actor_send(actor, source, message);
        return 1;
    }

    return 0;
}

int bsal_input_actor_check_open_error(struct bsal_actor *actor,
                struct bsal_message *message)
{
    struct bsal_input_actor *input;
    int source;
    int name;

    source = bsal_message_source(message);
    input = (struct bsal_input_actor *)bsal_actor_concrete_actor(actor);
    name = bsal_actor_name(actor);

    if (!input->open) {
        bsal_message_set_tag(message, BSAL_INPUT_ERROR_FILE_NOT_OPEN);
        bsal_actor_send(actor, source, message);

        bsal_message_set_tag(message, BSAL_ACTOR_STOP);
        bsal_actor_send(actor, name, message);

        return 1;
    }

    return 0;
}


