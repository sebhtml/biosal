
#ifndef CORE_FREE_LIST_H
#define CORE_FREE_LIST_H

#include <string.h>

struct core_free_list_element {
    struct core_free_list_element *next;
};

/*
 * A free list implementation
 *
 * \see http://en.wikipedia.org/wiki/Free_list
 *
 * Elements passed to core_free_list_add must be at least
 * sizeof(core_free_list_element) in size. This is typically
 * the same as sizeof(void *), which is 8 bytes (64 bits)
 * on modern systems.
 */
struct core_free_list {
    struct core_free_list_element *next;
    int size;
};

void core_free_list_init(struct core_free_list *self);
void core_free_list_destroy(struct core_free_list *self);

void core_free_list_add(struct core_free_list *self, void *element);
void *core_free_list_remove(struct core_free_list *self);

int core_free_list_empty(struct core_free_list *self);
int core_free_list_size(struct core_free_list *self);

/*
 * Check the size of elements (and correct it if necessary.
 */
size_t core_free_list_check_size(struct core_free_list *self, size_t size);

#endif
