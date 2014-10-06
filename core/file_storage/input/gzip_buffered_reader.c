
#include "gzip_buffered_reader.h"

#include "buffered_reader.h"

#include <core/system/memory.h>
#include <core/system/debugger.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>

/*
#define CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
*/

/*#define CORE_BUFFERED_READER_BUFFER_SIZE 1048576*/

/*
 * On Mira (Blue Gene/Q with GPFS file system), I/O nodes
 * lock 8-MiB blocks when reading or writing
 */
#define CORE_BUFFERED_READER_BUFFER_SIZE 8388608
#define GZ_FILE_EXTENSION ".gz"

#define MEMORY_GZIP 0x4480c242

void core_gzip_buffered_reader_init(struct core_buffered_reader *reader,
                const char *file, uint64_t offset);
void core_gzip_buffered_reader_destroy(struct core_buffered_reader *reader);

/*
 * \return number of bytes copied in buffer
 * This does not include the discarded \n, if any
 */
int core_gzip_buffered_reader_read_line(struct core_buffered_reader *reader,
                char *buffer, int length);

/* \return number of bytes copied in buffer
 */
int core_gzip_buffered_reader_pull(struct core_buffered_reader *reader);

void core_gzip_buffered_reader_open(struct core_buffered_reader *self,
                const char *file, uint64_t offset);

int core_gzip_buffered_reader_read(struct core_buffered_reader *self,
                char *buffer, int length);

#ifdef CORE_GZIP_BUFFERED_READER_USE_INFLATE
int core_gzip_buffered_reader_pull_raw(struct core_buffered_reader *self);

int core_gzip_buffered_reader_read_with_inflate(struct core_buffered_reader *self,
                char *buffer, int length);
#endif

int core_gzip_buffered_reader_detect(struct core_buffered_reader *self,
                const char *file);

uint64_t core_gzip_buffered_reader_get_offset(struct core_buffered_reader *self);

int core_gzip_buffered_reader_read_line_private(struct core_buffered_reader *self,
                char *buffer, int length);
int core_gzip_buffered_reader_get_previous_bytes(struct core_buffered_reader *self,
                char *buffer, int length);

struct core_buffered_reader_interface core_gzip_buffered_reader_implementation = {
    .init = core_gzip_buffered_reader_init,
    .destroy = core_gzip_buffered_reader_destroy,
    .read_line = core_gzip_buffered_reader_read_line,
    .detect = core_gzip_buffered_reader_detect,
    .get_offset = core_gzip_buffered_reader_get_offset,
    .get_previous_bytes = core_gzip_buffered_reader_get_previous_bytes,
    .size = sizeof(struct core_gzip_buffered_reader)
};

/*#define CORE_BUFFERED_READER_BUFFER_SIZE 4194304*/

void core_gzip_buffered_reader_init(struct core_buffered_reader *self,
                const char *file, uint64_t offset)
{
    struct core_gzip_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

    core_gzip_buffered_reader_open(self, file, offset);

#ifdef CORE_BUFFERED_READER_DEBUG
    printf("DEBUG fseek %" PRIu64 "\n", offset);
#endif

    reader->buffer = (char *)core_memory_allocate(CORE_BUFFERED_READER_BUFFER_SIZE * sizeof(char), MEMORY_GZIP);
    reader->buffer_capacity = CORE_BUFFERED_READER_BUFFER_SIZE;
    reader->position_in_buffer = 0;
    reader->buffer_size = 0;

    reader->offset = offset;
}

void core_gzip_buffered_reader_destroy(struct core_buffered_reader *self)
{
    struct core_gzip_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

    core_memory_free(reader->buffer, MEMORY_GZIP);

    reader->buffer = NULL;
    reader->buffer_capacity = 0;
    reader->buffer_size = 0;
    reader->position_in_buffer = 0;

#ifdef CORE_GZIP_BUFFERED_READER_USE_INFLATE
    fclose(reader->raw_descriptor);
    reader->raw_descriptor = NULL;

    core_memory_free(reader->input_buffer, MEMORY_GZIP);
    reader->input_buffer = NULL;
    reader->input_buffer_capacity = 0;
    reader->input_buffer_size = 0;
    reader->input_buffer_position = 0;

    if (reader->got_header) {

        inflateEnd(&reader->decompression_stream);

        reader->got_header = 0;
    }
#else
    gzclose(reader->descriptor);
    reader->descriptor = NULL;
#endif
}

int core_gzip_buffered_reader_read_line(struct core_buffered_reader *self,
                char *buffer, int length)
{
    int read;
    struct core_gzip_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);
    read = core_gzip_buffered_reader_read_line_private(self, buffer, length);

    reader->offset += read;

    return read;
}

int core_gzip_buffered_reader_read_line_private(struct core_buffered_reader *self,
                char *buffer, int length)
{
    struct core_gzip_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

    char new_line;
    int position;
    int has_new_line;
    int read;

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

    if (has_new_line) {

        read = position - reader->position_in_buffer;
        core_memory_copy(buffer, reader->buffer + reader->position_in_buffer,
                        read);
        buffer[read] = '\0';

        reader->position_in_buffer += read;

        /* skip '\n'
         */
        reader->position_in_buffer++;

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
        if (core_gzip_buffered_reader_pull(self)) {
            return core_gzip_buffered_reader_read_line_private(self, buffer, length);
        } else {
            return 0;
        }
    }

    /* otherwise, there is nothing to return
     */
    return 0;
}

int core_gzip_buffered_reader_pull(struct core_buffered_reader *self)
{
    int source;
    int destination;
    int count;
    int available;
    int read;
    struct core_gzip_buffered_reader *reader;

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

    /*
     * \see http://www.lemoda.net/c/gzfile-read/
     * \see http://www.zlib.net/manual.html
     */
    read = core_gzip_buffered_reader_read(self,
                    reader->buffer + reader->buffer_size, available);

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

void core_gzip_buffered_reader_open(struct core_buffered_reader *self,
                const char *file, uint64_t offset)
{
    struct core_gzip_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

#ifdef CORE_GZIP_BUFFERED_READER_USE_INFLATE

#ifdef CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE open %s\n",
                    file);
#endif

    /*
     * \see http://www.ncbi.nlm.nih.gov/IEB/ToolBox/CPP_DOC/lxr/source/src/util/compress/zlib/example.c
     * \see http://www.zlib.net/zpipe.c
     */
    reader->raw_descriptor = fopen(file, "r");

    fseek(reader->raw_descriptor, offset, SEEK_SET);

    reader->input_buffer_capacity = CORE_BUFFERED_READER_BUFFER_SIZE;
    reader->input_buffer = core_memory_allocate(reader->input_buffer_capacity, MEMORY_GZIP);

#else
    reader->descriptor = gzopen(file, "r");

    /* seek- in the file
     */
    gzseek(reader->descriptor, offset, SEEK_SET);
#endif
}

int core_gzip_buffered_reader_read(struct core_buffered_reader *self,
                char *buffer, int length)
{
#ifdef CORE_GZIP_BUFFERED_READER_USE_INFLATE
    int read;

#ifdef CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE read max %d bytes in buffer with inflate\n",
                    length);
#endif

    read = core_gzip_buffered_reader_read_with_inflate(self, buffer, length);

    return read;

#else
    int read;
    struct core_gzip_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

    read = gzread(reader->descriptor, buffer, length);

    return read;
#endif

}

#ifdef CORE_GZIP_BUFFERED_READER_USE_INFLATE
int core_gzip_buffered_reader_pull_raw(struct core_buffered_reader *self)
{
    int source;
    int destination;
    int count;
    int available;
    int read;
    struct core_gzip_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

    source = reader->input_buffer_position;
    destination = 0;
    count = reader->input_buffer_size - reader->input_buffer_position;

#ifdef CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE pull_raw, count= %d\n",
                    count);
#endif

    if (destination < source) {
        /* \see http://man7.org/linux/man-pages/man3/core_memory_move.3.html
         * regions may overlap
         */
        core_memory_move(reader->input_buffer + destination, reader->input_buffer + source,
                    count);

#ifdef CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
        printf("DEBUG move data on the left for gz raw buffer\n");
#endif

        reader->input_buffer_position = 0;
        reader->input_buffer_size = count;
    }

    available = reader->input_buffer_capacity - reader->input_buffer_size;

#ifdef CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE pull_raw, available= %d\n",
                    available);
#endif

    /*
     * \see http://www.lemoda.net/c/gzfile-read/
     * \see http://www.zlib.net/manual.html
     */
    read = fread(reader->input_buffer + reader->input_buffer_size, 1,
                    available,
                    reader->raw_descriptor);

#ifdef CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE pull_raw read= %d\n",
                    read);
#endif

    reader->input_buffer_size += read;

#ifdef CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("DEBUG input_buffer_position= %d input_buffer_size= %d input_buffer_capacity %d\n",
                    reader->input_buffer_position,
                    reader->input_buffer_size,
                    reader->input_buffer_capacity);
#endif

    return read;
}
#endif

#ifdef CORE_GZIP_BUFFERED_READER_USE_INFLATE
int core_gzip_buffered_reader_read_with_inflate(struct core_buffered_reader *self,
                char *buffer, int length)
{
    struct core_gzip_buffered_reader *reader;
    int available_in_input;
    char *input;
    int bytes_read_in_input;
    int bytes_written_in_output;
    int return_value;

    reader = core_buffered_reader_get_concrete_self(self);

    /*
     * Pull some bytes into the input buffer.
     */
    core_gzip_buffered_reader_pull_raw(self);

    /*
     * Uncompress at most length bytes from <input_buffer> + <input_buffer_position>
     * the number of available bytes in input is <input_buffer_size> - <input_buffer_position>
     */

    available_in_input = reader->input_buffer_size - reader->input_buffer_position;
    input = reader->input_buffer + reader->input_buffer_position;

#ifdef CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("DEBUG input at position %d\n",
                    reader->input_buffer_position);
#endif

    if (!reader->got_header) {
        reader->decompression_stream.zalloc = Z_NULL;
        reader->decompression_stream.zfree = Z_NULL;
        reader->decompression_stream.opaque = Z_NULL;
        reader->decompression_stream.avail_in = 0;
        reader->decompression_stream.next_in = Z_NULL;

#if 0
    return_value = inflateInit(&decompression_stream);
#endif
    /*
     * \see http://stackoverflow.com/questions/1838699/how-can-i-decompress-a-gzip-stream-with-zlib
     */

        return_value = inflateInit2(&reader->decompression_stream, 16 + MAX_WBITS);
        reader->got_header = 1;

        CORE_DEBUGGER_ASSERT(return_value == Z_OK);

        if (return_value != Z_OK) {
            return -1;
        }
    }

#ifdef CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE read_with_inflate inflateInit Z_OK\n");
#endif

    /*
     * Configure input buffer.
     */
    reader->decompression_stream.avail_in = available_in_input;
    reader->decompression_stream.next_in = (unsigned char *)input;

    /*
     * Configure output buffer.
     */
    reader->decompression_stream.avail_out = length;
    reader->decompression_stream.next_out = (unsigned char *)buffer;

#ifdef CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE read_with_inflate avail_in %d avail_out %d\n",
                    decompression_stream.avail_in,
                    decompression_stream.avail_out);
#endif

    return_value = Z_OK;

    while (return_value == Z_OK) {

        return_value = inflate(&reader->decompression_stream, Z_BLOCK);

#ifdef CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
        printf("while CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE read_with_inflate avail_in %d avail_out %d ret %d\n",
                    decompression_stream.avail_in,
                    decompression_stream.avail_out,
                    return_value);
#endif

        if (return_value == Z_DATA_ERROR) {

            printf("Error: Z_DATA_ERROR\n");

            return 0;
        } else if (return_value == Z_STREAM_ERROR) {

            printf("Error: Z_STREAM_ERROR\n");

            return 0;
        }
    }

    bytes_read_in_input = available_in_input - reader->decompression_stream.avail_in;

    CORE_DEBUGGER_ASSERT(bytes_read_in_input <= available_in_input);

    reader->input_buffer_position += bytes_read_in_input;

    bytes_written_in_output = length - reader->decompression_stream.avail_out;

#ifdef CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("CORE_GZIP_BUFFERED_READER_DEBUG_INFLATE input_read %d output_written %d\n",
                    bytes_read_in_input, bytes_written_in_output);
#endif

    CORE_DEBUGGER_ASSERT(bytes_written_in_output <= length);

    return bytes_written_in_output;
}

#endif

int core_gzip_buffered_reader_detect(struct core_buffered_reader *self,
                const char *file)
{
    const char *pointer;

    pointer = strstr(file, GZ_FILE_EXTENSION);

    /*
     * Compressed file with gzip.
     */
    if (pointer != NULL
                    && strlen(pointer) == strlen(GZ_FILE_EXTENSION)) {

        return 1;
    }

    return 0;
}

uint64_t core_gzip_buffered_reader_get_offset(struct core_buffered_reader *self)
{
    struct core_gzip_buffered_reader *reader;

    reader = core_buffered_reader_get_concrete_self(self);

    return reader->offset;
}

int core_gzip_buffered_reader_get_previous_bytes(struct core_buffered_reader *self,
                char *buffer, int length)
{
    /* TODO:
     * implement this
     */
    return -1;
}
