
#ifndef BSAL_ASSEMBLY_CONNECTIVITY_H
#define BSAL_ASSEMBLY_CONNECTIVITY_H

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
 * Connectivity
 */
struct bsal_assembly_connectivity {
    uint8_t bitmap;
};

void bsal_assembly_connectivity_init(struct bsal_assembly_connectivity *self);
void bsal_assembly_connectivity_destroy(struct bsal_assembly_connectivity *self);

/*
 * Functions for parents
 */
int bsal_assembly_connectivity_parent_count(struct bsal_assembly_connectivity *self);
int bsal_assembly_connectivity_get_parent(struct bsal_assembly_connectivity *self, int index);
void bsal_assembly_connectivity_add_parent(struct bsal_assembly_connectivity *self, int symbol_code);
void bsal_assembly_connectivity_delete_parent(struct bsal_assembly_connectivity *self, int symbol_code);

/*
 * Functions for children
 */
int bsal_assembly_connectivity_child_count(struct bsal_assembly_connectivity *self);
int bsal_assembly_connectivity_get_child(struct bsal_assembly_connectivity *self, int index);
void bsal_assembly_connectivity_add_child(struct bsal_assembly_connectivity *self, int symbol_code);
void bsal_assembly_connectivity_delete_child(struct bsal_assembly_connectivity *self, int symbol_code);

/*
 * Other functions
 */
int bsal_assembly_connectivity_parent_offset(int code);
int bsal_assembly_connectivity_child_offset(int code);

int bsal_assembly_connectivity_get_count(struct bsal_assembly_connectivity *self, int type);
int bsal_assembly_connectivity_get_element(struct bsal_assembly_connectivity *self, int index, int type);

void bsal_assembly_connectivity_print(struct bsal_assembly_connectivity *self);

#endif
