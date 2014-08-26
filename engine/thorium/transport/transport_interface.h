
#ifndef THORIUM_TRANSPORT_INTERFACE_H
#define THORIUM_TRANSPORT_INTERFACE_H

struct thorium_message;
struct thorium_transport;
struct thorium_worker_buffer;

/*
 * Interface for transport.
 */
struct thorium_transport_interface {
    char *name;
    int size;

    /*
     * Initialize the concrete transport
     */
    void (*init)(struct thorium_transport *self, int *argc, char ***argv);

    /*
     * Destroy the concrete transport
     */
    void (*destroy)(struct thorium_transport *self);

    /*
     * Send a message. This should be non-blocking.
     * \returns 1 if the message was sent. 0 Otherwise.
     */
    int (*send)(struct thorium_transport *self, struct thorium_message *message);

    /*
     * Receive a message. This should be non-blocking.
     *
     * \returns 1 if a message was received. 0 otherwise.
     */
    int (*receive)(struct thorium_transport *self, struct thorium_message *message);

    /*
     * Test to check if there are worker buffers to recycle.
     * \returns 1 if worker_buffer has been updated with the worker identifier
     * and buffer pointer.
     */
    int (*test)(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer);
};

#endif
