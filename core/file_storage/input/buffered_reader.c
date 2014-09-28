
#include "buffered_reader.h"

#include "raw_buffered_reader.h"
#include "gzip_buffered_reader.h"

#include <core/system/memory.h>
#include <core/system/debugger.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>

/*
#define BSAL_BUFFERED_READER_DEBUG
*/

/*#define BSAL_BUFFERED_READER_BUFFER_SIZE 1048576*/

/*#define BSAL_BUFFERED_READER_BUFFER_SIZE 4194304*/

#define MEMORY_READER 0x5fb8b2cc

void bsal_buffered_reader_init(struct bsal_buffered_reader *self,
                const char *file, uint64_t offset)
{
    self->concrete_self = NULL;
    self->interface = NULL;

    bsal_buffered_reader_select(self, file);

    if (self->interface != NULL) {
        self->concrete_self = bsal_memory_allocate(self->interface->size, MEMORY_READER);
        self->interface->init(self, file, offset);
    }
}

void bsal_buffered_reader_destroy(struct bsal_buffered_reader *self)
{
    self->interface->destroy(self);

    if (self->concrete_self != NULL) {

        bsal_memory_free(self->concrete_self, MEMORY_READER);
        self->concrete_self = NULL;

        self->interface = NULL;
    }
}

int bsal_buffered_reader_read_line(struct bsal_buffered_reader *self,
                char *buffer, int length)
{
    int return_value;

    return_value = self->interface->read_line(self, buffer, length);

#if 0
    printf("DEBUG READ_LINE %d %s\n", return_value, buffer);
#endif

    return return_value;
}

void *bsal_buffered_reader_get_concrete_self(struct bsal_buffered_reader *self)
{
    return self->concrete_self;
}

void bsal_buffered_reader_select(struct bsal_buffered_reader *self, const char *file)
{
    if (bsal_gzip_buffered_reader_implementation.detect(self, file)) {

        self->interface = &bsal_gzip_buffered_reader_implementation;
    } else {

        self->interface = &bsal_raw_buffered_reader_implementation;
    }
}

uint64_t bsal_buffered_reader_get_offset(struct bsal_buffered_reader *self)
{
    BSAL_DEBUGGER_ASSERT(self != NULL);
    BSAL_DEBUGGER_ASSERT(self->interface != NULL);

    return self->interface->get_offset(self);
}

int bsal_buffered_reader_get_previous_bytes(struct bsal_buffered_reader *self, char *buffer, int length)
{
    return self->interface->get_previous_bytes(self, buffer, length);
}
