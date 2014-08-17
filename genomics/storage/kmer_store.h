
#ifndef BSAL_KMER_STORE_H
#define BSAL_KMER_STORE_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>

#include <genomics/data/coverage_distribution.h>

#include <core/structures/map_iterator.h>
#include <core/structures/map.h>

#include <core/system/memory_pool.h>

#define BSAL_KMER_STORE_SCRIPT 0xe6d32c91

/*
 * For ephemeral storage, see
 * http://docs.openstack.org/openstack-ops/content/storage_decision.html
 */
struct bsal_kmer_store {
    struct bsal_map table;
    struct bsal_dna_codec transport_codec;
    struct bsal_dna_codec storage_codec;
    int kmer_length;
    int key_length_in_bytes;

    int customer;

    uint64_t received;
    uint64_t last_received;

    struct bsal_memory_pool persistent_memory;

    struct bsal_map coverage_distribution;
    struct bsal_map_iterator iterator;
    int source;
};

#define BSAL_PUSH_KMER_BLOCK 0x00004f09
#define BSAL_PUSH_KMER_BLOCK_REPLY 0x000058fb
#define BSAL_STORE_GET_ENTRY_COUNT 0x00007aad
#define BSAL_STORE_GET_ENTRY_COUNT_REPLY 0x00002e6a

extern struct thorium_script bsal_kmer_store_script;

void bsal_kmer_store_init(struct thorium_actor *actor);
void bsal_kmer_store_destroy(struct thorium_actor *actor);
void bsal_kmer_store_receive(struct thorium_actor *actor, struct thorium_message *message);

void bsal_kmer_store_print(struct thorium_actor *self);
void bsal_kmer_store_push_data(struct thorium_actor *self, struct thorium_message *message);
void bsal_kmer_store_yield_reply(struct thorium_actor *self, struct thorium_message *message);

#endif
