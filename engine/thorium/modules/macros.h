
#ifndef __MACROS_H
#define __MACROS_H

#include <core/helpers/integer.h>

#define UNPACK(arity, ...) \
    UNPACK_ ## arity(__VA_ARGS__)

#define UNPACK_1(type1, object1) \
    UNPACK_1_ ## type1(object1)

#define UNPACK_1_TYPE_INT(object1) \
       core_int_unpack(&object1, BUFFER(message))


/* Boilerplate code
 */

#define ACTOR_BOILERPLATE(actor_class) \
    struct actor_class *concrete_self = thorium_actor_concrete_actor(self)

#endif /* __MACROS_H */
