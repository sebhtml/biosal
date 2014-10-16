
#include "message.h"

#include <core/system/packer.h>
#include <core/system/debugger.h>

#include <stdlib.h>
#include <string.h>

#include <inttypes.h>

void thorium_message_init(struct thorium_message *self, int action, int count,
                void *buffer)
{
    self->action= action;
    self->buffer = buffer;
    self->count = count;

    self->source_actor = -1;
    self->destination_actor = -1;

    /* ranks are set with thorium_node_resolve */
    self->source_node = -1;
    self->destination_node = -1;

#ifdef THORIUM_MESSAGE_USE_ROUTING
    self->routing_source = -1;
    self->routing_destination = -1;
#endif

    self->worker = -1;

    thorium_message_set_type(self, THORIUM_MESSAGE_TYPE_NONE);

#ifdef THORIUM_MESSAGE_ENABLE_TRACEPOINTS
    thorium_message_set_tracepoint_time(self, THORIUM_MESSAGE_TRACEPOINT_ACTOR_1,
                    THORIUM_MESSAGE_TRACEPOINT_NO_VALUE);
    thorium_message_set_tracepoint_time(self, THORIUM_MESSAGE_TRACEPOINT_WORKER_1,
                    THORIUM_MESSAGE_TRACEPOINT_NO_VALUE);
    thorium_message_set_tracepoint_time(self, THORIUM_MESSAGE_TRACEPOINT_NODE_1,
                    THORIUM_MESSAGE_TRACEPOINT_NO_VALUE);
    thorium_message_set_tracepoint_time(self, THORIUM_MESSAGE_TRACEPOINT_NODE_2,
                    THORIUM_MESSAGE_TRACEPOINT_NO_VALUE);
    thorium_message_set_tracepoint_time(self, THORIUM_MESSAGE_TRACEPOINT_WORKER_2,
                    THORIUM_MESSAGE_TRACEPOINT_NO_VALUE);
    thorium_message_set_tracepoint_time(self, THORIUM_MESSAGE_TRACEPOINT_ACTOR_2,
                    THORIUM_MESSAGE_TRACEPOINT_NO_VALUE);
#endif
}

void thorium_message_destroy(struct thorium_message *self)
{
    self->source_actor = -1;
    self->destination_actor = -1;
    self->action= -1;
    self->buffer = NULL;
    self->count= 0;
}

int thorium_message_source(struct thorium_message *self)
{
    return self->source_actor;
}

int thorium_message_destination(struct thorium_message *self)
{
    return self->destination_actor;
}

int thorium_message_source_node(struct thorium_message *self)
{
    return self->source_node;
}

int thorium_message_destination_node(struct thorium_message *self)
{
    return self->destination_node;
}

int thorium_message_action(struct thorium_message *self)
{
    return self->action;
}

void thorium_message_set_source(struct thorium_message *self, int source)
{
    self->source_actor = source;
}

void thorium_message_set_destination(struct thorium_message *self, int destination)
{
    self->destination_actor = destination;
}

void thorium_message_print(struct thorium_message *self)
{
    printf("Message Tag %x Count %d SourceActor %d DestinationActor %d\n",
                    self->action,
                    self->count,
                    self->source_actor,
                    self->destination_actor);

    /*
    core_debugger_examine(self->buffer, self->count);
    */
}

void *thorium_message_buffer(struct thorium_message *self)
{
    return self->buffer;
}

int thorium_message_count(struct thorium_message *self)
{
    return self->count;
}

void thorium_message_set_source_node(struct thorium_message *self, int source)
{
    self->source_node = source;
}

void thorium_message_set_destination_node(struct thorium_message *self, int destination)
{
    self->destination_node = destination;
}

void thorium_message_set_buffer(struct thorium_message *self, void *buffer)
{
    self->buffer = buffer;
}

void thorium_message_set_action(struct thorium_message *self, int action)
{
    self->action= action;
}

int thorium_message_metadata_size(struct thorium_message *self)
{
    return thorium_message_pack_unpack(self, CORE_PACKER_OPERATION_PACK_SIZE,
                    NULL);
}

int thorium_message_write_metadata(struct thorium_message *self)
{
    return thorium_message_pack_unpack(self, CORE_PACKER_OPERATION_PACK,
                    (char *)self->buffer + self->count);
}

int thorium_message_read_metadata(struct thorium_message *self)
{
    return thorium_message_pack_unpack(self, CORE_PACKER_OPERATION_UNPACK,
                    (char *)self->buffer + self->count);
}

void thorium_message_set_count(struct thorium_message *self, int count)
{
    self->count = count;
}

void thorium_message_init_copy(struct thorium_message *self, struct thorium_message *old_message)
{
    thorium_message_init(self,
                    thorium_message_action(old_message),
                    thorium_message_count(old_message),
                    thorium_message_buffer(old_message));

    thorium_message_set_source(self,
                    thorium_message_source(old_message));
    thorium_message_set_destination(self,
                    thorium_message_destination(old_message));
}

void thorium_message_set_worker(struct thorium_message *self, int worker)
{
    self->worker = worker;

    thorium_message_set_type(self, THORIUM_MESSAGE_TYPE_WORKER_OUTBOUND);
}

int thorium_message_worker(struct thorium_message *self)
{
    return self->worker;
}

void thorium_message_init_with_nodes(struct thorium_message *self, int count, void *buffer, int source,
                int destination)
{
    int action;

    action = -1;

    thorium_message_init(self, action, count, buffer);

    /*
     * Initially assign the MPI source rank and MPI destination
     * rank for the actor source and actor destination, respectively.
     * Then, read the metadata and resolve the MPI rank from
     * that. The resolved MPI ranks should be the same in all cases
     */

    thorium_message_set_source(self, source);
    thorium_message_set_destination(self, destination);

    thorium_message_set_source_node(self, source);
    thorium_message_set_destination_node(self, destination);

    thorium_message_set_type(self, THORIUM_MESSAGE_TYPE_NODE_INBOUND);
}

int thorium_message_pack_unpack(struct thorium_message *self, int operation, void *buffer)
{
    struct core_packer packer;
    int count;

    core_packer_init(&packer, operation, buffer);

    core_packer_process(&packer, &self->source_actor, sizeof(self->source_actor));
    core_packer_process(&packer, &self->destination_actor, sizeof(self->destination_actor));
    core_packer_process(&packer, &self->action, sizeof(self->action));

    count = core_packer_get_byte_count(&packer);

    core_packer_destroy(&packer);

    return count;
}

int thorium_message_type(struct thorium_message *self)
{
    return self->type;
}

void thorium_message_set_type(struct thorium_message *self, int type)
{
    self->type = type;
}

#ifdef THORIUM_MESSAGE_ENABLE_TRACEPOINTS
void thorium_message_set_tracepoint_time(struct thorium_message *self, int tracepoint,
                uint64_t time)
{
    if (tracepoint >= 0 && tracepoint < THORIUM_MESSAGE_TRACEPOINT_COUNT)
        self->tracepoint_times[tracepoint] = time;
}

uint64_t thorium_message_get_tracepoint_time(struct thorium_message *self, int tracepoint)
{
    if (tracepoint >= 0 && tracepoint < THORIUM_MESSAGE_TRACEPOINT_COUNT)
        return self->tracepoint_times[tracepoint];

    return THORIUM_MESSAGE_TRACEPOINT_NO_VALUE;
}

void thorium_message_print_tracepoints(struct thorium_message *self)
{
    printf("thorium_message_print_tracepoints\n");

    printf("tracepoint %s %" PRIu64 "\n",
                    "THORIUM_MESSAGE_TRACEPOINT_ACTOR_1",
                    thorium_message_get_tracepoint_time(self, THORIUM_MESSAGE_TRACEPOINT_ACTOR_1));
    printf("tracepoint %s %" PRIu64 "\n",
                    "THORIUM_MESSAGE_TRACEPOINT_WORKER_1",
                    thorium_message_get_tracepoint_time(self, THORIUM_MESSAGE_TRACEPOINT_WORKER_1));
    printf("tracepoint %s %" PRIu64 "\n",
                    "THORIUM_MESSAGE_TRACEPOINT_NODE_1",
                    thorium_message_get_tracepoint_time(self, THORIUM_MESSAGE_TRACEPOINT_NODE_1));
    printf("tracepoint %s %" PRIu64 "\n",
                    "THORIUM_MESSAGE_TRACEPOINT_NODE_2",
                    thorium_message_get_tracepoint_time(self, THORIUM_MESSAGE_TRACEPOINT_NODE_2));
    printf("tracepoint %s %" PRIu64 "\n",
                    "THORIUM_MESSAGE_TRACEPOINT_WORKER_2",
                    thorium_message_get_tracepoint_time(self, THORIUM_MESSAGE_TRACEPOINT_WORKER_2));
    printf("tracepoint %s %" PRIu64 "\n",
                    "THORIUM_MESSAGE_TRACEPOINT_ACTOR_2",
                    thorium_message_get_tracepoint_time(self, THORIUM_MESSAGE_TRACEPOINT_ACTOR_2));
}
#endif
