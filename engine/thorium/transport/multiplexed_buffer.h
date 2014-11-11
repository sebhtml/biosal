
#ifndef THORIUM_MULTIPLEXED_BUFFER_H
#define THORIUM_MULTIPLEXED_BUFFER_H

#include <stdint.h>

/*
 * A multiplexed buffer.
 */
struct thorium_multiplexed_buffer {
    void *buffer_;
    uint64_t timestamp_;
    int current_size_;
    int maximum_size_;
    int message_count_;
};

void thorium_multiplexed_buffer_init(struct thorium_multiplexed_buffer *self,
                int maximum_size, void *buffer);
void thorium_multiplexed_buffer_destroy(struct thorium_multiplexed_buffer *self);

void thorium_multiplexed_buffer_print(struct thorium_multiplexed_buffer *self);
uint64_t thorium_multiplexed_buffer_time(struct thorium_multiplexed_buffer *self);
int thorium_multiplexed_buffer_current_size(struct thorium_multiplexed_buffer *self);
int thorium_multiplexed_buffer_maximum_size(struct thorium_multiplexed_buffer *self);

void thorium_multiplexed_buffer_append(struct thorium_multiplexed_buffer *self,
                int count, void *buffer);
int thorium_multiplexed_buffer_required_size(struct thorium_multiplexed_buffer *self,
                int count);
void thorium_multiplexed_buffer_set_time(struct thorium_multiplexed_buffer *self,
                uint64_t time);

void *thorium_multiplexed_buffer_buffer(struct thorium_multiplexed_buffer *self);

void thorium_multiplexed_buffer_reset(struct thorium_multiplexed_buffer *self);

#endif
