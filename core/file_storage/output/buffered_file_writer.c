
#include "buffered_file_writer.h"

#include <core/system/memory.h>

#include <stdlib.h>
#include <string.h>

#define MEMORY_WRITER 0x17f124a5

void biosal_buffered_file_writer_init(struct biosal_buffered_file_writer *self, const char *file)
{
    biosal_string_init(&self->file, file);

    self->descriptor = NULL;

    /*
     * Buffer for content
     */
    self->buffer_capacity = 8388608;
    self->buffer_length = 0;
    self->buffer = biosal_memory_allocate(self->buffer_capacity, MEMORY_WRITER);

    /*
     * Buffer for format
     */
    self->format_buffer_capacity = 2048;
    self->format_buffer = biosal_memory_allocate(self->format_buffer_capacity, MEMORY_WRITER);

    self->null_file = fopen("/dev/null", "w");
}

void biosal_buffered_file_writer_destroy(struct biosal_buffered_file_writer *self)
{
    biosal_buffered_file_writer_flush(self);

    if (self->descriptor != NULL) {
        fclose(self->descriptor);
        self->descriptor = NULL;
    }

    if (self->null_file != NULL) {
        fclose(self->null_file);
        self->null_file = NULL;
    }

    if (self->buffer != NULL) {

        biosal_memory_free(self->buffer, MEMORY_WRITER);

        self->buffer = NULL;
        self->buffer_capacity = 0;
        self->buffer_length = 0;
    }

    if (self->format_buffer != NULL) {

        biosal_memory_free(self->format_buffer, MEMORY_WRITER);

        self->format_buffer = NULL;
        self->format_buffer_capacity = 0;
    }

    biosal_string_destroy(&self->file);
}

int biosal_buffered_file_writer_write(struct biosal_buffered_file_writer *self,
                const char *data, int length)
{
    int available;
    char *destination;

    available = self->buffer_capacity - self->buffer_length;

    /*
     * Flush the buffer.
     */
    if (length > available) {

        biosal_buffered_file_writer_flush(self);

        available = self->buffer_capacity - self->buffer_length;
    }

    /*
     * Store it in the buffer.
     */
    if (length <= available) {

        destination = self->buffer + self->buffer_length;
        biosal_memory_copy(destination, data, length);
        self->buffer_length += length;

    /*
     * If it is too big, write it directly.
     * into the file.
     */
    } else {

        biosal_buffered_file_writer_write_back(self, data, length);
    }

    return length;
}

int biosal_buffered_file_writer_printf(struct biosal_buffered_file_writer *self, const char *format,
                ...)
{
    int bytes;
    int actual_byte_count;
    va_list arguments;

#if 0
    printf("DEBUG biosal_buffered_file_writer_printf %s\n", format);
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

        biosal_memory_free(self->format_buffer, MEMORY_WRITER);

        self->format_buffer_capacity = bytes;

        self->format_buffer = biosal_memory_allocate(self->format_buffer_capacity, MEMORY_WRITER);
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
    biosal_buffered_file_writer_write(self, self->format_buffer, actual_byte_count);

    return actual_byte_count;
}

int biosal_buffered_file_writer_flush(struct biosal_buffered_file_writer *self)
{
    int value;

#if 0
    printf("FLUSH !\n");
#endif

    /*
     * \see http://www.cplusplus.com/reference/cstdio/fwrite/
     */
    biosal_buffered_file_writer_write_back(self, self->buffer, self->buffer_length);

    value = self->buffer_length;
    self->buffer_length = 0;

    return value;
}

size_t biosal_buffered_file_writer_write_back(struct biosal_buffered_file_writer *self,
                const void *buffer, size_t count)
{
    size_t size;
    char *file_name;

    if (self->descriptor == NULL) {

        file_name = biosal_string_get(&self->file);
        self->descriptor = fopen(file_name, "w");
    }

    size = 1;

    return fwrite(buffer, size, count, self->descriptor);
}
