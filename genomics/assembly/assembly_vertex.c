
#include "assembly_vertex.h"

void bsal_assembly_vertex_init(struct bsal_assembly_vertex *self)
{
    self->coverage_depth = 0;

    bsal_assembly_connectivity_init(&self->connectivity);
}

void bsal_assembly_vertex_destroy(struct bsal_assembly_vertex *self)
{
    self->coverage_depth = 0;
    bsal_assembly_connectivity_destroy(&self->connectivity);
}

int bsal_assembly_vertex_coverage_depth(struct bsal_assembly_vertex *self)
{
    return self->coverage_depth;
}

void bsal_assembly_vertex_increase_coverage_depth(struct bsal_assembly_vertex *self,
                int value)
{
    int old_depth;

    old_depth = self->coverage_depth;

    self->coverage_depth += value;

    /*
     * Avoid integer overflow
     */
    if (self->coverage_depth <= 0) {
        self->coverage_depth = old_depth;
    }
}

int bsal_assembly_vertex_child_count(struct bsal_assembly_vertex *self)
{
    return bsal_assembly_connectivity_child_count(&self->connectivity);
}

void bsal_assembly_vertex_add_child(struct bsal_assembly_vertex *self, int symbol_code)
{
    bsal_assembly_connectivity_add_child(&self->connectivity, symbol_code);
}

void bsal_assembly_vertex_delete_child(struct bsal_assembly_vertex *self, int symbol_code)
{
    bsal_assembly_connectivity_delete_child(&self->connectivity, symbol_code);
}

int bsal_assembly_vertex_get_child(struct bsal_assembly_vertex *self, int index)
{
    return bsal_assembly_connectivity_get_child(&self->connectivity, index);
}

int bsal_assembly_vertex_parent_count(struct bsal_assembly_vertex *self)
{
    return bsal_assembly_connectivity_parent_count(&self->connectivity);
}

void bsal_assembly_vertex_add_parent(struct bsal_assembly_vertex *self, int symbol_code)
{
    bsal_assembly_connectivity_add_parent(&self->connectivity, symbol_code);
}

void bsal_assembly_vertex_delete_parent(struct bsal_assembly_vertex *self, int symbol_code)
{
    bsal_assembly_connectivity_delete_parent(&self->connectivity, symbol_code);
}

int bsal_assembly_vertex_get_parent(struct bsal_assembly_vertex *self, int index)
{
    return bsal_assembly_connectivity_get_parent(&self->connectivity, index);
}


