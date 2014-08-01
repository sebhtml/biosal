
#include "priority_scheduler.h"

#include "scheduling_queue.h"

#include <engine/thorium/actor.h>

#include <core/structures/map_iterator.h>

#include <core/system/debugger.h>

#include <stdio.h>

#define CALL_COUNT_THRESHOLD 1024

void bsal_priority_scheduler_init(struct bsal_priority_scheduler *scheduler)
{
    bsal_map_init(&scheduler->actor_sources, sizeof(int), sizeof(int));
    bsal_map_init(&scheduler->actor_source_frequencies, sizeof(int), sizeof(int));

    scheduler->low_priority_maximum_value = -1;
    scheduler->high_priority_minimum_value = -1;
    scheduler->max_priority_minimum_value = -1;

    scheduler->calls = 0;
    scheduler->changes = 0;
}

void bsal_priority_scheduler_destroy(struct bsal_priority_scheduler *scheduler)
{

    bsal_map_destroy(&scheduler->actor_sources);
    bsal_map_destroy(&scheduler->actor_source_frequencies);

    scheduler->low_priority_maximum_value = -1;
    scheduler->high_priority_minimum_value = -1;
    scheduler->max_priority_minimum_value = -1;

    scheduler->calls = 0;
    scheduler->changes = 0;

}

/*
 * Gather data for sources.
 * Detect hubs
 * Assign BSAL_PRIORITY_MAX to >=P95
 * Assign BSAL_PRIORITY_HIGH to >=P70
 * Assign BSAL_PRIORITY_LOW to <= P30
 * Assign BSAL_PRIORITY_NORMAL otherwise.
 */
void bsal_priority_scheduler_update(struct bsal_priority_scheduler *scheduler, struct bsal_actor *actor)
{
    int old_priority;
    int new_priority;
    int old_source_count;
    int new_source_count;

    int name;

    BSAL_DEBUGGER_ASSERT(actor != NULL);

    name = bsal_actor_get_name(actor);
    new_source_count = bsal_actor_get_source_count(actor);

    /*
     * If this actor is already registered, check if its source count changed.
     */
    if (bsal_map_get_value(&scheduler->actor_sources, &name, &old_source_count)) {

        /* Only update the value if it changed
         */
        if (new_source_count != old_source_count) {

            /* Update the actor source count
             */
            bsal_map_update_value(&scheduler->actor_sources, &name, &new_source_count);

            bsal_priority_scheduler_decrement(scheduler, old_source_count);
            bsal_priority_scheduler_increment(scheduler, new_source_count);

            ++scheduler->changes;
        }

    } else {

        /* It is the first time that this actor is encountered
         */
        bsal_map_add_value(&scheduler->actor_sources, &name, &new_source_count);

        bsal_priority_scheduler_increment(scheduler, new_source_count);

        ++scheduler->changes;
    }

    /* Update the priority now.
     */

    if (scheduler->max_priority_minimum_value != -1) {

        old_priority = bsal_actor_get_priority(actor);

        new_priority = BSAL_PRIORITY_NORMAL;

        if (new_source_count >= scheduler->max_priority_minimum_value) {
            new_priority = BSAL_PRIORITY_MAX;

        } else if (new_source_count >= scheduler->high_priority_minimum_value) {
            new_priority = BSAL_PRIORITY_HIGH;

        } else if (new_source_count <= scheduler->low_priority_maximum_value) {
            new_priority = BSAL_PRIORITY_LOW;

        }

        if (new_priority != old_priority) {
            bsal_actor_set_priority(actor, new_priority);
        }
    }

    /*
     * Increment the number of calls and update threshold if necessary
     */

    ++scheduler->calls;

    if (scheduler->calls >= CALL_COUNT_THRESHOLD) {

        if (scheduler->changes > 0) {
            bsal_priority_scheduler_update_thresholds(scheduler);
        }

        scheduler->calls = 0;
        scheduler->changes = 0;
    }
}

void bsal_priority_scheduler_update_thresholds(struct bsal_priority_scheduler *scheduler)
{
    struct bsal_map_iterator iterator;
    int value;
    int frequency;

#ifdef BSAL_PRIORITY_SCHEDULER_DEBUG
    printf("DEBUG bsal_priority_scheduler_update_thresholds: calls %d changes %d\n",
                    scheduler->calls, scheduler->changes);
#endif

    bsal_map_iterator_init(&iterator, &scheduler->actor_source_frequencies);

    while (bsal_map_iterator_get_next_key_and_value(&iterator, &value, &frequency)) {

#if 0
        if (frequency > 0) {
            printf("VALUE %d Frequency %d\n", value, frequency);
        }
#endif
    }

    bsal_map_iterator_destroy(&iterator);

    /*
     * Reset frequencies.
     */
    bsal_map_destroy(&scheduler->actor_source_frequencies);
    bsal_map_init(&scheduler->actor_source_frequencies, sizeof(int), sizeof(int));
}


void bsal_priority_scheduler_decrement(struct bsal_priority_scheduler *scheduler,
                int old_source_count)
{
    int old_source_count_old_frequency;
    int old_source_count_new_frequency;

    /*
     * Update the frequency for the old source count
     */

    if (bsal_map_get_value(&scheduler->actor_source_frequencies, &old_source_count,
                    &old_source_count_old_frequency)) {

        old_source_count_new_frequency = old_source_count_old_frequency - 1;

        bsal_map_update_value(&scheduler->actor_source_frequencies, &old_source_count,
                           &old_source_count_new_frequency);
    }
}

void bsal_priority_scheduler_increment(struct bsal_priority_scheduler *scheduler,
                int new_source_count)
{
    int new_source_count_old_frequency;
    int new_source_count_new_frequency;

    /*
     * Update the frequency for the new source count;
     */
    new_source_count_old_frequency = 0;

    if (bsal_map_get_value(&scheduler->actor_source_frequencies, &new_source_count,
                            &new_source_count_old_frequency)) {

        new_source_count_new_frequency = new_source_count_old_frequency + 1;
        bsal_map_update_value(&scheduler->actor_source_frequencies, &new_source_count,
                        &new_source_count_new_frequency);

    } else {
        /* This is the first actor with this source count
         */
        new_source_count_new_frequency = 1;
        bsal_map_add_value(&scheduler->actor_source_frequencies, &new_source_count,
                        &new_source_count_new_frequency);
    }

}
