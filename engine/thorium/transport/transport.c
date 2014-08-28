
#include "transport.h"

#include <core/system/command.h>
#include <core/helpers/bitmap.h>

#include <engine/thorium/worker_buffer.h>
#include <engine/thorium/message.h>
#include <engine/thorium/node.h>

#include <core/system/debugger.h>

#include <string.h>

/*
#define THORIUM_TRANSPORT_DEBUG
*/

#define FLAG_PROFILE 0

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
    bsal_bitmap_set_bit_value_uint32_t(&self->flags, FLAG_PROFILE, 0);

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

        self->concrete_transport = bsal_memory_allocate(self->transport_interface->size);
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

        bsal_bitmap_set_bit_value_uint32_t(&self->flags, FLAG_PROFILE, 1);
    }

    if (self->rank == 0) {
        printf("DEBUG TRANSPORT -> %s\n",
                    self->transport_interface->name);
    }
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

        bsal_memory_free(self->concrete_transport);
        self->concrete_transport = NULL;
    }

    self->node = NULL;
    self->rank = -1;
    self->size = -1;
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
                        thorium_message_tag(message),
                        thorium_message_count(message));
#endif
        ++self->active_request_count;
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
                        thorium_message_tag(message),
                        thorium_message_count(message));
#endif
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
#if defined(__bgq__)
    component = &thorium_pami_transport_implementation;
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
    printf("%s TRANSPORT Rank: %d RankCount: %d Implementation: %s\n",
                    THORIUM_NODE_THORIUM_PREFIX,
                self->rank, self->size,
                thorium_transport_get_name(self));
}

