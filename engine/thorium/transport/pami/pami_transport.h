
#ifndef THORIUM_PAMI_TRANSPORT_H
#define THORIUM_PAMI_TRANSPORT_H

#include <engine/thorium/transport/transport_interface.h>

/*
 * Decide if the code will use PAMI or MPI
 */

/*
 * This variable is set to 1 if PAMI support is ready to be
 * used
 */
#define THORIUM_TRANSPORT_PAMI_IS_READY 0

/*
 * Use IBM PAMI on IBM Blue Gene/Q
 * PAMI: Parallel Active Message Interface
 */
#if defined(__bgq__) && THORIUM_TRANSPORT_PAMI_IS_READY

#define THORIUM_TRANSPORT_USE_PAMI

#endif

#if defined(THORIUM_TRANSPORT_USE_PAMI)

#include <pami.h>

#endif

struct thorium_node;
struct thorium_message;
struct thorium_transport;
struct thorium_worker_buffer;

/*
 * Transport using IBM PAMI (Parallel Active Message Interface)
 */
struct thorium_pami_transport {
#ifdef THORIUM_TRANSPORT_USE_PAMI
    pami_client_t client;
#endif

    /*
     * Avoid having nothing in the structure.
     */
    int mock;
};

extern struct thorium_transport_interface thorium_pami_transport_implementation;

void thorium_pami_transport_init(struct thorium_transport *self, int *argc, char ***argv);
void thorium_pami_transport_destroy(struct thorium_transport *self);

int thorium_pami_transport_send(struct thorium_transport *self, struct thorium_message *message);
int thorium_pami_transport_receive(struct thorium_transport *self, struct thorium_message *message);

int thorium_pami_transport_test(struct thorium_transport *self, struct thorium_worker_buffer *buffer);

#endif
