
#ifndef CORE_COUNTER_H
#define CORE_COUNTER_H

#include <stdint.h>

/* counters
 */
#define CORE_COUNTER_SPAWNED_ACTORS 0
#define CORE_COUNTER_KILLED_ACTORS 1

#define CORE_COUNTER_RECEIVED_MESSAGES_FROM_SELF 2
#define CORE_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF 3

#define CORE_COUNTER_SENT_MESSAGES_TO_SELF 4
#define CORE_COUNTER_SENT_MESSAGES_NOT_TO_SELF 5

#define CORE_COUNTER_RECEIVED_BYTES_FROM_SELF 6
#define CORE_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF 7

#define CORE_COUNTER_SENT_BYTES_TO_SELF 8
#define CORE_COUNTER_SENT_BYTES_NOT_TO_SELF 9

#define CORE_COUNTER_MAXIMUM 10

/* meta counters
 */
#define CORE_COUNTER_RECEIVED_MESSAGES 100
#define CORE_COUNTER_SENT_MESSAGES 101
#define CORE_COUNTER_RECEIVED_BYTES 102
#define CORE_COUNTER_SENT_BYTES 103

#define CORE_COUNTER_BALANCE_MESSAGES 104
#define CORE_COUNTER_BALANCE_BYTES 105
#define CORE_COUNTER_BALANCE_ACTORS 200

struct core_counter {
    int64_t counters[CORE_COUNTER_MAXIMUM];
};

void core_counter_init(struct core_counter *self);
void core_counter_destroy(struct core_counter *self);

int64_t core_counter_get(struct core_counter *self, int counter);
void core_counter_increment(struct core_counter *self, int counter);
void core_counter_add(struct core_counter *self, int counter, int quantity);
void core_counter_reset(struct core_counter *self);
void core_counter_print(struct core_counter *self, int name);
int64_t core_counter_difference(struct core_counter *self, int counter1, int counter2);
int64_t core_counter_sum(struct core_counter *self, int counter1, int counter2);

#endif
