
#ifndef __SEND_MACROS
#define __SEND_MACROS

/*
 * This file defines a bunch of macros to simplify the sending of messages
 * with a common function macro called SEND().
 */

#define TYPE_INT
#undef TYPE_INT
#define TYPE_VECTOR
#undef TYPE_VECTOR
#define TYPE_MESSAGE
#undef TYPE_MESSAGE

#include <engine/thorium/modules/send_helpers.h>
#include <engine/thorium/modules/then.h>

#define ACTION_GENERATED_WITH_MACROS 0x0026dcb8

#define __ACTION ACTION_GENERATED_WITH_MACROS

#define REPLY(arity, ...) \
        TELL(arity, thorium_actor_source(self), __VA_ARGS__)

/*
 * SEND(destination, type, value)
 */

#define TELL(arity, ...) \
        __TELL ## _ ## arity(__VA_ARGS__)

#define ASK(arity, ...) \
        __ASK ## _ ## arity(__VA_ARGS__)

/*
        */


/*
 * Based on
 * http://stackoverflow.com/questions/11632219/c-preprocessor-macro-specialisation-based-on-an-argument
 */

#define __GET_SEND_NAME(arity, ...) \
        __GET_SEND_NAME_ ## arity(__VA_ARGS__)

#define __GET_SEND_NAME_1(type1) \
        __SEND_ ## 1 ## _ ## type1

#define __GET_SEND_NAME_2(type1, type2) \
        __SEND_ ## 2 ## _ ## type1 ## _ ## type2

#define __SEND_1_TYPE_INT(destination, action, type1, value1) \
        thorium_actor_send_int(self, destination, action, value1)

#define __SEND_1_TYPE_VECTOR(destination, action, type1, value1) \
        thorium_actor_send_vector(self, destination, action, value1)

#define __SEND_2_TYPE_INT_TYPE_VECTOR(destination, action, type1, value1, type2, value2) \
        thorium_actor_send_int_vector(self, destination, action, value1, value2)

/*
 * Depending on type1, dispatch to
 * SEND3_TYPE_INTEGER
 * or
 * SEND3_TYPE_VECTOR.
 */

#define __TELL_MSG(destination, message) \
        thorium_actor_send(self, destination, message)

#define __TELL_0(destination, action) \
        thorium_actor_send_empty(self, destination, action)

#define __TELL_1(destination, action, type1, value1) \
        __GET_SEND_NAME(1, type1)(destination, action, type1, value1)

#define __TELL_2(destination, action, type1, value1, type2, value2) \
        __GET_SEND_NAME(2, type1, type2)(destination, action, type1, value1, type2, value2)


#define __ASK_MSG(destination, message, callback) \
        thorium_actor_send_then(self, destination, message, callback)


#endif /* __SEND_MACROS */

