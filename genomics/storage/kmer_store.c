
#include "kmer_store.h"

#include "sequence_store.h"

#include <genomics/kernels/dna_kmer_counter_kernel.h>

#include <genomics/data/dna_kmer.h>
#include <genomics/data/dna_kmer_block.h>
#include <genomics/data/dna_kmer_frequency_block.h>

#include <engine/thorium/modules/message_helper.h>

#include <core/system/memory.h>

#include <core/structures/vector.h>
#include <core/structures/vector_iterator.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define MEMORY_KMER_STORE 0x51daca18

void biosal_kmer_store_init(struct thorium_actor *actor);
void biosal_kmer_store_destroy(struct thorium_actor *actor);
void biosal_kmer_store_receive(struct thorium_actor *actor, struct thorium_message *message);

void biosal_kmer_store_print(struct thorium_actor *self);
void biosal_kmer_store_push_data(struct thorium_actor *self, struct thorium_message *message);
void biosal_kmer_store_yield_reply(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script biosal_kmer_store_script = {
    .identifier = SCRIPT_KMER_STORE,
    .init = biosal_kmer_store_init,
    .destroy = biosal_kmer_store_destroy,
    .receive = biosal_kmer_store_receive,
    .size = sizeof(struct biosal_kmer_store),
    .name = "biosal_kmer_store"
};

void biosal_kmer_store_init(struct thorium_actor *self)
{
    struct biosal_kmer_store *concrete_actor;

    concrete_actor = (struct biosal_kmer_store *)thorium_actor_concrete_actor(self);

    core_memory_pool_init(&concrete_actor->persistent_memory, 0, MEMORY_KMER_STORE);
    concrete_actor->kmer_length = -1;
    concrete_actor->received = 0;

    biosal_dna_codec_init(&concrete_actor->transport_codec);

    if (biosal_dna_codec_must_use_two_bit_encoding(&concrete_actor->transport_codec,
                            thorium_actor_get_node_count(self))) {
        biosal_dna_codec_enable_two_bit_encoding(&concrete_actor->transport_codec);
    }

    biosal_dna_codec_init(&concrete_actor->storage_codec);

/* This option enables 2-bit encoding
 * for kmers.
 */
#ifdef BIOSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_STORAGE
    biosal_dna_codec_enable_two_bit_encoding(&concrete_actor->storage_codec);
#endif

    concrete_actor->last_received = 0;

    thorium_actor_add_action(self, ACTION_YIELD_REPLY, biosal_kmer_store_yield_reply);
}

void biosal_kmer_store_destroy(struct thorium_actor *self)
{
    struct biosal_kmer_store *concrete_actor;

    concrete_actor = (struct biosal_kmer_store *)thorium_actor_concrete_actor(self);

    printf("%s/%d MemoryUsageReport\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self));

    core_memory_pool_examine(&concrete_actor->persistent_memory);
    core_map_examine(&concrete_actor->table);

    if (concrete_actor->kmer_length != -1) {
        core_map_destroy(&concrete_actor->table);
    }

    biosal_dna_codec_destroy(&concrete_actor->transport_codec);
    biosal_dna_codec_destroy(&concrete_actor->storage_codec);

    concrete_actor->kmer_length = -1;

    core_memory_pool_destroy(&concrete_actor->persistent_memory);
}

void biosal_kmer_store_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    void *buffer;
    struct biosal_kmer_store *concrete_actor;
    struct biosal_dna_kmer_frequency_block block;
    void *key;
    struct core_map *kmers;
    struct core_map_iterator iterator;
    double value;
    struct biosal_dna_kmer kmer;
    struct biosal_dna_kmer *kmer_pointer;
    void *packed_kmer;
    int *frequency;
    int *bucket;
    struct core_memory_pool *ephemeral_memory;
    int customer;
    int period;
    struct biosal_dna_kmer encoded_kmer;
    char *raw_kmer;

#ifdef BIOSAL_KMER_STORE_DEBUG
    int name;
#endif

    if (thorium_actor_take_action(self, message)) {
        return;
    }

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    concrete_actor = (struct biosal_kmer_store *)thorium_actor_concrete_actor(self);
    tag = thorium_message_action(message);
    buffer = thorium_message_buffer(message);

    if (tag == ACTION_SET_KMER_LENGTH) {

        thorium_message_unpack_int(message, 0, &concrete_actor->kmer_length);

        biosal_dna_kmer_init_mock(&kmer, concrete_actor->kmer_length,
                        &concrete_actor->storage_codec, thorium_actor_get_ephemeral_memory(self));
        concrete_actor->key_length_in_bytes = biosal_dna_kmer_pack_size(&kmer,
                        concrete_actor->kmer_length, &concrete_actor->storage_codec);
        biosal_dna_kmer_destroy(&kmer, thorium_actor_get_ephemeral_memory(self));

#ifdef BIOSAL_KMER_STORE_DEBUG
        name = thorium_actor_name(self);
        printf("kmer store %d will use %d bytes for canonical kmers (k is %d)\n",
                        name, concrete_actor->key_length_in_bytes,
                        concrete_actor->kmer_length);
#endif

        core_map_init(&concrete_actor->table, concrete_actor->key_length_in_bytes,
                        sizeof(int));
        core_map_set_memory_pool(&concrete_actor->table,
                        &concrete_actor->persistent_memory);

        /*
         * Configure the map for better performance.
         */
        core_map_disable_deletion_support(&concrete_actor->table);

        /*
         * The threshold of the map is not very important because
         * requests that hit the map have to first arrive as messages,
         * which are slow.
         */
        core_map_set_threshold(&concrete_actor->table, 0.95);

        thorium_actor_send_reply_empty(self, ACTION_SET_KMER_LENGTH_REPLY);

    } else if (tag == ACTION_PUSH_KMER_BLOCK) {

        biosal_dna_kmer_frequency_block_init(&block, concrete_actor->kmer_length,
                        ephemeral_memory, &concrete_actor->transport_codec, 0);

        biosal_dna_kmer_frequency_block_unpack(&block, buffer, thorium_actor_get_ephemeral_memory(self),
                        &concrete_actor->transport_codec);

        key = core_memory_pool_allocate(ephemeral_memory, concrete_actor->key_length_in_bytes);

#ifdef BIOSAL_KMER_STORE_DEBUG
        printf("Allocating key %d bytes\n", concrete_actor->key_length_in_bytes);

        printf("kmer store receives block, kmers in table %" PRIu64 "\n",
                        core_map_size(&concrete_actor->table));
#endif

        kmers = biosal_dna_kmer_frequency_block_kmers(&block);
        core_map_iterator_init(&iterator, kmers);

        period = 2500000;

        raw_kmer = core_memory_pool_allocate(thorium_actor_get_ephemeral_memory(self),
                        concrete_actor->kmer_length + 1);

        while (core_map_iterator_has_next(&iterator)) {

            /*
             * add kmers to store
             */
            core_map_iterator_next(&iterator, (void **)&packed_kmer, (void **)&frequency);

            /* Store the kmer in 2 bit encoding
             */

            biosal_dna_kmer_init_empty(&kmer);
            biosal_dna_kmer_unpack(&kmer, packed_kmer, concrete_actor->kmer_length,
                        ephemeral_memory,
                        &concrete_actor->transport_codec);

            kmer_pointer = &kmer;

            /*
             * Get a copy of the sequence
             */
            biosal_dna_kmer_get_sequence(kmer_pointer, raw_kmer, concrete_actor->kmer_length,
                            &concrete_actor->transport_codec);

#if 0
            printf("DEBUG raw_kmer %s\n", raw_kmer);
#endif

            biosal_dna_kmer_destroy(&kmer, ephemeral_memory);

            biosal_dna_kmer_init(&encoded_kmer, raw_kmer, &concrete_actor->storage_codec,
                            thorium_actor_get_ephemeral_memory(self));

            biosal_dna_kmer_pack_store_key(&encoded_kmer, key,
                            concrete_actor->kmer_length, &concrete_actor->storage_codec,
                            thorium_actor_get_ephemeral_memory(self));

            biosal_dna_kmer_destroy(&encoded_kmer,
                            thorium_actor_get_ephemeral_memory(self));

            bucket = (int *)core_map_get(&concrete_actor->table, key);

            if (bucket == NULL) {
                /* This is the first time that this kmer is seen.
                 */
                bucket = (int *)core_map_add(&concrete_actor->table, key);
                *bucket = 0;
            }

            (*bucket) += *frequency;

            if (concrete_actor->received >= concrete_actor->last_received + period) {
                printf("kmer store %d received %" PRIu64 " kmers so far,"
                                " store has %" PRIu64 " canonical kmers, %" PRIu64 " kmers\n",
                                thorium_actor_name(self), concrete_actor->received,
                                core_map_size(&concrete_actor->table),
                                2 * core_map_size(&concrete_actor->table));

                concrete_actor->last_received = concrete_actor->received;
            }

            concrete_actor->received += *frequency;
        }

        core_memory_pool_free(ephemeral_memory, key);
        core_memory_pool_free(ephemeral_memory, raw_kmer);

        core_map_iterator_destroy(&iterator);
        biosal_dna_kmer_frequency_block_destroy(&block, thorium_actor_get_ephemeral_memory(self));

        thorium_actor_send_reply_empty(self, ACTION_PUSH_KMER_BLOCK_REPLY);

    } else if (tag == ACTION_SEQUENCE_STORE_REQUEST_PROGRESS_REPLY) {

        thorium_message_unpack_double(message, 0, &value);

        core_map_set_current_size_estimate(&concrete_actor->table, value);

    } else if (tag == ACTION_ASK_TO_STOP) {

#ifdef BIOSAL_KMER_STORE_DEBUG
        biosal_kmer_store_print(self);
#endif

        thorium_actor_ask_to_stop(self, message);

    } else if (tag == ACTION_SET_CONSUMER) {

        thorium_message_unpack_int(message, 0, &customer);

        printf("kmer store %d will use coverage distribution %d\n",
                        thorium_actor_name(self), customer);
#ifdef BIOSAL_KMER_STORE_DEBUG
#endif

        concrete_actor->customer = customer;

        thorium_actor_send_reply_empty(self, ACTION_SET_CONSUMER_REPLY);

    } else if (tag == ACTION_PUSH_DATA) {

        printf("DEBUG kmer store %d receives ACTION_PUSH_DATA\n",
                        thorium_actor_name(self));

        biosal_kmer_store_push_data(self, message);

    } else if (tag == ACTION_STORE_GET_ENTRY_COUNT) {

        thorium_actor_send_reply_uint64_t(self, ACTION_STORE_GET_ENTRY_COUNT_REPLY,
                        concrete_actor->received);
    }
}

void biosal_kmer_store_print(struct thorium_actor *self)
{
    struct core_map_iterator iterator;
    struct biosal_dna_kmer kmer;
    void *key;
    int *value;
    int coverage;
    char *sequence;
    struct biosal_kmer_store *concrete_actor;
    int maximum_length;
    int length;
    struct core_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    concrete_actor = (struct biosal_kmer_store *)thorium_actor_concrete_actor(self);
    core_map_iterator_init(&iterator, &concrete_actor->table);

    printf("map size %d\n", (int)core_map_size(&concrete_actor->table));

    maximum_length = 0;

    while (core_map_iterator_has_next(&iterator)) {
        core_map_iterator_next(&iterator, (void **)&key, (void **)&value);

        biosal_dna_kmer_init_empty(&kmer);
        biosal_dna_kmer_unpack(&kmer, key, concrete_actor->kmer_length,
                        thorium_actor_get_ephemeral_memory(self),
                        &concrete_actor->storage_codec);

        length = biosal_dna_kmer_length(&kmer, concrete_actor->kmer_length);

        /*
        printf("length %d\n", length);
        */
        if (length > maximum_length) {
            maximum_length = length;
        }
        biosal_dna_kmer_destroy(&kmer, thorium_actor_get_ephemeral_memory(self));
    }

    /*
    printf("MAx length %d\n", maximum_length);
    */

    sequence = core_memory_pool_allocate(ephemeral_memory, maximum_length + 1);
    sequence[0] = '\0';
    core_map_iterator_destroy(&iterator);
    core_map_iterator_init(&iterator, &concrete_actor->table);

    while (core_map_iterator_has_next(&iterator)) {
        core_map_iterator_next(&iterator, (void **)&key, (void **)&value);

        biosal_dna_kmer_init_empty(&kmer);
        biosal_dna_kmer_unpack(&kmer, key, concrete_actor->kmer_length,
                        thorium_actor_get_ephemeral_memory(self),
                        &concrete_actor->storage_codec);

        biosal_dna_kmer_get_sequence(&kmer, sequence, concrete_actor->kmer_length,
                        &concrete_actor->storage_codec);
        coverage = *value;

        printf("Sequence %s Coverage %d\n", sequence, coverage);

        biosal_dna_kmer_destroy(&kmer, thorium_actor_get_ephemeral_memory(self));
    }

    core_map_iterator_destroy(&iterator);
    core_memory_pool_free(ephemeral_memory, sequence);
}

void biosal_kmer_store_push_data(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_kmer_store *concrete_actor;
    int name;
    int source;

    concrete_actor = (struct biosal_kmer_store *)thorium_actor_concrete_actor(self);
    source = thorium_message_source(message);
    concrete_actor->source = source;
    name = thorium_actor_name(self);

    core_map_init(&concrete_actor->coverage_distribution, sizeof(int), sizeof(uint64_t));

    printf("kmer store %d: local table has %" PRIu64" canonical kmers (%" PRIu64 " kmers)\n",
                    name, core_map_size(&concrete_actor->table),
                    2 * core_map_size(&concrete_actor->table));

    core_map_iterator_init(&concrete_actor->iterator, &concrete_actor->table);

#ifdef BIOSAL_KMER_STORE_DEBUG
    printf("yield 1\n");
#endif

    thorium_actor_send_to_self_empty(self, ACTION_YIELD);
}

void biosal_kmer_store_yield_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_dna_kmer kmer;
    void *key;
    int *value;
    int coverage;
    struct biosal_kmer_store *concrete_actor;
    int customer;
    uint64_t *count;
    int new_count;
    void *new_buffer;
    struct thorium_message new_message;
    struct core_memory_pool *ephemeral_memory;
    int i;
    int max;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    concrete_actor = (struct biosal_kmer_store *)thorium_actor_concrete_actor(self);
    customer = concrete_actor->customer;

#if 0
    printf("YIELD REPLY\n");
#endif

    i = 0;
    max = 1024;

    key = NULL;
    value = NULL;

    while (i < max
                    && core_map_iterator_has_next(&concrete_actor->iterator)) {

        core_map_iterator_next(&concrete_actor->iterator, (void **)&key, (void **)&value);

        biosal_dna_kmer_init_empty(&kmer);
        biosal_dna_kmer_unpack(&kmer, key, concrete_actor->kmer_length,
                        ephemeral_memory,
                        &concrete_actor->storage_codec);

        coverage = *value;

        count = (uint64_t *)core_map_get(&concrete_actor->coverage_distribution, &coverage);

        if (count == NULL) {

            count = (uint64_t *)core_map_add(&concrete_actor->coverage_distribution, &coverage);

            (*count) = 0;
        }

        /* increment for the lowest kmer (canonical) */
        (*count)++;

        biosal_dna_kmer_destroy(&kmer, ephemeral_memory);

        ++i;
    }

    /* yield again if the iterator is not at the end
     */
    if (core_map_iterator_has_next(&concrete_actor->iterator)) {

#if 0
        printf("yield ! %d\n", i);
#endif

        thorium_actor_send_to_self_empty(self, ACTION_YIELD);

        return;
    }

    /*
    printf("ready...\n");
    */

    core_map_iterator_destroy(&concrete_actor->iterator);

    new_count = core_map_pack_size(&concrete_actor->coverage_distribution);

    new_buffer = thorium_actor_allocate(self, new_count);

    core_map_pack(&concrete_actor->coverage_distribution, new_buffer);

    printf("SENDING kmer store %d sends map to %d, %d bytes / %d entries\n",
                    thorium_actor_name(self),
                    customer, new_count,
                    (int)core_map_size(&concrete_actor->coverage_distribution));
#ifdef BIOSAL_KMER_STORE_DEBUG
#endif

    thorium_message_init(&new_message, ACTION_PUSH_DATA, new_count, new_buffer);

    thorium_actor_send(self, customer, &new_message);
    thorium_message_destroy(&new_message);

    core_map_destroy(&concrete_actor->coverage_distribution);

    thorium_actor_send_empty(self, concrete_actor->source,
                            ACTION_PUSH_DATA_REPLY);
}
