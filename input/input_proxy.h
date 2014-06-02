
#ifndef _BSAL_INPUT_PROXY_H
#define _BSAL_INPUT_PROXY_H

#include <formats/fastq_input.h>

struct bsal_dna_sequence;

struct bsal_input_proxy {
    struct bsal_input input;
    struct bsal_fastq_input fastq;

    int not_found;
    int not_supported;
    int done;
};

void bsal_input_proxy_init(struct bsal_input_proxy *proxy,
                char *file);
int bsal_input_proxy_get_sequence(struct bsal_input_proxy *proxy,
                struct bsal_dna_sequence *sequence);
void bsal_input_proxy_destroy(struct bsal_input_proxy *proxy);
int bsal_input_proxy_size(struct bsal_input_proxy *proxy);
int bsal_input_proxy_error(struct bsal_input_proxy *proxy);

void bsal_input_proxy_try(struct bsal_input_proxy *proxy,
                struct bsal_input *input, void *implementation,
                struct bsal_input_operations *operations, char *file);
#endif
