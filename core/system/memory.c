
#include "memory.h"

#include "tracer.h"
#include "debugger.h"

#include <engine/thorium/node.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <inttypes.h>
#include <stdint.h>

#ifdef CONFIG_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

/*
 * bound memory allocations in order
 * to detect provided negative numbers
 * size_t value of 18446744073709551615 corresponds to int value -1)
 *
 */

/*
 * Use System Programming Interface on the IBM Blue Gene/Q to get memory usage.
 */
#ifdef __bgq__
#include <spi/include/kernel/memory.h>
#endif

#ifdef __bgq__
#include <malloc.h> /* for mallopt */
#endif

#define FAST_MEMORY

/*
 * \see http://linux.die.net/man/3/jemalloc
 */
void *wrapper_malloc(size_t size);
void wrapper_free(void *pointer);

void *core_memory_allocate_private(size_t size, const char *function, const char *file, int line, int key)
{
    void *pointer;

    /*
    printf("DEBUG core_memory_allocate size: %zu (as int: %d)\n", size, (int)size);
    */
    pointer = NULL;

    if (size < CORE_MEMORY_MINIMUM) {
        printf("DEBUG Error core_memory_allocate received a number below the minimum: %zu bytes\n", size);

        if (file != NULL) {
            printf("CORE_MEMORY_DEBUG core_memory_allocate %d bytes %p %s %s %d\n",
                    (int)size, pointer, function, file, line);
        }
        core_tracer_print_stack_backtrace();
        core_exit_with_error();
    }

    if (size > CORE_MEMORY_MAXIMUM) {
        printf("DEBUG Error core_memory_allocate received a number above the maximum: %zu bytes (int value: %d)\n", size,
                        (int)size);
        if (file != NULL) {
            printf("CORE_MEMORY_DEBUG core_memory_allocate %d bytes %p %s %s %d\n",
                    (int)size, pointer, function, file, line);
        }
        core_tracer_print_stack_backtrace();
        core_exit_with_error();
    }

    CORE_DEBUGGER_ASSERT(size <= CORE_MEMORY_MAXIMUM);

#ifdef CORE_MEMORY_DEBUG_TRACK_TARGET
    char target[] = "core_vector_reserve";
    if (strcmp(function, target) == 0) {
        printf("TRACER: call to core_memory_allocate in %s\n", function);
        core_tracer_print_stack_backtrace();
    }
#endif

#if 0
    printf("Size is %d\n", (int)size);
#endif
    pointer = wrapper_malloc(size);

#ifdef CORE_MEMORY_DEBUG

    /*
     * 8 24 32 64 320 1280 2560
     */
    if ((DEBUG_KEY == DEBUG_ANY || key == (int)DEBUG_KEY)
                            && (DEBUG_SIZE == DEBUG_ANY || size == DEBUG_SIZE)) {
        printf("TRACEPOINT_EVENT memory:allocate size= %d pointer= %p function= %s file= %s line= %d key= 0x%x\n",
                    (int)size, pointer, function, file, line, key);

#if 1
    /*
     * Ask the tracer to print a stack
     */
        if (DEBUG_KEY != DEBUG_ANY && DEBUG_SIZE != DEBUG_ANY)
            core_tracer_print_stack_backtrace();
    }
#endif

#endif

    /*
     * On Linux, this does not happen.
     *
     * \see http://www.win.tue.nl/~aeb/linux/lk/lk-9.html
     * \see http://stackoverflow.com/questions/16674370/why-does-malloc-or-new-never-return-null
     */
    if (pointer == NULL) {
        printf("DEBUG Error core_memory_allocate returned %p, %zu bytes\n", pointer, size);
        printf("used / total -> %" PRIu64 " / %" PRIu64  "\n",
                        core_memory_get_utilized_byte_count(),
                        core_memory_get_total_byte_count());

        thorium_node_examine(thorium_node_global_self);

        core_tracer_print_stack_backtrace();

        /*
         * Flush file descriptos.
         * This is required on Blue Gene/Q, otherwise information
         * sometimes vanish.
         */
        fflush(stdout);
        fflush(stderr);

        core_exit_with_error();
    }

    return pointer;
}

void core_memory_free_private(void *pointer, const char *function, const char *file, int line, int key)
{
#ifdef CORE_MEMORY_DEBUG
    if (DEBUG_KEY == DEBUG_ANY || key == (int)DEBUG_KEY) {
        printf("TRACEPOINT_EVENT memory:free pointer= %p function= %s file= %s line= %d key= 0x%x\n",
                   pointer, function, file, line, key);
    }
#endif

    if (pointer == NULL) {
        return;
    }

#ifdef CORE_MEMORY_DEBUG
#endif

    wrapper_free(pointer);

    /*
     * Nothing else to do.
     */
}

uint64_t core_memory_get_total_byte_count()
{
#ifdef __bgq__
    uint64_t total_memory;
    uint64_t memory_for_kernel;
    uint64_t memory_for_application;

    /*
     * Each BGQ node has 16 GiB RAM and CNK uses 16 MiB.
     */
    total_memory = 17179869184;
    memory_for_kernel = 16777216;
    memory_for_application = total_memory - memory_for_kernel;

    /*
     * Example:
     * 16991182848 used
     * 17179869184 total
     */

    return memory_for_application;

#elif defined(__linux__)

    /*
     * use /proc/meminfo
     *
     * [seb@localhost ~]$ head -n 1 /proc/meminfo
     * MemTotal:        8168536 kB
     *
     */

    char buffer[16];
    uint64_t total_memory;
    FILE *file;
    file = fopen("/proc/meminfo", "r");

    /*
     * Fake a big memory byte count.
     */
    total_memory = 0;
    --total_memory;

    while (fscanf(file, "%s", buffer) > 0) {
        if (strcmp(buffer, "MemTotal:") == 0) {
            fscanf(file, "%" PRIu64, &total_memory);

            /*
             * Convert KiB to B
             */
            total_memory *= 1024;
            break;
        }
    }

    fclose(file);

    return total_memory;

#else
    uint64_t total;

    /*
     * Otherwise, this hint is currently unsupported.
     * This is obviously broken.
     */

    total = 18446744073709551615UL;
    return total;
#endif
}

uint64_t core_memory_get_remaining_byte_count()
{
    uint64_t total;
    uint64_t utilized;
    uint64_t remaining;

    total = core_memory_get_total_byte_count();
    utilized = core_memory_get_utilized_byte_count();
    remaining = total - utilized;

    return remaining;
}

int core_memory_has_enough_bytes()
{
    uint64_t remaining;
    uint64_t threshold;

    remaining = core_memory_get_remaining_byte_count();
    threshold = 536870912;

    if (remaining > threshold) {
        return 1;
    }

    return 0;
}

uint64_t core_memory_get_utilized_byte_count()
{
    uint64_t bytes;
    bytes = 0;

#if defined(__bgq__)
    Kernel_GetMemorySize(KERNEL_MEMSIZE_HEAP,&bytes);

#elif defined(__linux__)

    FILE *descriptor;
    char buffer [1024];
    int heap_size;
    int expected;
    int actual;

    expected = 1;
    descriptor = fopen("/proc/self/status", "r");

    while (!feof(descriptor)) {
        actual = fscanf(descriptor, "%s", buffer);

        if (actual == expected
                        && strcmp(buffer, "VmData:") == 0) {
            actual = fscanf(descriptor, "%d", &heap_size);

#ifdef CORE_MEMORY_DEBUG_HEAP
            printf("Scanned %d\n", heap_size);
#endif

            if (actual == expected) {
                bytes = (uint64_t)heap_size * 1024;
            }
            break;
        }
    }

    fclose(descriptor);
#endif

    return bytes;
}

size_t core_memory_align(size_t unaligned)
{
/*
 * On IBM Blue Gene/Q, use the configured alignment.
 * Otherwise, we don't care really.
 *
 * Align on 128 bytes on Blue Gene/Q.
 */
#if defined(__bgq__)
    return core_memory_align_private(unaligned, CORE_MEMORY_ALIGNMENT_BLUE_GENE_Q_L2_CACHE_LINE_SIZE);

#else
 /*
  * Align on 64 bytes on Intel and AMD.
  */
    return core_memory_align_private(unaligned, CORE_MEMORY_ALIGNMENT_INTEL_L3_CACHE_LINE_SIZE);
#endif
}

size_t core_memory_align_private(size_t unaligned, size_t alignment)
{
    size_t aligned;

    /*
     * Nothing to align, really.
     */
    if (alignment == 0) {
        return unaligned;
    }

    /*
     * Already aligned.
     */
    if (unaligned % alignment == 0) {
        return unaligned;
    }

    /*
     * We don't want to align 0 byte.
     */
    CORE_DEBUGGER_ASSERT(unaligned != 0);

    if (unaligned == 0) {
        return unaligned;
    }

    aligned = unaligned + (alignment - (unaligned % alignment));

#ifdef CORE_DNA_KMER_DEBUG_ALIGNMENT
    printf("ALIGNMENT %zu unaligned %zu aligned %zu\n",
                    alignment, unaligned, aligned);
#endif

    CORE_DEBUGGER_ASSERT(aligned >= CORE_MEMORY_MINIMUM);
    CORE_DEBUGGER_ASSERT(aligned <= CORE_MEMORY_MAXIMUM);

    return aligned;
}

size_t core_memory_normalize_segment_length_power_of_2(size_t size)
{
    uint32_t value;
    size_t maximum;
    size_t return_value;

    /*
     * Pick up the next power of 2.
     */

    /*
     * Check if the value fits in 32 bits.
     */
    value = 0;
    value--;

    maximum = value;

    if (size > maximum) {
        /*
         * Use a manual approach for things that don't fit in a uint32_t.
         */

        return_value = 1;

        while (return_value < size) {
            return_value *= 2;
        }

        return return_value;
    }

    /*
     * Otherwise, use the fancy algorithm.
     * The algorithm is from http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
     */

    value = size;

    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;

    return_value = value;

    return value;
}

size_t core_memory_normalize_segment_length_page_size(size_t size)
{
    size_t page_size;
    size_t remainder;
    size_t padding;

    page_size = sysconf(_SC_PAGE_SIZE);

    remainder = size % page_size;
    padding = page_size - remainder;

    return size + padding;
}

void *core_memory_copy(void *destination, const void *source, size_t count)
{
#ifdef CORE_DEBUGGER_ASSERT
    if (destination == NULL)
        printf("Error destination is NULL\n");
    if (source == NULL)
        printf("Error source is NULL\n");
    if (count < 1)
        printf("Error count is %zu\n", count);
#endif

    CORE_DEBUGGER_ASSERT(destination != NULL);
    CORE_DEBUGGER_ASSERT(source != NULL);
    CORE_DEBUGGER_ASSERT(count > 0);

#ifdef FAST_MEMORY
    return memcpy(destination, source, count);
#else
    size_t i;
    char *destination_buffer;
    char *source_buffer;

    destination_buffer = destination;
    source_buffer = source;

    for (i = 0; i < count; ++i)
        destination_buffer[i] = source_buffer[i];

    return destination;
#endif
}

void *core_memory_move(void *destination, const void *source, size_t count)
{
    CORE_DEBUGGER_ASSERT(destination != NULL);
    CORE_DEBUGGER_ASSERT(source != NULL);
    CORE_DEBUGGER_ASSERT(count > 0);

#ifdef FAST_MEMORY
    return memmove(destination, source, count);
#else
    return memmove(destination, source, count);
#endif
}

void core_memory_load_fence()
{
#ifdef LOAD_OPERATIONS_ARE_ORDERED_disabled

#elif defined(__GNUC__)

    __sync_synchronize();

#elif defined(__bgq__)

    /*
     * The macros with XL is:
     *
     * _ARCH_PPC
     * _ARCH_PPC64
     *
     * With GNU, it is __powerpc64__.
     */
    /* I am not sure if  __eieio  is a load fence */
    core_memory_fence();

#elif defined(_CRAYC)
    __builtin_ia32_lfence();

#else

    core_memory_fence();

#endif
}

void core_memory_store_fence()
{
#ifdef STORE_OPERATIONS_ARE_ORDERED_disabled

#elif defined(__GNUC__)

    __sync_synchronize();

#elif defined(__bgq__)

    /*
     * \see http://publib.boulder.ibm.com/infocenter/comphelp/v101v121/index.jsp?topic=/com.ibm.xlcpp101.aix.doc/compiler_ref/bif_lwsync_iospace_lwsync.html
     */
    __lwsync();

#elif defined(_CRAYC)
    __builtin_ia32_sfence();

#else

    core_memory_fence();
#endif
}

void core_memory_fence()
{
#if defined(__GNUC__)

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

#elif defined(__bgq__)

    /*
     * \see http://publib.boulder.ibm.com/infocenter/cellcomp/v101v121/index.jsp?topic=/com.ibm.xlcpp101.cell.doc/compiler_ref/compiler_builtins.html
     */
    __sync();

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

#elif defined(__APPLE__)

#error "Memory fence is not implemented for __APPLE__ systems"

#else

#error "Memory fence is not implemented for unknown systems"
    /* Do nothing... */
#endif
}

void *wrapper_malloc(size_t size)
{
#ifdef CONFIG_JEMALLOC
    #warning "Using jemalloc."
    return jemalloc_malloc(size);
#else
    return malloc(size);
#endif
}

void wrapper_free(void *pointer)
{
#ifdef CONFIG_JEMALLOC
    jemalloc_free(pointer);
#else
    free(pointer);
#endif
}

void core_memory_initialize_memory_subsystem()
{
    /*
     * \Link: http://man7.org/linux/man-pages/man3/mallopt.3.html
     * [alcf-support #240569] Sparse memory versus flat memory on Blue Gene/Q
     *
     * This was suggested by Hal Finkel at ALCF.
     *
     * \Link: https://github.com/GeneAssembly/biosal/issues/819
     * \Link: https://github.com/GeneAssembly/biosal/issues/824
     * \Link: https://github.com/GeneAssembly/biosal/issues/825
     *
     * The glibc on Blue Gene/Q seems to generate a lot of memory fragmentation.
     *
     * The code below was proposed by Hal Finkel (ALCF).
     *
     * This is not required on Linux thanks to CONFIG_SPARSEMEM.
     */
#ifdef __bgq__
    mallopt(M_MMAP_THRESHOLD, sysconf(_SC_PAGESIZE));
    mallopt(M_TRIM_THRESHOLD, 0);
    mallopt(M_TOP_PAD, 0);
#endif
}
