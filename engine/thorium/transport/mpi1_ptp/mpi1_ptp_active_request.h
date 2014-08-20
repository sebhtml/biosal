
#ifndef THORIUM_MPI1_PTP_ACTIVE_BUFFER_H
#define THORIUM_MPI1_PTP_ACTIVE_BUFFER_H

#include "mpi1_ptp_transport.h"

struct thorium_active_request;

struct thorium_mpi1_ptp_active_request {
#ifdef THORIUM_TRANSPORT_USE_MPI1_PTP
    MPI_Request request;
#else
    int mock;
#endif
};

void thorium_mpi1_ptp_active_request_init(struct thorium_active_request *self);
void thorium_mpi1_ptp_active_request_destroy(struct thorium_active_request *self);
int thorium_mpi1_ptp_active_request_test(struct thorium_active_request *self);
void *thorium_mpi1_ptp_active_request_request(struct thorium_active_request *self);

#endif
