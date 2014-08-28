#include <string.h>

#include "pami_transport.h"

#include <engine/thorium/transport/transport.h>
#include <engine/thorium/message.h>

#include <core/system/debugger.h>

struct thorium_transport_interface thorium_pami_transport_implementation = {
    .name = "thorium_pami_transport_implementation",
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
    const char client_name[] = "biosal/thorium";
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
    
    pami_transport->send_cookies = (send_cookie_t*)malloc(sizeof(send_cookie_t)*NUM_SEND_COOKIES);
    pami_transport->recv_cookies = (recv_cookie_t*)malloc(sizeof(recv_cookie_t)*NUM_RECV_COOKIES);

    pami_transport->recv_buffers = (void **)malloc(sizeof(void *)*NUM_RECV_BUFFERS);
    int i = 0;
    for(i = 0; i < NUM_RECV_BUFFERS; i++) {
	pami_transport->recv_buffers[i] = malloc(RECV_BUFFER_SIZE);
    }

    pami_transport->send_queue = (struct bsal_fast_queue*)malloc(sizeof(struct bsal_fast_queue));
    pami_transport->recv_queue = (struct bsal_fast_queue*)malloc(sizeof(struct bsal_fast_queue));

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
    pami_transport->size = query_configurations[0].value.intval;
    pami_transport->rank = query_configurations[1].value.intval;
    contexts = query_configurations[2].value.intval;

    BSAL_DEBUGGER_ASSERT(contexts > 1);

    /*Register dispatch IDs*/
    pami_dispatch_callback_function fn;
    pami_dispatch_hint_t options = {};
    fn.p2p = recv_message_fn;
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
    for(i = 0; i < NUM_RECV_BUFFERS; i++) {
        free(pami_transport->recv_buffers[i]);
    }
    free(pami_transport->recv_buffers);

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
    struct thorium_pami_transport *pami_transport;
    int destination_node = thorium_message_destination_node(message);
    void *buffer = thorium_message_buffer(message);
    int nbytes = thorium_message_count(message);
    int worker = thorium_message_get_worker(message);

    pami_result_t result;
    pami_transport = thorium_transport_get_concrete_transport(self);
    send_cookie_t *send_cookie = &(pami_transport->send_cookies[pami_transport->send_index]);
    send_cookie->send_queue = pami_transport->send_queue;
    send_cookie->send_info.worker = worker;
    send_cookie->send_info.buffer = buffer;
    pami_transport->send_index = (pami_transport->send_index+1)%NUM_SEND_COOKIES;

    if(nbytes <= MAX_SHORT_MESSAGE_LENGTH) {
	pami_send_immediate_t parameter;
	parameter.dispatch        = RECV_MESSAGE_DISPATCH_ID;
        parameter.header.iov_base = NULL;
        parameter.header.iov_len  = 0;
	parameter.data.iov_base   = (void *)buffer;
        parameter.data.iov_len    = nbytes;
        PAMI_Endpoint_create (pami_transport->client, destination_node, 0, &parameter.dest);

        result = PAMI_Send_immediate (pami_transport->context, &parameter);
	BSAL_DEBUGGER_ASSERT(result == PAMI_SUCCESS);
        if(result != PAMI_SUCCESS) {
	    return 0;
        }
	/*Add buffer and worker into send_queue*/
	bsal_fast_queue_enqueue(send_cookie->send_queue, (void*)&(send_cookie->send_info));
	
    } else {
	pami_send_t param_send;
        param_send.send.dest = destination_node;
        param_send.send.dispatch = RECV_MESSAGE_DISPATCH_ID;
	param_send.send.header.iov_base = NULL;
        param_send.send.header.iov_len = 0;
	param_send.send.data.iov_base  = buffer;
        param_send.send.data.iov_len = nbytes;
        param_send.events.cookie        = (void *)send_cookie;
        param_send.events.local_fn      = send_done_fn;
        param_send.events.remote_fn     = NULL;

        result = PAMI_Send(pami_transport->context, &param_send);
	BSAL_DEBUGGER_ASSERT(result == PAMI_SUCCESS);
        if(result != PAMI_SUCCESS) {
	    return 0;
        }
    }

    return 1;
}

int thorium_pami_transport_receive(struct thorium_transport *self, struct thorium_message *message)
{
    struct thorium_pami_transport *pami_transport;
    pami_transport = thorium_transport_get_concrete_transport(self);

    if(bsal_fast_queue_size(pami_transport->recv_queue) > 0) {
	recv_info_t recv_info;
	bsal_fast_queue_dequeue(pami_transport->recv_queue, (void *)&recv_info);
	thorium_message_init_with_nodes(message, -1, recv_info.count, (void *)recv_info.buffer, recv_info.source, -1);
    } else {
	return 0;
    }

    return 1;
}

int thorium_pami_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer)
{
    struct thorium_pami_transport *pami_transport;
    pami_transport = thorium_transport_get_concrete_transport(self);

    PAMI_Context_advance (pami_transport->context, 100);

    if(bsal_fast_queue_size(pami_transport->send_queue) > 0) {
	send_info_t send_info;
	bsal_fast_queue_dequeue(pami_transport->send_queue, (void *)&send_info);
	thorium_worker_buffer_init(worker_buffer, send_info.worker, send_info.buffer);
    } else {
	return 0;
    }

    return 1;
}

void send_done_fn (pami_context_t   context,
        void           * cookie,
        pami_result_t    result) {
    send_cookie_t *send_cookie = (send_cookie_t *)cookie;
    bsal_fast_queue_enqueue(send_cookie->send_queue, (void *)&(send_cookie->send_info));
}

void recv_done_fn (pami_context_t   context,
        void           * cookie,
        pami_result_t    result) {
    recv_cookie_t *recv_cookie = (recv_cookie_t *)cookie;
    bsal_fast_queue_enqueue(recv_cookie->recv_queue, (void*)&(recv_cookie->recv_info));
}

void recv_message_fn(
        pami_context_t    context,      /**< IN: PAMI context */
        void            * cookie,       /**< IN: dispatch cookie */
        const void      * header,       /**< IN: header address */
        size_t            header_size,  /**< IN: header size */
        const void      * data,         /**< IN: address of PAMI pipe buffer */
        size_t            data_size,    /**< IN: size of PAMI pipe buffer */
        pami_endpoint_t   origin,
        pami_recv_t     * recv)         /**< OUT: receive message structure */
{
    struct thorium_pami_transport *pami_transport = (struct thorium_pami_transport *) cookie;

    void *buffer = pami_transport->recv_buffers[pami_transport->buf_index];
    pami_transport->buf_index = (pami_transport->buf_index+1)%NUM_RECV_BUFFERS;

    recv_cookie_t *recv_cookie = &(pami_transport->recv_cookies[pami_transport->recv_index]);
    pami_transport->recv_index = (pami_transport->recv_index+1)%NUM_RECV_COOKIES;

    recv_cookie->recv_queue = pami_transport->recv_queue;
    recv_cookie->recv_info.buffer = buffer;
    recv_cookie->recv_info.source = origin;
    recv_cookie->recv_info.count = data_size;

    if(recv == NULL) {
        /*fprintf (stderr, "recv_message_fn() active = %zd => %d\n", cookie_recv->active, cookie_recv->active-1);*/
        memcpy(buffer, data, data_size);
	bsal_fast_queue_enqueue(recv_cookie->recv_queue, (void*)&(recv_cookie->recv_info));
    }
    else {
        /*fprintf (stderr, "recv_message_fn() data_size = %d, active = %d\n", data_size, cookie_recv->active);*/
        recv->local_fn = recv_done_fn;
        recv->cookie   = (void *)recv_cookie;
        recv->type     = PAMI_TYPE_BYTE;
        recv->addr     = buffer;
        recv->offset   = 0;
        recv->data_fn  = PAMI_DATA_COPY;
        recv->data_cookie = NULL;
    }
    return;
}
