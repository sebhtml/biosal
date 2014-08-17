
#include "command.h"

#include <string.h>

int bsal_command_has_argument(int argc, char **argv, const char *argument)
{
    int i;
    char *actual_argument;

    for (i = 0; i < argc; i++) {
        actual_argument = argv[i];

        if (strcmp(actual_argument, argument) == 0) {
            return 1;
        }
    }

    return 0;
}
