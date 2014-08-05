
#ifndef _BSAL_ASSEMBLY_VERTEX_H_
#define _BSAL_ASSEMBLY_VERTEX_H_

#include <genomics/data/dna_codec.h>

#include <stdint.h>

/*
 * Offsets for parents
 */
#define BSAL_ASSEMBLY_CONNECTIVITY_OFFSET_PARENTS 0

/*
 * Offsets for children
 */
#define BSAL_ASSEMBLY_CONNECTIVITY_OFFSET_CHILDREN 4




/*
 * Attributes of an assembly vertex
 */
struct bsal_assembly_vertex {

    int coverage_depth;

    /*
     * Connectivity.
     */
    uint8_t bitmap;
};

void bsal_assembly_vertex_init(struct bsal_assembly_vertex *self);
void bsal_assembly_vertex_destroy(struct bsal_assembly_vertex *self);

int bsal_assembly_vertex_coverage_depth(struct bsal_assembly_vertex *self);
void bsal_assembly_vertex_increase_coverage_depth(struct bsal_assembly_vertex *self);

int bsal_assembly_vertex_child_count(struct bsal_assembly_vertex *self);
void bsal_assembly_vertex_add_child(struct bsal_assembly_vertex *self, int symbol_code);
void bsal_assembly_vertex_delete_child(struct bsal_assembly_vertex *self, int symbol_code);
int bsal_assembly_vertex_get_child(struct bsal_assembly_vertex *self, int index);

int bsal_assembly_vertex_parent_count(struct bsal_assembly_vertex *self);
void bsal_assembly_vertex_add_parent(struct bsal_assembly_vertex *self, int symbol_code);
void bsal_assembly_vertex_delete_parent(struct bsal_assembly_vertex *self, int symbol_code);
int bsal_assembly_vertex_get_parent(struct bsal_assembly_vertex *self, int index);

int bsal_assembly_vertex_parent_offset(int code);
int bsal_assembly_vertex_child_offset(int code);

int bsal_assembly_vertex_get_element(struct bsal_assembly_vertex *self, int index, int type);
int bsal_assembly_vertex_get_count(struct bsal_assembly_vertex *self, int type);

#endif
