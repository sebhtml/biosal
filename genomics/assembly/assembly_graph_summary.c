
#include "assembly_graph_summary.h"

#include <core/system/packer.h>
#include <core/system/debugger.h>

#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <inttypes.h>

void bsal_assembly_graph_summary_init(struct bsal_assembly_graph_summary *self)
{
    int i;
    int j;
    int size;

    self->vertex_count = 0;
    self->vertex_observation_count = 0;
    self->arc_count = 0;

    size = BSAL_DNA_ALPHABET_SIZE;

    for (i = 0; i <= BSAL_DNA_ALPHABET_SIZE; ++i) {
        for (j = 0; j <= BSAL_DNA_ALPHABET_SIZE; ++j) {
            bsal_assembly_graph_summary_set_degree_frequency(self, i, j, 0);
        }
    }
}

void bsal_assembly_graph_summary_destroy(struct bsal_assembly_graph_summary *self)
{
    self->vertex_count = 0;
    self->vertex_observation_count = 0;
    self->arc_count = 0;
}

int bsal_assembly_graph_summary_pack_size(struct bsal_assembly_graph_summary *self)
{
    int bytes;

    bytes = bsal_assembly_graph_summary_pack_unpack(self, BSAL_PACKER_OPERATION_DRY_RUN, NULL);

    return bytes;
}

int bsal_assembly_graph_summary_pack(struct bsal_assembly_graph_summary *self, void *buffer)
{
    return bsal_assembly_graph_summary_pack_unpack(self, BSAL_PACKER_OPERATION_PACK, buffer);
}

int bsal_assembly_graph_summary_unpack(struct bsal_assembly_graph_summary *self, void *buffer)
{
    return bsal_assembly_graph_summary_pack_unpack(self, BSAL_PACKER_OPERATION_UNPACK, buffer);
}

int bsal_assembly_graph_summary_pack_unpack(struct bsal_assembly_graph_summary *self, int operation, void *buffer)
{
    int count;
    struct bsal_packer packer;
    int bytes;

    bsal_packer_init(&packer, operation, buffer);
    bsal_packer_process_uint64_t(&packer, &self->vertex_count);
    bsal_packer_process_uint64_t(&packer, &self->vertex_observation_count);
    bsal_packer_process_uint64_t(&packer, &self->arc_count);

    bytes = BSAL_DEGREE_VALUE_COUNT * BSAL_DEGREE_VALUE_COUNT * sizeof(uint64_t);
    bsal_packer_process(&packer, &self->degree_frequencies, bytes);

    count = bsal_packer_get_byte_count(&packer);
    bsal_packer_destroy(&packer);

    return count;
}

void bsal_assembly_graph_summary_print(struct bsal_assembly_graph_summary *self)
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

void bsal_assembly_graph_summary_merge(struct bsal_assembly_graph_summary *self,
            struct bsal_assembly_graph_summary *partial_summary)
{
    int range;
    int parent_count;
    int child_count;
    uint64_t frequency;

    self->vertex_count += partial_summary->vertex_count;
    self->vertex_observation_count += partial_summary->vertex_observation_count;
    self->arc_count += partial_summary->arc_count;

    range = BSAL_MAXIMUM_DEGREE;

    for (parent_count = 0; parent_count <= range; ++parent_count) {
        for (child_count = 0; child_count <= range; ++child_count) {

            frequency = bsal_assembly_graph_summary_get_degree_frequency(partial_summary, parent_count, child_count);
            bsal_assembly_graph_summary_increase_degree_frequency(self, parent_count, child_count, frequency);
        }
    }
}

void bsal_assembly_graph_summary_add(struct bsal_assembly_graph_summary *self, int coverage, int parent_count,
                int child_count)
{
    ++self->vertex_count;
    self->vertex_observation_count += coverage;

    /*
     * Only count one extermity of each arc.
     */
    self->arc_count += child_count;

    bsal_assembly_graph_summary_increment_degree_frequency(self, parent_count, child_count);
}

uint64_t *bsal_assembly_graph_summary_get_degree_bucket(struct bsal_assembly_graph_summary *self, int parent_count,
                int child_count)
{
    BSAL_DEBUGGER_ASSERT(parent_count >= 0);
    BSAL_DEBUGGER_ASSERT(parent_count <= BSAL_DNA_ALPHABET_SIZE);
    BSAL_DEBUGGER_ASSERT(child_count >= 0);
    BSAL_DEBUGGER_ASSERT(child_count <= BSAL_DNA_ALPHABET_SIZE);

    return self->degree_frequencies + parent_count * BSAL_DEGREE_VALUE_COUNT + child_count;
}

uint64_t bsal_assembly_graph_summary_get_degree_frequency(struct bsal_assembly_graph_summary *self, int parent_count,
                int child_count)
{
    uint64_t *bucket;
    bucket = bsal_assembly_graph_summary_get_degree_bucket(self, parent_count, child_count);
    return *bucket;
}

void bsal_assembly_graph_summary_set_degree_frequency(struct bsal_assembly_graph_summary *self, int parent_count,
                int child_count, uint64_t value)
{
    uint64_t *bucket;
    bucket = bsal_assembly_graph_summary_get_degree_bucket(self, parent_count, child_count);
    *bucket = value;
}

void bsal_assembly_graph_summary_increase_degree_frequency(struct bsal_assembly_graph_summary *self, int parent_count,
                int child_count, uint64_t frequency)
{
    uint64_t value;
    value = bsal_assembly_graph_summary_get_degree_frequency(self, parent_count, child_count);

#if 0
    printf("old value %" PRIu64 "\n", value);
#endif

    bsal_assembly_graph_summary_set_degree_frequency(self, parent_count, child_count, value + frequency);
}

void bsal_assembly_graph_summary_increment_degree_frequency(struct bsal_assembly_graph_summary *self, int parent_count,
                int child_count)
{
    bsal_assembly_graph_summary_increase_degree_frequency(self, parent_count, child_count, 1);
}

void bsal_assembly_graph_summary_write_summary(struct bsal_assembly_graph_summary *self,
                char *file_name, int kmer_length)
{
    FILE *file;
    uint64_t frequency;
    int parent_count;
    int child_count;
    int range;

    range = BSAL_DNA_ALPHABET_SIZE;

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

            frequency = bsal_assembly_graph_summary_get_degree_frequency(self, parent_count, child_count);

            fprintf(file, "    <class><child_count>%d></child_count>"
                            "<parent_count>%d</parent_count>"
                            "<frequency>%" PRIu64 "</frequency></class>\n",
                            parent_count, child_count, frequency);
        }
    }

    fprintf(file, "</connectivity>\n");

    fprintf(file, "</assembly_graph_summary>\n");

    fclose(file);
}
