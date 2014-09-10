
#include "unitig_visitor.h"

#include <genomics/assembly/assembly_graph_store.h>

#include <core/structures/vector.h>

#include <stdio.h>

#define STEP_GET_KMER_LENGTH 0
#define STEP_GET_START_KMER 1
#define STEP_GET_START_VERTEX 2
#define STEP_DECIDE 3

struct thorium_script bsal_unitig_visitor_script = {
    .identifier = SCRIPT_UNITIG_VISITOR,
    .name = "bsal_unitig_visitor",
    .init = bsal_unitig_visitor_init,
    .destroy = bsal_unitig_visitor_destroy,
    .receive = bsal_unitig_visitor_receive,
    .size = sizeof(struct bsal_unitig_visitor),
    .description = "A universe visitor for vertices."
};

void bsal_unitig_visitor_init(struct thorium_actor *self)
{
    struct bsal_unitig_visitor *concrete_self;

    concrete_self = (struct bsal_unitig_visitor *)thorium_actor_concrete_actor(self);
    bsal_memory_pool_init(&concrete_self->memory_pool, 131072);

    bsal_vector_init(&concrete_self->graph_stores, sizeof(int));
    bsal_vector_set_memory_pool(&concrete_self->graph_stores, &concrete_self->memory_pool);
    concrete_self->manager = -1;
    concrete_self->completed = 0;
    concrete_self->graph_store_index = -1;
    concrete_self->kmer_length = -1;
    concrete_self->step = STEP_GET_KMER_LENGTH;

    bsal_dna_codec_init(&concrete_self->codec);

    if (bsal_dna_codec_must_use_two_bit_encoding(&concrete_self->codec,
                            thorium_actor_get_node_count(self))) {
        bsal_dna_codec_enable_two_bit_encoding(&concrete_self->codec);
    }

    concrete_self->visited = 0;

}

void bsal_unitig_visitor_destroy(struct thorium_actor *self)
{
    struct bsal_unitig_visitor *concrete_self;
    concrete_self = (struct bsal_unitig_visitor *)thorium_actor_concrete_actor(self);
    concrete_self->manager = -1;
    concrete_self->completed = 0;

    bsal_vector_destroy(&concrete_self->graph_stores);

    bsal_memory_pool_destroy(&concrete_self->memory_pool);
}

void bsal_unitig_visitor_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    void *buffer;
    int source;
    struct bsal_unitig_visitor *concrete_self;
    int size;
    int count;

    tag = thorium_message_tag(message);
    count = thorium_message_count(message);
    buffer = thorium_message_buffer(message);
    concrete_self = (struct bsal_unitig_visitor *)thorium_actor_concrete_actor(self);
    source = thorium_message_source(message);

    if (tag == ACTION_START) {

        concrete_self->manager = source;

        printf("%s/%d is ready to visit places in the universe\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self));

        bsal_vector_unpack(&concrete_self->graph_stores, buffer);
        size = bsal_vector_size(&concrete_self->graph_stores);
        concrete_self->graph_store_index = rand() % size;

        concrete_self->step = STEP_GET_KMER_LENGTH;
        bsal_unitig_visitor_do_something(self);

    } else if (tag == ACTION_ASK_TO_STOP) {
        thorium_actor_send_to_self_empty(self, ACTION_STOP);

        thorium_actor_send_reply_empty(self, ACTION_ASK_TO_STOP_REPLY);

    } else if (tag == ACTION_ASSEMBLY_GET_STARTING_VERTEX_REPLY) {

        if (count == 0) {
            ++concrete_self->completed;
            concrete_self->step = STEP_GET_START_KMER;
            bsal_unitig_visitor_do_something(self);

        } else {
            bsal_dna_kmer_init_empty(&concrete_self->main_kmer);
            bsal_dna_kmer_unpack(&concrete_self->main_kmer, buffer, concrete_self->kmer_length,
                    &concrete_self->memory_pool, &concrete_self->codec);

            concrete_self->step = STEP_GET_START_VERTEX;
            bsal_unitig_visitor_do_something(self);
        }

    } else if (tag == ACTION_ASSEMBLY_GET_KMER_LENGTH_REPLY) {
        thorium_message_unpack_int(message, 0, &concrete_self->kmer_length);

        concrete_self->step = STEP_GET_START_KMER;

        bsal_unitig_visitor_do_something(self);

    } else if (tag == ACTION_ASSEMBLY_GET_VERTEX_REPLY) {
        
        if (concrete_self->step == STEP_GET_START_VERTEX) {

            bsal_assembly_vertex_init(&concrete_self->main_vertex);
            bsal_assembly_vertex_unpack(&concrete_self->main_vertex, buffer);

            concrete_self->step = STEP_DECIDE;
            bsal_unitig_visitor_do_something(self);
        }

    } else if (tag == ACTION_YIELD_REPLY) {
        bsal_unitig_visitor_do_something(self);
    }
}

void bsal_unitig_visitor_do_something(struct thorium_actor *self)
{
    struct bsal_unitig_visitor *concrete_self;
    int graph_store_index;
    int graph_store;
    int size;

    concrete_self = thorium_actor_concrete_actor(self);
    size = bsal_vector_size(&concrete_self->graph_stores);

#if 0
    printf("bsal_unitig_visitor_do_something step= %d\n",
                    concrete_self->step);
#endif

    if (concrete_self->completed == bsal_vector_size(&concrete_self->graph_stores)) {
        thorium_actor_send_empty(self, concrete_self->manager, ACTION_START_REPLY);
    } else if (concrete_self->step == STEP_GET_KMER_LENGTH) {
        graph_store_index = concrete_self->graph_store_index;
        graph_store = bsal_vector_at_as_int(&concrete_self->graph_stores, graph_store_index);

        thorium_actor_send_empty(self, graph_store, ACTION_ASSEMBLY_GET_KMER_LENGTH);

    } else if (concrete_self->step == STEP_GET_START_KMER) {
        graph_store_index = concrete_self->graph_store_index;
        ++concrete_self->graph_store_index;
        concrete_self->graph_store_index %= size;

        graph_store = bsal_vector_at_as_int(&concrete_self->graph_stores, graph_store_index);

        thorium_actor_send_empty(self, graph_store,
                    ACTION_ASSEMBLY_GET_STARTING_VERTEX);

    } else if (concrete_self->step == STEP_GET_START_VERTEX) {

        bsal_unitig_visitor_fetch_remote_memory(self, &concrete_self->main_kmer);

    } else if (concrete_self->step == STEP_DECIDE) {

        bsal_dna_kmer_destroy(&concrete_self->main_kmer, &concrete_self->memory_pool);
        bsal_assembly_vertex_destroy(&concrete_self->main_vertex);

        concrete_self->step = STEP_GET_START_KMER;

        if (concrete_self->visited % 10000 == 0) {
            printf("%s/%d visited %d vertices so far\n",
                            thorium_actor_script_name(self), thorium_actor_name(self),
                            concrete_self->visited);
        }
        ++concrete_self->visited;

        thorium_actor_send_to_self_empty(self, ACTION_YIELD);
    }
}

void bsal_unitig_visitor_fetch_remote_memory(struct thorium_actor *self, struct bsal_dna_kmer *kmer)
{
    struct bsal_unitig_visitor *concrete_self;
    struct bsal_memory_pool *ephemeral_memory;
    struct  thorium_message new_message;
    void *new_buffer;
    int new_count;
    int store_index;
    int store;
    int size;

    concrete_self = thorium_actor_concrete_actor(self);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    new_count = bsal_dna_kmer_pack_size(kmer, concrete_self->kmer_length,
                &concrete_self->codec);
    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);
    bsal_dna_kmer_pack(kmer, new_buffer, concrete_self->kmer_length, &concrete_self->codec);

    size = bsal_vector_size(&concrete_self->graph_stores);
    store_index = bsal_dna_kmer_store_index(kmer, size, concrete_self->kmer_length,
                &concrete_self->codec, ephemeral_memory);
    store = bsal_vector_at_as_int(&concrete_self->graph_stores, store_index);

    thorium_message_init(&new_message, ACTION_ASSEMBLY_GET_VERTEX, new_count, new_buffer);
    thorium_actor_send(self, store, &new_message);
    thorium_message_destroy(&new_message);

    bsal_memory_pool_free(ephemeral_memory, new_buffer);
}
