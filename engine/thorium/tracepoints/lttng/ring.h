
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
        int, ring_number,
        char *, operation,
        char *, time,
        int, ring_size,
        int, ring_capacity,
        int, ring_head,
        int, ring_tail,
        int, worker_name
    ),
    TP_FIELDS(
        ctf_string(operation, operation)
        ctf_string(time, time)
        ctf_integer(int, size, ring_size)
        ctf_integer(int, capacity, ring_capacity)
        ctf_integer(int, head, ring_head)
        ctf_integer(int, tail, ring_tail)
        ctf_integer(int, worker, worker_name)
    )
)



#endif /* ENGINE_THORIUM_TRACEPOINTS_LTTNG_RING_H */

#include <lttng/tracepoint-event.h>
