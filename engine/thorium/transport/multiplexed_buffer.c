
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

#define NO_TIME 0
#define EVALUATION_PERIOD 512
/*
#define UPDATE_TIMEOUT_DYNAMICALLY
*/

/*
 * This corresponds to a message promotion rate of 5%.
 */
#define MESSAGE_COUNT_PER_PARCEL 20

/*
#define PRINT_TIMEOUT_UPDATE
*/

void thorium_multiplexed_buffer_print_history(struct thorium_multiplexed_buffer *self);
void thorium_multiplexed_buffer_predict(struct thorium_multiplexed_buffer *self);

void thorium_multiplexed_buffer_profile(struct thorium_multiplexed_buffer *self, uint64_t time);
void thorium_multiplexed_buffer_profile_for_prediction(struct thorium_multiplexed_buffer *self, uint64_t time);

void thorium_multiplexed_buffer_init(struct thorium_multiplexed_buffer *self,
                int maximum_size, int timeout)
{
#ifdef THORIUM_MULTIPLEXED_BUFFER_PREDICT_MESSAGE_COUNT
    int i;
#endif

    self->configured_timeout = timeout;
    self->timeout_ = self->configured_timeout;

#ifdef THORIUM_MULTIPLEXED_BUFFER_PREDICT_MESSAGE_COUNT
    /*
     * Use a very big value so that the code works
     * with or without PREDICT_FUTURE.
     */
    self->predicted_message_count_ = 99999999;
#endif

    thorium_multiplexed_buffer_reset(self);

    self->target_message_count = 0;

    self->maximum_size_ = maximum_size;

#ifdef THORIUM_MULTIPLEXED_BUFFER_PREDICT_MESSAGE_COUNT
    self->prediction_iterator = 0;

    for (i = 0; i < PREDICTION_EVENT_COUNT; ++i) {
        self->prediction_ages[i] = -1;
        self->prediction_buffer_sizes[i] = -1;
        self->prediction_message_count[i] = -1;
    }
#endif

    self->profile_start = 0;
    self->profile_actor_message_count = 0;

    self->counter_original_message_count = 0;
    self->counter_real_message_count = 0;
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
                int count, void *buffer, uint64_t time)
{
    void *multiplexed_buffer;
    void *destination_in_buffer;
    int required_size;

    ++self->counter_original_message_count;

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

    thorium_multiplexed_buffer_profile(self, time);
}

int thorium_multiplexed_buffer_required_size(struct thorium_multiplexed_buffer *self,
                int count)
{
    return sizeof(count) + count;
}

void thorium_multiplexed_buffer_set_time(struct thorium_multiplexed_buffer *self,
                uint64_t time)
{
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

    /*
     * Assume that this generated a network message.
     */
    ++self->counter_real_message_count;
}

void thorium_multiplexed_buffer_set_buffer(struct thorium_multiplexed_buffer *self,
                void *buffer)
{
    self->buffer_ = buffer;
}

void thorium_multiplexed_buffer_profile_for_prediction(struct thorium_multiplexed_buffer *self, uint64_t time)
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

void thorium_multiplexed_buffer_profile(struct thorium_multiplexed_buffer *self,
                uint64_t time)
{
    uint64_t delta;
    int actor_message_period;
    int threshold;

    if (self->profile_start == NO_TIME)
        self->profile_start = time;

    ++self->profile_actor_message_count;

    /*
     * At the end of the period, evaluate the utilization profile,
     * and possibly turn off the multiplexer for the next period.
     */
    if (self->profile_actor_message_count == EVALUATION_PERIOD) {

        threshold = self->configured_timeout / 2;
        delta = time - self->profile_start;
        actor_message_period = delta / self->profile_actor_message_count;

#ifdef PRINT_TIMEOUT_UPDATE
        printf("thorium_multiplexed_buffer actor_message_period: %d ns profile_actor_message_count %d\n",
                        actor_message_period, self->profile_actor_message_count);
#endif

        /*
         * Reset the profile.
         */
        self->profile_start = time;
        self->profile_actor_message_count = 0;

#ifdef UPDATE_TIMEOUT_DYNAMICALLY
        /*
         * Turn off the multiplexer if the period is too high.
         */
        if (actor_message_period >= threshold) {
            self->timeout_ = 0;
        } else {
            self->timeout_ = MESSAGE_COUNT_PER_PARCEL * actor_message_period;

            if (self->timeout_ > self->configured_timeout)
                self->timeout_ = self->configured_timeout;
        }

#ifdef PRINT_TIMEOUT_UPDATE
        printf("thorium_multiplexed_buffer new timeout: %d ns\n",
                        self->timeout_);
#endif /* PRINT_TIMEOUT_UPDATE */

#endif
    }
}

int thorium_multiplexed_buffer_original_message_count(struct thorium_multiplexed_buffer *self)
{
    return self->counter_original_message_count;
}

int thorium_multiplexed_buffer_real_message_count(struct thorium_multiplexed_buffer *self)
{
    return self->counter_real_message_count;
}

int thorium_multiplexed_buffer_has_reached_target(struct thorium_multiplexed_buffer *self)
{
    int result;

    /*
     * The target starts at 0.
     * Then it changes dynamically.
     */
    result = self->message_count_ >= self->target_message_count;

    /*
     * We can do better !
     */
    if (result) {
        ++self->target_message_count;
    } else {

        /*
         * The target was too high.
         */
        --self->target_message_count;
    }

    return result;
}

double thorium_multiplexed_buffer_get_traffic_reduction(struct thorium_multiplexed_buffer *self)
{
    double value;

    if (self->message_count_ == 0)
        return 0;

    /*
     * y: traffic reduction
     * x: message count
     *
     * y = 1 - 1 / x
     *
     * y - 1 = - 1 / x
     *
     * 1 - y = 1 / x
     *
     * x = 1 / (1 - y)
     */
    value = 1.0 - 1.0 / self->message_count_;

    return value;
}
