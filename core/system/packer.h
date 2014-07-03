
#ifndef BSAL_PACKER_H
#define BSAL_PACKER_H

#define BSAL_PACKER_OPERATION_DO_NOTHING 0
#define BSAL_PACKER_OPERATION_DRY_RUN 1
#define BSAL_PACKER_OPERATION_PACK 2
#define BSAL_PACKER_OPERATION_UNPACK 3

/* This structure is used to pack and unpack things in buffers
 */
struct bsal_packer {
    int operation;
    int offset;
    void *buffer;
};

void bsal_packer_init(struct bsal_packer *self, int operation, void *buffer);
void bsal_packer_destroy(struct bsal_packer *self);

int bsal_packer_work(struct bsal_packer *self, void *object, int bytes);

void bsal_packer_rewind(struct bsal_packer *self);
int bsal_packer_worked_bytes(struct bsal_packer *self);
void bsal_packer_print_bytes(void *buffer, int bytes);

#endif
