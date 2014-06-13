
#include "input_stream.h"

#include "input_command.h"

#include <data/dna_sequence.h>
#include <storage/sequence_store.h>
#include <patterns/helper.h>

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

    input->file_name = NULL;
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

    if (input->file_name != NULL) {
        free(input->file_name);
        input->file_name = NULL;
    }

    input->file_name = NULL;
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
            bsal_helper_send_reply_int(actor, BSAL_INPUT_OPEN_REPLY, concrete_actor->error);
            bsal_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);

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

        concrete_actor->file_name = malloc(strlen(buffer) + 1);
        strcpy(concrete_actor->file_name, buffer);

        bsal_input_proxy_init(&concrete_actor->proxy, buffer);
        concrete_actor->proxy_ready = 1;

        /* Die if there is an error...
         */
        if (bsal_input_stream_has_error(actor, message)) {

#ifdef BSAL_INPUT_STREAM_DEBUG
            printf("DEBUG has error\n");
#endif

            bsal_helper_send_reply_int(actor, BSAL_INPUT_OPEN_REPLY, concrete_actor->error);
            bsal_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);

            return;
        }

        concrete_actor->controller = source;

        /* no error here... */
        bsal_helper_send_reply_int(actor, BSAL_INPUT_OPEN_REPLY, concrete_actor->error);

    } else if (tag == BSAL_INPUT_COUNT) {
        /* count a little bit and yield the worker */

        if (bsal_input_stream_check_open_error(actor, message)) {

            bsal_helper_send_reply_int(actor, BSAL_INPUT_COUNT_REPLY, concrete_actor->error);
            bsal_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);

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

            bsal_helper_send_to_self_empty(actor, BSAL_ACTOR_YIELD);

            /* notify the controller of our progress...
             */

            sequences = bsal_input_proxy_size(&concrete_actor->proxy);

#ifdef BSAL_INPUT_STREAM_DEBUG
            printf("DEBUG BSAL_INPUT_COUNT sequences %d...\n", sequences);
#endif

            bsal_helper_send_int(actor, concrete_actor->controller, BSAL_INPUT_COUNT_PROGRESS, sequences);

            bsal_message_destroy(message);

        } else {

            bsal_helper_send_to_self_empty(actor, BSAL_INPUT_COUNT_READY);
        }

    } else if (tag == BSAL_ACTOR_YIELD_REPLY) {

        if (bsal_input_stream_check_open_error(actor, message)) {
                /*
                 * it is not clear that there can be an error when receiving YIELD.
            error = concrete_actor->error;
            bsal_helper_send_reply_int(actor, BSAL_INPUT_COUNT_REPLY, error);
            bsal_actor_send_to_self(actor, BSAL_ACTOR_STOP);

                 */
            return;
        }

        bsal_helper_send_to_self_empty(actor, BSAL_INPUT_COUNT);

    } else if (tag == BSAL_INPUT_COUNT_READY) {

        if (bsal_input_stream_check_open_error(actor, message)) {
            return;
        }

        count = bsal_input_proxy_size(&concrete_actor->proxy);

        bsal_helper_send_int(actor, concrete_actor->controller, BSAL_INPUT_COUNT_REPLY, count);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        bsal_helper_send_to_self_empty(actor, BSAL_INPUT_CLOSE);

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

            bsal_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);
            return;
        }

        bsal_message_init(message, BSAL_INPUT_CLOSE_REPLY, sizeof(concrete_actor->error),
                &concrete_actor->error);
        bsal_actor_send(actor, source, message);

        bsal_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("actor %d sending BSAL_INPUT_CLOSE_REPLY to %d\n",
                        bsal_actor_name(actor), source);
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

    } else if (tag == BSAL_INPUT_PUSH_SEQUENCES) {

        bsal_input_stream_push_sequences(actor, message);
    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        bsal_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);
        bsal_helper_send_reply_empty(actor, BSAL_ACTOR_ASK_TO_STOP_REPLY);

    } else if (tag == BSAL_INPUT_STREAM_RESET) {

        /* fail silently
         */
        if (!concrete_actor->open) {
            bsal_helper_send_reply_empty(actor, BSAL_INPUT_STREAM_RESET_REPLY);
            return;
        }

#ifdef BSAL_INPUT_STREAM_DEBUG
        printf("DEBUG BSAL_INPUT_STREAM_RESET\n");
#endif

        bsal_input_proxy_destroy(&concrete_actor->proxy);
        bsal_input_proxy_init(&concrete_actor->proxy, concrete_actor->file_name);

        bsal_helper_send_reply_empty(actor, BSAL_INPUT_STREAM_RESET_REPLY);

    } else if (tag == BSAL_PUSH_SEQUENCE_DATA_BLOCK_REPLY) {

        bsal_helper_send_to_supervisor_empty(actor, BSAL_INPUT_PUSH_SEQUENCES_REPLY);
    }
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

void bsal_input_stream_push_sequences(struct bsal_actor *actor,
                struct bsal_message *message)
{
    struct bsal_input_command command;
    void *buffer;
    int store_name;
    uint64_t store_first;
    uint64_t store_last;
    int sequences_to_read;
    void *new_buffer;
    int new_count;
    struct bsal_message new_message;
    void *buffer_for_sequence;
    struct bsal_dna_sequence dna_sequence;
    struct bsal_vector *command_entries;
    struct bsal_dna_sequence *a_sequence;
    int has_sequence;
    int i;

#ifdef BSAL_INPUT_STREAM_DEBUG
    int count;
    int actual_count;
#endif

    has_sequence = 1;

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG bsal_input_stream_push_sequences entering...\n");
#endif

    struct bsal_input_stream *concrete_actor;

    buffer = bsal_message_buffer(message);

#ifdef BSAL_INPUT_STREAM_DEBUG
    count = bsal_message_count(message);
#endif

    concrete_actor = (struct bsal_input_stream *)bsal_actor_concrete_actor(actor);
    bsal_input_command_unpack(&command, buffer);

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG bsal_input_stream_push_sequences after unpack, count %d:\n",
                    count);

    bsal_input_command_print(&command);
#endif

    store_name = bsal_input_command_store_name(&command);
    store_first = bsal_input_command_store_first(&command);
    store_last = bsal_input_command_store_last(&command);

    sequences_to_read = store_last - store_first + 1;

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG bsal_input_stream_push_sequences received BSAL_INPUT_PUSH_SEQUENCES\n");
    printf("DEBUG Command before sending it\n");

    bsal_input_command_print(&command);
#endif

    /*
     * add sequences to the input command
     */

    buffer_for_sequence = concrete_actor->buffer_for_sequence;
    command_entries = bsal_input_command_entries(&command);

    i = 0;
    /* TODO: actually load something
     */
    while (sequences_to_read-- && has_sequence) {
        has_sequence = bsal_input_proxy_get_sequence(&concrete_actor->proxy,
                            buffer_for_sequence);

        bsal_dna_sequence_init(&dna_sequence, buffer_for_sequence);

        bsal_vector_push_back(command_entries,
                        &dna_sequence);

        i++;
    }

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG prepared %d sequences for command\n",
                    i);
    bsal_input_command_print(&command);
#endif

    new_count = bsal_input_command_pack_size(&command);

    new_buffer = malloc(new_count);

    bsal_input_command_pack(&command, new_buffer);

#ifdef BSAL_INPUT_STREAM_DEBUG
    actual_count = bsal_input_command_pack(&command, new_buffer);
    printf("DEBUG123 new_count %d actual_count %d\n", new_count,
                    actual_count);
#endif

    bsal_message_init(&new_message, BSAL_PUSH_SEQUENCE_DATA_BLOCK,
                    new_count, new_buffer);

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG bsal_input_stream_push_sequences sending BSAL_PUSH_SEQUENCE_DATA_BLOCK to %d\n",
                    store_name);
#endif

    bsal_actor_send(actor, store_name, &new_message);

    /*
     * send sequences to store.
     * The required information is the input command
     */
    /*
    bsal_helper_send_reply_empty(actor, BSAL_INPUT_PUSH_SEQUENCES_REPLY);
    */

    /* free memory
     */
    free(new_buffer);

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG freeing %d entries\n",
                    (int)bsal_vector_size(command_entries));
#endif

    for (i = 0; i < bsal_vector_size(command_entries); i++) {
        a_sequence = (struct bsal_dna_sequence *)bsal_vector_at(command_entries, i);

        bsal_dna_sequence_destroy(a_sequence);
    }

    bsal_vector_destroy(command_entries);

#ifdef BSAL_INPUT_STREAM_DEBUG
    printf("DEBUG bsal_input_stream_push_sequences EXIT\n");
#endif
}
