
#ifndef CORE_ATOMIC_H
#define CORE_ATOMIC_H

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
 */
#if defined(__bg__) || defined(__bgp__) || defined(__bgq__)

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

/*#error "GNU !"*/
#define core_atomic_read_int(pointer) \
        __sync_fetch_and_add (pointer, 0)

#define core_atomic_compare_and_swap_int(pointer, old_value, new_value) \
        __sync_bool_compare_and_swap(pointer, old_value, new_value)

#else

/* no atomic built in is available
 */
#undef CORE_ATOMIC_HAS_COMPARE_AND_SWAP

/* Otherwise, just return the value
 */
#define core_atomic_read_int(pointer) \
        core_atomic_read_int_mock(pointer)

#define core_atomic_compare_and_swap_int(pointer, old_value, new_value) \
        core_atomic_compare_and_swap_int_mock(pointer, old_value, new_value)

#warning "No atomic features found for this system"
#endif

int core_atomic_read_int_mock(int *pointer);
int core_atomic_compare_and_swap_int_mock(int *pointer, int old_value, int new_value);

#endif
