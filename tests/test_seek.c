
#include <stdio.h>

#include <stdint.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
    char *file;
    uint64_t offset;
    FILE *descriptor;
    char line[1000];

    if (argc != 3) {
        return 0;
    }

    file = argv[1];

    sscanf(argv[2], "%" PRIu64, &offset);

    descriptor = fopen(file, "r");

    fseek(descriptor, offset, SEEK_SET);

    fgets(line, 1000, descriptor);
    fclose(descriptor);

    printf("%s\n", line);

    return 0;
}
