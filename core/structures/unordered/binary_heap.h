
#ifndef CORE_BINARY_HEAP_H
#define CORE_BINARY_HEAP_H

#include <core/structures/vector.h>

#define CORE_BINARY_HEAP_MIN            (1 << 0)
#define CORE_BINARY_HEAP_MAX            (1 << 1)
#define CORE_BINARY_HEAP_UINT64_T_KEYS  (1 << 2)
#define CORE_BINARY_HEAP_INT_KEYS       (1 << 3)

/**
 * @see http://www.stack.nl/~dimitri/doxygen/manual/docblocks.html
 */

/**
 * Binary heap.
 *
 * This is either a min-heap or a max-heap.
 */
struct core_binary_heap {
    struct core_vector vector;
    int key_size;
    int value_size;
    int size;
    int (*test_relation)(struct core_binary_heap *self, void *key1, void *key2);
};

/**
 * @param flags setup flags, can include CORE_BINARY_HEAP_MIN or CORE_BINARY_HEAP_MAX.
 */
void core_binary_heap_init(struct core_binary_heap *self, int key_size,
                int value_size, uint32_t flags);
void core_binary_heap_destroy(struct core_binary_heap *self);

/**
 * Insert a key-value pair.
 *
 * @return true is inserted
 */
int core_binary_heap_insert(struct core_binary_heap *self, void *key, void *value);

/**
 *
 * @return true if there is a root.
 */
int core_binary_heap_get_root(struct core_binary_heap *self, void **key, void **value);

/**
 * Delete the root.
 */
int core_binary_heap_delete_root(struct core_binary_heap *self);

/**
 * @return the size of the heap (the number of key-value pairs).
 */
int core_binary_heap_size(struct core_binary_heap *self);

#endif
