
#ifndef BSAL_PACKER_H
#define BSAL_PACKER_H

/*
 * Perform nothing.
 */
#define BSAL_PACKER_OPERATION_NO_OPERATION 0

/*
 * Do a dry run to gather the size required by
 * BSAL_PACKER_OPERATION_PACK.
 */
#define BSAL_PACKER_OPERATION_PACK_SIZE 1

/*
 * Pack stuff in a buffer.
 */
#define BSAL_PACKER_OPERATION_PACK 2

/*
 * Unpack stuff.
 */
#define BSAL_PACKER_OPERATION_UNPACK 3

#include <stdint.h>

/*
 * This structure is used to pack and unpack things in buffers.
 * Hence the name.
 */
struct bsal_packer {
    int operation;
    int offset;
    void *buffer;
};

void bsal_packer_init(struct bsal_packer *self, int operation, void *buffer);
void bsal_packer_destroy(struct bsal_packer *self);

int bsal_packer_process(struct bsal_packer *self, void *object, int bytes);
int bsal_packer_process_uint64_t(struct bsal_packer *self, uint64_t *object);
int bsal_packer_process_int(struct bsal_packer *self, int *object);

void bsal_packer_rewind(struct bsal_packer *self);
int bsal_packer_get_byte_count(struct bsal_packer *self);
void bsal_packer_print_bytes(void *buffer, int bytes);

#endif
