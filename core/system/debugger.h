
#ifndef BSAL_DEBUGGER_H
#define BSAL_DEBUGGER_H

/*
 * Enable debug mode
 */
/*
#define BSAL_DEBUGGER_ENABLE_ASSERT
*/

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
        exit(1); \
    }

#else

/*
 * Do nothing
 */
#define BSAL_DEBUGGER_ASSERT(condition)

#endif
#endif
