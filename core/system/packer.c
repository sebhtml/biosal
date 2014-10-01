
#include "packer.h"

#include <core/system/memory.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
#define CORE_PACKER_DEBUG
*/

void core_packer_init(struct core_packer *self, int operation, void *buffer)
{
    self->operation = CORE_PACKER_OPERATION_NO_OPERATION;

#ifdef CORE_PACKER_DEBUG
    printf("DEBUG core_packer_init operation %d buffer %p\n", operation, buffer);
#endif

    /* The buffer is not needed for dry runs
     */
    if (operation == CORE_PACKER_OPERATION_PACK_SIZE) {
        self->buffer = NULL;
        self->offset = 0;
        self->operation = operation;

#ifdef CORE_PACKER_DEBUG
        printf("DEBUG dry run.\n");
#endif

        return;
    }

    if (buffer == NULL) {
        self->operation = CORE_PACKER_OPERATION_NO_OPERATION;
        self->offset = 0;
        self->buffer = buffer;
    }

    if (operation == CORE_PACKER_OPERATION_PACK
                    || operation == CORE_PACKER_OPERATION_UNPACK) {

        self->buffer = buffer;
        self->operation = operation;
        self->offset = 0;
        return;
    }

    self->operation = operation;
    self->offset = 0;
    self->buffer = buffer;

#ifdef CORE_PACKER_DEBUG
    printf("DEBUG core_packer_init config: operation %d\n", self->operation);
#endif
}

void core_packer_destroy(struct core_packer *self)
{
    self->operation = CORE_PACKER_OPERATION_NO_OPERATION;
    self->offset = 0;
    self->buffer = NULL;
}

int core_packer_process(struct core_packer *self, void *object, int bytes)
{

#ifdef CORE_PACKER_DEBUG
    printf("DEBUG ENTRY core_packer_process operation %d object %p bytes %d offset %d buffer %p\n",
                    self->operation,
                    object, bytes, self->offset, self->buffer);
#endif

    if (self->operation == CORE_PACKER_OPERATION_NO_OPERATION) {
        return self->offset;
    }

    if (bytes == 0) {
        return self->offset;
    }

#ifdef CORE_PACKER_DEBUG
    if (self->buffer != NULL) {
        core_packer_print_bytes((char *)self->buffer + self->offset, bytes);
    }
#endif

    if (self->operation == CORE_PACKER_OPERATION_PACK) {
        core_memory_copy((char *)self->buffer + self->offset, object, bytes);

    } else if (self->operation == CORE_PACKER_OPERATION_UNPACK) {

#ifdef CORE_PACKER_DEBUG
        printf("DEBUG unpack !\n");
#endif

        core_memory_copy(object, (char *)self->buffer + self->offset, bytes);

    } else if (self->operation == CORE_PACKER_OPERATION_PACK_SIZE) {
        /* just increase the offset.
         */
    }

#ifdef CORE_PACKER_DEBUG
    if (self->buffer != NULL) {
        core_packer_print_bytes((char *)self->buffer + self->offset, bytes);
    }
#endif


    self->offset += bytes;

#ifdef CORE_PACKER_DEBUG
    printf("DEBUG core_packer_process final offset %d\n", self->offset);
#endif

    return self->offset;
}

void core_packer_rewind(struct core_packer *self)
{
    self->offset = 0;
}

int core_packer_get_byte_count(struct core_packer *self)
{
    return self->offset;
}

void core_packer_print_bytes(void *buffer, int bytes)
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

int core_packer_process_int(struct core_packer *self, int *object)
{
    return core_packer_process(self, object, sizeof(int));
}

int core_packer_process_uint64_t(struct core_packer *self, uint64_t *object)
{
    return core_packer_process(self, object, sizeof(uint64_t));
}
