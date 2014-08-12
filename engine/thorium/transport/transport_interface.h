
#ifndef BSAL_TRANSPORT_INTERFACE_H
#define BSAL_TRANSPORT_INTERFACE_H

struct bsal_message;
struct bsal_transport;

/*
 * Interface for transport.
 */
struct bsal_transport_interface {
    int identifier;
    char *name;
    int size;
    void (*init)(struct bsal_transport *self, int *argc, char ***argv);
    void (*destroy)(struct bsal_transport *self);

    int (*send)(struct bsal_transport *self, struct bsal_message *message);
    int (*receive)(struct bsal_transport *self, struct bsal_message *message);
};

#endif
