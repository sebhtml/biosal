
#ifndef THORIUM_TRANSPORT_H
#define THORIUM_TRANSPORT_H

#include "transport_interface.h"
#include "transport_profiler.h"

#include <core/system/timer.h>

#include <time.h>

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
    int provided;
    int rank;
    int size;

    struct core_memory_pool *inbound_message_memory_pool;
    struct core_memory_pool *outbound_message_memory_pool;

    void *concrete_transport;
    struct thorium_transport_interface *transport_interface;
    int active_request_count;

    uint32_t flags;
    struct core_timer timer;
    uint64_t start_time;

    uint64_t received_message_count;
    uint64_t sent_message_count;
    uint64_t received_byte_count;
    uint64_t sent_byte_count;

    uint64_t last_sent_message_count;
    uint64_t last_time_high_resolution;
    time_t last_time_low_resolution;

    double current_outbound_throughput;
    double maximum_outbound_throughput;
};

void thorium_transport_init(struct thorium_transport *self, struct thorium_node *node,
                int *argc, char ***argv,
                struct core_memory_pool *inbound_message_memory_pool,
                struct core_memory_pool *outbound_message_memory_pool);

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
void thorium_transport_select_implementation(struct thorium_transport *self, int argc, char **argv);

void thorium_transport_print(struct thorium_transport *self);
void thorium_transport_print_event(struct thorium_transport *self, int type, struct thorium_message *message);

uint64_t thorium_transport_received_message_count(struct thorium_transport *self);
uint64_t thorium_transport_sent_message_count(struct thorium_transport *self);
uint64_t thorium_transport_received_byte_count(struct thorium_transport *self);
uint64_t thorium_transport_sent_byte_count(struct thorium_transport *self);

double thorium_transport_get_maximum_outbound_throughput(struct thorium_transport *self);
double thorium_transport_get_outbound_throughput(struct thorium_transport *self);

void thorium_transport_update_outbound_throughput(struct thorium_transport *self);

#endif
