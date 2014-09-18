
#ifndef BSAL_STRING_H
#define BSAL_STRING_H

struct bsal_memory_pool;

/*
 * A string of characters (bytes).
 */
struct bsal_string {

    char *data;
};

void bsal_string_init(struct bsal_string *self, const char *data);
void bsal_string_destroy(struct bsal_string *self);

void bsal_string_append(struct bsal_string *self, const char *data);
void bsal_string_prepend(struct bsal_string *self, const char *data);
char *bsal_string_get(struct bsal_string *self);

void bsal_string_combine(struct bsal_string *self, const char *data, int operation);

int bsal_string_pack_size(struct bsal_string *self);
int bsal_string_pack(struct bsal_string *self, void *buffer);
int bsal_string_unpack(struct bsal_string *self, void *buffer);
int bsal_string_pack_unpack(struct bsal_string *self, int operation, void *buffer);

int bsal_string_length(struct bsal_string *self);

void bsal_string_rotate_c_string(char *sequence, int length, int new_start);
void bsal_string_swap_c_string(char *sequence, int i, int j);
void bsal_string_reverse_c_string(char *sequence, int start, int end);

void bsal_string_rotate_path(char *sequence, int length, int rotation, int kmer_length,
                struct bsal_memory_pool *pool);

#endif
