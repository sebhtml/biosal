
#include "transport.h"

#include <core/system/command.h>
#include <core/helpers/bitmap.h>

#include "active_request.h"

#include <engine/thorium/worker_buffer.h>
#include <engine/thorium/message.h>
#include <engine/thorium/node.h>

#include <core/system/debugger.h>

#define FLAG_PROFILE 0

void bsal_transport_init(struct bsal_transport *self, struct bsal_node *node,
                int *argc, char ***argv,
                struct bsal_memory_pool *inbound_message_memory_pool)
{
    int actual_argc;
    char **actual_argv;

    actual_argc = *argc;
    actual_argv = *argv;

    self->flags = 0;
    bsal_bitmap_set_bit_value_uint32_t(&self->flags, FLAG_PROFILE, 0);

    self->transport_interface = NULL;
    self->concrete_transport = NULL;

    /*
    printf("DEBUG Initiating transport\n");
    */
    /* Select the transport layer
     */
    bsal_transport_select(self);

    /*
     * Assign functions
     */
    bsal_transport_set(self);

    self->node = node;
    bsal_ring_queue_init(&self->active_requests, sizeof(struct bsal_active_request));

    self->rank = -1;
    self->size = -1;

    if (self->transport_interface != NULL) {

        self->concrete_transport = bsal_memory_allocate(self->transport_interface->size);
        self->transport_interface->init(self, argc, argv);
    }

    BSAL_DEBUGGER_ASSERT(self->rank >= 0);
    BSAL_DEBUGGER_ASSERT(self->size >= 1);
    BSAL_DEBUGGER_ASSERT(self->node != NULL);

    self->inbound_message_memory_pool = inbound_message_memory_pool;

    thorium_transport_profiler_init(&self->transport_profiler);

    if (bsal_command_has_argument(actual_argc, actual_argv, "-enable-transport-profiler")) {

        printf("Enable transport profiler\n");

        bsal_bitmap_set_bit_value_uint32_t(&self->flags, FLAG_PROFILE, 1);
    }
}

void bsal_transport_destroy(struct bsal_transport *self)
{
    struct bsal_active_request active_request;

    /*
     * Print the report if requested.
     */
    if (bsal_bitmap_get_bit_value_uint32_t(&self->flags, FLAG_PROFILE)) {
        thorium_transport_profiler_print_report(&self->transport_profiler);
    }

    thorium_transport_profiler_destroy(&self->transport_profiler);

    BSAL_DEBUGGER_ASSERT(bsal_transport_get_active_request_count(self) == 0);

    if (self->transport_interface != NULL) {
        self->transport_interface->destroy(self);

        bsal_memory_free(self->concrete_transport);
        self->concrete_transport = NULL;
    }

    while (bsal_ring_queue_dequeue(&self->active_requests, &active_request)) {
        bsal_active_request_destroy(&active_request);
    }

    bsal_ring_queue_destroy(&self->active_requests);

    self->node = NULL;
    self->rank = -1;
    self->size = -1;
}

int bsal_transport_send(struct bsal_transport *self, struct bsal_message *message)
{
    if (self->transport_interface == NULL) {
        return 0;
    }

    /*
     * Send the message through the mock transport which is
     * a transport profiler.
     */
    if (bsal_bitmap_get_bit_value_uint32_t(&self->flags, FLAG_PROFILE)) {
        thorium_transport_profiler_send_mock(&self->transport_profiler, message);
    }

    return self->transport_interface->send(self, message);
}

int bsal_transport_receive(struct bsal_transport *self, struct bsal_message *message)
{
    if (self->transport_interface == NULL) {
        return 0;
    }

    return self->transport_interface->receive(self, message);
}

int bsal_transport_get_provided(struct bsal_transport *self)
{
    return self->provided;
}

int bsal_transport_get_rank(struct bsal_transport *self)
{
    return self->rank;
}

int bsal_transport_get_size(struct bsal_transport *self)
{
    return self->size;
}

int bsal_transport_test_requests(struct bsal_transport *self, struct bsal_worker_buffer *worker_buffer)
{
    struct bsal_active_request active_request;
    void *buffer;
    int worker;

    if (bsal_ring_queue_dequeue(&self->active_requests, &active_request)) {

        if (bsal_active_request_test(&active_request)) {

            worker = bsal_active_request_get_worker(&active_request);
            buffer = bsal_active_request_buffer(&active_request);

            bsal_worker_buffer_init(worker_buffer, worker, buffer);

            return 1;

        /* Just put it back in the FIFO for later */
        } else {
            bsal_ring_queue_enqueue(&self->active_requests, &active_request);

            return 0;
        }
    }

    return 0;
}

int bsal_transport_dequeue_active_request(struct bsal_transport *self, struct bsal_active_request *active_request)
{
    return bsal_ring_queue_dequeue(&self->active_requests, active_request);
}

int bsal_transport_get_implementation(struct bsal_transport *self)
{
    return self->implementation;
}

void *bsal_transport_get_concrete_transport(struct bsal_transport *self)
{
    return self->concrete_transport;
}

void bsal_transport_set(struct bsal_transport *self)
{
    self->transport_interface = NULL;

    if (self->implementation == bsal_pami_transport_implementation.identifier) {
        self->transport_interface = &bsal_pami_transport_implementation;

    } else if (self->implementation == bsal_mpi_transport_implementation.identifier) {

        self->transport_interface = &bsal_mpi_transport_implementation;
    }
}

int bsal_transport_get_active_request_count(struct bsal_transport *self)
{
    return bsal_ring_queue_size(&self->active_requests);
}

int bsal_transport_get_identifier(struct bsal_transport *self)
{
    if (self->transport_interface == NULL) {
        return -1;
    }

    return self->transport_interface->identifier;
}

const char *bsal_transport_get_name(struct bsal_transport *self)
{
    if (self->transport_interface == NULL) {
        return NULL;
    }

    return self->transport_interface->name;
}

void bsal_transport_select(struct bsal_transport *self)
{
    self->implementation = BSAL_TRANSPORT_MOCK_IDENTIFIER;

#if defined(BSAL_TRANSPORT_USE_PAMI)
    self->implementation = BSAL_TRANSPORT_PAMI_IDENTIFIER;

#elif defined(BSAL_TRANSPORT_USE_MPI)
    self->implementation = BSAL_TRANSPORT_MPI_IDENTIFIER;
#endif

    if (self->implementation == BSAL_TRANSPORT_MOCK_IDENTIFIER) {
        printf("Error: no transport implementation is available.\n");
        exit(1);
    }

    /*
    printf("DEBUG Transport is %d\n",
                    self->implementation);
                    */

}

void bsal_transport_print(struct bsal_transport *self)
{
    printf("%s TRANSPORT Rank: %d RankCount: %d Implementation: %s\n",
                    BSAL_NODE_THORIUM_PREFIX,
                self->rank, self->size,
                bsal_transport_get_name(self));
}

