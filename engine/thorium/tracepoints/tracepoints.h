
#ifndef THORIUM_TRACEPOINT_H
#define THORIUM_TRACEPOINT_H

#include <core/system/timer.h>

#ifdef THORIUM_USE_LTTNG
#include "lttng/message.h"
#include "lttng/transport.h"
#include "lttng/scheduler.h"
#include "lttng/actor.h"
#include "lttng/ring.h"
#endif

#include "actor_tracepoints.h"
#include "message_tracepoints.h"
#include "node_tracepoints.h"

#include <stdint.h>

/*
 * Here, the tracepoints are similar to those in
 * lttng. The difference is that in Thorium, tracepoint data
 * can be for instance embedded
 * into messages.
 *
 * \see http://lttng.org/man/3/lttng-ust/
 *
 * __VA_ARGS__ is defined in C 1999
 *
 * \see http://en.wikipedia.org/wiki/Variadic_macro
 *
 * \see https://www.kernel.org/doc/Documentation/trace/tracepoint-analysis.txt
 * \see https://www.kernel.org/doc/Documentation/trace/tracepoints.txt
 */

/*
 * Right now, tracepoints must be enabled with
 * THORIUM_ENABLE_TRACEPOINTS
 */
/*
#define THORIUM_ENABLE_TRACEPOINTS
*/

#define thorium_tracepoint_DISABLED(...) \

#define THORIUM_TRACEPOINT_NAME(provider_name, event_name) \
        thorium_tracepoint_##provider_name##_##event_name

#ifdef THORIUM_USE_LTTNG

/*
#define thorium_tracepoint tracepoint
*/

#elif defined(THORIUM_ENABLE_TRACEPOINTS)

/*
 * TODO: add a flag to activate these a run time.
 */
#define tracepoint(provider_name, event_name, ...) \
{ \
    uint64_t tracepoint_time; \
    tracepoint_time = 0; \
    struct core_timer tracepoint_timer; \
    core_timer_init(&tracepoint_timer); \
    tracepoint_time = core_timer_get_nanoseconds(&tracepoint_timer); \
    core_timer_destroy(&tracepoint_timer); \
    THORIUM_TRACEPOINT_NAME(provider_name, event_name)(tracepoint_time, __VA_ARGS__); \
}

#else

/*
 * Do nothing.
 */
#define tracepoint(provider_name, event_name, ...) \

#endif

/*
#define THORIUM_DEFINE_TRACEPOINT(provider_name, event_name, hook) \
        #ifdef THORIUM_TRACEPOINT_ENABLE##provider_name##event_name## \
        #define THORIUM_TRACEPOINT_NAME(provider_name, event_name) \
            hoo
            */
#endif
