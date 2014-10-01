
#ifndef CORE_DEBUGGER_H
#define CORE_DEBUGGER_H

/*
 * Enable debug mode.
 * Parameters for debugging are below.
 */

#ifdef THORIUM_DEBUG

#define CORE_DEBUGGER_ENABLE_ASSERT

/*
#define CORE_DEBUGGER_CHECK_DOUBLE_FREE_IN_POOL
*/

/*
 * Report memory leaks in memory pool.
 */
#define CORE_DEBUGGER_CHECK_LEAKS_IN_POOL

/*
 * Enable leak detection using
 * CORE_DEBUGGER_LEAK_DETECTION_BEGIN and CORE_DEBUGGER_LEAK_DETECTION_END
 */
#define CORE_DEBUGGER_ENABLE_LEAK_DETECTION

/*
 * Enable event counter for injections
 */

/*
 * Enable verification of injections.
#define THORIUM_WORKER_DEBUG_INJECTION
 */

#endif

#include "tracer.h"

#include <stdio.h>
#include <stdlib.h>

/*
 */

#define CORE_DEBUG_MARKER(marker) \
        printf("biosal> CORE_DEBUG_MARKER File: %s Line: %d Function %s: Marker: %s\n", __FILE__, __LINE__, __func__, marker);

#ifdef CORE_DEBUGGER_ENABLE_ASSERT

#define CORE_DEBUGGER_ASSERT(condition) \
    if (!(condition)) { \
        CORE_DEBUG_MARKER("CORE_DEBUGGER_ASSERT catched a bug !"); \
        core_tracer_print_stack_backtrace(); \
        fflush(stdout); \
        fflush(stderr); \
        exit(1); \
    }

#define CORE_DEBUGGER_ASSERT_IS_EQUAL_INT(actual, expected) \
    if (!((actual) == (expected))) { \
        CORE_DEBUG_MARKER("CORE_DEBUGGER_ASSERT catched a bug !"); \
        printf("Actual %d Expected %d\n", actual, expected); \
        core_tracer_print_stack_backtrace(); \
        fflush(stdout); \
        fflush(stderr); \
        exit(1); \
    }

#else

/*
 * Do nothing
 */
#define CORE_DEBUGGER_ASSERT(condition)

#define CORE_DEBUGGER_ASSERT_IS_EQUAL_INT(actual, expected)

#endif

void core_debugger_examine(void *pointer, int bytes);

#ifdef CORE_DEBUGGER_ENABLE_LEAK_DETECTION

#define CORE_DEBUGGER_LEAK_DETECTION_BEGIN(pool, state) \
    struct core_memory_pool_state state; \
    core_memory_pool_begin(pool, &state);

#define CORE_DEBUGGER_LEAK_DETECTION_END(pool, state) \
    core_memory_pool_end(pool, &state, # state, __func__, __FILE__, __LINE__);

#define CORE_DEBUGGER_LEAK_CHECK_DOUBLE_FREE(pool) \
    core_memory_pool_check_double_free(pool, __func__, __FILE__, __LINE__);

#else

#define CORE_DEBUGGER_LEAK_DETECTION_BEGIN(pool, state)
#define CORE_DEBUGGER_LEAK_DETECTION_END(pool, state)
#define CORE_DEBUGGER_LEAK_CHECK_DOUBLE_FREE(pool)

#endif

#define CORE_DEBUGGER_ASSERT_NOT_NULL(pointer) \
        CORE_DEBUGGER_ASSERT(pointer != NULL)
#endif
