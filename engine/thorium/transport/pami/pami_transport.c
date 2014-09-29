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

    pami_transport->self = self;
    pami_transport->num_contexts = 1;

    int i = 0;

    /*Queue of available send cookies*/
    pami_transport->avail_send_cookies_queue = (struct bsal_fast_queue *)malloc(sizeof(struct bsal_fast_queue));
    bsal_fast_queue_init(pami_transport->avail_send_cookies_queue, sizeof(thorium_send_cookie_t *));
    pami_transport->send_cookies = (thorium_send_cookie_t **)malloc(sizeof(thorium_send_cookie_t *) * NUM_SEND_COOKIES);
    BSAL_DEBUGGER_ASSERT(pami_transport->send_cookies != NULL);
    for (i = 0; i < NUM_SEND_COOKIES; i++) {
	pami_transport->send_cookies[i] = (thorium_send_cookie_t *)malloc(sizeof(thorium_send_cookie_t));
        bsal_fast_queue_enqueue(pami_transport->avail_send_cookies_queue, (void *)&pami_transport->send_cookies[i]);
    }

    /*Queue of available recv cookies*/
    pami_transport->avail_recv_cookies_queue = (struct bsal_fast_queue *)malloc(sizeof(struct bsal_fast_queue));
    bsal_fast_queue_init(pami_transport->avail_recv_cookies_queue, sizeof(thorium_recv_cookie_t *));
    pami_transport->recv_cookies = (thorium_recv_cookie_t **)malloc(sizeof(thorium_recv_cookie_t *) * NUM_RECV_COOKIES);
    BSAL_DEBUGGER_ASSERT(pami_transport->recv_cookies != NULL);
    for (i = 0; i < NUM_RECV_COOKIES; i++) {
	pami_transport->recv_cookies[i] = (thorium_recv_cookie_t *)malloc(sizeof(thorium_recv_cookie_t));
	bsal_fast_queue_enqueue(pami_transport->avail_recv_cookies_queue, (void *)&pami_transport->recv_cookies[i]);
    }

    /*Queue for in use send cookies*/
    pami_transport->in_use_send_cookies_queue = (struct bsal_fast_queue *)malloc(sizeof(struct bsal_fast_queue));
    BSAL_DEBUGGER_ASSERT(pami_transport->in_use_send_cookies_queue != NULL);
    bsal_fast_queue_init(pami_transport->in_use_send_cookies_queue, sizeof(thorium_send_info_t *));

    /*Queue for in use recv cookies*/
    pami_transport->in_use_recv_cookies_queue = (struct bsal_fast_queue *)malloc(sizeof(struct bsal_fast_queue));
    BSAL_DEBUGGER_ASSERT(pami_transport->in_use_recv_cookies_queue != NULL);
    bsal_fast_queue_init(pami_transport->in_use_recv_cookies_queue, sizeof(thorium_recv_info_t *));

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

    pami_transport->rank = self->rank;

    /*Create immediate send parameters, so we need to create endpoint only once*/
    pami_transport->endpoints = (pami_endpoint_t *)malloc(sizeof(pami_endpoint_t) * self->size);
    for (i = 0; i < self->size; i++) {
	PAMI_Endpoint_create(pami_transport->client, i, 0, &pami_transport->endpoints[i]);
    }

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
    thorium_send_cookie_t *send_cookie;
    while (bsal_fast_queue_dequeue(pami_transport->avail_send_cookies_queue, &send_cookie)) {
	free(send_cookie);
    }
    bsal_fast_queue_destroy(pami_transport->avail_send_cookies_queue);
    free(pami_transport->avail_send_cookies_queue);
    free(pami_transport->send_cookies);

    thorium_recv_cookie_t *recv_cookie;
    while (bsal_fast_queue_dequeue(pami_transport->avail_recv_cookies_queue, &recv_cookie)) {
        free(recv_cookie);
    }
     bsal_fast_queue_destroy(pami_transport->avail_recv_cookies_queue);
    free(pami_transport->avail_recv_cookies_queue);
    free(pami_transport->recv_cookies);

    bsal_fast_queue_destroy(pami_transport->in_use_send_cookies_queue);
    bsal_fast_queue_destroy(pami_transport->in_use_recv_cookies_queue);

    free(pami_transport->in_use_send_cookies_queue);
    free(pami_transport->in_use_recv_cookies_queue);

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

    pami_result_t result;
    pami_transport = thorium_transport_get_concrete_transport(self);

    thorium_send_cookie_t *send_cookie;
    if (bsal_fast_queue_dequeue(pami_transport->avail_send_cookies_queue, &send_cookie) == BSAL_FALSE) {
	send_cookie = (thorium_send_cookie_t *)malloc(sizeof(thorium_send_cookie_t));
	BSAL_DEBUGGER_ASSERT(send_cookie != NULL);
    }

    send_cookie->send_queue = pami_transport->in_use_send_cookies_queue;
    send_cookie->send_info.worker = worker;
    send_cookie->send_info.buffer = buffer;

    if (nbytes <= MAX_SHORT_MESSAGE_LENGTH) {
	pami_send_immediate_t parameter;
	parameter.dispatch = RECV_MESSAGE_DISPATCH_ID;
        parameter.header.iov_base = NULL;
        parameter.header.iov_len = 0;
	parameter.data.iov_base = (void *)buffer;
        parameter.data.iov_len = nbytes;
	parameter.dest = pami_transport->endpoints[destination_node];

        result = PAMI_Send_immediate (pami_transport->context, &parameter);
	BSAL_DEBUGGER_ASSERT(result == PAMI_SUCCESS);
        if (result != PAMI_SUCCESS) {
	    return 0;
        }
	/*Add buffer and worker into send_queue*/
	bsal_fast_queue_enqueue(send_cookie->send_queue, (void *)&send_cookie);
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
    }

    return 1;
#endif
}

int thorium_pami_transport_receive(struct thorium_transport *self, struct thorium_message *message)
{
#ifdef THORIUM_TRANSPORT_USE_PAMI
    struct thorium_pami_transport *pami_transport;
    pami_transport = thorium_transport_get_concrete_transport(self);

    /* Check if any send requests have been done, if not repeat the checking loop for 100 times.*/
    PAMI_Context_advance(pami_transport->context, 100);

    thorium_recv_cookie_t *recv_cookie;
    if (bsal_fast_queue_dequeue(pami_transport->in_use_recv_cookies_queue, (void *)&recv_cookie)) {
	thorium_message_init_with_nodes(message, recv_cookie->recv_info.count, recv_cookie->recv_info.buffer, recv_cookie->recv_info.source, self->rank);
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

    /*
    * Check if any send requests have been done, if not repeat the checking loop for 100 times.
    * */
    PAMI_Context_advance (pami_transport->context, 100);

    thorium_send_cookie_t *send_cookie;
    if (bsal_fast_queue_dequeue(pami_transport->in_use_send_cookies_queue, (void *)&send_cookie)) {
	thorium_worker_buffer_init(worker_buffer, send_cookie->send_info.worker, send_cookie->send_info.buffer);
	bsal_fast_queue_enqueue(pami_transport->avail_send_cookies_queue, (void *)&send_cookie);
    } else {
	return 0;
    }

    return 1;
#endif
}

void thorium_send_done_fn(pami_context_t context, void *cookie, pami_result_t result)
{
#ifdef THORIUM_TRANSPORT_USE_PAMI
    thorium_send_cookie_t *send_cookie = (thorium_send_cookie_t *)cookie;
    bsal_fast_queue_enqueue(send_cookie->send_queue, (void *)&send_cookie);
#endif
}

void thorium_recv_done_fn(pami_context_t context, void *cookie, pami_result_t result)
{
#ifdef THORIUM_TRANSPORT_USE_PAMI
    thorium_recv_cookie_t *recv_cookie = (thorium_recv_cookie_t *)cookie;
    bsal_fast_queue_enqueue(recv_cookie->recv_queue, &recv_cookie);
#endif
}

void thorium_recv_message_fn(pami_context_t context, void *cookie, const void *header, size_t header_size,
                const void *data, size_t data_size, pami_endpoint_t origin, pami_recv_t *recv)
{
#ifdef THORIUM_TRANSPORT_USE_PAMI
    struct thorium_pami_transport *pami_transport = (struct thorium_pami_transport *) cookie;
    thorium_recv_cookie_t *recv_cookie;

    if (bsal_fast_queue_dequeue(pami_transport->avail_recv_cookies_queue, (void *)&recv_cookie) == BSAL_FALSE) {
	recv_cookie = (thorium_recv_cookie_t *)malloc(sizeof(thorium_recv_cookie_t));
	BSAL_DEBUGGER_ASSERT(recv_cookie != NULL);
    }

    recv_cookie->recv_queue = pami_transport->in_use_recv_cookies_queue;
    recv_cookie->recv_info.source = origin;
    recv_cookie->recv_info.dest = pami_transport->rank;
    recv_cookie->recv_info.count = data_size;

    recv_cookie->recv_info.buffer = (char *)bsal_memory_pool_allocate(pami_transport->self->inbound_message_memory_pool, recv_cookie->recv_info.count);

    if (data != NULL) {
        memcpy(recv_cookie->recv_info.buffer, data, data_size);
	bsal_fast_queue_enqueue(recv_cookie->recv_queue, (void *)&recv_cookie);
    }
    else {
        recv->local_fn = thorium_recv_done_fn;
        recv->cookie = (void *)recv_cookie;
        recv->type = PAMI_TYPE_BYTE;
        recv->addr = (void *)recv_cookie->recv_info.buffer;
        recv->offset = 0;
        recv->data_fn = PAMI_DATA_COPY;
        recv->data_cookie = NULL;
    }
    return;
#endif
}

