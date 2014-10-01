
#ifndef BIOSAL_BUFFERED_READER_INTERFACE
#define BIOSAL_BUFFERED_READER_INTERFACE

struct biosal_buffered_reader;

#include <stdint.h>

/*
 * An interface for buffered readers.
 */
struct biosal_buffered_reader_interface {
    void (*init)(struct biosal_buffered_reader *self, const char *file, uint64_t offset);
    void (*destroy)(struct biosal_buffered_reader *self);
    int (*read_line)(struct biosal_buffered_reader *self, char *buffer, int length);
    int size;
    int (*detect)(struct biosal_buffered_reader *self, const char *file);
    uint64_t (*get_offset)(struct biosal_buffered_reader *self);
    int (*get_previous_bytes)(struct biosal_buffered_reader *self, char *buffer, int length);
};

#endif
