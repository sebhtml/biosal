
#include "buffered_file_writer.h"

#include <core/system/memory.h>

#include <stdlib.h>
#include <string.h>

void bsal_buffered_file_writer_init(struct bsal_buffered_file_writer *self, const char *file)
{
    self->descriptor = fopen(file, "w");

    /*
     * Buffer for content
     */
    self->buffer_capacity = 8388608;
    self->buffer_length = 0;
    self->buffer = bsal_memory_allocate(self->buffer_capacity);

    /*
     * Buffer for format
     */
    self->format_buffer_capacity = 2048;
    self->format_buffer = bsal_memory_allocate(self->format_buffer_capacity);

    self->null_file = fopen("/dev/null", "w");
}

void bsal_buffered_file_writer_destroy(struct bsal_buffered_file_writer *self)
{
    bsal_buffered_file_writer_flush(self);

    fclose(self->descriptor);
    self->descriptor = NULL;

    fclose(self->null_file);
    self->null_file = NULL;

    if (self->buffer != NULL) {

        bsal_memory_free(self->buffer);

        self->buffer = NULL;
        self->buffer_capacity = 0;
        self->buffer_length = 0;
    }

    if (self->format_buffer != NULL) {

        bsal_memory_free(self->format_buffer);

        self->format_buffer = NULL;
        self->format_buffer_capacity = 0;
    }
}

int bsal_buffered_file_writer_write(struct bsal_buffered_file_writer *self,
                const char *data, int length)
{
    int available;
    char *destination;

    available = self->buffer_capacity - self->buffer_length;

    /*
     * Flush the buffer.
     */
    if (length > available) {

        bsal_buffered_file_writer_flush(self);

        available = self->buffer_capacity - self->buffer_length;
    }

    /*
     * Store it in the buffer.
     */
    if (length <= available) {

        destination = self->buffer + self->buffer_length;
        memcpy(destination, data, length);
        self->buffer_length += length;

    /*
     * If it is too big, write it directly.
     * into the file.
     */
    } else {

        fwrite(data, 1, length, self->descriptor);
    }

    return length;
}

int bsal_buffered_file_writer_printf(struct bsal_buffered_file_writer *self, const char *format,
                ...)
{
    int bytes;
    int actual_byte_count;
    va_list arguments;

#if 0
    printf("DEBUG bsal_buffered_file_writer_printf %s\n", format);
#endif

    /*
     * Do a dry run to figure out the number of arguments
     *
     * \see http://www.cplusplus.com/reference/cstdio/vfprintf/
     */
    va_start(arguments, format);
    bytes = vfprintf(self->null_file, format, arguments);
    va_end(arguments);

    /*
     * Increase the size of the format buffer.
     */
    if (bytes > self->format_buffer_capacity) {

        bsal_memory_free(self->format_buffer);

        self->format_buffer_capacity = bytes;

        self->format_buffer = bsal_memory_allocate(self->format_buffer_capacity);
    }

    /*
     * \see http://www.cplusplus.com/reference/cstdio/vsprintf/
     */
    va_start(arguments, format);
    actual_byte_count = vsprintf(self->format_buffer, format, arguments);
    va_end(arguments);

    /*
     * Write that to the buffered file writer.
     */
    bsal_buffered_file_writer_write(self, self->format_buffer, actual_byte_count);

    return actual_byte_count;
}

int bsal_buffered_file_writer_flush(struct bsal_buffered_file_writer *self)
{
    int value;

#if 0
    printf("FLUSH !\n");
#endif

    /*
     * \see http://www.cplusplus.com/reference/cstdio/fwrite/
     */
    fwrite(self->buffer, 1, self->buffer_length, self->descriptor);

    value = self->buffer_length;
    self->buffer_length = 0;

    return value;
}
