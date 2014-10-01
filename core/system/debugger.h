
#ifndef BIOSAL_DEBUGGER_H
#define BIOSAL_DEBUGGER_H

/*
 * Enable debug mode.
 * Parameters for debugging are below.
 */

#ifdef THORIUM_DEBUG

#define BIOSAL_DEBUGGER_ENABLE_ASSERT

/*
#define BIOSAL_DEBUGGER_CHECK_DOUBLE_FREE_IN_POOL
*/

/*
 * Report memory leaks in memory pool.
 */
#define BIOSAL_DEBUGGER_CHECK_LEAKS_IN_POOL

/*
 * Enable leak detection using
 * BIOSAL_DEBUGGER_LEAK_DETECTION_BEGIN and BIOSAL_DEBUGGER_LEAK_DETECTION_END
 */
#define BIOSAL_DEBUGGER_ENABLE_LEAK_DETECTION

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

#define BIOSAL_DEBUG_MARKER(marker) \
        printf("biosal> BIOSAL_DEBUG_MARKER File: %s Line: %d Function %s: Marker: %s\n", __FILE__, __LINE__, __func__, marker);

#ifdef BIOSAL_DEBUGGER_ENABLE_ASSERT

#define BIOSAL_DEBUGGER_ASSERT(condition) \
    if (!(condition)) { \
        BIOSAL_DEBUG_MARKER("BIOSAL_DEBUGGER_ASSERT catched a bug !"); \
        biosal_tracer_print_stack_backtrace(); \
        fflush(stdout); \
        fflush(stderr); \
        exit(1); \
    }

#define BIOSAL_DEBUGGER_ASSERT_IS_EQUAL_INT(actual, expected) \
    if (!((actual) == (expected))) { \
        BIOSAL_DEBUG_MARKER("BIOSAL_DEBUGGER_ASSERT catched a bug !"); \
        printf("Actual %d Expected %d\n", actual, expected); \
        biosal_tracer_print_stack_backtrace(); \
        fflush(stdout); \
        fflush(stderr); \
        exit(1); \
    }

#else

/*
 * Do nothing
 */
#define BIOSAL_DEBUGGER_ASSERT(condition)

#define BIOSAL_DEBUGGER_ASSERT_IS_EQUAL_INT(actual, expected)

#endif

void biosal_debugger_examine(void *pointer, int bytes);

#ifdef BIOSAL_DEBUGGER_ENABLE_LEAK_DETECTION

#define BIOSAL_DEBUGGER_LEAK_DETECTION_BEGIN(pool, state) \
    struct biosal_memory_pool_state state; \
    biosal_memory_pool_begin(pool, &state);

#define BIOSAL_DEBUGGER_LEAK_DETECTION_END(pool, state) \
    biosal_memory_pool_end(pool, &state, # state, __func__, __FILE__, __LINE__);

#define BIOSAL_DEBUGGER_LEAK_CHECK_DOUBLE_FREE(pool) \
    biosal_memory_pool_check_double_free(pool, __func__, __FILE__, __LINE__);

#else

#define BIOSAL_DEBUGGER_LEAK_DETECTION_BEGIN(pool, state)
#define BIOSAL_DEBUGGER_LEAK_DETECTION_END(pool, state)
#define BIOSAL_DEBUGGER_LEAK_CHECK_DOUBLE_FREE(pool)

#endif

#define BIOSAL_DEBUGGER_ASSERT_NOT_NULL(pointer) \
        BIOSAL_DEBUGGER_ASSERT(pointer != NULL)
#endif
