
#ifndef _BSAL_INPUT_VTABLE_H
#define _BSAL_INPUT_VTABLE_H

struct bsal_input;
struct bsal_dna_sequence;

typedef void (*bsal_input_init_fn_t)(
    struct bsal_input *input
);

typedef void (*bsal_input_destroy_fn_t)(
    struct bsal_input *input
);

typedef int (*bsal_input_get_sequence_fn_t)(
    struct bsal_input *input,
    char *sequence
);

typedef int (*bsal_input_detect_fn_t)(
    struct bsal_input *input
);

struct bsal_input_operations {
    bsal_input_init_fn_t init;
    bsal_input_destroy_fn_t destroy;
    bsal_input_get_sequence_fn_t get_sequence;
    bsal_input_detect_fn_t detect;
};

void bsal_input_operations_init(struct bsal_input_operations *operations);
void bsal_input_operations_destroy(struct bsal_input_operations *operations);
bsal_input_init_fn_t bsal_input_operations_get_init(struct bsal_input_operations *operations);
bsal_input_destroy_fn_t bsal_input_operations_get_destroy(struct bsal_input_operations *operations);
bsal_input_get_sequence_fn_t bsal_input_operations_get_get_sequence(struct bsal_input_operations *operations);
bsal_input_detect_fn_t bsal_input_operations_get_detect(struct bsal_input_operations *operations);

#endif
