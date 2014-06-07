
#include "counter.h"

#include <stdio.h>
#include <inttypes.h>

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

/*
http://pubs.opengroup.org/onlinepubs/009696799/basedefs/inttypes.h.html
http://www.cplusplus.com/reference/cinttypes/
http://en.wikibooks.org/wiki/C_Programming/C_Reference/inttypes.h
http://stackoverflow.com/questions/16859500/mmh-who-are-you-priu64

#include <inttypes.h>

use PRIu64
*/
void bsal_counter_print(struct bsal_counter *self)
{
    /* print uint64_t, not int
     */
    printf("BSAL_COUNTER_EVENT_SPAWN_ACTOR %" PRIu64 "\n",
                    bsal_counter_get(self, BSAL_COUNTER_EVENT_SPAWN_ACTOR));
    printf("BSAL_COUNTER_EVENT_KILL_ACTOR %" PRIu64 "\n",
                    bsal_counter_get(self, BSAL_COUNTER_EVENT_KILL_ACTOR));

    printf("BSAL_COUNTER_EVENT_SEND_MESSAGE %" PRIu64 "\n",
                    bsal_counter_get(self, BSAL_COUNTER_EVENT_SEND_MESSAGE));
    printf("BSAL_COUNTER_EVENT_SEND_MESSAGE_TO_SELF %" PRIu64 "\n",
                    bsal_counter_get(self, BSAL_COUNTER_EVENT_SEND_MESSAGE_TO_SELF));
    printf("BSAL_COUNTER_EVENT_SEND_MESSAGE_NOT_TO_SELF %" PRIu64 "\n",
                    bsal_counter_get(self, BSAL_COUNTER_EVENT_SEND_MESSAGE_NOT_TO_SELF));

    printf("BSAL_COUNTER_EVENT_RECEIVE_MESSAGE %" PRIu64 "\n",
                    bsal_counter_get(self, BSAL_COUNTER_EVENT_RECEIVE_MESSAGE));
    printf("BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_FROM_SELF %" PRIu64 "\n",
                    bsal_counter_get(self, BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_FROM_SELF));
    printf("BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_NOT_FROM_SELF %" PRIu64 "\n",
                    bsal_counter_get(self, BSAL_COUNTER_EVENT_RECEIVE_MESSAGE_NOT_FROM_SELF));
}


