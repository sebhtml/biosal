
#include "raw_buffered_reader.h"

#include "buffered_reader.h"

#include <core/system/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>

#define MEMORY_RAW_READER 0xf853376c

/*
#define CORE_BUFFERED_READER_DEBUG
*/

/*#define CORE_BUFFERED_READER_BUFFER_SIZE 1048576*/

/*
 * On Mira (Blue Gene/Q with GPFS file system), I/O nodes
 * lock 8-MiB blocks when reading or writing
 */
#define CORE_BUFFERED_READER_BUFFER_SIZE 8388608

/*#define CORE_BUFFERED_READER_BUFFER_SIZE 4194304*/

void core_raw_buffered_reader_init(struct core_buffered_reader *self,
                const char *file, uint64_t offset);
void core_raw_buffered_reader_destroy(struct core_buffered_reader *self);

/*
 * \return number of bytes copied in buffer
 * This does not include the discarded \n, if any
 */
int core_raw_buffered_reader_read_line(struct core_buffered_reader *self,
                char *buffer, int length);

/* \return number of bytes copied in buffer
 */
int core_raw_buffered_reader_pull(struct core_buffered_reader *self);

int core_raw_buffered_reader_detect(struct core_buffered_reader *self,
                const char *file);
uint64_t core_raw_buffered_reader_get_offset(struct core_buffered_reader *self);

int core_raw_buffered_reader_read_line_private(struct core_buffered_reader *self,
                char *buffer, int length);
int core_raw_buffered_reader_get_previous_bytes(struct core_buffered_reader *self,
                char *buffer, int length);

struct core_buffered_reader_interface core_raw_buffered_reader_implementation = {
    .init = core_raw_buffered_reader_init,
    .destroy = core_raw_buffered_reader_destroy,
    .read_line = core_raw_buffered_reader_read_line,
    .detect = core_raw_buffered_reader_detect,
    .get_offset = core_raw_buffered_reader_get_offset,
    .get_previous_bytes = core_raw_buffered_reader_get_previous_bytes,
    .size = sizeof(struct core_raw_buffered_reader)
};

void core_raw_buffered_reader_init(struct core_buffered_reader *self,
                const char *file, uint64_t offset)
{
    struct core_raw_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

    reader->descriptor = fopen(file, "r");

    /* seek- in the file
     */
    fseek(reader->descriptor, offset, SEEK_SET);

#ifdef CORE_BUFFERED_READER_DEBUG
    printf("DEBUG fseek %" PRIu64 "\n", offset);
#endif

    reader->buffer = (char *)core_memory_allocate(CORE_BUFFERED_READER_BUFFER_SIZE * sizeof(char), MEMORY_RAW_READER);
    reader->buffer_capacity = CORE_BUFFERED_READER_BUFFER_SIZE;
    reader->position_in_buffer = 0;
    reader->buffer_size = 0;

    reader->offset = offset;
}

void core_raw_buffered_reader_destroy(struct core_buffered_reader *self)
{
    struct core_raw_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

    core_memory_free(reader->buffer, MEMORY_RAW_READER);

    reader->buffer = NULL;
    reader->buffer_capacity = 0;
    reader->buffer_size = 0;
    reader->position_in_buffer = 0;

    fclose(reader->descriptor);
    reader->descriptor = NULL;
}

int core_raw_buffered_reader_read_line(struct core_buffered_reader *self,
                char *buffer, int length)
{
    int read;
    struct core_raw_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

    read = core_raw_buffered_reader_read_line_private(self, buffer, length);

    reader->offset += read;

#if 0
    printf("OFFSET <%s> read %d new offset %" PRIu64 "\n",
                    buffer, read, reader->offset);
#endif

    return read;
}

int core_raw_buffered_reader_read_line_private(struct core_buffered_reader *self,
                char *buffer, int length)
{
    struct core_raw_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

    char new_line;
    int position;
    int has_new_line;
    int read;
    int end_of_file;

    /* the last character in the buffer is '\n'
     * discard everything
     */
    if (reader->position_in_buffer == reader->buffer_size) {
        reader->buffer_size = 0;
        reader->position_in_buffer = 0;
    }

    new_line = '\n';
    position = reader->position_in_buffer;
    has_new_line = 0;

    while (position < reader->buffer_size) {
        if (reader->buffer[position] == new_line) {
            has_new_line = 1;
            break;
        }
        position++;
    }

    /*
     * Keep the new line if any
     */

    if (has_new_line) {
        ++position;
    }

    end_of_file = feof(reader->descriptor);

    /*
     * Return the line
     */
    if (has_new_line || end_of_file) {

        read = position - reader->position_in_buffer;

        if (read > 0)
            core_memory_copy(buffer, reader->buffer + reader->position_in_buffer,
                        read);
        buffer[read] = '\0';

        /* skip the line and '\n' if any
         */
        reader->position_in_buffer += read;

#ifdef CORE_BUFFERED_READER_DEBUG9
        printf("DEBUG core_buffered_reader_read_line has line"
                        "  %i to %i-1 : %s\n", position,
                        reader->position_in_buffer,
                        buffer);
#endif

        return read;
    } else {
        /* try to pull some data and do a recursive call
         */
        if (core_raw_buffered_reader_pull(self)) {
            return core_raw_buffered_reader_read_line_private(self, buffer, length);
        } else {
            return 0;
        }
    }

    /* otherwise, there is nothing to return
     */
    return 0;
}

int core_raw_buffered_reader_pull(struct core_buffered_reader *self)
{
    int source;
    int destination;
    int count;
    int available;
    int read;
    struct core_raw_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

#ifdef CORE_BUFFERED_READER_DEBUG
    printf("DEBUG ENTERING position_in_buffer %i, buffer_size %i\n",
                    reader->position_in_buffer,
                    reader->buffer_size);
#endif

    source = reader->position_in_buffer;
    destination = 0;
    count = reader->buffer_size - reader->position_in_buffer;

#ifdef CORE_BUFFERED_READER_DEBUG
    printf("DEBUG core_buffered_reader_pull moving data %i to %i\n",
                    source, destination);
#endif

    if (destination < source) {
        /* \see http://man7.org/linux/man-pages/man3/core_memory_move.3.html
         * regions may overlap
         */
        core_memory_move(reader->buffer + destination, reader->buffer + source,
                    count);

        reader->position_in_buffer = 0;
        reader->buffer_size = count;
    }

    available = reader->buffer_capacity - reader->buffer_size;

    /* \see http://www.cplusplus.com/reference/cstdio/fread/
     */
    read = fread(reader->buffer + reader->buffer_size, 1, available,
                    reader->descriptor);

#ifdef CORE_BUFFERED_READER_DEBUG
    printf("DEBUG core_buffered_reader_pull available %i, read %i\n",
                    available, read);
#endif

    reader->buffer_size += read;

#ifdef CORE_BUFFERED_READER_DEBUG
    printf("DEBUG EXITING position_in_buffer %i, buffer_size %i\n",
                    reader->position_in_buffer,
                    reader->buffer_size);
#endif

    return read;
}

int core_raw_buffered_reader_detect(struct core_buffered_reader *self,
                const char *file)
{
    return 1;
}

uint64_t core_raw_buffered_reader_get_offset(struct core_buffered_reader *self)
{
    struct core_raw_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

    return reader->offset;
}

int core_raw_buffered_reader_get_previous_bytes(struct core_buffered_reader *self,
                char *buffer, int length)
{
#ifdef CHECK_PREVIOUS
    uint64_t saved_offset;
    uint64_t offset;
    struct core_raw_buffered_reader *reader;
    int bytes;
    uint64_t to_read;

    reader = core_buffered_reader_get_concrete_self(self);
    saved_offset = ftell(reader->descriptor);

    offset = saved_offset;

    if ((uint64_t)length < offset) {

        to_read = offset;
        offset = 0;

    } else {

        to_read = length;
        offset -= length;
    }

    bytes = fread(buffer, 1, to_read, reader->descriptor);

    fseek(reader->descriptor, saved_offset, SEEK_SET);

    return bytes;
#else

    return -1;
#endif
}
