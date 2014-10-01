
#ifndef BIOSAL_STRING_H
#define BIOSAL_STRING_H

struct biosal_memory_pool;

/*
 * A string of characters (bytes).
 */
struct biosal_string {

    char *data;
};

void biosal_string_init(struct biosal_string *self, const char *data);
void biosal_string_destroy(struct biosal_string *self);

void biosal_string_append(struct biosal_string *self, const char *data);
void biosal_string_prepend(struct biosal_string *self, const char *data);
char *biosal_string_get(struct biosal_string *self);

void biosal_string_combine(struct biosal_string *self, const char *data, int operation);

int biosal_string_pack_size(struct biosal_string *self);
int biosal_string_pack(struct biosal_string *self, void *buffer);
int biosal_string_unpack(struct biosal_string *self, void *buffer);
int biosal_string_pack_unpack(struct biosal_string *self, int operation, void *buffer);

int biosal_string_length(struct biosal_string *self);

void biosal_string_rotate_c_string(char *sequence, int length, int new_start);
void biosal_string_swap_c_string(char *sequence, int i, int j);
void biosal_string_reverse_c_string(char *sequence, int start, int end);

void biosal_string_rotate_path(char *sequence, int length, int rotation, int kmer_length,
                struct biosal_memory_pool *pool);

#endif
