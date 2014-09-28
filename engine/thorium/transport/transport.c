
#include "transport.h"

/*
 * Implementations of the interface.
 */

#ifdef CONFIG_PAMI
#include "pami/pami_transport.h"
#endif

#include "mpi1_pt2pt/mpi1_pt2pt_transport.h"
#include "mpi1_pt2pt_nonblocking/mpi1_pt2pt_nonblocking_transport.h"

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

#define FLAG_PROFILE 0
#define FLAG_PRINT_TRANSPORT_EVENTS 1

#define MEMORY_TRANSPORT 0xe1b48d97

void thorium_transport_init(struct thorium_transport *self, struct thorium_node *node,
                int *argc, char ***argv,
                struct bsal_memory_pool *inbound_message_memory_pool,
                struct bsal_memory_pool *outbound_message_memory_pool)
{
    int actual_argc;
    char **actual_argv;

    self->active_request_count = 0;

    actual_argc = *argc;
    actual_argv = *argv;

    self->flags = 0;
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_PROFILE);
    bsal_bitmap_clear_bit_uint32_t(&self->flags, FLAG_PRINT_TRANSPORT_EVENTS);

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

        self->concrete_transport = bsal_memory_allocate(self->transport_interface->size, MEMORY_TRANSPORT);
        self->transport_interface->init(self, argc, argv);
    }

    BSAL_DEBUGGER_ASSERT(self->rank >= 0);
    BSAL_DEBUGGER_ASSERT(self->size >= 1);
    BSAL_DEBUGGER_ASSERT(self->node != NULL);

    self->inbound_message_memory_pool = inbound_message_memory_pool;
    self->outbound_message_memory_pool = outbound_message_memory_pool;

    thorium_transport_profiler_init(&self->transport_profiler);

    if (bsal_command_has_argument(actual_argc, actual_argv, "-enable-transport-profiler")) {

        printf("Enable transport profiler\n");

        bsal_bitmap_set_bit_uint32_t(&self->flags, FLAG_PROFILE);
    }

    if (self->rank == 0) {
        printf("thorium_transport: type %s\n",
                    self->transport_interface->name);
    }

    if (bsal_command_has_argument(actual_argc, actual_argv, "-print-transport-events")) {
        bsal_bitmap_set_bit_uint32_t(&self->flags, FLAG_PRINT_TRANSPORT_EVENTS);
    }

    bsal_timer_init(&self->timer);
    self->start_time = bsal_timer_get_nanoseconds(&self->timer);
}

void thorium_transport_destroy(struct thorium_transport *self)
{
    /*
     * Print the report if requested.
     */
    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_PROFILE)) {
        thorium_transport_profiler_print_report(&self->transport_profiler);
    }

    thorium_transport_profiler_destroy(&self->transport_profiler);

    BSAL_DEBUGGER_ASSERT(thorium_transport_get_active_request_count(self) == 0);

    if (self->transport_interface != NULL) {
        self->transport_interface->destroy(self);

        bsal_memory_free(self->concrete_transport, MEMORY_TRANSPORT);
        self->concrete_transport = NULL;
    }

    self->node = NULL;
    self->rank = -1;
    self->size = -1;

    bsal_timer_destroy(&self->timer);
}

int thorium_transport_send(struct thorium_transport *self, struct thorium_message *message)
{
    int value;

    if (self->transport_interface == NULL) {
        return 0;
    }

    /*
     * Send the message through the mock transport which is
     * a transport profiler.
     */
    if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_PROFILE)) {
        thorium_transport_profiler_send_mock(&self->transport_profiler, message);
    }

    value = self->transport_interface->send(self, message);

    if (value) {
#ifdef THORIUM_TRANSPORT_DEBUG
        printf("TRANSPORT SEND Source %d Destination %d Tag %d Count %d\n",
                        thorium_message_source_node(message),
                        thorium_message_destination_node(message),
                        thorium_message_action(message),
                        thorium_message_count(message));
#endif
        ++self->active_request_count;

        if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_PRINT_TRANSPORT_EVENTS)) {
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
#ifdef THORIUM_TRANSPORT_DEBUG
        printf("TRANSPORT RECEIVE Source %d Destination %d Tag %d Count %d\n",
                        thorium_message_source_node(message),
                        thorium_message_destination_node(message),
                        thorium_message_action(message),
                        thorium_message_count(message));
#endif

        if (bsal_bitmap_get_bit_uint32_t(&self->flags, FLAG_PRINT_TRANSPORT_EVENTS)) {
            thorium_transport_print_event(self, EVENT_TYPE_RECEIVE, message);
        }
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
    struct bsal_vector implementations;
    int i;
    int size;
    struct thorium_transport_interface *component;
    char *available_name;

    /*
     * Prepare a list of potential transport implementations to
     * use.
     */

    bsal_vector_init(&implementations, sizeof(struct thorium_transport_interface *));

    /* MPI non-blocking, this is the default.
     */
    component = &thorium_mpi1_pt2pt_nonblocking_transport_implementation;
    bsal_vector_push_back(&implementations, &component);

    component = &thorium_mpi1_pt2pt_transport_implementation;
    bsal_vector_push_back(&implementations, &component);

    /*
     * Only enable the pami thing on Blue Gene/Q.
     */
#ifdef CONFIG_PAMI
#if defined(__bgq__)
    component = &thorium_pami_transport_implementation;
    bsal_vector_push_back(&implementations, &component);
#endif
#endif

#if defined(_CRAYC) && 0
    component = &thorium_gni_transport_implementation;
    bsal_vector_push_back(&implementations, &component);
#endif

    requested_implementation_name = bsal_command_get_argument_value(argc, argv, "-transport");

    /*
     * The default is the first one in the list.
     */
    self->transport_interface = *(struct thorium_transport_interface **)bsal_vector_at(&implementations, 0);

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

        size = bsal_vector_size(&implementations);

        for (i = 0; i < size; ++i) {
            component = *(struct thorium_transport_interface **)bsal_vector_at(&implementations, i);
            available_name = component->name;

            if (strcmp(available_name, requested_implementation_name) == 0) {
                self->transport_interface = component;
                break;
            }
        }
    }

    bsal_vector_destroy(&implementations);

    BSAL_DEBUGGER_ASSERT(self->transport_interface != NULL);
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

    time = bsal_timer_get_nanoseconds(&self->timer);
    time -= self->start_time;
    printf("thorium_transport print_event time_nanoseconds= %" PRIu64 " type= %s source= %d destination= %d count= %d\n",
                    time, description,
                    source_rank, destination_rank, count);
}
