
#include "command.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int core_command_has_argument(int argc, char **argv, const char *argument)
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

char *core_command_get_argument_value(int argc, char **argv, const char *argument)
{
    int i;
    char *value;

    value = NULL;

    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], argument) == 0 && i + 1 < argc) {
            value = argv[i + 1];
            break;
        }
    }

    return value;
}

int core_command_get_argument_value_int(int argc, char **argv, const char *argument)
{
    char *value;
    int integer_value;

    integer_value = -1;
    value = core_command_get_argument_value(argc, argv, argument);

    if (value != NULL) {
        integer_value = atoi(value);
    }

    return integer_value;
}
