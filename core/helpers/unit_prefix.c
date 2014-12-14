
#include "unit_prefix.h"

#define PREFIX_kilo (1000)
#define PREFIX_kilo_symbol 'k'
#define PREFIX_mega (1000 * 1000)
#define PREFIX_mega_symbol 'M'
#define PREFIX_giga (1000 * 1000 * 1000)
#define PREFIX_giga_symbol 'G'
#define PREFIX_tera 1000000000LL
#define PREFIX_tera_symbol 'T'

void core_get_metric_system_unit_prefix(double input, char *prefix, double *value)
{
    if (PREFIX_kilo <= input && input < PREFIX_mega) {
        *prefix = PREFIX_kilo_symbol;
        *value = input / PREFIX_kilo;

    } else if (PREFIX_mega <= input && input < PREFIX_giga) {
        *prefix = PREFIX_mega_symbol;
        *value = input / PREFIX_mega;

    } else if (PREFIX_giga <= input && input < PREFIX_tera) {
        *prefix = PREFIX_giga_symbol;
        *value = input / PREFIX_giga;

    } else {
        *prefix = ' ';
        *value = input;
    }
}
