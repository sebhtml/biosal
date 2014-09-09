
#include "assembly_vertex.h"

#include <engine/thorium/actor.h>

#include <core/helpers/bitmap.h>

#include <core/system/packer.h>
#include <core/system/debugger.h>

#include <stdio.h>

void bsal_assembly_vertex_init(struct bsal_assembly_vertex *self)
{
    self->coverage_depth = 0;

    self->flags = 0;

    bsal_assembly_vertex_clear_flag(self, BSAL_VERTEX_STATE_USED);
    bsal_assembly_vertex_clear_flag(self, BSAL_VERTEX_STATE_TIP);
    bsal_assembly_vertex_clear_flag(self, BSAL_VERTEX_STATE_BUBBLE);

    bsal_assembly_vertex_set_last_actor(self, THORIUM_ACTOR_NOBODY, -1);

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

void bsal_assembly_vertex_print(struct bsal_assembly_vertex *self)
{
    printf("BioSAL::AssemblyVertex coverage_depth: %d connectivity: ",
                    self->coverage_depth);

    bsal_assembly_connectivity_print(&self->connectivity);

    printf("\n");
}

int bsal_assembly_vertex_pack_size(struct bsal_assembly_vertex *self)
{
    return bsal_assembly_vertex_pack_unpack(self, BSAL_PACKER_OPERATION_PACK_SIZE, NULL);
}

int bsal_assembly_vertex_pack(struct bsal_assembly_vertex *self, void *buffer)
{
    return bsal_assembly_vertex_pack_unpack(self, BSAL_PACKER_OPERATION_PACK, buffer);
}

int bsal_assembly_vertex_unpack(struct bsal_assembly_vertex *self, void *buffer)
{
    return bsal_assembly_vertex_pack_unpack(self, BSAL_PACKER_OPERATION_UNPACK, buffer);
}

int bsal_assembly_vertex_pack_unpack(struct bsal_assembly_vertex *self, int operation, void *buffer)
{
    struct bsal_packer packer;
    int bytes;

    bytes = 0;

    bsal_packer_init(&packer, operation, buffer);

    bytes += bsal_packer_process(&packer, &self->coverage_depth, sizeof(self->coverage_depth));
    bytes += bsal_packer_process(&packer, &self->flags, sizeof(self->flags));
    bytes += bsal_packer_process(&packer, &self->last_actor, sizeof(self->last_actor));
    bytes += bsal_packer_process(&packer, &self->last_path_index, sizeof(self->last_path_index));

    bsal_packer_destroy(&packer);

    bytes += bsal_assembly_connectivity_pack_unpack(&self->connectivity, operation,
                    (char *)buffer + bytes);

    return bytes;
}

void bsal_assembly_vertex_init_copy(struct bsal_assembly_vertex *self,
                struct bsal_assembly_vertex *vertex)
{
    self->coverage_depth = vertex->coverage_depth;
    self->flags = vertex->flags;
    self->last_actor = vertex->last_actor;
    self->last_path_index = vertex->last_path_index;

    bsal_assembly_connectivity_init_copy(&self->connectivity, &vertex->connectivity);
}

void bsal_assembly_vertex_invert_arcs(struct bsal_assembly_vertex *self)
{
    bsal_assembly_connectivity_invert_arcs(&self->connectivity);
}

void bsal_assembly_vertex_set_last_actor(struct bsal_assembly_vertex *self, int last_actor,
                int last_path_index)
{
    self->last_actor = last_actor;
    self->last_path_index = last_path_index;
}

int bsal_assembly_vertex_last_actor(struct bsal_assembly_vertex *self)
{
    return self->last_actor;
}

int bsal_assembly_vertex_last_path_index(struct bsal_assembly_vertex *self)
{
    return self->last_path_index;
}

void bsal_assembly_vertex_set_flag(struct bsal_assembly_vertex *self, int flag)
{
    bsal_bitmap_set_bit_uint32_t(&self->flags, flag);
}

void bsal_assembly_vertex_clear_flag(struct bsal_assembly_vertex *self, int flag)
{
    bsal_bitmap_clear_bit_uint32_t(&self->flags, flag);
}

int bsal_assembly_vertex_get_flag(struct bsal_assembly_vertex *self, int flag)
{
    return bsal_bitmap_get_bit_uint32_t(&self->flags, flag);
}


