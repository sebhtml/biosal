
#include "buffered_reader.h"

#include <system/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>

/*
#define BSAL_BUFFERED_READER_DEBUG
*/

#define BSAL_BUFFERED_READER_BUFFER_SIZE 4194304

void bsal_buffered_reader_init(struct bsal_buffered_reader *reader,
                const char *file, uint64_t offset)
{
    reader->descriptor = fopen(file, "r");

    /* seek- in the file
     */
    fseek(reader->descriptor, offset, SEEK_SET);

#ifdef BSAL_BUFFERED_READER_DEBUG
    printf("DEBUG fseek %" PRIu64 "\n", offset);
#endif

    reader->buffer = (char *)bsal_malloc(BSAL_BUFFERED_READER_BUFFER_SIZE * sizeof(char));
    reader->buffer_capacity = BSAL_BUFFERED_READER_BUFFER_SIZE;
    reader->position_in_buffer = 0;
    reader->buffer_size = 0;

    reader->dummy = 0;
}

void bsal_buffered_reader_destroy(struct bsal_buffered_reader *reader)
{
    bsal_free(reader->buffer);

    reader->buffer = NULL;
    reader->buffer_capacity = 0;
    reader->buffer_size = 0;
    reader->position_in_buffer = 0;

    fclose(reader->descriptor);
    reader->descriptor = NULL;
}

int bsal_buffered_reader_read_line(struct bsal_buffered_reader *reader,
                char *buffer, int length)
{
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
        if (bsal_buffered_reader_pull(reader)) {
            return bsal_buffered_reader_read_line(reader, buffer, length);
        } else {
            return 0;
        }
    }

    /* otherwise, there is nothing to return
     */
    return 0;
}

int bsal_buffered_reader_pull(struct bsal_buffered_reader *reader)
{
    int source;
    int destination;
    int count;
    int available;
    int read;

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

    /* \see http://www.cplusplus.com/reference/cstdio/fread/
     */
    read = fread(reader->buffer + reader->buffer_size, 1, available,
                    reader->descriptor);

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

