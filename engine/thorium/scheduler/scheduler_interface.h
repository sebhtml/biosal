
#ifndef THORIUM_SCHEDULER_INTERFACE_H
#define THORIUM_SCHEDULER_INTERFACE_H

struct thorium_scheduler;
struct thorium_actor;

/*
 * Interface to define new actor schedulers
 */
struct thorium_scheduler_interface {
    int identifier;
    char *name;
    int object_size;
    void (*init)(struct thorium_scheduler *self);
    void (*destroy)(struct thorium_scheduler *self);
    int (*enqueue)(struct thorium_scheduler *self, struct thorium_actor *actor);
    int (*dequeue)(struct thorium_scheduler *self, struct thorium_actor **actor);
    int (*size)(struct thorium_scheduler *self);
    void (*print)(struct thorium_scheduler *self, int node, int worker);
};

#endif
