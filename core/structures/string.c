
#include "string.h"

#include <core/system/memory.h>
#include <core/system/memory_pool.h>
#include <core/system/packer.h>
#include <core/system/debugger.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define CORE_STRING_APPEND 100
#define CORE_STRING_PREPEND 200

#define MEMORY_STRING 0x89fd5efd

void core_string_init(struct core_string *string, const char *data)
{
    int length;

    string->data = NULL;

    if (data != NULL) {
        length = strlen(data);

        string->data = (char *)core_memory_allocate(length + 1, MEMORY_STRING);
        strcpy(string->data, data);
    }
}

void core_string_destroy(struct core_string *string)
{
    if (string->data != NULL) {
        core_memory_free(string->data, MEMORY_STRING);
        string->data = NULL;
    }
}

void core_string_append(struct core_string *string, const char *data)
{
    core_string_combine(string, data, CORE_STRING_APPEND);
}

void core_string_prepend(struct core_string *string, const char *data)
{
    core_string_combine(string, data, CORE_STRING_PREPEND);
}

char *core_string_get(struct core_string *string)
{
    return string->data;
}

void core_string_combine(struct core_string *string, const char *data, int operation)
{
    int old_length;
    int length;
    int new_length;
    char *new_data;

    /*
     * Nothing to do.
     */
    if (data == NULL) {
        return;
    }

    length = strlen(data);

    /* Nothing to combine
     */
    if (length == 0) {
        return;
    }

    old_length = 0;

    if (string->data != NULL) {
        old_length = strlen(string->data);
    }

    new_length = old_length + length;

    new_data = (char *)core_memory_allocate(new_length + 1, MEMORY_STRING);

    strcpy(new_data, "");

    if (operation == CORE_STRING_APPEND) {
        if (string->data != NULL) {
            strcat(new_data, string->data);
        }
        strcat(new_data, data);

    } else if (operation == CORE_STRING_PREPEND) {
        strcat(new_data, data);
        if (string->data != NULL) {
            strcat(new_data, string->data);
        }
    }

    if (string->data != NULL) {
        core_memory_free(string->data, MEMORY_STRING);
    }

    string->data = new_data;
}

int core_string_pack_size(struct core_string *self)
{
    return core_string_pack_unpack(self, CORE_PACKER_OPERATION_PACK_SIZE, NULL);
}

int core_string_pack(struct core_string *self, void *buffer)
{
    return core_string_pack_unpack(self, CORE_PACKER_OPERATION_PACK, buffer);
}

int core_string_unpack(struct core_string *self, void *buffer)
{
    return core_string_pack_unpack(self, CORE_PACKER_OPERATION_UNPACK, buffer);
}

int core_string_pack_unpack(struct core_string *self, int operation, void *buffer)
{
    struct core_packer packer;
    int bytes;
    int length;

    length = core_string_length(self);

    core_packer_init(&packer, operation, buffer);

    core_packer_process(&packer, &length, sizeof(length));

    if (operation == CORE_PACKER_OPERATION_UNPACK) {
        self->data = core_memory_allocate(length + 1, MEMORY_STRING);
    }

    core_packer_process(&packer, self->data, length + 1);

    bytes = core_packer_get_byte_count(&packer);

    core_packer_destroy(&packer);

    return bytes;
}

int core_string_length(struct core_string *self)
{
    if (self->data == NULL) {
        return 0;
    }

    return strlen(self->data);
}

void core_string_swap_c_string(char *sequence, int i, int j)
{
    char value;

    CORE_DEBUGGER_ASSERT(sequence != NULL);

    value = sequence[i];
    sequence[i] = sequence[j];
    sequence[j] = value;

#ifdef DEBUG_STRING
    printf("SWAP %d %d result %s\n", i, j, sequence);
#endif
}

void core_string_rotate_c_string(char *sequence, int length, int new_start)
{
    int offset;

    /*
     * in-place rotation
     */

    /*
     * Nothing to do.
     */
    if (new_start == 0)
        return;

#ifdef DEBUG_STRING
    printf("ROTATE %s %d\n", sequence, new_start);
#endif

    /*
     * See http://stackoverflow.com/questions/19316335/rotate-string-in-place-with-on
     */
    core_string_reverse_c_string(sequence, 0, length - 1);

    offset = length - new_start;

    core_string_reverse_c_string(sequence, 0, offset - 1);
    core_string_reverse_c_string(sequence, offset, length - 1);
}

void core_string_reverse_c_string(char *sequence, int start, int end)
{
#ifdef DEBUG_STRING
    printf("reverse before %s\n", sequence);
#endif

    while (start < end) {

        core_string_swap_c_string(sequence, start, end);
        ++start;
        --end;
    }

#ifdef DEBUG_STRING
    printf("reverse after %s\n", sequence);
#endif
}

void core_string_rotate_path(char *sequence, int length, int rotation, int kmer_length,
                struct core_memory_pool *pool)
{
    char *buffer;

    /*
     * Impossible.
     */
    if (length < kmer_length) {
        return;
    }

    /*
     * Simplify the rotation
     */
    rotation %= length;

    buffer = core_memory_pool_allocate(pool, length);

    /*
     * Algorithm:
     *
     * 1. Copy (l - r) from old @ r to new @ 0
     * 2. Copy (r - k + 1) from old @ (k - 1) to new @ (l - r)   (only if (r - k + 1 > 0))
     * 3. Copy (k - 1) from new @ 0 to new @ (l - k + 1)
     */

    core_memory_copy(buffer + 0, sequence + rotation, (length - rotation));

    /*
     * Copy the middle
     * */
    if ((rotation - kmer_length + 1) > 0)
        core_memory_copy(buffer + (length - rotation), sequence + (kmer_length - 1),
                    (rotation - kmer_length + 1));

    core_memory_copy(buffer + (length - kmer_length + 1), buffer + 0, (kmer_length - 1));

    /*
     * Copy the new sequence.
     */
    core_memory_copy(sequence, buffer, length);

    core_memory_pool_free(pool, buffer);
}
