
#ifndef _BSAL_INPUT_PROXY_H
#define _BSAL_INPUT_PROXY_H

#include <data/dna_sequence.h>

struct bsal_input_proxy {
    int sequences;
};

void bsal_input_proxy_init(struct bsal_input_proxy *proxy,
                char *file);
int bsal_input_proxy_get_sequence(struct bsal_input_proxy *proxy,
                struct bsal_dna_sequence *sequence);
void bsal_input_proxy_destroy(struct bsal_input_proxy *proxy);
int bsal_input_proxy_size(struct bsal_input_proxy *proxy);

#endif
