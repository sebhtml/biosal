
#ifndef BSAL_MEMORY_H
#define BSAL_MEMORY_H

#include <stdlib.h>

#include <stdlib.h>
#include <stdint.h>

/*
 * \see http://stackoverflow.com/questions/15884793/how-to-get-the-name-or-file-and-line-of-caller-method
 */

#define BSAL_TRUE 1
#define BSAL_FALSE 0

/*
 */
#define BSAL_MEMORY_DEBUG

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
 * The default alignment to use in biosal.
 * The default is 4 bytes.
 */

#define BSAL_MEMORY_ALIGNMENT_NONE 0
#define BSAL_MEMORY_ALIGNMENT_32_BITS 4
#define BSAL_MEMORY_ALIGNMENT_64_BITS 8

#define BSAL_MEMORY_ZERO 0

#define BSAL_MEMORY_ALIGNMENT_DEFAULT BSAL_MEMORY_ALIGNMENT_NONE
#define BSAL_MEMORY_DEFAULT_ALIGNMENT BSAL_MEMORY_ALIGNMENT_DEFAULT
#define BSAL_MEMORY_DEFAULT_VALUE BSAL_MEMORY_ZERO

/* enable alignment only if alignment is greater than 0
 */
#if BSAL_MEMORY_ALIGNMENT_DEFAULT > 0

#define BSAL_MEMORY_ALIGNMENT_ENABLED

#endif

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

uint64_t bsal_get_heap_size();

int bsal_memory_align(int unaligned, int alignment);

#endif
