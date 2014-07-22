
#include "atomic.h"

/*
 * These are mock implementations that don't work
 */

int bsal_atomic_read_int_mock(int *pointer)
{
    return *pointer;
}

int bsal_atomic_compare_and_swap_int_mock(int *pointer, int old_value, int new_value)
{
    if (*pointer != old_value) {
        return *pointer;
    }

    *pointer = new_value;

    return old_value;
}

void bsal_memory_fence()
{
#ifdef __bgq__

    /*
     * \see http://publib.boulder.ibm.com/infocenter/cellcomp/v101v121/index.jsp?topic=/com.ibm.xlcpp101.cell.doc/compiler_ref/compiler_builtins.html
     */
    __sync();

#elif defined(__GNUC__)

    /*
     * \see http://stackoverflow.com/questions/982129/what-does-sync-synchronize-do
     */
    /* on x86, write are not re-ordered according to
     * http://www.xml.com/ldd/chapter/book/ch08.html
     *
     * For example, on the x86 architecture, wmb() currently does nothing, since
     * writes outside the processor are not reordered. Reads are reordered, however, so mb() will be slower than wmb().
     */

    __sync_synchronize();
#endif
}
