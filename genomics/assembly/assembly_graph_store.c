
#include "assembly_graph_store.h"

#include "assembly_arc_kernel.h"
#include "assembly_arc_block.h"

#include "assembly_vertex.h"

/*
 * Include storage actors for message
 * tags.
 */
#include <genomics/storage/sequence_store.h>
#include <genomics/storage/kmer_store.h>

#include <genomics/kernels/dna_kmer_counter_kernel.h>

#include <genomics/data/dna_kmer.h>
#include <genomics/data/dna_kmer_block.h>
#include <genomics/data/dna_kmer_frequency_block.h>

#include <core/helpers/message_helper.h>
#include <core/system/memory.h>

#include <core/structures/vector.h>
#include <core/structures/vector_iterator.h>

#include <core/system/debugger.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

struct bsal_script bsal_assembly_graph_store_script = {
    .identifier = BSAL_ASSEMBLY_GRAPH_STORE_SCRIPT,
    .name = "bsal_assembly_graph_store",
    .init = bsal_assembly_graph_store_init,
    .destroy = bsal_assembly_graph_store_destroy,
    .receive = bsal_assembly_graph_store_receive,
    .size = sizeof(struct bsal_assembly_graph_store),
    .author = "Sébastien Boisvert",
    .description = "Build a distributed assembly graph with actors and active messages",
    .version = "Stable"
};

void bsal_assembly_graph_store_init(struct bsal_actor *self)
{
    struct bsal_assembly_graph_store *concrete_self;

    concrete_self = (struct bsal_assembly_graph_store *)bsal_actor_concrete_actor(self);
    concrete_self->kmer_length = -1;
    concrete_self->received = 0;

    bsal_dna_codec_init(&concrete_self->transport_codec);

    if (bsal_actor_get_node_count(self) >= BSAL_DNA_CODEC_MINIMUM_NODE_COUNT_FOR_TWO_BIT) {
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_TRANSPORT
        bsal_dna_codec_enable_two_bit_encoding(&concrete_self->transport_codec);
#endif
    }

    bsal_dna_codec_init(&concrete_self->storage_codec);

/* This option enables 2-bit encoding
 * for kmers.
 */
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_STORAGE
    bsal_dna_codec_enable_two_bit_encoding(&concrete_self->storage_codec);
#endif

    concrete_self->last_received = 0;

    bsal_actor_add_route(self, BSAL_ACTOR_YIELD_REPLY, bsal_assembly_graph_store_yield_reply);
    bsal_actor_add_route(self, BSAL_PUSH_KMER_BLOCK,
                    bsal_assembly_graph_store_push_kmer_block);

    concrete_self->received_arc_count = 0;

    bsal_actor_add_route(self, BSAL_ASSEMBLY_PUSH_ARC_BLOCK,
                    bsal_assembly_graph_store_push_arc_block);

    concrete_self->received_arc_block_count = 0;

    bsal_actor_add_route(self, BSAL_ASSEMBLY_GET_SUMMARY,
                    bsal_assembly_graph_store_get_summary);

    concrete_self->summary_in_progress = 0;
}

void bsal_assembly_graph_store_destroy(struct bsal_actor *self)
{
    struct bsal_assembly_graph_store *concrete_self;

    concrete_self = (struct bsal_assembly_graph_store *)bsal_actor_concrete_actor(self);

    if (concrete_self->kmer_length != -1) {
        bsal_map_destroy(&concrete_self->table);
    }

    bsal_dna_codec_destroy(&concrete_self->transport_codec);
    bsal_dna_codec_destroy(&concrete_self->storage_codec);

    concrete_self->kmer_length = -1;
}

void bsal_assembly_graph_store_receive(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    void *buffer;
    struct bsal_assembly_graph_store *concrete_self;
    double value;
    struct bsal_dna_kmer kmer;
    struct bsal_memory_pool *ephemeral_memory;
    int customer;


    if (bsal_actor_use_route(self, message)) {
        return;
    }

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
    concrete_self = (struct bsal_assembly_graph_store *)bsal_actor_concrete_actor(self);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);

    if (tag == BSAL_SET_KMER_LENGTH) {

        bsal_message_unpack_int(message, 0, &concrete_self->kmer_length);

        bsal_dna_kmer_init_mock(&kmer, concrete_self->kmer_length,
                        &concrete_self->storage_codec, bsal_actor_get_ephemeral_memory(self));
        concrete_self->key_length_in_bytes = bsal_dna_kmer_pack_size(&kmer,
                        concrete_self->kmer_length, &concrete_self->storage_codec);
        bsal_dna_kmer_destroy(&kmer, bsal_actor_get_ephemeral_memory(self));


        bsal_map_init(&concrete_self->table, concrete_self->key_length_in_bytes,
                        sizeof(struct bsal_assembly_vertex));

        /*
         * Configure the map for better performance.
         */
        bsal_map_disable_deletion_support(&concrete_self->table);

        /*
         * The threshold of the map is not very important because
         * requests that hit the map have to first arrive as messages,
         * which are slow.
         */
        bsal_map_set_threshold(&concrete_self->table, 0.95);

        bsal_actor_send_reply_empty(self, BSAL_SET_KMER_LENGTH_REPLY);

    } else if (tag == BSAL_SEQUENCE_STORE_REQUEST_PROGRESS_REPLY) {

        bsal_message_unpack_double(message, 0, &value);

        bsal_map_set_current_size_estimate(&concrete_self->table, value);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {


        bsal_actor_ask_to_stop(self, message);

    } else if (tag == BSAL_ACTOR_SET_CONSUMER) {

        bsal_message_unpack_int(message, 0, &customer);

        printf("%s/%d will use coverage distribution %d\n",
                        bsal_actor_script_name(self),
                        bsal_actor_name(self), customer);

        concrete_self->customer = customer;

        bsal_actor_send_reply_empty(self, BSAL_ACTOR_SET_CONSUMER_REPLY);

    } else if (tag == BSAL_PUSH_DATA) {

        printf("%s/%d receives BSAL_PUSH_DATA\n",
                        bsal_actor_script_name(self),
                        bsal_actor_name(self));

        bsal_assembly_graph_store_push_data(self, message);

    } else if (tag == BSAL_STORE_GET_ENTRY_COUNT) {

        bsal_actor_send_reply_uint64_t(self, BSAL_STORE_GET_ENTRY_COUNT_REPLY,
                        concrete_self->received);

    } else if (tag == BSAL_GET_RECEIVED_ARC_COUNT) {

        bsal_actor_send_reply_uint64_t(self, BSAL_GET_RECEIVED_ARC_COUNT_REPLY,
                        concrete_self->received_arc_count);
    }
}

void bsal_assembly_graph_store_print(struct bsal_actor *self)
{
    struct bsal_map_iterator iterator;
    struct bsal_dna_kmer kmer;
    void *key;
    struct bsal_assembly_vertex *value;
    int coverage;
    char *sequence;
    struct bsal_assembly_graph_store *concrete_self;
    int maximum_length;
    int length;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
    concrete_self = (struct bsal_assembly_graph_store *)bsal_actor_concrete_actor(self);
    bsal_map_iterator_init(&iterator, &concrete_self->table);

    printf("map size %d\n", (int)bsal_map_size(&concrete_self->table));

    maximum_length = 0;

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, (void **)&key, (void **)&value);

        bsal_dna_kmer_init_empty(&kmer);
        bsal_dna_kmer_unpack(&kmer, key, concrete_self->kmer_length,
                        bsal_actor_get_ephemeral_memory(self),
                        &concrete_self->storage_codec);

        length = bsal_dna_kmer_length(&kmer, concrete_self->kmer_length);

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
    bsal_map_iterator_init(&iterator, &concrete_self->table);

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, (void **)&key, (void **)&value);

        bsal_dna_kmer_init_empty(&kmer);
        bsal_dna_kmer_unpack(&kmer, key, concrete_self->kmer_length,
                        bsal_actor_get_ephemeral_memory(self),
                        &concrete_self->storage_codec);

        bsal_dna_kmer_get_sequence(&kmer, sequence, concrete_self->kmer_length,
                        &concrete_self->storage_codec);

        coverage = bsal_assembly_vertex_coverage_depth(value);

        printf("Sequence %s Coverage %d\n", sequence, coverage);

        bsal_dna_kmer_destroy(&kmer, bsal_actor_get_ephemeral_memory(self));
    }

    bsal_map_iterator_destroy(&iterator);
    bsal_memory_pool_free(ephemeral_memory, sequence);
}

void bsal_assembly_graph_store_push_data(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_store *concrete_self;
    int name;
    int source;

    concrete_self = (struct bsal_assembly_graph_store *)bsal_actor_concrete_actor(self);
    source = bsal_message_source(message);
    concrete_self->source = source;
    name = bsal_actor_name(self);

    bsal_map_init(&concrete_self->coverage_distribution, sizeof(int), sizeof(uint64_t));

    printf("%s/%d: local table has %" PRIu64" canonical kmers (%" PRIu64 " kmers)\n",
                        bsal_actor_script_name(self),
                    name, bsal_map_size(&concrete_self->table),
                    2 * bsal_map_size(&concrete_self->table));

    bsal_map_iterator_init(&concrete_self->iterator, &concrete_self->table);


    bsal_actor_send_to_self_empty(self, BSAL_ACTOR_YIELD);
}

void bsal_assembly_graph_store_yield_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_dna_kmer kmer;
    void *key;
    struct bsal_assembly_vertex *value;
    int coverage;
    int customer;
    uint64_t *count;
    int new_count;
    void *new_buffer;
    struct bsal_message new_message;
    struct bsal_memory_pool *ephemeral_memory;
    struct bsal_assembly_graph_store *concrete_self;
    int i;
    int max;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
    concrete_self = (struct bsal_assembly_graph_store *)bsal_actor_concrete_actor(self);
    customer = concrete_self->customer;

#if 0
    printf("YIELD REPLY\n");
#endif

    i = 0;
    max = 1024;

    key = NULL;
    value = NULL;

    while (i < max
                    && bsal_map_iterator_has_next(&concrete_self->iterator)) {

        bsal_map_iterator_next(&concrete_self->iterator, (void **)&key, (void **)&value);

        bsal_dna_kmer_init_empty(&kmer);
        bsal_dna_kmer_unpack(&kmer, key, concrete_self->kmer_length,
                        ephemeral_memory,
                        &concrete_self->storage_codec);

        coverage = bsal_assembly_vertex_coverage_depth(value);

        count = (uint64_t *)bsal_map_get(&concrete_self->coverage_distribution, &coverage);

        if (count == NULL) {

            count = (uint64_t *)bsal_map_add(&concrete_self->coverage_distribution, &coverage);

            (*count) = 0;
        }

        /* increment for the lowest kmer (canonical) */
        (*count)++;

        bsal_dna_kmer_destroy(&kmer, ephemeral_memory);

        ++i;
    }

    /* yield again if the iterator is not at the end
     */
    if (bsal_map_iterator_has_next(&concrete_self->iterator)) {

#if 0
        printf("yield ! %d\n", i);
#endif

        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_YIELD);

        return;
    }

    /*
    printf("ready...\n");
    */

    bsal_map_iterator_destroy(&concrete_self->iterator);

    new_count = bsal_map_pack_size(&concrete_self->coverage_distribution);

    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

    bsal_map_pack(&concrete_self->coverage_distribution, new_buffer);

    printf("SENDING %s/%d sends map to %d, %d bytes / %d entries\n",
                        bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    customer, new_count,
                    (int)bsal_map_size(&concrete_self->coverage_distribution));

    bsal_message_init(&new_message, BSAL_PUSH_DATA, new_count, new_buffer);

    bsal_actor_send(self, customer, &new_message);
    bsal_message_destroy(&new_message);

    bsal_map_destroy(&concrete_self->coverage_distribution);

    bsal_actor_send_empty(self, concrete_self->source,
                            BSAL_PUSH_DATA_REPLY);

}

void bsal_assembly_graph_store_push_kmer_block(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_memory_pool *ephemeral_memory;
    struct bsal_dna_kmer_frequency_block block;
    struct bsal_assembly_vertex *bucket;
    void *packed_kmer;
    struct bsal_map_iterator iterator;
    struct bsal_assembly_graph_store *concrete_self;
    int tag;
    void *key;
    struct bsal_map *kmers;
    struct bsal_dna_kmer kmer;
    void *buffer;
    struct bsal_dna_kmer encoded_kmer;
    char *raw_kmer;
    int period;
    struct bsal_dna_kmer *kmer_pointer;
    int *frequency;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
    concrete_self = (struct bsal_assembly_graph_store *)bsal_actor_concrete_actor(self);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);

    /*
     * Handler for PUSH_DATA
     */

    bsal_dna_kmer_frequency_block_init(&block, concrete_self->kmer_length,
                    ephemeral_memory, &concrete_self->transport_codec, 0);

    bsal_dna_kmer_frequency_block_unpack(&block, buffer, bsal_actor_get_ephemeral_memory(self),
                    &concrete_self->transport_codec);

    key = bsal_memory_pool_allocate(ephemeral_memory, concrete_self->key_length_in_bytes);

    kmers = bsal_dna_kmer_frequency_block_kmers(&block);
    bsal_map_iterator_init(&iterator, kmers);

    period = 2500000;

    raw_kmer = bsal_memory_pool_allocate(bsal_actor_get_ephemeral_memory(self),
                    concrete_self->kmer_length + 1);

    while (bsal_map_iterator_has_next(&iterator)) {

        /*
         * add kmers to store
         */
        bsal_map_iterator_next(&iterator, (void **)&packed_kmer, (void **)&frequency);

        /* Store the kmer in 2 bit encoding
         */

        bsal_dna_kmer_init_empty(&kmer);
        bsal_dna_kmer_unpack(&kmer, packed_kmer, concrete_self->kmer_length,
                    ephemeral_memory,
                    &concrete_self->transport_codec);

        kmer_pointer = &kmer;

        /*
         * Get a copy of the sequence
         */
        bsal_dna_kmer_get_sequence(kmer_pointer, raw_kmer, concrete_self->kmer_length,
                        &concrete_self->transport_codec);

        bsal_dna_kmer_destroy(&kmer, ephemeral_memory);

        bsal_dna_kmer_init(&encoded_kmer, raw_kmer, &concrete_self->storage_codec,
                        bsal_actor_get_ephemeral_memory(self));

        bsal_dna_kmer_pack_store_key(&encoded_kmer, key,
                        concrete_self->kmer_length, &concrete_self->storage_codec,
                        bsal_actor_get_ephemeral_memory(self));

        bucket = bsal_map_get(&concrete_self->table, key);

        if (bucket == NULL) {
            /* This is the first time that this kmer is seen.
             */
            bucket = bsal_map_add(&concrete_self->table, key);

            bsal_assembly_vertex_init(bucket);

#if 0
            printf("DEBUG303 ADD_KEY");
            bsal_dna_kmer_print(&encoded_kmer, concrete_self->kmer_length,
                            &concrete_self->storage_codec, ephemeral_memory);
#endif
        }

        bsal_dna_kmer_destroy(&encoded_kmer,
                        bsal_actor_get_ephemeral_memory(self));

        bsal_assembly_vertex_increase_coverage_depth(bucket, *frequency);

        if (concrete_self->received >= concrete_self->last_received + period) {
            printf("%s/%d received %" PRIu64 " kmers so far,"
                            " store has %" PRIu64 " canonical kmers, %" PRIu64 " kmers\n",
                        bsal_actor_script_name(self),
                            bsal_actor_name(self), concrete_self->received,
                            bsal_map_size(&concrete_self->table),
                            2 * bsal_map_size(&concrete_self->table));

            concrete_self->last_received = concrete_self->received;
        }

        concrete_self->received += *frequency;
    }

    bsal_memory_pool_free(ephemeral_memory, key);
    bsal_memory_pool_free(ephemeral_memory, raw_kmer);

    bsal_map_iterator_destroy(&iterator);
    bsal_dna_kmer_frequency_block_destroy(&block, bsal_actor_get_ephemeral_memory(self));

    bsal_actor_send_reply_empty(self, BSAL_PUSH_KMER_BLOCK_REPLY);
}

void bsal_assembly_graph_store_push_arc_block(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_store *concrete_self;
    int size;
    int i;
    void *buffer;
    int count;
    struct bsal_assembly_arc_block input_block;
    struct bsal_assembly_arc *arc;
    struct bsal_memory_pool *ephemeral_memory;
    struct bsal_vector *input_arcs;
    char *sequence;

    concrete_self = (struct bsal_assembly_graph_store *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    sequence = bsal_memory_pool_allocate(ephemeral_memory, concrete_self->kmer_length + 1);

    ++concrete_self->received_arc_block_count;

    count = bsal_message_count(message);
    buffer = bsal_message_buffer(message);

    bsal_assembly_arc_block_init(&input_block, ephemeral_memory, concrete_self->kmer_length,
                    &concrete_self->transport_codec);

    bsal_assembly_arc_block_unpack(&input_block, buffer, concrete_self->kmer_length,
                    &concrete_self->transport_codec, ephemeral_memory);

    input_arcs = bsal_assembly_arc_block_get_arcs(&input_block);

    size = bsal_vector_size(input_arcs);

    for (i = 0; i < size; i++) {
        arc = bsal_vector_at(input_arcs, i);

#ifdef BSAL_ASSEMBLY_ADD_ARCS
        bsal_assembly_graph_store_add_arc(self, arc, sequence);
#endif

        ++concrete_self->received_arc_count;
    }

    bsal_assembly_arc_block_destroy(&input_block, ephemeral_memory);

    /*
     *
     * Add the arcs to the graph
     */

    bsal_actor_send_reply_empty(self, BSAL_ASSEMBLY_PUSH_ARC_BLOCK_REPLY);

    bsal_memory_pool_free(ephemeral_memory, sequence);
}

void bsal_assembly_graph_store_add_arc(struct bsal_actor *self,
                struct bsal_assembly_arc *arc, char *sequence)
{
    struct bsal_assembly_graph_store *concrete_self;
    struct bsal_dna_kmer *source;
    struct bsal_dna_kmer real_source;
    int destination;
    int type;
    struct bsal_assembly_vertex *vertex;
    void *key;
    struct bsal_memory_pool *ephemeral_memory;
    int is_canonical;

#ifdef BSAL_ASSEMBLY_GRAPH_STORE_DEBUG_ARC
    int verbose;
#endif

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
    concrete_self = (struct bsal_assembly_graph_store *)bsal_actor_concrete_actor(self);
    key = bsal_memory_pool_allocate(ephemeral_memory, concrete_self->key_length_in_bytes);

#ifdef BSAL_ASSEMBLY_GRAPH_STORE_DEBUG_ARC
    verbose = 0;

    if (concrete_self->received_arc_count == 0) {
        verbose = 1;
    }

    if (verbose) {
        printf("DEBUG BioSAL::GraphStore::AddArc\n");

        bsal_assembly_arc_print(arc, concrete_self->kmer_length, &concrete_self->transport_codec,
                    ephemeral_memory);
    }
#endif

    source = bsal_assembly_arc_source(arc);
    destination = bsal_assembly_arc_destination(arc);
    type = bsal_assembly_arc_type(arc);

    bsal_dna_kmer_get_sequence(source, sequence, concrete_self->kmer_length,
                        &concrete_self->transport_codec);

    bsal_dna_kmer_init(&real_source, sequence, &concrete_self->storage_codec,
                        ephemeral_memory);

    bsal_dna_kmer_pack_store_key(&real_source, key,
                        concrete_self->kmer_length, &concrete_self->storage_codec,
                        ephemeral_memory);

    vertex = bsal_map_get(&concrete_self->table, key);

#ifdef BSAL_DEBUGGER_ENABLE_ASSERT
    if (vertex == NULL) {
        printf("Error: vertex is NULL, key_length_in_bytes %d size %" PRIu64 "\n",
                        concrete_self->key_length_in_bytes,
                        bsal_map_size(&concrete_self->table));
    }
#endif

    BSAL_DEBUGGER_ASSERT(vertex != NULL);

#ifdef BSAL_ASSEMBLY_GRAPH_STORE_DEBUG_ARC
    if (verbose) {
        printf("DEBUG BEFORE:\n");
        bsal_assembly_vertex_print(vertex);
    }
#endif

    /*
     * Inverse the arc if the source is not canonical
     */
    is_canonical = bsal_dna_kmer_is_canonical(&real_source, concrete_self->kmer_length,
                    &concrete_self->storage_codec);

    if (!is_canonical) {

        if (type == BSAL_ARC_TYPE_PARENT) {
            type = BSAL_ARC_TYPE_CHILD;

        } else if (type == BSAL_ARC_TYPE_CHILD) {

            type = BSAL_ARC_TYPE_PARENT;
        }

        destination = bsal_dna_codec_get_complement(destination);
    }

    if (type == BSAL_ARC_TYPE_PARENT) {

        bsal_assembly_vertex_add_parent(vertex, destination);

    } else if (type == BSAL_ARC_TYPE_CHILD) {

        bsal_assembly_vertex_add_child(vertex, destination);
    }

#ifdef BSAL_ASSEMBLY_GRAPH_STORE_DEBUG_ARC
    if (verbose) {
        printf("DEBUG AFTER:\n");
        bsal_assembly_vertex_print(vertex);
    }
#endif

    bsal_memory_pool_free(ephemeral_memory, key);

    bsal_dna_kmer_destroy(&real_source, ephemeral_memory);
}

void bsal_assembly_graph_store_get_summary(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_store *concrete_self;
    int source;

    source = bsal_message_source(message);
    concrete_self = (struct bsal_assembly_graph_store *)bsal_actor_concrete_actor(self);

    concrete_self->vertex_count = 0;
    concrete_self->vertex_observation_count = 0;
    concrete_self->arc_count = 0;
    concrete_self->summary_in_progress = 1;
    concrete_self->source_for_summary = source;

    bsal_actor_add_route_with_condition(self, BSAL_ACTOR_YIELD_REPLY,
                    bsal_assembly_graph_store_yield_reply_summary,
                    &concrete_self->summary_in_progress, 1);

    bsal_map_iterator_init(&concrete_self->iterator, &concrete_self->table);

    bsal_actor_send_to_self_empty(self, BSAL_ACTOR_YIELD);
}


void bsal_assembly_graph_store_yield_reply_summary(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_graph_store *concrete_self;
    int limit;
    int processed;
    struct bsal_vector vector;
    struct bsal_assembly_vertex *vertex;
    int coverage;
    int parent_count;
    /*int child_count;*/

    concrete_self = (struct bsal_assembly_graph_store *)bsal_actor_concrete_actor(self);
    limit = 4321;
    processed = 0;

    while (processed < limit
                    && bsal_map_iterator_has_next(&concrete_self->iterator)) {

        bsal_map_iterator_next(&concrete_self->iterator, NULL, (void **)&vertex);

        ++concrete_self->vertex_count;

        coverage = bsal_assembly_vertex_coverage_depth(vertex);
        concrete_self->vertex_observation_count += coverage;

        parent_count = bsal_assembly_vertex_parent_count(vertex);
        concrete_self->arc_count += parent_count;

        /*
         * Don't count any real arc twice.
         */
        /*
        child_count = bsal_assembly_vertex_child_count(vertex);
        concrete_self->arc_count += child_count;
        */

        ++processed;
    }

    if (bsal_map_iterator_has_next(&concrete_self->iterator)) {

        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_YIELD);

    } else {

        /*
         * Send the answer
         */

        bsal_vector_init(&vector, sizeof(uint64_t));

        bsal_vector_push_back_uint64_t(&vector, concrete_self->vertex_count);
        bsal_vector_push_back_uint64_t(&vector, concrete_self->vertex_observation_count);
        bsal_vector_push_back_uint64_t(&vector, concrete_self->arc_count);

        bsal_actor_send_vector(self, concrete_self->source_for_summary,
                        BSAL_ASSEMBLY_GET_SUMMARY_REPLY, &vector);

        bsal_vector_destroy(&vector);
    }
}
