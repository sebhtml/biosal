
#ifndef _BSAL_EVENT_COUNTER_H
#define _BSAL_EVENT_COUNTER_H

#include <stdint.h>

/* event counters */

#define BSAL_EVENT_COUNTER_RECEIVE_MESSAGE 0
#define BSAL_EVENT_COUNTER_RECEIVE_MESSAGE_FROM_SELF 1
#define BSAL_EVENT_COUNTER_RECEIVE_MESSAGE_NOT_FROM_SELF 2
#define BSAL_EVENT_COUNTER_SEND_MESSAGE 3
#define BSAL_EVENT_COUNTER_SEND_MESSAGE_TO_SELF 4
#define BSAL_EVENT_COUNTER_SEND_MESSAGE_NOT_TO_SELF 5
#define BSAL_EVENT_COUNTER_SPAWN_ACTOR 6
#define BSAL_EVENT_COUNTER_KILL_ACTOR 7

struct bsal_event_counter {
    uint64_t event_counter_send_message_to_self;
    uint64_t event_counter_send_message_not_to_self;
    uint64_t event_counter_receive_message_from_self;
    uint64_t event_counter_receive_message_not_from_self;
    uint64_t event_counter_spawn_actor;
    uint64_t event_counter_kill_actor;
};

void bsal_event_counter_init(struct bsal_event_counter *self);
void bsal_event_counter_destroy(struct bsal_event_counter *self);

uint64_t bsal_event_counter_get(struct bsal_event_counter *self, int counter);
void bsal_event_counter_increment(struct bsal_event_counter *self, int counter);
void bsal_event_counter_reset(struct bsal_event_counter *self);
void bsal_event_counter_print(struct bsal_event_counter *self);

#endif
