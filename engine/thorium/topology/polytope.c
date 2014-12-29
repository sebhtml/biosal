
#include "polytope.h"
#include "tuple.h"

#include <core/system/debugger.h>

#include <engine/thorium/thorium_engine.h>

#include <stdio.h>

void thorium_polytope_generate_tuples(struct thorium_polytope *self);

static int *get_load_bucket(struct thorium_polytope *self, int position, int value);
static void set_load(struct thorium_polytope *self, int position, int value, int load);
static int get_load(struct thorium_polytope *self, int position, int value);

void thorium_polytope_get_tuple(struct thorium_polytope *self,
        int i, struct thorium_tuple *tuple);

void thorium_polytope_initialize_loads(struct thorium_polytope *self);

void thorium_polytope_init(struct thorium_polytope *self, int size)
{
    core_vector_init(&self->tuples, sizeof(struct thorium_tuple));
    core_vector_init(&self->loads, sizeof(int));
    self->size = size;
    self->valid = 0;

    thorium_polytope_generate_tuples(self);

    thorium_polytope_initialize_loads(self);
}

void thorium_polytope_destroy(struct thorium_polytope *self)
{
    core_vector_destroy(&self->tuples);
    core_vector_destroy(&self->loads);
}

int thorium_polytope_get_next_rank_in_route(struct thorium_polytope *self, int source,
                int current, int destination)
{
    struct thorium_tuple *current_tuple;
    struct thorium_tuple next_tuple;
    struct thorium_tuple *destination_tuple;
    int position_in_tuple;
    int desired_value;
    int actual_value;
    int best_position;
    int load;
    int best_load;
    int next;
    int best_value = -1;

#ifdef VERBOSE_POLYTOPE
    printf("get_next source %d current %d destination %d\n",
                    source, current, destination);
#endif

    CORE_DEBUGGER_ASSERT(self->valid);

    if (!self->valid)
        return -1;

    /*
     * We are already at the destination...
     */
    if (current == destination)
        return destination;

    current_tuple = core_vector_at(&self->tuples, current);
    destination_tuple = core_vector_at(&self->tuples, destination);

    best_position = -1;
    best_load = -1;

    /*
     * source -> ... -> current -> next -> ... -> destination
     */

    for (position_in_tuple = 0; position_in_tuple < self->diameter; ++position_in_tuple) {

        actual_value = current_tuple->values[position_in_tuple];
        desired_value = destination_tuple->values[position_in_tuple];

        /*
         * This position in the tuple is already set to the
         * correct value.
         */
        if (actual_value == desired_value)
            continue;

        /*
         * Store the load.
         */
        load = get_load(self, position_in_tuple, desired_value);

        if (best_load == -1 || load > best_load) {
            best_position = position_in_tuple;
            best_load = load;
        }
    }

    next_tuple = *current_tuple;
    best_value = destination_tuple->values[best_position];
    next_tuple.values[best_position] = best_value;

#ifdef VERBOSE_POLYTOPE
    printf("Route\n");
    printf("source %d\n", source);
    printf("current ");
    thorium_tuple_print(current_tuple, self->radix, self->diameter);
    printf("next ");
    thorium_tuple_print(&next_tuple, self->radix, self->diameter);
    printf("...\n");
    printf("destination ");
    thorium_tuple_print(destination_tuple, self->radix, self->diameter);

    printf("best position %d value %d load %d\n",
                    best_position, best_value, best_load);
#endif

    next = thorium_tuple_get_base10_value(&next_tuple, self->radix, self->diameter);

    set_load(self, best_position, best_value, best_load + 1);

    return next;
}

void thorium_polytope_generate_tuples(struct thorium_polytope *self)
{
    int diameter;
    int radix;
    int size;
    int result;
    int found = 0;
    int i;
    struct thorium_tuple *tuple;

    size = self->size;
    result = -1;

    /*
     * Find a diameter.
     */
    for (diameter = 2; diameter <= THORIUM_MAXIMUM_DIAMETER; ++diameter) {
        for (radix = 2; radix <= 64; ++radix) {
            result = thorium_polytope_get_power(radix, diameter);

            if (result == size) {
                found = 1;
                break;
            }
        }

        if (found)
            break;
    }

    if (!found)
        return;

    thorium_printf("[thorium] Found polytope geometry: size %d diameter %d radix %d\n",
                                size, diameter, radix);
#ifdef VERBOSE_POLYTOPE
#endif

    self->diameter = diameter;
    self->radix = radix;

    core_vector_resize(&self->tuples, self->size);

    for (i = 0; i < size; ++i) {
        tuple = core_vector_at(&self->tuples, i);

        thorium_polytope_get_tuple(self, i, tuple);
    }

    self->valid = 1;
}

int thorium_polytope_get_power(int radix, int diameter)
{
    int value = 1;

    while (diameter--) {
        value *= radix;
    }

    return value;
}

void thorium_polytope_get_tuple(struct thorium_polytope *self,
        int i, struct thorium_tuple *tuple)
{
    thorium_tuple_init(tuple, self->radix, self->diameter, i);
}

static int get_load(struct thorium_polytope *self, int position, int value)
{
    int *bucket;
    bucket = get_load_bucket(self, position, value);
    return *bucket;
}

static void set_load(struct thorium_polytope *self, int position, int value, int load)
{
    int *bucket;
    bucket = get_load_bucket(self, position, value);
    *bucket = load;
}

static int *get_load_bucket(struct thorium_polytope *self, int position, int value)
{
    int index;
    int *bucket;
    index = position * self->radix + value;
    bucket = core_vector_at(&self->loads, index);
    return bucket;
}

void thorium_polytope_initialize_loads(struct thorium_polytope *self)
{
    /*
     * Allocate space for load values.
     */

    int position;
    int value;

    if (!self->valid)
        return;

    core_vector_resize(&self->loads, self->diameter * self->radix);

    for (position = 0; position < self->diameter; ++position) {
        for (value = 0; value < self->radix; ++value) {
            set_load(self, position, value, 0);
        }
    }
}
