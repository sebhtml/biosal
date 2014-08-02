
#include "gc_ratio_calculator.h"

struct bsal_script gc_ratio_calculator_script;

int main(int argc, char *argv[])
{
    return bsal_thorium_engine_boot_initial_actor(&argc, &argv, GC_RATIO_CALCULATOR_SCRIPT, &gc_ratio_calculator_script);
}
