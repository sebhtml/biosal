
#ifndef THORIUM_MPI1_PT2PT_TRANSPORT_H
#define THORIUM_MPI1_PT2PT_TRANSPORT_H

#include <engine/thorium/transport/transport_interface.h>
#include <core/structures/fast_queue.h>

#include <mpi.h>

struct thorium_node;
struct thorium_message;
struct biosal_active_buffer;
struct thorium_transport;

/*
 * MPI 1 point-to-point transport layer.
 */
struct thorium_mpi1_pt2pt_transport {
    struct core_fast_queue active_requests;
    MPI_Comm comm;
    MPI_Datatype datatype;
};

extern struct thorium_transport_interface thorium_mpi1_pt2pt_transport_implementation;

#endif
