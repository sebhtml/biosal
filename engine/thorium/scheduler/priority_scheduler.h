
#ifndef THORIUM_PRIORITY_SCHEDULER
#define THORIUM_PRIORITY_SCHEDULER

#include <core/structures/map.h>

#include <time.h>

struct thorium_actor;

/*
 * This scheduler assigns priorities to actors.
 *
 * Typically, each worker has its own scheduler
 * to assign priority values.
 */
struct thorium_priority_scheduler {

    /*
     * Stuff to update priorities in real time
     */
    int name;
    struct bsal_map actor_sources;
    struct bsal_map actor_source_frequencies;

    int normal_priority_minimum_value;
    int normal_priority_maximum_value;
    int max_priority_minimum_value;

    time_t last_update;
};

void thorium_priority_scheduler_init(struct thorium_priority_scheduler *self, int name);
void thorium_priority_scheduler_destroy(struct thorium_priority_scheduler *self);

void thorium_priority_scheduler_update(struct thorium_priority_scheduler *self, struct thorium_actor *actor);
void thorium_priority_scheduler_update_thresholds(struct thorium_priority_scheduler *self);

void thorium_priority_scheduler_increment(struct thorium_priority_scheduler *self,
                int new_source_count);

void thorium_priority_scheduler_decrement(struct thorium_priority_scheduler *self,
                int old_source_count);

#endif
