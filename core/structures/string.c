
#include "string.h"

#include <core/system/memory.h>
#include <core/system/packer.h>

#include <string.h>
#include <stdlib.h>

#define BSAL_STRING_APPEND 100
#define BSAL_STRING_PREPEND 200

void bsal_string_init(struct bsal_string *string, const char *data)
{
    int length;

    string->data = NULL;

    if (data != NULL) {
        length = strlen(data);

        string->data = (char *)bsal_memory_allocate(length + 1);
        strcpy(string->data, data);
    }
}

void bsal_string_destroy(struct bsal_string *string)
{
    if (string->data != NULL) {
        bsal_memory_free(string->data);
        string->data = NULL;
    }
}

void bsal_string_append(struct bsal_string *string, const char *data)
{
    bsal_string_combine(string, data, BSAL_STRING_APPEND);
}

void bsal_string_prepend(struct bsal_string *string, const char *data)
{
    bsal_string_combine(string, data, BSAL_STRING_PREPEND);
}

char *bsal_string_get(struct bsal_string *string)
{
    return string->data;
}

void bsal_string_combine(struct bsal_string *string, const char *data, int operation)
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

    new_data = (char *)bsal_memory_allocate(new_length + 1);

    strcpy(new_data, "");

    if (operation == BSAL_STRING_APPEND) {
        if (string->data != NULL) {
            strcat(new_data, string->data);
        }
        strcat(new_data, data);

    } else if (operation == BSAL_STRING_PREPEND) {
        strcat(new_data, data);
        if (string->data != NULL) {
            strcat(new_data, string->data);
        }
    }

    if (string->data != NULL) {
        bsal_memory_free(string->data);
    }

    string->data = new_data;
}

int bsal_string_pack_size(struct bsal_string *self)
{
    return bsal_string_pack_unpack(self, BSAL_PACKER_OPERATION_PACK_SIZE, NULL);
}

int bsal_string_pack(struct bsal_string *self, void *buffer)
{
    return bsal_string_pack_unpack(self, BSAL_PACKER_OPERATION_PACK, buffer);
}

int bsal_string_unpack(struct bsal_string *self, void *buffer)
{
    return bsal_string_pack_unpack(self, BSAL_PACKER_OPERATION_UNPACK, buffer);
}

int bsal_string_pack_unpack(struct bsal_string *self, int operation, void *buffer)
{
    struct bsal_packer packer;
    int bytes;
    int length;

    length = bsal_string_length(self);

    bsal_packer_init(&packer, operation, buffer);

    bsal_packer_process(&packer, &length, sizeof(length));

    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        self->data = bsal_memory_allocate(length + 1);
    }

    bsal_packer_process(&packer, self->data, length + 1);

    bytes = bsal_packer_get_byte_count(&packer);

    bsal_packer_destroy(&packer);

    return bytes;
}

int bsal_string_length(struct bsal_string *self)
{
    if (self->data == NULL) {
        return 0;
    }

    return strlen(self->data);
}
