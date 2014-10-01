
#include "gc_ratio_calculator.h"

struct thorium_script gc_ratio_calculator_script;

int main(int argc, char *argv[])
{
    return biosal_thorium_engine_boot_initial_actor(&argc, &argv, SCRIPT_GC_RATIO_CALCULATOR, &gc_ratio_calculator_script);
}
