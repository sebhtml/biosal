
#include "input_format.h"
#include "input_format_interface.h"

#include <core/system/debugger.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h>

void biosal_input_format_init(struct biosal_input_format *input, void *implementation,
                struct biosal_input_format_interface *operations, char *file,
                uint64_t start_offset, uint64_t end_offset)
{
    biosal_input_format_interface_init_fn_t handler;
    FILE *descriptor;

#if 0
    printf("DEBUG biosal_input_format_init\n");
#endif

    input->implementation = implementation;

    BIOSAL_DEBUGGER_ASSERT(operations != NULL);

    input->operations = operations;
    input->sequences = 0;
    input->file = file;
    input->error = BIOSAL_INPUT_ERROR_NO_ERROR;

    input->start_offset = start_offset;
    input->end_offset = end_offset;

    descriptor = fopen(input->file, "r");

    if (descriptor == NULL) {
        input->error = BIOSAL_INPUT_ERROR_FILE_NOT_FOUND;
        return;
    } else {
        fclose(descriptor);
    }

#if 0
    printf("DEBUG INPUT_FORMAT %s from %" PRIu64 " to %" PRIu64 "\n",
                    file, start_offset, end_offset);
#endif

    handler = biosal_input_format_interface_get_init(input->operations);
    handler(input);

#if 0
    printf("DEBUG after concrete init\n");
#endif
}

void biosal_input_format_destroy(struct biosal_input_format *input)
{
    biosal_input_format_interface_destroy_fn_t handler;

    if (biosal_input_format_valid(input)) {
        handler = biosal_input_format_interface_get_destroy(input->operations);
        handler(input);
    }

    input->implementation = NULL;
    input->operations = NULL;
    input->sequences = -1;
    input->file = NULL;
}

int biosal_input_format_get_sequence(struct biosal_input_format *input,
                char *sequence)
{
    biosal_input_format_interface_get_sequence_fn_t handler;
    int value;

    if (!biosal_input_format_valid(input)) {
        return 0;
    }

    /*
     * Check if the end offset has been reached.
     */

    if (input->operations->get_offset(input) > input->end_offset) {

        return 0;
    }

    handler = biosal_input_format_interface_get_sequence(input->operations);
    value = handler(input, sequence);

    if (value) {
        input->sequences++;
    }

#ifdef BIOSAL_INPUT_DEBUG
    if (input->sequences % 10000000 == 0) {
        printf("DEBUG biosal_input_get_sequence %i\n",
                        input->sequences);
    }
#endif

    return value;
}

uint64_t biosal_input_format_size(struct biosal_input_format *input)
{
    return input->sequences;
}

char *biosal_input_format_file(struct biosal_input_format *input)
{
    return input->file;
}

void *biosal_input_format_implementation(struct biosal_input_format *input)
{
    return input->implementation;
}

int biosal_input_format_valid(struct biosal_input_format *input)
{
    return biosal_input_format_error(input) == BIOSAL_INPUT_ERROR_NO_ERROR;
}

int biosal_input_format_error(struct biosal_input_format *input)
{
    return input->error;
}

int biosal_input_format_detect(struct biosal_input_format *input)
{
    biosal_input_format_interface_detect_fn_t handler;

    if (!biosal_input_format_valid(input)) {
        return 0;
    }

    handler = biosal_input_format_interface_get_detect(input->operations);
    return handler(input);
}

int biosal_input_format_has_suffix(struct biosal_input_format *input, const char *suffix)
{
    int position_in_file;
    int position_in_suffix;

    position_in_file = strlen(input->file) - 1;
    position_in_suffix = strlen(suffix) - 1;

    while (position_in_file >= 0 && position_in_suffix >= 0) {
        if (input->file[position_in_file] != suffix[position_in_suffix]) {
            return 0;
        }

        position_in_file--;
        position_in_suffix--;
    }

    if (position_in_suffix < 0) {
        return 1;
    }

    return 0;
}

uint64_t biosal_input_format_offset(struct biosal_input_format *input)
{
    BIOSAL_DEBUGGER_ASSERT(input->operations != NULL);

    return input->operations->get_offset(input);
}

uint64_t biosal_input_format_start_offset(struct biosal_input_format *input)
{
    return input->start_offset;
}

uint64_t biosal_input_format_end_offset(struct biosal_input_format *input)
{
    return input->end_offset;
}
