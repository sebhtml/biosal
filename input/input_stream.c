
#include "input_stream.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#define BSAL_INPUT_STREAM_DEBUG
*/

struct bsal_script bsal_input_stream_script = {
    .name = BSAL_INPUT_STREAM_SCRIPT,
    .init = bsal_input_stream_init,
    .destroy = bsal_input_stream_destroy,
    .receive = bsal_input_stream_receive,
    .size = sizeof(struct bsal_input_stream)
};

void bsal_input_stream_init(struct bsal_actor *actor)
{
    struct bsal_input_stream *input;

    input = (struct bsal_input_stream *)bsal_actor_concrete_actor(actor);
    input->proxy_ready = 0;
    input->buffer_for_sequence = NULL;
    input->maximum_sequence_length = 0;
    input->open = 0;
    input->error = BSAL_INPUT_ERROR_NO_ERROR;
}

void bsal_input_stream_destroy(struct bsal_actor *actor)
{
    struct bsal_input_stream *input;

    input = (struct bsal_input_stream *)bsal_actor_concrete_actor(actor);

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

void bsal_input_stream_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int count;
    struct bsal_input_stream *concrete_actor;
    int i;
    int has_sequence;
    int sequences;
    int sequence_index;
    int buffer_size;
    char *buffer;
    char *read_buffer;

    concrete_actor = (struct bsal_input_stream *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);
    buffer = (char *)bsal_message_buffer(message);

    /* Do nothing if there is an error.
     * has_error returns the error to the source.
     */
    /*
    if (bsal_input_stream_has_error(actor, message)) {
        return;
    }

    */

    if (tag == BSAL_INPUT_OPEN) {

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("DEBUG BSAL_INPUT_OPEN\n");
#endif

        if (concrete_actor->open) {

            concrete_actor->error = BSAL_INPUT_ERROR_ALREADY_OPEN;
            bsal_actor_send_reply_int(actor, BSAL_INPUT_OPEN_REPLY, concrete_actor->error);
            bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);

            return;
        }

        concrete_actor->open = 1;

        /* TODO: find out the maximum read length in some way */
        concrete_actor->maximum_sequence_length = BSAL_INPUT_MAXIMUM_SEQUENCE_LENGTH;

        concrete_actor->buffer_for_sequence = (char *)malloc(concrete_actor->maximum_sequence_length);

        /*bsal_input_stream_init(actor);*/

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("DEBUG bsal_input_stream_receive open %s\n",
                        buffer);
#endif

        bsal_input_proxy_init(&concrete_actor->proxy, buffer);
        concrete_actor->proxy_ready = 1;

        /* Die if there is an error...
         */
        if (bsal_input_stream_has_error(actor, message)) {

#ifdef BSAL_INPUT_STREAM_DEBUG
            printf("DEBUG has error\n");
#endif

            bsal_actor_send_reply_int(actor, BSAL_INPUT_OPEN_REPLY, concrete_actor->error);
            bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);

            return;
        }

        concrete_actor->controller = source;

        /* no error here... */
        bsal_actor_send_reply_int(actor, BSAL_INPUT_OPEN_REPLY, concrete_actor->error);

    } else if (tag == BSAL_INPUT_COUNT) {
        /* count a little bit and yield the worker */

        if (bsal_input_stream_check_open_error(actor, message)) {

            bsal_actor_send_reply_int(actor, BSAL_INPUT_COUNT_REPLY, concrete_actor->error);
            bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);

            return;
        }

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("DEBUG BSAL_INPUT_COUNT received...\n");
#endif

        i = 0;
        /* continue counting ... */
        has_sequence = 1;
        while (i < 1000 && has_sequence) {
            has_sequence = bsal_input_proxy_get_sequence(&concrete_actor->proxy,
                            concrete_actor->buffer_for_sequence);
            i++;
        }

        if (has_sequence) {

            /*printf("DEBUG yield\n");*/

            bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_YIELD);

            /* notify the controller of our progress...
             */

            sequences = bsal_input_proxy_size(&concrete_actor->proxy);

#ifdef BSAL_INPUT_STREAM_DEBUG
            printf("DEBUG BSAL_INPUT_COUNT sequences %d...\n", sequences);
#endif

            bsal_actor_send_int(actor, concrete_actor->controller, BSAL_INPUT_COUNT_PROGRESS, sequences);

            bsal_message_destroy(message);

        } else {

            bsal_actor_send_to_self_empty(actor, BSAL_INPUT_COUNT_READY);
        }

    } else if (tag == BSAL_ACTOR_YIELD_REPLY) {

        if (bsal_input_stream_check_open_error(actor, message)) {
                /*
                 * it is not clear that there can be an error when receiving YIELD.
            error = concrete_actor->error;
            bsal_actor_send_reply_int(actor, BSAL_INPUT_COUNT_REPLY, error);
            bsal_actor_send_to_self(actor, BSAL_ACTOR_STOP);

                 */
            return;
        }

        bsal_actor_send_to_self_empty(actor, BSAL_INPUT_COUNT);

    } else if (tag == BSAL_INPUT_COUNT_READY) {

        if (bsal_input_stream_check_open_error(actor, message)) {
            return;
        }

        count = bsal_input_proxy_size(&concrete_actor->proxy);

        bsal_actor_send_int(actor, concrete_actor->controller, BSAL_INPUT_COUNT_REPLY, count);

    } else if (tag == BSAL_INPUT_CLOSE) {

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("DEBUG destroy proxy\n");
#endif
        concrete_actor->error = BSAL_INPUT_ERROR_NO_ERROR;

        if (bsal_input_stream_check_open_error(actor, message)) {
            concrete_actor->error = BSAL_INPUT_ERROR_FILE_NOT_OPEN;

            bsal_message_init(message, BSAL_INPUT_CLOSE_REPLY, sizeof(concrete_actor->error),
                &concrete_actor->error);
            bsal_actor_send(actor, source, message);

            bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
            return;
        }

        bsal_message_init(message, BSAL_INPUT_CLOSE_REPLY, sizeof(concrete_actor->error),
                &concrete_actor->error);
        bsal_actor_send(actor, source, message);

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("actor %d sending BSAL_INPUT_CLOSE_REPLY to %d\n", name, source);
#endif

    } else if (tag == BSAL_INPUT_GET_SEQUENCE) {

        if (bsal_input_stream_check_open_error(actor, message)) {

            concrete_actor->error = BSAL_INPUT_ERROR_FILE_NOT_OPEN;
            bsal_message_init(message, BSAL_INPUT_GET_SEQUENCE_REPLY, sizeof(concrete_actor->error),
                            &concrete_actor->error);
            bsal_actor_send(actor, source, message);

            return;
        }

        sequence_index = bsal_input_proxy_size(&concrete_actor->proxy);

        /* TODO it would be clearer to use a struct to pack a int and a char []
         * then to use the code below.
         */
        read_buffer = concrete_actor->buffer_for_sequence + sizeof(sequence_index);
        has_sequence = bsal_input_proxy_get_sequence(&concrete_actor->proxy,
                            read_buffer);

        if (!has_sequence) {
            bsal_message_init(message, BSAL_INPUT_GET_SEQUENCE_END, 0, NULL);
            bsal_actor_send(actor, source, message);

            return;
        }

        buffer_size = sizeof(sequence_index) + strlen(read_buffer) + 1;

        memcpy(concrete_actor->buffer_for_sequence, &sequence_index, sizeof(sequence_index));

        bsal_message_init(message, BSAL_INPUT_GET_SEQUENCE_REPLY,
                        buffer_size, concrete_actor->buffer_for_sequence);
        bsal_actor_send(actor, source, message);

    } else if (tag == BSAL_INPUT_SEND_SEQUENCES_TO) {
        bsal_input_stream_send_sequences_to(actor, message);

    } else if (tag == BSAL_INPUT_PUSH_SEQUENCES_REPLY) {

        bsal_actor_send_empty(actor, concrete_actor->controller, BSAL_INPUT_SEND_SEQUENCES_TO_REPLY);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_ASK_TO_STOP_REPLY);
    }
}

void bsal_input_stream_send_sequences_to(struct bsal_actor *actor,
                struct bsal_message *message)
{
    int target_for_sequences;
    int first_sequence;
    int last_sequence;
    int name;
    int offset;
    struct bsal_input_stream *concrete_actor;
    int source;

    source = bsal_message_source(message);
    concrete_actor = (struct bsal_input_stream *)bsal_actor_concrete_actor(actor);
    concrete_actor->controller = source;
    offset = 0;
    offset = bsal_message_unpack_int(message, offset, &first_sequence);
    offset = bsal_message_unpack_int(message, offset, &last_sequence);
    offset = bsal_message_unpack_int(message, offset, &target_for_sequences);
    name = bsal_actor_name(actor);


    /* TODO
     * send sequences from first_sequence to last_sequence to
     * actor target_for_sequences
     */

    printf("DEBUG stream %d, sequences: %d-%d, storage target: %d\n",
                    name, first_sequence, last_sequence, target_for_sequences);

    bsal_actor_send_empty(actor, target_for_sequences, BSAL_INPUT_PUSH_SEQUENCES);
}

int bsal_input_stream_has_error(struct bsal_actor *actor,
                struct bsal_message *message)
{
    struct bsal_input_stream *input;

    input = (struct bsal_input_stream *)bsal_actor_concrete_actor(actor);

    if (!input->proxy_ready) {
        return 0;
    }

    input->error = bsal_input_proxy_error(&input->proxy);

    if (input->error == BSAL_INPUT_ERROR_FILE_NOT_FOUND) {

        return 1;

    } else if (input->error == BSAL_INPUT_ERROR_FORMAT_NOT_SUPPORTED) {
        return 1;
    }

    return 0;
}

int bsal_input_stream_check_open_error(struct bsal_actor *actor,
                struct bsal_message *message)
{
    struct bsal_input_stream *input;

    input = (struct bsal_input_stream *)bsal_actor_concrete_actor(actor);

    if (!input->open) {

        input->error = BSAL_INPUT_ERROR_FILE_NOT_OPEN;
        return 1;
    }

    return 0;
}


