
#include "sequence_store.h"

#include <structures/vector_iterator.h>
#include <input/input_command.h>
#include <data/dna_sequence.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h>

/*
#define BSAL_SEQUENCE_STORE_DEBUG
*/

struct bsal_script bsal_sequence_store_script = {
    .name = BSAL_SEQUENCE_STORE_SCRIPT,
    .init = bsal_sequence_store_init,
    .destroy = bsal_sequence_store_destroy,
    .receive = bsal_sequence_store_receive,
    .size = sizeof(struct bsal_sequence_store)
};

void bsal_sequence_store_init(struct bsal_actor *actor)
{
    struct bsal_sequence_store *concrete_actor;

    concrete_actor = (struct bsal_sequence_store *)bsal_actor_concrete_actor(actor);
    bsal_vector_init(&concrete_actor->sequences, sizeof(struct bsal_dna_sequence));

#ifdef BSAL_SEQUENCE_STORE_DEBUG
    printf("DEBUG bsal_sequence_store_init name %d\n",
                    bsal_actor_name(actor));
#endif
}

void bsal_sequence_store_destroy(struct bsal_actor *actor)
{
    struct bsal_sequence_store *concrete_actor;
    struct bsal_vector_iterator iterator;
    struct bsal_dna_sequence *sequence;

    concrete_actor = (struct bsal_sequence_store *)bsal_actor_concrete_actor(actor);

    bsal_vector_iterator_init(&iterator, &concrete_actor->sequences);

    while (bsal_vector_iterator_has_next(&iterator)) {

        bsal_vector_iterator_next(&iterator, (void**)&sequence);
        bsal_dna_sequence_destroy(sequence);
    }

    bsal_vector_destroy(&concrete_actor->sequences);
}

void bsal_sequence_store_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;

    tag = bsal_message_tag(message);

    if (tag == BSAL_STORE_SEQUENCES) {

        bsal_sequence_store_store_sequences(actor, message);

    } else if (tag == BSAL_SEQUENCE_STORE_RESERVE) {

        bsal_sequence_store_reserve(actor, message);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}

void bsal_sequence_store_store_sequences(struct bsal_actor *actor, struct bsal_message *message)
{
    uint64_t first;
    /*
    uint64_t last;
    */
    struct bsal_vector *new_entries;
    struct bsal_input_command payload;
    struct bsal_sequence_store *concrete_actor;
    void *buffer;
    uint64_t i;
    int count;
    struct bsal_dna_sequence *bucket_in_message;
    struct bsal_dna_sequence *bucket_in_store;

    buffer = bsal_message_buffer(message);
    count = bsal_message_count(message);
    concrete_actor = (struct bsal_sequence_store *)bsal_actor_concrete_actor(actor);

    printf("DEBUG store receives BSAL_STORE_SEQUENCES %d bytes\n",
                    count);

    bsal_input_command_unpack(&payload, buffer);

    printf("DEBUG store %d bsal_sequence_store_receive command:\n",
                    bsal_actor_name(actor));

    bsal_input_command_print(&payload);

    first = bsal_input_command_store_first(&payload);
    /*
    last = bsal_input_command_store_last(&payload);
    */
    new_entries = bsal_input_command_entries(&payload);

    printf("DEBUG store %d bsal_sequence_store_store_sequences entries %d\n",
                    bsal_actor_name(actor),
                    bsal_vector_size(new_entries));

    for (i = 0; i < bsal_vector_size(new_entries); i++) {

        bucket_in_message = (struct bsal_dna_sequence *)bsal_vector_at(new_entries,
                        i);

        bucket_in_store = (struct bsal_dna_sequence *)bsal_vector_at(&concrete_actor->sequences,
                        first + i);

        /* join the bucket, this load DNA into the store
         */
        /*
        *bucket_in_store = *bucket_in_message;
        */

#ifdef BSAL_SEQUENCE_STORE_DEBUG
        if (i == 0) {
            printf("DEBUG first in payload\n");
            printf("DEBUG-thor i %d bucket_in_store %p bucket_in_message %p\n",
                        (int)i,
                        (void *)bucket_in_store, (void *)bucket_in_message);

            printf("DEBUG i %d first %d size %d store size %d\n",
                   (int)i, (int)first,
                   bsal_vector_size(new_entries),
                   bsal_vector_size(&concrete_actor->sequences));

            bsal_dna_sequence_print(bucket_in_message);
        }
#endif

        bsal_dna_sequence_init_same_data(bucket_in_store, bucket_in_message);
    }

    printf("DONE.\n");

    /* The DNA sequences are kept and are not
     * destroyed.
     */
    /* free payload
     */
    bsal_input_command_destroy(&payload);

    bsal_actor_send_reply_empty(actor, BSAL_STORE_SEQUENCES_REPLY);
}

void bsal_sequence_store_reserve(struct bsal_actor *actor, struct bsal_message *message)
{
    uint64_t amount;
    int i;
    void *buffer;
    struct bsal_dna_sequence *dna_sequence;
    struct bsal_sequence_store *concrete_actor;

    buffer = bsal_message_buffer(message);
    amount = *(uint64_t*)buffer;
    concrete_actor = (struct bsal_sequence_store *)bsal_actor_concrete_actor(actor);

    printf("DEBUG store %d reserves %" PRIu64 " buckets\n",
                    bsal_actor_name(actor),
                    amount);

    bsal_vector_resize(&concrete_actor->sequences, amount);

#ifdef BSAL_SEQUENCE_STORE_DEBUG
    printf("DEBUG store %d now has %d buckets\n",
                    bsal_actor_name(actor),
                    bsal_vector_size(&concrete_actor->sequences));
#endif

    for ( i = 0; i < bsal_vector_size(&concrete_actor->sequences); i++) {
        /*
         * initialize sequences with empty things
         */
        dna_sequence = (struct bsal_dna_sequence *)bsal_vector_at(&concrete_actor->sequences,
                        i);

        bsal_dna_sequence_init(dna_sequence, NULL);
    }

    bsal_actor_send_reply_empty(actor, BSAL_SEQUENCE_STORE_RESERVE_REPLY);
}
