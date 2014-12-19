
#include "tuple.h"
#include "polytope.h"

#include <stdio.h>

void thorium_tuple_init(struct thorium_tuple *self, int radix, int diameter, int i)
{
    /*
     * Based on the algorithm in
     * https://github.com/sebhtml/RayPlatform/blob/master/RayPlatform/routing/Polytope.cpp#L47
     */

    int power;
    int value;
    int result_power_plus_1;
    int result_power;

    for (power = 0; power < diameter; ++power) {

        result_power = thorium_polytope_get_power(radix, power);
        result_power_plus_1 = result_power * radix;

        value = (i % result_power_plus_1) / result_power;
        self->values[power] = value;
    }

#if 0
    printf("%d -> ", i);
    thorium_tuple_print(tuple, self->diameter);
#endif

}

void thorium_tuple_destroy(struct thorium_tuple *self)
{

}

void thorium_tuple_print(struct thorium_tuple *self, int radix, int diameter)
{
    int i;
    int value;
    int base10_value;

    printf("[");

    for (i = 0; i < diameter; ++i) {
        value = self->values[i];

        if (i != 0)
            printf(",%d", value);
        else
            printf("%d", value);

    }

    base10_value = thorium_tuple_get_base10_value(self, radix, diameter);

    printf("] (%d)\n", base10_value);
}

int thorium_tuple_get_base10_value(struct thorium_tuple *self, int radix, int diameter)
{
    int value = 0;
    int power;

    for (power = 0; power < diameter; ++power) {
        value += self->values[power] * thorium_polytope_get_power(radix, power);
    }

    return value;
}
