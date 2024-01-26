#include <stddef.h>
#include "tpm_error.h"
#include "tpm_library_intern.h"

static TPM_RESULT Disabled_MainInit(void)
{
    return TPM_FAIL;
}

static void Disabled_Terminate(void)
{
}

static TPM_RESULT Disabled_Process(unsigned char **respbuffer, uint32_t *resp_size,
                               uint32_t *respbufsize,
                               unsigned char *command, uint32_t command_size)
{
    return TPM_FAIL;
}

static TPM_RESULT Disabled_VolatileAllStore(unsigned char **buffer,
                                        uint32_t *buflen)
{
    return TPM_FAIL;
}

static TPM_RESULT Disabled_CancelCommand(void)
{
    return TPM_FAIL;
}

static TPM_RESULT Disabled_GetTPMProperty(enum TPMLIB_TPMProperty prop,
                                      int *result)
{
    return TPM_FAIL;
}

static char *Disabled_GetInfo(enum TPMLIB_InfoFlags flags)
{
    return NULL;
}

static uint32_t Disabled_SetBufferSize(uint32_t wanted_size,
                                   uint32_t *min_size,
                                   uint32_t *max_size)
{
    return 0;
}

static TPM_RESULT Disabled_ValidateState(enum TPMLIB_StateType st,
                                     unsigned int flags)
{
    return TPM_FAIL;
}

static TPM_RESULT Disabled_GetState(enum TPMLIB_StateType st,
                                unsigned char **buffer, uint32_t *buflen)
{
    return TPM_FAIL;
}

static TPM_RESULT Disabled_SetState(enum TPMLIB_StateType st,
                                const unsigned char *buffer, uint32_t buflen)
{
    return TPM_FAIL;
}

static TPM_RESULT Disabled_IO_Hash_Start(void)
{
    return TPM_FAIL;
}

static TPM_RESULT Disabled_IO_Hash_Data(const unsigned char *data,
                                        uint32_t data_length)
{
    return TPM_FAIL;
}

static TPM_RESULT Disabled_IO_Hash_End(void)
{
    return TPM_FAIL;
}

static TPM_RESULT Disabled_IO_TpmEstablished_Get(TPM_BOOL *tpmEstablished)
{
    return TPM_FAIL;
}

static TPM_RESULT Disabled_IO_TpmEstablished_Reset(void)
{
    return TPM_FAIL;
}

const struct tpm_interface DisabledInterface = {
    .MainInit = Disabled_MainInit,
    .Terminate = Disabled_Terminate,
    .Process = Disabled_Process,
    .VolatileAllStore = Disabled_VolatileAllStore,
    .CancelCommand = Disabled_CancelCommand,
    .GetTPMProperty = Disabled_GetTPMProperty,
    .GetInfo = Disabled_GetInfo,
    .TpmEstablishedGet = Disabled_IO_TpmEstablished_Get,
    .TpmEstablishedReset = Disabled_IO_TpmEstablished_Reset,
    .HashStart = Disabled_IO_Hash_Start,
    .HashData = Disabled_IO_Hash_Data,
    .HashEnd = Disabled_IO_Hash_End,
    .SetBufferSize = Disabled_SetBufferSize,
    .ValidateState = Disabled_ValidateState,
    .SetState = Disabled_SetState,
    .GetState = Disabled_GetState,
};
