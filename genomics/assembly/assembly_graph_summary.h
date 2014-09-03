
#ifndef BSAL_ASSEMBLY_GRAPH_SUMMARY_H
#define BSAL_ASSEMBLY_GRAPH_SUMMARY_H

#include <stdint.h>

/*
 * A helper structure to summarize an
 * assembly graph.
 */
struct bsal_assembly_graph_summary {

    uint64_t vertex_count;
    uint64_t vertex_observation_count;
    uint64_t arc_count;
};

void bsal_assembly_graph_summary_init(struct bsal_assembly_graph_summary *self);
void bsal_assembly_graph_summary_destroy(struct bsal_assembly_graph_summary *self);

int bsal_assembly_graph_summary_pack_size(struct bsal_assembly_graph_summary *self);
int bsal_assembly_graph_summary_pack(struct bsal_assembly_graph_summary *self, void *buffer);
int bsal_assembly_graph_summary_unpack(struct bsal_assembly_graph_summary *self, void *buffer);
int bsal_assembly_graph_summary_pack_unpack(struct bsal_assembly_graph_summary *self, int operation, void *buffer);

void bsal_assembly_graph_summary_print(struct bsal_assembly_graph_summary *self);

void bsal_assembly_graph_summary_merge(struct bsal_assembly_graph_summary *self,
            struct bsal_assembly_graph_summary *partial_summary);
void bsal_assembly_graph_summary_add(struct bsal_assembly_graph_summary *self, int coverage, int parent_count,
                int child_count);

#endif
