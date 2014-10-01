#ifndef CORE_BUFFERED_FILE_WRITER_H
#define CORE_BUFFERED_FILE_WRITER_H

#include <core/structures/string.h>

#include <stdio.h>
#include <stdarg.h>

/*
 * A buffered file writer.
 */
struct core_buffered_file_writer {
    FILE *descriptor;
    char *buffer;
    char *format_buffer;
    int format_buffer_capacity;
    int buffer_capacity;
    int buffer_length;

    FILE *null_file;

    struct core_string file;
};

void core_buffered_file_writer_init(struct core_buffered_file_writer *self, const char *file);
void core_buffered_file_writer_destroy(struct core_buffered_file_writer *self);

int core_buffered_file_writer_write(struct core_buffered_file_writer *self,
                const char *data, int length);
int core_buffered_file_writer_printf(struct core_buffered_file_writer *self, const char *format,
                ...);

int core_buffered_file_writer_flush(struct core_buffered_file_writer *self);

size_t core_buffered_file_writer_write_back(struct core_buffered_file_writer *self,
                const void *buffer, size_t count);

#endif
