
#include "actor_helper.h"
#include "vector_helper.h"

#include <core/structures/vector_iterator.h>
#include <engine/thorium/actor.h>
#include <engine/thorium/message.h>

#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void thorium_actor_send_vector(struct thorium_actor *actor, int destination,
                int tag, struct bsal_vector *vector)
{
    int count;
    struct thorium_message message;
    void *buffer;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    count = bsal_vector_pack_size(vector);
    buffer = bsal_memory_pool_allocate(ephemeral_memory, count);
    bsal_vector_pack(vector, buffer);

    thorium_message_init(&message, tag, count, buffer);

    thorium_actor_send(actor, destination, &message);

    bsal_memory_pool_free(ephemeral_memory, buffer);

    thorium_message_destroy(&message);
}

void thorium_actor_send_reply_vector(struct thorium_actor *actor, int tag, struct bsal_vector *vector)
{
    thorium_actor_send_vector(actor, thorium_actor_source(actor), tag, vector);
}

void thorium_actor_send_reply_empty(struct thorium_actor *actor, int tag)
{
    thorium_actor_send_empty(actor, thorium_actor_source(actor), tag);
}

void thorium_actor_send_to_self_empty(struct thorium_actor *actor, int tag)
{
    thorium_actor_send_empty(actor, thorium_actor_name(actor), tag);
}

void thorium_actor_send_empty(struct thorium_actor *actor, int destination, int tag)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, 0, NULL);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_to_supervisor_empty(struct thorium_actor *actor, int tag)
{
    thorium_actor_send_empty(actor, thorium_actor_supervisor(actor), tag);
}

void thorium_actor_send_to_supervisor_int(struct thorium_actor *actor, int tag, int value)
{
    thorium_actor_send_int(actor, thorium_actor_supervisor(actor), tag, value);
}

void thorium_actor_send_to_self_int(struct thorium_actor *actor, int tag, int value)
{
    thorium_actor_send_int(actor, thorium_actor_name(actor), tag, value);
}

void thorium_actor_send_reply_int(struct thorium_actor *actor, int tag, int value)
{
    thorium_actor_send_int(actor, thorium_actor_source(actor), tag, value);
}

void thorium_actor_send_reply_int64_t(struct thorium_actor *actor, int tag, int64_t value)
{
    thorium_actor_send_int64_t(actor, thorium_actor_source(actor), tag, value);
}

void thorium_actor_send_reply_uint64_t(struct thorium_actor *actor, int tag, uint64_t value)
{
    thorium_actor_send_uint64_t(actor, thorium_actor_source(actor), tag, value);
}

void thorium_actor_send_int(struct thorium_actor *actor, int destination, int tag, int value)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, sizeof(value), &value);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_uint64_t(struct thorium_actor *actor, int destination, int tag, uint64_t value)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, sizeof(value), &value);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_int64_t(struct thorium_actor *actor, int destination, int tag, int64_t value)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, sizeof(value), &value);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
void thorium_actor_get_acquaintances(struct thorium_actor *actor, struct bsal_vector *indices,
                struct bsal_vector *names)
{
    struct bsal_vector_iterator iterator;
    int index;
    int *bucket;
    int name;

#if 0 /* The caller must call the constructor first.
         */
    bsal_vector_init(names, sizeof(int));
#endif

    bsal_vector_iterator_init(&iterator, indices);

    while (bsal_vector_iterator_has_next(&iterator)) {
        bsal_vector_iterator_next(&iterator, (void **)&bucket);

        index = *bucket;
        name = thorium_actor_get_acquaintance(actor, index);

        bsal_vector_push_back(names, &name);
    }

    bsal_vector_iterator_destroy(&iterator);
}

#endif

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
void thorium_actor_add_acquaintances(struct thorium_actor *actor,
                struct bsal_vector *names, struct bsal_vector *indices)
{
    struct bsal_vector_iterator iterator;
    int index;
    int *bucket;
    int name;

    bsal_vector_iterator_init(&iterator, names);

    while (bsal_vector_iterator_has_next(&iterator)) {
        bsal_vector_iterator_next(&iterator, (void **)&bucket);

        name = *bucket;
        index = thorium_actor_add_acquaintance(actor, name);

#ifdef THORIUM_ACTOR_HELPER_DEBUG
        printf("DEBUG thorium_actor_add_acquaintances name %d index %d\n",
                        name, index);
#endif

        bsal_vector_push_back(indices, &index);
    }

    bsal_vector_iterator_destroy(&iterator);

#ifdef THORIUM_ACTOR_HELPER_DEBUG
    bsal_vector_print_int(names);
    printf("\n");
    bsal_vector_print_int(indices);
    printf("\n");
#endif
}
#endif

void thorium_actor_send_range(struct thorium_actor *actor, struct bsal_vector *actors,
                struct thorium_message *message)
{
        /*
    int real_source;

    real_source = thorium_actor_name(actor);
    */

    thorium_actor_send_range_standard(actor, actors, message);
/*
    thorium_actor_pack_proxy_message(actor, message, real_source);
    thorium_actor_send_range_binomial_tree(actor, actors, message);
    */
}

void thorium_actor_send_range_standard(struct thorium_actor *actor, struct bsal_vector *actors,
                struct thorium_message *message)
{
    int i;
    int first;
    int last;
    int the_actor;

    first = 0;
    last = bsal_vector_size(actors) - 1;

#ifdef THORIUM_ACTOR_DEBUG1
    printf("DEBUG thorium_actor_send_range_standard %i-%i\n",
                    first, last);
#endif

    i = first;

    while (i <= last) {

#ifdef THORIUM_ACTOR_DEBUG_20140601_1
        printf("DEBUG sending %d to %d\n",
                       thorium_message_tag(message), i);
#endif
        the_actor = *(int *)bsal_vector_at(actors, i);
        thorium_actor_send(actor, the_actor, message);
        i++;
    }
}

void thorium_actor_send_range_binomial_tree(struct thorium_actor *actor, struct bsal_vector *actors,
                struct thorium_message *message)
{
    int middle;
    int first1;
    int last1;
    int middle1;
    int first2;
    int last2;
    int middle2;
    void *buffer;
    void *new_buffer;
    int tag;
    int count;
    int new_count;
    int source;
    int offset;
    int first;
    int last;
    struct bsal_vector left_part;
    struct bsal_vector right_part;
    int left_actor;
    int right_actor;
    struct thorium_message new_message;
    int magic_offset;
    int limit;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    limit = 0;

    if (bsal_vector_size(actors) < limit) {
        thorium_actor_send_range_standard(actor, actors, message);
        return;
    }

    bsal_vector_init(&left_part, sizeof(int));
    bsal_vector_init(&right_part, sizeof(int));

#ifdef THORIUM_ACTOR_DEBUG_BINOMIAL_TREE
    int name;

    name = thorium_actor_name(actor);
#endif

    source = thorium_actor_name(actor);
    thorium_message_set_source(message, source);

    first = 0;
    last = bsal_vector_size(actors) - 1;
    middle = first + (last - first) / 2;

#ifdef THORIUM_ACTOR_DEBUG_BINOMIAL_TREE
    printf("DEBUG %i thorium_actor_send_range_binomial_tree\n", name);
    printf("DEBUG %i first: %i last: %i middle: %i\n", name, first, last, middle);
#endif

    first1 = first;
    last1 = middle - 1;
    first2 = middle;
    last2 = last;
    middle1 = first1 + (last1 - first1) / 2;
    middle2 = first2 + (last2 - first2) / 2;

    tag = thorium_message_tag(message);
    buffer = thorium_message_buffer(message);
    count = thorium_message_count(message);

    magic_offset = count + sizeof(source) + sizeof(tag);

    if (bsal_vector_size(&left_part) > 0) {
        bsal_vector_copy_range(actors, first1, last1, &left_part);

        /* TODO use slab allocator */
        new_count = count + sizeof(source) + sizeof(tag) + bsal_vector_pack_size(&left_part) + sizeof(magic_offset);
        new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

#ifdef THORIUM_ACTOR_DEBUG_BINOMIAL_TREE
        printf("DEBUG12 bsal_memory_pool_allocate %p (send_binomial_range)\n",
                    new_buffer);
#endif

        memcpy(new_buffer, buffer, count);

        /* send to the left actor
        */
        offset = count;
        memcpy((char *)new_buffer + offset, &source, sizeof(source));
        offset += sizeof(source);
        memcpy((char *)new_buffer + offset, &tag, sizeof(tag));
        offset += sizeof(tag);
        bsal_vector_pack(&left_part, (char *)new_buffer + offset);
        offset += bsal_vector_pack_size(&left_part);
        memcpy((char *)new_buffer + offset, &magic_offset, sizeof(magic_offset));
        offset += sizeof(magic_offset);

        thorium_message_init(&new_message, ACTION_BINOMIAL_TREE_SEND, new_count, new_buffer);

    #ifdef THORIUM_ACTOR_DEBUG_BINOMIAL_TREE
        printf("DEBUG111111 actor %i sending ACTION_BINOMIAL_TREE_SEND to %i, range is %i-%i\n",
                        name, middle1, first1, last1);
    #endif

        printf("DEBUG left part size: %d\n", (int)bsal_vector_size(&left_part));

        left_actor = *(int *)bsal_vector_at(&left_part, middle1);
        thorium_actor_send(actor, left_actor, &new_message);

        /* restore the buffer for the user */
        bsal_memory_pool_free(ephemeral_memory, new_buffer);
        bsal_vector_destroy(&left_part);
    }

    /* send to the right actor
     */

    bsal_vector_copy_range(actors, first2, last2, &right_part);

    if (bsal_vector_size(&right_part) > 0) {

        new_count = count + sizeof(source) + sizeof(tag) + bsal_vector_pack_size(&right_part) + sizeof(magic_offset);
        new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

        memcpy(new_buffer, buffer, count);
        offset = count;
        memcpy((char *)new_buffer + offset, &source, sizeof(source));
        offset += sizeof(source);
        memcpy((char *)new_buffer + offset, &tag, sizeof(tag));
        offset += sizeof(tag);
        bsal_vector_pack(&right_part, (char *)new_buffer + offset);
        offset += bsal_vector_pack_size(&right_part);
        memcpy((char *)new_buffer + offset, &magic_offset, sizeof(magic_offset));
        offset += sizeof(magic_offset);

#ifdef THORIUM_ACTOR_DEBUG_BINOMIAL_TREE2
        printf("DEBUG78 %i source: %i tag: %i ACTION_BINOMIAL_TREE_SEND\n",
                    name, source, tag);
#endif

        thorium_message_init(&new_message, ACTION_BINOMIAL_TREE_SEND, new_count, new_buffer);

#ifdef THORIUM_ACTOR_DEBUG_BINOMIAL_TREE
        printf("DEBUG %i sending ACTION_BINOMIAL_TREE_SEND to %i, range is %i-%i\n",
                    name, middle2, first2, last2);
#endif

        right_actor = *(int *)bsal_vector_at(&right_part, middle2);
        thorium_actor_send(actor, right_actor, &new_message);

        bsal_vector_destroy(&right_part);
        bsal_memory_pool_free(ephemeral_memory, new_buffer);
        new_buffer = NULL;
    }
}

void thorium_actor_send_range_empty(struct thorium_actor *actor, struct bsal_vector *actors,
                int tag)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, 0, NULL);
    thorium_actor_send_range(actor, actors, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_range_int(struct thorium_actor *actor, struct bsal_vector *actors,
                int tag, int value)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, sizeof(value), &value);
    thorium_actor_send_range(actor, actors, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_range_buffer(struct thorium_actor *actor, struct bsal_vector *destinations,
                int tag, int count, void *buffer)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, count, buffer);
    thorium_actor_send_range(actor, destinations, &message);
    thorium_message_destroy(&message);
}

void thorium_actor_send_range_vector(struct thorium_actor *actor, struct bsal_vector *actors,
                int tag, struct bsal_vector *vector)
{
    struct thorium_message message;
    int count;
    void *buffer;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    count = bsal_vector_pack_size(vector);
    buffer = bsal_memory_pool_allocate(ephemeral_memory, count);
    bsal_vector_pack(vector, buffer);
    thorium_message_init(&message, tag, count, buffer);
    thorium_actor_send_range(actor, actors, &message);
    thorium_message_destroy(&message);

    bsal_memory_pool_free(ephemeral_memory, buffer);
}

void thorium_actor_receive_binomial_tree_send(struct thorium_actor *actor, struct thorium_message *message)
{
    int real_tag;
    void *buffer;
    int count;
    int amount;
    int new_count;
    int limit;
    int real_source;
    int offset;
    struct bsal_vector actors;
    int magic_offset;

    limit = 42;
    count = thorium_message_count(message);
    buffer = thorium_message_buffer(message);

    offset = count - 1 - sizeof(magic_offset);

    magic_offset = *(int *)(char *)buffer + offset;
    bsal_vector_init(&actors, 0);
    bsal_vector_unpack(&actors, (char *)buffer + magic_offset);

    new_count = magic_offset;
    new_count -= sizeof(real_source);
    new_count -= sizeof(real_tag);

    offset = new_count;

    real_source = *(int *)((char *)buffer + offset);
    offset += sizeof(real_source);
    real_tag = *(int *)((char *)buffer + offset);
    offset += sizeof(real_tag);

#ifdef THORIUM_ACTOR_DEBUG
    printf("DEBUG78 actor %i received ACTION_BINOMIAL_TREE_SEND "
                    "real_source: %i real_tag: %i first: %i last: %i\n",
                    thorium_actor_name(actor), real_source, real_tag, first,
                    last);
#endif

    amount = bsal_vector_size(&actors);

    thorium_message_init(message, real_tag, new_count, buffer);

    if (amount < limit) {
        thorium_actor_send_range_standard(actor, &actors, message);
    } else {
        thorium_actor_send_range_binomial_tree(actor, &actors, message);
    }

    bsal_vector_destroy(&actors);
}

void thorium_actor_ask_to_stop(struct thorium_actor *actor, struct thorium_message *message)
{
    /* only the supervisor or self can
     * call this.
     */

    int source = thorium_message_source(message);
    int name = thorium_actor_name(actor);
    int supervisor = thorium_actor_supervisor(actor);

#ifdef THORIUM_ACTOR_STORE_CHILDREN
    int i;
    int child;
#endif

    if (source != name && source != supervisor) {
        printf("actor/%d: permission denied, will not stop (source: %d, name: %d, supervisor: %d\n",
                        thorium_actor_name(actor), source, name, supervisor);
#ifdef THORIUM_ACTOR_HELPER_DEBUG_STOP
#endif
        return;
    }

#ifdef THORIUM_ACTOR_STORE_CHILDREN
    for (i = 0; i < thorium_actor_child_count(actor); i++) {

        child = thorium_actor_get_child(actor, i);

#ifdef THORIUM_ACTOR_HELPER_DEBUG_STOP
        printf("actor/%d tells actor %d to stop\n",
                            thorium_actor_name(actor), child);
#endif

        thorium_actor_send_empty(actor, child, ACTION_ASK_TO_STOP);
    }

#ifdef THORIUM_ACTOR_HELPER_DEBUG_STOP
    printf("DEBUG121212 actor/%d dies\n",
                    thorium_actor_name(actor));

    printf("DEBUG actor/%d send ACTION_STOP to self\n",
                    thorium_actor_name(actor));
#endif

#endif

    /*
     * Stop self only if the message was sent by supervisor or self.
     * This is the default behavior and can be overwritten by
     * the concrete actor
     */
    thorium_actor_send_to_self_empty(actor, ACTION_STOP);

    thorium_actor_send_reply_empty(actor, ACTION_ASK_TO_STOP_REPLY);
}

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
int thorium_actor_get_acquaintance(struct thorium_actor *actor, struct bsal_vector *indices,
                int index)
{
    int index2;

    if (index >= bsal_vector_size(indices)) {
        return THORIUM_ACTOR_NOBODY;
    }

    index2 = bsal_vector_at_as_int(indices, index);

    return thorium_actor_get_acquaintance(actor, index2);
}
#endif

void thorium_actor_send_double(struct thorium_actor *actor, int destination, int tag, double value)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, sizeof(value), &value);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
int thorium_actor_get_acquaintance_index(struct thorium_actor *actor, struct bsal_vector *indices,
                int name)
{
    int i;
    int index;
    int actor_name;
    int size;

    size = bsal_vector_size(indices);


    for (i = 0; i < size; ++i) {
        index = bsal_vector_at_as_int(indices, i);
        actor_name = thorium_actor_get_acquaintance(actor, index);

        if (actor_name == name) {
            return i;
        }
    }

    return -1;
}
#endif

void thorium_actor_send_reply(struct thorium_actor *actor, struct thorium_message *message)
{
    thorium_actor_send(actor, thorium_actor_source(actor), message);
}

void thorium_actor_send_to_self(struct thorium_actor *actor, struct thorium_message *message)
{
    thorium_actor_send(actor, thorium_actor_name(actor), message);
}

void thorium_actor_send_to_supervisor(struct thorium_actor *actor, struct thorium_message *message)
{
    thorium_actor_send(actor, thorium_actor_supervisor(actor), message);
}

void thorium_actor_add_action_with_sources(struct thorium_actor *self, int tag,
                thorium_actor_receive_fn_t handler, struct bsal_vector *sources)
{
    int i;
    int source;
    int size;

    size = bsal_vector_size(sources);

    for (i = 0; i < size; i++) {

        source = bsal_vector_at_as_int(sources, i);

        thorium_actor_add_action_with_source(self, tag, handler, source);
    }
}

void thorium_actor_add_action(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler)
{

#ifdef THORIUM_ACTOR_DEBUG_10335
    if (tag == 10335) {
        printf("DEBUG actor %d thorium_actor_register 10335\n",
                        thorium_actor_name(self));
    }
#endif

    thorium_actor_add_action_with_source(self, tag, handler, THORIUM_ACTOR_ANYBODY);
}

void thorium_actor_add_action_with_source(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler,
                int source)
{
    thorium_actor_add_action_with_source_and_condition(self, tag,
                    handler, source, NULL, -1);
}

void thorium_actor_add_action_with_condition(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler, int *actual,
                int expected)
{
    thorium_actor_add_action_with_source_and_condition(self, tag, handler, THORIUM_ACTOR_ANYBODY, actual, expected);
}

void thorium_actor_send_to_self_buffer(struct thorium_actor *actor, int tag, int count, void *buffer)
{
    thorium_actor_send_buffer(actor, thorium_actor_name(actor),
                    tag, count, buffer);
}

void thorium_actor_send_buffer(struct thorium_actor *actor, int destination, int tag,
                int count, void *buffer)
{
    struct thorium_message message;

    thorium_message_init(&message, tag, count, buffer);
    thorium_actor_send(actor, destination, &message);
    thorium_message_destroy(&message);
}


