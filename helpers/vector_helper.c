
#include "vector_helper.h"

#include <structures/vector.h>

#include <stdlib.h>
#include <stdio.h>

int bsal_vector_helper_at_as_int(struct bsal_vector *self, int64_t index)
{
    int *bucket;

    bucket = (int *)bsal_vector_at(self, index);

    if (bucket == NULL) {
        return -1;
    }

    return *bucket;
}

char *bsal_vector_helper_at_as_char_pointer(struct bsal_vector *self, int64_t index)
{
    return (char *)bsal_vector_helper_at_as_void_pointer(self, index);
}

void *bsal_vector_helper_at_as_void_pointer(struct bsal_vector *self, int64_t index)
{
    void **bucket;

    bucket = (void **)bsal_vector_at(self, index);

    if (bucket == NULL) {
        return NULL;
    }

    return *bucket;
}

void bsal_vector_helper_print_int(struct bsal_vector *self)
{
    int64_t i;
    int64_t size;

    size = bsal_vector_size(self);
    i = 0;

    printf("[");
    while (i < size) {

        if (i > 0) {
            printf(", ");
        }
        printf("%d", bsal_vector_helper_at_as_int(self, i));
        i++;
    }
    printf("]");
}

void bsal_vector_helper_set_int(struct bsal_vector *self, int64_t index, int value)
{
    bsal_vector_set(self, index, &value);
}

void bsal_vector_helper_push_back_int(struct bsal_vector *self, int value)
{
    bsal_vector_push_back(self, &value);
}


