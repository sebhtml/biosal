
#include "tracer.h"

/* The Blue Gene/Q uses the GNU C Library
 */
#if defined(__GNUC__) || defined(__bgq__)

/*
 * GNU glibc has a strack backtrace generator built in
 * \see http://www.gnu.org/software/libc/manual/html_node/Backtraces.html
 */
#define BSAL_TRACER_AVAILABLE

#else

#warning "GNU backtrace is not available"

#endif

#ifdef BSAL_TRACER_AVAILABLE
#include <execinfo.h>
#endif

#include <core/system/memory.h>

#include <stdio.h>

#define BSAL_TRACER_STACK_DEPTH 100

void bsal_tracer_print_stack_backtrace()
{
    printf("bsal_tracer_print_stack_backtrace\n");

#ifdef BSAL_TRACER_AVAILABLE
    void* trace_pointers[BSAL_TRACER_STACK_DEPTH];
    int count;
    char** function_names;
    int i;

    count = backtrace(trace_pointers, BSAL_TRACER_STACK_DEPTH);

    function_names = backtrace_symbols(trace_pointers, count);

    printf("Stack backtrace has %d frames\n", count);

    /*
     * Print the stack trace
     */
    for (i = 0; i < count; i++) {
        printf("#%d  %s\n", i, function_names[i]);
    }

    /*
     * Free the string pointers
     */
    bsal_memory_free(function_names);

#else
    printf("TRACE IS NOT AVAILABLE.\n");
#endif

}
