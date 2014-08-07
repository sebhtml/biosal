
#ifndef _BSAL_ASSEMBLY_VERTEX_H_
#define _BSAL_ASSEMBLY_VERTEX_H_

#include "assembly_connectivity.h"

#include <genomics/data/dna_codec.h>

/*
 * Attributes of an assembly vertex
 */
struct bsal_assembly_vertex {

    int coverage_depth;

    /*
     * Connectivity.
     */
    struct bsal_assembly_connectivity connectivity;
};

void bsal_assembly_vertex_init(struct bsal_assembly_vertex *self);
void bsal_assembly_vertex_destroy(struct bsal_assembly_vertex *self);

int bsal_assembly_vertex_coverage_depth(struct bsal_assembly_vertex *self);
void bsal_assembly_vertex_increase_coverage_depth(struct bsal_assembly_vertex *self, int value);

int bsal_assembly_vertex_child_count(struct bsal_assembly_vertex *self);
void bsal_assembly_vertex_add_child(struct bsal_assembly_vertex *self, int symbol_code);
void bsal_assembly_vertex_delete_child(struct bsal_assembly_vertex *self, int symbol_code);
int bsal_assembly_vertex_get_child(struct bsal_assembly_vertex *self, int index);

int bsal_assembly_vertex_parent_count(struct bsal_assembly_vertex *self);
void bsal_assembly_vertex_add_parent(struct bsal_assembly_vertex *self, int symbol_code);
void bsal_assembly_vertex_delete_parent(struct bsal_assembly_vertex *self, int symbol_code);
int bsal_assembly_vertex_get_parent(struct bsal_assembly_vertex *self, int index);

void bsal_assembly_vertex_print(struct bsal_assembly_vertex *self);

#endif
