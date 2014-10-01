
#ifndef BIOSAL_DNA_KMER_H
#define BIOSAL_DNA_KMER_H

#include "dna_codec.h"

#include <core/system/memory_pool.h>

#include <stdint.h>

struct biosal_dna_kmer {
    void *encoded_data;
};

void biosal_dna_kmer_init(struct biosal_dna_kmer *self,
                char *data, struct biosal_dna_codec *codec,
                struct biosal_memory_pool *memory);
void biosal_dna_kmer_destroy(struct biosal_dna_kmer *self, struct biosal_memory_pool *memory);

void biosal_dna_kmer_init_empty(struct biosal_dna_kmer *self);

int biosal_dna_kmer_unpack(struct biosal_dna_kmer *self,
                void *buffer, int kmer_length, struct biosal_memory_pool *memory, struct biosal_dna_codec *codec);
int biosal_dna_kmer_pack(struct biosal_dna_kmer *self,
                void *buffer, int kmer_length, struct biosal_dna_codec *codec);
int biosal_dna_kmer_pack_size(struct biosal_dna_kmer *self, int kmer_length, struct biosal_dna_codec *codec);
int biosal_dna_kmer_pack_unpack(struct biosal_dna_kmer *self,
                void *buffer, int operation, int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec);

int biosal_dna_kmer_length(struct biosal_dna_kmer *self, int kmer_length);
void biosal_dna_kmer_init_mock(struct biosal_dna_kmer *self, int kmer_length, struct biosal_dna_codec *codec,
                struct biosal_memory_pool *memory);
void biosal_dna_kmer_init_random(struct biosal_dna_kmer *self, int kmer_length, struct biosal_dna_codec *codec,
                struct biosal_memory_pool *memory);
void biosal_dna_kmer_init_copy(struct biosal_dna_kmer *self, struct biosal_dna_kmer *other,
                int kmer_length, struct biosal_memory_pool *memory, struct biosal_dna_codec *codec);
void biosal_dna_kmer_init_as_child(struct biosal_dna_kmer *self, struct biosal_dna_kmer *other,
                int code, int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec);
void biosal_dna_kmer_init_as_parent(struct biosal_dna_kmer *self, struct biosal_dna_kmer *other,
                int code, int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec);

void biosal_dna_kmer_print(struct biosal_dna_kmer *self, int kmer_length, struct biosal_dna_codec *codec,
                struct biosal_memory_pool *memory);
void biosal_dna_kmer_get_sequence(struct biosal_dna_kmer *self, char *sequence, int kmer_length,
        struct biosal_dna_codec *codec);

int biosal_dna_kmer_store_index(struct biosal_dna_kmer *self, int stores, int kmer_length,
        struct biosal_dna_codec *codec, struct biosal_memory_pool *memory);
int biosal_dna_kmer_pack_store_key(struct biosal_dna_kmer *self,
                void *buffer, int kmer_length, struct biosal_dna_codec *codec, struct biosal_memory_pool *memory);
int biosal_dna_kmer_pack_store_key_size(struct biosal_dna_kmer *self, int kmer_length);

uint64_t biosal_dna_kmer_hash(struct biosal_dna_kmer *self, int kmer_length, struct biosal_dna_codec *codec);

void biosal_dna_kmer_reverse_complement_self(struct biosal_dna_kmer *self, int kmer_length,
                struct biosal_dna_codec *codec, struct biosal_memory_pool *memory);

int biosal_dna_kmer_is_lower(struct biosal_dna_kmer *self, struct biosal_dna_kmer *other, int kmer_length,
                struct biosal_dna_codec *codec);
int biosal_dna_kmer_compare(struct biosal_dna_kmer *self, struct biosal_dna_kmer *other, int kmer_length,
                struct biosal_dna_codec *codec);
int biosal_dna_kmer_is_canonical(struct biosal_dna_kmer *self, int kmer_length,
                struct biosal_dna_codec *codec);

int biosal_dna_kmer_equals(struct biosal_dna_kmer *self, struct biosal_dna_kmer *kmer,
                int kmer_length, struct biosal_dna_codec *codec);

int biosal_dna_kmer_first_symbol(struct biosal_dna_kmer *self,
                int kmer_length, struct biosal_dna_codec *codec);
int biosal_dna_kmer_last_symbol(struct biosal_dna_kmer *self,
                int kmer_length, struct biosal_dna_codec *codec);
int biosal_dna_kmer_get_symbol(struct biosal_dna_kmer *self, int position,
                int kmer_length, struct biosal_dna_codec *codec);

uint64_t biosal_dna_kmer_canonical_hash(struct biosal_dna_kmer *self, int kmer_length,
                struct biosal_dna_codec *codec, struct biosal_memory_pool *memory);

#endif
