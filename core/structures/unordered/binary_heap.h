
#ifndef CORE_BINARY_HEAP_H
#define CORE_BINARY_HEAP_H

#include "binary_heap_array.h"

#define CORE_BINARY_HEAP_MIN            (1 << 0)
#define CORE_BINARY_HEAP_MAX            (1 << 1)
#define CORE_BINARY_HEAP_UINT64_T_KEYS  (1 << 2)
#define CORE_BINARY_HEAP_INT_KEYS       (1 << 3)

#define core_binary_heap                    core_binary_heap_array

#define core_binary_heap_init               core_binary_heap_array_init
#define core_binary_heap_destroy            core_binary_heap_array_destroy
#define core_binary_heap_insert             core_binary_heap_array_insert
#define core_binary_heap_delete_root        core_binary_heap_array_delete_root
#define core_binary_heap_get_root           core_binary_heap_array_get_root
#define core_binary_heap_empty              core_binary_heap_array_empty
#define core_binary_heap_size               core_binary_heap_array_size
#define core_binary_heap_set_memory_pool    core_binary_heap_array_set_memory_pool

#endif
