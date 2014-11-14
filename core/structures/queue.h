
#ifndef CORE_QUEUE_H
#define CORE_QUEUE_H

#include "block_queue.h"
#include "fast_queue.h"
#include "simple_queue.h"

#define core_queue                  core_simple_queue

#define core_queue_init             core_simple_queue_init
#define core_queue_destroy          core_simple_queue_destroy

#define core_queue_enqueue          core_simple_queue_enqueue
#define core_queue_dequeue          core_simple_queue_dequeue

#define core_queue_set_memory_pool  core_simple_queue_set_memory_pool

#define core_queue_size             core_simple_queue_size
#define core_queue_empty            core_simple_queue_empty
#define core_queue_full             core_simple_queue_full
#define core_queue_capacity         core_simple_queue_capacity

#endif

