
#ifndef THORIUM_MULTIPLEXED_BUFFER_H
#define THORIUM_MULTIPLEXED_BUFFER_H

#include <stdint.h>

/*
 * A multiplexed buffer.
 */
struct thorium_multiplexed_buffer {
    void *buffer;
    uint64_t timestamp;
    int current_size;
    int maximum_size;
    int message_count;
};

#endif
