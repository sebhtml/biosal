
#include "string.h"

#include <core/system/memory.h>

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
        strcat(new_data, string->data);
        if (string->data != NULL) {
            strcat(new_data, data);
        }
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


