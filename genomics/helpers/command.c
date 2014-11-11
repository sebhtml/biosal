
#include "command.h"

#include <core/system/command.h>

#include <stdlib.h>
#include <stdio.h>

char BIOSAL_DEFAULT_OUTPUT[] = "output";

char *biosal_command_get_output_directory(int argc, char **argv)
{
    char *directory_name;
    char *value;

    directory_name = BIOSAL_DEFAULT_OUTPUT;

    value = core_command_get_argument_value(argc, argv, "-o");

    if (value != NULL) {
        directory_name = value;
    }

    return directory_name;
}

int biosal_command_get_kmer_length(int argc, char **argv)
{
    int kmer_length;
    int provided_value;

    kmer_length = BIOSAL_DEFAULT_KMER_LENGTH;

    if (core_command_has_argument(argc, argv, "-k")) {
        provided_value = core_command_get_argument_value_int(argc, argv, "-k");

        /*
         * Use a odd kmer length
         */
        if (provided_value % 2 == 0) {
            ++provided_value;
        }

        kmer_length = provided_value;
    }

    printf("DEBUG kmer_length %d\n", kmer_length);

    return kmer_length;
}

int biosal_command_get_minimum_coverage(int argc, char **argv)
{
    int value;

    value = BIOSAL_DEFAULT_MINIMUM_COVERAGE;

    return value;
}
