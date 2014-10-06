
#ifndef PROCESS_H
#define PROCESS_H

#include <biosal.h>

#define SCRIPT_TRANSPORT_PROCESS 0x8b4c0b93

/*
 * Test a transport implementation.
 */
struct process {
    struct core_vector actors;
    int ready;

    /*
     * State for events
     */
    int event_count;
    int concurrent_event_count;
    int events;
    int active_messages;

    /*
     * state for test results.
     */
    int passed;
    int failed;

    /*
     * Configuration for buffer size
     */
    int minimum_buffer_size;
    int maximum_buffer_size;
};

extern struct thorium_script process_script;

#endif
