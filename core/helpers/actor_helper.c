
#include "actor_helper.h"
#include "vector_helper.h"

#include <core/structures/vector_iterator.h>
#include <core/engine/actor.h>
#include <core/engine/message.h>

#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void bsal_actor_helper_send_vector(struct bsal_actor *actor, int destination,
                int tag, struct bsal_vector *vector)
{
    int count;
    struct bsal_message message;
    void *buffer;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(actor);
    count = bsal_vector_pack_size(vector);
    buffer = bsal_memory_pool_allocate(ephemeral_memory, count);
    bsal_vector_pack(vector, buffer);

    bsal_message_init(&message, tag, count, buffer);

    bsal_actor_send(actor, destination, &message);

    bsal_memory_pool_free(ephemeral_memory, buffer);

    bsal_message_destroy(&message);
}

void bsal_actor_helper_send_reply_vector(struct bsal_actor *actor, int tag, struct bsal_vector *vector)
{
    bsal_actor_helper_send_vector(actor, bsal_actor_source(actor), tag, vector);
}

void bsal_actor_helper_send_reply_empty(struct bsal_actor *actor, int tag)
{
    bsal_actor_helper_send_empty(actor, bsal_actor_source(actor), tag);
}

void bsal_actor_helper_send_to_self_empty(struct bsal_actor *actor, int tag)
{
    bsal_actor_helper_send_empty(actor, bsal_actor_name(actor), tag);
}

void bsal_actor_helper_send_empty(struct bsal_actor *actor, int destination, int tag)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, 0, NULL);
    bsal_actor_send(actor, destination, &message);
}

void bsal_actor_helper_send_to_supervisor_empty(struct bsal_actor *actor, int tag)
{
    bsal_actor_helper_send_empty(actor, bsal_actor_supervisor(actor), tag);
}

void bsal_actor_helper_send_to_supervisor_int(struct bsal_actor *actor, int tag, int value)
{
    bsal_actor_helper_send_int(actor, bsal_actor_supervisor(actor), tag, value);
}

void bsal_actor_helper_send_to_self_int(struct bsal_actor *actor, int tag, int value)
{
    bsal_actor_helper_send_int(actor, bsal_actor_name(actor), tag, value);
}

void bsal_actor_helper_send_reply_int(struct bsal_actor *actor, int tag, int value)
{
    bsal_actor_helper_send_int(actor, bsal_actor_source(actor), tag, value);
}

void bsal_actor_helper_send_reply_int64_t(struct bsal_actor *actor, int tag, int64_t value)
{
    bsal_actor_helper_send_int64_t(actor, bsal_actor_source(actor), tag, value);
}

void bsal_actor_helper_send_reply_uint64_t(struct bsal_actor *actor, int tag, uint64_t value)
{
    bsal_actor_helper_send_uint64_t(actor, bsal_actor_source(actor), tag, value);
}

void bsal_actor_helper_send_int(struct bsal_actor *actor, int destination, int tag, int value)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, sizeof(value), &value);
    bsal_actor_send(actor, destination, &message);
}

void bsal_actor_helper_send_uint64_t(struct bsal_actor *actor, int destination, int tag, uint64_t value)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, sizeof(value), &value);
    bsal_actor_send(actor, destination, &message);
}

void bsal_actor_helper_send_int64_t(struct bsal_actor *actor, int destination, int tag, int64_t value)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, sizeof(value), &value);
    bsal_actor_send(actor, destination, &message);
}

void bsal_actor_helper_get_acquaintances(struct bsal_actor *actor, struct bsal_vector *indices,
                struct bsal_vector *names)
{
    struct bsal_vector_iterator iterator;
    int index;
    int *bucket;
    int name;

    bsal_vector_init(names, sizeof(int));

    bsal_vector_iterator_init(&iterator, indices);

    while (bsal_vector_iterator_has_next(&iterator)) {
        bsal_vector_iterator_next(&iterator, (void **)&bucket);

        index = *bucket;
        name = bsal_actor_get_acquaintance(actor, index);

        bsal_vector_push_back(names, &name);
    }

    bsal_vector_iterator_destroy(&iterator);
}

void bsal_actor_helper_add_acquaintances(struct bsal_actor *actor,
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
        index = bsal_actor_add_acquaintance(actor, name);

#ifdef BSAL_ACTOR_HELPER_DEBUG
        printf("DEBUG bsal_actor_helper_add_acquaintances name %d index %d\n",
                        name, index);
#endif

        bsal_vector_push_back(indices, &index);
    }

    bsal_vector_iterator_destroy(&iterator);

#ifdef BSAL_ACTOR_HELPER_DEBUG
    bsal_vector_helper_print_int(names);
    printf("\n");
    bsal_vector_helper_print_int(indices);
    printf("\n");
#endif
}

void bsal_actor_helper_send_range(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message)
{
        /*
    int real_source;

    real_source = bsal_actor_name(actor);
    */

    bsal_actor_helper_send_range_standard(actor, actors, message);
/*
    bsal_actor_pack_proxy_message(actor, message, real_source);
    bsal_actor_send_range_binomial_tree(actor, actors, message);
    */
}

void bsal_actor_helper_send_range_standard(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message)
{
    int i;
    int first;
    int last;
    int the_actor;

    first = 0;
    last = bsal_vector_size(actors) - 1;

#ifdef BSAL_ACTOR_DEBUG1
    printf("DEBUG bsal_actor_helper_send_range_standard %i-%i\n",
                    first, last);
#endif

    i = first;

    while (i <= last) {

#ifdef BSAL_ACTOR_DEBUG_20140601_1
        printf("DEBUG sending %d to %d\n",
                       bsal_message_tag(message), i);
#endif
        the_actor = *(int *)bsal_vector_at(actors, i);
        bsal_actor_send(actor, the_actor, message);
        i++;
    }
}

void bsal_actor_helper_send_range_binomial_tree(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message)
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
    struct bsal_message new_message;
    int magic_offset;
    int limit;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(actor);
    limit = 0;

    if (bsal_vector_size(actors) < limit) {
        bsal_actor_helper_send_range_standard(actor, actors, message);
        return;
    }

    bsal_vector_init(&left_part, sizeof(int));
    bsal_vector_init(&right_part, sizeof(int));

#ifdef BSAL_ACTOR_DEBUG_BINOMIAL_TREE
    int name;

    name = bsal_actor_name(actor);
#endif

    source = bsal_actor_name(actor);
    bsal_message_set_source(message, source);

    first = 0;
    last = bsal_vector_size(actors) - 1;
    middle = first + (last - first) / 2;

#ifdef BSAL_ACTOR_DEBUG_BINOMIAL_TREE
    printf("DEBUG %i bsal_actor_send_range_binomial_tree\n", name);
    printf("DEBUG %i first: %i last: %i middle: %i\n", name, first, last, middle);
#endif

    first1 = first;
    last1 = middle - 1;
    first2 = middle;
    last2 = last;
    middle1 = first1 + (last1 - first1) / 2;
    middle2 = first2 + (last2 - first2) / 2;

    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    count = bsal_message_count(message);

    magic_offset = count + sizeof(source) + sizeof(tag);

    if (bsal_vector_size(&left_part) > 0) {
        bsal_vector_copy_range(actors, first1, last1, &left_part);

        /* TODO use slab allocator */
        new_count = count + sizeof(source) + sizeof(tag) + bsal_vector_pack_size(&left_part) + sizeof(magic_offset);
        new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);

#ifdef BSAL_ACTOR_DEBUG_BINOMIAL_TREE
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

        bsal_message_init(&new_message, BSAL_ACTOR_BINOMIAL_TREE_SEND, new_count, new_buffer);

    #ifdef BSAL_ACTOR_DEBUG_BINOMIAL_TREE
        printf("DEBUG111111 actor %i sending BSAL_ACTOR_BINOMIAL_TREE_SEND to %i, range is %i-%i\n",
                        name, middle1, first1, last1);
    #endif

        printf("DEBUG left part size: %d\n", (int)bsal_vector_size(&left_part));

        left_actor = *(int *)bsal_vector_at(&left_part, middle1);
        bsal_actor_send(actor, left_actor, &new_message);

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

#ifdef BSAL_ACTOR_DEBUG_BINOMIAL_TREE2
        printf("DEBUG78 %i source: %i tag: %i BSAL_ACTOR_BINOMIAL_TREE_SEND\n",
                    name, source, tag);
#endif

        bsal_message_init(&new_message, BSAL_ACTOR_BINOMIAL_TREE_SEND, new_count, new_buffer);

#ifdef BSAL_ACTOR_DEBUG_BINOMIAL_TREE
        printf("DEBUG %i sending BSAL_ACTOR_BINOMIAL_TREE_SEND to %i, range is %i-%i\n",
                    name, middle2, first2, last2);
#endif

        right_actor = *(int *)bsal_vector_at(&right_part, middle2);
        bsal_actor_send(actor, right_actor, &new_message);

        bsal_vector_destroy(&right_part);
        bsal_memory_pool_free(ephemeral_memory, new_buffer);
        new_buffer = NULL;
    }
}

void bsal_actor_helper_send_range_empty(struct bsal_actor *actor, struct bsal_vector *actors,
                int tag)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, 0, NULL);
    bsal_actor_helper_send_range(actor, actors, &message);

    bsal_message_destroy(&message);
}

void bsal_actor_helper_send_range_int(struct bsal_actor *actor, struct bsal_vector *actors,
                int tag, int value)
{
    struct bsal_message message;

    bsal_message_init(&message, tag, sizeof(value), &value);
    bsal_actor_helper_send_range(actor, actors, &message);

    bsal_message_destroy(&message);
}

void bsal_actor_helper_send_range_vector(struct bsal_actor *actor, struct bsal_vector *actors,
                int tag, struct bsal_vector *vector)
{
    struct bsal_message message;
    int count;
    void *buffer;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(actor);
    count = bsal_vector_pack_size(vector);
    buffer = bsal_memory_pool_allocate(ephemeral_memory, count);
    bsal_vector_pack(vector, buffer);
    bsal_message_init(&message, tag, count, buffer);
    bsal_actor_helper_send_range(actor, actors, &message);

    bsal_message_destroy(&message);

    bsal_memory_pool_free(ephemeral_memory, buffer);
}

void bsal_actor_helper_receive_binomial_tree_send(struct bsal_actor *actor, struct bsal_message *message)
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
    count = bsal_message_count(message);
    buffer = bsal_message_buffer(message);

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

#ifdef BSAL_ACTOR_DEBUG
    printf("DEBUG78 actor %i received BSAL_ACTOR_BINOMIAL_TREE_SEND "
                    "real_source: %i real_tag: %i first: %i last: %i\n",
                    bsal_actor_name(actor), real_source, real_tag, first,
                    last);
#endif

    amount = bsal_vector_size(&actors);

    bsal_message_init(message, real_tag, new_count, buffer);

    if (amount < limit) {
        bsal_actor_helper_send_range_standard(actor, &actors, message);
    } else {
        bsal_actor_helper_send_range_binomial_tree(actor, &actors, message);
    }

    bsal_vector_destroy(&actors);
}

void bsal_actor_helper_ask_to_stop(struct bsal_actor *actor, struct bsal_message *message)
{
    /* only the supervisor or self can
     * call this.
     */

    int source = bsal_message_source(message);
    int name = bsal_actor_name(actor);
    int supervisor = bsal_actor_supervisor(actor);
    int i;
    int child;

    if (source != name && source != supervisor) {
        printf("actor/%d: permission denied, will not stop\n",
                        bsal_actor_name(actor));
        return;
    }

    for (i = 0; i < bsal_actor_child_count(actor); i++) {

        child = bsal_actor_get_child(actor, i);

#ifdef BSAL_ACTOR_HELPER_DEBUG_STOP
        printf("actor/%d tells actor %d to stop\n",
                            bsal_actor_name(actor), child);
#endif

        bsal_actor_helper_send_empty(actor, child, BSAL_ACTOR_ASK_TO_STOP);
    }

#ifdef BSAL_ACTOR_HELPER_DEBUG_STOP
    printf("DEBUG121212 actor/%d dies\n",
                    bsal_actor_name(actor));

    printf("DEBUG actor/%d send BSAL_ACTOR_STOP to self\n",
                    bsal_actor_name(actor));
#endif

    bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_STOP);

    bsal_actor_helper_send_reply_empty(actor, BSAL_ACTOR_ASK_TO_STOP_REPLY);
}

int bsal_actor_helper_get_acquaintance(struct bsal_actor *actor, struct bsal_vector *indices,
                int index)
{
    int index2;

    if (index >= bsal_vector_size(indices)) {
        return BSAL_ACTOR_NOBODY;
    }

    index2 = bsal_vector_helper_at_as_int(indices, index);

    return bsal_actor_get_acquaintance(actor, index2);
}


