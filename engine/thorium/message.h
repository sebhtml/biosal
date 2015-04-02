
#ifndef THORIUM_MESSAGE_H
#define THORIUM_MESSAGE_H

/* helpers */

#include "modules/message_helper.h"

#include <stdint.h>

#define THORIUM_MESSAGE_TYPE_NONE               0
#define THORIUM_MESSAGE_TYPE_NODE_INBOUND       1
#define THORIUM_MESSAGE_TYPE_NODE_OUTBOUND      2
#define THORIUM_MESSAGE_TYPE_WORKER_OUTBOUND    3

/*
 * A message can not be distributed with the action specific
 * ACTION_INVALID.
 */

#define ACTION_INVALID -1

/*
 * Enable the routing information to be stored
 * inside messages.
 * This is required for the code in engine/thorium/cache/.
 */
/*
*/
#define THORIUM_MESSAGE_USE_ROUTING

/*
 * Use tracepoints to analyze the life cycle of messages.
 */
/*
#define THORIUM_MESSAGE_ENABLE_TRACEPOINTS
*/

/*
 * There are 8 message tracepoints
 */
#define THORIUM_TRACEPOINT_message_actor_send               0
#define THORIUM_TRACEPOINT_message_worker_send              1
#define THORIUM_TRACEPOINT_message_node_send                2
#define THORIUM_TRACEPOINT_message_node_send_system         3
#define THORIUM_TRACEPOINT_message_node_send_dispatch       4
#define THORIUM_TRACEPOINT_message_worker_pool_enqueue      5
#define THORIUM_TRACEPOINT_message_transport_send           6
#define THORIUM_TRACEPOINT_message_transport_receive        7
#define THORIUM_TRACEPOINT_message_node_receive             8
#define THORIUM_TRACEPOINT_message_worker_receive           9
#define THORIUM_TRACEPOINT_message_actor_receive           10
#define THORIUM_TRACEPOINT_message_node_dispatch_message   11
#define THORIUM_TRACEPOINT_message_worker_enqueue_message  12
#define THORIUM_TRACEPOINT_message_worker_send_mailbox     13
#define THORIUM_TRACEPOINT_message_worker_send_schedule    14
#define THORIUM_TRACEPOINT_message_worker_dequeue_message  15
#define THORIUM_TRACEPOINT_message_worker_pool_dequeue     16

#define THORIUM_MESSAGE_TRACEPOINT_COUNT        17

#define THORIUM_MESSAGE_TRACEPOINT_NO_VALUE     0

/*
 * Tracepoints in messages is a compilation option.
 */
#ifdef THORIUM_MESSAGE_ENABLE_TRACEPOINTS

#define TRACEPOINT_SIZE (THORIUM_MESSAGE_TRACEPOINT_COUNT * sizeof(uint64_t))

#else

#define TRACEPOINT_SIZE 0

#endif

#ifdef THORIUM_MESSAGE_USE_ROUTING
#define ROUTING_SIZE (2 * sizeof(int))
#else
#define ROUTING_SIZE (0)
#endif

/*
 * - action
 * - number
 * - source_actor
 * - destination_actor
 *
 * - routing_source
 * - routing_destination
 *
 * - tracepoint stuff (disabled by default)
 */
#define THORIUM_MESSAGE_METADATA_SIZE (5 * sizeof(int) + ROUTING_SIZE + TRACEPOINT_SIZE)

/*
 * This is a message.
 */
struct thorium_message {
    int action;

    /*
     * Message identifiers.
     * _message_identifier is unique for a given source_actor.
     *
     * the pair (_parent_message_actor, _parent_message_identifier) is unique too.
     */
    int _message_identifier;
    int _parent_message_actor;
    int _parent_message_identifier;

    void *buffer;
    int count;

    int source_actor;
    int destination_actor;

    int source_node;
    int destination_node;

#ifdef THORIUM_MESSAGE_USE_ROUTING
    int routing_source;
    int routing_destination;
#endif

    int worker;

    int type;

#ifdef THORIUM_MESSAGE_ENABLE_TRACEPOINTS
    uint64_t tracepoint_times[THORIUM_MESSAGE_TRACEPOINT_COUNT];
#endif
};

void thorium_message_init(struct thorium_message *self, int action, int count, void *buffer);
void thorium_message_init_with_nodes(struct thorium_message *self, int count, void *buffer, int source,
                int destination);
void thorium_message_init_copy(struct thorium_message *self, struct thorium_message *old_message);
void thorium_message_destroy(struct thorium_message *self);

int thorium_message_source(struct thorium_message *self);
void thorium_message_set_source(struct thorium_message *self, int source);

int thorium_message_destination(struct thorium_message *self);
void thorium_message_set_destination(struct thorium_message *self, int destination);

int thorium_message_action(struct thorium_message *self);
void thorium_message_set_action(struct thorium_message *self, int action);

void *thorium_message_buffer(struct thorium_message *self);
void thorium_message_set_buffer(struct thorium_message *self, void *buffer);

int thorium_message_count(struct thorium_message *self);
void thorium_message_set_count(struct thorium_message *self, int count);

int thorium_message_source_node(struct thorium_message *self);
void thorium_message_set_source_node(struct thorium_message *self, int source);

int thorium_message_destination_node(struct thorium_message *self);
void thorium_message_set_destination_node(struct thorium_message *self, int destination);

void thorium_message_print(struct thorium_message *self);

void thorium_message_set_worker(struct thorium_message *self, int worker);
int thorium_message_worker(struct thorium_message *self);

int thorium_message_is_recycled(struct thorium_message *self);

int thorium_message_pack_unpack(struct thorium_message *self, int operation, void *buffer);

int thorium_message_type(struct thorium_message *self);
void thorium_message_set_type(struct thorium_message *self, int type);

#ifdef THORIUM_MESSAGE_ENABLE_TRACEPOINTS
void thorium_message_set_tracepoint_time(struct thorium_message *self, int tracepoint,
                uint64_t time);
uint64_t thorium_message_get_tracepoint_time(struct thorium_message *self, int tracepoint);
void thorium_message_print_tracepoints(struct thorium_message *self);
#endif

void thorium_message_set_identifier(struct thorium_message *self, int identifier);
void thorium_message_set_parent_identifier(struct thorium_message *self, int identifier);
void thorium_message_set_parent_actor(struct thorium_message *self, int identifier);
int thorium_message_get_identifier(struct thorium_message *self);
int thorium_message_get_parent_identifier(struct thorium_message *self);
int thorium_message_get_parent_actor(struct thorium_message *self);

/*
 * Metadata functions.
 */
int thorium_message_metadata_size(struct thorium_message *self);
int thorium_message_read_metadata(struct thorium_message *self);
int thorium_message_write_metadata(struct thorium_message *self);

int thorium_message_read_metadata_for_tracepoint(struct thorium_message *self);
void thorium_message_add_metadata_to_count(struct thorium_message *self);
void thorium_message_remove_metadata_from_count(struct thorium_message *self);

uint64_t thorium_message_signature(struct thorium_message *self);

void thorium_message_set_routing_destination_node(struct thorium_message *self, int destination);
void thorium_message_set_routing_source_node(struct thorium_message *self, int source);

#define SOURCE(self) \
        thorium_message_source(self)

#define BUFFER(self) \
        thorium_message_buffer(self)

#define COUNT(self) \
        thorium_message_count(self)

#define SRC SOURCE

#endif
