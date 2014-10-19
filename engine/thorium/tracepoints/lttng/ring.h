
#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER ring

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./engine/thorium/tracepoints/lttng/ring.h"

#if !defined(ENGINE_THORIUM_TRACEPOINTS_LTTNG_RING_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define ENGINE_THORIUM_TRACEPOINTS_LTTNG_RING_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(
    ring,
    operation,
    TP_ARGS(
        char *, operation,
        int, action,
        int, head,
        int, tail,
        int, claimed_tail,
        int, worker,
        int, size,
        int, capacity
    ),
    TP_FIELDS(
        ctf_string(operation, operation)
        ctf_integer(int, action, action)
        ctf_integer(int, head, head)
        ctf_integer(int, tail, tail)
        ctf_integer(int, claimed_tail, claimed_tail)
        ctf_integer(int, worker, worker)
        ctf_integer(int, size, size)
        ctf_integer(int, capacity, capacity)
    )
)



#endif /* ENGINE_THORIUM_TRACEPOINTS_LTTNG_RING_H */

#include <lttng/tracepoint-event.h>
