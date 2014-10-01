
#ifndef BIOSAL_INPUT_FORMAT_H
#define BIOSAL_INPUT_FORMAT_H

/*
#define BIOSAL_INPUT_DEBUG
*/

#define BIOSAL_INPUT_ERROR_NO_ERROR -1
#define BIOSAL_INPUT_ERROR_FILE_NOT_FOUND -2
#define BIOSAL_INPUT_ERROR_FORMAT_NOT_SUPPORTED -3
#define BIOSAL_INPUT_ERROR_ALREADY_OPEN -4
#define BIOSAL_INPUT_ERROR_FILE_NOT_OPEN -5
#define BIOSAL_INPUT_ERROR_END_OF_FILE -6

#define BIOSAL_INPUT_MAXIMUM_SEQUENCE_LENGTH 524288

struct biosal_dna_sequence;
struct biosal_input_format_interface;

#include <stdint.h>

/*
 * The abstract class for input formats.
 */
struct biosal_input_format {
    struct biosal_input_format_interface *operations;
    void *implementation;
    char *file;
    uint64_t sequences;

    uint64_t start_offset;
    uint64_t end_offset;

    int error;
};

void biosal_input_format_init(struct biosal_input_format *self, void *implementation,
                struct biosal_input_format_interface *operations, char *file,
                uint64_t start_offset, uint64_t end_offset);
void biosal_input_format_destroy(struct biosal_input_format *self);

int biosal_input_format_get_sequence(struct biosal_input_format *self,
                char *sequence);
char *biosal_input_format_file(struct biosal_input_format *self);
uint64_t biosal_input_format_size(struct biosal_input_format *self);
uint64_t biosal_input_format_start_offset(struct biosal_input_format *self);
uint64_t biosal_input_format_end_offset(struct biosal_input_format *self);
uint64_t biosal_input_format_offset(struct biosal_input_format *self);
void *biosal_input_format_implementation(struct biosal_input_format *self);
int biosal_input_format_detect(struct biosal_input_format *self);
int biosal_input_format_has_suffix(struct biosal_input_format *self, const char *suffix);
int biosal_input_format_valid(struct biosal_input_format *self);
int biosal_input_format_error(struct biosal_input_format *self);

#endif
