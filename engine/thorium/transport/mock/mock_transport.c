
#include "mock_transport.h"

#include <engine/thorium/transport/transport.h>

static void thorium_mock_transport_init(struct thorium_transport *self, int *argc, char ***argv);
static void thorium_mock_transport_destroy(struct thorium_transport *self);

static int thorium_mock_transport_send(struct thorium_transport *self, struct thorium_message *message);
static int thorium_mock_transport_receive(struct thorium_transport *self, struct thorium_message *message);

static int thorium_mock_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer);

struct thorium_transport_interface thorium_mock_transport_implementation = {
    .name = "mock_transport",
    .size = sizeof(struct thorium_mock_transport),
    .init = thorium_mock_transport_init,
    .destroy = thorium_mock_transport_destroy,
    .send = thorium_mock_transport_send,
    .receive = thorium_mock_transport_receive,
    .test = thorium_mock_transport_test
};

static void thorium_mock_transport_init(struct thorium_transport *self, int *argc, char ***argv)
{
    self->rank = 0;
    self->size = 1;
    self->provided = THORIUM_THREAD_FUNNELED;
}

static void thorium_mock_transport_destroy(struct thorium_transport *self)
{
    self->rank = -1;
    self->size = -1;
    self->provided = -1;
}

static int thorium_mock_transport_send(struct thorium_transport *self, struct thorium_message *message)
{
    /*
     * Always fail
     */
    return 0;
}

static int thorium_mock_transport_receive(struct thorium_transport *self, struct thorium_message *message)
{
    /*
     * Always fail
     */
    return 0;
}

static int thorium_mock_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer)
{
    /*
     * Does nothing.
     */
    return 0;
}

