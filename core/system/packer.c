
#include "packer.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
#define BSAL_PACKER_DEBUG
*/

void bsal_packer_init(struct bsal_packer *self, int operation, void *buffer)
{
    self->operation = BSAL_PACKER_OPERATION_DO_NOTHING;

#ifdef BSAL_PACKER_DEBUG
    printf("DEBUG bsal_packer_init operation %d buffer %p\n", operation, buffer);
#endif

    /* The buffer is not needed for dry runs
     */
    if (operation == BSAL_PACKER_OPERATION_DRY_RUN) {
        self->buffer = NULL;
        self->offset = 0;
        self->operation = operation;

#ifdef BSAL_PACKER_DEBUG
        printf("DEBUG dry run.\n");
#endif

        return;
    }

    if (buffer == NULL) {
        self->operation = BSAL_PACKER_OPERATION_DO_NOTHING;
        self->offset = 0;
        self->buffer = buffer;
    }

    if (operation == BSAL_PACKER_OPERATION_PACK
                    || operation == BSAL_PACKER_OPERATION_UNPACK) {

        self->buffer = buffer;
        self->operation = operation;
        self->offset = 0;
        return;
    }

    self->operation = operation;
    self->offset = 0;
    self->buffer = buffer;

#ifdef BSAL_PACKER_DEBUG
    printf("DEBUG bsal_packer_init config: operation %d\n", self->operation);
#endif
}

void bsal_packer_destroy(struct bsal_packer *self)
{
    self->operation = BSAL_PACKER_OPERATION_DO_NOTHING;
    self->offset = 0;
    self->buffer = NULL;
}

int bsal_packer_process(struct bsal_packer *self, void *object, int bytes)
{

#ifdef BSAL_PACKER_DEBUG
    printf("DEBUG ENTRY bsal_packer_work operation %d object %p bytes %d offset %d buffer %p\n",
                    self->operation,
                    object, bytes, self->offset, self->buffer);
#endif

    if (self->operation == BSAL_PACKER_OPERATION_DO_NOTHING) {
        return self->offset;
    }

    if (bytes == 0) {
        return self->offset;
    }

#ifdef BSAL_PACKER_DEBUG
    if (self->buffer != NULL) {
        bsal_packer_print_bytes((char *)self->buffer + self->offset, bytes);
    }
#endif

    if (self->operation == BSAL_PACKER_OPERATION_PACK) {
        memcpy((char *)self->buffer + self->offset, object, bytes);

    } else if (self->operation == BSAL_PACKER_OPERATION_UNPACK) {

#ifdef BSAL_PACKER_DEBUG
        printf("DEBUG unpack !\n");
#endif

        memcpy(object, (char *)self->buffer + self->offset, bytes);

    } else if (self->operation == BSAL_PACKER_OPERATION_DRY_RUN) {
        /* just increase the offset.
         */
    }

#ifdef BSAL_PACKER_DEBUG
    if (self->buffer != NULL) {
        bsal_packer_print_bytes((char *)self->buffer + self->offset, bytes);
    }
#endif


    self->offset += bytes;

#ifdef BSAL_PACKER_DEBUG
    printf("DEBUG bsal_packer_work final offset %d\n", self->offset);
#endif

    return self->offset;
}

void bsal_packer_rewind(struct bsal_packer *self)
{
    self->offset = 0;
}

int bsal_packer_worked_bytes(struct bsal_packer *self)
{
    return self->offset;
}

void bsal_packer_print_bytes(void *buffer, int bytes)
{
    int i;
    char byte;
    int *integer_value;

    printf("BYTES, count: %d, starting address: %p", bytes, buffer);

    for (i = 0; i < bytes; i++) {
        byte = ((char *)buffer)[i];
        printf(" %d", (int)byte);
    }

    integer_value = (int *)buffer;

    printf(" integer value: %d\n", *integer_value);
}

int bsal_packer_work(struct bsal_packer *self, void *object, int bytes)
{
    return bsal_packer_process(self, object, bytes);
}

int bsal_packer_process_int(struct bsal_packer *self, int *object)
{
    return bsal_packer_process(self, object, sizeof(int));
}

int bsal_packer_process_uint64_t(struct bsal_packer *self, uint64_t *object)
{
    return bsal_packer_process(self, object, sizeof(uint64_t));
}
