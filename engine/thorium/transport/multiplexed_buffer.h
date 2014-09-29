
#ifndef THORIUM_MULTIPLEXED_BUFFER_H
#define THORIUM_MULTIPLEXED_BUFFER_H

/*
 * A multiplexed buffer.
 */
struct thorium_multiplexed_buffer {
    void *buffer;
    int current_size;
    int maximum_size;
    int message_count;
};

#endif
