
#include "priority_assigner.h"

#include "scheduler.h"

#include <engine/thorium/actor.h>

#include <core/structures/map_iterator.h>

#include <core/helpers/statistics.h>
#include <core/system/debugger.h>

#include <stdio.h>

#define TIME_THRESHOLD 10

#define THORIUM_PRIORITY_SCHEDULER_DEBUG

void thorium_priority_assigner_init(struct thorium_priority_assigner *scheduler, int name)
{
    scheduler->name = name;
    bsal_map_init(&scheduler->actor_sources, sizeof(int), sizeof(int));
    bsal_map_init(&scheduler->actor_source_frequencies, sizeof(int), sizeof(int));

    scheduler->normal_priority_minimum_value = -1;
    scheduler->normal_priority_maximum_value= -1;
    scheduler->max_priority_minimum_value = -1;

    scheduler->last_update = time(NULL);
}

void thorium_priority_assigner_destroy(struct thorium_priority_assigner *scheduler)
{

    bsal_map_destroy(&scheduler->actor_sources);
    bsal_map_destroy(&scheduler->actor_source_frequencies);

    scheduler->normal_priority_minimum_value = -1;
    scheduler->normal_priority_maximum_value= -1;
    scheduler->max_priority_minimum_value = -1;
}

/*
 * Gather data for sources.
 * Detect hubs
 *
 *           P30              P70             P95
 *          |     NORMAL         | HIGH      | MAX
 */
void thorium_priority_assigner_update(struct thorium_priority_assigner *scheduler, struct thorium_actor *actor)
{
    int old_priority;
    int new_priority;
    int old_source_count;
    int new_source_count;
    int class_count;
    int name;
    time_t now;

    BSAL_DEBUGGER_ASSERT(actor != NULL);

    name = thorium_actor_name(actor);
    new_source_count = thorium_actor_get_source_count(actor);

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

            thorium_priority_assigner_decrement(scheduler, old_source_count);
            thorium_priority_assigner_increment(scheduler, new_source_count);
        }

    } else {

        /* It is the first time that this actor is encountered
         */
        bsal_map_add_value(&scheduler->actor_sources, &name, &new_source_count);

        thorium_priority_assigner_increment(scheduler, new_source_count);
    }

    /* Update the priority now.
     */

    if (scheduler->normal_priority_minimum_value != -1) {

        old_priority = thorium_actor_get_priority(actor);

        new_priority = THORIUM_PRIORITY_NORMAL;

        if (new_source_count >= scheduler->max_priority_minimum_value) {
            new_priority = THORIUM_PRIORITY_MAX;
        }
#if 0
        if (scheduler->normal_priority_minimum_value <= new_source_count
                        && new_source_count <= scheduler->normal_priority_maximum_value) {
            new_priority = THORIUM_PRIORITY_NORMAL;

        } else if (new_source_count < scheduler->normal_priority_minimum_value) {
            new_priority = THORIUM_PRIORITY_LOW;

        } else if (new_source_count >= scheduler->max_priority_minimum_value) {
            new_priority = THORIUM_PRIORITY_MAX;

        } else {
            new_priority = THORIUM_PRIORITY_HIGH;
        }
#endif
        if (new_priority != old_priority) {
            thorium_actor_set_priority(actor, new_priority);

#ifdef THORIUM_PRIORITY_SCHEDULER_DEBUG
            printf("THORIUM_PRIORITY_SCHEDULER_DEBUG update priority %s/%d, old %d new %d\n",
                            thorium_actor_script_name(actor),
                            thorium_actor_name(actor), old_priority, new_priority);
#endif
        }
    }

    class_count = bsal_map_size(&scheduler->actor_source_frequencies);

    now = time(NULL);

    if (now - scheduler->last_update >= TIME_THRESHOLD) {

        thorium_priority_assigner_update_thresholds(scheduler);
        scheduler->last_update = now;
    }
}

void thorium_priority_assigner_update_thresholds(struct thorium_priority_assigner *scheduler)
{
    struct bsal_map_iterator iterator;
    int class_count;
    int value;
    int frequency;

    class_count = bsal_map_size(&scheduler->actor_source_frequencies);

#if 0
#ifdef THORIUM_PRIORITY_SCHEDULER_DEBUG
    printf("THORIUM_PRIORITY_SCHEDULER_DEBUG thorium_priority_assigner_update_thresholds: calls %d changes %d, class_count %d\n",
                    scheduler->calls, scheduler->changes, class_count);
#endif
#endif

    bsal_map_iterator_init(&iterator, &scheduler->actor_source_frequencies);

    /*
     * use percentiles instead of average, minimum and maximum.
     */

    class_count = 0;
    while (bsal_map_iterator_get_next_key_and_value(&iterator, &value, &frequency)) {

#ifdef THORIUM_PRIORITY_SCHEDULER_DEBUG
        if (frequency > 0) {
            printf("THORIUM_PRIORITY_SCHEDULER_DEBUG %d VALUE %d Frequency %d\n",
                            scheduler->name, value, frequency);
        }
#endif
        ++class_count;
    }

    if (class_count < 2)
        return;

#ifdef THORIUM_PRIORITY_SCHEDULER_DEBUG
    printf("THORIUM_PRIORITY_SCHEDULER_DEBUG class_count %d\n",
                    class_count);
#endif

    bsal_map_iterator_destroy(&iterator);

    /*
     * Actually update thresholds.
     */

    scheduler->normal_priority_minimum_value = bsal_statistics_get_percentile_int_map(&scheduler->actor_source_frequencies,
                    30);
    scheduler->normal_priority_maximum_value = bsal_statistics_get_percentile_int_map(&scheduler->actor_source_frequencies,
                    70);
    scheduler->max_priority_minimum_value = bsal_statistics_get_percentile_int_map(&scheduler->actor_source_frequencies,
                    95);

    printf("THORIUM_PRIORITY_SCHEDULER_DEBUG %d new thresholds P30 %d P70 %d P95 %d\n",
                    scheduler->name,
                    scheduler->normal_priority_minimum_value, scheduler->normal_priority_maximum_value,
                    scheduler->max_priority_minimum_value);

    /*
     * Reset frequencies.
     */
    bsal_map_destroy(&scheduler->actor_source_frequencies);
    bsal_map_init(&scheduler->actor_source_frequencies, sizeof(int), sizeof(int));
}

void thorium_priority_assigner_decrement(struct thorium_priority_assigner *scheduler,
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

void thorium_priority_assigner_increment(struct thorium_priority_assigner *scheduler,
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
