
#include "input_vtable.h"

void bsal_input_vtable_init(struct bsal_input_vtable *vtable)
{
}

void bsal_input_vtable_destroy(struct bsal_input_vtable *vtable)
{
}

bsal_input_init_fn_t bsal_input_vtable_get_init(struct bsal_input_vtable *vtable)
{
    return vtable->init;
}

bsal_input_destroy_fn_t bsal_input_vtable_get_destroy(struct bsal_input_vtable *vtable)
{
    return vtable->destroy;
}

bsal_input_get_sequence_fn_t bsal_input_vtable_get_get_sequence(
                struct bsal_input_vtable *vtable)
{
    return vtable->get_sequence;
}

bsal_input_detect_fn_t bsal_input_vtable_get_detect(
                struct bsal_input_vtable *vtable)
{
    return vtable->detect;
}
