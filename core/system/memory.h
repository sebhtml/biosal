
#ifndef BSAL_MEMORY_H
#define BSAL_MEMORY_H

#include <stdlib.h>

#include <stdlib.h>
#include <stdint.h>

/*
 * \see http://stackoverflow.com/questions/15884793/how-to-get-the-name-or-file-and-line-of-caller-method
 */

/* minimum is 1 byte
 */
#define BSAL_MEMORY_MINIMUM 1

/*
 * maximum is 1000 * 1000 * 1000 * 1000 bytes (1 terabyte)
 */
#define BSAL_MEMORY_MAXIMUM 1000000000000

#define BSAL_TRUE 1
#define BSAL_FALSE 0

/*
 * Show memory allocation events.
 */
/*
#define BSAL_MEMORY_DEBUG_DETAIL
*/

/*
 */
#ifdef BSAL_MEMORY_DEBUG_DETAIL
#define BSAL_MEMORY_DEBUG
#endif

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

#define BSAL_MEMORY_ALIGNMENT_INTEL_L1_CACHE_LINE_SIZE 64
#define BSAL_MEMORY_ALIGNMENT_INTEL_L2_CACHE_LINE_SIZE 64
#define BSAL_MEMORY_ALIGNMENT_INTEL_L3_CACHE_LINE_SIZE 64

/*
 * \see http://bmagic.sourceforge.net/bmsse2opt.html
 * \see http://www.songho.ca/misc/alignment/dataalign.html
 */
#define BSAL_MEMORY_ALIGNMENT_INTEL_SSE2 16

/*
 * The AMD Opteron architecture also seems to use
 * cache lines of 64 bytes
 *
 * \see http://www.csee.wvu.edu/~ammar/cpe242_files/project%20example/AMD%20OPTERON%20ARCHITECTURE.pptx
 */

#define BSAL_MEMORY_ALIGNMENT_AMD_OPTERON_L1_CACHE_LINE_SIZE 64

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

#define BSAL_MEMORY_ALIGNMENT_BLUE_GENE_Q_L1_CACHE_LINE_SIZE 64
#define BSAL_MEMORY_ALIGNMENT_BLUE_GENE_Q_L2_CACHE_LINE_SIZE 128

/*
 * IBM Blue Gene/Q alignment
 *
 * \see https://wiki.alcf.anl.gov/parts/index.php/Blue_Gene/Q
 */

#define BSAL_MEMORY_ALIGNMENT_BLUE_GENE_Q_DESIRABLE 32

/*
 * The default alignment to use in biosal.
 * The default is 4 bytes.
 */

#define BSAL_MEMORY_ALIGNMENT_NONE 0
#define BSAL_MEMORY_ALIGNMENT_FOUR_BYTES 4
#define BSAL_MEMORY_ALIGNMENT_EIGHT_BYTES 8

#define BSAL_MEMORY_ALIGNMENT_DEFAULT BSAL_MEMORY_ALIGNMENT_NONE
/*
#define BSAL_MEMORY_ALIGNMENT_DEFAULT BSAL_MEMORY_ALIGNMENT_FOUR_BYTES
*/

#if BSAL_MEMORY_ALIGNMENT_DEFAULT > 0
#define BSAL_MEMORY_ALIGNMENT_ENABLED
#endif

#define BSAL_MEMORY_ZERO 0
#define BSAL_MEMORY_DEFAULT_VALUE BSAL_MEMORY_ZERO

#ifdef BSAL_MEMORY_DEBUG

#define bsal_memory_allocate(size) \
        bsal_memory_allocate_private(size, __func__, __FILE__, __LINE__)

#define bsal_memory_free(pointer) \
        bsal_memory_free_private(pointer, __func__, __FILE__, __LINE__)

#else

#define bsal_memory_allocate(size) \
        bsal_memory_allocate_private(size, NULL, NULL, -1)

#define bsal_memory_free(pointer) \
        bsal_memory_free_private(pointer, NULL, NULL, -1)

#endif

void *bsal_memory_allocate_private(size_t size, const char *function, const char *file, int line);
void bsal_memory_free_private(void *pointer, const char *function, const char *file, int line);

/*
 * Get size of the data segment (also called heap)
 *
 * \see http://www.hep.wisc.edu/~pinghc/Process_Memory.htm
 */
uint64_t bsal_memory_get_total_byte_count();
uint64_t bsal_memory_get_utilized_byte_count();
uint64_t bsal_memory_get_remaining_byte_count();
int bsal_memory_has_enough_bytes();

size_t bsal_memory_align(size_t unaligned);
size_t bsal_memory_align_private(size_t unaligned, size_t alignment);

void bsal_memory_fence();
void bsal_l_fence();
void bsal_s_fence();
void bsal_fence();

size_t bsal_memory_normalize_segment_length_power_of_2(size_t size);
size_t bsal_memory_normalize_segment_length_page_size(size_t size);

#endif
