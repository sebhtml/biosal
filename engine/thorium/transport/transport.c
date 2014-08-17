
#include "transport.h"

#include <core/system/command.h>
#include <core/helpers/bitmap.h>

#include "active_request.h"

#include <engine/thorium/worker_buffer.h>
#include <engine/thorium/message.h>
#include <engine/thorium/node.h>

#include <core/system/debugger.h>

#define FLAG_PROFILE 0

void thorium_transport_init(struct thorium_transport *self, struct thorium_node *node,
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
    thorium_transport_select(self);

    /*
     * Assign functions
     */
    thorium_transport_set(self);

    self->node = node;
    bsal_ring_queue_init(&self->active_requests, sizeof(struct thorium_active_request));

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

void thorium_transport_destroy(struct thorium_transport *self)
{
    struct thorium_active_request active_request;

    /*
     * Print the report if requested.
     */
    if (bsal_bitmap_get_bit_value_uint32_t(&self->flags, FLAG_PROFILE)) {
        thorium_transport_profiler_print_report(&self->transport_profiler);
    }

    thorium_transport_profiler_destroy(&self->transport_profiler);

    BSAL_DEBUGGER_ASSERT(thorium_transport_get_active_request_count(self) == 0);

    if (self->transport_interface != NULL) {
        self->transport_interface->destroy(self);

        bsal_memory_free(self->concrete_transport);
        self->concrete_transport = NULL;
    }

    while (bsal_ring_queue_dequeue(&self->active_requests, &active_request)) {
        thorium_active_request_destroy(&active_request);
    }

    bsal_ring_queue_destroy(&self->active_requests);

    self->node = NULL;
    self->rank = -1;
    self->size = -1;
}

int thorium_transport_send(struct thorium_transport *self, struct thorium_message *message)
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

int thorium_transport_receive(struct thorium_transport *self, struct thorium_message *message)
{
    if (self->transport_interface == NULL) {
        return 0;
    }

    return self->transport_interface->receive(self, message);
}

int thorium_transport_get_provided(struct thorium_transport *self)
{
    return self->provided;
}

int thorium_transport_get_rank(struct thorium_transport *self)
{
    return self->rank;
}

int thorium_transport_get_size(struct thorium_transport *self)
{
    return self->size;
}

int thorium_transport_test_requests(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer)
{
    struct thorium_active_request active_request;
    void *buffer;
    int worker;

    if (bsal_ring_queue_dequeue(&self->active_requests, &active_request)) {

        if (thorium_active_request_test(&active_request)) {

            worker = thorium_active_request_get_worker(&active_request);
            buffer = thorium_active_request_buffer(&active_request);

            thorium_worker_buffer_init(worker_buffer, worker, buffer);

            return 1;

        /* Just put it back in the FIFO for later */
        } else {
            bsal_ring_queue_enqueue(&self->active_requests, &active_request);

            return 0;
        }
    }

    return 0;
}

int thorium_transport_dequeue_active_request(struct thorium_transport *self, struct thorium_active_request *active_request)
{
    return bsal_ring_queue_dequeue(&self->active_requests, active_request);
}

int thorium_transport_get_implementation(struct thorium_transport *self)
{
    return self->implementation;
}

void *thorium_transport_get_concrete_transport(struct thorium_transport *self)
{
    return self->concrete_transport;
}

void thorium_transport_set(struct thorium_transport *self)
{
    self->transport_interface = NULL;

    if (self->implementation == thorium_pami_transport_implementation.identifier) {
        self->transport_interface = &thorium_pami_transport_implementation;

    } else if (self->implementation == thorium_mpi_transport_implementation.identifier) {

        self->transport_interface = &thorium_mpi_transport_implementation;
    }
}

int thorium_transport_get_active_request_count(struct thorium_transport *self)
{
    return bsal_ring_queue_size(&self->active_requests);
}

int thorium_transport_get_identifier(struct thorium_transport *self)
{
    if (self->transport_interface == NULL) {
        return -1;
    }

    return self->transport_interface->identifier;
}

const char *thorium_transport_get_name(struct thorium_transport *self)
{
    if (self->transport_interface == NULL) {
        return NULL;
    }

    return self->transport_interface->name;
}

void thorium_transport_select(struct thorium_transport *self)
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

void thorium_transport_print(struct thorium_transport *self)
{
    printf("%s TRANSPORT Rank: %d RankCount: %d Implementation: %s\n",
                    BSAL_NODE_THORIUM_PREFIX,
                self->rank, self->size,
                thorium_transport_get_name(self));
}

