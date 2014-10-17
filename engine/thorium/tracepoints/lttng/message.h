
#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER message

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./engine/thorium/tracepoints/lttng/message.h"

#include <engine/thorium/message.h>

#if !defined(ENGINE_THORIUM_TRACEPOINTS_LTTNG_MESSAGE_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define ENGINE_THORIUM_TRACEPOINTS_LTTNG_MESSAGE_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(
    message,
    actor_send,
    TP_ARGS(
        struct thorium_message *, message
    ),
    TP_FIELDS(
        ctf_integer(int, message_number, message->number)
        ctf_integer(int, message_action, message->action)
        ctf_integer(int, message_count, message->count)
        ctf_integer(int, message_source_actor, message->source_actor)
        ctf_integer(int, message_destination_actor, message->destination_actor)
        ctf_integer(int, message_source_node, message->source_node)
        ctf_integer(int, message_destination_node, message->destination_node)
    )
)

#endif /* ENGINE_THORIUM_TRACEPOINTS_LTTNG_MESSAGE_H */

#include <lttng/tracepoint-event.h>
