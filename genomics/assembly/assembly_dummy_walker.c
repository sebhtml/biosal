
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
    struct bsal_dna_kmer kmer;
    struct bsal_assembly_dummy_walker *concrete_self;
    void *buffer;
    struct bsal_memory_pool *ephemeral_memory;
    int count;
    struct bsal_message new_message;

    count = bsal_message_count(message);
    buffer = bsal_message_buffer(message);
    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    printf("%s/%d received starting vertex (%d bytes)\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self),
                    count);

    bsal_dna_kmer_init_empty(&kmer);
    bsal_dna_kmer_unpack(&kmer, buffer, concrete_self->kmer_length, ephemeral_memory,
                    &concrete_self->codec);
    bsal_dna_kmer_print(&kmer, concrete_self->kmer_length, &concrete_self->codec,
                    ephemeral_memory);
    bsal_dna_kmer_destroy(&kmer, ephemeral_memory);

    bsal_message_init(&new_message, BSAL_ASSEMBLY_GET_VERTEX, count, buffer);
    bsal_actor_send_reply(self, &new_message);
    bsal_message_destroy(&new_message);
}

void bsal_assembly_dummy_walker_get_vertex_reply(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    struct bsal_assembly_dummy_walker *concrete_self;
    struct bsal_memory_pool *ephemeral_memory;
    struct bsal_assembly_vertex vertex;

    buffer = bsal_message_buffer(message);
    concrete_self = (struct bsal_assembly_dummy_walker *)bsal_actor_concrete_actor(self);
    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);

    bsal_assembly_vertex_unpack(&vertex, buffer);

    printf("Connectivity: \n");

    bsal_assembly_vertex_print(&vertex);

    bsal_actor_send_to_supervisor_empty(self, BSAL_ACTOR_START_REPLY);
}
