
#include "assembly_arc_classifier.h"

#include "assembly_arc_kernel.h"

#include <genomics/kernels/dna_kmer_counter_kernel.h>

#include <stdio.h>

struct bsal_script bsal_assembly_arc_classifier_script = {
    .identifier = BSAL_ASSEMBLY_ARC_CLASSIFIER_SCRIPT,
    .name = "bsal_assembly_arc_classifier",
    .init = bsal_assembly_arc_classifier_init,
    .destroy = bsal_assembly_arc_classifier_destroy,
    .receive = bsal_assembly_arc_classifier_receive,
    .size = sizeof(struct bsal_assembly_arc_classifier)
};

void bsal_assembly_arc_classifier_init(struct bsal_actor *self)
{
    struct bsal_assembly_arc_classifier *concrete_self;

    concrete_self = (struct bsal_assembly_arc_classifier *)bsal_actor_concrete_actor(self);

    concrete_self->kmer_length = -1;

    bsal_actor_add_route(self, BSAL_ACTOR_ASK_TO_STOP,
                    bsal_actor_ask_to_stop);

    bsal_actor_add_route(self, BSAL_SET_KMER_LENGTH,
                    bsal_assembly_arc_classifier_set_kmer_length);

    printf("%s/%d is now active\n",
                    bsal_actor_script_name(self),
                    bsal_actor_name(self));

    /*
     *
     * Configure the codec.
     */

    bsal_dna_codec_init(&concrete_self->codec);

    if (bsal_actor_get_node_count(self) >= BSAL_DNA_CODEC_MINIMUM_NODE_COUNT_FOR_TWO_BIT) {
#ifdef BSAL_DNA_CODEC_USE_TWO_BIT_ENCODING_FOR_TRANSPORT
        bsal_dna_codec_enable_two_bit_encoding(&concrete_self->codec);
#endif
    }

    bsal_vector_init(&concrete_self->consumers, sizeof(int));

    bsal_actor_add_route(self, BSAL_ASSEMBLY_PUSH_ARC_BLOCK,
                    bsal_assembly_arc_classifier_push_arc_block);

    concrete_self->received_blocks = 0;
}

void bsal_assembly_arc_classifier_destroy(struct bsal_actor *self)
{
    struct bsal_assembly_arc_classifier *concrete_self;

    concrete_self = (struct bsal_assembly_arc_classifier *)bsal_actor_concrete_actor(self);

    concrete_self->kmer_length = -1;

    bsal_vector_destroy(&concrete_self->consumers);

    bsal_dna_codec_destroy(&concrete_self->codec);
}

void bsal_assembly_arc_classifier_receive(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    void *buffer;
    struct bsal_assembly_arc_classifier *concrete_self;

    if (bsal_actor_use_route(self, message)) {
        return;
    }

    concrete_self = (struct bsal_assembly_arc_classifier *)bsal_actor_concrete_actor(self);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);

    if (tag == BSAL_ACTOR_SET_CONSUMERS) {

        bsal_vector_unpack(&concrete_self->consumers, buffer);

        bsal_actor_send_reply_empty(self, BSAL_ACTOR_SET_CONSUMERS_REPLY);

    }
}

void bsal_assembly_arc_classifier_set_kmer_length(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_arc_classifier *concrete_self;

    concrete_self = (struct bsal_assembly_arc_classifier *)bsal_actor_concrete_actor(self);

    bsal_message_unpack_int(message, 0, &concrete_self->kmer_length);

    bsal_actor_send_reply_empty(self, BSAL_SET_KMER_LENGTH_REPLY);
}

void bsal_assembly_arc_classifier_push_arc_block(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_assembly_arc_classifier *concrete_self;
    int source;

    concrete_self = (struct bsal_assembly_arc_classifier *)bsal_actor_concrete_actor(self);
    source = bsal_message_source(message);

    concrete_self->source = source;

    ++concrete_self->received_blocks;

    /*
     * TODO
     *
     * Classify every arc in the input block
     * and put them in output blocks.
     *
     * Finally, send these output blocks to consumers.
     */

    bsal_actor_send_empty(self, concrete_self->source,
                    BSAL_ASSEMBLY_PUSH_ARC_BLOCK_REPLY);
}
