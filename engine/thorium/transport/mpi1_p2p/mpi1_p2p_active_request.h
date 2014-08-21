
#ifndef THORIUM_MPI1_P2P_ACTIVE_BUFFER_H
#define THORIUM_MPI1_P2P_ACTIVE_BUFFER_H

#include "mpi1_p2p_transport.h"

/*
 * An MPI active request.
 */
struct thorium_mpi1_p2p_active_request {
#ifdef THORIUM_TRANSPORT_USE_MPI1_P2P
    void *buffer;
    MPI_Request request;
    int worker;
#endif
};

void thorium_mpi1_p2p_active_request_init(struct thorium_mpi1_p2p_active_request *self, void *buffer, int worker);
void thorium_mpi1_p2p_active_request_destroy(struct thorium_mpi1_p2p_active_request *self);

void *thorium_mpi1_p2p_active_request_request(struct thorium_mpi1_p2p_active_request *self);
int thorium_mpi1_p2p_active_request_test(struct thorium_mpi1_p2p_active_request *self);

void *thorium_mpi1_p2p_active_request_buffer(struct thorium_mpi1_p2p_active_request *self);
int thorium_mpi1_p2p_active_request_worker(struct thorium_mpi1_p2p_active_request *self);

#endif
