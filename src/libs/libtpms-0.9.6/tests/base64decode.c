#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libtpms/tpm_types.h>
#include <libtpms/tpm_library.h>
#include <libtpms/tpm_error.h>

static unsigned char *read_file(const char *name, size_t *len)
{
    long sz;
    unsigned char *res;
    FILE *f = fopen(name, "rb");

    if (!f) {
        printf("Could not open file %s for reading.", name);
        exit(EXIT_FAILURE);
    }

    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    res = malloc(sz + 1);
    if (res != NULL) {
        *len = fread(res, 1, sz, f);
        res[sz] = 0;
    }

    fclose(f);

    return res;
}

int main(int argc, char *argv[])
{
    int res = EXIT_SUCCESS;
    TPM_RESULT rc;
    unsigned char *buf_input = NULL, *buf_cmp = NULL;
    size_t len_input, len_cmp;
    unsigned char *result = NULL;
    size_t result_len;

    if (argc != 3) {
        printf("Need 2 files as parameters.\n");
        return EXIT_FAILURE;
    }

    buf_input = read_file(argv[1], &len_input);
    buf_cmp = read_file(argv[2], &len_cmp);

    rc = TPMLIB_DecodeBlob((char *)buf_input, TPMLIB_BLOB_TYPE_INITSTATE,
                           &result, &result_len);

    if (rc != TPM_SUCCESS) {
        printf("Decoding of the input file failed.\n");
        res = EXIT_FAILURE;
        goto cleanup;
    }

    if (result_len != len_cmp) {
        printf("Length of decoded blob (%zu) does "
               "not match length of 2nd file (%zu).\n",
               result_len, len_cmp);
        res = EXIT_FAILURE;
        goto cleanup;
    }

    if (memcmp(result, buf_cmp, result_len) != 0) {
       printf("Decoded blob does not match input from 2nd file.\n");
       res = EXIT_FAILURE;
       goto cleanup;
    }

cleanup:
    free(result);
    free(buf_cmp);
    free(buf_input);

    return res;
}
