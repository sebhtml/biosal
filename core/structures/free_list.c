
#include "free_list.h"

#include <core/system/debugger.h>

#include <stdlib.h>

void core_free_list_init(struct core_free_list *self)
{
    self->next = NULL;
    self->size = 0;
}

void core_free_list_destroy(struct core_free_list *self)
{
    self->next = NULL;
    self->size = 0;
}

void core_free_list_add(struct core_free_list *self, void *element)
{
    struct core_free_list_element *item;

    CORE_DEBUGGER_ASSERT_NOT_NULL(element);

    item = element;
    item->next = self->next;
    self->next = item;

    ++self->size;
}

void *core_free_list_remove(struct core_free_list *self)
{
    struct core_free_list_element *item;

    item = self->next;

    if (item != NULL) {
        --self->size;
        self->next = self->next->next;
    }

    return item;
}

int core_free_list_empty(struct core_free_list *self)
{
    return self->size == 0;
}

int core_free_list_size(struct core_free_list *self)
{
    return self->size;
}

size_t core_free_list_check_size(struct core_free_list *self, size_t size)
{
    size_t minimum;

    minimum = sizeof(struct core_free_list_element);

    if (size < minimum)
        return minimum;
    return size;
}
