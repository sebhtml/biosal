
#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER thorium_scheduler

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./engine/thorium/tracepoints/lttng/scheduler.h"

#if !defined(ENGINE_THORIUM_TRACEPOINTS_LTTNG_SCHEDULER_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define ENGINE_THORIUM_TRACEPOINTS_LTTNG_SCHEDULER_H

#include <lttng/tracepoint.h>


#include <engine/thorium/actor.h>

TRACEPOINT_EVENT_CLASS(
    thorium_scheduler,
    class0,
    TP_ARGS(
        int, worker,
        int, size_before,
        struct thorium_actor *, actor
    ),
    TP_FIELDS(
        ctf_integer(int, worker, worker)
        ctf_integer(int, size_before, size_before)
        ctf_integer(int, actor, actor->name)
    )
)

/*
        ctf_integer_hex(int, script, actor->script->identifier)
*/

#define THORIUM_SCHEDULER_EVENT(event_name) \
    TRACEPOINT_EVENT_INSTANCE(thorium_scheduler, class0, event_name, \
    TP_ARGS(int, worker, int, size_before, struct thorium_actor *, actor))

THORIUM_SCHEDULER_EVENT(enqueue)
THORIUM_SCHEDULER_EVENT(dequeue)

#endif /* ENGINE_THORIUM_TRACEPOINTS_LTTNG_SCHEDULER_H */

#include <lttng/tracepoint-event.h>
