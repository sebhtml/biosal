
#include "time_in_seconds.h"

#include <time.h>

uint64_t thorium_actor_get_time_in_seconds(struct thorium_actor *self)
{
    uint64_t time_in_seconds;

    time_in_seconds = time(NULL);

    return time_in_seconds;
}
