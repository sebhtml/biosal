
#ifndef _BSAL_INPUT_H
#define _BSAL_INPUT_H

/*
#define BSAL_INPUT_DEBUG
*/

#define BSAL_INPUT_ERROR_NONE 0
#define BSAL_INPUT_ERROR_NOT_FOUND 1

struct bsal_dna_sequence;
struct bsal_input_vtable;

struct bsal_input {
    struct bsal_input_vtable *vtable;
    void *pointer;
    char *file;
    int sequences;
    int error;
};

void bsal_input_init(struct bsal_input *input, void *pointer,
                struct bsal_input_vtable *vtable, char *file);
void bsal_input_destroy(struct bsal_input *input);

int bsal_input_get_sequence(struct bsal_input *input,
                struct bsal_dna_sequence *sequence);
char *bsal_input_file(struct bsal_input *input);
int bsal_input_size(struct bsal_input *input);
void *bsal_input_pointer(struct bsal_input *input);
int bsal_input_detect(struct bsal_input *input);
int bsal_input_has_suffix(struct bsal_input *input, const char *suffix);
int bsal_input_valid(struct bsal_input *input);
int bsal_input_error(struct bsal_input *input);

#endif
