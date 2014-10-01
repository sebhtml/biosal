
#ifndef CORE_BUFFERED_READER_INTERFACE
#define CORE_BUFFERED_READER_INTERFACE

struct core_buffered_reader;

#include <stdint.h>

/*
 * An interface for buffered readers.
 */
struct core_buffered_reader_interface {
    void (*init)(struct core_buffered_reader *self, const char *file, uint64_t offset);
    void (*destroy)(struct core_buffered_reader *self);
    int (*read_line)(struct core_buffered_reader *self, char *buffer, int length);
    int size;
    int (*detect)(struct core_buffered_reader *self, const char *file);
    uint64_t (*get_offset)(struct core_buffered_reader *self);
    int (*get_previous_bytes)(struct core_buffered_reader *self, char *buffer, int length);
};

#endif
