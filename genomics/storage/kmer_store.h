
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
    struct core_map table;
    struct biosal_dna_codec transport_codec;
    struct biosal_dna_codec storage_codec;
    int kmer_length;
    int key_length_in_bytes;

    int customer;

    uint64_t received;
    uint64_t last_received;

    struct core_memory_pool persistent_memory;

    struct core_map coverage_distribution;
    struct core_map_iterator iterator;
    int source;
};

#define ACTION_PUSH_KMER_BLOCK 0x00004f09
#define ACTION_PUSH_KMER_BLOCK_REPLY 0x000058fb
#define ACTION_STORE_GET_ENTRY_COUNT 0x00007aad
#define ACTION_STORE_GET_ENTRY_COUNT_REPLY 0x00002e6a

extern struct thorium_script biosal_kmer_store_script;

#endif
