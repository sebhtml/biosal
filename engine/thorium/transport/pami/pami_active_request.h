
#ifndef BSAL_PAMI_ACTIVE_BUFFER_H
#define BSAL_PAMI_ACTIVE_BUFFER_H

#include "pami_transport.h"

struct thorium_active_request;

/*
 * This is a PAMI active request.
 */
struct thorium_pami_active_request {
    int mock;
};

void thorium_pami_active_request_init(struct thorium_active_request *self);
void thorium_pami_active_request_destroy(struct thorium_active_request *self);
int thorium_pami_active_request_test(struct thorium_active_request *self);
void *thorium_pami_active_request_request(struct thorium_active_request *self);

#endif
