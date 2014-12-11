
#include "sequence_store.h"

#include <genomics/input/input_command.h>
#include <genomics/data/dna_sequence.h>

/*
 * For BIOSAL_MAXIMUM_GRAPH_STORE_COUNT
 */
#include <genomics/assembly/assembly_graph_store.h>

#include <core/structures/vector_iterator.h>
#include <engine/thorium/modules/message_helper.h>
#include <core/system/memory.h>

#include <biosal.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h>

#define MEMORY_POOL_NAME_SEQUENCE_STORE    0x84a83916

/*
#define BIOSAL_SEQUENCE_STORE_DEBUG
*/

#define MINIMUM_PERIOD 4096
#define NO_USEFUL_VALUE (-879)

void biosal_sequence_store_init(struct thorium_actor *actor);
void biosal_sequence_store_destroy(struct thorium_actor *actor);
void biosal_sequence_store_receive(struct thorium_actor *actor, struct thorium_message *message);

int biosal_sequence_store_has_error(struct thorium_actor *actor,
                struct thorium_message *message);

int biosal_sequence_store_check_open_error(struct thorium_actor *actor,
                struct thorium_message *message);
void biosal_sequence_store_push_sequence_data_block(struct thorium_actor *actor, struct thorium_message *message);
void biosal_sequence_store_reserve(struct thorium_actor *actor, struct thorium_message *message);
void biosal_sequence_store_show_progress(struct thorium_actor *actor, struct thorium_message *message);

void biosal_sequence_store_ask(struct thorium_actor *self, struct thorium_message *message);

int biosal_sequence_store_get_required_kmers(struct thorium_actor *actor, struct thorium_message *message);

struct thorium_script biosal_sequence_store_script = {
    .identifier = SCRIPT_SEQUENCE_STORE,
    .init = biosal_sequence_store_init,
    .destroy = biosal_sequence_store_destroy,
    .receive = biosal_sequence_store_receive,
    .size = sizeof(struct biosal_sequence_store),
    .name = "biosal_sequence_store"
};

void biosal_sequence_store_init(struct thorium_actor *actor)
{
    struct biosal_sequence_store *concrete_actor;
    int block_size;

    concrete_actor = thorium_actor_concrete_actor(actor);

    block_size = 67108864;

    /* 2^26 */
    core_memory_pool_init(&concrete_actor->persistent_memory, block_size,
                    MEMORY_POOL_NAME_SEQUENCE_STORE);
    core_memory_pool_disable_tracking(&concrete_actor->persistent_memory);

    core_vector_init(&concrete_actor->sequences, sizeof(struct biosal_dna_sequence));
    core_vector_set_memory_pool(&concrete_actor->sequences,
                    &concrete_actor->persistent_memory);

    concrete_actor->required_kmers = NO_USEFUL_VALUE;

    printf("DEBUG sequence store %d is online on node %d\n",
                    thorium_actor_name(actor),
                    thorium_actor_node_name(actor));
#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
#endif

    concrete_actor->received = 0;

    biosal_dna_codec_init(&concrete_actor->codec);

    if (biosal_dna_codec_must_use_two_bit_encoding(&concrete_actor->codec,
                            thorium_actor_get_node_count(actor))) {
        biosal_dna_codec_enable_two_bit_encoding(&concrete_actor->codec);
    }

    thorium_actor_add_action(actor, ACTION_SEQUENCE_STORE_ASK,
                    biosal_sequence_store_ask);

    concrete_actor->iterator_started = 0;
    concrete_actor->reservation_producer = -1;

    concrete_actor->left = -1;
    concrete_actor->last = -1;

    concrete_actor->progress_supervisor = THORIUM_ACTOR_NOBODY;

    /*
     * Payload for the first production round is
     * 2 KiB.
     * The next round is half of that, so that's
     * 1 KiB. This reduction is required because arcs generation
     * generates twice the amount of bytes in the deliveries.
     */

    concrete_actor->small_production_block_size = thorium_actor_suggested_buffer_size(actor);
    /*
    concrete_actor->big_production_block_size = 256 * 1024;
    */

    /*
     * 8 MiB
     */
    concrete_actor->big_production_block_size = 4 *1024 * 1024;
}

void biosal_sequence_store_destroy(struct thorium_actor *actor)
{
    struct biosal_sequence_store *concrete_actor;
    struct core_vector_iterator iterator;
    struct biosal_dna_sequence *sequence;

    concrete_actor = thorium_actor_concrete_actor(actor);

    core_vector_iterator_init(&iterator, &concrete_actor->sequences);

    while (core_vector_iterator_has_next(&iterator)) {

        core_vector_iterator_next(&iterator, (void**)&sequence);
        biosal_dna_sequence_destroy(sequence, &concrete_actor->persistent_memory);
    }

    core_vector_destroy(&concrete_actor->sequences);
    biosal_dna_codec_destroy(&concrete_actor->codec);

    if (concrete_actor->iterator_started) {

        printf("%s/%d starts iterator for production\n",
                        thorium_actor_script_name(actor),
                        thorium_actor_name(actor));

        core_vector_iterator_destroy(&concrete_actor->iterator);
        concrete_actor->iterator_started = 0;
    }

    core_memory_pool_destroy(&concrete_actor->persistent_memory);
}

void biosal_sequence_store_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int source;
    struct biosal_sequence_store *concrete_actor;

    if (thorium_actor_take_action(actor, message)) {
        return;
    }

    tag = thorium_message_action(message);
    source = thorium_message_source(message);
    concrete_actor = thorium_actor_concrete_actor(actor);

    if (tag == ACTION_PUSH_SEQUENCE_DATA_BLOCK) {

        biosal_sequence_store_push_sequence_data_block(actor, message);

    } else if (tag == ACTION_RESERVE) {

        biosal_sequence_store_reserve(actor, message);

    } else if (tag == ACTION_ASK_TO_STOP) {

        printf("%s/%d %d dies\n",
                        thorium_actor_script_name(actor),
                        thorium_actor_name(actor),
                        thorium_actor_name(actor));
#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
#endif

        thorium_actor_ask_to_stop(actor, message);

    } else if (tag == ACTION_SEQUENCE_STORE_REQUEST_PROGRESS) {

        concrete_actor->progress_supervisor = source;

    } else if (tag == ACTION_RESET) {

#if 0
        printf("RESET\n");
#endif

        /* Destroy iterator if it is started.
         */
        if (concrete_actor->iterator_started) {
            core_vector_iterator_destroy(&concrete_actor->iterator);
            concrete_actor->iterator_started = 0;

       /*
        * THis is no longer needed
        */
#if 0
            /*
             * Assume that the second round will be for
             * arcs.
             * TODO: don't assume that, instead, add a requirement
             * that the ACTOR_RESET payload contains a desired final
             * message buffer size.
             *
             * Currently, 2048 bytes is used for ACTION_PUSH_KMER_BLOCK,
             * and 512 bytes (predicted) for ACTION_ASSEMBLY_PUSH_ARC_BLOCK.
             *
             * The reason is that arc payload are a bit larger and twice
             * in number too.
             */
            concrete_actor->production_block_size = B2ASIC_PRODUCTION_BYTE_COUNT / 2;

            /*
             * Also reset the number of required kmers so that the old
             * cached value is not being used.
             */

            concrete_actor->required_kmers = NO_USEFUL_VALUE;
#endif
            /*
             * Here, it is not required to reduce the production size.
             */
            /*
            concrete_actor->big_production_block_size /= 2;
            */
        }

        concrete_actor->left = concrete_actor->received;
        concrete_actor->last = 0;

        thorium_actor_send_reply_empty(actor, ACTION_RESET_REPLY);
    }
}

void biosal_sequence_store_push_sequence_data_block(struct thorium_actor *actor, struct thorium_message *message)
{
    uint64_t first;
    /*
    uint64_t last;
    */
    struct core_vector *new_entries;
    struct biosal_input_command payload;
    struct biosal_sequence_store *concrete_actor;
    void *buffer;
    int64_t i;

#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
    int count;
#endif

    struct biosal_dna_sequence *bucket_in_message;
    struct biosal_dna_sequence *bucket_in_store;

    buffer = thorium_message_buffer(message);
    concrete_actor = thorium_actor_concrete_actor(actor);

#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
    count = thorium_message_count(message);
    printf("DEBUG store receives ACTION_PUSH_SEQUENCE_DATA_BLOCK %d bytes\n",
                    count);
#endif

    biosal_input_command_init_empty(&payload);
    biosal_input_command_unpack(&payload, buffer, thorium_actor_get_ephemeral_memory(actor),
                    &concrete_actor->codec);

#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
    printf("DEBUG store %d biosal_sequence_store_receive command:\n",
                    thorium_actor_name(actor));

    biosal_input_command_print(&payload);
#endif

    first = biosal_input_command_store_first(&payload);
    /*
    last = biosal_input_command_store_last(&payload);
    */
    new_entries = biosal_input_command_entries(&payload);

#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
    printf("DEBUG store %d biosal_sequence_store_push_sequence_data_block entries %d\n",
                    thorium_actor_name(actor),
                    (int)core_vector_size(new_entries));
#endif

    for (i = 0; i < core_vector_size(new_entries); i++) {

        if (concrete_actor->received % 1000000 == 0) {
            biosal_sequence_store_show_progress(actor, message);
        }

        bucket_in_message = (struct biosal_dna_sequence *)core_vector_at(new_entries,
                        i);

        bucket_in_store = (struct biosal_dna_sequence *)core_vector_at(&concrete_actor->sequences,
                        first + i);

        /* join the bucket, this load DNA into the store
         */
        /*
        *bucket_in_store = *bucket_in_message;
        */

#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
        if (i == 0) {
            printf("DEBUG first in payload\n");
            printf("DEBUG-thor i %d bucket_in_store %p bucket_in_message %p\n",
                        (int)i,
                        (void *)bucket_in_store, (void *)bucket_in_message);

            printf("DEBUG i %d first %d size %d store size %d\n",
                   (int)i, (int)first,
                   (int)core_vector_size(new_entries),
                   (int)core_vector_size(&concrete_actor->sequences));

            biosal_dna_sequence_print(bucket_in_message);
        }
#endif

        biosal_dna_sequence_init_copy(bucket_in_store, bucket_in_message,
                        &concrete_actor->codec, &concrete_actor->persistent_memory);

        concrete_actor->received++;

#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
        printf("%" PRId64 "/%" PRId64 "\n",
                        concrete_actor->received,
                        core_vector_size(&concrete_actor->sequences));
#endif

        if (concrete_actor->received == concrete_actor->expected) {
            biosal_sequence_store_show_progress(actor, message);

            concrete_actor->left = concrete_actor->received;
            concrete_actor->last = 0;
        }
    }

#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
    printf("DONE.\n");
#endif

    /* The DNA sequences are kept and are not
     * destroyed.
     */
    /* free payload
     */

    biosal_input_command_destroy(&payload, thorium_actor_get_ephemeral_memory(actor));

    thorium_actor_send_reply_empty(actor, ACTION_PUSH_SEQUENCE_DATA_BLOCK_REPLY);
}

void biosal_sequence_store_reserve(struct thorium_actor *actor, struct thorium_message *message)
{
    uint64_t amount;
    int i;
    void *buffer;
    struct biosal_dna_sequence *dna_sequence;
    struct biosal_sequence_store *concrete_actor;
    int source;

    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);
    amount = *(uint64_t*)buffer;
    concrete_actor = thorium_actor_concrete_actor(actor);

    concrete_actor->expected = amount;

    concrete_actor->reservation_producer = source;
    printf("DEBUG store %d reserves %" PRIu64 " buckets\n",
                    thorium_actor_name(actor),
                    amount);

    core_vector_resize(&concrete_actor->sequences, amount);

#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
    printf("DEBUG store %d now has %d buckets\n",
                    thorium_actor_name(actor),
                    (int)core_vector_size(&concrete_actor->sequences));
#endif

    for ( i = 0; i < core_vector_size(&concrete_actor->sequences); i++) {
        /*
         * initialize sequences with empty things
         */
        dna_sequence = (struct biosal_dna_sequence *)core_vector_at(&concrete_actor->sequences,
                        i);

        biosal_dna_sequence_init(dna_sequence, NULL, &concrete_actor->codec, &concrete_actor->persistent_memory);
    }

    thorium_actor_send_reply_empty(actor, ACTION_RESERVE_REPLY);

    if (concrete_actor->expected == 0) {

        biosal_sequence_store_show_progress(actor, message);
    }
}

void biosal_sequence_store_show_progress(struct thorium_actor *actor, struct thorium_message *message)
{
    struct biosal_sequence_store *concrete_actor;

    concrete_actor = thorium_actor_concrete_actor(actor);

    printf("sequence store %d has %" PRId64 "/%" PRId64 " entries\n",
                    thorium_actor_name(actor),
                    concrete_actor->received,
                    core_vector_size(&concrete_actor->sequences));

    /* core_memory_pool_examine(&concrete_actor->persistent_memory); */

    if (concrete_actor->received == concrete_actor->expected) {

        thorium_actor_send_empty(actor,
                                concrete_actor->reservation_producer,
                        ACTION_SEQUENCE_STORE_READY);
    }
}

void biosal_sequence_store_ask(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_sequence_store *concrete_actor;
    struct biosal_input_command payload;
    struct biosal_dna_sequence *sequence;
    int new_count;
    void *new_buffer;
    struct thorium_message new_message;
    int entry_count;
    float completion;
    int name;
    int period;
    double ratio;
    int kmer_length;
    int required_kmers;
    int kmers;
    int length;
    int sequence_kmers;
    struct core_memory_pool *ephemeral_memory_pool;

    ephemeral_memory_pool = thorium_actor_get_ephemeral_memory(self);
    required_kmers = biosal_sequence_store_get_required_kmers(self, message);
    thorium_message_unpack_int(message, 0, &kmer_length);

    name = thorium_actor_name(self);
#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
#endif

    concrete_actor = thorium_actor_concrete_actor(self);

    if (concrete_actor->received != concrete_actor->expected) {
        printf("Error: sequence store %d is not ready %" PRIu64 "/%" PRIu64 " (reservation producer %d)\n",
                        name,
                        concrete_actor->received, concrete_actor->expected,
                        concrete_actor->reservation_producer);
    }

    if (!concrete_actor->iterator_started) {
        core_vector_iterator_init(&concrete_actor->iterator, &concrete_actor->sequences);

        concrete_actor->iterator_started = 1;
    }

    /* the metadata are not important here.
     * This is just a container.
     */
    biosal_input_command_init(&payload, -1, 0, 0, ephemeral_memory_pool);

    kmers = 0;

    while ( core_vector_iterator_has_next(&concrete_actor->iterator)
                    && kmers < required_kmers) {

        core_vector_iterator_next(&concrete_actor->iterator, (void **)&sequence);

        /*printf("ADDING %d\n", i);*/
        biosal_input_command_add_entry(&payload, sequence, &concrete_actor->codec,
                        thorium_actor_get_ephemeral_memory(self));

        length = biosal_dna_sequence_length(sequence);

        sequence_kmers = length - kmer_length + 1;
        kmers += sequence_kmers;

        /*
        printf("Yielded %d kmers... %d/%d\n",
                        sequence_kmers, kmers, required_kmers);
                        */
    }

    entry_count = biosal_input_command_entry_count(&payload);

    if (entry_count > 0) {
        new_count = biosal_input_command_pack_size(&payload,
                        &concrete_actor->codec);
        new_buffer = thorium_actor_allocate(self, new_count);

        biosal_input_command_pack(&payload, new_buffer,
                        &concrete_actor->codec);

        thorium_message_init(&new_message, ACTION_PUSH_SEQUENCE_DATA_BLOCK, new_count, new_buffer);

        thorium_actor_send_reply(self, &new_message);
        thorium_message_destroy(&new_message);

#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
        printf("store/%d fulfill order\n", name);
#endif

        concrete_actor->left -= entry_count;

    } else {
        thorium_actor_send_reply_empty(self, ACTION_SEQUENCE_STORE_ASK_REPLY);
#ifdef BIOSAL_SEQUENCE_STORE_DEBUG
        printf("store/%d can not fulfill order\n", name);
#endif
    }

    /* Show something every 0.05
     */
    period = concrete_actor->received / 20;

    if (period < MINIMUM_PERIOD) {
        period = MINIMUM_PERIOD;
    }

    if (concrete_actor->last >= 0
                    && (concrete_actor->last == 0
                    || concrete_actor->left < concrete_actor->last - period
                    || concrete_actor->left == 0)) {

        completion = 1.0;

        if (concrete_actor->received != 0) {
            completion = (concrete_actor->left + 0.0) / concrete_actor->received;
        }

        printf("sequence store %d has %" PRId64 "/%" PRId64 " (%.2f) entries left to produce\n",
                        name,
                        concrete_actor->left, concrete_actor->received,
                        completion);

        concrete_actor->last = concrete_actor->left;

        if (concrete_actor->last == 0) {
            concrete_actor->last = -1;
        }
    }

    biosal_input_command_destroy(&payload, thorium_actor_get_ephemeral_memory(self));

    /*
     * Send a progress report to the supervisor of progression
     */
    if (concrete_actor->progress_supervisor != THORIUM_ACTOR_NOBODY) {
        ratio = (concrete_actor->received - concrete_actor->left + 0.0) / concrete_actor->received;

        if (ratio >= 0.16) {

            thorium_actor_send_double(self,
                                    concrete_actor->progress_supervisor,
                            ACTION_SEQUENCE_STORE_REQUEST_PROGRESS_REPLY,
                            ratio);
            concrete_actor->progress_supervisor = THORIUM_ACTOR_NOBODY;
        }
    }
}

int biosal_sequence_store_get_required_kmers(struct thorium_actor *actor, struct thorium_message *message)
{
    size_t sum_of_buffer_sizes;
    int minimum_end_buffer_size_in_bytes;
    int minimum_end_buffer_size_in_nucleotides;
    int minimum_end_buffer_size_in_ascii_kmers;
    int total_kmer_stores;
    int nodes;
    int workers;
    struct biosal_sequence_store *concrete_actor;
    int kmer_length;
    int stores_per_node;
    int position;
    int count;
    int production_block_size;

    count = thorium_message_count(message);
    concrete_actor = thorium_actor_concrete_actor(actor);

    if (concrete_actor->required_kmers != NO_USEFUL_VALUE) {
        return concrete_actor->required_kmers;
    }

    workers = thorium_actor_node_worker_count(actor);
    nodes = thorium_actor_get_node_count(actor);

    /*
     * biosal_assembly_graph_store_get_store_count_per_node is a trait
     * and any actor type can use that.
     */
    stores_per_node = biosal_assembly_graph_store_get_store_count_per_node(actor);

    position = 0;
    position += thorium_message_unpack_int(message, 0, &kmer_length);


    /*
     * A bigger buffer means that the source wants a big message.
     */
    if (count == 8) {
        total_kmer_stores = nodes * stores_per_node;
        production_block_size = concrete_actor->small_production_block_size;

    } else {
        /*
         * The current actor does not care what is being done with the
         * message down the road.
         *
         * Request a block size of around 8 MiB.
         */
        total_kmer_stores = 1;

        /*
         * Use a larger buffer size
         */
        production_block_size = concrete_actor->big_production_block_size;
    }

    if (kmer_length <= 0) {
        printf("%s/%d Error, invalid kmer length: %d\n",
                        thorium_actor_script_name(actor),
                        thorium_actor_name(actor),
                        kmer_length);
        return 0;
    }

    minimum_end_buffer_size_in_bytes = production_block_size;
    sum_of_buffer_sizes = minimum_end_buffer_size_in_bytes * total_kmer_stores;

    /* Assume 1 byte per nucleotide since transportation does not use 2-bit encoding in the
     * DNA codec.
     *
     * However, the 2-bit DNA codec is used for the graph.
     *
     * Update: use 2-bit encoding everywhere.
     */

    minimum_end_buffer_size_in_nucleotides = minimum_end_buffer_size_in_bytes;

#if 0
    /*
     * Check if two-bit encoding is being used.
     */
    if (thorium_actor_get_node_count(actor) >= BIOSAL_DNA_CODEC_MINIMUM_NODE_COUNT_FOR_TWO_BIT) {
        minimum_end_buffer_size_in_nucleotides *= 4;
    }
#endif

    minimum_end_buffer_size_in_ascii_kmers = minimum_end_buffer_size_in_nucleotides / kmer_length;

    concrete_actor->required_kmers = minimum_end_buffer_size_in_ascii_kmers * total_kmer_stores;

    printf("INFO KmerLength %d Workers: %d Consumers: %d BufferSizeForConsumer: %d BufferSizeForWorker: %zu required_kmers %d\n",
                    kmer_length,
                    workers,
                    total_kmer_stores,
                    minimum_end_buffer_size_in_bytes,
                    sum_of_buffer_sizes,
                    concrete_actor->required_kmers);

    return concrete_actor->required_kmers;
}
