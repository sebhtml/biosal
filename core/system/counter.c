
#include "counter.h"

#include <stdio.h>
#include <inttypes.h>

int64_t core_counter_difference(struct core_counter *self, int counter1, int counter2);
int64_t core_counter_sum(struct core_counter *self, int counter1, int counter2);

void core_counter_init(struct core_counter *self)
{
    core_counter_reset(self);
}

void core_counter_reset(struct core_counter *self)
{
    int i;

    for (i = 0; i < CORE_COUNTER_MAXIMUM; i++) {
        self->counters[i] = 0;
    }
}

void core_counter_destroy(struct core_counter *self)
{
    core_counter_reset(self);
}

int64_t core_counter_get(struct core_counter *self, int counter)
{
    if (counter == CORE_COUNTER_RECEIVED_MESSAGES) {
        return core_counter_sum(self, CORE_COUNTER_RECEIVED_MESSAGES_FROM_SELF,
                        CORE_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF);

    } else if (counter == CORE_COUNTER_SENT_MESSAGES) {

        return core_counter_sum(self, CORE_COUNTER_SENT_MESSAGES_TO_SELF,
                        CORE_COUNTER_SENT_MESSAGES_NOT_TO_SELF);

    } else if (counter == CORE_COUNTER_RECEIVED_BYTES) {
        return core_counter_sum(self, CORE_COUNTER_RECEIVED_BYTES_FROM_SELF,
                        CORE_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF);

    } else if (counter == CORE_COUNTER_SENT_BYTES) {

        return core_counter_sum(self, CORE_COUNTER_SENT_BYTES_TO_SELF,
                        CORE_COUNTER_SENT_BYTES_NOT_TO_SELF);

    } else if (counter == CORE_COUNTER_BALANCE_MESSAGES) {

        return core_counter_difference(self, CORE_COUNTER_RECEIVED_MESSAGES,
                        CORE_COUNTER_SENT_MESSAGES);

    } else if (counter == CORE_COUNTER_BALANCE_BYTES) {

        return core_counter_difference(self, CORE_COUNTER_RECEIVED_BYTES,
                        CORE_COUNTER_SENT_BYTES);
    }

    if (counter >= CORE_COUNTER_MAXIMUM) {
        return 0;
    }

    return self->counters[counter];
}

void core_counter_increment(struct core_counter *self, int counter)
{
    core_counter_add(self, counter, 1);
}

/*
http://pubs.opengroup.org/onlinepubs/009696799/basedefs/inttypes.h.html
http://www.cplusplus.com/reference/cinttypes/
http://en.wikibooks.org/wiki/C_Programming/C_Reference/inttypes.h
http://stackoverflow.com/questions/16859500/mmh-who-are-you-priu64

#include <inttypes.h>

use PRIu64
*/

#define CORE_DISPLAY_COUNTER(counter, spacing, name) \
    printf("%i %s" #counter " %" PRId64 "\n", name, spacing, \
                    core_counter_get(self, counter));

void core_counter_print(struct core_counter *self, int name)
{
    /* print int64_t, not int
     */
    CORE_DISPLAY_COUNTER(CORE_COUNTER_SPAWNED_ACTORS, "", name);
    CORE_DISPLAY_COUNTER(CORE_COUNTER_KILLED_ACTORS, "", name);
    CORE_DISPLAY_COUNTER(CORE_COUNTER_BALANCE_ACTORS, " balance ", name);

    CORE_DISPLAY_COUNTER(CORE_COUNTER_RECEIVED_MESSAGES, "", name);
    CORE_DISPLAY_COUNTER(CORE_COUNTER_RECEIVED_MESSAGES_FROM_SELF, "  ", name);
    CORE_DISPLAY_COUNTER(CORE_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF, "  ", name);

    CORE_DISPLAY_COUNTER(CORE_COUNTER_SENT_MESSAGES, "", name);
    CORE_DISPLAY_COUNTER(CORE_COUNTER_SENT_MESSAGES_TO_SELF, "  ", name);
    CORE_DISPLAY_COUNTER(CORE_COUNTER_SENT_MESSAGES_NOT_TO_SELF, "  ", name);

    CORE_DISPLAY_COUNTER(CORE_COUNTER_BALANCE_MESSAGES, " balance ", name);

    CORE_DISPLAY_COUNTER(CORE_COUNTER_RECEIVED_BYTES, "", name);
    CORE_DISPLAY_COUNTER(CORE_COUNTER_RECEIVED_BYTES_FROM_SELF, "  ", name);
    CORE_DISPLAY_COUNTER(CORE_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF, "  ", name);

    CORE_DISPLAY_COUNTER(CORE_COUNTER_SENT_BYTES, "", name);
    CORE_DISPLAY_COUNTER(CORE_COUNTER_SENT_BYTES_TO_SELF, "  ", name);
    CORE_DISPLAY_COUNTER(CORE_COUNTER_SENT_BYTES_NOT_TO_SELF, " ", name);

    CORE_DISPLAY_COUNTER(CORE_COUNTER_BALANCE_BYTES, " balance ", name);
}

void core_counter_add(struct core_counter *self, int counter, int quantity)
{
    if (counter < CORE_COUNTER_MAXIMUM) {
        self->counters[counter] += quantity;
    }
}

int64_t core_counter_sum(struct core_counter *self, int counter1, int counter2)
{
    return core_counter_get(self, counter1) + core_counter_get(self, counter2);
}

int64_t core_counter_difference(struct core_counter *self, int counter1, int counter2)
{
    return core_counter_get(self, counter1) - core_counter_get(self, counter2);
}
