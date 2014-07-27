
#include "pami_active_buffer.h"

#include <engine/thorium/transport/active_buffer.h>

void bsal_pami_active_buffer_init(struct bsal_active_buffer *active_buffer)
{
    struct bsal_pami_active_buffer *pami_active_buffer;

    pami_active_buffer = bsal_active_buffer_get_concrete_object(active_buffer);

    pami_active_buffer->mock = 42;
}

void bsal_pami_active_buffer_destroy(struct bsal_active_buffer *active_buffer)
{
    struct bsal_pami_active_buffer *pami_active_buffer;

    pami_active_buffer = bsal_active_buffer_get_concrete_object(active_buffer);

    pami_active_buffer->mock = 42;
}

int bsal_pami_active_buffer_test(struct bsal_active_buffer *active_buffer)
{
    return 0;
}

void *bsal_pami_active_buffer_request(struct bsal_active_buffer *active_buffer)
{
    return NULL;
}


