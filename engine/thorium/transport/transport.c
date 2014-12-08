
#include "transport.h"

#include <tracepoints/tracepoints.h>

/*
 * Implementations of the interface.
 */

#ifdef CONFIG_PAMI
#include "pami/pami_transport.h"
#endif

#ifdef CONFIG_MPI
#include "mpi1_pt2pt/mpi1_pt2pt_transport.h"
#include "mpi1_pt2pt_nonblocking/mpi1_pt2pt_nonblocking_transport.h"
#endif

/*
 * The mock transport does nothing. It is required however when
 * no other transport implementation is available.
 *
 * The only thing it does is set the rank (to 0) and size (to 1).
 */
#include "mock/mock_transport.h"

#include <core/system/command.h>
#include <core/helpers/bitmap.h>

#include <engine/thorium/worker_buffer.h>
#include <engine/thorium/message.h>
#include <engine/thorium/node.h>

#include <core/system/debugger.h>

#include <string.h>
#include <stdint.h>
#include <inttypes.h>

/*
#define THORIUM_TRANSPORT_DEBUG
*/

#define EVENT_TYPE_SEND 0
#define EVENT_TYPE_RECEIVE 1
#define EVENT_STRING_SEND "EVENT_SEND"
#define EVENT_STRING_RECEIVE "EVENT_RECEIVE"

#define FLAG_PRINT_TRANSPORT_EVENTS 0

#define MEMORY_TRANSPORT 0xe1b48d97

void thorium_transport_init(struct thorium_transport *self, struct thorium_node *node,
                int *argc, char ***argv,
                struct core_memory_pool *inbound_message_memory_pool,
                struct core_memory_pool *outbound_message_memory_pool)
{
    int actual_argc;
    char **actual_argv;

    self->active_request_count = 0;

    actual_argc = *argc;
    actual_argv = *argv;

    self->flags = 0;
    CORE_BITMAP_CLEAR_BIT(self->flags, FLAG_PRINT_TRANSPORT_EVENTS);

    self->transport_interface = NULL;
    self->concrete_transport = NULL;

    /*
    printf("DEBUG Initiating transport\n");
    */
    /* Select the transport layer
     */
    /*
     * Assign functions
     */
    thorium_transport_select_implementation(self, actual_argc, actual_argv);

    self->node = node;

    self->rank = -1;
    self->size = -1;

    if (self->transport_interface != NULL) {

        self->concrete_transport = core_memory_allocate(self->transport_interface->size, MEMORY_TRANSPORT);
        self->transport_interface->init(self, argc, argv);
    }

    CORE_DEBUGGER_ASSERT(self->rank >= 0);
    CORE_DEBUGGER_ASSERT(self->size >= 1);
    CORE_DEBUGGER_ASSERT(self->node != NULL);

    self->inbound_message_memory_pool = inbound_message_memory_pool;
    self->outbound_message_memory_pool = outbound_message_memory_pool;

    if (self->rank == 0
         && thorium_node_must_print_data(self->node)) {
        printf("thorium_transport: type %s\n",
                    self->transport_interface->name);
    }

    if (core_command_has_argument(actual_argc, actual_argv, "-print-transport-events")) {
        CORE_BITMAP_SET_BIT(self->flags, FLAG_PRINT_TRANSPORT_EVENTS);
    }

    core_timer_init(&self->timer);
    self->start_time = core_timer_get_nanoseconds(&self->timer);

    self->sent_message_count = 0;
    self->received_message_count = 0;
}

void thorium_transport_destroy(struct thorium_transport *self)
{
    CORE_DEBUGGER_ASSERT(thorium_transport_get_active_request_count(self) == 0);

    if (self->transport_interface != NULL) {
        self->transport_interface->destroy(self);

        core_memory_free(self->concrete_transport, MEMORY_TRANSPORT);
        self->concrete_transport = NULL;
    }

    self->node = NULL;
    self->rank = -1;
    self->size = -1;

    core_timer_destroy(&self->timer);
}

int thorium_transport_send(struct thorium_transport *self, struct thorium_message *message)
{
    int value;

    if (self->transport_interface == NULL) {
        return 0;
    }

    tracepoint(thorium_transport, send, message);

    /*
     * Trace the event "message:transport_send".
     */
    tracepoint(thorium_message, transport_send, message);

    value = self->transport_interface->send(self, message);

    if (value) {

#ifdef THORIUM_TRANSPORT_DEBUG
        printf("TRANSPORT SEND Source %d Destination %d Action %x Count %d\n",
                        thorium_message_source_node(message),
                        thorium_message_destination_node(message),
                        thorium_message_action(message),
                        thorium_message_count(message));
#endif
        ++self->active_request_count;
        ++self->sent_message_count;

        if (CORE_BITMAP_GET_BIT(self->flags, FLAG_PRINT_TRANSPORT_EVENTS)) {
            thorium_transport_print_event(self, EVENT_TYPE_SEND, message);
        }
    }

    return value;
}

int thorium_transport_receive(struct thorium_transport *self, struct thorium_message *message)
{
    int value;

    if (self->transport_interface == NULL) {
        return 0;
    }

    value = self->transport_interface->receive(self, message);

    if (value) {

        /*
         * MPICH produces this error if this is enabled:
         * Assertion failed in file /notbackedup/tmp/ulib/mpt/nightly/6.0/081313/mpich2/src/mpi/datatype/get_count.c at line 34: size >= 0 && status->count >= 0
         *
         * \see https://github.com/GeneAssembly/biosal/issues/774
         */
#if 0
        /*
         * Prepare the message.
         *
         * This fetches the metadata from the buffer.
         */
        thorium_node_prepare_received_message(self->node, message);
#endif

        tracepoint(thorium_transport, receive, message);

        /*
         * Trace the event "message:transport_receive"
         */
        tracepoint(thorium_message, transport_receive, message);

#ifdef THORIUM_TRANSPORT_DEBUG
        printf("TRANSPORT RECEIVE Source %d Destination %d Action %x Count %d\n",
                        thorium_message_source_node(message),
                        thorium_message_destination_node(message),
                        thorium_message_action(message),
                        thorium_message_count(message));
#endif

        if (CORE_BITMAP_GET_BIT(self->flags, FLAG_PRINT_TRANSPORT_EVENTS)) {
            thorium_transport_print_event(self, EVENT_TYPE_RECEIVE, message);
        }

        ++self->received_message_count;
    }

    return value;
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

int thorium_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer)
{
    int value;

    if (self->transport_interface == NULL) {
        return 0;
    }

    value = self->transport_interface->test(self, worker_buffer);

    if (value) {
        --self->active_request_count;
    }

    return value;
}

void *thorium_transport_get_concrete_transport(struct thorium_transport *self)
{
    return self->concrete_transport;
}

void thorium_transport_select_implementation(struct thorium_transport *self, int argc, char ** argv)
{
    char *requested_implementation_name;
    struct core_vector implementations;
    int i;
    int size;
    struct thorium_transport_interface *component;
    char *available_name;

    /*
     * Prepare a list of potential transport implementations to
     * use.
     */

    core_vector_init(&implementations, sizeof(struct thorium_transport_interface *));

#ifdef CONFIG_MPI
    /*
     * MPI 1 point-to-point blocking communication.
     */
    component = &thorium_mpi1_pt2pt_transport_implementation;
    core_vector_push_back(&implementations, &component);

    /*
     * MPI 1 point-to-point non-blocking communication.
     */
    component = &thorium_mpi1_pt2pt_nonblocking_transport_implementation;
    core_vector_push_back(&implementations, &component);

#endif

    /*
     * Only enable the pami thing on Blue Gene/Q.
     */
#ifdef CONFIG_PAMI
#if defined(__bgq__)
    component = &thorium_pami_transport_implementation;
    core_vector_push_back(&implementations, &component);
#endif
#endif

#if defined(_CRAYC) && 0
    component = &thorium_gni_transport_implementation;
    core_vector_push_back(&implementations, &component);
#endif

    /*
     * Add the mock transport at the end so that it acts as a
     * fall-back implementation when nothing else is available.
     */
    component = &thorium_mock_transport_implementation;
    core_vector_push_back(&implementations, &component);

    requested_implementation_name = core_command_get_argument_value(argc, argv, "-transport");

    self->transport_interface = NULL;

    /*
     * The default is the first one in the list.
     */
    if (!core_vector_empty(&implementations)) {
        self->transport_interface = *(struct thorium_transport_interface **)core_vector_at(&implementations, 0);
    }

    /*
     * The option -transport was provided.
     *
     * Possible values are:
     *
     * -transport mpi1_pt2pt_nonblocking_transport
     * -transport mpi1_pt2pt_transport
     * -transport thorium_pami_transport_implementation
     */
    if (requested_implementation_name != NULL) {

        size = core_vector_size(&implementations);

        for (i = 0; i < size; ++i) {
            component = *(struct thorium_transport_interface **)core_vector_at(&implementations, i);
            available_name = component->name;

            if (strcmp(available_name, requested_implementation_name) == 0) {
                self->transport_interface = component;
                break;
            }
        }
    }

    core_vector_destroy(&implementations);

    CORE_DEBUGGER_ASSERT(self->transport_interface != NULL);
}

int thorium_transport_get_active_request_count(struct thorium_transport *self)
{
    return self->active_request_count;
}

const char *thorium_transport_get_name(struct thorium_transport *self)
{
    if (self->transport_interface == NULL) {
        return NULL;
    }

    return self->transport_interface->name;
}

void thorium_transport_print(struct thorium_transport *self)
{
    printf("thorium_transport: TRANSPORT Rank: %d RankCount: %d Implementation: %s\n",
                self->rank, self->size,
                thorium_transport_get_name(self));
}

void thorium_transport_print_event(struct thorium_transport *self, int type, struct thorium_message *message)
{
    char *description;
    int count;
    int source_rank;
    int destination_rank;
    uint64_t time;

    description = EVENT_STRING_SEND;

    if (type == EVENT_TYPE_RECEIVE)
        description = EVENT_STRING_RECEIVE;

    count = thorium_message_count(message);
    source_rank = thorium_message_source_node(message);
    destination_rank = thorium_message_destination_node(message);

    time = core_timer_get_nanoseconds(&self->timer);
    time -= self->start_time;
    printf("thorium_transport print_event time_nanoseconds= %" PRIu64 " type= %s source= %d destination= %d count= %d\n",
                    time, description,
                    source_rank, destination_rank, count);
}

int thorium_transport_sent_message_count(struct thorium_transport *self)
{
    return self->sent_message_count;
}

int thorium_transport_received_message_count(struct thorium_transport *self)
{
    return self->received_message_count;
}
