
#ifndef CORE_MEMORY_H
#define CORE_MEMORY_H

#include <stdlib.h>

#include <stdlib.h>
#include <stdint.h>

/*
 * \see http://stackoverflow.com/questions/15884793/how-to-get-the-name-or-file-and-line-of-caller-method
 */

/* minimum is 1 byte
 */
#define CORE_MEMORY_MINIMUM 1

/*
 * maximum is 1000 * 1000 * 1000 * 1000 bytes (1 terabyte)
 */
#define CORE_MEMORY_MAXIMUM 1000000000000

#define CORE_TRUE 1
#define CORE_FALSE 0

/*
 * Show memory allocation events.
 */
/*
#define CORE_MEMORY_DEBUG
*/

/* Intel processors usually have a cache line of 64 bytes.
 * At least, it is the case for Sandy Bridge, Ivy Bridge, and Haswell.
 *
 * \see https://software.intel.com/en-us/forums/topic/307517
 * \see https://software.intel.com/en-us/forums/topic/332887
 * \see http://www.7-cpu.com/cpu/SandyBridge.html
 * \see http://www.7-cpu.com/cpu/IvyBridge.html
 * \see http://www.7-cpu.com/cpu/Haswell.html
 * \see http://stackoverflow.com/questions/8620303/how-many-bytes-does-a-xeon-bring-into-the-cache-per-memory-access
 * \see http://stackoverflow.com/questions/716145/l1-memory-cache-on-intel-x86-processors
 * \see www.prace-project.eu/Best-Practice-Guide-Intel-Xeon-Phi-HTML#id-1.13.5.3
 */

#define CORE_MEMORY_ALIGNMENT_INTEL_L1_CACHE_LINE_SIZE 64
#define CORE_MEMORY_ALIGNMENT_INTEL_L2_CACHE_LINE_SIZE 64
#define CORE_MEMORY_ALIGNMENT_INTEL_L3_CACHE_LINE_SIZE 64

/*
 * \see http://bmagic.sourceforge.net/bmsse2opt.html
 * \see http://www.songho.ca/misc/alignment/dataalign.html
 */
#define CORE_MEMORY_ALIGNMENT_INTEL_SSE2 16

/*
 * The AMD Opteron architecture also seems to use
 * cache lines of 64 bytes
 *
 * \see http://www.csee.wvu.edu/~ammar/cpe242_files/project%20example/AMD%20OPTERON%20ARCHITECTURE.pptx
 */

#define CORE_MEMORY_ALIGNMENT_AMD_OPTERON_L1_CACHE_LINE_SIZE 64

/*
 * The Blue Gene/Q has these cache line sizes:
 * On the Internet, people tend to use 32 byte alignment on the
 * Blue Gene/Q.
 *
 * \see https://github.com/etmc/tmLQCD/wiki/BlueGene-Q-stuff
 * \see https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q
 * \see http://www.training.prace-ri.eu/uploads/tx_pracetmo/sc-bgq-porting.pdf
 * \see http://pic.dhe.ibm.com/infocenter/compbg/v121v141/index.jsp?topic=%2Fcom.ibm.xlcpp121.bg.doc%2Fproguide%2Falignment.html
 */

#define CORE_MEMORY_ALIGNMENT_BLUE_GENE_Q_L1_CACHE_LINE_SIZE 64
#define CORE_MEMORY_ALIGNMENT_BLUE_GENE_Q_L2_CACHE_LINE_SIZE 128

/*
 * IBM Blue Gene/Q alignment
 *
 * \see https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q
 */

#define CORE_MEMORY_ALIGNMENT_BLUE_GENE_Q_DESIRABLE 32

/*
 * The default alignment to use in biosal.
 * The default is 4 bytes.
 */

#define CORE_MEMORY_ALIGNMENT_NONE 0
#define CORE_MEMORY_ALIGNMENT_FOUR_BYTES 4
#define CORE_MEMORY_ALIGNMENT_EIGHT_BYTES 8

#define CORE_MEMORY_ALIGNMENT_DEFAULT CORE_MEMORY_ALIGNMENT_NONE
/*
#define CORE_MEMORY_ALIGNMENT_DEFAULT CORE_MEMORY_ALIGNMENT_FOUR_BYTES
*/

#if CORE_MEMORY_ALIGNMENT_DEFAULT > 0
#define CORE_MEMORY_ALIGNMENT_ENABLED
#endif

#define CORE_MEMORY_ZERO 0
#define CORE_MEMORY_DEFAULT_VALUE CORE_MEMORY_ZERO

#ifdef CORE_MEMORY_DEBUG

#define core_memory_allocate(size, key) \
        core_memory_allocate_private(size, __func__, __FILE__, __LINE__, key)

#define core_memory_free(pointer, key) \
        core_memory_free_private(pointer, __func__, __FILE__, __LINE__, key)

#else

#define core_memory_allocate(size, key) \
        core_memory_allocate_private(size, NULL, NULL, -1, key)

#define core_memory_free(pointer, key) \
        core_memory_free_private(pointer, NULL, NULL, -1, key)

#endif

void *core_memory_allocate_private(size_t size, const char *function, const char *file, int line, int key);
void core_memory_free_private(void *pointer, const char *function, const char *file, int line, int key);

/*
 * Get size of the data segment (also called heap)
 *
 * \see http://www.hep.wisc.edu/~pinghc/Process_Memory.htm
 */
uint64_t core_memory_get_total_byte_count();
uint64_t core_memory_get_utilized_byte_count();
uint64_t core_memory_get_remaining_byte_count();
int core_memory_has_enough_bytes();

size_t core_memory_align(size_t unaligned);
size_t core_memory_align_private(size_t unaligned, size_t alignment);

void core_memory_fence();
void core_l_fence();
void core_s_fence();
void core_fence();

size_t core_memory_normalize_segment_length_power_of_2(size_t size);
size_t core_memory_normalize_segment_length_page_size(size_t size);

void *core_memory_copy(void *destination, const void *source, size_t count);
void *core_memory_move(void *destination, const void *source, size_t count);

#endif
