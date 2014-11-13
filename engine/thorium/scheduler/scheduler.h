
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

#define THORIUM_SCHEDULER_STATUS_CREATED        0
#define THORIUM_SCHEDULER_STATUS_TERMINATED     1
#define THORIUM_SCHEDULER_STATUS_SCHEDULED      2
#define THORIUM_SCHEDULER_STATUS_RECEIVING      3
#define THORIUM_SCHEDULER_STATUS_IDLE           4

/*
 * This is an actor scheduling queue.
 * Each worker has one of these.
 */
struct thorium_scheduler {
    void *concrete_self;
    struct thorium_scheduler_interface *implementation;
    int scheduler;
    int node;
    int worker;
};

void thorium_scheduler_init(struct thorium_scheduler *self, int node, int worker);
void thorium_scheduler_destroy(struct thorium_scheduler *self);

int thorium_scheduler_enqueue(struct thorium_scheduler *self, struct thorium_actor *actor);
int thorium_scheduler_dequeue(struct thorium_scheduler *self, struct thorium_actor **actor);

int thorium_scheduler_size(struct thorium_scheduler *self);

void thorium_scheduler_print(struct thorium_scheduler *self);
void thorium_scheduler_print_type(struct thorium_scheduler *self);

#endif
