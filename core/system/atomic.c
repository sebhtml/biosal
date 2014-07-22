
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
    bsal_fence();
}

void bsal_l_fence()
{
#if defined(__bgq__)

    /* I am not sure if  __eieio  is a load fence */
    bsal_fence();

#elif defined(__GNUC__)

    bsal_fence();

#elif defined(_CRAYC)
    __builtin_ia32_lfence();

#else

    bsal_fence();

#endif

}

void bsal_s_fence()
{
#if defined(__bgq__)
    __lwsync();

#elif defined(__GNUC__)

    bsal_fence();

#elif defined(_CRAYC)
    __builtin_ia32_sfence();

#else

    bsal_fence();

#endif
}

void bsal_fence()
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

    /*
     * \see https://gcc.gnu.org/onlinedocs/gcc-4.4.5/gcc/Atomic-Builtins.html
     */
    __sync_synchronize();


#elif defined(_CRAYC)

    /*
     * \see http://docs.cray.com/books/004-2179-001/html-004-2179-001/imwlrwh.html
     */
    /*
    _memory_barrier();
    */

    /*
     * \see http://docs.cray.com/books/S-2179-52/html-S-2179-52/fixedcdw3qe3i7.html
     * _gsync();
     *
     */

    /*
     * \see http://docs.cray.com/books/S-2179-81/S-2179-81.pdf
     */
    __builtin_ia32_mfence();

#else

    /* Do nothing... */
#endif
}
