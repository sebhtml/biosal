
#ifndef THORIUM_CONFIGURATION_H
#define THORIUM_CONFIGURATION_H

#define THORIUM_DEFAULT_EVENT_COUNT 8

/*
 * Maximum number of messages in a message block.
 */
#define THORIUM_MESSAGE_BLOCK_MAXIMUM_SIZE 32

/*
 * Maximum number of messages received from transport in the
 * main loop of node.
 */
#define THORIUM_NODE_MAXIMUM_RECEIVED_MESSAGE_COUNT_PER_CALL 4

/*
 * Maximum number of messages pulled from the worker
 * outbound message ring in the main loop of the node.
 *
 * The number of clean messages to pull at every call must be
 * greater.
 */
#define THORIUM_NODE_MAXIMUM_PULLED_MESSAGE_COUNT_PER_CALL 20
#define THORIUM_NODE_MAXIMUM_PULLED_CLEAN_MESSAGE_COUNT_PER_CALL 32

/*
 * Maximum number of messages pulled from the inbound message
 * ring in workers.
 */
#define THORIUM_WORKER_MAXIMUM_RECEIVED_MESSAGE_COUNT_PER_CALL THORIUM_DEFAULT_EVENT_COUNT

/*
 * Maximum number of received message requests to test in
 * transport.
 */
#define THORIUM_TRANSPORT_MAXIMUM_RECEIVED_MESSAGE_REQUEST_COUNT_PER_CALL THORIUM_DEFAULT_EVENT_COUNT

/*
 * Settings for a distributed system.
 */
#define BIOSAL_IDEAL_BUFFER_SIZE 1024
#define BIOSAL_IDEAL_ACTIVE_MESSAGE_LIMIT 4

/*
 * Settings for a shared memory system (1 process on 1 machine
 * yielding 1 Thorium runtime node.
 */
#define BIOSAL_IDEAL_BUFFER_SIZE_SHARED_MEMORY  4096
#define BIOSAL_IDEAL_ACTIVE_MESSAGE_LIMIT_SHARED_MEMORY 1

/*
 * Multiplexer options.
 */
/*
 * In nanoseconds
 */
/*
#define THORIUM_MULTIPLEXER_TIMEOUT 200000
*/
#define THORIUM_MULTIPLEXER_TIMEOUT 200000
#define THORIUM_MULTIPLEXER_BUFFER_SIZE (1024*4)

#define THORIUM_ACTOR_COUNT_PER_NODE_TO_NODE_COUNT_RATIO 0.05

#endif
