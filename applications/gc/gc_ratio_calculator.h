
#ifndef _GC_RATIO_CALCULATOR_H_
#define _GC_RATIO_CALCULATOR_H_

#include <biosal.h>

#define SCRIPT_GC_RATIO_CALCULATOR 0x74c20114

#define ACTION_GC_HELLO 0x00003fa8
#define ACTION_GC_HELLO_REPLY 0x00004790

struct gc_ratio_calculator {
    struct core_vector spawners;
    int completed;
};

extern struct thorium_script gc_ratio_calculator_script;

#endif /* _GC_RATIO_CALCULATOR_H_ */
