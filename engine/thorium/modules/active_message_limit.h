
#ifndef THORIUM_ACTOR_ACTIVE_MESSAGE_LIMIT_H
#define THORIUM_ACTOR_ACTIVE_MESSAGE_LIMIT_H

struct thorium_actor;

/*
 * This is the maximum number of in-flight messages for a given actor.
 * Any actor should respect this policy, but this is not a requirement.
 * Having a number greater than 1 makes it possible to keep the
 * processing pipeline busy, at the expense of memory usage.
 *
 * Here are some calculations.
 *
 * Technology: Cray XE6
 * Nodes: 256
 * Cores per node: 24
 *      For Thorium: 23
 *          Thorium node: 1
 *          Thorium workers: 22
 *      For system (core specialization / nemesis engine): 1
 * Memory per node with 3 active messages and 4096 bytes per message:
 * irb(main):002:0> 256*22*4096*22*3
 * => 1522532352
 *
 * Technology: IBM Blue Gene/Q
 * Nodes: 2048
 * Cores per node: 16
 *      For Thorium: 16
 *          Thorium node: 1
 *          Thorium workers: 15
 * irb(main):004:0> 2048 * 15 * 4096 * 15 * 3
 * => 5662310400
 *
 * Technology: Cray XC30
 * Technology: Cray CX40
 * Cray XC30 and Cray XC40 have a lot of memory per core, so it is fine.
 * (We did not try Cray XC30 and Cray XC40 so far.)
 *
 * Strategies for reduce memory usage for messages:
 *
 * - reduce number of graph stores
 * - reduce buffer size
 */
int thorium_actor_active_message_limit(struct thorium_actor *self);

/*
 * The idea of the suggested buffer size is simple. Any message should have a size
 * of around the value returned here. Obviously this is not a requirement.
 */
int thorium_actor_suggested_buffer_size(struct thorium_actor *self);

#endif
