
#ifndef _BSAL_COUNTER_H
#define _BSAL_COUNTER_H

#include <stdint.h>

/* counters */

#define BSAL_COUNTER_EVENT_RECEIVE_MESSAGE 0
#define BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_FROM_SELF 1
#define BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_NOT_FROM_SELF 2

#define BSAL_COUNTER_EVENT_SEND_MESSAGE 3
#define BSAL_COUNTER_EVENT_SEND_MESSAGE_TO_SELF 4
#define BSAL_COUNTER_EVENT_SEND_MESSAGE_NOT_TO_SELF 5

#define BSAL_COUNTER_EVENT_SPAWN_ACTOR 6
#define BSAL_COUNTER_EVENT_KILL_ACTOR 7

struct bsal_counter {
    uint64_t count_event_send_message_to_self;
    uint64_t count_event_send_message_not_to_self;
    uint64_t count_event_receive_message_from_self;
    uint64_t count_event_receive_message_not_from_self;
    uint64_t count_event_spawn_actor;
    uint64_t count_event_kill_actor;
};

void bsal_counter_init(struct bsal_counter *self);
void bsal_counter_destroy(struct bsal_counter *self);

uint64_t bsal_counter_get(struct bsal_counter *self, int counter);
void bsal_counter_increment(struct bsal_counter *self, int counter);
void bsal_counter_reset(struct bsal_counter *self);
void bsal_counter_print(struct bsal_counter *self);

#endif
