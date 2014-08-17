
#ifndef BSAL_MPI_TRANSPORT_H
#define BSAL_MPI_TRANSPORT_H

#include <engine/thorium/transport/transport_interface.h>
#include <core/structures/ring_queue.h>

#define BSAL_TRANSPORT_USE_MPI
#define BSAL_TRANSPORT_MPI_IDENTIFIER 10

#include <mpi.h>

struct thorium_node;
struct thorium_message;
struct bsal_active_buffer;
struct thorium_transport;

struct thorium_mpi_transport {
    MPI_Comm comm;
    MPI_Datatype datatype;
};

extern struct thorium_transport_interface thorium_mpi_transport_implementation;

void thorium_mpi_transport_init(struct thorium_transport *self, int *argc, char ***argv);
void thorium_mpi_transport_destroy(struct thorium_transport *self);

int thorium_mpi_transport_send(struct thorium_transport *self, struct thorium_message *message);
int thorium_mpi_transport_receive(struct thorium_transport *self, struct thorium_message *message);

int thorium_mpi_transport_get_identifier(struct thorium_transport *self);
const char *thorium_mpi_transport_get_name(struct thorium_transport *self);

#endif
