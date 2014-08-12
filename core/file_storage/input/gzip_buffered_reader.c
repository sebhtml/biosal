
#include "gzip_buffered_reader.h"

#include "buffered_reader.h"

#include <core/system/memory.h>
#include <core/system/debugger.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>

/*
#define BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
*/

/*#define BSAL_BUFFERED_READER_BUFFER_SIZE 1048576*/

/*
 * On Mira (Blue Gene/Q with GPFS file system), I/O nodes
 * lock 8-MiB blocks when reading or writing
 */
#define BSAL_BUFFERED_READER_BUFFER_SIZE 8388608

/*#define BSAL_BUFFERED_READER_BUFFER_SIZE 4194304*/

void bsal_gzip_buffered_reader_init(struct bsal_buffered_reader *self,
                const char *file, uint64_t offset)
{
    struct bsal_gzip_buffered_reader *reader;

    reader = bsal_buffered_reader_get_concrete_self(self);

    bsal_gzip_buffered_reader_open(self, file, offset);

#ifdef BSAL_BUFFERED_READER_DEBUG
    printf("DEBUG fseek %" PRIu64 "\n", offset);
#endif

    reader->buffer = (char *)bsal_memory_allocate(BSAL_BUFFERED_READER_BUFFER_SIZE * sizeof(char));
    reader->buffer_capacity = BSAL_BUFFERED_READER_BUFFER_SIZE;
    reader->position_in_buffer = 0;
    reader->buffer_size = 0;
}

void bsal_gzip_buffered_reader_destroy(struct bsal_buffered_reader *self)
{
    struct bsal_gzip_buffered_reader *reader;

    reader = bsal_buffered_reader_get_concrete_self(self);

    bsal_memory_free(reader->buffer);

    reader->buffer = NULL;
    reader->buffer_capacity = 0;
    reader->buffer_size = 0;
    reader->position_in_buffer = 0;

#ifdef BSAL_GZIP_BUFFERED_READER_USE_INFLATE
    fclose(reader->raw_descriptor);
    reader->raw_descriptor = NULL;

    bsal_memory_free(reader->input_buffer);
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

int bsal_gzip_buffered_reader_read_line(struct bsal_buffered_reader *self,
                char *buffer, int length)
{
    struct bsal_gzip_buffered_reader *reader;

    reader = bsal_buffered_reader_get_concrete_self(self);

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
        memcpy(buffer, reader->buffer + reader->position_in_buffer,
                        read);
        buffer[read] = '\0';

        reader->position_in_buffer += read;

        /* skip '\n'
         */
        reader->position_in_buffer++;

#ifdef BSAL_BUFFERED_READER_DEBUG9
        printf("DEBUG bsal_buffered_reader_read_line has line"
                        "  %i to %i-1 : %s\n", position,
                        reader->position_in_buffer,
                        buffer);
#endif

        return read;
    } else {
        /* try to pull some data and do a recursive call
         */
        if (bsal_gzip_buffered_reader_pull(self)) {
            return bsal_buffered_reader_read_line(self, buffer, length);
        } else {
            return 0;
        }
    }

    /* otherwise, there is nothing to return
     */
    return 0;
}

int bsal_gzip_buffered_reader_pull(struct bsal_buffered_reader *self)
{
    int source;
    int destination;
    int count;
    int available;
    int read;
    struct bsal_gzip_buffered_reader *reader;

    reader = bsal_buffered_reader_get_concrete_self(self);

#ifdef BSAL_BUFFERED_READER_DEBUG
    printf("DEBUG ENTERING position_in_buffer %i, buffer_size %i\n",
                    reader->position_in_buffer,
                    reader->buffer_size);
#endif

    source = reader->position_in_buffer;
    destination = 0;
    count = reader->buffer_size - reader->position_in_buffer;

#ifdef BSAL_BUFFERED_READER_DEBUG
    printf("DEBUG bsal_buffered_reader_pull moving data %i to %i\n",
                    source, destination);
#endif

    if (destination < source) {
        /* \see http://man7.org/linux/man-pages/man3/memmove.3.html
         * regions may overlap
         */
        memmove(reader->buffer + destination, reader->buffer + source,
                    count);

        reader->position_in_buffer = 0;
        reader->buffer_size = count;
    }

    available = reader->buffer_capacity - reader->buffer_size;

    /*
     * \see http://www.lemoda.net/c/gzfile-read/
     * \see http://www.zlib.net/manual.html
     */
    read = bsal_gzip_buffered_reader_read(self,
                    reader->buffer + reader->buffer_size, available);

#ifdef BSAL_BUFFERED_READER_DEBUG
    printf("DEBUG bsal_buffered_reader_pull available %i, read %i\n",
                    available, read);
#endif

    reader->buffer_size += read;

#ifdef BSAL_BUFFERED_READER_DEBUG
    printf("DEBUG EXITING position_in_buffer %i, buffer_size %i\n",
                    reader->position_in_buffer,
                    reader->buffer_size);
#endif

    return read;
}

void bsal_gzip_buffered_reader_open(struct bsal_buffered_reader *self,
                const char *file, uint64_t offset)
{
    struct bsal_gzip_buffered_reader *reader;

    reader = bsal_buffered_reader_get_concrete_self(self);

#ifdef BSAL_GZIP_BUFFERED_READER_USE_INFLATE

#ifdef BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE open %s\n",
                    file);
#endif

    /*
     * \see http://www.ncbi.nlm.nih.gov/IEB/ToolBox/CPP_DOC/lxr/source/src/util/compress/zlib/example.c
     * \see http://www.zlib.net/zpipe.c
     */
    reader->raw_descriptor = fopen(file, "r");

    fseek(reader->raw_descriptor, offset, SEEK_SET);

    reader->input_buffer_capacity = BSAL_BUFFERED_READER_BUFFER_SIZE;
    reader->input_buffer = bsal_memory_allocate(reader->input_buffer_capacity);

#else
    reader->descriptor = gzopen(file, "r");

    /* seek- in the file
     */
    gzseek(reader->descriptor, offset, SEEK_SET);
#endif
}

int bsal_gzip_buffered_reader_read(struct bsal_buffered_reader *self,
                char *buffer, int length)
{
#ifdef BSAL_GZIP_BUFFERED_READER_USE_INFLATE
    int read;

#ifdef BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE read max %d bytes in buffer with inflate\n",
                    length);
#endif

    read = bsal_gzip_buffered_reader_read_with_inflate(self, buffer, length);

    return read;

#else
    int read;
    struct bsal_gzip_buffered_reader *reader;

    reader = bsal_buffered_reader_get_concrete_self(self);

    read = gzread(reader->descriptor, buffer, length);

    return read;
#endif

}

#ifdef BSAL_GZIP_BUFFERED_READER_USE_INFLATE
int bsal_gzip_buffered_reader_pull_raw(struct bsal_buffered_reader *self)
{
    int source;
    int destination;
    int count;
    int available;
    int read;
    struct bsal_gzip_buffered_reader *reader;

    reader = bsal_buffered_reader_get_concrete_self(self);

    source = reader->input_buffer_position;
    destination = 0;
    count = reader->input_buffer_size - reader->input_buffer_position;

#ifdef BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE pull_raw, count= %d\n",
                    count);
#endif

    if (destination < source) {
        /* \see http://man7.org/linux/man-pages/man3/memmove.3.html
         * regions may overlap
         */
        memmove(reader->input_buffer + destination, reader->input_buffer + source,
                    count);

#ifdef BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
        printf("DEBUG move data on the left for gz raw buffer\n");
#endif

        reader->input_buffer_position = 0;
        reader->input_buffer_size = count;
    }

    available = reader->input_buffer_capacity - reader->input_buffer_size;

#ifdef BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE pull_raw, available= %d\n",
                    available);
#endif

    /*
     * \see http://www.lemoda.net/c/gzfile-read/
     * \see http://www.zlib.net/manual.html
     */
    read = fread(reader->input_buffer + reader->input_buffer_size, 1,
                    available,
                    reader->raw_descriptor);

#ifdef BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE pull_raw read= %d\n",
                    read);
#endif

    reader->input_buffer_size += read;

#ifdef BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("DEBUG input_buffer_position= %d input_buffer_size= %d input_buffer_capacity %d\n",
                    reader->input_buffer_position,
                    reader->input_buffer_size,
                    reader->input_buffer_capacity);
#endif

    return read;
}
#endif

#ifdef BSAL_GZIP_BUFFERED_READER_USE_INFLATE
int bsal_gzip_buffered_reader_read_with_inflate(struct bsal_buffered_reader *self,
                char *buffer, int length)
{
    struct bsal_gzip_buffered_reader *reader;
    int available_in_input;
    char *input;
    int bytes_read_in_input;
    int bytes_written_in_output;
    int return_value;

    reader = bsal_buffered_reader_get_concrete_self(self);

    /*
     * Pull some bytes into the input buffer.
     */
    bsal_gzip_buffered_reader_pull_raw(self);

    /*
     * Uncompress at most length bytes from <input_buffer> + <input_buffer_position>
     * the number of available bytes in input is <input_buffer_size> - <input_buffer_position>
     */

    available_in_input = reader->input_buffer_size - reader->input_buffer_position;
    input = reader->input_buffer + reader->input_buffer_position;

#ifdef BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
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

        BSAL_DEBUGGER_ASSERT(return_value == Z_OK);

        if (return_value != Z_OK) {
            return -1;
        }
    }

#ifdef BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE read_with_inflate inflateInit Z_OK\n");
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

#ifdef BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE read_with_inflate avail_in %d avail_out %d\n",
                    decompression_stream.avail_in,
                    decompression_stream.avail_out);
#endif

    return_value = Z_OK;

    while (return_value == Z_OK) {

        return_value = inflate(&reader->decompression_stream, Z_BLOCK);

#ifdef BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
        printf("while BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE read_with_inflate avail_in %d avail_out %d ret %d\n",
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

    BSAL_DEBUGGER_ASSERT(bytes_read_in_input <= available_in_input);

    reader->input_buffer_position += bytes_read_in_input;

    bytes_written_in_output = length - reader->decompression_stream.avail_out;

#ifdef BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE
    printf("BSAL_GZIP_BUFFERED_READER_DEBUG_INFLATE input_read %d output_written %d\n",
                    bytes_read_in_input, bytes_written_in_output);
#endif

    BSAL_DEBUGGER_ASSERT(bytes_written_in_output <= length);

    return bytes_written_in_output;
}

#endif
