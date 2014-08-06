
#ifndef BSAL_ASSEMBLY_ARC_CLASSIFIER_H
#define BSAL_ASSEMBLY_ARC_CLASSIFIER_H

#include <engine/thorium/actor.h>

#include <genomics/data/dna_codec.h>
#include <core/structures/vector.h>

#include <core/system/memory_pool.h>

#define BSAL_ASSEMBLY_ARC_CLASSIFIER_SCRIPT 0x115b87ed

struct bsal_assembly_arc_classifier {
    int kmer_length;
};

extern struct bsal_script bsal_assembly_arc_classifier_script;

void bsal_assembly_arc_classifier_init(struct bsal_actor *self);
void bsal_assembly_arc_classifier_destroy(struct bsal_actor *self);
void bsal_assembly_arc_classifier_receive(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_arc_classifier_set_kmer_length(struct bsal_actor *self, struct bsal_message *message);

#endif
