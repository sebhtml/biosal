
#include "input_format_interface.h"

void bsal_input_format_interface_init(struct bsal_input_format_interface *operations)
{
}

void bsal_input_format_interface_destroy(struct bsal_input_format_interface *operations)
{
}

bsal_input_format_interface_init_fn_t bsal_input_format_interface_get_init(struct bsal_input_format_interface *operations)
{
    return operations->init;
}

bsal_input_format_interface_destroy_fn_t bsal_input_format_interface_get_destroy(struct bsal_input_format_interface *operations)
{
    return operations->destroy;
}

bsal_input_format_interface_get_sequence_fn_t bsal_input_format_interface_get_sequence(
                struct bsal_input_format_interface *operations)
{
    return operations->get_sequence;
}

bsal_input_format_interface_detect_fn_t bsal_input_format_interface_get_detect(
                struct bsal_input_format_interface *operations)
{
    return operations->detect;
}
