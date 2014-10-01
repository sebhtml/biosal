
#ifndef BIOSAL_INPUT_FORMAT_INTERFACE_H
#define BIOSAL_INPUT_FORMAT_INTERFACE_H

struct biosal_input_format;
struct biosal_dna_sequence;

#include <stdint.h>

typedef void (*biosal_input_format_interface_init_fn_t)(
    struct biosal_input_format *self
);

typedef void (*biosal_input_format_interface_destroy_fn_t)(
    struct biosal_input_format *self
);

/*
 * return the number of bytes read to get
 * the sequence.
 */
typedef uint64_t (*biosal_input_format_interface_get_sequence_fn_t)(
    struct biosal_input_format *self,
    char *sequence
);

typedef int (*biosal_input_format_interface_detect_fn_t)(
    struct biosal_input_format *self
);

typedef uint64_t (*biosal_input_format_interface_get_offset_fn_t)(
    struct biosal_input_format *self
);

struct biosal_input_format_interface {
    biosal_input_format_interface_init_fn_t init;
    biosal_input_format_interface_destroy_fn_t destroy;
    biosal_input_format_interface_get_sequence_fn_t get_sequence;
    biosal_input_format_interface_detect_fn_t detect;
    biosal_input_format_interface_get_offset_fn_t get_offset;
};

void biosal_input_format_interface_init(struct biosal_input_format_interface *operations);
void biosal_input_format_interface_destroy(struct biosal_input_format_interface *operations);
biosal_input_format_interface_init_fn_t biosal_input_format_interface_get_init(struct biosal_input_format_interface *operations);
biosal_input_format_interface_destroy_fn_t biosal_input_format_interface_get_destroy(struct biosal_input_format_interface *operations);
biosal_input_format_interface_get_sequence_fn_t biosal_input_format_interface_get_sequence(struct biosal_input_format_interface *operations);
biosal_input_format_interface_detect_fn_t biosal_input_format_interface_get_detect(struct biosal_input_format_interface *operations);

#endif
