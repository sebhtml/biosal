
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

    void (*init)(struct thorium_transport *self, int *argc, char ***argv);
    void (*destroy)(struct thorium_transport *self);

    int (*send)(struct thorium_transport *self, struct thorium_message *message);
    int (*receive)(struct thorium_transport *self, struct thorium_message *message);

    int (*test)(struct thorium_transport *self, struct thorium_worker_buffer *worker_buffer);
};

#endif
