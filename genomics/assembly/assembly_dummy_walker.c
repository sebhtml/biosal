
#include "assembly_dummy_walker.h"

#include "assembly_graph_store.h"
#include "assembly_vertex.h"

#include <genomics/data/dna_kmer.h>
#include <genomics/data/dna_codec.h>

#include <stdio.h>

struct bsal_script bsal_assembly_dummy_walker_script = {
    .identifier = BSAL_ASSEMBLY_DUMMY_WALKER_SCRIPT,
    .name = "bsal_assembly_dummy_walker",
    .init = bsal_assembly_dummy_walker_init,
    .destroy = bsal_assembly_dummy_walker_destroy,
    .receive = bsal_assembly_dummy_walker_receive,
    .size = sizeof(struct bsal_assembly_dummy_walker)
};

void bsal_assembly_dummy_walker_init(struct bsal_actor *self)
{
    struct bsal_assembly_dummy_walker *concrete_self;

    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);

    bsal_vector_init(&concrete_self->graph_stores, sizeof(int));

    bsal_actor_add_route(self, BSAL_ASSEMBLY_GET_STARTING_VERTEX_REPLY,
        bsal_assembly_dummy_walker_get_starting_vertex_reply);

    bsal_actor_add_route(self, BSAL_ACTOR_START,
                    bsal_assembly_dummy_walker_start);

    bsal_actor_add_route(self, BSAL_ASSEMBLY_GET_VERTEX_REPLY,
                    bsal_assembly_dummy_walker_get_vertex_reply);

    bsal_actor_add_route(self, BSAL_ASSEMBLY_PUSH_AND_FETCH,
                    bsal_assembly_dummy_walker_push_and_fetch);

    bsal_actor_add_route(self, BSAL_ASSEMBLY_PUSH_AND_FETCH_REPLY,
                    bsal_assembly_dummy_walker_push_and_fetch_reply);

    bsal_memory_pool_init(&concrete_self->memory_pool, 32768);

    bsal_vector_init(&concrete_self->path, sizeof(int));

    /*
     * Configure the codec.
     */

    bsal_dna_codec_init(&concrete_self->codec);

    if (bsal_actor_get_node_count(self) >= BSAL_DNA_CODEC_MINIMUM_NODE_COUNT_FOR_TWO_BIT) {
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_TRANSPORT
        bsal_dna_codec_enable_two_bit_encoding(&concrete_self->codec);
#endif
    }
}

void bsal_assembly_dummy_walker_destroy(struct bsal_actor *self)
{
    struct bsal_assembly_dummy_walker *concrete_self;

    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);

    bsal_vector_destroy(&concrete_self->graph_stores);

    bsal_vector_destroy(&concrete_self->path);
}

void bsal_assembly_dummy_walker_receive(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    struct bsal_assembly_dummy_walker *concrete_self;

    if (bsal_actor_use_route(self, message)) {
        return;
    }

    tag = bsal_message_tag(message);
    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);

    if (tag == BSAL_ACTOR_START) {

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_STOP);

    } else if (tag == BSAL_ASSEMBLY_GET_KMER_LENGTH_REPLY) {

        bsal_message_unpack_int(message, 0, &concrete_self->kmer_length);

        bsal_actor_send_reply_empty(self, BSAL_ASSEMBLY_GET_STARTING_VERTEX);
    }
}

void bsal_assembly_dummy_walker_start(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_dummy_walker *concrete_self;
    int graph;

    buffer = bsal_message_buffer(message);
    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);

    printf("%s/%d is ready to surf the graph !\n",
                        bsal_actor_script_name(self),
                        bsal_actor_name(self));

    bsal_vector_unpack(&concrete_self->graph_stores, buffer);

    graph = bsal_vector_at_as_int(&concrete_self->graph_stores, 0);

    bsal_actor_send_empty(self, graph, BSAL_ASSEMBLY_GET_KMER_LENGTH);
}

void bsal_assembly_dummy_walker_get_starting_vertex_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_dummy_walker *concrete_self;
    void *buffer;
    struct bsal_memory_pool *ephemeral_memory;
    int count;

    count = bsal_message_count(message);
    buffer = bsal_message_buffer(message);
    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    printf("%s/%d received starting vertex (%d bytes)\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    count);

    bsal_dna_kmer_init_empty(&concrete_self->current_kmer);
    bsal_dna_kmer_unpack(&concrete_self->current_kmer, buffer, concrete_self->kmer_length,
                    &concrete_self->memory_pool,
                    &concrete_self->codec);
    bsal_dna_kmer_print(&concrete_self->current_kmer, concrete_self->kmer_length, &concrete_self->codec,
                    ephemeral_memory);

    bsal_dna_kmer_init_copy(&concrete_self->first_kmer, &concrete_self->current_kmer,
                    concrete_self->kmer_length, &concrete_self->memory_pool,
                    &concrete_self->codec);

    bsal_actor_send_to_self_empty(self, BSAL_ASSEMBLY_PUSH_AND_FETCH);
}

void bsal_assembly_dummy_walker_get_vertex_reply(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_dummy_walker *concrete_self;
    struct bsal_memory_pool *ephemeral_memory;

    buffer = bsal_message_buffer(message);
    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    bsal_assembly_vertex_init(&concrete_self->current_vertex);
    bsal_assembly_vertex_unpack(&concrete_self->current_vertex, buffer);

    printf("Connectivity: \n");

    bsal_assembly_vertex_print(&concrete_self->current_vertex);

    concrete_self->current_child = 0;

    bsal_actor_send_to_self_empty(self, BSAL_ASSEMBLY_PUSH_AND_FETCH_REPLY);
}

void bsal_assembly_dummy_walker_push_and_fetch(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_dummy_walker *concrete_self;
    struct bsal_message new_message;
    int new_count;
    void *new_buffer;
    struct bsal_memory_pool *ephemeral_memory;
    int store_index;
    int store;

    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    new_count = bsal_dna_kmer_pack_size(&concrete_self->current_kmer, concrete_self->kmer_length,
                    &concrete_self->codec);
    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

    bsal_dna_kmer_pack(&concrete_self->current_kmer, new_buffer,
                    concrete_self->kmer_length,
                    &concrete_self->codec);

    store_index = bsal_dna_kmer_store_index(&concrete_self->current_kmer,
                    bsal_vector_size(&concrete_self->graph_stores),
                    concrete_self->kmer_length,
                    &concrete_self->codec, ephemeral_memory);

    store = bsal_vector_at_as_int(&concrete_self->graph_stores, store_index);

    bsal_message_init(&new_message, BSAL_ASSEMBLY_GET_VERTEX, new_count, new_buffer);
    bsal_actor_send(self, store, &new_message);
    bsal_message_destroy(&new_message);

    bsal_memory_pool_free(ephemeral_memory, new_buffer);
}

void bsal_assembly_dummy_walker_push_and_fetch_reply(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_send_to_supervisor_empty(self, BSAL_ACTOR_START_REPLY);
}
