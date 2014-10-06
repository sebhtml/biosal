
#ifndef CORE_STRING_H
#define CORE_STRING_H

struct core_memory_pool;

/*
 * A string of characters (bytes).
 */
struct core_string {

    char *data;
};

void core_string_init(struct core_string *self, const char *data);
void core_string_destroy(struct core_string *self);

void core_string_append(struct core_string *self, const char *data);
void core_string_prepend(struct core_string *self, const char *data);
char *core_string_get(struct core_string *self);

int core_string_pack_size(struct core_string *self);
int core_string_pack(struct core_string *self, void *buffer);
int core_string_unpack(struct core_string *self, void *buffer);
int core_string_pack_unpack(struct core_string *self, int operation, void *buffer);

int core_string_length(struct core_string *self);

void core_string_rotate_c_string(char *sequence, int length, int new_start);
void core_string_swap_c_string(char *sequence, int i, int j);
void core_string_reverse_c_string(char *sequence, int start, int end);

void core_string_rotate_path(char *sequence, int length, int rotation, int kmer_length,
                struct core_memory_pool *pool);

#endif
