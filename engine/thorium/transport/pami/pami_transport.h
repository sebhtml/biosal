
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
#define NUM_RECV_BUFFERS_LARGE 64
#define NUM_RECV_BUFFERS_LARGE_POINTERS 512
#define RECV_BUFFER_SIZE_SMALL 4096
#define NUM_RECV_BUFFERS_SMALL 4096
#define NUM_RECV_BUFFERS_SMALL_POINTERS 8192
#define NUM_RECV_COOKIES 65536
#define NUM_SEND_COOKIES 65536

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
    volatile char *buffer;
    int buf_index;
    volatile int count;
    volatile int source;
    volatile int dest;
} thorium_recv_info_t;

typedef struct {
    struct bsal_fast_queue *recv_queue;
    volatile thorium_recv_info_t recv_info;
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
   
    char **recv_buffers_small;
    char **recv_buffers_large;
    thorium_send_cookie_t *send_cookies;
    thorium_recv_cookie_t *recv_cookies;
    struct bsal_fast_queue *send_queue;
    struct bsal_fast_queue *recv_queue;
    int send_index;
    volatile int recv_index;
    volatile int num_preallocated_large_buffers;
    volatile int num_preallocated_small_buffers;
    volatile int num_avail_large_buffers;
    volatile int num_avail_small_buffers;
    int *avail_large_buffers;
    int *avail_small_buffers;

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

void thorium_recv_done_fn (pami_context_t   context,
        void           * cookie,
        pami_result_t    result);

void thorium_send_done_fn (pami_context_t   context,
        void           * cookie,
        pami_result_t    result);

void thorium_recv_message_fn (
        pami_context_t    context,      /**< IN: PAMI context */
        void            * cookie,       /**< IN: dispatch cookie */
        const void      * header,       /**< IN: header address */
        size_t            header_size,  /**< IN: header size */
        const void      * data,         /**< IN: address of PAMI pipe buffer */
        size_t            data_size,    /**< IN: size of PAMI pipe buffer */
        pami_endpoint_t   origin,
        pami_recv_t     * recv);        /**< OUT: receive message structure */

#endif
