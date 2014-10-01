
#include "file.h"

#include <stdio.h>

uint64_t core_file_get_size(const char *file)
{
    FILE *descriptor;
    uint64_t size;

    descriptor = fopen(file, "r");

    fseek(descriptor, 0, SEEK_END);
    size = ftell(descriptor);

    fclose(descriptor);

    return size;
}
