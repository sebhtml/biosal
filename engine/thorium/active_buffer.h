
#ifndef BSAL_ACTIVE_BUFFER_H
#define BSAL_ACTIVE_BUFFER_H

#include "transport.h"

#include "mpi_active_buffer.h"
#include "pami_active_buffer.h"

struct bsal_active_buffer {

    void *buffer;
    int worker;

    int implementation;

#ifdef BSAL_TRANSPORT_USE_PAMI
    struct bsal_pami_active_buffer concrete_object;

#elif defined(BSAL_TRANSPORT_USE_MPI)
    struct bsal_mpi_active_buffer concrete_object;
#endif

    /*
     * Interface
     */
    void (*active_buffer_init)(struct bsal_active_buffer *active_buffer);
    void (*active_buffer_destroy)(struct bsal_active_buffer *active_buffer);
    int (*active_buffer_test)(struct bsal_active_buffer *active_buffer);
    void *(*active_buffer_request)(struct bsal_active_buffer *active_buffer);

};

void bsal_active_buffer_init(struct bsal_active_buffer *active_buffer, void *buffer, int worker);
void bsal_active_buffer_destroy(struct bsal_active_buffer *active_buffer);
int bsal_active_buffer_test(struct bsal_active_buffer *active_buffer);
void *bsal_active_buffer_request(struct bsal_active_buffer *active_buffer);
void *bsal_active_buffer_buffer(struct bsal_active_buffer *active_buffer);
int bsal_active_buffer_get_worker(struct bsal_active_buffer *active_buffer);

void *bsal_active_buffer_get_concrete_object(struct bsal_active_buffer *active_buffer);


#endif
