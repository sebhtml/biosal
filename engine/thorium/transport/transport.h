
#ifndef THORIUM_TRANSPORT_H
#define THORIUM_TRANSPORT_H

#include "transport_interface.h"
#include "transport_profiler.h"

#include "pami/pami_transport.h"
#include "mpi1_p2p/mpi1_p2p_transport.h"
#include "mpi1_pt2pt_nonblocking/mpi1_pt2pt_nonblocking_transport.h"

#define THORIUM_TRANSPORT_IMPLEMENTATION_MOCK 0

#define THORIUM_THREAD_SINGLE 0
#define THORIUM_THREAD_FUNNELED 1
#define THORIUM_THREAD_SERIALIZED 2
#define THORIUM_THREAD_MULTIPLE 3

#define THORIUM_TRANSPORT_MOCK_IDENTIFIER (-1)

struct thorium_active_request;
struct thorium_worker_buffer;

/*
 * This is the transport layer in
 * the thorium actor engine
 */
struct thorium_transport {

    /* Common stuff
     */
    struct thorium_node *node;
    struct thorium_transport_profiler transport_profiler;
    int provided;
    int rank;
    int size;

    struct bsal_memory_pool *inbound_message_memory_pool;
    struct bsal_memory_pool *outbound_message_memory_pool;

    struct thorium_pami_transport pami_transport;
    struct thorium_mpi1_p2p_transport mpi_transport;

    void *concrete_transport;
    struct thorium_transport_interface *transport_interface;
    int active_request_count;

    uint32_t flags;
};

void thorium_transport_init(struct thorium_transport *self, struct thorium_node *node,
                int *argc, char ***argv,
                struct bsal_memory_pool *inbound_message_memory_pool,
                struct bsal_memory_pool *outbound_message_memory_pool);

void thorium_transport_destroy(struct thorium_transport *self);

int thorium_transport_send(struct thorium_transport *self, struct thorium_message *message);
int thorium_transport_receive(struct thorium_transport *self, struct thorium_message *message);

int thorium_transport_get_provided(struct thorium_transport *self);
int thorium_transport_get_rank(struct thorium_transport *self);
int thorium_transport_get_size(struct thorium_transport *self);

int thorium_transport_get_identifier(struct thorium_transport *self);
const char *thorium_transport_get_name(struct thorium_transport *self);

int thorium_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer);

int thorium_transport_get_active_request_count(struct thorium_transport *self);

void *thorium_transport_get_concrete_transport(struct thorium_transport *self);
void thorium_transport_set(struct thorium_transport *self);

void thorium_transport_print(struct thorium_transport *self);

#endif
