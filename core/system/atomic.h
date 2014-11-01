
#ifndef CORE_ATOMIC_H
#define CORE_ATOMIC_H

#define USE_X86_PAUSE

#if defined(USE_X86_PAUSE) && defined(__GNUC__) &&  defined(__SSE2__)
#include <xmmintrin.h>
#endif

#define CORE_ATOMIC_HAS_COMPARE_AND_SWAP

/*
 *
 * \see http://stackoverflow.com/questions/5534145/is-gccs-atomic-test-and-set-builtin-the-same-as-an-atomic-fetch-and-store-opera
 * \see http://msdn.microsoft.com/en-us/library/z8785ft6%28v=vs.90%29.aspx
 * \see https://software.intel.com/en-us/blogs/2007/11/30/volatile-almost-useless-for-multi-threaded-programming
 * \see http://www-01.ibm.com/support/docview.wss?uid=swg27027065&aid=1
 * \see http://pic.dhe.ibm.com/infocenter/compbg/v121v141/index.jsp?topic=%2Fcom.ibm.xlcpp121.bg.doc%2Fcompiler_ref%2Fbifs_gcc_atomic_fetch_op.html
 * \see http://stackoverflow.com/questions/9363899/why-mark-function-argument-as-volatile
 */

/* IBM Blue Gene
 * XL compiler
 * \see http://pic.dhe.ibm.com/infocenter/compbg/v121v141/index.jsp?topic=%2Fcom.ibm.xlcpp121.bg.doc%2Fcompiler_ref%2Fbifs_gcc_atomic_fetch_op.html
 * int __lwarx (volatile int* addr);
 *
 * \see http://pic.dhe.ibm.com/infocenter/comphelp/v121v141/index.jsp?topic=%2Fcom.ibm.xlc121.aix.doc%2Fcompiler_ref%2Fbif_gcc_atomic_fetch_add.html
 */
#if defined(__bg__) || defined(__bgp__) || defined(__bgq__)

#define core_atomic_add(pointer, value) \
        __sync_fetch_and_add(pointer, value)

/*
 * \see http://pic.dhe.ibm.com/infocenter/compbg/v121v141/topic/com.ibm.xlcpp121.bg.doc/compiler_ref/bif_gcc_atomic_fetch_add.html
 */
/*
#define core_atomic_read_int(pointer) \
        __lwarx(pointer)
        */
#define core_atomic_read_int(pointer) \
        __sync_fetch_and_and (pointer, 1)
        /*__sync_fetch_and_and (pointer, 0xffffffff)*/

#define core_atomic_compare_and_swap_int(pointer, old_value, new_value) \
        __sync_bool_compare_and_swap(pointer, old_value, new_value)

/* \see http://docs.cray.com/cgi-bin/craydoc.cgi?mode=View;id=S-2179-74 */
#elif defined(_CRAYC)

#define core_atomic_add(pointer, value) \
        __sync_fetch_and_add(pointer, value)

/* Cray
 * \see https://fs.hlrs.de/projects/craydoc/docs_merged/man/xt_libintm/74/cat3/amo.3i.html
 */
#define core_atomic_read_int(pointer) \
        __sync_fetch_and_add (pointer, 0)

#define core_atomic_compare_and_swap_int(pointer, old_value, new_value) \
        __sync_bool_compare_and_swap(pointer, old_value, new_value)

/* Intel compiler
 * \see https://software.intel.com/en-us/forums/topic/281802
 * \see https://www.cs.fsu.edu/~engelen/courses/HPC-adv/intref_cls.pdf
#elif defined(__INTEL_COMPILER)

#define core_atomic_read_int(pointer) \

 * I did not found anything to read a 4-byte word atomically.
 */

#elif defined(__GNUC__)

#define core_atomic_add(pointer, value) \
        __sync_fetch_and_add(pointer, value)

/*#error "GNU !"*/
#define core_atomic_read_int(pointer) \
        __sync_fetch_and_add (pointer, 0)

#define core_atomic_compare_and_swap_int(pointer, old_value, new_value) \
        __sync_bool_compare_and_swap(pointer, old_value, new_value)

#else

/* no atomic built in is available
 */
#undef CORE_ATOMIC_HAS_COMPARE_AND_SWAP

#error "No atomic features found for this system (currently supported: XL compiler, GNU compiler, Cray compiler)."
#endif

#define core_atomic_increment(pointer) \
        core_atomic_add(pointer, 1)

/*
 * "NOP instruction can be between 0.4-0.5 clocks and PAUSE instruction can consume 38-40 clocks."
 *
 * \see http://stackoverflow.com/questions/7371869/minimum-time-a-thread-can-pause-in-linux
 * \see https://software.intel.com/en-us/forums/topic/309231
 */
static inline void core_atomic_spin()
{
#if defined(USE_X86_PAUSE) && defined(__GNUC__) && defined(__SSE2__)
     _mm_pause();

/*#warning "using SSE2 for PAUSE"*/
#else
     int i;

     i = 512;
     while (i--) {
     }
#endif
}

#endif /* CORE_ATOMIC_H */
