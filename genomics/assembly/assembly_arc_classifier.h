
#ifndef BSAL_ASSEMBLY_ARC_CLASSIFIER_H
#define BSAL_ASSEMBLY_ARC_CLASSIFIER_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>
#include <core/structures/vector.h>

#include <core/system/memory_pool.h>

#define SCRIPT_ASSEMBLY_ARC_CLASSIFIER 0x115b87ed

/*
 * This actor classifies arcs.
 */
struct bsal_assembly_arc_classifier {
    int kmer_length;

    int source;
    struct bsal_vector consumers;

    struct bsal_dna_codec codec;

    int received_blocks;

    struct bsal_vector pending_requests;
    int producer_is_waiting;
    int maximum_pending_request_count;

    int consumer_count_above_threshold;
};

extern struct thorium_script bsal_assembly_arc_classifier_script;

void bsal_assembly_arc_classifier_init(struct thorium_actor *self);
void bsal_assembly_arc_classifier_destroy(struct thorium_actor *self);
void bsal_assembly_arc_classifier_receive(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_arc_classifier_set_kmer_length(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_arc_classifier_push_arc_block(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_arc_classifier_verify_counters(struct thorium_actor *self);

#endif
