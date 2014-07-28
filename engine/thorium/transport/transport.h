
#ifndef BSAL_TRANSPORT_H
#define BSAL_TRANSPORT_H

#include "pami/pami_transport.h"
#include "mpi/mpi_transport.h"

#define BSAL_TRANSPORT_IMPLEMENTATION_MOCK 0

#define BSAL_THREAD_SINGLE 0
#define BSAL_THREAD_FUNNELED 1
#define BSAL_THREAD_SERIALIZED 2
#define BSAL_THREAD_MULTIPLE 3

#define BSAL_TRANSPORT_MOCK_IDENTIFIER (-1)

/*
 * This is the transport layer in
 * the thorium actor engine
 */
struct bsal_transport {

    /* Common stuff
     */
    struct bsal_node *node;
    struct bsal_ring_queue active_buffers;
    int provided;
    int rank;
    int size;

    int implementation;

#if defined(BSAL_TRANSPORT_USE_PAMI)
    struct bsal_pami_transport concrete_object;

#elif defined(BSAL_TRANSPORT_USE_MPI)
    struct bsal_mpi_transport concrete_object;
#endif

    void (*transport_init)(struct bsal_transport *transport, int *argc, char ***argv);
    void (*transport_destroy)(struct bsal_transport *transport);

    int (*transport_send)(struct bsal_transport *transport, struct bsal_message *message);
    int (*transport_receive)(struct bsal_transport *transport, struct bsal_message *message);

    int (*transport_get_identifier)(struct bsal_transport *transport);
    const char *(*transport_get_name)(struct bsal_transport *transport);
};

void bsal_transport_init(struct bsal_transport *transport, struct bsal_node *node,
                int *argc, char ***argv);
void bsal_transport_destroy(struct bsal_transport *transport);
int bsal_transport_send(struct bsal_transport *transport, struct bsal_message *message);
int bsal_transport_receive(struct bsal_transport *transport, struct bsal_message *message);
void bsal_transport_resolve(struct bsal_transport *transport, struct bsal_message *message);

int bsal_transport_get_provided(struct bsal_transport *transport);
int bsal_transport_get_rank(struct bsal_transport *transport);
int bsal_transport_get_size(struct bsal_transport *transport);

int bsal_transport_get_identifier(struct bsal_transport *transport);
const char *bsal_transport_get_name(struct bsal_transport *transport);

int bsal_transport_test_requests(struct bsal_transport *transport, struct bsal_active_buffer *active_buffer);
int bsal_transport_get_active_buffer_count(struct bsal_transport *transport);

int bsal_transport_dequeue_active_buffer(struct bsal_transport *transport, struct bsal_active_buffer *active_buffer);

int bsal_transport_get_implementation(struct bsal_transport *transport);


void *bsal_transport_get_concrete_transport(struct bsal_transport *transport);
void bsal_transport_set_functions(struct bsal_transport *transport);

void bsal_transport_configure_mpi(struct bsal_transport *transport);
void bsal_transport_configure_pami(struct bsal_transport *transport);
void bsal_transport_configure_mock(struct bsal_transport *transport);

void bsal_transport_prepare_received_message(struct bsal_transport *transport, struct bsal_message *message,
                int source, int tag, int count, void *buffer);

void bsal_transport_select(struct bsal_transport *transport);

#endif
