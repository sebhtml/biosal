
#include "assembly_graph_summary.h"

#include <core/system/packer.h>

#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <inttypes.h>

void bsal_assembly_graph_summary_init(struct bsal_assembly_graph_summary *self)
{
    self->vertex_count = 0;
    self->vertex_observation_count = 0;
    self->arc_count = 0;
}

void bsal_assembly_graph_summary_destroy(struct bsal_assembly_graph_summary *self)
{
    self->vertex_count = 0;
    self->vertex_observation_count = 0;
    self->arc_count = 0;
}

int bsal_assembly_graph_summary_pack_size(struct bsal_assembly_graph_summary *self)
{
    return bsal_assembly_graph_summary_pack_unpack(self, BSAL_PACKER_OPERATION_DRY_RUN, NULL);
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

    bsal_packer_init(&packer, operation, buffer);
    bsal_packer_process_uint64_t(&packer, &self->vertex_count);
    bsal_packer_process_uint64_t(&packer, &self->vertex_observation_count);
    bsal_packer_process_uint64_t(&packer, &self->arc_count);
    count = bsal_packer_worked_bytes(&packer);
    bsal_packer_destroy(&packer);

    return count;
}

void bsal_assembly_graph_summary_print(struct bsal_assembly_graph_summary *self)
{
    printf("GRAPH -> ");

    printf(" %" PRIu64 " vertices,",
                    2 * self->vertex_count);
    printf(" %" PRIu64 " vertex observations,",
                    2 * self->vertex_observation_count);
    printf(" and %" PRIu64 " arcs.",
                    2 * self->arc_count);

    printf("\n");
}

void bsal_assembly_graph_summary_merge(struct bsal_assembly_graph_summary *self,
            struct bsal_assembly_graph_summary *partial_summary)
{
    self->vertex_count += partial_summary->vertex_count;
    self->vertex_observation_count += partial_summary->vertex_observation_count;
    self->arc_count += partial_summary->arc_count;
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
}
