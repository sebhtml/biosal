
#ifndef _BSAL_INPUT_H
#define _BSAL_INPUT_H

/*
#define BSAL_INPUT_DEBUG
*/

#define BSAL_INPUT_ERROR_NONE 0
#define BSAL_INPUT_ERROR_NOT_FOUND 1
#define BSAL_INPUT_ERROR_NOT_SUPPORTED 2

#define BSAL_INPUT_MAXIMUM_SEQUENCE_LENGTH 524288

struct bsal_dna_sequence;
struct bsal_input_operations;

struct bsal_input {
    struct bsal_input_operations *operations;
    void *implementation;
    char *file;
    int sequences;
    int error;
};

void bsal_input_init(struct bsal_input *input, void *implementation,
                struct bsal_input_operations *operations, char *file);
void bsal_input_destroy(struct bsal_input *input);

int bsal_input_get_sequence(struct bsal_input *input,
                char *sequence);
char *bsal_input_file(struct bsal_input *input);
int bsal_input_size(struct bsal_input *input);
void *bsal_input_implementation(struct bsal_input *input);
int bsal_input_detect(struct bsal_input *input);
int bsal_input_has_suffix(struct bsal_input *input, const char *suffix);
int bsal_input_valid(struct bsal_input *input);
int bsal_input_error(struct bsal_input *input);

#endif
