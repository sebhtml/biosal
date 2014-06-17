
#ifndef BSAL_KMER_STORE_H
#define BSAL_KMER_STORE_H

#include <engine/actor.h>

#include <structures/map.h>

#define BSAL_KMER_STORE_SCRIPT 0xe6d32c91

struct bsal_kmer_store {
    struct bsal_map table;
    int kmer_length;
    int key_length_in_bytes;

    int customer;

    uint64_t received;
};

#define BSAL_PUSH_KMER_BLOCK 0x00004f09
#define BSAL_PUSH_KMER_BLOCK_REPLY 0x000058fb
#define BSAL_STORE_GET_ENTRY_COUNT 0x00007aad
#define BSAL_STORE_GET_ENTRY_COUNT_REPLY 0x00002e6a

extern struct bsal_script bsal_kmer_store_script;

void bsal_kmer_store_init(struct bsal_actor *actor);
void bsal_kmer_store_destroy(struct bsal_actor *actor);
void bsal_kmer_store_receive(struct bsal_actor *actor, struct bsal_message *message);

void bsal_kmer_store_print(struct bsal_actor *self);
void bsal_kmer_store_push_data(struct bsal_actor *self, struct bsal_message *message);

#endif
