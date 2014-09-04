#include <string.h>

#include "pami_transport.h"

#include <engine/thorium/transport/transport.h>
#include <engine/thorium/message.h>

#include <core/system/debugger.h>

#define SEED 0x92a96a40

struct thorium_transport_interface thorium_pami_transport_implementation = {
    .name = "pami_transport",
    .size = sizeof(struct thorium_pami_transport),
    .init = thorium_pami_transport_init,
    .destroy = thorium_pami_transport_destroy,
    .send = thorium_pami_transport_send,
    .receive = thorium_pami_transport_receive,
    .test = thorium_pami_transport_test
};

void thorium_pami_transport_init(struct thorium_transport *self, int *argc, char ***argv)
{
#ifdef THORIUM_TRANSPORT_USE_PAMI
    const char client_name[] = "THORIUM";
    struct thorium_pami_transport *pami_transport;
    int configuration_count;
    pami_result_t result;
    pami_configuration_t *configurations;
    pami_configuration_t query_configurations[3];
    size_t contexts;

    configuration_count = 0;
    configurations = NULL;

    pami_transport = thorium_transport_get_concrete_transport(self);

    pami_transport->num_contexts = 1;
    pami_transport->send_index = 0;
    pami_transport->recv_index = 0;
    pami_transport->num_preallocated_large_buffers = NUM_RECV_BUFFERS_LARGE;
    pami_transport->num_preallocated_small_buffers = NUM_RECV_BUFFERS_SMALL;
    
    pami_transport->send_cookies = (thorium_send_cookie_t*)malloc(sizeof(thorium_send_cookie_t)*NUM_SEND_COOKIES);
    BSAL_DEBUGGER_ASSERT(pami_transport->send_cookies != NULL);

    pami_transport->recv_cookies = (thorium_recv_cookie_t*)malloc(sizeof(thorium_recv_cookie_t)*NUM_RECV_COOKIES);
    BSAL_DEBUGGER_ASSERT(pami_transport->recv_cookies != NULL);

    pami_transport->recv_buffers_small = (char **)malloc(sizeof(char *)*NUM_RECV_BUFFERS_SMALL_POINTERS);
    BSAL_DEBUGGER_ASSERT(pami_transport->recv_buffers_small != NULL);

    int i = 0;
    for (i = 0; i < NUM_RECV_BUFFERS_SMALL; i++) {
	pami_transport->recv_buffers_small[i] = (char*)malloc(RECV_BUFFER_SIZE_SMALL);
	BSAL_DEBUGGER_ASSERT(pami_transport->recv_buffers_small[i] != NULL);
	pami_transport->avail_small_buffers[i] = 1;
    }

    pami_transport->recv_buffers_large = (char **)malloc(sizeof(char *)*NUM_RECV_BUFFERS_LARGE_POINTERS);
    BSAL_DEBUGGER_ASSERT(pami_transport->recv_buffers_large != NULL);

    for (i = 0; i < NUM_RECV_BUFFERS_LARGE; i++) {
        pami_transport->recv_buffers_large[i] = (char*)malloc(RECV_BUFFER_SIZE_LARGE);
        BSAL_DEBUGGER_ASSERT(pami_transport->recv_buffers_large[i] != NULL);
	pami_transport->avail_large_buffers[i] = 1;
    }

    pami_transport->send_queue = (struct bsal_fast_queue*)malloc(sizeof(struct bsal_fast_queue));
    BSAL_DEBUGGER_ASSERT(pami_transport->send_queue != NULL);

    pami_transport->recv_queue = (struct bsal_fast_queue*)malloc(sizeof(struct bsal_fast_queue));
    BSAL_DEBUGGER_ASSERT(pami_transport->recv_queue != NULL);

    bsal_fast_queue_init(pami_transport->send_queue, sizeof(thorium_send_info_t));
    bsal_fast_queue_init(pami_transport->recv_queue, sizeof(thorium_recv_info_t));

    /*
     * \see http://www-01.ibm.com/support/knowledgecenter/SSFK3V_1.3.0/com.ibm.cluster.protocols.v1r3.pp400.doc/bl510_pclientc.htm
     */
    result = PAMI_Client_create(client_name, &pami_transport->client, configurations,
                    configuration_count);

    BSAL_DEBUGGER_ASSERT(result == PAMI_SUCCESS);

    if (result != PAMI_SUCCESS) {
        return;
    }

    /*Create context*/
    result = PAMI_Context_createv(pami_transport->client, configurations, configuration_count, &pami_transport->context, pami_transport->num_contexts);
    BSAL_DEBUGGER_ASSERT(result == PAMI_SUCCESS);

    if (result != PAMI_SUCCESS) {
        return;
    }

    query_configurations[0].name = PAMI_CLIENT_NUM_TASKS;
    query_configurations[1].name = PAMI_CLIENT_TASK_ID;
    query_configurations[2].name = PAMI_CLIENT_NUM_CONTEXTS;

    result = PAMI_Client_query(pami_transport->client, query_configurations, 3);
    self->size = query_configurations[0].value.intval;
    self->rank = query_configurations[1].value.intval;
    contexts = query_configurations[2].value.intval;

    /*For debugging purpose*/
    pami_transport->rank = self->rank;
    for (i = 0; i < NUM_RECV_COOKIES; i++)
	pami_transport->recv_cookies[i].recv_info.dest = self->rank;
    /*End of for debugging purpose*/

    BSAL_DEBUGGER_ASSERT(contexts > 1);

    /*Register dispatch IDs*/
    pami_dispatch_callback_function fn;
    pami_dispatch_hint_t options = {};
    fn.p2p = thorium_recv_message_fn;
    result = PAMI_Dispatch_set (pami_transport->context,
            RECV_MESSAGE_DISPATCH_ID,
            fn,
            (void *) pami_transport,
            options);

    BSAL_DEBUGGER_ASSERT(result == PAMI_SUCCESS);
    if (result != PAMI_SUCCESS) {
        return;
    }

#endif
}

void thorium_pami_transport_destroy(struct thorium_transport *self)
{
#ifdef THORIUM_TRANSPORT_USE_PAMI
    struct thorium_pami_transport *pami_transport;
    pami_result_t result;

    pami_transport = thorium_transport_get_concrete_transport(self);

    /*Free memory*/
    free(pami_transport->send_cookies);
    free(pami_transport->recv_cookies);
    int i = 0;
    for (i = 0; i < pami_transport->num_preallocated_small_buffers; i++) {
        free(pami_transport->recv_buffers_small[i]);
	/*fprintf (stderr, "Rank %d freeing buf[%d]\n", self->rank, i);*/
    }
    free(pami_transport->recv_buffers_small);

    for (i = 0; i < pami_transport->num_preallocated_large_buffers; i++) {
        free(pami_transport->recv_buffers_large[i]);
        /*fprintf (stderr, "Rank %d freeing buf[%d]\n", self->rank, i);*/
    }
    free(pami_transport->recv_buffers_large);

    bsal_fast_queue_destroy(pami_transport->recv_queue);
    bsal_fast_queue_destroy(pami_transport->send_queue);

    free(pami_transport->recv_queue);
    free(pami_transport->send_queue);
    
    /*Destroy context*/
    result = PAMI_Context_destroyv(&pami_transport->context, pami_transport->num_contexts);
    BSAL_DEBUGGER_ASSERT(result == PAMI_SUCCESS);

    /*Destroy client*/
    result = PAMI_Client_destroy(&pami_transport->client);
    BSAL_DEBUGGER_ASSERT(result == PAMI_SUCCESS);

#endif
}

int thorium_pami_transport_send(struct thorium_transport *self, struct thorium_message *message)
{
#ifdef THORIUM_TRANSPORT_USE_PAMI
    struct thorium_pami_transport *pami_transport;
    int destination_node = thorium_message_destination_node(message);
    char *buffer = thorium_message_buffer(message);
    int nbytes = thorium_message_count(message);
    int worker = thorium_message_worker(message);
    int source_actor = thorium_message_source(message);
    int dest_actor = thorium_message_destination(message);

    pami_result_t result;
    pami_transport = thorium_transport_get_concrete_transport(self);
    thorium_send_cookie_t *send_cookie = &(pami_transport->send_cookies[pami_transport->send_index]);
    send_cookie->send_queue = pami_transport->send_queue;
    send_cookie->send_info.worker = worker;
    send_cookie->send_info.buffer = buffer;
    pami_transport->send_index = (pami_transport->send_index + 1) % NUM_SEND_COOKIES;

    if (nbytes <= MAX_SHORT_MESSAGE_LENGTH) {
	pami_send_immediate_t parameter;
	parameter.dispatch = RECV_MESSAGE_DISPATCH_ID;
        parameter.header.iov_base = NULL;
        parameter.header.iov_len = 0;
	parameter.data.iov_base = (void *)buffer;
        parameter.data.iov_len = nbytes;
        PAMI_Endpoint_create (pami_transport->client, destination_node, 0, &parameter.dest);

        result = PAMI_Send_immediate (pami_transport->context, &parameter);
	BSAL_DEBUGGER_ASSERT(result == PAMI_SUCCESS);
        if (result != PAMI_SUCCESS) {
	    return 0;
        }
	/*Add buffer and worker into send_queue*/
	bsal_fast_queue_enqueue(send_cookie->send_queue, (void*)&(send_cookie->send_info));
	/*fprintf (stderr, "send: source = %d, dest = %d, nbytes = %d done worker = %d\n", self->rank, destination_node, nbytes, send_cookie->send_info.worker);*/
    } else {
	pami_send_t param_send;
        param_send.send.dest = destination_node;
        param_send.send.dispatch = RECV_MESSAGE_DISPATCH_ID;
	param_send.send.header.iov_base = NULL;
        param_send.send.header.iov_len = 0;
	param_send.send.data.iov_base = (void *)buffer;
        param_send.send.data.iov_len = nbytes;
        param_send.events.cookie = (void *)send_cookie;
        param_send.events.local_fn = thorium_send_done_fn;
        param_send.events.remote_fn = NULL;

        result = PAMI_Send(pami_transport->context, &param_send);
	BSAL_DEBUGGER_ASSERT(result == PAMI_SUCCESS);
        if (result != PAMI_SUCCESS) {
	    return 0;
        }
	uint8_t *b = (uint8_t*)buffer;
	/*fprintf (stderr, "send: source: %d (%d) destination: %d (%d) count = %d %d%d%d%d_%d_%d_%d_%d\n", source_actor, self->rank, dest_actor, destination_node, nbytes, b[0], b[1], b[2], b[3], b[nbytes-24], b[nbytes-23], b[nbytes-22], b[nbytes-21]);*/
    }

    return 1;
#endif
}

int thorium_pami_transport_receive(struct thorium_transport *self, struct thorium_message *message)
{
#ifdef THORIUM_TRANSPORT_USE_PAMI
    struct thorium_pami_transport *pami_transport;
    pami_transport = thorium_transport_get_concrete_transport(self);

    PAMI_Context_advance(pami_transport->context, 100);

    thorium_recv_info_t recv_info;
    if (bsal_fast_queue_dequeue(pami_transport->recv_queue, (void *)&recv_info)) {
	/*fprintf(stderr, "Dequeue: source = %d, dest = %d, count = %d\n", recv_info.source, recv_info.dest, recv_info.count);*/
	char *buffer = (char*)bsal_memory_pool_allocate(self->inbound_message_memory_pool, recv_info.count);
	memcpy(buffer, (void *)recv_info.buffer, recv_info.count);

	/*if (recv_info.count > MAX_SHORT_MESSAGE_LENGTH) {
	    int data_size = recv_info.count - sizeof(uint64_t) - 12;
	    uint64_t *bucket = (uint64_t*)(buffer + data_size);
	    uint64_t expected_checksum = *bucket;
	    uint64_t actual_checksum = bsal_murmur_hash_2_64_a(buffer, data_size, SEED);

	    if  (expected_checksum != actual_checksum) {
		int i = 0;
		for (i = 0; i < data_size; i++) {
		    if (buffer[i] != i% 256) {
			fprintf(stderr, "Invalid checksum at %d source = %d, dest = %d, count = %d sent = %d, recv = %d\n", i, recv_info.source, recv_info.dest, recv_info.count, i%256, buffer[i]);
		    }
		}
	    }
	}*/

	thorium_message_init_with_nodes(message, recv_info.count, buffer, recv_info.source, self->rank);
	/*Update the availablity of the buffer*/
	if (recv_info.count <= MAX_SHORT_MESSAGE_LENGTH) {
	    pami_transport->avail_small_buffers[recv_info.buf_index] = 1;
	    pami_transport->num_avail_small_buffers++;
	} else {
	    pami_transport->avail_large_buffers[recv_info.buf_index] = 1;
	    pami_transport->num_avail_large_buffers++;
	}

	/*fprintf(stderr, "Initialized: source = %d, dest = %d, count = %d\n", recv_info.source, recv_info.dest, recv_info.count);*/
    } else {
	return 0;
    }

    return 1;
#endif
}

int thorium_pami_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer)
{
#ifdef THORIUM_TRANSPORT_USE_PAMI
    struct thorium_pami_transport *pami_transport;
    pami_transport = thorium_transport_get_concrete_transport(self);

    PAMI_Context_advance (pami_transport->context, 100);

    if (bsal_fast_queue_size(pami_transport->send_queue) > 0) {
	thorium_send_info_t send_info;
	bsal_fast_queue_dequeue(pami_transport->send_queue, (void *)&send_info);
	thorium_worker_buffer_init(worker_buffer, send_info.worker, send_info.buffer);
    } else {
	return 0;
    }

    return 1;
#endif
}

void thorium_send_done_fn (pami_context_t   context,
        void           * cookie,
        pami_result_t    result) 
{
    thorium_send_cookie_t *send_cookie = (thorium_send_cookie_t *)cookie;
    bsal_fast_queue_enqueue(send_cookie->send_queue, (void *)&(send_cookie->send_info));
    /*fprintf (stderr, "thorium_send_done_fn() worker = %d\n", send_cookie->send_info.worker);*/
}

void thorium_recv_done_fn (pami_context_t context, void *cookie, pami_result_t result) 
{
    thorium_recv_cookie_t *recv_cookie = (thorium_recv_cookie_t *)cookie;
    bsal_fast_queue_enqueue(recv_cookie->recv_queue, (void*)&(recv_cookie->recv_info));

    /*fprintf (stderr, "thorium_recv_done_fn: source = %d, dest = %d, count = %d\n", recv_cookie->recv_info.source, recv_cookie->recv_info.dest, recv_cookie->recv_info.count);*/
}

void thorium_recv_message_fn( pami_context_t context, void * cookie, 
        const void *header,
        size_t header_size,
        const void * data,
        size_t data_size,
        pami_endpoint_t origin,
        pami_recv_t * recv) 
{
    struct thorium_pami_transport *pami_transport = (struct thorium_pami_transport *) cookie;
    int i = 0;
    char *buffer;
    int buf_index = 0;
    if (data_size <= RECV_BUFFER_SIZE_SMALL) {
	/*If running out of preallocated buffer*/
	if (pami_transport->num_avail_small_buffers == 0) {
	    pami_transport->recv_buffers_small[pami_transport->num_preallocated_small_buffers] = (char*)malloc(RECV_BUFFER_SIZE_SMALL);
	    pami_transport->num_preallocated_small_buffers++;
	    pami_transport->num_avail_small_buffers++;
	    fprintf("Allocated more small mem to %d\n", pami_transport->num_preallocated_small_buffers);
	}
	/*Go through the small buffers and get the first available one*/
	for (i = 0; i < pami_transport->num_preallocated_small_buffers; i++) {
	    if (pami_transport->avail_small_buffers[i] == 1) {
		pami_transport->avail_small_buffers[i] = 0;
		buffer = pami_transport->recv_buffers_small[i];
		buf_index = i;
		pami_transport->num_avail_small_buffers--;
		break;
	    }
	}
    } else {
	/*If running out of preallocated buffer*/
        if (pami_transport->num_avail_large_buffers == 0) {
            pami_transport->recv_buffers_large[pami_transport->num_preallocated_large_buffers] = (char*)malloc(RECV_BUFFER_SIZE_LARGE);
            pami_transport->num_preallocated_large_buffers++;
	    pami_transport->num_avail_large_buffers++;
	    fprintf("Allocated more large mem to %d\n", pami_transport->num_preallocated_large_buffers);
        }
        /*Go through the small buffers and get the first available one*/
        for (i = 0; i < pami_transport->num_preallocated_large_buffers; i++) {
            if (pami_transport->avail_large_buffers[i] == 1) {
                pami_transport->avail_large_buffers[i] = 0;
                buffer = pami_transport->recv_buffers_large[i];
		buf_index = i;
                pami_transport->num_avail_large_buffers--;
                break;
            }
        }
    }

    thorium_recv_cookie_t *recv_cookie = &(pami_transport->recv_cookies[pami_transport->recv_index]);
    pami_transport->recv_index = (pami_transport->recv_index + 1) % NUM_RECV_COOKIES;

    recv_cookie->recv_queue = pami_transport->recv_queue;
    recv_cookie->recv_info.buffer = buffer;
    recv_cookie->recv_info.buf_index = buf_index;
    recv_cookie->recv_info.source = origin;
    recv_cookie->recv_info.count = data_size;

    /*fprintf (stderr, "recv: source = %d, dest = %d, count = %d\n", origin, pami_transport->rank, recv_cookie->recv_info.count);*/

    if (data != NULL) {
        /*fprintf (stderr, "thorium_recv_message_fn() source = %d, count = %d\n", origin, data_size);*/
        memcpy(buffer, data, data_size);
	bsal_fast_queue_enqueue(recv_cookie->recv_queue, (void*)&(recv_cookie->recv_info));
    }
    else {
        /*fprintf (stderr, "thorium_recv_message_fn() data_size = %d, active = %d\n", data_size, cookie_recv->active);*/
        recv->local_fn = thorium_recv_done_fn;
        recv->cookie = (void *)recv_cookie;
        recv->type = PAMI_TYPE_BYTE;
        recv->addr = (void *)buffer;
        recv->offset = 0;
        recv->data_fn = PAMI_DATA_COPY;
        recv->data_cookie = NULL;
    }
    return;
}
