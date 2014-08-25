
#ifndef THORIUM_MPI1_PT2PT_NONBLOCKING_TRANSPORT_H
#define THORIUM_MPI1_PT2PT_NONBLOCKING_TRANSPORT_H

#include <engine/thorium/transport/transport_interface.h>
#include <core/structures/fast_queue.h>

#define THORIUM_TRANSPORT_USE_MPI1_PT2PT_NONBLOCKING

#include <mpi.h>

struct thorium_node;
struct thorium_message;
struct bsal_active_buffer;
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

    struct bsal_fast_queue send_requests;
    struct bsal_fast_queue receive_requests;

    MPI_Comm communicator;
    MPI_Datatype datatype;

    int maximum_buffer_size;
    int maximum_receive_request_count;
    int small_request_count;

    int maximum_big_receive_request_count;
    int big_request_count;

    int current_big_tag;
    int mpi_tag_ub;
};

extern struct thorium_transport_interface thorium_mpi1_pt2pt_nonblocking_transport_implementation;

void thorium_mpi1_pt2pt_nonblocking_transport_init(struct thorium_transport *self, int *argc, char ***argv);
void thorium_mpi1_pt2pt_nonblocking_transport_destroy(struct thorium_transport *self);

int thorium_mpi1_pt2pt_nonblocking_transport_send(struct thorium_transport *self, struct thorium_message *message);
int thorium_mpi1_pt2pt_nonblocking_transport_receive(struct thorium_transport *self, struct thorium_message *message);

int thorium_mpi1_pt2pt_nonblocking_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer);

void thorium_mpi1_pt2pt_nonblocking_transport_add_receive_request(struct thorium_transport *self, int tag, int count, int source);

int thorium_mpi1_pt2pt_nonblocking_transport_get_big_tag(struct thorium_transport *self);

#endif
