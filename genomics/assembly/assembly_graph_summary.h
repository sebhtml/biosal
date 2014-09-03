
#ifndef BSAL_ASSEMBLY_GRAPH_SUMMARY_H
#define BSAL_ASSEMBLY_GRAPH_SUMMARY_H

#include "assembly_connectivity.h"
#include <core/structures/map.h>

#include <stdint.h>

#define BSAL_MAXIMUM_DEGREE BSAL_DNA_ALPHABET_SIZE
#define BSAL_DEGREE_VALUE_COUNT (BSAL_MAXIMUM_DEGREE + 1)

/*
 * A helper structure to summarize an
 * assembly graph.
 */
struct bsal_assembly_graph_summary {

    uint64_t vertex_count;
    uint64_t vertex_observation_count;
    uint64_t arc_count;

    uint64_t degree_frequencies[(BSAL_DEGREE_VALUE_COUNT * BSAL_DEGREE_VALUE_COUNT)];
    struct bsal_map coverage_distribution;
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

uint64_t *bsal_assembly_graph_summary_get_degree_bucket(struct bsal_assembly_graph_summary *self, int parent_count,
                int child_count);

uint64_t bsal_assembly_graph_summary_get_degree_frequency(struct bsal_assembly_graph_summary *self, int parent_count,
                int child_count);
void bsal_assembly_graph_summary_set_degree_frequency(struct bsal_assembly_graph_summary *self, int parent_count,
                int child_count, uint64_t value);
void bsal_assembly_graph_summary_increment_degree_frequency(struct bsal_assembly_graph_summary *self, int parent_count,
                int child_count);

void bsal_assembly_graph_summary_increase_degree_frequency(struct bsal_assembly_graph_summary *self, int parent_count,
                int child_count, uint64_t frequency);

void bsal_assembly_graph_summary_write_summary(struct bsal_assembly_graph_summary *self,
                char *file_name, int kmer_length);

#endif
