
#include "string.h"

#include <core/system/memory.h>
#include <core/system/memory_pool.h>
#include <core/system/packer.h>
#include <core/system/debugger.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define BIOSAL_STRING_APPEND 100
#define BIOSAL_STRING_PREPEND 200

#define MEMORY_STRING 0x89fd5efd

void biosal_string_init(struct biosal_string *string, const char *data)
{
    int length;

    string->data = NULL;

    if (data != NULL) {
        length = strlen(data);

        string->data = (char *)biosal_memory_allocate(length + 1, MEMORY_STRING);
        strcpy(string->data, data);
    }
}

void biosal_string_destroy(struct biosal_string *string)
{
    if (string->data != NULL) {
        biosal_memory_free(string->data, MEMORY_STRING);
        string->data = NULL;
    }
}

void biosal_string_append(struct biosal_string *string, const char *data)
{
    biosal_string_combine(string, data, BIOSAL_STRING_APPEND);
}

void biosal_string_prepend(struct biosal_string *string, const char *data)
{
    biosal_string_combine(string, data, BIOSAL_STRING_PREPEND);
}

char *biosal_string_get(struct biosal_string *string)
{
    return string->data;
}

void biosal_string_combine(struct biosal_string *string, const char *data, int operation)
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

    new_data = (char *)biosal_memory_allocate(new_length + 1, MEMORY_STRING);

    strcpy(new_data, "");

    if (operation == BIOSAL_STRING_APPEND) {
        if (string->data != NULL) {
            strcat(new_data, string->data);
        }
        strcat(new_data, data);

    } else if (operation == BIOSAL_STRING_PREPEND) {
        strcat(new_data, data);
        if (string->data != NULL) {
            strcat(new_data, string->data);
        }
    }

    if (string->data != NULL) {
        biosal_memory_free(string->data, MEMORY_STRING);
    }

    string->data = new_data;
}

int biosal_string_pack_size(struct biosal_string *self)
{
    return biosal_string_pack_unpack(self, BIOSAL_PACKER_OPERATION_PACK_SIZE, NULL);
}

int biosal_string_pack(struct biosal_string *self, void *buffer)
{
    return biosal_string_pack_unpack(self, BIOSAL_PACKER_OPERATION_PACK, buffer);
}

int biosal_string_unpack(struct biosal_string *self, void *buffer)
{
    return biosal_string_pack_unpack(self, BIOSAL_PACKER_OPERATION_UNPACK, buffer);
}

int biosal_string_pack_unpack(struct biosal_string *self, int operation, void *buffer)
{
    struct biosal_packer packer;
    int bytes;
    int length;

    length = biosal_string_length(self);

    biosal_packer_init(&packer, operation, buffer);

    biosal_packer_process(&packer, &length, sizeof(length));

    if (operation == BIOSAL_PACKER_OPERATION_UNPACK) {
        self->data = biosal_memory_allocate(length + 1, MEMORY_STRING);
    }

    biosal_packer_process(&packer, self->data, length + 1);

    bytes = biosal_packer_get_byte_count(&packer);

    biosal_packer_destroy(&packer);

    return bytes;
}

int biosal_string_length(struct biosal_string *self)
{
    if (self->data == NULL) {
        return 0;
    }

    return strlen(self->data);
}

void biosal_string_swap_c_string(char *sequence, int i, int j)
{
    char value;

    BIOSAL_DEBUGGER_ASSERT(sequence != NULL);

    value = sequence[i];
    sequence[i] = sequence[j];
    sequence[j] = value;

#ifdef DEBUG_STRING
    printf("SWAP %d %d result %s\n", i, j, sequence);
#endif
}

void biosal_string_rotate_c_string(char *sequence, int length, int new_start)
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
    biosal_string_reverse_c_string(sequence, 0, length - 1);

    offset = length - new_start;

    biosal_string_reverse_c_string(sequence, 0, offset - 1);
    biosal_string_reverse_c_string(sequence, offset, length - 1);
}

void biosal_string_reverse_c_string(char *sequence, int start, int end)
{
#ifdef DEBUG_STRING
    printf("reverse before %s\n", sequence);
#endif

    while (start < end) {

        biosal_string_swap_c_string(sequence, start, end);
        ++start;
        --end;
    }

#ifdef DEBUG_STRING
    printf("reverse after %s\n", sequence);
#endif
}

void biosal_string_rotate_path(char *sequence, int length, int rotation, int kmer_length,
                struct biosal_memory_pool *pool)
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

    buffer = biosal_memory_pool_allocate(pool, length);

    /*
     * Algorithm:
     *
     * 1. Copy (l - r) from old @ r to new @ 0
     * 2. Copy (r - k + 1) from old @ (k - 1) to new @ (l - r)   (only if (r - k + 1 > 0))
     * 3. Copy (k - 1) from new @ 0 to new @ (l - k + 1)
     */

    biosal_memory_copy(buffer + 0, sequence + rotation, (length - rotation));

    /*
     * Copy the middle
     * */
    if ((rotation - kmer_length + 1) > 0)
        biosal_memory_copy(buffer + (length - rotation), sequence + (kmer_length - 1),
                    (rotation - kmer_length + 1));

    biosal_memory_copy(buffer + (length - kmer_length + 1), buffer + 0, (kmer_length - 1));

    /*
     * Copy the new sequence.
     */
    biosal_memory_copy(sequence, buffer, length);

    biosal_memory_pool_free(pool, buffer);
}
