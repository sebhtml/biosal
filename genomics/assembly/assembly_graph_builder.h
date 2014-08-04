
#ifndef _BIOSAL_ASSEMBLY_GRAPH_BUILDER_H_
#define _BIOSAL_ASSEMBLY_GRAPH_BUILDER_H_

#include <biosal.h>

#define BSAL_ASSEMBLY_GRAPH_BUILDER_SCRIPT 0xc0b1a2b3
#define BSAL_ASSEMBLY_GRAPH_BUILDER_NAME "bsal_assembly_graph_builder"
#define BSAL_ASSEMBLY_GRAPH_BUILDER_DESCRIPTION "This builder implements a distributed actor algorithm for building an assembly graph"
#define BSAL_ASSEMBLY_GRAPH_BUILDER_VERSION "Cool version"
#define BSAL_ASSEMBLY_GRAPH_BUILDER_AUTHOR "Sébastien Boisvert, Argonne National Laboratory"

#define BSAL_ASSEMBLY_GRAPH_BUILDER_DEFAULT_KMER_LENGTH 49

#define BSAL_ASSEMBLY_GRAPH_BUILDER_CONTROL_COMPLEXITY 0x00006439

/*
 * This actor builds an assembly graph.
 * It needs a list of sequence stores and returns basically a list of
 * graph stores.
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

    uint64_t total_kmer_count;
    int notified_windows;

    uint64_t actual_kmer_count;
    int synchronized_graph_stores;

    int completed_sliding_windows;
};

extern struct bsal_script bsal_assembly_graph_builder_script;

/*
 * Constructor and destructor
 */
void bsal_assembly_graph_builder_init(struct bsal_actor *self);
void bsal_assembly_graph_builder_destroy(struct bsal_actor *self);

/*
 * The main handler
 */
void bsal_assembly_graph_builder_receive(struct bsal_actor *self, struct bsal_message *message);

/*
 * Other handlers
 */
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

void bsal_assembly_graph_builder_connect_actors(struct bsal_actor *self);

void bsal_assembly_graph_builder_set_consumers_reply(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_graph_builder_verify(struct bsal_actor *self);
void bsal_assembly_graph_builder_set_consumer_reply(struct bsal_actor *self, struct bsal_message *message);

void bsal_assembly_graph_builder_tell_source(struct bsal_actor *self);
void bsal_assembly_graph_builder_set_producer_reply(struct bsal_actor *self, struct bsal_message *message);
int bsal_assembly_graph_builder_get_kmer_length(struct bsal_actor *self);
void bsal_assembly_graph_builder_notify_reply(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_graph_builder_control_complexity(struct bsal_actor *self, struct bsal_message *message);
void bsal_assembly_graph_builder_get_entry_count_reply(struct bsal_actor *self, struct bsal_message *message);

#endif
