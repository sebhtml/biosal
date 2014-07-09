
#include "transport.h"

#include "message.h"
#include "active_buffer.h"

#include <core/system/memory.h>

/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Iprobe.html */
/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Recv.html */
/* \see http://www.malcolmmclean.site11.com/www/MpiTutorial/MPIStatus.html */
int bsal_transport_receive(struct bsal_transport *self, struct bsal_message *message)
{
    char *buffer;
    int count;
    int source;
    int destination;
    int tag;
    int flag;
    int metadata_size;
    MPI_Status status;

    source = MPI_ANY_SOURCE;
    tag = MPI_ANY_TAG;
    destination = self->rank;

    /* TODO get return value */
    MPI_Iprobe(source, tag, self->comm, &flag, &status);

    if (!flag) {
        return 0;
    }

    /* TODO get return value */
    MPI_Get_count(&status, self->datatype, &count);

    /* TODO actually allocate (slab allocator) a buffer with count bytes ! */
    buffer = (char *)bsal_memory_allocate(count * sizeof(char));

    source = status.MPI_SOURCE;
    tag = status.MPI_TAG;

    /* TODO get return value */
    MPI_Recv(buffer, count, self->datatype, source, tag,
                    self->comm, &status);

    metadata_size = bsal_message_metadata_size(message);
    count -= metadata_size;

    /* Initially assign the MPI source rank and MPI destination
     * rank for the actor source and actor destination, respectively.
     * Then, read the metadata and resolve the MPI rank from
     * that. The resolved MPI ranks should be the same in all cases
     */
    bsal_message_init(message, tag, count, buffer);
    bsal_message_set_source(message, destination);
    bsal_message_set_destination(message, destination);
    bsal_message_read_metadata(message);
    bsal_transport_resolve(self, message);

    return 1;
}

void bsal_transport_test_requests(struct bsal_transport *self)
{
    struct bsal_active_buffer active_buffer;
    void *buffer;

    if (bsal_ring_queue_dequeue(&self->active_buffers, &active_buffer)) {

        if (bsal_active_buffer_test(&active_buffer)) {
            buffer = bsal_active_buffer_buffer(&active_buffer);

            /* TODO use an allocator
             */
            bsal_memory_free(buffer);

            bsal_active_buffer_destroy(&active_buffer);

        /* Just put it back in the FIFO for later */
        } else {
            bsal_ring_queue_enqueue(&self->active_buffers, &active_buffer);
        }
    }
}


