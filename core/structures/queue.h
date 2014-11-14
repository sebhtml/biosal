
#ifndef CORE_QUEUE_H
#define CORE_QUEUE_H

#include "block_queue.h"
#include "fast_queue.h"

#define core_queue                  core_fast_queue

#define core_queue_init             core_fast_queue_init
#define core_queue_destroy          core_fast_queue_destroy

#define core_queue_enqueue          core_fast_queue_enqueue
#define core_queue_dequeue          core_fast_queue_dequeue

#define core_queue_set_memory_pool  core_fast_queue_set_memory_pool

#define core_queue_size             core_fast_queue_size
#define core_queue_empty            core_fast_queue_empty
#define core_queue_full             core_fast_queue_full
#define core_queue_capacity         core_fast_queue_capacity

#endif

