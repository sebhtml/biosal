
#include "counter.h"

#include <stdio.h>
#include <inttypes.h>

void bsal_counter_init(struct bsal_counter *self)
{
    bsal_counter_reset(self);
}

void bsal_counter_reset(struct bsal_counter *self)
{
    int i;

    for (i = 0; i < BSAL_COUNTER_MAXIMUM; i++) {
        self->counters[i] = 0;
    }
}

void bsal_counter_destroy(struct bsal_counter *self)
{
    bsal_counter_reset(self);
}

int64_t bsal_counter_get(struct bsal_counter *self, int counter)
{
    if (counter == BSAL_COUNTER_RECEIVED_MESSAGES) {
        return bsal_counter_sum(self, BSAL_COUNTER_RECEIVED_MESSAGES_FROM_SELF,
                        BSAL_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF);

    } else if (counter == BSAL_COUNTER_SENT_MESSAGES) {

        return bsal_counter_sum(self, BSAL_COUNTER_SENT_MESSAGES_TO_SELF,
                        BSAL_COUNTER_SENT_MESSAGES_NOT_TO_SELF);

    } else if (counter == BSAL_COUNTER_RECEIVED_BYTES) {
        return bsal_counter_sum(self, BSAL_COUNTER_RECEIVED_BYTES_FROM_SELF,
                        BSAL_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF);

    } else if (counter == BSAL_COUNTER_SENT_BYTES) {

        return bsal_counter_sum(self, BSAL_COUNTER_SENT_BYTES_TO_SELF,
                        BSAL_COUNTER_SENT_BYTES_NOT_TO_SELF);

    } else if (counter == BSAL_COUNTER_BALANCE_MESSAGES) {

        return bsal_counter_difference(self, BSAL_COUNTER_RECEIVED_MESSAGES,
                        BSAL_COUNTER_SENT_MESSAGES);

    } else if (counter == BSAL_COUNTER_BALANCE_BYTES) {

        return bsal_counter_difference(self, BSAL_COUNTER_RECEIVED_BYTES,
                        BSAL_COUNTER_SENT_BYTES);
    }

    if (counter >= BSAL_COUNTER_MAXIMUM) {
        return 0;
    }

    return self->counters[counter];
}

void bsal_counter_increment(struct bsal_counter *self, int counter)
{
    bsal_counter_add(self, counter, 1);
}

/*
http://pubs.opengroup.org/onlinepubs/009696799/basedefs/inttypes.h.html
http://www.cplusplus.com/reference/cinttypes/
http://en.wikibooks.org/wiki/C_Programming/C_Reference/inttypes.h
http://stackoverflow.com/questions/16859500/mmh-who-are-you-priu64

#include <inttypes.h>

use PRIu64
*/

#define BSAL_DISPLAY_COUNTER(counter, spacing) \
    printf("%s" #counter " %" PRId64 "\n", spacing, \
                    bsal_counter_get(self, counter));

void bsal_counter_print(struct bsal_counter *self)
{
    /* print int64_t, not int
     */
    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_SPAWNED_ACTORS, "");
    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_KILLED_ACTORS, "");
    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_BALANCE_ACTORS, " balance ");

    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_RECEIVED_MESSAGES, "");
    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_RECEIVED_MESSAGES_FROM_SELF, "  ");
    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF, "  ");

    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_SENT_MESSAGES, "");
    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_SENT_MESSAGES_TO_SELF, "  ");
    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_SENT_MESSAGES_NOT_TO_SELF, "  ");

    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_BALANCE_MESSAGES, " balance ");

    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_RECEIVED_BYTES, "");
    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_RECEIVED_BYTES_FROM_SELF, "  ");
    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF, "  ");

    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_SENT_BYTES, "");
    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_SENT_BYTES_TO_SELF, "  ");
    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_SENT_BYTES_NOT_TO_SELF, " ");

    BSAL_DISPLAY_COUNTER(BSAL_COUNTER_BALANCE_BYTES, " balance ");
}

void bsal_counter_add(struct bsal_counter *self, int counter, int quantity)
{
    if (counter < BSAL_COUNTER_MAXIMUM) {
        self->counters[counter] += quantity;
    }
}

int64_t bsal_counter_sum(struct bsal_counter *self, int counter1, int counter2)
{
    return bsal_counter_get(self, counter1) + bsal_counter_get(self, counter2);
}

int64_t bsal_counter_difference(struct bsal_counter *self, int counter1, int counter2)
{
    return bsal_counter_get(self, counter1) - bsal_counter_get(self, counter2);
}
