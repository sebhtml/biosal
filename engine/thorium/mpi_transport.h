
#ifndef BSAL_MPI_TRANSPORT_H
#define BSAL_MPI_TRANSPORT_H

#include <core/structures/ring_queue.h>

#define BSAL_TRANSPORT_USE_MPI

#include <mpi.h>

#define BSAL_TRANSPORT_IMPLEMENTATION_MPI 10

struct bsal_node;
struct bsal_message;
struct bsal_active_buffer;
struct bsal_transport;

struct bsal_mpi_transport {
    MPI_Comm comm;
    MPI_Datatype datatype;

};

void bsal_mpi_transport_init(struct bsal_transport *transport, int *argc, char ***argv);
void bsal_mpi_transport_destroy(struct bsal_transport *transport);
int bsal_mpi_transport_send(struct bsal_transport *transport, struct bsal_message *message);
int bsal_mpi_transport_receive(struct bsal_transport *transport, struct bsal_message *message);

#endif
