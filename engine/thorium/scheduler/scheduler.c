
#include "scheduler.h"

#include "cfs_scheduler.h"
#include "fifo_scheduler.h"

#include <core/system/memory.h>
#include <core/system/debugger.h>

/*
 * The available schedulers are:
 *
 * - THORIUM_CFS_SCHEDULER
 * - THORIUM_FIFO_SCHEDULER
 */
#define THORIUM_DEFAULT_SCHEDULER THORIUM_CFS_SCHEDULER

void thorium_scheduler_init(struct thorium_scheduler *self, int node, int worker)
{
    self->scheduler = THORIUM_DEFAULT_SCHEDULER;
    self->node = node;
    self->worker = worker;

    self->concrete_self = NULL;
    self->implementation = NULL;

    /*
     * Find implementation.
     */
    if (self->scheduler == thorium_fifo_scheduler_implementation.identifier) {
        self->implementation = &thorium_fifo_scheduler_implementation;
    } else if (self->scheduler == thorium_cfs_scheduler_implementation.identifier) {
        self->implementation = &thorium_cfs_scheduler_implementation;
    }

    BSAL_DEBUGGER_ASSERT(self->implementation != NULL);

    self->concrete_self = bsal_memory_allocate(self->implementation->object_size);

    self->implementation->init(self);

    if (self->node == 0 && self->worker == 0)
        printf("thorium_scheduler: type %s\n", self->implementation->name);
}

void thorium_scheduler_destroy(struct thorium_scheduler *self)
{
    self->implementation->destroy(self);

    if (self->concrete_self != NULL) {
        bsal_memory_free(self->concrete_self);
        self->concrete_self = NULL;
    }

    self->implementation = NULL;
}

int thorium_scheduler_enqueue(struct thorium_scheduler *self, struct thorium_actor *actor)
{
    return self->implementation->enqueue(self, actor);
}

int thorium_scheduler_dequeue(struct thorium_scheduler *self, struct thorium_actor **actor)
{
    return self->implementation->dequeue(self, actor);
}

int thorium_scheduler_size(struct thorium_scheduler *self)
{
    return self->implementation->size(self);
}

void thorium_scheduler_print(struct thorium_scheduler *self)
{
    self->implementation->print(self);
}
