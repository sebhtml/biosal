
#ifndef BSAL_PRIORITY_SCHEDULER
#define BSAL_PRIORITY_SCHEDULER

#include <core/structures/map.h>

struct bsal_actor;

struct bsal_priority_scheduler {

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

void bsal_priority_scheduler_init(struct bsal_priority_scheduler *scheduler);
void bsal_priority_scheduler_destroy(struct bsal_priority_scheduler *scheduler);

void bsal_priority_scheduler_update(struct bsal_priority_scheduler *scheduler, struct bsal_actor *actor);
void bsal_priority_scheduler_update_thresholds(struct bsal_priority_scheduler *scheduler);

void bsal_priority_scheduler_increment(struct bsal_priority_scheduler *scheduler,
                int new_source_count);

void bsal_priority_scheduler_decrement(struct bsal_priority_scheduler *scheduler,
                int old_source_count);

#endif
