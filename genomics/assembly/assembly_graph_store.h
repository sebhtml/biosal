
#ifndef BSAL_ASSEMBLY_GRAPH_STORE_H
#define BSAL_ASSEMBLY_GRAPH_STORE_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>

#include <genomics/data/coverage_distribution.h>

#include <core/structures/map_iterator.h>
#include <core/structures/map.h>

#include <core/system/memory_pool.h>

#define BSAL_ASSEMBLY_GRAPH_STORE_SCRIPT 0xc81a1596

#define BSAL_GET_RECEIVED_ARC_COUNT 0x00004f17
#define BSAL_GET_RECEIVED_ARC_COUNT_REPLY 0x00001cd9

#define BSAL_ASSEMBLY_GET_SUMMARY 0x00000f4c
#define BSAL_ASSEMBLY_GET_SUMMARY_REPLY 0x00003991

/*
 * Enable arc registration with arc actors
 */
#define BSAL_ASSEMBLY_ADD_ARCS

struct bsal_assembly_arc;

/*
 * This is a graph store
 * for assembling sequences.
 *
 * For ephemeral storage, see
 * http://docs.openstack.org/openstack-ops/content/storage_decision.html
 */
struct bsal_assembly_graph_store {
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

    uint64_t received_arc_count;

    int received_arc_block_count;

    /*
     * Summary stuff.
     */

    int source_for_summary;
    int summary_in_progress;
    uint64_t vertex_count;
    uint64_t vertex_observation_count;
    uint64_t arc_count;
};

extern struct bsal_script bsal_assembly_graph_store_script;

void bsal_assembly_graph_store_init(struct bsal_actor *actor);
void bsal_assembly_graph_store_destroy(struct bsal_actor *actor);
void bsal_assembly_graph_store_receive(struct bsal_actor *actor, struct bsal_message *message);

void bsal_assembly_graph_store_print(struct bsal_actor *self);
void bsal_assembly_graph_store_push_data(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_graph_store_yield_reply(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_graph_store_push_kmer_block(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_graph_store_push_arc_block(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_graph_store_add_arc(struct bsal_actor *self,
                struct bsal_assembly_arc *arc, char *sequence);

void bsal_assembly_graph_store_get_summary(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_graph_store_yield_reply_summary(struct bsal_actor *self, struct bsal_message *message);

#endif
