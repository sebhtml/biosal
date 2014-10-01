
#ifndef BIOSAL_COUNTER_H
#define BIOSAL_COUNTER_H

#include <stdint.h>

/* counters
 */
#define BIOSAL_COUNTER_SPAWNED_ACTORS 0
#define BIOSAL_COUNTER_KILLED_ACTORS 1

#define BIOSAL_COUNTER_RECEIVED_MESSAGES_FROM_SELF 2
#define BIOSAL_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF 3

#define BIOSAL_COUNTER_SENT_MESSAGES_TO_SELF 4
#define BIOSAL_COUNTER_SENT_MESSAGES_NOT_TO_SELF 5

#define BIOSAL_COUNTER_RECEIVED_BYTES_FROM_SELF 6
#define BIOSAL_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF 7

#define BIOSAL_COUNTER_SENT_BYTES_TO_SELF 8
#define BIOSAL_COUNTER_SENT_BYTES_NOT_TO_SELF 9

#define BIOSAL_COUNTER_MAXIMUM 10

/* meta counters
 */
#define BIOSAL_COUNTER_RECEIVED_MESSAGES 100
#define BIOSAL_COUNTER_SENT_MESSAGES 101
#define BIOSAL_COUNTER_RECEIVED_BYTES 102
#define BIOSAL_COUNTER_SENT_BYTES 103

#define BIOSAL_COUNTER_BALANCE_MESSAGES 104
#define BIOSAL_COUNTER_BALANCE_BYTES 105
#define BIOSAL_COUNTER_BALANCE_ACTORS 200

struct biosal_counter {
    int64_t counters[BIOSAL_COUNTER_MAXIMUM];
};

void biosal_counter_init(struct biosal_counter *self);
void biosal_counter_destroy(struct biosal_counter *self);

int64_t biosal_counter_get(struct biosal_counter *self, int counter);
void biosal_counter_increment(struct biosal_counter *self, int counter);
void biosal_counter_add(struct biosal_counter *self, int counter, int quantity);
void biosal_counter_reset(struct biosal_counter *self);
void biosal_counter_print(struct biosal_counter *self, int name);
int64_t biosal_counter_difference(struct biosal_counter *self, int counter1, int counter2);
int64_t biosal_counter_sum(struct biosal_counter *self, int counter1, int counter2);

#endif
