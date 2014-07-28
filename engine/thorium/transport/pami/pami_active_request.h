
#ifndef BSAL_PAMI_ACTIVE_BUFFER_H
#define BSAL_PAMI_ACTIVE_BUFFER_H

#include "pami_transport.h"

struct bsal_active_request;

struct bsal_pami_active_request {
    int mock;
};

void bsal_pami_active_request_init(struct bsal_active_request *active_request);
void bsal_pami_active_request_destroy(struct bsal_active_request *active_request);
int bsal_pami_active_request_test(struct bsal_active_request *active_request);
void *bsal_pami_active_request_request(struct bsal_active_request *active_request);

#endif
