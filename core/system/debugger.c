
#include "debugger.h"

void biosal_debugger_examine(void *pointer, int bytes)
{
    int position;
    char *array;
    char byte_value;

    array = pointer;

    for (position = 0; position < bytes; position++) {
        byte_value = array[position];
        printf(" %x", byte_value & 0xff);
    }

    printf("\n");
}
