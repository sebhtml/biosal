
#ifndef BSAL_DEBUGGER_H
#define BSAL_DEBUGGER_H

/*
 * Enable debug mode.
 * Parameters for debugging are below.
 */

#ifdef THORIUM_DEBUG

#define BSAL_DEBUGGER_ENABLE_ASSERT

/*
#define BSAL_DEBUGGER_CHECK_DOUBLE_FREE_IN_POOL
*/

/*
 * Report memory leaks in memory pool.
 */
#define BSAL_DEBUGGER_CHECK_LEAKS_IN_POOL

/*
 * Enable leak detection using
 * BSAL_DEBUGGER_LEAK_DETECTION_BEGIN and BSAL_DEBUGGER_LEAK_DETECTION_END
 */
#define BSAL_DEBUGGER_ENABLE_LEAK_DETECTION

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

#define BSAL_DEBUG_MARKER(marker) \
        printf("biosal> BSAL_DEBUG_MARKER File: %s Line: %d Function %s: Marker: %s\n", __FILE__, __LINE__, __func__, marker);

#ifdef BSAL_DEBUGGER_ENABLE_ASSERT

#define BSAL_DEBUGGER_ASSERT(condition) \
    if (!(condition)) { \
        BSAL_DEBUG_MARKER("BSAL_DEBUGGER_ASSERT catched a bug !"); \
        bsal_tracer_print_stack_backtrace(); \
        fflush(stdout); \
        fflush(stderr); \
        exit(1); \
    }

#define BSAL_DEBUGGER_ASSERT_IS_EQUAL_INT(actual, expected) \
    if (!((actual) == (expected))) { \
        BSAL_DEBUG_MARKER("BSAL_DEBUGGER_ASSERT catched a bug !"); \
        printf("Actual %d Expected %d\n", actual, expected); \
        bsal_tracer_print_stack_backtrace(); \
        fflush(stdout); \
        fflush(stderr); \
        exit(1); \
    }

#else

/*
 * Do nothing
 */
#define BSAL_DEBUGGER_ASSERT(condition)

#define BSAL_DEBUGGER_ASSERT_IS_EQUAL_INT(actual, expected)

#endif

void bsal_debugger_examine(void *pointer, int bytes);

#ifdef BSAL_DEBUGGER_ENABLE_LEAK_DETECTION

#define BSAL_DEBUGGER_LEAK_DETECTION_BEGIN(pool, state) \
    struct bsal_memory_pool_state state; \
    bsal_memory_pool_begin(pool, &state);

#define BSAL_DEBUGGER_LEAK_DETECTION_END(pool, state) \
    bsal_memory_pool_end(pool, &state, # state, __func__, __FILE__, __LINE__);

#define BSAL_DEBUGGER_LEAK_CHECK_DOUBLE_FREE(pool) \
    bsal_memory_pool_check_double_free(pool, __func__, __FILE__, __LINE__);

#else

#define BSAL_DEBUGGER_LEAK_DETECTION_BEGIN(pool, state)
#define BSAL_DEBUGGER_LEAK_DETECTION_END(pool, state)
#define BSAL_DEBUGGER_LEAK_CHECK_DOUBLE_FREE(pool)

#endif

#endif
