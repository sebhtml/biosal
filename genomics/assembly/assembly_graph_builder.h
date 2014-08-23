
#ifndef _BIOSAL_ASSEMBLY_GRAPH_BUILDER_H_
#define _BIOSAL_ASSEMBLY_GRAPH_BUILDER_H_

#include <biosal.h>

#define SCRIPT_ASSEMBLY_GRAPH_BUILDER 0xc0b1a2b3

#define BSAL_ASSEMBLY_GRAPH_BUILDER_CONTROL_COMPLEXITY 0x00006439

/*
 * Message tags for the graph builder.
 */
#define BSAL_VERIFY_ARCS 0x000032be

/*
 * This actor builds an assembly graph.
 * It needs a list of sequence stores and returns basically a list of
 * graph stores.
 */
struct bsal_assembly_graph_builder {
    struct bsal_vector spawners;
    struct bsal_vector sequence_stores;
    struct bsal_timer timer;
    struct bsal_timer vertex_timer;
    struct bsal_timer arc_timer;

    int source;

    int manager_for_graph_stores;
    struct bsal_vector graph_stores;

    int manager_for_windows;
    struct bsal_vector sliding_windows;

    int manager_for_classifiers;
    struct bsal_vector block_classifiers;

    int manager_for_arc_kernels;
    struct bsal_vector arc_kernels;

    int manager_for_arc_classifiers;
    struct bsal_vector arc_classifiers;

    int configured_sliding_windows;
    int configured_block_classifiers;
    int configured_graph_stores;

    int coverage_distribution;

    int actors_with_kmer_length;

    int kmer_length;

    uint64_t total_kmer_count;
    int notified_windows;

    uint64_t actual_kmer_count;
    int synchronized_graph_stores;

    int completed_sliding_windows;

    int doing_arcs;

    int configured_actors_for_arcs;
    int completed_arc_kernels;
    int configured_sequence_stores;

    uint64_t expected_arc_count;

    /*
     * Summary data
     */

    uint64_t vertex_count;
    uint64_t vertex_observation_count;
    uint64_t arc_count;
    int ready_graph_store_count;
};

extern struct thorium_script bsal_assembly_graph_builder_script;

/*
 * Constructor and destructor
 */
void bsal_assembly_graph_builder_init(struct thorium_actor *self);
void bsal_assembly_graph_builder_destroy(struct thorium_actor *self);

/*
 * The main handler
 */
void bsal_assembly_graph_builder_receive(struct thorium_actor *self, struct thorium_message *message);

/*
 * Other handlers
 */
void bsal_assembly_graph_builder_ask_to_stop(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_graph_builder_start(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_set_producers(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_spawn_reply(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_spawn_reply_graph_store_manager(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_spawn_reply_window_manager(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_graph_builder_set_script_reply_store_manager(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_start_reply_store_manager(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_graph_builder_set_script_reply_window_manager(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_start_reply_window_manager(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_graph_builder_set_script_reply_classifier_manager(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_start_reply_classifier_manager(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_graph_builder_configure(struct thorium_actor *self);
void bsal_assembly_graph_builder_set_kmer_reply(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_graph_builder_connect_actors(struct thorium_actor *self);

void bsal_assembly_graph_builder_set_consumers_reply(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_verify(struct thorium_actor *self);
void bsal_assembly_graph_builder_set_consumer_reply(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_graph_builder_tell_source(struct thorium_actor *self);
void bsal_assembly_graph_builder_set_producer_reply(struct thorium_actor *self, struct thorium_message *message);
int bsal_assembly_graph_builder_get_kmer_length(struct thorium_actor *self);

void bsal_assembly_graph_builder_notify_reply(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_control_complexity(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_get_entry_count_reply(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_graph_builder_set_consumer_reply_graph_stores(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_set_consumer_reply_windows(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_set_expected_message_count_reply(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_notify_from_distribution(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_graph_builder_spawn_reply_arc_kernel_manager(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_start_reply_arc_kernel_manager(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_set_script_reply_arc_kernel_manager(struct thorium_actor *self, struct thorium_message *message);

/*
 * For the manager of arc classifiers.
 */
void bsal_assembly_graph_builder_start_reply_arc_classifier_manager(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_set_script_reply_arc_classifier_manager(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_spawn_reply_arc_classifier_manager(struct thorium_actor *self, struct thorium_message *message);

/*
 * Configure kmer length for arcs.
 */

void bsal_assembly_graph_builder_set_kmer_reply_arcs(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_configure_arc_actors(struct thorium_actor *self, struct thorium_message *message);


/*
 * Functions to verify arcs for high quality.
 */
void bsal_assembly_graph_builder_verify_arcs(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_verify_arc_kernels(struct thorium_actor *self, struct thorium_message *message);
void bsal_assembly_graph_builder_notify_reply_arc_kernels(struct thorium_actor *self, struct thorium_message *message);

void bsal_assembly_graph_builder_get_summary_reply(struct thorium_actor *self, struct thorium_message *message);

#endif
