
#ifndef CORE_ORDER_H
#define CORE_ORDER_H

/*
 * TODO: Investigate inlining this and having typesafe versions for each known case,
 *       e.g. int, char, etc.
 */

#define core_order_minimum(a, b) \
        ((a) < (b)) ? (a) : (b)

#endif
