
#ifndef BSAL_INPUT_FORMAT_H
#define BSAL_INPUT_FORMAT_H

/*
#define BSAL_INPUT_DEBUG
*/

#define BSAL_INPUT_ERROR_NO_ERROR -1
#define BSAL_INPUT_ERROR_FILE_NOT_FOUND -2
#define BSAL_INPUT_ERROR_FORMAT_NOT_SUPPORTED -3
#define BSAL_INPUT_ERROR_ALREADY_OPEN -4
#define BSAL_INPUT_ERROR_FILE_NOT_OPEN -5
#define BSAL_INPUT_ERROR_END_OF_FILE -6

#define BSAL_INPUT_MAXIMUM_SEQUENCE_LENGTH 524288

struct bsal_dna_sequence;
struct bsal_input_format_interface;

#include <stdint.h>

/*
 * The abstract class for input formats.
 */
struct bsal_input_format {
    struct bsal_input_format_interface *operations;
    void *implementation;
    char *file;
    uint64_t sequences;
    uint64_t offset;

    uint64_t start_offset;
    uint64_t end_offset;

    int error;
};

void bsal_input_format_init(struct bsal_input_format *input, void *implementation,
                struct bsal_input_format_interface *operations, char *file,
                uint64_t offset, uint64_t maximum_offset);
void bsal_input_format_destroy(struct bsal_input_format *input);

int bsal_input_format_get_sequence(struct bsal_input_format *input,
                char *sequence);
char *bsal_input_format_file(struct bsal_input_format *input);
uint64_t bsal_input_format_size(struct bsal_input_format *input);
uint64_t bsal_input_format_start_offset(struct bsal_input_format *input);
uint64_t bsal_input_format_end_offset(struct bsal_input_format *input);
uint64_t bsal_input_format_offset(struct bsal_input_format *input);
void *bsal_input_format_implementation(struct bsal_input_format *input);
int bsal_input_format_detect(struct bsal_input_format *input);
int bsal_input_format_has_suffix(struct bsal_input_format *input, const char *suffix);
int bsal_input_format_valid(struct bsal_input_format *input);
int bsal_input_format_error(struct bsal_input_format *input);

#endif
