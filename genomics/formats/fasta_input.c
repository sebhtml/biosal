
#include "fasta_input.h"

#include <core/system/memory.h>
#include <core/system/debugger.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MEMORY_FASTA 0x480002a5

struct biosal_input_format_interface core_fasta_input_operations = {
    .init = core_fasta_input_init,
    .destroy = core_fasta_input_destroy,
    .get_sequence = core_fasta_input_get_sequence,
    .detect = core_fasta_input_detect,
    .get_offset = core_fasta_input_get_offset
};

void core_fasta_input_init(struct biosal_input_format *input)
{
    char *file;
    struct core_fasta_input *fasta;
    uint64_t offset;

    fasta = (struct core_fasta_input *)biosal_input_format_implementation(input);

    file = biosal_input_format_file(input);

    CORE_DEBUGGER_ASSERT(input->operations != NULL);

#if 0
    printf("DEBUG BEFORE faulty call.\n");
#endif
    offset = biosal_input_format_start_offset(input);

    core_buffered_reader_init(&fasta->reader, file, offset);

    fasta->buffer = NULL;
    fasta->next_header = NULL;
    fasta->has_header = 0;

    fasta->has_first = 0;
}

void core_fasta_input_destroy(struct biosal_input_format *input)
{
    struct core_fasta_input *fasta;

    fasta = (struct core_fasta_input *)biosal_input_format_implementation(input);
    core_buffered_reader_destroy(&fasta->reader);

    if (fasta->buffer != NULL) {
        core_memory_free(fasta->buffer, MEMORY_FASTA);
        fasta->buffer = NULL;
    }

    if (fasta->next_header != NULL) {
        core_memory_free(fasta->next_header, MEMORY_FASTA);
        fasta->next_header= NULL;
    }
}

uint64_t core_fasta_input_get_sequence(struct biosal_input_format *input,
                char *sequence)
{
    struct core_fasta_input *fasta;

    /* TODO use a dynamic buffer to accept long reads... */
    int maximum_sequence_length = BIOSAL_INPUT_MAXIMUM_SEQUENCE_LENGTH;
    int value;
    int lines;
    int total;
    int position_in_sequence;
    int is_header;
    int block_length;

    fasta = (struct core_fasta_input *)biosal_input_format_implementation(input);

    if (fasta->buffer == NULL) {
        fasta->buffer = core_memory_allocate(maximum_sequence_length + 1, MEMORY_FASTA);
        fasta->next_header= core_memory_allocate(maximum_sequence_length + 1, MEMORY_FASTA);

        fasta->buffer[0] = '\0';
        fasta->next_header[0] = '\0';
    }

    value = 0;
    total = 0;
    lines = 0;

    /*
     * Read name
     */

    if (fasta->has_header) {

        strcpy(fasta->buffer, fasta->next_header);

        value = strlen(fasta->buffer);

        fasta->has_header = 0;

    } else {
        value = core_buffered_reader_read_line(&fasta->reader, fasta->buffer,
                    maximum_sequence_length);

        /* Make sure that this is an identifier.
         */
        if (!fasta->has_first) {

            while (!core_fasta_input_check_header(input, fasta->buffer)) {

                value = core_buffered_reader_read_line(&fasta->reader, fasta->buffer,
                    maximum_sequence_length);
            }

            fasta->has_first = 1;
        }
    }

    /*
     * Add new line.
     */
    if (value) {
        ++lines;
    }

    total += value;

    /*
     * Read sequence.
     *
     * Discard any new line symbol too.
     */

    position_in_sequence = 0;

    while (1) {
        value = core_buffered_reader_read_line(&fasta->reader, fasta->buffer,
                    maximum_sequence_length);

        if (value == 0) {
            break;
        }

        is_header = 0;

        if (strlen(fasta->buffer) > 0
                        && fasta->buffer[0] == '>') {

            is_header = 1;
        }

        if (is_header) {
            sequence[position_in_sequence] = '\0';

            strcpy(fasta->next_header, fasta->buffer);
            fasta->has_header = 1;
            break;
        }

        /*
         * Otherwise, add the sequence.
         */

        if (value) {
            ++lines;
        }

        block_length = strlen(fasta->buffer);

        /*
         * Remove the new line.
         */
        if (fasta->buffer[block_length - 1] == '\n') {
            --block_length;
        }

        core_memory_copy(sequence + position_in_sequence,
                        fasta->buffer,
                        block_length);

        position_in_sequence += block_length;
    }

    return total;
}

int core_fasta_input_detect(struct biosal_input_format *input)
{
    if (biosal_input_format_has_suffix(input, ".fasta")) {
        return 1;
    } else if (biosal_input_format_has_suffix(input, ".fa")) {
        return 1;
    } else if (biosal_input_format_has_suffix(input, ".fa.gz")) {
        return 1;
    } else if (biosal_input_format_has_suffix(input, ".fasta.gz")) {
        return 1;
    }

    return 0;
}

uint64_t core_fasta_input_get_offset(struct biosal_input_format *self)
{
    struct core_fasta_input *fasta;

    fasta = (struct core_fasta_input *)biosal_input_format_implementation(self);

    return core_buffered_reader_get_offset(&fasta->reader);
}

int core_fasta_input_check_header(struct biosal_input_format *self, const char *line)
{
    int length;
    char character;

    length = strlen(line);

    if (length == 0) {
        return 0;
    }

    character = line[0];

    if (character != '>') {
        return 0;
    }

    /*
     * Assume that the identifier line can not contain the >
     * symbol after the first > symbol.
     */

    printf("DEBUG is FASTA header.\n");

    return 1;
}
