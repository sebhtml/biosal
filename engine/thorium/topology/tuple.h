
#ifndef THORIUM_TUPLE_H
#define THORIUM_TUPLE_H

#define THORIUM_MAXIMUM_DIAMETER 5

struct thorium_tuple {
    int values[THORIUM_MAXIMUM_DIAMETER];
};

void thorium_tuple_init(struct thorium_tuple *self, int radix, int diameter, int i);
void thorium_tuple_destroy(struct thorium_tuple *self);

void thorium_tuple_print(struct thorium_tuple *self, int radix, int diameter);
int thorium_tuple_get_base10_value(struct thorium_tuple *self, int radix, int diameter);

#endif
