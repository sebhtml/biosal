
#include "kmer_counter_kernel.h"

#include <data/dna_kmer.h>
#include <data/dna_kmer_block.h>
#include <data/dna_sequence.h>
#include <storage/sequence_store.h>
#include <input/input_command.h>

#include <helpers/actor_helper.h>
#include <helpers/message_helper.h>

#include <patterns/aggregator.h>
#include <system/memory.h>
#include <system/timer.h>
#include <system/debugger.h>

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

/* options for this kernel
 */
/*
*/
#define BSAL_KMER_COUNTER_KERNEL_ENABLED

/* debugging options
 */
/*
#define BSAL_KMER_COUNTER_KERNEL_DEBUG
*/

struct bsal_script bsal_kmer_counter_kernel_script = {
    .name = BSAL_KMER_COUNTER_KERNEL_SCRIPT,
    .init = bsal_kmer_counter_kernel_init,
    .destroy = bsal_kmer_counter_kernel_destroy,
    .receive = bsal_kmer_counter_kernel_receive,
    .size = sizeof(struct bsal_kmer_counter_kernel)
};

void bsal_kmer_counter_kernel_init(struct bsal_actor *actor)
{
    struct bsal_kmer_counter_kernel *concrete_actor;

    concrete_actor = (struct bsal_kmer_counter_kernel *)bsal_actor_concrete_actor(actor);

    concrete_actor->expected = 0;
    concrete_actor->actual = 0;
    concrete_actor->last = 0;
    concrete_actor->blocks = 0;

    concrete_actor->kmer_length = -1;
    concrete_actor->customer = -1;
}

void bsal_kmer_counter_kernel_destroy(struct bsal_actor *actor)
{
    struct bsal_kmer_counter_kernel *concrete_actor;

    concrete_actor = (struct bsal_kmer_counter_kernel *)bsal_actor_concrete_actor(actor);

    concrete_actor->customer = 0;
}

void bsal_kmer_counter_kernel_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    struct bsal_dna_kmer kmer;
    int name;
    struct bsal_input_command payload;
    void *buffer;
    int entries;
    struct bsal_kmer_counter_kernel *concrete_actor;
    int source_index;
    int customer;
    int i;
    struct bsal_dna_sequence *sequence;
    char *sequence_data;
    struct bsal_vector *command_entries;
    int sequence_length;
    int new_count;
    void *new_buffer;
    struct bsal_message new_message;
    int j;
    int limit;
    char saved;
    struct bsal_timer timer;
    struct bsal_dna_kmer_block block;
    int to_reserve;
    int maximum_length;

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG
    int count;

    count = bsal_message_count(message);
#endif

    concrete_actor = (struct bsal_kmer_counter_kernel *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    name = bsal_actor_name(actor);
    source = bsal_message_source(message);
    buffer = bsal_message_buffer(message);

    if (tag == BSAL_PUSH_SEQUENCE_DATA_BLOCK) {

        if (concrete_actor->kmer_length == -1) {
            printf("Error no kmer length set in kernel\n");
            return;
        }

        if (concrete_actor->customer == -1) {
            printf("Error no customer set in kernel\n");
            return;
        }

        bsal_timer_init(&timer);
        bsal_timer_start(&timer);

        customer = bsal_actor_get_acquaintance(actor, concrete_actor->customer);
        source_index = bsal_actor_add_acquaintance(actor, source);

        bsal_input_command_unpack(&payload, buffer);

        command_entries = bsal_input_command_entries(&payload);

        entries = bsal_vector_size(command_entries);

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG
        printf("DEBUG kernel receives %d entries (%d bytes), kmer length: %d, bytes per object: %d\n",
                        entries, count, concrete_actor->kmer_length,
                        concrete_actor->bytes_per_kmer);
#endif

        to_reserve = 0;

        maximum_length = 0;

        for (i = 0; i < entries; i++) {

            sequence = (struct bsal_dna_sequence *)bsal_vector_at(command_entries, i);

            sequence_length = bsal_dna_sequence_length(sequence);

            if (sequence_length > maximum_length) {
                maximum_length = sequence_length;
            }

            to_reserve += (sequence_length - concrete_actor->kmer_length + 1);
        }

        bsal_dna_kmer_block_init(&block, concrete_actor->kmer_length, source_index, to_reserve);

        sequence_data = bsal_malloc(maximum_length + 1);

        /* extract kmers
         */
        for (i = 0; i < entries; i++) {

            /* TODO improve this */
            sequence = (struct bsal_dna_sequence *)bsal_vector_at(command_entries, i);

            bsal_dna_sequence_get_sequence(sequence, sequence_data);

            sequence_length = bsal_dna_sequence_length(sequence);
            limit = sequence_length - concrete_actor->kmer_length + 1;

            for (j = 0; j < limit; j++) {
                saved = sequence_data[j + concrete_actor->kmer_length];
                sequence_data[j + concrete_actor->kmer_length] = '\0';

                bsal_dna_kmer_init(&kmer, sequence_data + j);

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG_LEVEL_2
                printf("KERNEL kmer %d,%d %s\n", i, j, sequence_data + j);
#endif

                /*
                 * add kmer in block
                 */
                bsal_dna_kmer_block_add_kmer(&block, &kmer);

                bsal_dna_kmer_destroy(&kmer);

                sequence_data[j + concrete_actor->kmer_length] = saved;
            }
        }

        bsal_free(sequence_data);
        sequence_data = NULL;

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG
        BSAL_DEBUG_MARKER("after generating kmers\n");
#endif

        concrete_actor->actual += entries;
        concrete_actor->blocks++;

        bsal_input_command_destroy(&payload);

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG
        printf("customer %d\n", customer);
#endif

#ifdef BSAL_KMER_COUNTER_KERNEL_ENABLED

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG
        BSAL_DEBUG_MARKER("kernel sends to customer\n");
        printf("customer is %d\n", customer);
#endif

        new_count = bsal_dna_kmer_block_pack_size(&block);
        new_buffer = bsal_malloc(new_count);
        bsal_dna_kmer_block_pack(&block, new_buffer);

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG
        printf("name %d destination %d PACK with %d bytes\n", name,
                       customer, new_count);
#endif

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG
        BSAL_DEBUG_MARKER("kernel sends to aggregator");
#endif

        bsal_message_init(&new_message, BSAL_AGGREGATE_KERNEL_OUTPUT,
                        new_count, new_buffer);

        /*
        bsal_message_init(&new_message, BSAL_AGGREGATE_KERNEL_OUTPUT,
                        sizeof(source_index), &source_index);
                        */

        bsal_actor_send(actor, customer, &new_message);
        bsal_free(new_buffer);

#else

        bsal_actor_helper_send_empty(actor,
                        bsal_actor_get_acquaintance(actor, source_index),
                        BSAL_PUSH_SEQUENCE_DATA_BLOCK_REPLY);
#endif

        if (concrete_actor->actual == concrete_actor->expected
                        || concrete_actor->actual > concrete_actor->last + 100000
                        || concrete_actor->last == 0) {

            printf("kernel actor/%d processed %" PRIu64 " entries (%d blocks) so far\n",
                            name, concrete_actor->actual,
                            concrete_actor->blocks);

            concrete_actor->last = concrete_actor->actual;
        }

        bsal_timer_stop(&timer);

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG

        bsal_timer_print(&timer);
#endif

        bsal_timer_destroy(&timer);

        bsal_dna_kmer_block_destroy(&block);

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG
        BSAL_DEBUG_MARKER("leaving call.\n");
#endif

    } else if (tag == BSAL_AGGREGATE_KERNEL_OUTPUT_REPLY) {

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG
        BSAL_DEBUG_MARKER("kernel receives reply from aggregator\n");
#endif

        source_index = *(int *)buffer;
        bsal_actor_helper_send_empty(actor,
                        bsal_actor_get_acquaintance(actor, source_index),
                        BSAL_PUSH_SEQUENCE_DATA_BLOCK_REPLY);

    } else if (tag == BSAL_ACTOR_START) {

        bsal_actor_helper_send_reply_empty(actor, BSAL_ACTOR_START_REPLY);

    } else if (tag == BSAL_RESERVE) {

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG
        printf("kmer counter kernel actor/%d is online !\n", name);
#endif

        concrete_actor->expected = *(uint64_t *)buffer;

        bsal_actor_helper_send_reply_empty(actor, BSAL_RESERVE_REPLY);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP
                    && source == bsal_actor_supervisor(actor)) {

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG
        printf("kernel actor/%d receives request to stop from actor/%d, supervisor is actor/%d\n",
                        name, source, bsal_actor_supervisor(actor));
#endif

        bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);

    } else if (tag == BSAL_SET_CUSTOMER) {

        customer = *(int *)buffer;
        concrete_actor->customer = bsal_actor_add_acquaintance(actor, customer);

#ifdef BSAL_KMER_COUNTER_KERNEL_DEBUG
        printf("BSAL_SET_CUSTOMER customer %d index %d\n", customer,
                        concrete_actor->customer);
#endif

        bsal_actor_helper_send_reply_empty(actor, BSAL_SET_CUSTOMER_REPLY);

    } else if (tag == BSAL_SET_KMER_LENGTH) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->kmer_length);

        bsal_dna_kmer_init_mock(&kmer, concrete_actor->kmer_length);
        concrete_actor->bytes_per_kmer = bsal_dna_kmer_pack_size(&kmer);
        bsal_dna_kmer_destroy(&kmer);

        bsal_actor_helper_send_reply_empty(actor, BSAL_SET_KMER_LENGTH_REPLY);
    }
}


