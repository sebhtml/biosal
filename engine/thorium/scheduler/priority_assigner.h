
#ifndef THORIUM_PRIORITY_ASSIGNER_H
#define THORIUM_PRIORITY_ASSIGNER_H

#include <core/structures/map.h>

#include <time.h>

struct thorium_actor;

/*
 * This scheduler assigns priorities to actors.
 *
 * Typically, each worker has its own scheduler
 * to assign priority values.
 */
struct thorium_priority_assigner {

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

void thorium_priority_assigner_init(struct thorium_priority_assigner *self, int name);
void thorium_priority_assigner_destroy(struct thorium_priority_assigner *self);

void thorium_priority_assigner_update(struct thorium_priority_assigner *self, struct thorium_actor *actor);
void thorium_priority_assigner_update_thresholds(struct thorium_priority_assigner *self);

void thorium_priority_assigner_increment(struct thorium_priority_assigner *self,
                int new_source_count);

void thorium_priority_assigner_decrement(struct thorium_priority_assigner *self,
                int old_source_count);

#endif
