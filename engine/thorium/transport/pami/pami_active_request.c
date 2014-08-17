
#include "pami_active_request.h"

#include <engine/thorium/transport/active_request.h>

void thorium_pami_active_request_init(struct thorium_active_request *self)
{
    struct thorium_pami_active_request *pami_active_request;

    pami_active_request = thorium_active_request_get_concrete_object(self);

    pami_active_request->mock = 42;
}

void thorium_pami_active_request_destroy(struct thorium_active_request *self)
{
    struct thorium_pami_active_request *pami_active_request;

    pami_active_request = thorium_active_request_get_concrete_object(self);

    pami_active_request->mock = 42;
}

int thorium_pami_active_request_test(struct thorium_active_request *self)
{
    return 0;
}

void *thorium_pami_active_request_request(struct thorium_active_request *self)
{
    return NULL;
}


