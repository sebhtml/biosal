
#ifndef THORIUM_POLYTOPE_H
#define THORIUM_POLYTOPE_H

#define TOPOLOGY_POLYTOPE 9

#include <core/structures/vector.h>

struct thorium_polytope {
    struct core_vector tuples;
    struct core_vector loads;
    int valid;
    int size;
    int radix;
    int diameter;
};

int thorium_polytope_get_power(int radix, int diameter);

void thorium_polytope_init(struct thorium_polytope *self, int size);
void thorium_polytope_destroy(struct thorium_polytope *self);

int thorium_polytope_get_next_rank_in_route(struct thorium_polytope *self, int source,
                int current, int destination);

#endif
