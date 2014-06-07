
#include "event_counter.h"

#include <stdio.h>

void bsal_event_counter_init(struct bsal_event_counter *self)
{
    bsal_event_counter_reset(self);
}

void bsal_event_counter_reset(struct bsal_event_counter *self)
{
    self->event_counter_receive_message_from_self = 0;
    self->event_counter_receive_message_not_from_self = 0;
    self->event_counter_send_message_to_self = 0;
    self->event_counter_send_message_not_to_self = 0;
    self->event_counter_spawn_actor = 0;
    self->event_counter_kill_actor = 0;
}

void bsal_event_counter_destroy(struct bsal_event_counter *self)
{
    bsal_event_counter_reset(self);
}

uint64_t bsal_event_counter_get(struct bsal_event_counter *self, int counter)
{
    if (counter == BSAL_EVENT_COUNTER_RECEIVE_MESSAGE) {
        return bsal_event_counter_get(self, BSAL_EVENT_COUNTER_RECEIVE_MESSAGE_FROM_SELF) +
                bsal_event_counter_get(self, BSAL_EVENT_COUNTER_RECEIVE_MESSAGE_NOT_FROM_SELF);

    } else if (counter == BSAL_EVENT_COUNTER_SEND_MESSAGE) {
        return bsal_event_counter_get(self, BSAL_EVENT_COUNTER_SEND_MESSAGE_TO_SELF) +
                bsal_event_counter_get(self, BSAL_EVENT_COUNTER_SEND_MESSAGE_NOT_TO_SELF);

    } else if (counter == BSAL_EVENT_COUNTER_RECEIVE_MESSAGE_FROM_SELF) {
        return self->event_counter_receive_message_from_self;

    } else if (counter == BSAL_EVENT_COUNTER_SEND_MESSAGE_TO_SELF) {
        return self->event_counter_send_message_to_self;

    } else if (counter == BSAL_EVENT_COUNTER_RECEIVE_MESSAGE_NOT_FROM_SELF) {
        return self->event_counter_receive_message_not_from_self;

    } else if (counter == BSAL_EVENT_COUNTER_SEND_MESSAGE_NOT_TO_SELF) {
        return self->event_counter_send_message_not_to_self;

    } else if (counter == BSAL_EVENT_COUNTER_SPAWN_ACTOR) {
        return self->event_counter_spawn_actor;

    } else if (counter == BSAL_EVENT_COUNTER_KILL_ACTOR) {
        return self->event_counter_kill_actor;
    }

    return 0;
}

void bsal_event_counter_increment(struct bsal_event_counter *self, int counter)
{
    if (counter == BSAL_EVENT_COUNTER_RECEIVE_MESSAGE) {

    } else if (counter == BSAL_EVENT_COUNTER_SEND_MESSAGE) {

    } else if (counter == BSAL_EVENT_COUNTER_RECEIVE_MESSAGE_FROM_SELF) {
        self->event_counter_receive_message_from_self++;

    } else if (counter == BSAL_EVENT_COUNTER_SEND_MESSAGE_TO_SELF) {
        self->event_counter_send_message_to_self++;

    } else if (counter == BSAL_EVENT_COUNTER_RECEIVE_MESSAGE_NOT_FROM_SELF) {
        self->event_counter_receive_message_not_from_self++;

    } else if (counter == BSAL_EVENT_COUNTER_SEND_MESSAGE_NOT_TO_SELF) {
        self->event_counter_send_message_not_to_self++;

    } else if (counter == BSAL_EVENT_COUNTER_SPAWN_ACTOR) {
        self->event_counter_spawn_actor++;

    } else if (counter == BSAL_EVENT_COUNTER_KILL_ACTOR) {
        self->event_counter_kill_actor++;
    }
}

void bsal_event_counter_print(struct bsal_event_counter *self)
{
    /* TODO print uint64_t, not int
     */
    printf("BSAL_EVENT_COUNTER_SPAWN_ACTOR %d\n",
                    (int)bsal_event_counter_get(self, BSAL_EVENT_COUNTER_SPAWN_ACTOR));
    printf("BSAL_EVENT_COUNTER_KILL_ACTOR %d\n",
                    (int)bsal_event_counter_get(self, BSAL_EVENT_COUNTER_KILL_ACTOR));

    printf("BSAL_EVENT_COUNTER_SEND_MESSAGE %d\n",
                    (int)bsal_event_counter_get(self, BSAL_EVENT_COUNTER_SEND_MESSAGE));
    printf("BSAL_EVENT_COUNTER_SEND_MESSAGE_TO_SELF %d\n",
                    (int)bsal_event_counter_get(self, BSAL_EVENT_COUNTER_SEND_MESSAGE_TO_SELF));
    printf("BSAL_EVENT_COUNTER_SEND_MESSAGE_NOT_TO_SELF %d\n",
                    (int)bsal_event_counter_get(self, BSAL_EVENT_COUNTER_SEND_MESSAGE_NOT_TO_SELF));

    printf("BSAL_EVENT_COUNTER_RECEIVE_MESSAGE %d\n",
                    (int)bsal_event_counter_get(self, BSAL_EVENT_COUNTER_RECEIVE_MESSAGE));
    printf("BSAL_EVENT_COUNTER_RECEIVE_MESSAGE_FROM_SELF %d\n",
                    (int)bsal_event_counter_get(self, BSAL_EVENT_COUNTER_RECEIVE_MESSAGE_FROM_SELF));
    printf("BSAL_EVENT_COUNTER_RECEIVE_MESSAGE_NOT_FROM_SELF %d\n",
                    (int)bsal_event_counter_get(self, BSAL_EVENT_COUNTER_RECEIVE_MESSAGE_NOT_FROM_SELF));
}


