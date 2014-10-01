
#ifndef BIOSAL_ASSEMBLY_CONNECTIVITY_H
#define BIOSAL_ASSEMBLY_CONNECTIVITY_H

#include <stdint.h>

#define BIOSAL_DNA_ALPHABET_SIZE 4

/*
 * Offsets for parents
 */
#define BIOSAL_ASSEMBLY_CONNECTIVITY_OFFSET_PARENTS 0

/*
 * Offsets for children
 */
#define BIOSAL_ASSEMBLY_CONNECTIVITY_OFFSET_CHILDREN BIOSAL_DNA_ALPHABET_SIZE

/*
 * Connectivity
 */
struct biosal_assembly_connectivity {
    uint8_t bitmap;
};

void biosal_assembly_connectivity_init(struct biosal_assembly_connectivity *self);
void biosal_assembly_connectivity_destroy(struct biosal_assembly_connectivity *self);

/*
 * Functions for parents
 */
int biosal_assembly_connectivity_parent_count(struct biosal_assembly_connectivity *self);
int biosal_assembly_connectivity_get_parent(struct biosal_assembly_connectivity *self, int index);
void biosal_assembly_connectivity_add_parent(struct biosal_assembly_connectivity *self, int symbol_code);
void biosal_assembly_connectivity_delete_parent(struct biosal_assembly_connectivity *self, int symbol_code);

/*
 * Functions for children
 */
int biosal_assembly_connectivity_child_count(struct biosal_assembly_connectivity *self);
int biosal_assembly_connectivity_get_child(struct biosal_assembly_connectivity *self, int index);
void biosal_assembly_connectivity_add_child(struct biosal_assembly_connectivity *self, int symbol_code);
void biosal_assembly_connectivity_delete_child(struct biosal_assembly_connectivity *self, int symbol_code);

/*
 * Other functions
 */
int biosal_assembly_connectivity_parent_offset(int code);
int biosal_assembly_connectivity_child_offset(int code);

int biosal_assembly_connectivity_get_count(struct biosal_assembly_connectivity *self, int type);
int biosal_assembly_connectivity_get_element(struct biosal_assembly_connectivity *self, int index, int type);

void biosal_assembly_connectivity_print(struct biosal_assembly_connectivity *self);

int biosal_assembly_connectivity_pack_size(struct biosal_assembly_connectivity *self);
int biosal_assembly_connectivity_pack(struct biosal_assembly_connectivity *self, void *buffer);
int biosal_assembly_connectivity_unpack(struct biosal_assembly_connectivity *self, void *buffer);
int biosal_assembly_connectivity_pack_unpack(struct biosal_assembly_connectivity *self, int operation, void *buffer);

void biosal_assembly_connectivity_init_copy(struct biosal_assembly_connectivity *self,
                struct biosal_assembly_connectivity *connectivity);
void biosal_assembly_connectivity_invert_arcs(struct biosal_assembly_connectivity *self);

#endif
