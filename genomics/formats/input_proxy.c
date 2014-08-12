
#include "input_proxy.h"

#include "input_format.h"
#include "input_format_interface.h"

#include <stdio.h>

#include <inttypes.h>

/*#define BSAL_INPUT_PROXY_DEBUG*/

void bsal_input_proxy_init(struct bsal_input_proxy *proxy,
                char *file, uint64_t offset, uint64_t maximum_offset)
{
#ifdef BSAL_INPUT_PROXY_DEBUG
    printf("DEBUG bsal_input_proxy_init open file %s @%" PRIu64 "\n", file,
                    offset);
#endif

    proxy->done = 0;

    /* Try to open the input with various formats
     */

    bsal_input_proxy_try(proxy, &proxy->input, &proxy->fastq,
                    &bsal_fastq_input_operations, file, offset, maximum_offset);

    bsal_input_proxy_try(proxy, &proxy->input, &proxy->fasta,
                    &bsal_fasta_input_operations, file, offset, maximum_offset);
}

int bsal_input_proxy_get_sequence(struct bsal_input_proxy *proxy,
                char *sequence)
{
    return bsal_input_format_get_sequence(&proxy->input, sequence);
}

void bsal_input_proxy_destroy(struct bsal_input_proxy *proxy)
{
#ifdef BSAL_INPUT_PROXY_DEBUG
    printf("DEBUG bsal_input_proxy_destroy\n");
#endif

    if (proxy->not_supported == 0) {
        bsal_input_format_destroy(&proxy->input);
    }

    proxy->not_supported = 1;
    proxy->not_found = 1;
    proxy->done = 1;
}

uint64_t bsal_input_proxy_size(struct bsal_input_proxy *proxy)
{
#ifdef BSAL_INPUT_PROXY_DEBUG12
    printf("DEBUG size %i\n", proxy->sequences);
#endif

    return bsal_input_format_size(&proxy->input);
}

uint64_t bsal_input_proxy_offset(struct bsal_input_proxy *self)
{

    return bsal_input_format_offset(&self->input);
}

int bsal_input_proxy_error(struct bsal_input_proxy *proxy)
{
    if (proxy->not_found) {
        return BSAL_INPUT_ERROR_FILE_NOT_FOUND;

    } else if (proxy->not_supported) {
        return BSAL_INPUT_ERROR_FORMAT_NOT_SUPPORTED;
    }

    return BSAL_INPUT_ERROR_NO_ERROR;
}

void bsal_input_proxy_try(struct bsal_input_proxy *proxy,
                struct bsal_input_format *input, void *implementation,
                struct bsal_input_format_interface *operations, char *file,
                uint64_t offset, uint64_t maximum_offset)
{
    int error;

#ifdef BSAL_INPUT_DEBUG
    printf("DEBUG bsal_input_proxy_try\n");
#endif

    if (proxy->done) {
        return;
    }

    /* Assume the worst case
     */
    proxy->not_supported = 1;
    proxy->not_found = 1;

    bsal_input_format_init(input, implementation, operations, file, offset,
                    maximum_offset);

    error = bsal_input_format_error(input);

    /* File does not exist.
     */
    if (error == BSAL_INPUT_ERROR_FILE_NOT_FOUND) {
        bsal_input_format_destroy(input);
        proxy->not_found = 1;
        proxy->not_supported = 0;
        proxy->done = 1;
        return;
    }

    /* Format is not supported
     */
    if (!bsal_input_format_detect(input)) {
        proxy->not_supported = 1;
        proxy->not_found = 0;
        bsal_input_format_destroy(input);

    } else {
        /* Found the format
         */
        proxy->not_supported = 0;
        proxy->not_found = 0;
        proxy->done = 1;

#ifdef BSAL_INPUT_PROXY_DEBUG
        printf("Found format.\n");
#endif
    }
}
