
#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER thorium_message

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./engine/thorium/tracepoints/lttng/message.h"

#if !defined(ENGINE_THORIUM_TRACEPOINTS_LTTNG_MESSAGE_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define ENGINE_THORIUM_TRACEPOINTS_LTTNG_MESSAGE_H

#include <lttng/tracepoint.h>

#include <engine/thorium/message.h>
#include <core/structures/fast_ring.h>

TRACEPOINT_EVENT_CLASS(
    thorium_message,
    template,
    TP_ARGS(
        struct thorium_message *, message
    ),
    TP_FIELDS(
        ctf_integer(int, message_number, message->number)
        ctf_integer_hex(int, message_action, message->action)
        ctf_integer(int, message_count, message->count)
        ctf_integer(int, message_source_actor, message->source_actor)
        ctf_integer(int, message_destination_actor, message->destination_actor)
        ctf_integer(int, message_source_node, message->source_node)
        ctf_integer(int, message_destination_node, message->destination_node)
    )
)

TRACEPOINT_EVENT(
    thorium_message,
    dummy,
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

/*
 * This macro does work.
 */
#define THORIUM_MESSAGE_TRACEPOINT(event_name) \
    TRACEPOINT_EVENT_INSTANCE(thorium_message, template, event_name, \
    TP_ARGS(struct thorium_message *, message))

THORIUM_MESSAGE_TRACEPOINT(actor_send)
THORIUM_MESSAGE_TRACEPOINT(node_send)
THORIUM_MESSAGE_TRACEPOINT(worker_send)
THORIUM_MESSAGE_TRACEPOINT(worker_receive)
THORIUM_MESSAGE_TRACEPOINT(worker_send_mailbox)
THORIUM_MESSAGE_TRACEPOINT(worker_send_schedule)
THORIUM_MESSAGE_TRACEPOINT(worker_pool_enqueue)
THORIUM_MESSAGE_TRACEPOINT(worker_pool_dequeue)
THORIUM_MESSAGE_TRACEPOINT(worker_enqueue_message)
THORIUM_MESSAGE_TRACEPOINT(actor_receive)
THORIUM_MESSAGE_TRACEPOINT(worker_dequeue_message)
THORIUM_MESSAGE_TRACEPOINT(node_send_system)
THORIUM_MESSAGE_TRACEPOINT(node_receive)
THORIUM_MESSAGE_TRACEPOINT(node_receive_message)
THORIUM_MESSAGE_TRACEPOINT(node_send_dispatch)
THORIUM_MESSAGE_TRACEPOINT(node_dispatch_message)
THORIUM_MESSAGE_TRACEPOINT(transport_send)
THORIUM_MESSAGE_TRACEPOINT(transport_receive)

/*
 * This one is a bit more complex.
 */
TRACEPOINT_EVENT(
    thorium_message,
    worker_publish_message,
    TP_ARGS(
        struct thorium_message *, message,
        struct core_fast_ring *, ring
    ),
    TP_FIELDS(
        ctf_integer(int, message_number, message->number)
        ctf_integer(int, message_action, message->action)
        ctf_integer(int, message_count, message->count)
        ctf_integer(int, message_source_actor, message->source_actor)
        ctf_integer(int, message_destination_actor, message->destination_actor)
        ctf_integer(int, ring_size, core_fast_ring_size_from_producer(ring))
        ctf_integer(int, ring_capacity, core_fast_ring_capacity(ring))
        ctf_integer(int, ring_head, ring->head)
        ctf_integer(int, ring_tail, ring->tail)
    )
)



#endif /* ENGINE_THORIUM_TRACEPOINTS_LTTNG_MESSAGE_H */

#include <lttng/tracepoint-event.h>
