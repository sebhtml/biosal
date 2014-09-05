
#ifndef BSAL_DNA_KMER_H
#define BSAL_DNA_KMER_H

#include "dna_codec.h"

#include <core/system/memory_pool.h>

#include <stdint.h>

struct bsal_dna_kmer {
    void *encoded_data;
};

void bsal_dna_kmer_init(struct bsal_dna_kmer *self,
                char *data, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *memory);
void bsal_dna_kmer_destroy(struct bsal_dna_kmer *self, struct bsal_memory_pool *memory);

void bsal_dna_kmer_init_empty(struct bsal_dna_kmer *self);

int bsal_dna_kmer_unpack(struct bsal_dna_kmer *self,
                void *buffer, int kmer_length, struct bsal_memory_pool *memory, struct bsal_dna_codec *codec);
int bsal_dna_kmer_pack(struct bsal_dna_kmer *self,
                void *buffer, int kmer_length, struct bsal_dna_codec *codec);
int bsal_dna_kmer_pack_size(struct bsal_dna_kmer *self, int kmer_length, struct bsal_dna_codec *codec);
int bsal_dna_kmer_pack_unpack(struct bsal_dna_kmer *self,
                void *buffer, int operation, int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec);

int bsal_dna_kmer_length(struct bsal_dna_kmer *self, int kmer_length);
void bsal_dna_kmer_init_mock(struct bsal_dna_kmer *self, int kmer_length, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *memory);
void bsal_dna_kmer_init_random(struct bsal_dna_kmer *self, int kmer_length, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *memory);
void bsal_dna_kmer_init_copy(struct bsal_dna_kmer *self, struct bsal_dna_kmer *other,
                int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec);
void bsal_dna_kmer_init_as_child(struct bsal_dna_kmer *self, struct bsal_dna_kmer *other,
                int code, int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec);
void bsal_dna_kmer_init_as_parent(struct bsal_dna_kmer *self, struct bsal_dna_kmer *other,
                int code, int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec);

void bsal_dna_kmer_print(struct bsal_dna_kmer *self, int kmer_length, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *memory);
void bsal_dna_kmer_get_sequence(struct bsal_dna_kmer *self, char *sequence, int kmer_length,
        struct bsal_dna_codec *codec);

int bsal_dna_kmer_store_index(struct bsal_dna_kmer *self, int stores, int kmer_length,
        struct bsal_dna_codec *codec, struct bsal_memory_pool *memory);
int bsal_dna_kmer_pack_store_key(struct bsal_dna_kmer *self,
                void *buffer, int kmer_length, struct bsal_dna_codec *codec, struct bsal_memory_pool *memory);
int bsal_dna_kmer_pack_store_key_size(struct bsal_dna_kmer *self, int kmer_length);

uint64_t bsal_dna_kmer_hash(struct bsal_dna_kmer *self, int kmer_length, struct bsal_dna_codec *codec);

void bsal_dna_kmer_reverse_complement_self(struct bsal_dna_kmer *self, int kmer_length,
                struct bsal_dna_codec *codec, struct bsal_memory_pool *memory);

int bsal_dna_kmer_is_lower(struct bsal_dna_kmer *self, struct bsal_dna_kmer *other, int kmer_length,
                struct bsal_dna_codec *codec);
int bsal_dna_kmer_compare(struct bsal_dna_kmer *self, struct bsal_dna_kmer *other, int kmer_length,
                struct bsal_dna_codec *codec);
int bsal_dna_kmer_is_canonical(struct bsal_dna_kmer *self, int kmer_length,
                struct bsal_dna_codec *codec);

int bsal_dna_kmer_equals(struct bsal_dna_kmer *self, struct bsal_dna_kmer *kmer,
                int kmer_length, struct bsal_dna_codec *codec);

int bsal_dna_kmer_first_symbol(struct bsal_dna_kmer *self,
                int kmer_length, struct bsal_dna_codec *codec);
int bsal_dna_kmer_last_symbol(struct bsal_dna_kmer *self,
                int kmer_length, struct bsal_dna_codec *codec);

uint64_t bsal_dna_kmer_canonical_hash(struct bsal_dna_kmer *self, int kmer_length,
                struct bsal_dna_codec *codec, struct bsal_memory_pool *memory);

#endif
