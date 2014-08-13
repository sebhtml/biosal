
#include "input_format.h"
#include "input_format_interface.h"

#include <core/system/debugger.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void bsal_input_format_init(struct bsal_input_format *input, void *implementation,
                struct bsal_input_format_interface *operations, char *file,
                uint64_t offset, uint64_t maximum_offset)
{
    bsal_input_format_interface_init_fn_t handler;
    FILE *descriptor;

#if 0
    printf("DEBUG bsal_input_format_init\n");
#endif

    input->implementation = implementation;

    BSAL_DEBUGGER_ASSERT(operations != NULL);

    input->operations = operations;
    input->sequences = 0;
    input->file = file;
    input->error = BSAL_INPUT_ERROR_NO_ERROR;

    input->start_offset = offset;
    input->end_offset = maximum_offset;

    descriptor = fopen(input->file, "r");

    if (descriptor == NULL) {
        input->error = BSAL_INPUT_ERROR_FILE_NOT_FOUND;
        return;
    } else {
        fclose(descriptor);
    }

    handler = bsal_input_format_interface_get_init(input->operations);
    handler(input);

#if 0
    printf("DEBUG after concrete init\n");
#endif
}

void bsal_input_format_destroy(struct bsal_input_format *input)
{
    bsal_input_format_interface_destroy_fn_t handler;

    if (bsal_input_format_valid(input)) {
        handler = bsal_input_format_interface_get_destroy(input->operations);
        handler(input);
    }

    input->implementation = NULL;
    input->operations = NULL;
    input->sequences = -1;
    input->file = NULL;
}

int bsal_input_format_get_sequence(struct bsal_input_format *input,
                char *sequence)
{
    bsal_input_format_interface_get_sequence_fn_t handler;
    int value;

    if (!bsal_input_format_valid(input)) {
        return 0;
    }

    /*
     * Check if the end offset has been reached.
     */

    if (input->operations->get_offset(input) >= input->end_offset) {

        return 0;
    }

    handler = bsal_input_format_interface_get_sequence(input->operations);
    value = handler(input, sequence);

    if (value) {
        input->sequences++;
    }

#ifdef BSAL_INPUT_DEBUG
    if (input->sequences % 10000000 == 0) {
        printf("DEBUG bsal_input_get_sequence %i\n",
                        input->sequences);
    }
#endif

    return value;
}

uint64_t bsal_input_format_size(struct bsal_input_format *input)
{
    return input->sequences;
}

char *bsal_input_format_file(struct bsal_input_format *input)
{
    return input->file;
}

void *bsal_input_format_implementation(struct bsal_input_format *input)
{
    return input->implementation;
}

int bsal_input_format_valid(struct bsal_input_format *input)
{
    return bsal_input_format_error(input) == BSAL_INPUT_ERROR_NO_ERROR;
}

int bsal_input_format_error(struct bsal_input_format *input)
{
    return input->error;
}

int bsal_input_format_detect(struct bsal_input_format *input)
{
    bsal_input_format_interface_detect_fn_t handler;

    if (!bsal_input_format_valid(input)) {
        return 0;
    }

    handler = bsal_input_format_interface_get_detect(input->operations);
    return handler(input);
}

int bsal_input_format_has_suffix(struct bsal_input_format *input, const char *suffix)
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

uint64_t bsal_input_format_offset(struct bsal_input_format *input)
{
    BSAL_DEBUGGER_ASSERT(input->operations != NULL);

    return input->operations->get_offset(input);
}

uint64_t bsal_input_format_start_offset(struct bsal_input_format *input)
{
    return input->start_offset;
}

uint64_t bsal_input_format_end_offset(struct bsal_input_format *input)
{
    return input->end_offset;
}
