
#ifndef CORE_PACKER_H
#define CORE_PACKER_H

/*
 * Perform nothing.
 */
#define CORE_PACKER_OPERATION_NO_OPERATION 0

/*
 * Do a dry run to gather the size required by
 * CORE_PACKER_OPERATION_PACK.
 */
#define CORE_PACKER_OPERATION_PACK_SIZE 1

/*
 * Pack stuff in a buffer.
 */
#define CORE_PACKER_OPERATION_PACK 2

/*
 * Unpack stuff.
 */
#define CORE_PACKER_OPERATION_UNPACK 3

#include <stdint.h>

/*
 * This structure is used to pack and unpack things in buffers.
 * Hence the name.
 */
struct core_packer {
    int operation;
    int offset;
    void *buffer;
};

void core_packer_init(struct core_packer *self, int operation, void *buffer);
void core_packer_destroy(struct core_packer *self);

int core_packer_process(struct core_packer *self, void *object, int bytes);
int core_packer_process_uint64_t(struct core_packer *self, uint64_t *object);
int core_packer_process_int(struct core_packer *self, int *object);

void core_packer_rewind(struct core_packer *self);
int core_packer_get_byte_count(struct core_packer *self);
void core_packer_print_bytes(void *buffer, int bytes);

#endif
