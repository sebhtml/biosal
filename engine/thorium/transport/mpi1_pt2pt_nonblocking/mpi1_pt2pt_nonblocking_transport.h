
#ifndef THORIUM_MPI1_PT2PT_NONBLOCKING_TRANSPORT_H
#define THORIUM_MPI1_PT2PT_NONBLOCKING_TRANSPORT_H

#include <engine/thorium/transport/transport_interface.h>
#include <core/structures/ring_queue.h>

#define THORIUM_TRANSPORT_USE_MPI1_PT2PT_NONBLOCKING

#include <mpi.h>

struct thorium_node;
struct thorium_message;
struct bsal_active_buffer;
struct thorium_transport;

struct thorium_mpi1_send_request {
    MPI_Request request;
    int worker;
    void *buffer;
};

struct thorium_mpi1_receive_request{
    MPI_Request request;
    void *buffer;
};

/*
 * MPI 1 point-to-point transport layer
 * using nonblocking communication.
 */
struct thorium_mpi1_pt2pt_nonblocking_transport {
    struct bsal_ring_queue send_requests;
    struct bsal_ring_queue receive_requests;
    MPI_Comm comm;
    MPI_Datatype datatype;
    int threshold;
};

extern struct thorium_transport_interface thorium_mpi1_pt2pt_nonblocking_transport_implementation;

void thorium_mpi1_pt2pt_nonblocking_transport_init(struct thorium_transport *self, int *argc, char ***argv);
void thorium_mpi1_pt2pt_nonblocking_transport_destroy(struct thorium_transport *self);

int thorium_mpi1_pt2pt_nonblocking_transport_send(struct thorium_transport *self, struct thorium_message *message);
int thorium_mpi1_pt2pt_nonblocking_transport_receive(struct thorium_transport *self, struct thorium_message *message);

int thorium_mpi1_pt2pt_nonblocking_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer);

#endif
