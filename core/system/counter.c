
#include "counter.h"

#include <stdio.h>
#include <inttypes.h>

void biosal_counter_init(struct biosal_counter *self)
{
    biosal_counter_reset(self);
}

void biosal_counter_reset(struct biosal_counter *self)
{
    int i;

    for (i = 0; i < BIOSAL_COUNTER_MAXIMUM; i++) {
        self->counters[i] = 0;
    }
}

void biosal_counter_destroy(struct biosal_counter *self)
{
    biosal_counter_reset(self);
}

int64_t biosal_counter_get(struct biosal_counter *self, int counter)
{
    if (counter == BIOSAL_COUNTER_RECEIVED_MESSAGES) {
        return biosal_counter_sum(self, BIOSAL_COUNTER_RECEIVED_MESSAGES_FROM_SELF,
                        BIOSAL_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF);

    } else if (counter == BIOSAL_COUNTER_SENT_MESSAGES) {

        return biosal_counter_sum(self, BIOSAL_COUNTER_SENT_MESSAGES_TO_SELF,
                        BIOSAL_COUNTER_SENT_MESSAGES_NOT_TO_SELF);

    } else if (counter == BIOSAL_COUNTER_RECEIVED_BYTES) {
        return biosal_counter_sum(self, BIOSAL_COUNTER_RECEIVED_BYTES_FROM_SELF,
                        BIOSAL_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF);

    } else if (counter == BIOSAL_COUNTER_SENT_BYTES) {

        return biosal_counter_sum(self, BIOSAL_COUNTER_SENT_BYTES_TO_SELF,
                        BIOSAL_COUNTER_SENT_BYTES_NOT_TO_SELF);

    } else if (counter == BIOSAL_COUNTER_BALANCE_MESSAGES) {

        return biosal_counter_difference(self, BIOSAL_COUNTER_RECEIVED_MESSAGES,
                        BIOSAL_COUNTER_SENT_MESSAGES);

    } else if (counter == BIOSAL_COUNTER_BALANCE_BYTES) {

        return biosal_counter_difference(self, BIOSAL_COUNTER_RECEIVED_BYTES,
                        BIOSAL_COUNTER_SENT_BYTES);
    }

    if (counter >= BIOSAL_COUNTER_MAXIMUM) {
        return 0;
    }

    return self->counters[counter];
}

void biosal_counter_increment(struct biosal_counter *self, int counter)
{
    biosal_counter_add(self, counter, 1);
}

/*
http://pubs.opengroup.org/onlinepubs/009696799/basedefs/inttypes.h.html
http://www.cplusplus.com/reference/cinttypes/
http://en.wikibooks.org/wiki/C_Programming/C_Reference/inttypes.h
http://stackoverflow.com/questions/16859500/mmh-who-are-you-priu64

#include <inttypes.h>

use PRIu64
*/

#define BIOSAL_DISPLAY_COUNTER(counter, spacing, name) \
    printf("%i %s" #counter " %" PRId64 "\n", name, spacing, \
                    biosal_counter_get(self, counter));

void biosal_counter_print(struct biosal_counter *self, int name)
{
    /* print int64_t, not int
     */
    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_SPAWNED_ACTORS, "", name);
    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_KILLED_ACTORS, "", name);
    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_BALANCE_ACTORS, " balance ", name);

    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_RECEIVED_MESSAGES, "", name);
    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_RECEIVED_MESSAGES_FROM_SELF, "  ", name);
    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF, "  ", name);

    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_SENT_MESSAGES, "", name);
    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_SENT_MESSAGES_TO_SELF, "  ", name);
    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_SENT_MESSAGES_NOT_TO_SELF, "  ", name);

    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_BALANCE_MESSAGES, " balance ", name);

    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_RECEIVED_BYTES, "", name);
    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_RECEIVED_BYTES_FROM_SELF, "  ", name);
    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF, "  ", name);

    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_SENT_BYTES, "", name);
    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_SENT_BYTES_TO_SELF, "  ", name);
    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_SENT_BYTES_NOT_TO_SELF, " ", name);

    BIOSAL_DISPLAY_COUNTER(BIOSAL_COUNTER_BALANCE_BYTES, " balance ", name);
}

void biosal_counter_add(struct biosal_counter *self, int counter, int quantity)
{
    if (counter < BIOSAL_COUNTER_MAXIMUM) {
        self->counters[counter] += quantity;
    }
}

int64_t biosal_counter_sum(struct biosal_counter *self, int counter1, int counter2)
{
    return biosal_counter_get(self, counter1) + biosal_counter_get(self, counter2);
}

int64_t biosal_counter_difference(struct biosal_counter *self, int counter1, int counter2)
{
    return biosal_counter_get(self, counter1) - biosal_counter_get(self, counter2);
}
