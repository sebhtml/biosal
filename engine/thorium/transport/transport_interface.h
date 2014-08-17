
#ifndef BSAL_TRANSPORT_INTERFACE_H
#define BSAL_TRANSPORT_INTERFACE_H

struct thorium_message;
struct thorium_transport;

/*
 * Interface for transport.
 */
struct thorium_transport_interface {
    int identifier;
    char *name;
    int size;
    void (*init)(struct thorium_transport *self, int *argc, char ***argv);
    void (*destroy)(struct thorium_transport *self);

    int (*send)(struct thorium_transport *self, struct thorium_message *message);
    int (*receive)(struct thorium_transport *self, struct thorium_message *message);
};

#endif
