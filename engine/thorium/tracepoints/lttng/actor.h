
#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER thorium_actor

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./engine/thorium/tracepoints/lttng/actor.h"

#if !defined(ENGINE_THORIUM_TRACEPOINTS_LTTNG_ACTOR_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define ENGINE_THORIUM_TRACEPOINTS_LTTNG_ACTOR_H

#include <lttng/tracepoint.h>


#include <engine/thorium/message.h>
#include <engine/thorium/actor.h>

/*
 * The trick below is from: http://lists.lttng.org/pipermail/lttng-dev/2014-October/023607.html
 */
TRACEPOINT_EVENT_CLASS(
    thorium_actor,
    thorium_event_template,
    TP_ARGS(
        struct thorium_actor *, actor,
        struct thorium_message *, message
    ),
    TP_FIELDS(
        ctf_integer(int, name, actor->name)
        ctf_integer_hex(int, script, actor->script->identifier)
        ctf_integer_hex(int, action, message->action)
        ctf_integer(int, count, message->count)
    )
)

/*
 * foo is required because TRACEPOINT_EVENT_INSTANCE takes exactly 4 arguments:
 *
 * - provider name;
 * - event template name;
 * - event name;
 * - a 4th argument for arguments.
 */

TRACEPOINT_EVENT_INSTANCE(thorium_actor, thorium_event_template, receive_enter,
    TP_ARGS(
        struct thorium_actor *, actor,
        struct thorium_message *, message
    ))
TRACEPOINT_EVENT_INSTANCE(thorium_actor, thorium_event_template, receive_exit,
    TP_ARGS(
        struct thorium_actor *, actor,
        struct thorium_message *, message
    ))

#endif /* ENGINE_THORIUM_TRACEPOINTS_LTTNG_ACTOR_H */

#include <lttng/tracepoint-event.h>
