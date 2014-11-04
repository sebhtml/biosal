
#ifndef THORIUM_MOCK_TRANSPORT_H
#define THORIUM_MOCK_TRANSPORT_H

#include <engine/thorium/transport/transport_interface.h>

/*
 * MPI 1 point-to-point transport layer.
 */
struct thorium_mock_transport {
    int foo;
};

extern struct thorium_transport_interface thorium_mock_transport_implementation;

#endif
