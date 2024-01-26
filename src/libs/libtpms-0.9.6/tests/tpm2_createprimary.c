#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

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
    unsigned char *perm = NULL;
    uint32_t permlen = 0;
    unsigned char *vol = NULL;
    uint32_t vollen = 0;
    unsigned char startup[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00,
        0x01, 0x44, 0x00, 0x00
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

    res = TPMLIB_Process(&rbuffer, &rlength, &rtotal, startup, sizeof(startup));
    if (res) {
        fprintf(stderr, "TPMLIB_Process(Startup) failed: 0x%02x\n",
                res);
        goto exit;
    }

    unsigned char tpm2_createprimary[] = {
        0x80, 0x02, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00,
        0x01, 0x31, 0x40, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x09, 0x40, 0x00, 0x00, 0x09, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x1a, 0x00, 0x01, 0x00, 0x0b, 0x00,
        0x03, 0x04, 0x72, 0x00, 0x00, 0x00, 0x06, 0x00,
        0x80, 0x00, 0x43, 0x00, 0x10, 0x08, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00
     };

    res = TPMLIB_Process(&rbuffer, &rlength, &rtotal,
                         tpm2_createprimary, sizeof(tpm2_createprimary));
    if (res) {
        fprintf(stderr, "TPMLIB_Process(TPM2_CreatePrimary) failed: 0x%02x\n",
                res);
        goto exit;
    }

    if (rlength != 506) {
        fprintf(stderr, "Expected response is %u bytes, but got %u.\n",
                506, rlength);
        goto exit;
    }

    unsigned char tpm2_evictcontrol[] = {
        0x80, 0x02, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00,
        0x01, 0x20, 0x40, 0x00, 0x00, 0x01, 0x80, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x40, 0x00,
        0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81,
        0x00, 0x00, 0x00
    };

    const unsigned char tpm2_evictcontrol_exp_resp[] = {
        0x80, 0x02, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00
    };

    res = TPMLIB_Process(&rbuffer, &rlength, &rtotal,
                         tpm2_evictcontrol, sizeof(tpm2_evictcontrol));
    if (res) {
        fprintf(stderr, "TPMLIB_Process(TPM2_EvictControl) failed: %02x\n",
                res);
        goto exit;
    }

    if (rlength != sizeof(tpm2_evictcontrol_exp_resp)) {
        fprintf(stderr, "Expected TPM2_EvictControl response is %zu bytes, "
                "but got %u.\n",
                sizeof(tpm2_evictcontrol_exp_resp), rlength);
        goto exit;
    }

    if (memcmp(rbuffer, tpm2_evictcontrol_exp_resp, rlength)) {
        fprintf(stderr,
                "Expected TPM2_EvictControl response is different than "
                "received one.\n");
        goto exit;
    }

    /* Expecting a handle 0x81000000 for the persisted key now */
    unsigned char tpm2_getcapability[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00,
        0x01, 0x7a, 0x00, 0x00, 0x00, 0x01, 0x81, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40
    };
    const unsigned char tpm2_getcapability_exp_resp[] = {
        0x80, 0x01, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x00, 0x00, 0x01, 0x81, 0x00, 0x00, 0x00
    };

    res = TPMLIB_Process(&rbuffer, &rlength, &rtotal,
                         tpm2_getcapability, sizeof(tpm2_getcapability));
    if (res) {
        fprintf(stderr, "TPMLIB_Process(TPM2_GetCapability) failed: 0x%02x\n",
                res);
        goto exit;
    }

    if (rlength != sizeof(tpm2_getcapability_exp_resp)) {
        fprintf(stderr, "Expected TPM2_GetCapability response is %zu bytes, "
                "but got %u.\n", sizeof(tpm2_getcapability_exp_resp), rlength);
        goto exit;
    }

    if (memcmp(rbuffer, tpm2_getcapability_exp_resp, rlength)) {
        fprintf(stderr,
                "Expected TPM2_GetCapability response is different than "
                "received one.\n");
        goto exit;
    }

    /* save permanent and volatile state */
    res = TPMLIB_GetState(TPMLIB_STATE_PERMANENT, &perm, &permlen);
    if (res) {
        fprintf(stderr, "TPMLIB_GetState(PERMANENT) failed: 0x%02x\n", res);
        goto exit;
    }

    res = TPMLIB_GetState(TPMLIB_STATE_VOLATILE, &vol, &vollen);
    if (res) {
        fprintf(stderr, "TPMLIB_GetState(VOLATILE) failed: 0x%02x\n", res);
        goto exit;
    }

    /* terminate and resume where we left off */
    TPMLIB_Terminate();

    res = TPMLIB_SetState(TPMLIB_STATE_PERMANENT, perm, permlen);
    if (res) {
        fprintf(stderr, "TPMLIB_SetState(PERMANENT) failed: 0x%02x\n", res);
        goto exit;
    }

    res = TPMLIB_SetState(TPMLIB_STATE_VOLATILE, vol, vollen);
    if (res) {
        fprintf(stderr, "TPMLIB_SetState(VOLATILE) failed: 0x%02x\n", res);
        goto exit;
    }

    res = TPMLIB_MainInit();
    if (res) {
        fprintf(stderr, "TPMLIB_MainInit() after SetState failed: 0x%02x\n",
                res);
        goto exit;
    }

    /* Again expecting the handle 0x81000000 for the persisted key */
    res = TPMLIB_Process(&rbuffer, &rlength, &rtotal,
                         tpm2_getcapability, sizeof(tpm2_getcapability));
    if (res) {
        fprintf(stderr,
                "TPMLIB_Process(TPM2_GetCapability) failed: 0x%02x\n",
                res);
        goto exit;
    }

    if (rlength != sizeof(tpm2_getcapability_exp_resp)) {
        fprintf(stderr,
                "Expected TPM2_GetCapability response is %zu bytes,"
                "but got %u.\n",
                sizeof(tpm2_getcapability_exp_resp), rlength);
        goto exit;
    }

    if (memcmp(rbuffer, tpm2_getcapability_exp_resp, rlength)) {
        fprintf(stderr,
                "Expected TPM2_GetCapability response is different than "
                "received one.\n");
        goto exit;
    }

    /* Shutdown */
    unsigned char tpm2_shutdown[] = {
         0x80, 0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00,
         0x01, 0x45, 0x00, 0x00
    };

    res = TPMLIB_Process(&rbuffer, &rlength, &rtotal,
                         tpm2_shutdown, sizeof(tpm2_shutdown));
    if (res) {
        fprintf(stderr,
                "TPMLIB_Process(Shutdown) after SetState failed: 0x%02x\n",
                res);
        goto exit;
    }

    unsigned char tpm2_shutdown_resp[] = {
         0x80, 0x01, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00,
         0x00, 0x00
    };

    if (memcmp(tpm2_shutdown_resp, rbuffer, rlength)) {
        fprintf(stderr,
                "TPM2_PCRRead(Shutdown) after SetState did not return "
                "expected result\n");
        goto exit;
    }

    ret = 0;

    fprintf(stdout, "OK\n");

exit:
    free(perm);
    free(vol);
    TPMLIB_Terminate();
    TPM_Free(rbuffer);

    return ret;
}
