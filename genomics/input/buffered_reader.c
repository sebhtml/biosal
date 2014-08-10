
#include "buffered_reader.h"

#include "raw_buffered_reader.h"
#include "gzip_buffered_reader.h"

#include <core/system/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>

/*
#define BSAL_BUFFERED_READER_DEBUG
*/

/*#define BSAL_BUFFERED_READER_BUFFER_SIZE 1048576*/

/*
 * On Mira (Blue Gene/Q with GPFS file system), I/O nodes
 * lock 8-MiB blocks when reading or writing
 */
#define BSAL_BUFFERED_READER_BUFFER_SIZE 8388608

#define GZ_FILE_EXTENSION ".gz"

/*#define BSAL_BUFFERED_READER_BUFFER_SIZE 4194304*/

void bsal_buffered_reader_init(struct bsal_buffered_reader *self,
                const char *file, uint64_t offset)
{
    bsal_buffered_reader_select(self, file);

    self->init(self, file, offset);
}

void bsal_buffered_reader_destroy(struct bsal_buffered_reader *self)
{
    self->destroy(self);

    if (self->concrete_self != NULL) {

        bsal_memory_free(self->concrete_self);
        self->concrete_self = NULL;

        self->init = NULL;
        self->destroy = NULL;
        self->read_line = NULL;
    }
}

int bsal_buffered_reader_read_line(struct bsal_buffered_reader *self,
                char *buffer, int length)
{
    return self->read_line(self, buffer, length);
}

void *bsal_buffered_reader_get_concrete_self(struct bsal_buffered_reader *self)
{
    return self->concrete_self;
}

void bsal_buffered_reader_select(struct bsal_buffered_reader *self, const char *file)
{
    const char *pointer;

    pointer = strstr(file, GZ_FILE_EXTENSION);

    /*
     * Compressed file with gzip.
     */
    if (pointer != NULL
                    && strlen(pointer) == strlen(GZ_FILE_EXTENSION)) {

        self->concrete_self = bsal_memory_allocate(sizeof(struct bsal_gzip_buffered_reader));
        self->init =bsal_gzip_buffered_reader_init;
        self->destroy = bsal_gzip_buffered_reader_destroy;
        self->read_line = bsal_gzip_buffered_reader_read_line;

    } else {
        /*
         * Uncompressed file.
         */
        self->concrete_self = bsal_memory_allocate(sizeof(struct bsal_raw_buffered_reader));
        self->init =bsal_raw_buffered_reader_init;
        self->destroy = bsal_raw_buffered_reader_destroy;
        self->read_line = bsal_raw_buffered_reader_read_line;
    }
}
