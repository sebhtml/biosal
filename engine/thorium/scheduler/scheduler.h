
#ifndef THORIUM_SCHEDULER_H
#define THORIUM_SCHEDULER_H

#include "scheduler_interface.h"

#include <stdint.h>

struct thorium_actor;

/*
 * \see http://dictionary.cambridge.org/dictionary/british/max_1
 */
#define THORIUM_PRIORITY_LOW 4
#define THORIUM_PRIORITY_NORMAL 64
#define THORIUM_PRIORITY_HIGH 128
#define THORIUM_PRIORITY_MAX 1048576

/*
 * This is an actor scheduling queue.
 * Each worker has one of these.
 */
struct thorium_scheduler {
    int scheduler;
    void *concrete_self;
    struct thorium_scheduler_interface *implementation;
};

void thorium_scheduler_init(struct thorium_scheduler *self);
void thorium_scheduler_destroy(struct thorium_scheduler *self);

int thorium_scheduler_enqueue(struct thorium_scheduler *self, struct thorium_actor *actor);
int thorium_scheduler_dequeue(struct thorium_scheduler *self, struct thorium_actor **actor);

int thorium_scheduler_size(struct thorium_scheduler *self);

void thorium_scheduler_print(struct thorium_scheduler *self, int node, int worker);

#endif
