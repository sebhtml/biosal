
#include "assembly_vertex.h"

#include <engine/thorium/actor.h>

#include <core/helpers/bitmap.h>

#include <core/system/packer.h>
#include <core/system/debugger.h>

#include <stdio.h>

void biosal_assembly_vertex_init(struct biosal_assembly_vertex *self)
{
    self->coverage_depth = 0;

    CORE_BITMAP_CLEAR_FLAGS(self->flags);

    biosal_assembly_vertex_clear_flag(self, BIOSAL_VERTEX_FLAG_USED_BY_WALKER);
    biosal_assembly_vertex_clear_flag(self, BIOSAL_VERTEX_FLAG_TIP);
    biosal_assembly_vertex_clear_flag(self, BIOSAL_VERTEX_FLAG_BUBBLE);
    biosal_assembly_vertex_clear_flag(self, BIOSAL_VERTEX_FLAG_VISITED);
    biosal_assembly_vertex_clear_flag(self, BIOSAL_VERTEX_FLAG_UNITIG);
    biosal_assembly_vertex_clear_flag(self, BIOSAL_VERTEX_FLAG_PROCESSED_BY_VISITOR);

    biosal_assembly_vertex_set_last_actor(self, THORIUM_ACTOR_NOBODY, -1);

    biosal_assembly_connectivity_init(&self->connectivity);
}

void biosal_assembly_vertex_destroy(struct biosal_assembly_vertex *self)
{
    self->coverage_depth = 0;
    biosal_assembly_connectivity_destroy(&self->connectivity);
}

int biosal_assembly_vertex_coverage_depth(struct biosal_assembly_vertex *self)
{
    return self->coverage_depth;
}

void biosal_assembly_vertex_increase_coverage_depth(struct biosal_assembly_vertex *self,
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

int biosal_assembly_vertex_child_count(struct biosal_assembly_vertex *self)
{
    return biosal_assembly_connectivity_child_count(&self->connectivity);
}

void biosal_assembly_vertex_add_child(struct biosal_assembly_vertex *self, int symbol_code)
{
    biosal_assembly_connectivity_add_child(&self->connectivity, symbol_code);
}

void biosal_assembly_vertex_delete_child(struct biosal_assembly_vertex *self, int symbol_code)
{
    biosal_assembly_connectivity_delete_child(&self->connectivity, symbol_code);
}

int biosal_assembly_vertex_get_child(struct biosal_assembly_vertex *self, int index)
{
    return biosal_assembly_connectivity_get_child(&self->connectivity, index);
}

int biosal_assembly_vertex_parent_count(struct biosal_assembly_vertex *self)
{
    return biosal_assembly_connectivity_parent_count(&self->connectivity);
}

void biosal_assembly_vertex_add_parent(struct biosal_assembly_vertex *self, int symbol_code)
{
    biosal_assembly_connectivity_add_parent(&self->connectivity, symbol_code);
}

void biosal_assembly_vertex_delete_parent(struct biosal_assembly_vertex *self, int symbol_code)
{
    biosal_assembly_connectivity_delete_parent(&self->connectivity, symbol_code);
}

int biosal_assembly_vertex_get_parent(struct biosal_assembly_vertex *self, int index)
{
    return biosal_assembly_connectivity_get_parent(&self->connectivity, index);
}

void biosal_assembly_vertex_print(struct biosal_assembly_vertex *self)
{
    printf("BioSAL::AssemblyVertex coverage_depth: %d connectivity: ",
                    self->coverage_depth);

    biosal_assembly_connectivity_print(&self->connectivity);

    printf("\n");
}

int biosal_assembly_vertex_pack_size(struct biosal_assembly_vertex *self)
{
    return biosal_assembly_vertex_pack_unpack(self, CORE_PACKER_OPERATION_PACK_SIZE, NULL);
}

int biosal_assembly_vertex_pack(struct biosal_assembly_vertex *self, void *buffer)
{
    return biosal_assembly_vertex_pack_unpack(self, CORE_PACKER_OPERATION_PACK, buffer);
}

int biosal_assembly_vertex_unpack(struct biosal_assembly_vertex *self, void *buffer)
{
    return biosal_assembly_vertex_pack_unpack(self, CORE_PACKER_OPERATION_UNPACK, buffer);
}

int biosal_assembly_vertex_pack_unpack(struct biosal_assembly_vertex *self, int operation, void *buffer)
{
    struct core_packer packer;
    int bytes;

    bytes = 0;

    core_packer_init(&packer, operation, buffer);

    bytes += core_packer_process(&packer, &self->coverage_depth, sizeof(self->coverage_depth));
    bytes += core_packer_process(&packer, &self->flags, sizeof(self->flags));
    bytes += core_packer_process(&packer, &self->last_actor, sizeof(self->last_actor));
    bytes += core_packer_process(&packer, &self->last_path_index, sizeof(self->last_path_index));

    core_packer_destroy(&packer);

    bytes += biosal_assembly_connectivity_pack_unpack(&self->connectivity, operation,
                    (char *)buffer + bytes);

    return bytes;
}

void biosal_assembly_vertex_init_copy(struct biosal_assembly_vertex *self,
                struct biosal_assembly_vertex *vertex)
{
    self->coverage_depth = vertex->coverage_depth;
    self->flags = vertex->flags;
    self->last_actor = vertex->last_actor;
    self->last_path_index = vertex->last_path_index;

    biosal_assembly_connectivity_init_copy(&self->connectivity, &vertex->connectivity);
}

void biosal_assembly_vertex_invert_arcs(struct biosal_assembly_vertex *self)
{
    biosal_assembly_connectivity_invert_arcs(&self->connectivity);
}

void biosal_assembly_vertex_set_last_actor(struct biosal_assembly_vertex *self, int last_actor,
                int last_path_index)
{
    self->last_actor = last_actor;
    self->last_path_index = last_path_index;
}

int biosal_assembly_vertex_last_actor(struct biosal_assembly_vertex *self)
{
    return self->last_actor;
}

int biosal_assembly_vertex_last_path_index(struct biosal_assembly_vertex *self)
{
    return self->last_path_index;
}

void biosal_assembly_vertex_set_flag(struct biosal_assembly_vertex *self, int flag)
{
    CORE_DEBUGGER_ASSERT(flag >= BIOSAL_VERTEX_FLAG_START_VALUE);
    CORE_DEBUGGER_ASSERT(flag <= BIOSAL_VERTEX_FLAG_END_VALUE);

    CORE_BITMAP_SET_FLAG(self->flags, flag);
}

void biosal_assembly_vertex_clear_flag(struct biosal_assembly_vertex *self, int flag)
{
    CORE_BITMAP_CLEAR_FLAG(self->flags, flag);
}

int biosal_assembly_vertex_get_flag(struct biosal_assembly_vertex *self, int flag)
{
    return CORE_BITMAP_GET_FLAG(self->flags, flag);
}

void biosal_assembly_vertex_init_empty(struct biosal_assembly_vertex *self)
{
    biosal_assembly_vertex_init(self);
}


