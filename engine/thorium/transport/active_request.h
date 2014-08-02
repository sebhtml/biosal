
#ifndef BSAL_ACTIVE_BUFFER_H
#define BSAL_ACTIVE_BUFFER_H

#include "transport.h"

#include "pami/pami_active_request.h"
#include "mpi/mpi_active_request.h"

struct bsal_active_request {

    void *buffer;
    int worker;

    int implementation;

#ifdef BSAL_TRANSPORT_USE_PAMI
    struct bsal_pami_active_request concrete_object;

#elif defined(BSAL_TRANSPORT_USE_MPI)
    struct bsal_mpi_active_request concrete_object;
#endif

    /*
     * Interface
     */
    void (*active_request_init)(struct bsal_active_request *self);
    void (*active_request_destroy)(struct bsal_active_request *self);
    int (*active_request_test)(struct bsal_active_request *self);
    void *(*active_request_request)(struct bsal_active_request *self);

};

void bsal_active_request_init(struct bsal_active_request *self, void *buffer, int worker);
void bsal_active_request_destroy(struct bsal_active_request *self);
int bsal_active_request_test(struct bsal_active_request *self);
void *bsal_active_request_request(struct bsal_active_request *self);
void *bsal_active_request_buffer(struct bsal_active_request *self);
int bsal_active_request_get_worker(struct bsal_active_request *self);

void *bsal_active_request_get_concrete_object(struct bsal_active_request *self);


#endif
