
#ifndef BSAL_BUFFERED_READER_INTERFACE
#define BSAL_BUFFERED_READER_INTERFACE

struct bsal_buffered_reader;

#include <stdint.h>

/*
 * An interface for buffered readers.
 */
struct bsal_buffered_reader_interface {
    void (*init)(struct bsal_buffered_reader *self, const char *file, uint64_t offset);
    void (*destroy)(struct bsal_buffered_reader *self);
    int (*read_line)(struct bsal_buffered_reader *self, char *buffer, int length);
    int size;
    int (*detect)(struct bsal_buffered_reader *self, const char *file);
};

#endif
