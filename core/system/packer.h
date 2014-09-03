
#ifndef BSAL_PACKER_H
#define BSAL_PACKER_H

#define BSAL_PACKER_OPERATION_DO_NOTHING 0
#define BSAL_PACKER_OPERATION_DRY_RUN 1
#define BSAL_PACKER_OPERATION_PACK 2
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

int bsal_packer_work(struct bsal_packer *self, void *object, int bytes);
int bsal_packer_process(struct bsal_packer *self, void *object, int bytes);
int bsal_packer_process_uint64_t(struct bsal_packer *self, uint64_t *object);
int bsal_packer_process_int(struct bsal_packer *self, int *object);

void bsal_packer_rewind(struct bsal_packer *self);
int bsal_packer_worked_bytes(struct bsal_packer *self);
void bsal_packer_print_bytes(void *buffer, int bytes);

#endif
