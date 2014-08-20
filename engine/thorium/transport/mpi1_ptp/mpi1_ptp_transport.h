
#ifndef THORIUM_MPI1_PTP_TRANSPORT_H
#define THORIUM_MPI1_PTP_TRANSPORT_H

#include <engine/thorium/transport/transport_interface.h>
#include <core/structures/ring_queue.h>

#define THORIUM_TRANSPORT_USE_MPI1_PTP
#define THORIUM_TRANSPORT_MPI1_PTP_IDENTIFIER 10

#include <mpi.h>

struct thorium_node;
struct thorium_message;
struct bsal_active_buffer;
struct thorium_transport;

/*
 * MPI 1 point-to-point transport layer.
 */
struct thorium_mpi1_ptp_transport {
    struct bsal_ring_queue active_requests;
    MPI_Comm comm;
    MPI_Datatype datatype;
};

extern struct thorium_transport_interface thorium_mpi1_ptp_transport_implementation;

void thorium_mpi1_ptp_transport_init(struct thorium_transport *self, int *argc, char ***argv);
void thorium_mpi1_ptp_transport_destroy(struct thorium_transport *self);

int thorium_mpi1_ptp_transport_send(struct thorium_transport *self, struct thorium_message *message);
int thorium_mpi1_ptp_transport_receive(struct thorium_transport *self, struct thorium_message *message);

int thorium_mpi1_ptp_transport_get_identifier(struct thorium_transport *self);
const char *thorium_mpi1_ptp_transport_get_name(struct thorium_transport *self);

int thorium_mpi1_ptp_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer);
int thorium_mpi1_ptp_transport_get_active_request_count(struct thorium_transport *self);

#endif
