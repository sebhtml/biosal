#ifndef BIOSAL_BUFFERED_FILE_WRITER_H
#define BIOSAL_BUFFERED_FILE_WRITER_H

#include <core/structures/string.h>

#include <stdio.h>
#include <stdarg.h>

/*
 * A buffered file writer.
 */
struct biosal_buffered_file_writer {
    FILE *descriptor;
    char *buffer;
    char *format_buffer;
    int format_buffer_capacity;
    int buffer_capacity;
    int buffer_length;

    FILE *null_file;

    struct biosal_string file;
};

void biosal_buffered_file_writer_init(struct biosal_buffered_file_writer *self, const char *file);
void biosal_buffered_file_writer_destroy(struct biosal_buffered_file_writer *self);

int biosal_buffered_file_writer_write(struct biosal_buffered_file_writer *self,
                const char *data, int length);
int biosal_buffered_file_writer_printf(struct biosal_buffered_file_writer *self, const char *format,
                ...);

int biosal_buffered_file_writer_flush(struct biosal_buffered_file_writer *self);

size_t biosal_buffered_file_writer_write_back(struct biosal_buffered_file_writer *self,
                const void *buffer, size_t count);

#endif
