
#ifndef BIOSAL_PACKER_H
#define BIOSAL_PACKER_H

/*
 * Perform nothing.
 */
#define BIOSAL_PACKER_OPERATION_NO_OPERATION 0

/*
 * Do a dry run to gather the size required by
 * BIOSAL_PACKER_OPERATION_PACK.
 */
#define BIOSAL_PACKER_OPERATION_PACK_SIZE 1

/*
 * Pack stuff in a buffer.
 */
#define BIOSAL_PACKER_OPERATION_PACK 2

/*
 * Unpack stuff.
 */
#define BIOSAL_PACKER_OPERATION_UNPACK 3

#include <stdint.h>

/*
 * This structure is used to pack and unpack things in buffers.
 * Hence the name.
 */
struct biosal_packer {
    int operation;
    int offset;
    void *buffer;
};

void biosal_packer_init(struct biosal_packer *self, int operation, void *buffer);
void biosal_packer_destroy(struct biosal_packer *self);

int biosal_packer_process(struct biosal_packer *self, void *object, int bytes);
int biosal_packer_process_uint64_t(struct biosal_packer *self, uint64_t *object);
int biosal_packer_process_int(struct biosal_packer *self, int *object);

void biosal_packer_rewind(struct biosal_packer *self);
int biosal_packer_get_byte_count(struct biosal_packer *self);
void biosal_packer_print_bytes(void *buffer, int bytes);

#endif
