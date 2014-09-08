
#ifndef THORIUM_PAMI_TRANSPORT_H
#define THORIUM_PAMI_TRANSPORT_H

#include <engine/thorium/transport/transport_interface.h>
#include <core/structures/fast_queue.h>

/*
 *  * Decide if the code will use PAMI or MPI
 *   */

/*
 *  * This variable is set to 1 if PAMI support is ready to be
 *   * used
 *    */
#define THORIUM_TRANSPORT_PAMI_IS_READY 1

/*
 *  * Use IBM PAMI on IBM Blue Gene/Q
 *   * PAMI: Parallel Active Message Interface
 *    */
#if defined(__bgq__) && THORIUM_TRANSPORT_PAMI_IS_READY

#define THORIUM_TRANSPORT_USE_PAMI

#endif

#if defined(THORIUM_TRANSPORT_USE_PAMI)

#include <pami.h>

#define MAX_SHORT_MESSAGE_LENGTH 128

#define RECV_BUFFER_SIZE_LARGE 4194304
#define NUM_RECV_BUFFERS_LARGE 16

#define RECV_BUFFER_SIZE_MEDIUM 4096
#define NUM_RECV_BUFFERS_MEDIUM 4096

#define RECV_BUFFER_SIZE_SMALL 128
#define NUM_RECV_BUFFERS_SMALL 131072

#define NUM_RECV_COOKIES 131072
#define NUM_SEND_COOKIES 131072

#define RECV_MESSAGE_DISPATCH_ID 17

#endif

struct thorium_node;
struct thorium_message;
struct thorium_transport;
struct thorium_worker_buffer;

typedef struct {
    char *buffer;
    int worker;
} thorium_send_info_t;

typedef struct {
    struct bsal_fast_queue *send_queue;
    thorium_send_info_t send_info;
} thorium_send_cookie_t;

typedef struct {
    char *buffer;
    int count;
    int source;
    int dest;
} thorium_recv_info_t;

typedef struct {
    struct bsal_fast_queue *recv_queue;
    thorium_recv_info_t recv_info;
} thorium_recv_cookie_t;

/*
 *  * Transport using IBM PAMI (Parallel Active Message Interface)
 *   */
struct thorium_pami_transport {
#ifdef THORIUM_TRANSPORT_USE_PAMI
    pami_client_t client;
    pami_context_t context;
    size_t num_contexts;

    int rank; 

    pami_endpoint_t *endpoints;
  
    thorium_send_cookie_t **send_cookies;
    struct bsal_fast_queue *avail_send_cookies_queue;
    struct bsal_fast_queue *in_use_send_cookies_queue;

    thorium_recv_cookie_t **recv_cookies;
    struct bsal_fast_queue *avail_recv_cookies_queue;
    struct bsal_fast_queue *in_use_recv_cookies_queue;

    struct bsal_fast_queue *large_buffer_queue;
    struct bsal_fast_queue *medium_buffer_queue;
    struct bsal_fast_queue *small_buffer_queue;

    char **large_buffers;
    char **medium_buffers;
    char **small_buffers;
#endif

    /*
 *      * Avoid having nothing in the structure.
 *           */
    int mock;
};

extern struct thorium_transport_interface thorium_pami_transport_implementation;

void thorium_pami_transport_init(struct thorium_transport *self, int *argc, char ***argv);
void thorium_pami_transport_destroy(struct thorium_transport *self);

int thorium_pami_transport_send(struct thorium_transport *self, struct thorium_message *message);
int thorium_pami_transport_receive(struct thorium_transport *self, struct thorium_message *message);

int thorium_pami_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *buffer);

void thorium_pami_transport_mem_pool_alloc(struct thorium_pami_transport *pami_transport, int data_size, void *buffer);

void thorium_pami_transport_mem_pool_return(struct thorium_pami_transport *pami_transport, int data_size, void *buffer);

void thorium_recv_done_fn (pami_context_t   context,
        void           *cookie,
        pami_result_t    result);

void thorium_send_done_fn (pami_context_t   context,
        void           *cookie,
        pami_result_t    result);

void thorium_recv_message_fn (
        pami_context_t    context,      /**< IN: PAMI context */
        void            *cookie,       /**< IN: dispatch cookie */
        const void      *header,       /**< IN: header address */
        size_t            header_size,  /**< IN: header size */
        const void      *data,         /**< IN: address of PAMI pipe buffer */
        size_t            data_size,    /**< IN: size of PAMI pipe buffer */
        pami_endpoint_t   origin,
        pami_recv_t     *recv);        /**< OUT: receive message structure */

#endif
