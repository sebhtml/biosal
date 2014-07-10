
#ifndef BSAL_TRANSPORT_H
#define BSAL_TRANSPORT_H

#include "active_buffer.h"

#include <core/structures/ring_queue.h>

#include <mpi.h>

struct bsal_node;
struct bsal_message;

struct bsal_transport {
    struct bsal_node *node;
    struct bsal_ring_queue active_buffers;
    MPI_Comm comm;
    MPI_Datatype datatype;
    int provided;
    int rank;
    int size;
};

void bsal_transport_init(struct bsal_transport *self, struct bsal_node *node,
                int *argc, char ***argv);
void bsal_transport_destroy(struct bsal_transport *self);
void bsal_transport_send(struct bsal_transport *self, struct bsal_message *message);
int bsal_transport_receive(struct bsal_transport *self, struct bsal_message *message);
void bsal_transport_resolve(struct bsal_transport *self, struct bsal_message *message);

int bsal_transport_get_provided(struct bsal_transport *self);
int bsal_transport_get_rank(struct bsal_transport *self);
int bsal_transport_get_size(struct bsal_transport *self);

int bsal_transport_test_requests(struct bsal_transport *self, struct bsal_active_buffer *active_buffer);

#endif
