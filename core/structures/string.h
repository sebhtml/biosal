
#ifndef BSAL_STRING_H
#define BSAL_STRING_H

struct bsal_string {

    char *data;
};

void bsal_string_init(struct bsal_string *string, const char *data);
void bsal_string_destroy(struct bsal_string *string);

void bsal_string_append(struct bsal_string *string, const char *data);
void bsal_string_prepend(struct bsal_string *string, const char *data);
char *bsal_string_get(struct bsal_string *string);

void bsal_string_combine(struct bsal_string *string, const char *data, int operation);

#endif
