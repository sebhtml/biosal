
#ifndef BIOSAL_KMER_STORE_H
#define BIOSAL_KMER_STORE_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>

#include <genomics/data/coverage_distribution.h>

#include <core/structures/map_iterator.h>
#include <core/structures/map.h>

#include <core/system/memory_pool.h>

#define SCRIPT_KMER_STORE 0xe6d32c91

/*
 * For ephemeral storage, see
 * http://docs.openstack.org/openstack-ops/content/storage_decision.html
 */
struct biosal_kmer_store {
    struct biosal_map table;
    struct biosal_dna_codec transport_codec;
    struct biosal_dna_codec storage_codec;
    int kmer_length;
    int key_length_in_bytes;

    int customer;

    uint64_t received;
    uint64_t last_received;

    struct biosal_memory_pool persistent_memory;

    struct biosal_map coverage_distribution;
    struct biosal_map_iterator iterator;
    int source;
};

#define ACTION_PUSH_KMER_BLOCK 0x00004f09
#define ACTION_PUSH_KMER_BLOCK_REPLY 0x000058fb
#define ACTION_STORE_GET_ENTRY_COUNT 0x00007aad
#define ACTION_STORE_GET_ENTRY_COUNT_REPLY 0x00002e6a

extern struct thorium_script biosal_kmer_store_script;

void biosal_kmer_store_init(struct thorium_actor *actor);
void biosal_kmer_store_destroy(struct thorium_actor *actor);
void biosal_kmer_store_receive(struct thorium_actor *actor, struct thorium_message *message);

void biosal_kmer_store_print(struct thorium_actor *self);
void biosal_kmer_store_push_data(struct thorium_actor *self, struct thorium_message *message);
void biosal_kmer_store_yield_reply(struct thorium_actor *self, struct thorium_message *message);

#endif
