
#include "multiplexed_buffer.h"

#include <core/system/debugger.h>
#include <core/system/memory.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

/*
 * Print the past history in the prediction engine.
 */
/*
*/
#define PRINT_HISTORY

/*
 * This compilation option enables the future prediction
 * subsystem.
 */
#define PREDICT_FUTURE

#define PREDICTED_VARIATION 0

void thorium_multiplexed_buffer_print_history(struct thorium_multiplexed_buffer *self);
void thorium_multiplexed_buffer_predict(struct thorium_multiplexed_buffer *self);

void thorium_multiplexed_buffer_init(struct thorium_multiplexed_buffer *self,
                int maximum_size, int timeout)
{
    int i;

    self->timeout_ = timeout;

#ifdef THORIUM_MULTIPLEXED_BUFFER_PREDICT_MESSAGE_COUNT
    /*
     * Use a very big value so that the code works
     * with or without PREDICT_FUTURE.
     */
    self->predicted_message_count_ = 99999999;
#endif

    thorium_multiplexed_buffer_reset(self);

    self->maximum_size_ = maximum_size;

#ifdef THORIUM_MULTIPLEXED_BUFFER_PREDICT_MESSAGE_COUNT
    self->prediction_iterator = 0;

    for (i = 0; i < PREDICTION_EVENT_COUNT; ++i) {
        self->prediction_ages[i] = -1;
        self->prediction_buffer_sizes[i] = -1;
        self->prediction_message_count[i] = -1;
    }
#endif
}

void thorium_multiplexed_buffer_destroy(struct thorium_multiplexed_buffer *self)
{
    thorium_multiplexed_buffer_reset(self);

    self->buffer_ = NULL;
    self->maximum_size_ = 0;
}

void thorium_multiplexed_buffer_print(struct thorium_multiplexed_buffer *self)
{
    printf("multiplexed_buffer_print buffer %p timestamp %" PRIu64 " current_size %d"
                    " maximum_size %d message_count %d"
                    "\n", self->buffer_, self->timestamp_,
                    self->current_size_, self->maximum_size_,
                    self->message_count_);
}

uint64_t thorium_multiplexed_buffer_time(struct thorium_multiplexed_buffer *self)
{
    return self->timestamp_;
}

int thorium_multiplexed_buffer_current_size(struct thorium_multiplexed_buffer *self)
{
    return self->current_size_;
}

int thorium_multiplexed_buffer_maximum_size(struct thorium_multiplexed_buffer *self)
{
    return self->maximum_size_;
}

void thorium_multiplexed_buffer_append(struct thorium_multiplexed_buffer *self,
                int count, void *buffer)
{
    void *multiplexed_buffer;
    void *destination_in_buffer;
    int required_size;

    CORE_DEBUGGER_ASSERT(self->buffer_ != NULL);

    required_size = thorium_multiplexed_buffer_required_size(self, count);

    /*
     * Make sure there is enough space.
     */
    CORE_DEBUGGER_ASSERT(self->maximum_size_ - self->current_size_ >= required_size);

    multiplexed_buffer = self->buffer_;
    destination_in_buffer = ((char *)multiplexed_buffer) + self->current_size_;

    /*
     * Append <count><buffer> to the <multiplexed_buffer>
     */
    core_memory_copy(destination_in_buffer, &count, sizeof(count));
    core_memory_copy((char *)destination_in_buffer + sizeof(count),
                    buffer, count);

    /*
     * Add the message.
     */
    CORE_DEBUGGER_ASSERT(self->current_size_ <= self->maximum_size_);
    self->current_size_ += required_size;
    ++self->message_count_;

    CORE_DEBUGGER_ASSERT(self->message_count_ >= 1);
}

int thorium_multiplexed_buffer_required_size(struct thorium_multiplexed_buffer *self,
                int count)
{
    return sizeof(count) + count;
}

void thorium_multiplexed_buffer_set_time(struct thorium_multiplexed_buffer *self,
                uint64_t time)
{
    CORE_DEBUGGER_ASSERT(self->timestamp_ == 0);

    self->timestamp_ = time;
}

void *thorium_multiplexed_buffer_buffer(struct thorium_multiplexed_buffer *self)
{
    return self->buffer_;
}

void thorium_multiplexed_buffer_reset(struct thorium_multiplexed_buffer *self)
{
    self->current_size_ = 0;
    self->message_count_ = 0;
    self->timestamp_ = 0;

    self->buffer_ = NULL;
}

void thorium_multiplexed_buffer_set_buffer(struct thorium_multiplexed_buffer *self,
                void *buffer)
{
    self->buffer_ = buffer;
}

void thorium_multiplexed_buffer_profile(struct thorium_multiplexed_buffer *self, uint64_t time)
{
#ifdef THORIUM_MULTIPLEXED_BUFFER_PREDICT_MESSAGE_COUNT
    self->prediction_ages[self->prediction_iterator] = time - self->timestamp_;
    self->prediction_message_count[self->prediction_iterator] = self->message_count_;
    self->prediction_buffer_sizes[self->prediction_iterator] = self->current_size_;

    ++self->prediction_iterator;

    if (self->prediction_iterator == PREDICTION_EVENT_COUNT) {

#ifdef PRINT_HISTORY
        thorium_multiplexed_buffer_print_history(self);
#endif

#ifdef PREDICT_FUTURE
        thorium_multiplexed_buffer_predict(self);
#endif

        self->prediction_iterator = 0;
    }
#endif
}

void thorium_multiplexed_buffer_print_history(struct thorium_multiplexed_buffer *self)
{
#ifdef THORIUM_MULTIPLEXED_BUFFER_PREDICT_MESSAGE_COUNT
    int i;
    int age;
    int current_size;
    int message_count;

    for (i = 0; i < PREDICTION_EVENT_COUNT; ++i) {

        age = self->prediction_ages[i];
        message_count = self->prediction_message_count[i];
        current_size = self->prediction_buffer_sizes[i];

        printf("MULTIPLEXER DATA #%i age %d buffer_size %d / %d message_count %d\n",
                        i, age, current_size, self->maximum_size_,
                        message_count);
    }
#endif
}

int thorium_multiplexed_buffer_timeout(struct thorium_multiplexed_buffer *self)
{
#ifdef THORIUM_MULTIPLEXED_BUFFER_PREDICT_MESSAGE_COUNT
    /*
     * When the predicted message count is reached, return a timeout of
     * 0 nanoseconds so that the buffer will be flushed right away.
     */
    if (self->message_count_ >= self->predicted_message_count_ + PREDICTED_VARIATION)
        return 0;
#endif

    return self->timeout_;
}

void thorium_multiplexed_buffer_predict(struct thorium_multiplexed_buffer *self)
{
#ifdef THORIUM_MULTIPLEXED_BUFFER_PREDICT_MESSAGE_COUNT
    int i;
    int message_count;
    int predicted_message_count;
    int must_increase;

    must_increase = self->message_count_ == self->predicted_message_count_;

    predicted_message_count = 0;

    /*
     * Use the maximum message count in the past history to
     * predict the message count for the next period.
     *
     * This is O(PREDICTION_EVENT_COUNT).
     */
    for (i = 0; i < PREDICTION_EVENT_COUNT; ++i) {

        message_count = self->prediction_message_count[i];

        if (message_count > predicted_message_count)
            predicted_message_count = message_count;

        /*
         * Break the loop if the maximum count was already found.
         */
        if (predicted_message_count == self->predicted_message_count_)
            break;
    }

    /*
     * Increase the value by 1 if the current period reached the maximum
     * value.
     */
    if (must_increase
                    && predicted_message_count >= self->predicted_message_count_) {
        ++predicted_message_count;
    }

#ifdef PRINT_HISTORY
    printf("DEBUG _predict() (must_increase: %d) -> "
                    "predicted_message_count %d (old %d\n",
                    must_increase, predicted_message_count,
                    self->predicted_message_count_);
#endif

    /*
     * Update the real value for the next period.
     */
    self->predicted_message_count_ = predicted_message_count;
#endif
}
