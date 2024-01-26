#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <libtpms/tpm_library.h>
#include <libtpms/tpm_error.h>
#include <libtpms/tpm_memory.h>

int main(void)
{
    unsigned char *rbuffer = NULL;
    uint32_t rlength;
    uint32_t rtotal = 0;
    TPM_RESULT res;
    int ret = 1;
    unsigned char tpm2_startup[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00,
        0x01, 0x44, 0x00, 0x00
    };
    unsigned char tpm2_selftest[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00,
        0x01, 0x43, 0x01
    };
    const unsigned char tpm2_selftest_resp[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00,
        0x00, 0x00
    };

    res = TPMLIB_ChooseTPMVersion(TPMLIB_TPM_VERSION_2);
    if (res) {
        fprintf(stderr, "TPMLIB_ChooseTPMVersion() failed: 0x%02x\n", res);
        goto exit;
    }

    res = TPMLIB_MainInit();
    if (res) {
        fprintf(stderr, "TPMLIB_MainInit() failed: 0x%02x\n", res);
        goto exit;
    }

    res = TPMLIB_Process(&rbuffer, &rlength, &rtotal,
                         tpm2_startup, sizeof(tpm2_startup));
    if (res) {
        fprintf(stderr, "TPMLIB_Process(Startup) failed: 0x%02x\n", res);
        goto exit;
    }

    res = TPMLIB_Process(&rbuffer, &rlength, &rtotal,
                         tpm2_selftest, sizeof(tpm2_selftest));
    if (res) {
        fprintf(stderr, "TPMLIB_Process(TPM2_Self) failed: 0x%02x\n",
                res);
        goto exit;
    }

    if (memcmp(rbuffer, tpm2_selftest_resp, rlength)) {
        fprintf(stderr, "Expected response from TPM2_Selftest is different than received one.\n");
        goto exit;
    }

    fprintf(stdout, "OK\n");

    ret = 0;

exit:
    TPMLIB_Terminate();
    TPM_Free(rbuffer);

    return ret;
}
