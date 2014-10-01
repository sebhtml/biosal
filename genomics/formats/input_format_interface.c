
#include "input_format_interface.h"

void biosal_input_format_interface_init(struct biosal_input_format_interface *operations)
{
}

void biosal_input_format_interface_destroy(struct biosal_input_format_interface *operations)
{
}

biosal_input_format_interface_init_fn_t biosal_input_format_interface_get_init(struct biosal_input_format_interface *operations)
{
    return operations->init;
}

biosal_input_format_interface_destroy_fn_t biosal_input_format_interface_get_destroy(struct biosal_input_format_interface *operations)
{
    return operations->destroy;
}

biosal_input_format_interface_get_sequence_fn_t biosal_input_format_interface_get_sequence(
                struct biosal_input_format_interface *operations)
{
    return operations->get_sequence;
}

biosal_input_format_interface_detect_fn_t biosal_input_format_interface_get_detect(
                struct biosal_input_format_interface *operations)
{
    return operations->detect;
}
