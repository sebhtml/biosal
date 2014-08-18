
#ifndef THORIUM_PRIORITY_SCHEDULER
#define THORIUM_PRIORITY_SCHEDULER

#include <core/structures/map.h>

struct thorium_actor;

struct thorium_priority_scheduler {

    /*
     * Stuff to update priorities in real time
     */

    struct bsal_map actor_sources;
    struct bsal_map actor_source_frequencies;

    int max_priority_minimum_value;
    int high_priority_minimum_value;
    int low_priority_maximum_value;

    int calls;
    int changes;
};

void thorium_priority_scheduler_init(struct thorium_priority_scheduler *self);
void thorium_priority_scheduler_destroy(struct thorium_priority_scheduler *self);

void thorium_priority_scheduler_update(struct thorium_priority_scheduler *self, struct thorium_actor *actor);
void thorium_priority_scheduler_update_thresholds(struct thorium_priority_scheduler *self);

void thorium_priority_scheduler_increment(struct thorium_priority_scheduler *self,
                int new_source_count);

void thorium_priority_scheduler_decrement(struct thorium_priority_scheduler *self,
                int old_source_count);

#endif
