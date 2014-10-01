
#include "input_proxy.h"

#include "input_format.h"
#include "input_format_interface.h"

#include <stdio.h>

#include <inttypes.h>

/*#define BIOSAL_INPUT_PROXY_DEBUG*/

void biosal_input_proxy_init(struct biosal_input_proxy *proxy,
                char *file, uint64_t offset, uint64_t maximum_offset)
{
#ifdef BIOSAL_INPUT_PROXY_DEBUG
    printf("DEBUG biosal_input_proxy_init open file %s @%" PRIu64 "\n", file,
                    offset);
#endif

    proxy->done = 0;

    /* Try to open the input with various formats
     */

#ifdef BIOSAL_INPUT_PROXY_DEBUG
    printf("Trying fastq\n");
#endif
    biosal_input_proxy_try(proxy, &proxy->input, &proxy->fastq,
                    &biosal_fastq_input_operations, file, offset, maximum_offset);

#ifdef BIOSAL_INPUT_PROXY_DEBUG
    printf("Trying fasta\n");
#endif

    biosal_input_proxy_try(proxy, &proxy->input, &proxy->fasta,
                    &biosal_fasta_input_operations, file, offset, maximum_offset);
}

int biosal_input_proxy_get_sequence(struct biosal_input_proxy *proxy,
                char *sequence)
{
    int value;

    value = biosal_input_format_get_sequence(&proxy->input, sequence);

#if 0
    printf("DEBUG biosal_input_format_get_sequence %s\n",
                    sequence);
#endif

    return value;
}

void biosal_input_proxy_destroy(struct biosal_input_proxy *proxy)
{
#ifdef BIOSAL_INPUT_PROXY_DEBUG
    printf("DEBUG biosal_input_proxy_destroy\n");
#endif

    if (proxy->not_supported == 0) {
        biosal_input_format_destroy(&proxy->input);
    }

    proxy->not_supported = 1;
    proxy->not_found = 1;
    proxy->done = 1;
}

uint64_t biosal_input_proxy_size(struct biosal_input_proxy *proxy)
{
#ifdef BIOSAL_INPUT_PROXY_DEBUG12
    printf("DEBUG size %i\n", proxy->sequences);
#endif

    return biosal_input_format_size(&proxy->input);
}

uint64_t biosal_input_proxy_offset(struct biosal_input_proxy *self)
{

    return biosal_input_format_offset(&self->input);
}

int biosal_input_proxy_error(struct biosal_input_proxy *proxy)
{
    if (proxy->not_found) {
        return BIOSAL_INPUT_ERROR_FILE_NOT_FOUND;

    } else if (proxy->not_supported) {
        return BIOSAL_INPUT_ERROR_FORMAT_NOT_SUPPORTED;
    }

    return BIOSAL_INPUT_ERROR_NO_ERROR;
}

void biosal_input_proxy_try(struct biosal_input_proxy *proxy,
                struct biosal_input_format *input, void *implementation,
                struct biosal_input_format_interface *operations, char *file,
                uint64_t offset, uint64_t maximum_offset)
{
    int error;

#ifdef BIOSAL_INPUT_DEBUG
    printf("DEBUG biosal_input_proxy_try\n");
#endif

    if (proxy->done) {
        return;
    }

    /* Assume the worst case
     */
    proxy->not_supported = 1;
    proxy->not_found = 1;

    biosal_input_format_init(input, implementation, operations, file, offset,
                    maximum_offset);

    error = biosal_input_format_error(input);

    /* File does not exist.
     */
    if (error == BIOSAL_INPUT_ERROR_FILE_NOT_FOUND) {
        biosal_input_format_destroy(input);
        proxy->not_found = 1;
        proxy->not_supported = 0;
        proxy->done = 1;
        return;
    }

    /* Format is not supported
     */
    if (!biosal_input_format_detect(input)) {
        proxy->not_supported = 1;
        proxy->not_found = 0;
        biosal_input_format_destroy(input);

    } else {
        /* Found the format
         */
        proxy->not_supported = 0;
        proxy->not_found = 0;
        proxy->done = 1;

#ifdef BIOSAL_INPUT_PROXY_DEBUG
        printf("Found format.\n");
#endif
    }
}
