
#ifndef BSAL_COUNTER_H
#define BSAL_COUNTER_H

#include <stdint.h>

/* counters
 */
#define BSAL_COUNTER_SPAWNED_ACTORS 0
#define BSAL_COUNTER_KILLED_ACTORS 1

#define BSAL_COUNTER_RECEIVED_MESSAGES_FROM_SELF 2
#define BSAL_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF 3

#define BSAL_COUNTER_SENT_MESSAGES_TO_SELF 4
#define BSAL_COUNTER_SENT_MESSAGES_NOT_TO_SELF 5

#define BSAL_COUNTER_RECEIVED_BYTES_FROM_SELF 6
#define BSAL_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF 7

#define BSAL_COUNTER_SENT_BYTES_TO_SELF 8
#define BSAL_COUNTER_SENT_BYTES_NOT_TO_SELF 9

#define BSAL_COUNTER_MAXIMUM 10

/* meta counters
 */
#define BSAL_COUNTER_RECEIVED_MESSAGES 100
#define BSAL_COUNTER_SENT_MESSAGES 101
#define BSAL_COUNTER_RECEIVED_BYTES 102
#define BSAL_COUNTER_SENT_BYTES 103

#define BSAL_COUNTER_BALANCE_MESSAGES 104
#define BSAL_COUNTER_BALANCE_BYTES 105
#define BSAL_COUNTER_BALANCE_ACTORS 200

struct bsal_counter {
    int64_t counters[BSAL_COUNTER_MAXIMUM];
};

void bsal_counter_init(struct bsal_counter *self);
void bsal_counter_destroy(struct bsal_counter *self);

int64_t bsal_counter_get(struct bsal_counter *self, int counter);
void bsal_counter_increment(struct bsal_counter *self, int counter);
void bsal_counter_add(struct bsal_counter *self, int counter, int quantity);
void bsal_counter_reset(struct bsal_counter *self);
void bsal_counter_print(struct bsal_counter *self);
int64_t bsal_counter_difference(struct bsal_counter *self, int counter1, int counter2);
int64_t bsal_counter_sum(struct bsal_counter *self, int counter1, int counter2);

#endif
