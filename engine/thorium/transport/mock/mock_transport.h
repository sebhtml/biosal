
#ifndef THORIUM_MOCK_TRANSPORT_H
#define THORIUM_MOCK_TRANSPORT_H

#include <engine/thorium/transport/transport_interface.h>

/*
 * The mock transport only do one thing: it sets the rank to 0
 * and it sets the size to 1.
 *
 * It always fail when sending messages.
 *
 * This is a fallback mechanism so that the thorium_node can still
 * rely on the transport subsystem to obtain specific information
 * (rank and size).
 */
struct thorium_mock_transport {
    int foo;
};

extern struct thorium_transport_interface thorium_mock_transport_implementation;

#endif
