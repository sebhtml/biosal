
#include "pami_active_request.h"

#include <engine/thorium/transport/active_request.h>

void bsal_pami_active_request_init(struct bsal_active_request *self)
{
    struct bsal_pami_active_request *pami_active_request;

    pami_active_request = bsal_active_request_get_concrete_object(self);

    pami_active_request->mock = 42;
}

void bsal_pami_active_request_destroy(struct bsal_active_request *self)
{
    struct bsal_pami_active_request *pami_active_request;

    pami_active_request = bsal_active_request_get_concrete_object(self);

    pami_active_request->mock = 42;
}

int bsal_pami_active_request_test(struct bsal_active_request *self)
{
    return 0;
}

void *bsal_pami_active_request_request(struct bsal_active_request *self)
{
    return NULL;
}


