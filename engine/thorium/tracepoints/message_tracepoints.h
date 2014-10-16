
#ifndef THORIUM_MESSAGE_TRACEPOINTS_H
#define THORIUM_MESSAGE_TRACEPOINTS_H

/*
 * Thorium tracepoints are used to gather information about events.
 * Tracepoint information can be embedded in actor messages.
 *
 * \see https://github.com/giraldeau/lttng-ust/blob/master/include/lttng/tracepoint.h
 */
#include "tracepoints.h"

#include <stdint.h>

struct thorium_message;

void thorium_tracepoint_message_actor_send(struct thorium_message *message, uint64_t time);
void thorium_tracepoint_message_worker_send(struct thorium_message *message, uint64_t time);
void thorium_tracepoint_message_node_send(struct thorium_message *message, uint64_t time);
void thorium_tracepoint_message_transport_send(struct thorium_message *message, uint64_t time);
void thorium_tracepoint_message_transport_receive(struct thorium_message *message, uint64_t time);
void thorium_tracepoint_message_node_receive(struct thorium_message *message, uint64_t time);
void thorium_tracepoint_message_worker_receive(struct thorium_message *message, uint64_t time);
void thorium_tracepoint_message_actor_receive(struct thorium_message *message, uint64_t time);

#endif
