
#ifndef BSAL_PAMI_ACTIVE_BUFFER_H
#define BSAL_PAMI_ACTIVE_BUFFER_H

#include "pami_transport.h"

struct bsal_active_buffer;

struct bsal_pami_active_buffer {
    int mock;
};

void bsal_pami_active_buffer_init(struct bsal_active_buffer *active_buffer);
void bsal_pami_active_buffer_destroy(struct bsal_active_buffer *active_buffer);
int bsal_pami_active_buffer_test(struct bsal_active_buffer *active_buffer);
void *bsal_pami_active_buffer_request(struct bsal_active_buffer *active_buffer);

#endif
