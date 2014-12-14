
#ifndef THORIUM_CONFIGURATION_H
#define THORIUM_CONFIGURATION_H

/*
 * Settings for a distributed system.
 */
#define BIOSAL_IDEAL_BUFFER_SIZE (2 * 1024)
#define BIOSAL_IDEAL_ACTIVE_MESSAGE_LIMIT 2

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
#define THORIUM_MULTIPLEXER_TIMEOUT 200000
#define THORIUM_MULTIPLEXER_BUFFER_SIZE (1024*4)

#define THORIUM_ACTOR_COUNT_PER_NODE_TO_NODE_COUNT_RATIO 0.01

/*
 * The IBM Blue Gene/Q has a lower amount of memory per core.
 */
#ifdef __bgq__
#undef THORIUM_ACTOR_COUNT_PER_NODE_TO_NODE_COUNT_RATIO
#define THORIUM_ACTOR_COUNT_PER_NODE_TO_NODE_COUNT_RATIO 0.05
#endif

#endif
