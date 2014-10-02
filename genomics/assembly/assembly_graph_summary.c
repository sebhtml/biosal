
#include "assembly_graph_summary.h"

#include <core/system/packer.h>
#include <core/system/debugger.h>

#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <inttypes.h>

void biosal_assembly_graph_summary_init(struct biosal_assembly_graph_summary *self)
{
    int i;
    int j;
    int size;

    self->vertex_count = 0;
    self->vertex_observation_count = 0;
    self->arc_count = 0;

    size = BIOSAL_DNA_ALPHABET_SIZE;

    for (i = 0; i <= size; ++i) {
        for (j = 0; j <= size; ++j) {
            biosal_assembly_graph_summary_set_degree_frequency(self, i, j, 0);
        }
    }
}

void biosal_assembly_graph_summary_destroy(struct biosal_assembly_graph_summary *self)
{
    self->vertex_count = 0;
    self->vertex_observation_count = 0;
    self->arc_count = 0;
}

int biosal_assembly_graph_summary_pack_size(struct biosal_assembly_graph_summary *self)
{
    int bytes;

    bytes = biosal_assembly_graph_summary_pack_unpack(self, CORE_PACKER_OPERATION_PACK_SIZE, NULL);

    return bytes;
}

int biosal_assembly_graph_summary_pack(struct biosal_assembly_graph_summary *self, void *buffer)
{
    return biosal_assembly_graph_summary_pack_unpack(self, CORE_PACKER_OPERATION_PACK, buffer);
}

int biosal_assembly_graph_summary_unpack(struct biosal_assembly_graph_summary *self, void *buffer)
{
    return biosal_assembly_graph_summary_pack_unpack(self, CORE_PACKER_OPERATION_UNPACK, buffer);
}

int biosal_assembly_graph_summary_pack_unpack(struct biosal_assembly_graph_summary *self, int operation, void *buffer)
{
    int count;
    struct core_packer packer;
    int bytes;

    core_packer_init(&packer, operation, buffer);
    core_packer_process_uint64_t(&packer, &self->vertex_count);
    core_packer_process_uint64_t(&packer, &self->vertex_observation_count);
    core_packer_process_uint64_t(&packer, &self->arc_count);

    bytes = BIOSAL_DEGREE_VALUE_COUNT * BIOSAL_DEGREE_VALUE_COUNT * sizeof(uint64_t);
    core_packer_process(&packer, &self->degree_frequencies, bytes);

    count = core_packer_get_byte_count(&packer);
    core_packer_destroy(&packer);

    return count;
}

void biosal_assembly_graph_summary_print(struct biosal_assembly_graph_summary *self)
{
    printf("GRAPH -> ");
    printf(" %" PRIu64 " vertices,",
                    self->vertex_count);
    printf(" %" PRIu64 " vertex observations,",
                    self->vertex_observation_count);
    printf(" and %" PRIu64 " arcs.",
                    self->arc_count);

    printf("\n");
}

void biosal_assembly_graph_summary_merge(struct biosal_assembly_graph_summary *self,
            struct biosal_assembly_graph_summary *partial_summary)
{
    int range;
    int parent_count;
    int child_count;
    uint64_t frequency;

    self->vertex_count += partial_summary->vertex_count;
    self->vertex_observation_count += partial_summary->vertex_observation_count;
    self->arc_count += partial_summary->arc_count;

    range = BIOSAL_MAXIMUM_DEGREE;

    for (parent_count = 0; parent_count <= range; ++parent_count) {
        for (child_count = 0; child_count <= range; ++child_count) {

            frequency = biosal_assembly_graph_summary_get_degree_frequency(partial_summary, parent_count, child_count);
            biosal_assembly_graph_summary_increase_degree_frequency(self, parent_count, child_count, frequency);
        }
    }
}

void biosal_assembly_graph_summary_add(struct biosal_assembly_graph_summary *self, int coverage, int parent_count,
                int child_count)
{
    ++self->vertex_count;
    self->vertex_observation_count += coverage;

    /*
     * Only count one extermity of each arc.
     */
    self->arc_count += child_count;

    biosal_assembly_graph_summary_increment_degree_frequency(self, parent_count, child_count);
}

uint64_t *biosal_assembly_graph_summary_get_degree_bucket(struct biosal_assembly_graph_summary *self, int parent_count,
                int child_count)
{
    CORE_DEBUGGER_ASSERT(parent_count >= 0);
    CORE_DEBUGGER_ASSERT(parent_count <= BIOSAL_DNA_ALPHABET_SIZE);
    CORE_DEBUGGER_ASSERT(child_count >= 0);
    CORE_DEBUGGER_ASSERT(child_count <= BIOSAL_DNA_ALPHABET_SIZE);

    return self->degree_frequencies + parent_count * BIOSAL_DEGREE_VALUE_COUNT + child_count;
}

uint64_t biosal_assembly_graph_summary_get_degree_frequency(struct biosal_assembly_graph_summary *self, int parent_count,
                int child_count)
{
    uint64_t *bucket;
    bucket = biosal_assembly_graph_summary_get_degree_bucket(self, parent_count, child_count);
    return *bucket;
}

void biosal_assembly_graph_summary_set_degree_frequency(struct biosal_assembly_graph_summary *self, int parent_count,
                int child_count, uint64_t value)
{
    uint64_t *bucket;
    bucket = biosal_assembly_graph_summary_get_degree_bucket(self, parent_count, child_count);
    *bucket = value;
}

void biosal_assembly_graph_summary_increase_degree_frequency(struct biosal_assembly_graph_summary *self, int parent_count,
                int child_count, uint64_t frequency)
{
    uint64_t value;
    value = biosal_assembly_graph_summary_get_degree_frequency(self, parent_count, child_count);

#if 0
    printf("old value %" PRIu64 "\n", value);
#endif

    biosal_assembly_graph_summary_set_degree_frequency(self, parent_count, child_count, value + frequency);
}

void biosal_assembly_graph_summary_increment_degree_frequency(struct biosal_assembly_graph_summary *self, int parent_count,
                int child_count)
{
    biosal_assembly_graph_summary_increase_degree_frequency(self, parent_count, child_count, 1);
}

void biosal_assembly_graph_summary_write_summary(struct biosal_assembly_graph_summary *self,
                char *file_name, int kmer_length)
{
    FILE *file;
    uint64_t frequency;
    int parent_count;
    int child_count;
    int range;

    range = BIOSAL_DNA_ALPHABET_SIZE;

    file = fopen(file_name, "w");

    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(file, "<assembly_graph_summary>\n");

    fprintf(file, "<kmer_length>%d</kmer_length>\n", kmer_length);
    fprintf(file, "<vertex_count>%" PRIu64 "</vertex_count>\n", self->vertex_count);
    fprintf(file, "<arc_count>%" PRIu64 "</arc_count>\n", self->arc_count);
    fprintf(file, "<vertex_observation_count>%" PRIu64 "</vertex_observation_count>\n",
                    self->vertex_observation_count);

    fprintf(file, "<connectivity>\n");

    for (parent_count = 0; parent_count <= range; ++parent_count) {
        for (child_count = 0; child_count <= range; ++child_count) {

            frequency = biosal_assembly_graph_summary_get_degree_frequency(self, parent_count, child_count);

            fprintf(file, "    <class>"
                            "<parent_count>%d</parent_count>"
                            "<child_count>%d></child_count>"
                            "<frequency>%" PRIu64 "</frequency></class>\n",
                            parent_count, child_count, frequency);
        }
    }

    fprintf(file, "</connectivity>\n");
    fprintf(file, "</assembly_graph_summary>\n");

    fclose(file);
}
