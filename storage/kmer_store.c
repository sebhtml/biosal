
#include "kmer_store.h"

#include <data/dna_kmer_block.h>
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

void bsal_kmer_store_init(struct bsal_actor *self)
{
    struct bsal_kmer_store *concrete_actor;

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);
    concrete_actor->kmer_length = -1;
}

void bsal_kmer_store_destroy(struct bsal_actor *self)
{
    struct bsal_kmer_store *concrete_actor;

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);

    if (concrete_actor->kmer_length != -1) {
        bsal_map_destroy(&concrete_actor->table);
    }

    concrete_actor->kmer_length = -1;
}

void bsal_kmer_store_receive(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    void *buffer;
    struct bsal_kmer_store *concrete_actor;
    struct bsal_dna_kmer kmer;
    struct bsal_dna_kmer_block block;
    int name;

    concrete_actor = (struct bsal_kmer_store *)bsal_actor_concrete_actor(self);
    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    name = bsal_actor_name(self);

    if (tag == BSAL_ACTOR_START) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->kmer_length);

        bsal_dna_kmer_init_mock(&kmer, concrete_actor->kmer_length);
        concrete_actor->key_length_in_bytes = bsal_dna_kmer_pack_size(&kmer);
        bsal_dna_kmer_destroy(&kmer);

        printf("kmer store actor/%d will use %d bytes for keys (k is %d)\n",
                        name, concrete_actor->key_length_in_bytes,
                        concrete_actor->kmer_length);

        bsal_map_init(&concrete_actor->table, concrete_actor->key_length_in_bytes,
                        sizeof(int));

        bsal_actor_helper_send_reply_empty(self, BSAL_ACTOR_START_REPLY);

    } else if (tag == BSAL_PUSH_KMER_BLOCK) {

        bsal_dna_kmer_block_unpack(&block, buffer);

        bsal_dna_kmer_block_destroy(&block);

        bsal_actor_helper_send_reply_empty(self, BSAL_PUSH_KMER_BLOCK_REPLY);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        bsal_actor_helper_ask_to_stop(self, message);
    }
}


