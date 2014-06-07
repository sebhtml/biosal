
#include "counter.h"

#include <stdio.h>

void bsal_counter_init(struct bsal_counter *self)
{
    bsal_counter_reset(self);
}

void bsal_counter_reset(struct bsal_counter *self)
{
    self->count_event_receive_message_from_self = 0;
    self->count_event_receive_message_not_from_self = 0;
    self->count_event_send_message_to_self = 0;
    self->count_event_send_message_not_to_self = 0;
    self->count_event_spawn_actor = 0;
    self->count_event_kill_actor = 0;
}

void bsal_counter_destroy(struct bsal_counter *self)
{
    bsal_counter_reset(self);
}

uint64_t bsal_counter_get(struct bsal_counter *self, int counter)
{
    if (counter == BSAL_COUNTER_EVENT_RECEIVE_MESSAGE) {
        return bsal_counter_get(self, BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_FROM_SELF) +
                bsal_counter_get(self, BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_NOT_FROM_SELF);

    } else if (counter == BSAL_COUNTER_EVENT_SEND_MESSAGE) {
        return bsal_counter_get(self, BSAL_COUNTER_EVENT_SEND_MESSAGE_TO_SELF) +
                bsal_counter_get(self, BSAL_COUNTER_EVENT_SEND_MESSAGE_NOT_TO_SELF);

    } else if (counter == BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_FROM_SELF) {
        return self->count_event_receive_message_from_self;

    } else if (counter == BSAL_COUNTER_EVENT_SEND_MESSAGE_TO_SELF) {
        return self->count_event_send_message_to_self;

    } else if (counter == BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_NOT_FROM_SELF) {
        return self->count_event_receive_message_not_from_self;

    } else if (counter == BSAL_COUNTER_EVENT_SEND_MESSAGE_NOT_TO_SELF) {
        return self->count_event_send_message_not_to_self;

    } else if (counter == BSAL_COUNTER_EVENT_SPAWN_ACTOR) {
        return self->count_event_spawn_actor;

    } else if (counter == BSAL_COUNTER_EVENT_KILL_ACTOR) {
        return self->count_event_kill_actor;
    }

    return 0;
}

void bsal_counter_increment(struct bsal_counter *self, int counter)
{
    if (counter == BSAL_COUNTER_EVENT_RECEIVE_MESSAGE) {

    } else if (counter == BSAL_COUNTER_EVENT_SEND_MESSAGE) {

    } else if (counter == BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_FROM_SELF) {
        self->count_event_receive_message_from_self++;

    } else if (counter == BSAL_COUNTER_EVENT_SEND_MESSAGE_TO_SELF) {
        self->count_event_send_message_to_self++;

    } else if (counter == BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_NOT_FROM_SELF) {
        self->count_event_receive_message_not_from_self++;

    } else if (counter == BSAL_COUNTER_EVENT_SEND_MESSAGE_NOT_TO_SELF) {
        self->count_event_send_message_not_to_self++;

    } else if (counter == BSAL_COUNTER_EVENT_SPAWN_ACTOR) {
        self->count_event_spawn_actor++;

    } else if (counter == BSAL_COUNTER_EVENT_KILL_ACTOR) {
        self->count_event_kill_actor++;
    }
}

void bsal_counter_print(struct bsal_counter *self)
{
    /* print uint64_t, not int
     */
    printf("BSAL_COUNTER_EVENT_SPAWN_ACTOR %d\n",
                    (int)bsal_counter_get(self, BSAL_COUNTER_EVENT_SPAWN_ACTOR));
    printf("BSAL_COUNTER_EVENT_KILL_ACTOR %d\n",
                    (int)bsal_counter_get(self, BSAL_COUNTER_EVENT_KILL_ACTOR));

    printf("BSAL_COUNTER_EVENT_SEND_MESSAGE %d\n",
                    (int)bsal_counter_get(self, BSAL_COUNTER_EVENT_SEND_MESSAGE));
    printf("BSAL_COUNTER_EVENT_SEND_MESSAGE_TO_SELF %d\n",
                    (int)bsal_counter_get(self, BSAL_COUNTER_EVENT_SEND_MESSAGE_TO_SELF));
    printf("BSAL_COUNTER_EVENT_SEND_MESSAGE_NOT_TO_SELF %d\n",
                    (int)bsal_counter_get(self, BSAL_COUNTER_EVENT_SEND_MESSAGE_NOT_TO_SELF));

    printf("BSAL_COUNTER_EVENT_RECEIVE_MESSAGE %d\n",
                    (int)bsal_counter_get(self, BSAL_COUNTER_EVENT_RECEIVE_MESSAGE));
    printf("BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_FROM_SELF %d\n",
                    (int)bsal_counter_get(self, BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_FROM_SELF));
    printf("BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_NOT_FROM_SELF %d\n",
                    (int)bsal_counter_get(self, BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_NOT_FROM_SELF));
}


