
#ifndef BIOSAL_ASSEMBLY_GRAPH_SUMMARY_H
#define BIOSAL_ASSEMBLY_GRAPH_SUMMARY_H

#include "assembly_connectivity.h"
#include <core/structures/map.h>

#include <stdint.h>

#define BIOSAL_MAXIMUM_DEGREE BIOSAL_DNA_ALPHABET_SIZE
#define BIOSAL_DEGREE_VALUE_COUNT (BIOSAL_MAXIMUM_DEGREE + 1)

/*
 * A helper structure to summarize an
 * assembly graph.
 */
struct biosal_assembly_graph_summary {

    uint64_t vertex_count;
    uint64_t vertex_observation_count;
    uint64_t arc_count;

    uint64_t degree_frequencies[(BIOSAL_DEGREE_VALUE_COUNT * BIOSAL_DEGREE_VALUE_COUNT)];
    struct biosal_map coverage_distribution;
};

void biosal_assembly_graph_summary_init(struct biosal_assembly_graph_summary *self);
void biosal_assembly_graph_summary_destroy(struct biosal_assembly_graph_summary *self);

int biosal_assembly_graph_summary_pack_size(struct biosal_assembly_graph_summary *self);
int biosal_assembly_graph_summary_pack(struct biosal_assembly_graph_summary *self, void *buffer);
int biosal_assembly_graph_summary_unpack(struct biosal_assembly_graph_summary *self, void *buffer);
int biosal_assembly_graph_summary_pack_unpack(struct biosal_assembly_graph_summary *self, int operation, void *buffer);

void biosal_assembly_graph_summary_print(struct biosal_assembly_graph_summary *self);

void biosal_assembly_graph_summary_merge(struct biosal_assembly_graph_summary *self,
            struct biosal_assembly_graph_summary *partial_summary);
void biosal_assembly_graph_summary_add(struct biosal_assembly_graph_summary *self, int coverage, int parent_count,
                int child_count);

uint64_t *biosal_assembly_graph_summary_get_degree_bucket(struct biosal_assembly_graph_summary *self, int parent_count,
                int child_count);

uint64_t biosal_assembly_graph_summary_get_degree_frequency(struct biosal_assembly_graph_summary *self, int parent_count,
                int child_count);
void biosal_assembly_graph_summary_set_degree_frequency(struct biosal_assembly_graph_summary *self, int parent_count,
                int child_count, uint64_t value);
void biosal_assembly_graph_summary_increment_degree_frequency(struct biosal_assembly_graph_summary *self, int parent_count,
                int child_count);

void biosal_assembly_graph_summary_increase_degree_frequency(struct biosal_assembly_graph_summary *self, int parent_count,
                int child_count, uint64_t frequency);

void biosal_assembly_graph_summary_write_summary(struct biosal_assembly_graph_summary *self,
                char *file_name, int kmer_length);

#endif
