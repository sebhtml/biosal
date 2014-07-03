
#include "input_operations.h"

void bsal_input_operations_init(struct bsal_input_operations *operations)
{
}

void bsal_input_operations_destroy(struct bsal_input_operations *operations)
{
}

bsal_input_init_fn_t bsal_input_operations_get_init(struct bsal_input_operations *operations)
{
    return operations->init;
}

bsal_input_destroy_fn_t bsal_input_operations_get_destroy(struct bsal_input_operations *operations)
{
    return operations->destroy;
}

bsal_input_get_sequence_fn_t bsal_input_operations_get_get_sequence(
                struct bsal_input_operations *operations)
{
    return operations->get_sequence;
}

bsal_input_detect_fn_t bsal_input_operations_get_detect(
                struct bsal_input_operations *operations)
{
    return operations->detect;
}
