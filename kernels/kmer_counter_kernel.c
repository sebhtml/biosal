
#include "kmer_counter_kernel.h"

#include <data/dna_kmer.h>
#include <storage/sequence_store.h>

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
    bsal_vector_init(&concrete_actor->customers, sizeof(int));
}

void bsal_kmer_counter_kernel_destroy(struct bsal_actor *actor)
{
    struct bsal_kmer_counter_kernel *concrete_actor;

    concrete_actor = (struct bsal_kmer_counter_kernel *)bsal_actor_concrete_actor(actor);

    bsal_vector_destroy(&concrete_actor->customers);
}

void bsal_kmer_counter_kernel_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    struct bsal_dna_kmer kmer;

    source = bsal_message_tag(message);
    tag = bsal_message_tag(message);

    if (tag == BSAL_STORE_SEQUENCES) {

        bsal_dna_kmer_init(&kmer, NULL);

        bsal_actor_send_reply_empty(actor, BSAL_STORE_SEQUENCES_REPLY);

    } else if (tag == BSAL_ACTOR_START) {

        bsal_actor_send_reply_empty(actor, BSAL_ACTOR_START_REPLY);

    } else if (tag == BSAL_SEQUENCE_STORE_RESERVE) {

        bsal_actor_send_reply_empty(actor, BSAL_SEQUENCE_STORE_RESERVE_REPLY);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP && source == bsal_actor_supervisor(actor)) {

        bsal_actor_send_to_self_empty(actor, BSAL_ACTOR_STOP);
    }
}


