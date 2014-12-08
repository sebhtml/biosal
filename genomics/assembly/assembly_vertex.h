
#ifndef _BIOSAL_ASSEMBLY_VERTEX_H_
#define _BIOSAL_ASSEMBLY_VERTEX_H_

#include "assembly_connectivity.h"

#include <genomics/data/dna_codec.h>

typedef int coverage_t;

#define BIOSAL_VERTEX_FLAG_START_VALUE 0

#define BIOSAL_VERTEX_FLAG_USED_BY_WALKER 0
#define BIOSAL_VERTEX_FLAG_TIP 1
#define BIOSAL_VERTEX_FLAG_BUBBLE 2
#define BIOSAL_VERTEX_FLAG_VISITED 3
#define BIOSAL_VERTEX_FLAG_UNITIG 4
#define BIOSAL_VERTEX_FLAG_PROCESSED_BY_VISITOR 5

#define BIOSAL_VERTEX_FLAG_END_VALUE 5
/*
 * Attributes of an assembly vertex
 */
struct biosal_assembly_vertex {

    int coverage_depth;
    uint32_t flags;

    int last_actor;
    int last_path_index;

    /*
     * Connectivity.
     */
    struct biosal_assembly_connectivity connectivity;
};

void biosal_assembly_vertex_init(struct biosal_assembly_vertex *self);
void biosal_assembly_vertex_init_copy(struct biosal_assembly_vertex *self,
                struct biosal_assembly_vertex *vertex);
void biosal_assembly_vertex_init_empty(struct biosal_assembly_vertex *self);

void biosal_assembly_vertex_destroy(struct biosal_assembly_vertex *self);

int biosal_assembly_vertex_coverage_depth(struct biosal_assembly_vertex *self);
void biosal_assembly_vertex_increase_coverage_depth(struct biosal_assembly_vertex *self, int value);

int biosal_assembly_vertex_child_count(struct biosal_assembly_vertex *self);
void biosal_assembly_vertex_add_child(struct biosal_assembly_vertex *self, int symbol_code);
void biosal_assembly_vertex_delete_child(struct biosal_assembly_vertex *self, int symbol_code);
int biosal_assembly_vertex_get_child(struct biosal_assembly_vertex *self, int index);

int biosal_assembly_vertex_parent_count(struct biosal_assembly_vertex *self);
void biosal_assembly_vertex_add_parent(struct biosal_assembly_vertex *self, int symbol_code);
void biosal_assembly_vertex_delete_parent(struct biosal_assembly_vertex *self, int symbol_code);
int biosal_assembly_vertex_get_parent(struct biosal_assembly_vertex *self, int index);

void biosal_assembly_vertex_print(struct biosal_assembly_vertex *self);

int biosal_assembly_vertex_pack_size(struct biosal_assembly_vertex *self);
int biosal_assembly_vertex_pack(struct biosal_assembly_vertex *self, void *buffer);
int biosal_assembly_vertex_unpack(struct biosal_assembly_vertex *self, void *buffer);
int biosal_assembly_vertex_pack_unpack(struct biosal_assembly_vertex *self, int operation, void *buffer);

void biosal_assembly_vertex_invert_arcs(struct biosal_assembly_vertex *self);

void biosal_assembly_vertex_set_flag(struct biosal_assembly_vertex *self, int flag);
void biosal_assembly_vertex_clear_flag(struct biosal_assembly_vertex *self, int flag);
int biosal_assembly_vertex_get_flag(struct biosal_assembly_vertex *self, int flag);

void biosal_assembly_vertex_set_last_actor(struct biosal_assembly_vertex *self, int last_actor, int last_path_index);
int biosal_assembly_vertex_last_actor(struct biosal_assembly_vertex *self);

int biosal_assembly_vertex_last_path_index(struct biosal_assembly_vertex *self);

#endif
