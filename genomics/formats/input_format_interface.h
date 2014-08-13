
#ifndef BSAL_INPUT_FORMAT_INTERFACE_H
#define BSAL_INPUT_FORMAT_INTERFACE_H

struct bsal_input_format;
struct bsal_dna_sequence;

#include <stdint.h>

typedef void (*bsal_input_format_interface_init_fn_t)(
    struct bsal_input_format *self
);

typedef void (*bsal_input_format_interface_destroy_fn_t)(
    struct bsal_input_format *self
);

/*
 * return the number of bytes read to get
 * the sequence.
 */
typedef uint64_t (*bsal_input_format_interface_get_sequence_fn_t)(
    struct bsal_input_format *self,
    char *sequence
);

typedef int (*bsal_input_format_interface_detect_fn_t)(
    struct bsal_input_format *self
);

typedef uint64_t (*bsal_input_format_interface_get_offset_fn_t)(
    struct bsal_input_format *self
);

struct bsal_input_format_interface {
    bsal_input_format_interface_init_fn_t init;
    bsal_input_format_interface_destroy_fn_t destroy;
    bsal_input_format_interface_get_sequence_fn_t get_sequence;
    bsal_input_format_interface_detect_fn_t detect;
    bsal_input_format_interface_get_offset_fn_t get_offset;
};

void bsal_input_format_interface_init(struct bsal_input_format_interface *operations);
void bsal_input_format_interface_destroy(struct bsal_input_format_interface *operations);
bsal_input_format_interface_init_fn_t bsal_input_format_interface_get_init(struct bsal_input_format_interface *operations);
bsal_input_format_interface_destroy_fn_t bsal_input_format_interface_get_destroy(struct bsal_input_format_interface *operations);
bsal_input_format_interface_get_sequence_fn_t bsal_input_format_interface_get_sequence(struct bsal_input_format_interface *operations);
bsal_input_format_interface_detect_fn_t bsal_input_format_interface_get_detect(struct bsal_input_format_interface *operations);

#endif
