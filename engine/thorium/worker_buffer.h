
#ifndef THORIUM_WORKER_BUFFER_H
#define THORIUM_WORKER_BUFFER_H

/*
 * This is a worker buffer.
 * if worker is lower than 0, then this means that this is
 * a Thorium core node buffer.
 */
struct thorium_worker_buffer {
    int worker;
    void *buffer;
};

void thorium_worker_buffer_init(struct thorium_worker_buffer *self, int worker, void *buffer);
void thorium_worker_buffer_destroy(struct thorium_worker_buffer *self);

int thorium_worker_buffer_get_worker(struct thorium_worker_buffer *self);
void *thorium_worker_buffer_get_buffer(struct thorium_worker_buffer *self);

#endif
