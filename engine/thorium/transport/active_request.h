
#ifndef THORIUM_ACTIVE_BUFFER_H
#define THORIUM_ACTIVE_BUFFER_H

#include "transport.h"

#include "pami/pami_active_request.h"
#include "mpi1_ptp/mpi1_ptp_active_request.h"

/*
 * A transport active request.
 * This is used to store worker buffers and
 * Thorium core buffers.
 */
struct thorium_active_request {

    void *buffer;
    int worker;

    int implementation;

#ifdef THORIUM_TRANSPORT_USE_PAMI
    struct thorium_pami_active_request concrete_object;

#elif defined(THORIUM_TRANSPORT_USE_MPI1_PTP)
    struct thorium_mpi1_ptp_active_request concrete_object;
#endif

    /*
     * Interface
     */
    void (*active_request_init)(struct thorium_active_request *self);
    void (*active_request_destroy)(struct thorium_active_request *self);
    int (*active_request_test)(struct thorium_active_request *self);
    void *(*active_request_request)(struct thorium_active_request *self);
};

void thorium_active_request_init(struct thorium_active_request *self, void *buffer, int worker);
void thorium_active_request_destroy(struct thorium_active_request *self);
int thorium_active_request_test(struct thorium_active_request *self);
void *thorium_active_request_request(struct thorium_active_request *self);
void *thorium_active_request_buffer(struct thorium_active_request *self);
int thorium_active_request_get_worker(struct thorium_active_request *self);

void *thorium_active_request_get_concrete_object(struct thorium_active_request *self);

#endif
