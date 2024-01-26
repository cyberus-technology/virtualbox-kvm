/********************************************************************************/
/*										*/
/*			LibTPM interface functions				*/
/*                        Written by Stefan Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_library.h $		*/
/*										*/
/* (c) Copyright IBM Corporation 2010.						*/
/*										*/
/* All rights reserved.								*/
/* 										*/
/* Redistribution and use in source and binary forms, with or without		*/
/* modification, are permitted provided that the following conditions are	*/
/* met:										*/
/* 										*/
/* Redistributions of source code must retain the above copyright notice,	*/
/* this list of conditions and the following disclaimer.			*/
/* 										*/
/* Redistributions in binary form must reproduce the above copyright		*/
/* notice, this list of conditions and the following disclaimer in the		*/
/* documentation and/or other materials provided with the distribution.		*/
/* 										*/
/* Neither the names of the IBM Corporation nor the names of its		*/
/* contributors may be used to endorse or promote products derived from		*/
/* this software without specific prior written permission.			*/
/* 										*/
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS		*/
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT		*/
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR	*/
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT		*/
/* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,	*/
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT		*/
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,	*/
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY	*/
/* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT		*/
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE	*/
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.		*/
/********************************************************************************/
#ifndef TPM_LIBRARY_H
#define TPM_LIBRARY_H

#include <stdint.h>
#include <sys/types.h>

#include "tpm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TPM_LIBRARY_VER_MAJOR 0
#define TPM_LIBRARY_VER_MINOR 9
#define TPM_LIBRARY_VER_MICRO 6

#define TPM_LIBRARY_VERSION_GEN(MAJ, MIN, MICRO) \
    (( MAJ << 16 ) | ( MIN << 8 ) | ( MICRO ))

#define TPM_LIBRARY_VERSION \
    TPM_LIBRARY_VERSION_GEN(TPM_LIBRARY_VER_MAJOR, \
                            TPM_LIBRARY_VER_MINOR, \
                            TPM_LIBRARY_VER_MICRO)


uint32_t TPMLIB_GetVersion(void);

/* TPM implementation version to choose */
typedef enum TPMLIB_TPMVersion {
    TPMLIB_TPM_VERSION_1_2,
    TPMLIB_TPM_VERSION_2,
} TPMLIB_TPMVersion;

TPM_RESULT TPMLIB_ChooseTPMVersion(TPMLIB_TPMVersion ver);
TPM_RESULT TPMLIB_MainInit(void);

void TPMLIB_Terminate(void);

TPM_RESULT TPMLIB_Process(unsigned char **respbuffer, uint32_t *resp_size,
                          uint32_t *respbufsize,
                          unsigned char *command, uint32_t command_size);

TPM_RESULT TPMLIB_VolatileAll_Store(unsigned char **buffer, uint32_t *buflen);

TPM_RESULT TPMLIB_CancelCommand(void);

enum TPMLIB_TPMProperty {
    TPMPROP_TPM_RSA_KEY_LENGTH_MAX = 1,
    TPMPROP_TPM_BUFFER_MAX,
    TPMPROP_TPM_KEY_HANDLES,
    TPMPROP_TPM_OWNER_EVICT_KEY_HANDLES,
    TPMPROP_TPM_MIN_AUTH_SESSIONS,
    TPMPROP_TPM_MIN_TRANS_SESSIONS,
    TPMPROP_TPM_MIN_DAA_SESSIONS,
    TPMPROP_TPM_MIN_SESSION_LIST,
    TPMPROP_TPM_MIN_COUNTERS,
    TPMPROP_TPM_NUM_FAMILY_TABLE_ENTRY_MIN,
    TPMPROP_TPM_NUM_DELEGATE_TABLE_ENTRY_MIN,
    TPMPROP_TPM_SPACE_SAFETY_MARGIN,
    TPMPROP_TPM_MAX_NV_SPACE,
    TPMPROP_TPM_MAX_SAVESTATE_SPACE,
    TPMPROP_TPM_MAX_VOLATILESTATE_SPACE,
};

TPM_RESULT TPMLIB_GetTPMProperty(enum TPMLIB_TPMProperty prop, int *result);

enum TPMLIB_InfoFlags {
    TPMLIB_INFO_TPMSPECIFICATION = 1,
    TPMLIB_INFO_TPMATTRIBUTES = 2,
    TPMLIB_INFO_TPMFEATURES = 4,
};

char *TPMLIB_GetInfo(enum TPMLIB_InfoFlags flags);

struct libtpms_callbacks {
    int sizeOfStruct;
    TPM_RESULT (*tpm_nvram_init)(void);
    TPM_RESULT (*tpm_nvram_loaddata)(unsigned char **data,
                                     uint32_t *length,
                                     uint32_t tpm_number,
                                     const char *name);
    TPM_RESULT (*tpm_nvram_storedata)(const unsigned char *data,
                                      uint32_t length,
                                      uint32_t tpm_number,
                                      const char *name);
    TPM_RESULT (*tpm_nvram_deletename)(uint32_t tpm_number,
                                       const char *name,
                                       TPM_BOOL mustExist);
    TPM_RESULT (*tpm_io_init)(void);
    TPM_RESULT (*tpm_io_getlocality)(TPM_MODIFIER_INDICATOR *localityModifer,
				     uint32_t tpm_number);
    TPM_RESULT (*tpm_io_getphysicalpresence)(TPM_BOOL *physicalPresence,
					     uint32_t tpm_number);
};

TPM_RESULT TPMLIB_RegisterCallbacks(struct libtpms_callbacks *);

enum TPMLIB_BlobType {
    TPMLIB_BLOB_TYPE_INITSTATE,

    TPMLIB_BLOB_TYPE_LAST,
};

#define TPMLIB_INITSTATE_START_TAG  "-----BEGIN INITSTATE-----"
#define TPMLIB_INITSTATE_END_TAG    "-----END INITSTATE-----"

TPM_RESULT TPMLIB_DecodeBlob(const char *data, enum TPMLIB_BlobType type,
                             unsigned char **result, size_t *result_len);

void TPMLIB_SetDebugFD(int fd);
void TPMLIB_SetDebugLevel(unsigned int level);
TPM_RESULT TPMLIB_SetDebugPrefix(const char *prefix);

uint32_t TPMLIB_SetBufferSize(uint32_t wanted_size,
                              uint32_t *min_size,
                              uint32_t *max_size);

enum TPMLIB_StateType {
    TPMLIB_STATE_PERMANENT  = (1 << 0),
    TPMLIB_STATE_VOLATILE   = (1 << 1),
    TPMLIB_STATE_SAVE_STATE = (1 << 2),
};

TPM_RESULT TPMLIB_ValidateState(enum TPMLIB_StateType st,
                                unsigned int flags);
TPM_RESULT TPMLIB_SetState(enum TPMLIB_StateType st,
                           const unsigned char *buffer, uint32_t buflen);
TPM_RESULT TPMLIB_GetState(enum TPMLIB_StateType st,
                           unsigned char **buffer, uint32_t *buflen);

#ifdef __cplusplus
}
#endif

#endif /* TPM_LIBRARY_H */
