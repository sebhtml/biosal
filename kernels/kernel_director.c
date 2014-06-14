
#include "kernel_director.h"

#include "kmer_counter_kernel.h"

#include <storage/sequence_store.h>

#include <helpers/actor_helper.h>
#include <helpers/message_helper.h>

#include <system/memory.h>
#include <system/debugger.h>

#include <stdio.h>

struct bsal_script bsal_kernel_director_script = {
    .name = BSAL_KERNEL_DIRECTOR_SCRIPT,
    .init = bsal_kernel_director_init,
    .destroy = bsal_kernel_director_destroy,
    .receive = bsal_kernel_director_receive,
    .size = sizeof(struct bsal_kernel_director)
};

void bsal_kernel_director_init(struct bsal_actor *actor)
{
    struct bsal_kernel_director *concrete_actor;

    concrete_actor = (struct bsal_kernel_director *)bsal_actor_concrete_actor(actor);

    bsal_queue_init(&concrete_actor->available_kernels, sizeof(int));
    bsal_vector_init(&concrete_actor->kernels, sizeof(int));

    concrete_actor->maximum_kernels = bsal_actor_node_worker_count(actor) * 4;
    concrete_actor->kmer_length = -1;
    concrete_actor->aggregator = -1;
}

void bsal_kernel_director_destroy(struct bsal_actor *actor)
{
    struct bsal_kernel_director *concrete_actor;

    concrete_actor = (struct bsal_kernel_director *)bsal_actor_concrete_actor(actor);

    bsal_queue_destroy(&concrete_actor->available_kernels);
    bsal_vector_destroy(&concrete_actor->kernels);
}

void bsal_kernel_director_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int kernel;
    int kernel_index;
    int name;
    int aggregator;
    int source;
    struct bsal_kernel_director *concrete_actor;

    concrete_actor = (struct bsal_kernel_director *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);
    source = bsal_message_source(message);
    name = bsal_actor_name(actor);

    if (tag == BSAL_SEQUENCE_STORE_RESERVE) {

        bsal_actor_helper_send_reply_empty(actor, BSAL_SEQUENCE_STORE_RESERVE_REPLY);

    } else if (tag == BSAL_PUSH_SEQUENCE_DATA_BLOCK) {

#ifdef BSAL_KERNEL_DIRECTOR_DEBUG
        BSAL_DEBUG_MARKER("received BSAL_PUSH_SEQUENCE_DATA_BLOCK");
#endif

        concrete_actor->received++;

        if (bsal_queue_empty(&concrete_actor->available_kernels)) {

            printf("kernel director actor/%d: no idle kernel, queuing message\n",
                            name);
            /* a kernel can be spawned ...
             */
            if (bsal_vector_size(&concrete_actor->kernels) < concrete_actor->maximum_kernels) {
                bsal_actor_helper_send_to_self_empty(actor, BSAL_SPAWN_KERNEL);
            }

            /* enqueue the message because it is impossible to fulfil it right
             * now.
             */
            bsal_actor_enqueue_message(actor, message);

            return;
        }

        /* a kernel is available to process the command
         */
        bsal_queue_dequeue(&concrete_actor->available_kernels, &kernel_index);
        kernel = bsal_actor_get_acquaintance(actor, kernel_index);

        /* just send the message to the kernel
         */
        bsal_actor_send(actor, kernel, message);

        /* tell the source that it's OK to send other commands now
         */
        bsal_actor_helper_send_reply_empty(actor, BSAL_PUSH_SEQUENCE_DATA_BLOCK_REPLY);

    } else if (tag == BSAL_SPAWN_KERNEL_REPLY) {

        /* a new kernel was spawned to handle the load.
         */
        bsal_message_helper_unpack_int(message, 0, &kernel);

        kernel_index = bsal_actor_add_acquaintance(actor, kernel);
        bsal_vector_push_back(&concrete_actor->kernels, &kernel_index);

        printf("director actor/%d now has %d kernels\n", name,
                        (int)bsal_vector_size(&concrete_actor->kernels));

        bsal_kernel_director_try_kernel(actor, kernel);

    } else if (tag == BSAL_SPAWN_KERNEL) {

        if (concrete_actor->kmer_length == -1) {
            printf("director actor/%d: error, kmer_length not set.\n",
                            name);
            return;
        }

        if (concrete_actor->aggregator == -1) {
            printf("director actor/%d: error, aggregator not set.\n",
                            name);
            return;
        }

        printf("kernel director actor/%d spawns a kernel\n",
                        name);

        /* spawn the kernel, but don't add it to the kernel index vector
         * just yet as it is not ready.
         * It most be trained.
         */
        kernel = bsal_actor_spawn(actor, BSAL_KMER_COUNTER_KERNEL_SCRIPT);

        bsal_actor_helper_send_int(actor, kernel, BSAL_SET_KMER_LENGTH, concrete_actor->kmer_length);

    } else if (tag == BSAL_SET_KMER_LENGTH_REPLY) {

        aggregator = bsal_actor_get_acquaintance(actor, concrete_actor->aggregator);

        bsal_actor_helper_send_reply_int(actor, BSAL_SET_CUSTOMER, aggregator);

    } else if (tag == BSAL_SET_CUSTOMER_REPLY) {

        bsal_actor_helper_send_to_self_int(actor, BSAL_SPAWN_KERNEL_REPLY, source);

    } else if (tag == BSAL_SET_KMER_LENGTH) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->kmer_length);

        bsal_actor_helper_send_reply_empty(actor, BSAL_SET_KMER_LENGTH_REPLY);

    } else if (tag == BSAL_PUSH_SEQUENCE_DATA_BLOCK_REPLY) {

        /* one of the kernel completed his work.
         */

#ifdef BSAL_KERNEL_DIRECTOR_DEBUG
        BSAL_DEBUG_MARKER("received BSAL_PUSH_SEQUENCE_DATA_BLOCK_REPLY from kernel");
#endif

        kernel = source;

        bsal_kernel_director_try_kernel(actor, kernel);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        printf("director actor/%d: spawned kernels: %d/%d, received %d data blocks\n",
                        bsal_actor_name(actor),
                        (int)bsal_vector_size(&concrete_actor->kernels),
                        concrete_actor->maximum_kernels,
                        concrete_actor->received);

        bsal_actor_helper_ask_to_stop(actor, message);

    } else if (tag == BSAL_SET_CUSTOMER) {

        bsal_message_helper_unpack_int(message, 0, &aggregator);

        concrete_actor->aggregator = bsal_actor_add_acquaintance(actor, aggregator);

        bsal_actor_helper_send_reply_empty(actor, BSAL_SET_CUSTOMER_REPLY);

    }
}

void bsal_kernel_director_try_kernel(struct bsal_actor *actor, int kernel)
{
    struct bsal_message new_message;
    int original_source;
    void *new_buffer;
    int kernel_index;
    struct bsal_kernel_director *concrete_actor;

#ifdef BSAL_KERNEL_DIRECTOR_DEBUG
    int name;
    name = bsal_actor_name(actor);
#endif

    concrete_actor = (struct bsal_kernel_director *)bsal_actor_concrete_actor(actor);

#ifdef BSAL_KERNEL_DIRECTOR_DEBUG
    printf("kernel director actor/%d tries to assign a message to kernel actor/%d\n",
                    name, kernel);
#endif

    /* if there are enqueued messages,
     * distribute one of them
     */
    if (bsal_actor_enqueued_message_count(actor) > 0) {

#ifdef BSAL_KERNEL_DIRECTOR_DEBUG
        BSAL_DEBUG_MARKER("got message");
#endif

        bsal_actor_dequeue_message(actor, &new_message);
        original_source = bsal_message_source(&new_message);

        bsal_actor_send(actor, kernel, &new_message);

        new_buffer = bsal_message_buffer(&new_message);

#ifdef BSAL_KERNEL_DIRECTOR_DEBUG
        BSAL_DEBUG_MARKER("TODO: patch memory leak");
#endif

        if (new_buffer != NULL) {
            /* free memory of queued message
             */
            bsal_free(new_buffer);
        }

#ifdef BSAL_KERNEL_DIRECTOR_DEBUG
        BSAL_DEBUG_MARKER("after freeing");
#endif

        new_buffer = NULL;

        /* tell the original source that it is OK
         * to continue now.
         */

#ifdef BSAL_KERNEL_DIRECTOR_DEBUG
        printf("DEBUG772 sending BSAL_PUSH_SEQUENCE_DATA_BLOCK_REPLY to original source %d\n",
                        original_source);
#endif

        bsal_actor_helper_send_empty(actor, original_source, BSAL_PUSH_SEQUENCE_DATA_BLOCK_REPLY);

#ifdef BSAL_KERNEL_DIRECTOR_DEBUG
        BSAL_DEBUG_MARKER("after sending");
#endif

    } else {

#ifdef BSAL_KERNEL_DIRECTOR_DEBUG
        BSAL_DEBUG_MARKER("no message");
#endif

        kernel_index = bsal_actor_get_acquaintance_index(actor, kernel);

        /* otherwise, enqueue the kernel
         * This case won't happen because kernels are spawned when they are needed.
         */
        bsal_queue_enqueue(&concrete_actor->available_kernels, &kernel_index);
    }
}

