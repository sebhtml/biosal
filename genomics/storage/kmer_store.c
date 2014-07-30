
#include "kmer_store.h"

#include "sequence_store.h"

#include <genomics/kernels/dna_kmer_counter_kernel.h>
#include <genomics/data/dna_kmer.h>
#include <genomics/data/dna_kmer_block.h>
#include <genomics/data/dna_kmer_frequency_block.h>

#include <core/helpers/message_helper.h>
#include <core/helpers/actor_helper.h>
#include <core/system/memory.h>
#include <core/structures/vector.h>
#include <core/structures/vector_iterator.h>

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

    bsal_dna_codec_init(&concrete_actor->transport_codec);

    if (bsal_actor_get_node_count(self) >= BSAL_DNA_CODEC_MINIMUM_NODE_COUNT_FOR_TWO_BIT) {
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_TRANSPORT
        bsal_dna_codec_enable_two_bit_encoding(&concrete_actor->transport_codec);
#endif
    }

    bsal_dna_codec_init(&concrete_actor->storage_codec);

/* This option enables 2-bit encoding
 * for kmers.
 */
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_STORAGE
    bsal_dna_codec_enable_two_bit_encoding(&concrete_actor->storage_codec);
#endif

    concrete_actor->last_received = 0;

    bsal_actor_register(self, BSAL_ACTOR_YIELD_REPLY, bsal_kmer_store_yield_reply);
}

void bsal_kmer_store_destroy(struct bsal_actor *self)
{
    struct bsal_kmer_store *concrete_actor;

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);

    if (concrete_actor->kmer_length != -1) {
        bsal_map_destroy(&concrete_actor->table);
    }

    bsal_dna_codec_destroy(&concrete_actor->transport_codec);
    bsal_dna_codec_destroy(&concrete_actor->storage_codec);

    concrete_actor->kmer_length = -1;
}

void bsal_kmer_store_receive(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    void *buffer;
    struct bsal_kmer_store *concrete_actor;
    struct bsal_dna_kmer_frequency_block block;
    void *key;
    struct bsal_map *kmers;
    struct bsal_map_iterator iterator;
    double value;
    struct bsal_dna_kmer kmer;
    struct bsal_dna_kmer *kmer_pointer;
    void *packed_kmer;
    int *frequency;
    int *bucket;
    struct bsal_memory_pool *ephemeral_memory;
    int customer;
    int period;
    struct bsal_dna_kmer encoded_kmer;
    char *raw_kmer;

#ifdef BSAL_KMER_STORE_DEBUG
    int name;
#endif

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);

    if (tag == BSAL_SET_KMER_LENGTH) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->kmer_length);

        bsal_dna_kmer_init_mock(&kmer, concrete_actor->kmer_length,
                        &concrete_actor->storage_codec, bsal_actor_get_ephemeral_memory(self));
        concrete_actor->key_length_in_bytes = bsal_dna_kmer_pack_size(&kmer,
                        concrete_actor->kmer_length, &concrete_actor->storage_codec);
        bsal_dna_kmer_destroy(&kmer, bsal_actor_get_ephemeral_memory(self));

#ifdef BSAL_KMER_STORE_DEBUG
        name = bsal_actor_get_name(self);
        printf("kmer store %d will use %d bytes for canonical kmers (k is %d)\n",
                        name, concrete_actor->key_length_in_bytes,
                        concrete_actor->kmer_length);
#endif

        bsal_map_init(&concrete_actor->table, concrete_actor->key_length_in_bytes,
                        sizeof(int));

        /*
         * Configure the map for better performance.
         */
        bsal_map_disable_deletion_support(&concrete_actor->table);

        /*
         * The threshold of the map is not very important because
         * requests that hit the map have to first arrive as messages,
         * which are slow.
         */
        bsal_map_set_threshold(&concrete_actor->table, 0.95);

        bsal_actor_helper_send_reply_empty(self, BSAL_SET_KMER_LENGTH_REPLY);

    } else if (tag == BSAL_PUSH_KMER_BLOCK) {

        bsal_dna_kmer_frequency_block_init(&block, concrete_actor->kmer_length,
                        ephemeral_memory, &concrete_actor->transport_codec, 0);

        bsal_dna_kmer_frequency_block_unpack(&block, buffer, bsal_actor_get_ephemeral_memory(self),
                        &concrete_actor->transport_codec);

        key = bsal_memory_pool_allocate(ephemeral_memory, concrete_actor->key_length_in_bytes);

#ifdef BSAL_KMER_STORE_DEBUG
        printf("Allocating key %d bytes\n", concrete_actor->key_length_in_bytes);

        printf("kmer store receives block, kmers in table %" PRIu64 "\n",
                        bsal_map_size(&concrete_actor->table));
#endif

        kmers = bsal_dna_kmer_frequency_block_kmers(&block);
        bsal_map_iterator_init(&iterator, kmers);

        period = 2500000;

        raw_kmer = bsal_memory_pool_allocate(bsal_actor_get_ephemeral_memory(self),
                        concrete_actor->kmer_length + 1);

        while (bsal_map_iterator_has_next(&iterator)) {

            /*
             * add kmers to store
             */
            bsal_map_iterator_next(&iterator, (void **)&packed_kmer, (void **)&frequency);

            /* Store the kmer in 2 bit encoding
             */

            bsal_dna_kmer_init_empty(&kmer);
            bsal_dna_kmer_unpack(&kmer, packed_kmer, concrete_actor->kmer_length,
                        ephemeral_memory,
                        &concrete_actor->transport_codec);

            kmer_pointer = &kmer;

            /*
             * Get a copy of the sequence
             */
            bsal_dna_kmer_get_sequence(kmer_pointer, raw_kmer, concrete_actor->kmer_length,
                            &concrete_actor->transport_codec);

#if 0
            printf("DEBUG raw_kmer %s\n", raw_kmer);
#endif

            bsal_dna_kmer_destroy(&kmer, ephemeral_memory);

            bsal_dna_kmer_init(&encoded_kmer, raw_kmer, &concrete_actor->storage_codec,
                            bsal_actor_get_ephemeral_memory(self));

            bsal_dna_kmer_pack_store_key(&encoded_kmer, key,
                            concrete_actor->kmer_length, &concrete_actor->storage_codec,
                            bsal_actor_get_ephemeral_memory(self));

            bsal_dna_kmer_destroy(&encoded_kmer,
                            bsal_actor_get_ephemeral_memory(self));

            bucket = (int *)bsal_map_get(&concrete_actor->table, key);

            if (bucket == NULL) {
                /* This is the first time that this kmer is seen.
                 */
                bucket = (int *)bsal_map_add(&concrete_actor->table, key);
                *bucket = 0;
            }

            (*bucket) += *frequency;

            if (concrete_actor->received >= concrete_actor->last_received + period) {
                printf("kmer store %d received %" PRIu64 " kmers so far,"
                                " store has %" PRIu64 " canonical kmers, %" PRIu64 " kmers\n",
                                bsal_actor_get_name(self), concrete_actor->received,
                                bsal_map_size(&concrete_actor->table),
                                2 * bsal_map_size(&concrete_actor->table));

                concrete_actor->last_received = concrete_actor->received;
            }

            concrete_actor->received += *frequency;
        }

        bsal_memory_pool_free(ephemeral_memory, key);
        bsal_memory_pool_free(ephemeral_memory, raw_kmer);

        bsal_map_iterator_destroy(&iterator);
        bsal_dna_kmer_frequency_block_destroy(&block, bsal_actor_get_ephemeral_memory(self));

        bsal_actor_helper_send_reply_empty(self, BSAL_PUSH_KMER_BLOCK_REPLY);

    } else if (tag == BSAL_SEQUENCE_STORE_REQUEST_PROGRESS_REPLY) {

        bsal_message_helper_unpack_double(message, 0, &value);

        bsal_map_set_current_size_estimate(&concrete_actor->table, value);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

#ifdef BSAL_KMER_STORE_DEBUG
        bsal_kmer_store_print(self);
#endif

        bsal_actor_helper_ask_to_stop(self, message);

    } else if (tag == BSAL_ACTOR_SET_CONSUMER) {

        bsal_message_helper_unpack_int(message, 0, &customer);

        printf("kmer store %d will use coverage distribution %d\n",
                        bsal_actor_get_name(self), customer);
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
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);
    bsal_map_iterator_init(&iterator, &concrete_actor->table);

    printf("map size %d\n", (int)bsal_map_size(&concrete_actor->table));

    maximum_length = 0;

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, (void **)&key, (void **)&value);

        bsal_dna_kmer_init_empty(&kmer);
        bsal_dna_kmer_unpack(&kmer, key, concrete_actor->kmer_length,
                        bsal_actor_get_ephemeral_memory(self),
                        &concrete_actor->storage_codec);

        length = bsal_dna_kmer_length(&kmer, concrete_actor->kmer_length);

        /*
        printf("length %d\n", length);
        */
        if (length > maximum_length) {
            maximum_length = length;
        }
        bsal_dna_kmer_destroy(&kmer, bsal_actor_get_ephemeral_memory(self));
    }

    /*
    printf("MAx length %d\n", maximum_length);
    */

    sequence = bsal_memory_pool_allocate(ephemeral_memory, maximum_length + 1);
    sequence[0] = '\0';
    bsal_map_iterator_destroy(&iterator);
    bsal_map_iterator_init(&iterator, &concrete_actor->table);

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, (void **)&key, (void **)&value);

        bsal_dna_kmer_init_empty(&kmer);
        bsal_dna_kmer_unpack(&kmer, key, concrete_actor->kmer_length,
                        bsal_actor_get_ephemeral_memory(self),
                        &concrete_actor->storage_codec);

        bsal_dna_kmer_get_sequence(&kmer, sequence, concrete_actor->kmer_length,
                        &concrete_actor->storage_codec);
        coverage = *value;

        printf("Sequence %s Coverage %d\n", sequence, coverage);

        bsal_dna_kmer_destroy(&kmer, bsal_actor_get_ephemeral_memory(self));
    }

    bsal_map_iterator_destroy(&iterator);
    bsal_memory_pool_free(ephemeral_memory, sequence);
}

void bsal_kmer_store_push_data(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_kmer_store *concrete_actor;
    int name;
    int source;

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);
    source = bsal_message_source(message);
    concrete_actor->source = bsal_actor_add_acquaintance(self, source);
    name = bsal_actor_get_name(self);

    bsal_map_init(&concrete_actor->coverage_distribution, sizeof(int), sizeof(uint64_t));

    printf("kmer store %d: local table has %" PRIu64" canonical kmers (%" PRIu64 " kmers)\n",
                    name, bsal_map_size(&concrete_actor->table),
                    2 * bsal_map_size(&concrete_actor->table));

    bsal_map_iterator_init(&concrete_actor->iterator, &concrete_actor->table);

#ifdef BSAL_KMER_STORE_DEBUG
    printf("yield 1\n");
#endif

    bsal_actor_helper_send_to_self_empty(self, BSAL_ACTOR_YIELD);
}

void bsal_kmer_store_yield_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_dna_kmer kmer;
    void *key;
    int *value;
    int coverage;
    struct bsal_kmer_store *concrete_actor;
    int customer;
    uint64_t *count;
    int new_count;
    void *new_buffer;
    struct bsal_message new_message;
    struct bsal_memory_pool *ephemeral_memory;
    int i;
    int max;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);
    customer = bsal_actor_get_acquaintance(self, concrete_actor->customer);

#if 0
    printf("YIELD REPLY\n");
#endif

    i = 0;
    max = 1024;

    key = NULL;
    value = NULL;

    while (i < max
                    && bsal_map_iterator_has_next(&concrete_actor->iterator)) {

        bsal_map_iterator_next(&concrete_actor->iterator, (void **)&key, (void **)&value);

        bsal_dna_kmer_init_empty(&kmer);
        bsal_dna_kmer_unpack(&kmer, key, concrete_actor->kmer_length,
                        ephemeral_memory,
                        &concrete_actor->storage_codec);

        coverage = *value;

        count = (uint64_t *)bsal_map_get(&concrete_actor->coverage_distribution, &coverage);

        if (count == NULL) {

            count = (uint64_t *)bsal_map_add(&concrete_actor->coverage_distribution, &coverage);

            (*count) = 0;
        }

        /* increment for the lowest kmer (canonical) */
        (*count)++;

        bsal_dna_kmer_destroy(&kmer, ephemeral_memory);

        ++i;
    }

    /* yield again if the iterator is not at the end
     */
    if (bsal_map_iterator_has_next(&concrete_actor->iterator)) {

#if 0
        printf("yield ! %d\n", i);
#endif

        bsal_actor_helper_send_to_self_empty(self, BSAL_ACTOR_YIELD);

        return;
    }

    /*
    printf("ready...\n");
    */

    bsal_map_iterator_destroy(&concrete_actor->iterator);

    new_count = bsal_map_pack_size(&concrete_actor->coverage_distribution);

    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

    bsal_map_pack(&concrete_actor->coverage_distribution, new_buffer);

    printf("SENDING kmer store %d sends map to %d, %d bytes / %d entries\n", customer, new_count,
                    bsal_actor_get_name(self),
                    (int)bsal_map_size(&concrete_actor->coverage_distribution));
#ifdef BSAL_KMER_STORE_DEBUG
#endif

    bsal_message_init(&new_message, BSAL_PUSH_DATA, new_count, new_buffer);

    bsal_actor_send(self, customer, &new_message);
    bsal_message_destroy(&new_message);

    bsal_map_destroy(&concrete_actor->coverage_distribution);

    bsal_actor_helper_send_empty(self, bsal_actor_get_acquaintance(self, concrete_actor->source),
                            BSAL_PUSH_DATA_REPLY);

}
