
#include "fastq_input.h"

#include <core/system/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
#define BIOSAL_FASTQ_INPUT_DEBUG

#define BIOSAL_FASTQ_INPUT_DEBUG2
*/

#define FIND_IDENTIFIER
#define CHECK_PREVIOUS_BYTES

#define MEMORY_FASTQ 0x1c318138

struct biosal_input_format_interface core_fastq_input_operations = {
    .init = core_fastq_input_init,
    .destroy = core_fastq_input_destroy,
    .get_sequence = core_fastq_input_get_sequence,
    .detect = core_fastq_input_detect,
    .get_offset = core_fastq_input_get_offset
};

void core_fastq_input_init(struct biosal_input_format *input)
{
    char *file;
    struct core_fastq_input *fastq;
    uint64_t offset;

    file = biosal_input_format_file(input);
    offset = biosal_input_format_start_offset(input);

#ifdef BIOSAL_FASTQ_INPUT_DEBUG
    printf("DEBUG core_fastq_input_init %s\n",
                    file);
#endif

    fastq = (struct core_fastq_input *)biosal_input_format_implementation(input);

    core_buffered_reader_init(&fastq->reader, file, offset);
    fastq->buffer = NULL;

    fastq->has_first = 0;
}

void core_fastq_input_destroy(struct biosal_input_format *input)
{
    struct core_fastq_input *fastq;

    fastq = (struct core_fastq_input *)biosal_input_format_implementation(input);
    core_buffered_reader_destroy(&fastq->reader);

    if (fastq->buffer != NULL) {
        core_memory_free(fastq->buffer, MEMORY_FASTQ);
        fastq->buffer = NULL;
    }
}

uint64_t core_fastq_input_get_sequence(struct biosal_input_format *input,
                char *sequence)
{
    struct core_fastq_input *fastq;

    /*
     * Input sequence has at least BIOSAL_INPUT_MAXIMUM_SEQUENCE_LENGTH
     * which is currently 512k
     */

    /* TODO use a dynamic buffer to accept long reads... */
    int maximum_sequence_length = BIOSAL_INPUT_MAXIMUM_SEQUENCE_LENGTH;
    int value;
    int length;

    fastq = (struct core_fastq_input *)biosal_input_format_implementation(input);

    if (fastq->buffer == NULL) {
        fastq->buffer = (char *)core_memory_allocate(maximum_sequence_length + 1, MEMORY_FASTQ);
    }

    value = 0;

    /*
     * Read name
     */
    value += core_buffered_reader_read_line(&fastq->reader, fastq->buffer,
                    maximum_sequence_length);

#ifdef FIND_IDENTIFIER
    /*
     * If we do not have the first entry yet,
     * make sure that the line is a good line.
     */
    if (!fastq->has_first) {

        while (!core_fastq_input_is_identifier(input, fastq->buffer)) {

            value += core_buffered_reader_read_line(&fastq->reader, fastq->buffer,
                    maximum_sequence_length);
        }

        fastq->has_first = 1;
    }
#endif

    /*
     * Read DNA sequence
     */
    length = core_buffered_reader_read_line(&fastq->reader, sequence,
                    maximum_sequence_length);

#ifdef BIOSAL_FASTQ_INPUT_DEBUG_READ_LINE
    printf("FASTQ ReadLine <<%s>>\n", sequence);
#endif

    if (sequence[length - 1] == '\n') {
        /*
         * Remove new line symbol.
         */
        sequence[length - 1] = '\0';
    }

    value += length;

#ifdef BIOSAL_FASTQ_INPUT_DEBUG2
    printf("DEBUG core_fastq_input_get_sequence %s\n", buffer);
#endif

    /*
     * Read the + symbol
     */
    value += core_buffered_reader_read_line(&fastq->reader, fastq->buffer,
                    maximum_sequence_length);

    /*
     * Read quality string.
     */
    value += core_buffered_reader_read_line(&fastq->reader, fastq->buffer,
                    maximum_sequence_length);

    return value;
}

int core_fastq_input_detect(struct biosal_input_format *input)
{
    if (biosal_input_format_has_suffix(input, ".fastq")) {
        return 1;
    }

    if (biosal_input_format_has_suffix(input, ".fq")) {
        return 1;
    }

    if (biosal_input_format_has_suffix(input, ".fastq.gz")) {
        return 1;
    }

    if (biosal_input_format_has_suffix(input, ".fq.gz")) {
        return 1;
    }

    return 0;
}

uint64_t core_fastq_input_get_offset(struct biosal_input_format *self)
{
    struct core_fastq_input *fastq;

    fastq = (struct core_fastq_input *)biosal_input_format_implementation(self);

    return core_buffered_reader_get_offset(&fastq->reader);
}

int core_fastq_input_is_identifier(struct biosal_input_format *self, const char *line)
{
    int length;
    char buffer[2];
    int read;
    struct core_fastq_input *fastq;

    fastq = (struct core_fastq_input *)biosal_input_format_implementation(self);

    length = strlen(line);

    if (length < 1) {
        return 0;
    }

    if (line[0] != '@') {
        return 0;
    }

    /*
     * Now, the line is either a quality string
     * or an identifier string since it starts with a @.
     */

    read = -1;

    read = core_buffered_reader_get_previous_bytes(&fastq->reader,
                    buffer, 3);

    /*
     * This is an identifier if nothing is available before.
     */
    if (read == 0) {
        return 1;
    }

    /*
     * Operation not supported by the driver.
     */
    if (read < 0) {
        return core_fastq_input_is_identifier_mock(self, line);
    }

    /*
     * Fall back on this method call.
     */
    return core_fastq_input_is_identifier_mock(self, line);
}

int core_fastq_input_is_identifier_mock(struct biosal_input_format *self, const char *line)
{
    int length;
    int i;
    int forward_slash_count;
    int colon_count;
    char character;

    length = strlen(line);

    if (length < 1) {
        return 0;
    }

    if (line[0] != '@') {
        return 0;
    }

    /*
     * At this point, it is known that the line starts with a
     * '@'. It is not known if this is the real line start.
     *
     * A line is one of the following:
     *
     * - identifier line
     * - sequence line
     * - + line
     * - quality line
     *
     * Here, those were eliminated:
     *
     * - sequence line
     * + line
     *
     * SO it is one of these:
     *
     * - identifier line
     * - quality line
     */

    /*
     * A typical entry looks like this:
     *
     * @1424:1:1:2886:1915/1
     * TAAATATTGATGTCTTACGAAATATTTTCAATTTGAGCAAGTATTATTTCAGAATTTTTATATATTTAATATTTCAAATCAAGTTTTTTTACATGCTCTA
     * +
     * d_W\d`eed\_Z\R_\YQ[XTTSRWffd[eabbb]cd_ede\dddcbbV[d\badddddNXR`^bbXa`a]W__BBBBBBBBBBBBBBBBBBBBBBBBBB
     */

    /*
     * If there is a space, then this is a identifier for sure
     * since spaces are not allowed in quality strings.
     */

    for (i = 0; i < length; i++) {
        if (line[i] == ' ') {
            return 1;
        }
    }

    /*
     * Check if the string is in the Illumina format.
     * \see http://en.wikipedia.org/wiki/FASTQ_format
     *
     * Example: @HWUSI-EAS100R:6:73:941:1973#0/1
     */

    colon_count = 0;
    forward_slash_count = 0;

    for (i = 0; i < length; i++) {

        character = line[i];

        if (character == ':') {
            ++colon_count;
        } else if (character == '/') {
            ++forward_slash_count;
        }
    }

    /*
     * This is too long for a typical identifier.
     *
     */
    if (length > 100) {

        return 0;
    }

    if (colon_count == 4 && forward_slash_count == 1) {
        return 1;
    }

    /*
     * It starts with a '@' so it is probably an identifier.
     */

    return 1;
}
