
#include "command.h"

#include <string.h>

char BSAL_DEFAULT_OUTPUT[] = "output";

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

char *bsal_command_get_argument_value(int argc, char **argv, const char *argument)
{
    int i;
    char *value;

    value = NULL;

    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            value = argv[i + 1];
            break;
        }
    }

    return value;
}


char *bsal_command_get_output_directory(int argc, char **argv)
{
    char *directory_name;
    char *value;

    directory_name = BSAL_DEFAULT_OUTPUT;

    value = bsal_command_get_argument_value(argc, argv, "-o");

    if (value != NULL) {
        value = directory_name;
    }

    return directory_name;
}

