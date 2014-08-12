
#ifndef BSAL_TRANSPORT_H
#define BSAL_TRANSPORT_H

#include "transport_interface.h"

#include "pami/pami_transport.h"
#include "mpi/mpi_transport.h"

#define BSAL_TRANSPORT_IMPLEMENTATION_MOCK 0

#define BSAL_THREAD_SINGLE 0
#define BSAL_THREAD_FUNNELED 1
#define BSAL_THREAD_SERIALIZED 2
#define BSAL_THREAD_MULTIPLE 3

#define BSAL_TRANSPORT_MOCK_IDENTIFIER (-1)

struct bsal_active_request;
struct bsal_worker_buffer;

/*
 * This is the transport layer in
 * the thorium actor engine
 */
struct bsal_transport {

    /* Common stuff
     */
    struct bsal_node *node;
    struct bsal_ring_queue active_requests;
    int provided;
    int rank;
    int size;

    struct bsal_memory_pool *inbound_message_memory_pool;

    struct bsal_pami_transport pami_transport;
    struct bsal_mpi_transport mpi_transport;

    int implementation;

    void *concrete_transport;
    struct bsal_transport_interface *transport_interface;
};

void bsal_transport_init(struct bsal_transport *self, struct bsal_node *node,
                int *argc, char ***argv,
                struct bsal_memory_pool *inbound_message_memory_pool);
void bsal_transport_destroy(struct bsal_transport *self);
int bsal_transport_send(struct bsal_transport *self, struct bsal_message *message);
int bsal_transport_receive(struct bsal_transport *self, struct bsal_message *message);
void bsal_transport_resolve(struct bsal_transport *self, struct bsal_message *message);

int bsal_transport_get_provided(struct bsal_transport *self);
int bsal_transport_get_rank(struct bsal_transport *self);
int bsal_transport_get_size(struct bsal_transport *self);

int bsal_transport_get_identifier(struct bsal_transport *self);
const char *bsal_transport_get_name(struct bsal_transport *self);

int bsal_transport_test_requests(struct bsal_transport *self, struct bsal_worker_buffer *worker_buffer);
int bsal_transport_get_active_request_count(struct bsal_transport *self);

int bsal_transport_dequeue_active_request(struct bsal_transport *self, struct bsal_active_request *active_request);

int bsal_transport_get_implementation(struct bsal_transport *self);


void *bsal_transport_get_concrete_transport(struct bsal_transport *self);
void bsal_transport_set(struct bsal_transport *self);

void bsal_transport_prepare_received_message(struct bsal_transport *self, struct bsal_message *message,
                int source, int tag, int count, void *buffer);

void bsal_transport_select(struct bsal_transport *self);

void bsal_transport_print(struct bsal_transport *self);

#endif
