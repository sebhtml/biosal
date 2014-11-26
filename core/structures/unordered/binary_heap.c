
#include "binary_heap.h"

#include <core/constants.h>

#include <core/system/debugger.h>
#include <core/system/memory.h>

#include <stdlib.h>
#include <string.h>

void *core_binary_heap_get_key(struct core_binary_heap *self, int i);
void *core_binary_heap_get_value(struct core_binary_heap *self, int i);

static inline int core_binary_heap_get_first_child(int i);
static inline int core_binary_heap_get_parent(int i);
static inline int core_binary_heap_get_second_child(int i);

void core_binary_heap_move_up(struct core_binary_heap *self, int i);
void core_binary_heap_move_down(struct core_binary_heap *self, int i);

void core_binary_heap_swap(struct core_binary_heap *self, int i, int j);

/*
 * relation functions.
 */

int core_binary_heap_test_relation(struct core_binary_heap *self, int i, int j);

int core_binary_heap_test_relation_lower_than_int(struct core_binary_heap *self,
                void *key1, void *key2);
int core_binary_heap_test_relation_lower_than_uint64_t(struct core_binary_heap *self,
                void *key1, void *key2);
int core_binary_heap_test_relation_lower_than_void_pointer(struct core_binary_heap *self,
                void *key1, void *key2);

int core_binary_heap_test_relation_greater_than_int(struct core_binary_heap *self,
                void *key1, void *key2);
int core_binary_heap_test_relation_greater_than_uint64_t(struct core_binary_heap *self,
                void *key1, void *key2);
int core_binary_heap_test_relation_greater_than_void_pointer(struct core_binary_heap *self,
                void *key1, void *key2);

void core_binary_heap_init(struct core_binary_heap *self, int key_size,
                int value_size, uint32_t flags)
{
    core_vector_init(&self->vector, key_size + value_size);

    self->size = 0;
    self->test_relation = NULL;
    self->key_size = key_size;
    self->value_size = value_size;

    if (flags & CORE_BINARY_HEAP_MIN) {

        if (flags & CORE_BINARY_HEAP_INT_KEYS)
            self->test_relation = core_binary_heap_test_relation_lower_than_int;
        else if (flags & CORE_BINARY_HEAP_UINT64_T_KEYS)
            self->test_relation = core_binary_heap_test_relation_lower_than_uint64_t;
        else
            self->test_relation = core_binary_heap_test_relation_lower_than_void_pointer;

    } else if (flags & CORE_BINARY_HEAP_MAX) {

        if (flags & CORE_BINARY_HEAP_INT_KEYS)
            self->test_relation = core_binary_heap_test_relation_greater_than_int;
        else if (flags & CORE_BINARY_HEAP_UINT64_T_KEYS)
            self->test_relation = core_binary_heap_test_relation_greater_than_uint64_t;
        else
            self->test_relation = core_binary_heap_test_relation_greater_than_void_pointer;
    }

    CORE_DEBUGGER_ASSERT_NOT_NULL(self->test_relation);
    CORE_DEBUGGER_ASSERT(self->key_size >= 1);
    CORE_DEBUGGER_ASSERT(self->value_size >= 0);
}

void core_binary_heap_destroy(struct core_binary_heap *self)
{
    core_vector_destroy(&self->vector);
}

int core_binary_heap_get_root(struct core_binary_heap *self, void **key, void **value)
{
    void *stored_key;
    void *stored_value;

    if (self->size == 0)
        return FALSE;

    stored_key = core_binary_heap_get_key(self, 0);
    stored_value = core_binary_heap_get_value(self, 0);

    if (key != NULL)
        *key = stored_key;

    if (value != NULL)
        *value = stored_value;

    return TRUE;
}

int core_binary_heap_delete_root(struct core_binary_heap *self)
{
    int last;
    int root;

    if (self->size == 0)
        return FALSE;

    last = self->size - 1;
    root = 0;

    /*
     * Swap with the end.
     */
    core_binary_heap_swap(self, root, last);
    --self->size;

    core_binary_heap_move_down(self, root);

    return TRUE;
}

int core_binary_heap_insert(struct core_binary_heap *self, void *key, void *value)
{
    int position;
    void *stored_key;
    void *stored_value;

#ifdef HEAP_DEBUG_INSERT
    printf("DEBUG before insert, %d elements\n", self->size);
    core_vector_print_int(&self->vector);
    printf("\n");
#endif

    position = self->size;

#ifdef HEAP_DEBUG_INSERT
    printf("DEBUG insert at position %d\n", position);
#endif

    if (position + 1 > core_vector_size(&self->vector))
        core_vector_resize(&self->vector, position + 1);

    ++self->size;

    /*
     * Add the pair at the end on the last level.
     */
    stored_key = core_binary_heap_get_key(self, position);
    stored_value = core_binary_heap_get_value(self, position);

    core_memory_copy(stored_key, key, self->key_size);

    if (self->value_size)
        core_memory_copy(stored_value, value, self->value_size);

#ifdef HEAP_DEBUG_INSERT
    printf("DEBUG before move_up\n");
    core_vector_print_int(&self->vector);
    printf("\n");
#endif

    core_binary_heap_move_up(self, position);

    return TRUE;
}

void *core_binary_heap_get_key(struct core_binary_heap *self, int i)
{
    void *stored_key;

    stored_key = core_vector_at(&self->vector, i);

    return stored_key;
}

void *core_binary_heap_get_value(struct core_binary_heap *self, int i)
{
    void *stored_value;

    stored_value = ((char *)core_vector_at(&self->vector, i)) + self->key_size;

    return stored_value;
}

void core_binary_heap_move_up(struct core_binary_heap *self, int i)
{
    int parent;

    /*
     * It is the root already.
     */
    if (i == 0)
        return;

    while (1) {

        parent = core_binary_heap_get_parent(i);

        /*
         * This is already correct.
         *
         * With CORE_BINARY_HEAP_MIN: key(i) < key(parent).
         *
         * With CORE_BINARY_HEAP_MAX: key(i) > key(parent).
         *
         * If key(i) and key(parent) are equal, no change is required.
         */
        if (!core_binary_heap_test_relation(self, i, parent))
            break;

        /*
         * Otherwise, swap the entries and continue.
         */
        core_binary_heap_swap(self, parent, i);
        i = parent;
    }
}

int core_binary_heap_test_relation(struct core_binary_heap *self, int i, int j)
{
    void *key_i;
    void *key_j;

    key_i = core_binary_heap_get_key(self, i);
    key_j = core_binary_heap_get_key(self, j);

#ifdef HEAP_DEBUG_INSERT
    printf("DEBUG Test %d %d\n", i, j);
#endif

    return self->test_relation(self, key_i, key_j);
}

int core_binary_heap_test_relation_lower_than_void_pointer(struct core_binary_heap *self,
                void *key1, void *key2)
{
    return core_memory_compare(key1, key2, self->key_size) < 0;
}

static inline int core_binary_heap_get_first_child(int i)
{
    return 2 * i + 1;
}

static inline int core_binary_heap_get_second_child(int i)
{
    return 2 * i + 2;
}

static inline int core_binary_heap_get_parent(int i)
{
    return (i - 1) / 2;
}

int core_binary_heap_test_relation_lower_than_int(struct core_binary_heap *self,
                void *key1, void *key2)
{
    int a;
    int b;

    a = *(int *)key1;
    b = *(int *)key2;

#ifdef HEAP_DEBUG_INSERT
    printf("DEBUG lower_than_int %d %d\n", a, b);
#endif

    return a < b;
}

int core_binary_heap_test_relation_lower_than_uint64_t(struct core_binary_heap *self,
                void *key1, void *key2)
{
    uint64_t a;
    uint64_t b;

    a = *(uint64_t *)key1;
    b = *(uint64_t *)key2;

    return a < b;
}

int core_binary_heap_test_relation_greater_than_int(struct core_binary_heap *self,
                void *key1, void *key2)
{
    int a;
    int b;

    a = *(int *)key1;
    b = *(int *)key2;

    return a > b;
}

int core_binary_heap_test_relation_greater_than_uint64_t(struct core_binary_heap *self,
                void *key1, void *key2)
{
    uint64_t a;
    uint64_t b;

    a = *(uint64_t *)key1;
    b = *(uint64_t *)key2;

    return a > b;
}

int core_binary_heap_test_relation_greater_than_void_pointer(struct core_binary_heap *self,
                void *key1, void *key2)
{
    return core_memory_compare(key1, key2, self->key_size) > 0;
}

void core_binary_heap_swap(struct core_binary_heap *self, int i, int j)
{
    void *item_i;
    void *item_j;
    void *temporary_place;
    int pair_size;

    /*
     * Make space for a temporary place.
     */
    if (self->size + 1 > core_vector_size(&self->vector))
        core_vector_resize(&self->vector, self->size + 1);

    temporary_place = core_vector_at(&self->vector, self->size);
    item_i = core_vector_at(&self->vector, i);
    item_j = core_vector_at(&self->vector, j);

    pair_size = self->key_size + self->value_size;

    core_memory_copy(temporary_place, item_i, pair_size);
    core_memory_copy(item_i, item_j, pair_size);
    core_memory_copy(item_j, temporary_place, pair_size);
}

int core_binary_heap_size(struct core_binary_heap *self)
{
    return self->size;
}

int core_binary_heap_empty(struct core_binary_heap *self)
{
    return self->size == 0;
}

void core_binary_heap_move_down(struct core_binary_heap *self, int i)
{
    int left_child;
    int right_child;
    int selected_child;

    while (1) {
        left_child = core_binary_heap_get_first_child(i);
        right_child = core_binary_heap_get_second_child(i);

        /*
         * The node i is already at the good place with respect to
         * its children (which are possible not existent).
         */
        if ((!(left_child < self->size) || !core_binary_heap_test_relation(self, left_child, i))
            && (!(right_child < self->size) || !core_binary_heap_test_relation(self, right_child, i)))
            return;

        selected_child = -1;

        /*
         * At this point, some swap operations are required.
         *
         * There is at least 1 child.
         */
         if (left_child < self->size)
            selected_child = left_child;

        /*
         * There is no right child (and therefore there is a left child)
         * or the RELATION(left, right) is TRUE (left < right or left > right).
         */
        if (selected_child == -1
               || (right_child < self->size
                       && core_binary_heap_test_relation(self, left_child, right_child)))
            selected_child = right_child;

        /*
        */

        CORE_DEBUGGER_ASSERT(selected_child != -1);

#ifdef CORE_DEBUGGER_ASSERT_ENABLED
        if (!(selected_child < self->size)) {
            printf("selected_child = %d size = %d\n", selected_child,
                            self->size);
        }
#endif

        CORE_DEBUGGER_ASSERT(selected_child < self->size);
        CORE_DEBUGGER_ASSERT(selected_child >= 0);

        /*
         * Swap items and continue.
         */
        core_binary_heap_swap(self, i, selected_child);
        i = selected_child;
    }
}
