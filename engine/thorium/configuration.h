
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

#define THORIUM_LIGHTWEIGHT_ACTOR_COUNT_PER_WORKER 1

/*
 * N: number of nodes
 * W: number of workers per node
 * A: number of actors per worker
 *
 * Ratio = (W * A) / N
 *
 * Example:
 * N = 4
 * W = 7
 * A = 57
 *
 * Ratio = (7 * 57) / 4 = 99.75
 */
#define THORIUM_ACTOR_COUNT_PER_NODE_TO_NODE_COUNT_RATIO 100

#endif
