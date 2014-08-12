
#ifndef BSAL_MPI_TRANSPORT_H
#define BSAL_MPI_TRANSPORT_H

#include <engine/thorium/transport/transport_interface.h>
#include <core/structures/ring_queue.h>

#define BSAL_TRANSPORT_USE_MPI
#define BSAL_TRANSPORT_MPI_IDENTIFIER 10

#include <mpi.h>

struct bsal_node;
struct bsal_message;
struct bsal_active_buffer;
struct bsal_transport;

struct bsal_mpi_transport {
    MPI_Comm comm;
    MPI_Datatype datatype;
};

extern struct bsal_transport_interface bsal_mpi_transport_implementation;

void bsal_mpi_transport_init(struct bsal_transport *self, int *argc, char ***argv);
void bsal_mpi_transport_destroy(struct bsal_transport *self);

int bsal_mpi_transport_send(struct bsal_transport *self, struct bsal_message *message);
int bsal_mpi_transport_receive(struct bsal_transport *self, struct bsal_message *message);

int bsal_mpi_transport_get_identifier(struct bsal_transport *self);
const char *bsal_mpi_transport_get_name(struct bsal_transport *self);

#endif
