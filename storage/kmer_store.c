
#include "kmer_store.h"

#include <kernels/dna_kmer_counter_kernel.h>

#include <data/dna_kmer_block.h>
#include <data/coverage_distribution.h>

#include <helpers/message_helper.h>
#include <helpers/actor_helper.h>

#include <data/dna_kmer.h>
#include <system/memory.h>

#include <structures/map_iterator.h>
#include <structures/vector.h>
#include <structures/vector_iterator.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

struct bsal_script bsal_kmer_store_script = {
    .name = BSAL_KMER_STORE_SCRIPT,
    .init = bsal_kmer_store_init,
    .destroy = bsal_kmer_store_destroy,
    .receive = bsal_kmer_store_receive,
    .size = sizeof(struct bsal_kmer_store),
    .description = "kmer_store"
};

void bsal_kmer_store_init(struct bsal_actor *self)
{
    struct bsal_kmer_store *concrete_actor;

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);
    concrete_actor->kmer_length = -1;
    concrete_actor->received = 0;

    bsal_dna_codec_init(&concrete_actor->codec);

    concrete_actor->last_received = 0;

    bsal_memory_pool_init(&concrete_actor->ephemeral_memory, 2097152);
    bsal_memory_pool_disable_tracking(&concrete_actor->ephemeral_memory);
}

void bsal_kmer_store_destroy(struct bsal_actor *self)
{
    struct bsal_kmer_store *concrete_actor;

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);

    if (concrete_actor->kmer_length != -1) {
        bsal_map_destroy(&concrete_actor->table);
    }

    bsal_dna_codec_destroy(&concrete_actor->codec);

    concrete_actor->kmer_length = -1;
    bsal_memory_pool_destroy(&concrete_actor->ephemeral_memory);
}

void bsal_kmer_store_receive(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    void *buffer;
    struct bsal_kmer_store *concrete_actor;
    struct bsal_dna_kmer kmer;
    struct bsal_dna_kmer_block block;
    void *key;
    struct bsal_vector *kmers;
    struct bsal_vector_iterator iterator;
    struct bsal_dna_kmer *kmer_pointer;
    int *bucket;
    int customer;
    int period;

#ifdef BSAL_KMER_STORE_DEBUG
    int name;
#endif

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);

    bsal_memory_pool_free_all(&concrete_actor->ephemeral_memory);

    if (tag == BSAL_SET_KMER_LENGTH) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->kmer_length);

        bsal_dna_kmer_init_mock(&kmer, concrete_actor->kmer_length,
                        &concrete_actor->codec, &concrete_actor->ephemeral_memory);
        concrete_actor->key_length_in_bytes = bsal_dna_kmer_pack_size(&kmer,
                        concrete_actor->kmer_length);
        bsal_dna_kmer_destroy(&kmer, &concrete_actor->ephemeral_memory);

#ifdef BSAL_KMER_STORE_DEBUG
        name = bsal_actor_name(self);
        printf("kmer store %d will use %d bytes for canonical kmers (k is %d)\n",
                        name, concrete_actor->key_length_in_bytes,
                        concrete_actor->kmer_length);
#endif

        bsal_map_init(&concrete_actor->table, concrete_actor->key_length_in_bytes,
                        sizeof(int));

        bsal_actor_helper_send_reply_empty(self, BSAL_SET_KMER_LENGTH_REPLY);

    } else if (tag == BSAL_PUSH_KMER_BLOCK) {

        bsal_dna_kmer_block_unpack(&block, buffer, &concrete_actor->ephemeral_memory);

        key = bsal_allocate(concrete_actor->key_length_in_bytes);

#ifdef BSAL_KMER_STORE_DEBUG
        printf("Allocating key %d bytes\n", concrete_actor->key_length_in_bytes);

        printf("kmer store receives block, kmers in table %" PRIu64 "\n",
                        bsal_map_size(&concrete_actor->table));
#endif

        kmers = bsal_dna_kmer_block_kmers(&block);
        bsal_vector_iterator_init(&iterator, kmers);

        period = 250000;

        while (bsal_vector_iterator_has_next(&iterator)) {

            /*
             * add kmers to store
             */
            bsal_vector_iterator_next(&iterator, (void **)&kmer_pointer);

            bsal_dna_kmer_pack_store_key(kmer_pointer, key,
                            concrete_actor->kmer_length, &concrete_actor->codec,
                            &concrete_actor->ephemeral_memory);

            bucket = (int *)bsal_map_get(&concrete_actor->table, key);

            if (bucket == NULL) {
                /* This is the first time that this kmer is seen.
                 */
                bucket = (int *)bsal_map_add(&concrete_actor->table, key);
                *bucket = 0;
            }

            (*bucket)++;

            if (concrete_actor->received >= concrete_actor->last_received + period) {
                printf("kmer store %d received %" PRIu64 " kmers so far,"
                                " store has %" PRIu64 " canonical kmers, %" PRIu64 " kmers\n",
                                bsal_actor_name(self), concrete_actor->received,
                                bsal_map_size(&concrete_actor->table),
                                2 * bsal_map_size(&concrete_actor->table));

                concrete_actor->last_received = concrete_actor->received;
            }
            concrete_actor->received++;
        }

        bsal_free(key);

        bsal_vector_iterator_destroy(&iterator);
        bsal_dna_kmer_block_destroy(&block, &concrete_actor->ephemeral_memory);

        bsal_actor_helper_send_reply_empty(self, BSAL_PUSH_KMER_BLOCK_REPLY);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

#ifdef BSAL_KMER_STORE_DEBUG
        bsal_kmer_store_print(self);
#endif

        bsal_actor_helper_ask_to_stop(self, message);

    } else if (tag == BSAL_ACTOR_SET_CONSUMER) {

        bsal_message_helper_unpack_int(message, 0, &customer);

        printf("kmer store %d will use coverage distribution %d\n",
                        bsal_actor_name(self), customer);
#ifdef BSAL_KMER_STORE_DEBUG
#endif

        concrete_actor->customer = bsal_actor_add_acquaintance(self, customer);

        bsal_actor_helper_send_reply_empty(self, BSAL_ACTOR_SET_CONSUMER_REPLY);

    } else if (tag == BSAL_PUSH_DATA) {

        bsal_kmer_store_push_data(self, message);

    } else if (tag == BSAL_STORE_GET_ENTRY_COUNT) {

        bsal_actor_helper_send_reply_uint64_t(self, BSAL_STORE_GET_ENTRY_COUNT_REPLY,
                        concrete_actor->received);
    }
}

void bsal_kmer_store_print(struct bsal_actor *self)
{
    struct bsal_map_iterator iterator;
    struct bsal_dna_kmer kmer;
    void *key;
    int *value;
    int coverage;
    char *sequence;
    struct bsal_kmer_store *concrete_actor;
    int maximum_length;
    int length;

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);
    bsal_map_iterator_init(&iterator, &concrete_actor->table);

    printf("map size %d\n", (int)bsal_map_size(&concrete_actor->table));

    maximum_length = 0;

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, (void **)&key, (void **)&value);
        bsal_dna_kmer_unpack(&kmer, key, concrete_actor->kmer_length,
                        &concrete_actor->ephemeral_memory);

        length = bsal_dna_kmer_length(&kmer, concrete_actor->kmer_length);

        /*
        printf("length %d\n", length);
        */
        if (length > maximum_length) {
            maximum_length = length;
        }
        bsal_dna_kmer_destroy(&kmer, &concrete_actor->ephemeral_memory);
    }

    /*
    printf("MAx length %d\n", maximum_length);
    */

    sequence = bsal_allocate(maximum_length + 1);
    sequence[0] = '\0';
    bsal_map_iterator_destroy(&iterator);
    bsal_map_iterator_init(&iterator, &concrete_actor->table);

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, (void **)&key, (void **)&value);

        bsal_dna_kmer_unpack(&kmer, key, concrete_actor->kmer_length,
                        &concrete_actor->ephemeral_memory);

        bsal_dna_kmer_get_sequence(&kmer, sequence, concrete_actor->kmer_length,
                        &concrete_actor->codec);
        coverage = *value;

        printf("Sequence %s Coverage %d\n", sequence, coverage);

        bsal_dna_kmer_destroy(&kmer, &concrete_actor->ephemeral_memory);
    }

    bsal_map_iterator_destroy(&iterator);
    bsal_free(sequence);
}

void bsal_kmer_store_push_data(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_map_iterator iterator;
    struct bsal_dna_kmer kmer;
    void *key;
    int *value;
    int coverage;
    struct bsal_kmer_store *concrete_actor;
    int customer;
    struct bsal_map coverage_distribution;
    uint64_t *count;
    int new_count;
    void *new_buffer;
    struct bsal_message new_message;
    int name;

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);
    customer = bsal_actor_get_acquaintance(self, concrete_actor->customer);
    name = bsal_actor_name(self);

    bsal_map_init(&coverage_distribution, sizeof(int), sizeof(uint64_t));

    printf("kmer store %d: local table has %" PRIu64" canonical kmers (%" PRIu64 " kmers)\n",
                    name, bsal_map_size(&concrete_actor->table),
                    2 * bsal_map_size(&concrete_actor->table));
#ifdef BSAL_KMER_STORE_DEBUG
#endif

    bsal_map_iterator_init(&iterator, &concrete_actor->table);

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, (void **)&key, (void **)&value);

        bsal_dna_kmer_unpack(&kmer, key, concrete_actor->kmer_length,
                        &concrete_actor->ephemeral_memory);

        coverage = *value;

        count = (uint64_t *)bsal_map_get(&coverage_distribution, &coverage);

        if (count == NULL) {

            count = (uint64_t *)bsal_map_add(&coverage_distribution, &coverage);

            (*count) = 0;
        }

        /* increment for the lowest kmer (canonical) */
        (*count)++;

        bsal_dna_kmer_destroy(&kmer, &concrete_actor->ephemeral_memory);
    }

    bsal_map_iterator_destroy(&iterator);

    new_count = bsal_map_pack_size(&coverage_distribution);
    new_buffer = bsal_allocate(new_count);

    bsal_map_pack(&coverage_distribution, new_buffer);

#ifdef BSAL_KMER_STORE_DEBUG
    printf("SENDING map to %d, %d bytes / %d entries\n", customer, new_count,
                    (int)bsal_map_size(&coverage_distribution));
#endif

    bsal_message_init(&new_message, BSAL_PUSH_DATA, new_count, new_buffer);

    bsal_actor_send(self, customer, &new_message);
    bsal_free(new_buffer);

    bsal_map_destroy(&coverage_distribution);

    bsal_actor_helper_send_reply_empty(self, BSAL_PUSH_DATA_REPLY);
}
