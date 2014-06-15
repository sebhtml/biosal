
#include "kmer_store.h"

#include <helpers/message_helper.h>
#include <helpers/actor_helper.h>

#include <data/dna_kmer.h>
#include <system/memory.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

struct bsal_script bsal_kmer_store_script = {
    .name = BSAL_KMER_STORE_SCRIPT,
    .init = bsal_kmer_store_init,
    .destroy = bsal_kmer_store_destroy,
    .receive = bsal_kmer_store_receive,
    .size = sizeof(struct bsal_kmer_store)
};

void bsal_kmer_store_init(struct bsal_actor *actor)
{
    struct bsal_kmer_store *concrete_actor;

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(actor);
    concrete_actor->kmer_length = -1;
}

void bsal_kmer_store_destroy(struct bsal_actor *actor)
{
    struct bsal_kmer_store *concrete_actor;

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(actor);

    if (concrete_actor->kmer_length != -1) {
        bsal_dynamic_hash_table_destroy(&concrete_actor->table);
    }

    concrete_actor->kmer_length = -1;
}

void bsal_kmer_store_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    struct bsal_kmer_store *concrete_actor;
    int buckets;
    struct bsal_dna_kmer kmer;
    int name;

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    name = bsal_actor_name(actor);

    if (tag == BSAL_ACTOR_START) {

        buckets = 8;
        bsal_message_helper_unpack_int(message, 0, &concrete_actor->kmer_length);

        bsal_dna_kmer_init_mock(&kmer, concrete_actor->kmer_length);
        concrete_actor->key_length_in_bytes = bsal_dna_kmer_pack_size(&kmer);
        bsal_dna_kmer_destroy(&kmer);

        printf("kmer store actor/%d will use %d bytes for keys (k is %d)\n",
                        name, concrete_actor->key_length_in_bytes,
                        concrete_actor->kmer_length);

        bsal_dynamic_hash_table_init(&concrete_actor->table, buckets, concrete_actor->key_length_in_bytes,
                        sizeof(int));

        bsal_actor_helper_send_reply_empty(actor, BSAL_ACTOR_START_REPLY);

    } else if (tag == BSAL_PUSH_KMER_BLOCK) {

        bsal_actor_helper_send_reply_empty(actor, BSAL_PUSH_KMER_BLOCK_REPLY);
    }
}


