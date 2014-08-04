
#ifndef _BIOSAL_ASSEMBLY_GRAPH_BUILDER_H_
#define _BIOSAL_ASSEMBLY_GRAPH_BUILDER_H_

#include <biosal.h>

#define BSAL_ASSEMBLY_GRAPH_BUILDER_SCRIPT 0xc0b1a2b3

#define BSAL_ASSEMBLY_GRAPH_BUILDER_DEFAULT_KMER_LENGTH 49

/*
 * This actor builds an assembly graph
 */
struct bsal_assembly_graph_builder {
    struct bsal_vector spawners;
    struct bsal_vector sequence_stores;
    struct bsal_timer timer;

    int source;

    int manager_for_graph_stores;
    struct bsal_vector graph_stores;

    int manager_for_windows;
    struct bsal_vector sliding_windows;

    int manager_for_classifiers;
    struct bsal_vector block_classifiers;

    int configured_sliding_windows;
    int configured_graph_stores;
    int configured_block_classifiers;

    int actors_with_kmer_length;

    int kmer_length;
};

extern struct bsal_script bsal_assembly_graph_builder_script;

void bsal_assembly_graph_builder_init(struct bsal_actor *self);
void bsal_assembly_graph_builder_destroy(struct bsal_actor *self);
void bsal_assembly_graph_builder_receive(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_graph_builder_ask_to_stop(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_graph_builder_start(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_graph_builder_set_producers(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_graph_builder_spawn_reply(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_graph_builder_set_script_reply_store_manager(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_graph_builder_start_reply_store_manager(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_graph_builder_set_script_reply_window_manager(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_graph_builder_start_reply_window_manager(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_graph_builder_set_script_reply_classifier_manager(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_graph_builder_start_reply_classifier_manager(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_graph_builder_configure(struct bsal_actor *self);
void bsal_assembly_graph_builder_set_kmer_reply(struct bsal_actor *self, struct bsal_message *message);

#endif
