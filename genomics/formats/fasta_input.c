
#include "fasta_input.h"

#include <core/system/memory.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct bsal_input_format_interface bsal_fasta_input_operations = {
    .init = bsal_fasta_input_init,
    .destroy = bsal_fasta_input_destroy,
    .get_sequence = bsal_fasta_input_get_sequence,
    .detect = bsal_fasta_input_detect
};

void bsal_fasta_input_init(struct bsal_input_format *input)
{
    char *file;
    struct bsal_fasta_input *fasta;
    uint64_t offset;

    file = bsal_input_format_file(input);
    offset = bsal_input_format_offset(input);

    fasta = (struct bsal_fasta_input *)bsal_input_format_implementation(input);

    bsal_buffered_reader_init(&fasta->reader, file, offset);
    fasta->buffer = NULL;
    fasta->next_header = NULL;
    fasta->has_header = 0;
}

void bsal_fasta_input_destroy(struct bsal_input_format *input)
{
    struct bsal_fasta_input *fasta;

    fasta = (struct bsal_fasta_input *)bsal_input_format_implementation(input);
    bsal_buffered_reader_destroy(&fasta->reader);

    if (fasta->buffer != NULL) {
        bsal_memory_free(fasta->buffer);
        fasta->buffer = NULL;
    }

    if (fasta->next_header != NULL) {
        bsal_memory_free(fasta->next_header);
        fasta->next_header= NULL;
    }
}

uint64_t bsal_fasta_input_get_sequence(struct bsal_input_format *input,
                char *sequence)
{
    struct bsal_fasta_input *fasta;

    /* TODO use a dynamic buffer to accept long reads... */
    int maximum_sequence_length = BSAL_INPUT_MAXIMUM_SEQUENCE_LENGTH;
    int value;
    int lines;
    int total;
    int position_in_sequence;
    int is_header;
    int block_length;

    fasta = (struct bsal_fasta_input *)bsal_input_format_implementation(input);

    if (fasta->buffer == NULL) {
        fasta->buffer = bsal_memory_allocate(maximum_sequence_length + 1);
        fasta->next_header= bsal_memory_allocate(maximum_sequence_length + 1);

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
        value = bsal_buffered_reader_read_line(&fasta->reader, fasta->buffer,
                    maximum_sequence_length);
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
     */

    position_in_sequence = 0;

    while (1) {
        value = bsal_buffered_reader_read_line(&fasta->reader, fasta->buffer,
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
        memcpy(sequence + position_in_sequence,
                        fasta->buffer,
                        block_length);

        position_in_sequence += block_length;
    }

    /* add the new lines if a sequence was
     * found.
     *
     * This is required because bsal_buffered_reader_read_line does not give
     * the new line.
     */

    if (value) {
        total += lines;
    }

#if 0
    printf("FASTA_DEBUG SEQ= %s\n", sequence);
#endif

    return total;
}

int bsal_fasta_input_detect(struct bsal_input_format *input)
{
    if (bsal_input_format_has_suffix(input, ".fasta")) {
        return 1;
    } else if (bsal_input_format_has_suffix(input, ".fa")) {
        return 1;
    } else if (bsal_input_format_has_suffix(input, ".fa.gz")) {
        return 1;
    } else if (bsal_input_format_has_suffix(input, ".fasta.gz")) {
        return 1;
    }

    return 0;
}
