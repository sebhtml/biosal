
#ifndef THORIUM_MPI1_PT2PT_NONBLOCKING_TRANSPORT_H
#define THORIUM_MPI1_PT2PT_NONBLOCKING_TRANSPORT_H

#include <engine/thorium/transport/transport_interface.h>
#include <core/structures/fast_queue.h>

#define THORIUM_TRANSPORT_USE_MPI1_PT2PT_NONBLOCKING

#include <mpi.h>

struct thorium_node;
struct thorium_message;
struct biosal_active_buffer;
struct thorium_transport;

/*
 * MPI 1 point-to-point transport layer
 * using nonblocking communication.
 *
 * Designed by: Pavan Balaji
 * Implemented by: Sebastien Boisvert
 * August 2014
 */
struct thorium_mpi1_pt2pt_nonblocking_transport {

    struct core_fast_queue send_requests;
    struct core_fast_queue receive_requests;

    MPI_Comm communicator;
    MPI_Datatype datatype;

    int probe_operation_count;
    int maximum_buffer_size;
    int maximum_receive_request_count;
    int small_request_count;

    int maximum_big_receive_request_count;
    int big_request_count;

    int current_big_tag;
    int mpi_tag_ub;
};

extern struct thorium_transport_interface thorium_mpi1_pt2pt_nonblocking_transport_implementation;

#endif
