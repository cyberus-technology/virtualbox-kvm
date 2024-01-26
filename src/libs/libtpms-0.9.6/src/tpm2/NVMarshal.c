/********************************************************************************/
/*										*/
/*			  Marshalling and unmarshalling of state		*/
/*			     Written by Stefan Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*										*/
/* (c) Copyright IBM Corporation 2017,2018.					*/
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

#include <string.h>
#include <inttypes.h>

#include "assert.h"

#define _CRYPT_HASH_C_
#define SESSION_PROCESS_C
#define NV_C
#define OBJECT_C
#define PCR_C
#define SESSION_C
#include "Platform.h"
#include "NVMarshal.h"
#include "Marshal_fp.h"
#include "Unmarshal_fp.h"
#include "Global.h"
#include "TpmTcpProtocol.h"
#include "Simulator_fp.h"

#define TPM_HAVE_TPM2_DECLARATIONS
#include "tpm_library_intern.h"

/*
 * The TPM2 maintains a pcrAllocated shadow variable; the current active one is
 * in gp.pcrAllocated and the one to be active after reboot is in NVRAM. So,
 * this means we have to restore two of these variables when we resume. The
 * tricky part is that the global gp will be restored by reading from NVRAM.
 * Once that has been done the gp.pcrAllocated needs to be restored with the
 * one that is supposed to be active. All of this is only supposed to happen
 * when we resume a VM's volatile state.
 */
static struct shadow {
    TPML_PCR_SELECTION  pcrAllocated;
    BOOL                pcrAllocatedIsNew;
} shadow;

/* prevent misconfiguration: */
#ifndef VBOX
typedef char assertion_failed_nvram[
    (NV_USER_DYNAMIC_END < NV_USER_DYNAMIC) ? -1 : 0];
#else
# include <iprt/assert.h>
# include <iprt/cdefs.h>
# include <iprt/mem.h>

AssertCompile(NV_USER_DYNAMIC_END >= NV_USER_DYNAMIC);
#endif

typedef struct
{
    UINT16 version;
    UINT32 magic;
    UINT16 min_version; /* min. implementation version to accept the blob */
} NV_HEADER;

static UINT8 BOOL_Marshal(BOOL *boolean, BYTE **buffer, INT32 *size);
static TPM_RC BOOL_Unmarshal(BOOL *boolean, BYTE **buffer, INT32 *size);

/*
 * There are compile-time optional variables that we marshal. To allow
 * for some flexibility, we marshal them in such a way that these
 * variables can be skipped if they are in the byte stream but are not
 * needed by the implementation. The following block_skip data structure
 * and related functions address this issue.
 */
typedef struct {
    size_t idx;
    size_t sz;
    struct position {
        BYTE *buffer;
        INT32 size;
    } pos[5]; /* more only needed for nested compile-time #ifdef's */
} block_skip;

/*
 * This function is to be called when an optional block follows. It inserts
 * a BOOL into the byte stream indicating whether the block is there or not.
 * Then it leaves a 16bit zero in the byt stream and remembers the location
 * of that zero. We will update the location with the number of optional
 * bytes written when block_skip_write_pop() is called.
 */
static UINT16
block_skip_write_push(block_skip *bs, BOOL has_block,
                      BYTE **buffer, INT32 *size) {
    UINT16 written , w;
    UINT16 zero = 0;
    written = BOOL_Marshal(&has_block, buffer, size);
    bs->pos[bs->idx].buffer = *buffer;
    bs->pos[bs->idx].size = *size;
    w = UINT16_Marshal(&zero, buffer, size);
    if (w) {
        bs->idx++;
        pAssert(bs->idx < bs->sz);
        written += w;
    }
    return written;
}

/*
 * This function must be called for every block_skip_write_push() call.
 * It has to be called once a compile-time optional block has been
 * processed. It must be called after the #endif.
 * In this function we updated the previously remembered location with
 * the numbers of bytes to skip in case a block is there but it is not
 * needed.
 */
static void
block_skip_write_pop(block_skip *bs, INT32 *size) {
    UINT16 skip;
    unsigned i = --bs->idx;
    pAssert((int)bs->idx >= 0);
    skip = bs->pos[i].size - *size - sizeof(UINT16);
    UINT16_Marshal(&skip, &bs->pos[i].buffer, &bs->pos[i].size);
}

/*
 * This function must be called when unmarshalling a byte stream and
 * a compile-time optional block follows. In case the compile-time
 * optional block is there but not in the byte stream, we log an error.
 * In case the bytes stream contains the block, but we don't need it
 * we skip it. In the other cases we don't need to do anything since
 * the code is 'in sync' with the byte stream.
 */
static TPM_RC
block_skip_read(BOOL needs_block, BYTE **buffer, INT32 *size,
                const char *name, const char *field,
                BOOL *skip_code)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    BOOL has_block;
    UINT16 blocksize;

    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&has_block, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&blocksize, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        if (!has_block && needs_block) {
            TPMLIB_LogTPM2Error("%s needs missing %s\n", name, field);
            rc = TPM_RC_BAD_PARAMETER;
        } else if (has_block && !needs_block) {
            /* byte stream has the data but we don't need them */
            *buffer += blocksize;
            *size -= blocksize;
            *skip_code = TRUE;
        }
    }
    return rc;
}

#define BLOCK_SKIP_INIT				\
    block_skip block_skip = {			\
        .idx = 0,				\
        .sz = ARRAY_SIZE(block_skip.pos),	\
    }

#define BLOCK_SKIP_WRITE_PUSH(HAS_BLOCK, BUFFER, POS) \
    block_skip_write_push(&block_skip, HAS_BLOCK, BUFFER, POS)

#define BLOCK_SKIP_WRITE_POP(SIZE) \
    block_skip_write_pop(&block_skip, SIZE)

#define BLOCK_SKIP_WRITE_CHECK \
    pAssert(block_skip.idx == 0)

#define BLOCK_SKIP_READ(SKIP_MARK, NEEDS_BLOCK, BUFFER, SIZE, NAME, FIELD) \
    {									\
        BOOL skip_code = FALSE;						\
        rc = block_skip_read(NEEDS_BLOCK, buffer, size, 		\
                             NAME, FIELD, &skip_code);			\
        if (rc == TPM_RC_SUCCESS && skip_code)				\
            goto SKIP_MARK;						\
    }

static unsigned int _ffsll(long long bits)
{
    size_t i = 0;

    for (i = 0; i < 8 * sizeof(bits); i++) {
        if (bits & (1ULL << i))
            return i + 1;
    }
    return 0;
}

/* BOOL is 'int' but we store a single byte */
static UINT8
BOOL_Marshal(BOOL *boolean, BYTE **buffer, INT32 *size)
{
    UINT8 _bool = (*boolean != 0);
    UINT16 written = 0;
    written += UINT8_Marshal(&_bool, buffer, size);
    return written;
}

static TPM_RC
BOOL_Unmarshal(BOOL *boolean, BYTE **buffer, INT32 *size)
{
    UINT8 _bool;
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT8_Unmarshal(&_bool, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS) {
        *boolean = (_bool != 0);
    }

    return rc;
}

static UINT16
SEED_COMPAT_LEVEL_Marshal(SEED_COMPAT_LEVEL *source,
                          BYTE **buffer, INT32 *size)
{
    return UINT8_Marshal((UINT8 *)source, buffer, size);
}

static TPM_RC
SEED_COMPAT_LEVEL_Unmarshal(SEED_COMPAT_LEVEL *source,
                            BYTE **buffer, INT32 *size,
                            const char *name)
{
    TPM_RC rc;

    rc = UINT8_Unmarshal((UINT8 *)source, buffer, size);
    if (rc == TPM_RC_SUCCESS && *source > SEED_COMPAT_LEVEL_LAST) {
        TPMLIB_LogTPM2Error("%s compatLevel '%u' higher than supported '%u'\n",
                            name, *source, SEED_COMPAT_LEVEL_LAST);
        rc = TPM_RC_BAD_VERSION;
    }
    return rc;
}

static int
TPM2B_Cmp(const TPM2B *t1, const TPM2B *t2)
{
    if (t1->size != t2->size)
        return 1;

    return memcmp(t1->buffer, t2->buffer, t1->size);
}

static UINT16
TPM2B_PROOF_Marshal(TPM2B_PROOF *source, BYTE **buffer, INT32 *size)
{
    UINT16 written = 0;
    written += TPM2B_Marshal(&source->b, sizeof(source->t.buffer), buffer, size);
    return written;
}

static TPM_RC
TPM2B_PROOF_Unmarshal(TPM2B_PROOF *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, sizeof(target->t.buffer), buffer, size);
    }
    return rc;
}

static TPM_RC
UINT32_Unmarshal_Check(UINT32 *data, UINT32 exp, BYTE **buffer, INT32 *size,
                       const char *msg)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(data, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS && exp != *data) {
        TPMLIB_LogTPM2Error("%s: Expected value: 0x%08x, found: 0x%08x\n",
                            __func__, exp, *data);
        rc = TPM_RC_BAD_TAG;
    }
    return rc;
}

static void
NV_HEADER_INIT(NV_HEADER *t, UINT16 version, UINT32 magic, UINT16 min_version)
{
    t->version = version;
    t->magic = magic;
    t->min_version = min_version;
}

static UINT16
NV_HEADER_Marshal(BYTE **buffer, INT32 *size, UINT16 version, UINT32 magic,
                  UINT16 min_version)
{
    UINT16 written;
    NV_HEADER hdr;

    NV_HEADER_INIT(&hdr, version, magic, min_version);

    written = UINT16_Marshal(&hdr.version, buffer, size);
    written += UINT32_Marshal(&hdr.magic, buffer, size);
    if (version >= 2)
        written += UINT16_Marshal(&hdr.min_version, buffer, size);

    return written;
}

static TPM_RC
NV_HEADER_UnmarshalVerbose(NV_HEADER *data, BYTE **buffer, INT32 *size,
                           UINT16 cur_version, UINT32 exp_magic, BOOL verbose)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&data->version, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->magic, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS && exp_magic != data->magic) {
        if (verbose)
            TPMLIB_LogTPM2Error("%s: Invalid magic. Expected 0x%08x, got 0x%08x\n",
                                __func__, exp_magic, data->magic);
        rc = TPM_RC_BAD_TAG;
    }

    data->min_version = 0;
    if (rc == TPM_RC_SUCCESS && data->version >= 2) {

        rc = UINT16_Unmarshal(&data->min_version, buffer, size);

        if (rc == TPM_RC_SUCCESS && data->min_version > cur_version) {
            if (verbose)
                TPMLIB_LogTPM2Error("%s: Minimum version %u higher than "
                                    "implementation version %u for type 0x%08x\n",
                                    __func__, data->min_version, cur_version,
                                    exp_magic);
            rc = TPM_RC_BAD_VERSION;
        }
    }

    return rc;
}

static TPM_RC
NV_HEADER_Unmarshal(NV_HEADER *data, BYTE **buffer, INT32 *size,
                    UINT16 cur_version, UINT32 exp_magic)
{
    return NV_HEADER_UnmarshalVerbose(data, buffer, size, cur_version,
                                      exp_magic, true);
}

#define NV_INDEX_MAGIC 0x2547265a
#define NV_INDEX_VERSION 2
static UINT16
NV_INDEX_Marshal(NV_INDEX *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                NV_INDEX_VERSION,
                                NV_INDEX_MAGIC, 1);

    written += TPMS_NV_PUBLIC_Marshal(&data->publicArea, buffer, size);
    written += TPM2B_AUTH_Marshal(&data->authValue, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
NV_INDEX_Unmarshal(NV_INDEX *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 NV_INDEX_VERSION, NV_INDEX_MAGIC);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = TPMS_NV_PUBLIC_Unmarshal(&data->publicArea, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_AUTH_Unmarshal(&data->authValue, buffer, size);
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "NV_INDEX", "version 3 or later");
        /* future versions nest-append here */
    }
skip_future_versions:
    return rc;
}

#define DRBG_STATE_MAGIC 0x6fe83ea1
#define DRBG_STATE_VERSION 2
static UINT16
DRBG_STATE_Marshal(DRBG_STATE *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    size_t i;
    UINT16 array_size;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                DRBG_STATE_VERSION, DRBG_STATE_MAGIC, 1);
    written += UINT64_Marshal(&data->reseedCounter, buffer, size);
    written += UINT32_Marshal(&data->magic, buffer, size);

    array_size = sizeof(data->seed.bytes);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal(&data->seed.bytes[0], array_size, buffer, size);

    array_size = ARRAY_SIZE(data->lastValue);
    written += UINT16_Marshal(&array_size, buffer, size);
    for (i = 0; i < array_size; i++) {
        written += UINT32_Marshal(&data->lastValue[i], buffer, size);
    }

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
DRBG_STATE_Unmarshal(DRBG_STATE *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc= TPM_RC_SUCCESS;
    size_t i;
    NV_HEADER hdr;
    UINT16 array_size = 0;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 DRBG_STATE_VERSION, DRBG_STATE_MAGIC);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&data->reseedCounter, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->magic, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        if (array_size != ARRAY_SIZE(data->seed.bytes)) {
            TPMLIB_LogTPM2Error("Non-matching DRBG_STATE seed array size. "
                                "Expected %zu, got %u\n",
                                ARRAY_SIZE(data->seed.bytes), array_size);
            rc = TPM_RC_SIZE;
        }
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = Array_Unmarshal(&data->seed.bytes[0], array_size, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        if (array_size != ARRAY_SIZE(data->lastValue)) {
            TPMLIB_LogTPM2Error("Non-matching DRBG_STATE lastValue array size. "
                                "Expected %zu, got %u\n",
                                ARRAY_SIZE(data->lastValue), array_size);
            rc = TPM_RC_SIZE;
        }
    }
    if (rc == TPM_RC_SUCCESS) {
        for (i = 0; i < ARRAY_SIZE(data->lastValue) && rc == TPM_RC_SUCCESS; i++) {
            rc = UINT32_Unmarshal(&data->lastValue[i], buffer, size);
        }
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "DRBG_STATE", "version 3 or later");
        /* future versions nest-append here */
    }
skip_future_versions:

    return rc;
}

#define PCR_POLICY_MAGIC 0x176be626
#define PCR_POLICY_VERSION 2
static UINT16
PCR_POLICY_Marshal(PCR_POLICY *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    unsigned i;
    UINT16 array_size = ARRAY_SIZE(data->hashAlg);
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                PCR_POLICY_VERSION,
                                PCR_POLICY_MAGIC, 1);

    written += UINT16_Marshal(&array_size, buffer, size);

    for (i = 0; i < array_size; i++) {
        /* TPMI_ALG_HASH_Unmarshal errors on algid 0 */
        written += TPM_ALG_ID_Marshal(&data->hashAlg[i], buffer, size);
        written += TPM2B_DIGEST_Marshal(&data->policy[i], buffer, size);
    }

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
PCR_POLICY_Unmarshal(PCR_POLICY *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc= TPM_RC_SUCCESS;
    unsigned i;
    UINT16 array_size;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 PCR_POLICY_VERSION, PCR_POLICY_MAGIC);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS) {
        if (array_size != ARRAY_SIZE(data->hashAlg)) {
            TPMLIB_LogTPM2Error("Non-matching PCR_POLICY array size. "
                                "Expected %zu, got %u\n",
                                ARRAY_SIZE(data->hashAlg), array_size);
            rc = TPM_RC_SIZE;
        }
    }

    for (i = 0;
         rc == TPM_RC_SUCCESS &&
         i < ARRAY_SIZE(data->hashAlg);
         i++) {
        /* TPMI_ALG_HASH_Unmarshal errors on algid 0 */
        if (rc == TPM_RC_SUCCESS) {
            rc = TPM_ALG_ID_Unmarshal(&data->hashAlg[i], buffer, size);
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = TPM2B_DIGEST_Unmarshal(&data->policy[i], buffer, size);
        }
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "PCR_POLICY", "version 3 or later");
        /* future versions nest-append here */
    }
skip_future_versions:
    return rc;
}

#define ORDERLY_DATA_MAGIC      0x56657887
#define ORDERLY_DATA_VERSION 2

static UINT16
ORDERLY_DATA_Marshal(ORDERLY_DATA *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    BOOL has_block;
    BLOCK_SKIP_INIT;

    written =  NV_HEADER_Marshal(buffer, size,
                                 ORDERLY_DATA_VERSION, ORDERLY_DATA_MAGIC, 1);
    written += UINT64_Marshal(&data->clock, buffer, size);
    written += UINT8_Marshal(&data->clockSafe, buffer, size);

    written += DRBG_STATE_Marshal(&data->drbgState, buffer, size);

#if ACCUMULATE_SELF_HEAL_TIMER
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#if ACCUMULATE_SELF_HEAL_TIMER
    written += UINT64_Marshal(&data->selfHealTimer, buffer, size);
    written += UINT64_Marshal(&data->lockoutTimer, buffer, size);
    written += UINT64_Marshal(&data->time, buffer, size);
#endif // ACCUMULATE_SELF_HEAL_TIMER

    BLOCK_SKIP_WRITE_POP(size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
ORDERLY_DATA_Unmarshal(ORDERLY_DATA *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    BOOL needs_block;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 ORDERLY_DATA_VERSION, ORDERLY_DATA_MAGIC);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&data->clock, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT8_Unmarshal(&data->clockSafe, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = DRBG_STATE_Unmarshal(&data->drbgState, buffer, size);
    }

#if ACCUMULATE_SELF_HEAL_TIMER
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_self_heal_timer, needs_block, buffer, size,
                        "ORDERLY_DATA", "selfHealTimer");
    }
#if ACCUMULATE_SELF_HEAL_TIMER
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&data->selfHealTimer, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&data->lockoutTimer, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&data->time, buffer, size);
    }
#endif // ACCUMULATE_SELF_HEAL_TIMER
skip_self_heal_timer:

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "ORDERLY_DATA", "version 3 or later");
        /* future versions nest-append here */
    }
skip_future_versions:

    return rc;
}

#define PCR_SAVE_MAGIC 0x7372eabc
#define PCR_SAVE_VERSION 2
static UINT16
PCR_SAVE_Marshal(PCR_SAVE *data, BYTE **buffer, INT32 *size)
{
    UINT16 written = 0;
    TPM_ALG_ID algid;
    UINT16 array_size;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                PCR_SAVE_VERSION, PCR_SAVE_MAGIC, 1);

    array_size = NUM_STATIC_PCR;
    written += UINT16_Marshal(&array_size, buffer, size);

#if ALG_SHA1
    algid = TPM_ALG_SHA1;
    written += TPM_ALG_ID_Marshal(&algid, buffer, size);

    array_size = sizeof(data->Sha1);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->Sha1, array_size,
                            buffer, size);
#endif
#if ALG_SHA256
    algid = TPM_ALG_SHA256;
    written += TPM_ALG_ID_Marshal(&algid, buffer, size);

    array_size = sizeof(data->Sha256);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->Sha256, array_size,
                              buffer, size);
#endif
#if ALG_SHA384
    algid = TPM_ALG_SHA384;
    written += TPM_ALG_ID_Marshal(&algid, buffer, size);

    array_size = sizeof(data->Sha384);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->Sha384, array_size,
                             buffer, size);
#endif
#if ALG_SHA512
    algid = TPM_ALG_SHA512;
    written += TPM_ALG_ID_Marshal(&algid, buffer, size);

    array_size = sizeof(data->Sha512);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->Sha512, array_size,
                             buffer, size);
#endif
#if ALG_SM3_256
    algid = TPM_ALG_SM3_256;
    written += TPM_ALG_ID_Marshal(&algid, buffer, size);

    array_size = sizeof(data->Sm3_256);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->Sm3_256, array_size,
                             buffer, size);
#endif
#if ALG_SHA3_256 || ALG_SHA3_384 || ALG_SHA3_512 || ALG_SM3_256
#error SHA3 and SM3 are not supported
#endif

    /* end marker */
    algid = TPM_ALG_NULL;
    written += TPM_ALG_ID_Marshal(&algid, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

/*
 * Get the PCR banks that are active so that we know what PCRs need to be
 * restored. Only data for active PCR banks needs to restored, inactive PCR
 * banks need no data restored.
 */
static UINT64
pcrbanks_algs_active(const TPML_PCR_SELECTION *pcrAllocated)
{
    UINT64 algs_active = 0;
    unsigned i, j;

    for(i = 0; i < pcrAllocated->count; i++) {
        for (j = 0; j < pcrAllocated->pcrSelections[i].sizeofSelect; j++) {
            if (pcrAllocated->pcrSelections[i].pcrSelect[j]) {
#ifndef VBOX
                algs_active |= 1 << pcrAllocated->pcrSelections[i].hash;
#else
                algs_active |= RT_BIT_64(pcrAllocated->pcrSelections[i].hash);
#endif
                break;
            }
        }
    }

    return algs_active;
}

static TPM_RC
PCR_SAVE_Unmarshal(PCR_SAVE *data, BYTE **buffer, INT32 *size,
                   const TPML_PCR_SELECTION *pcrAllocated)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    UINT16 array_size, needed_size = 0;
    NV_HEADER hdr;
    TPM_ALG_ID algid;
    BOOL end = FALSE;
    BYTE *t = NULL;
    UINT64 algs_needed = pcrbanks_algs_active(pcrAllocated);

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 PCR_SAVE_VERSION, PCR_SAVE_MAGIC);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != NUM_STATIC_PCR) {
        TPMLIB_LogTPM2Error("Non-matching PCR_SAVE NUM_STATIC_PCR. "
                            "Expected %zu, got %u\n",
                            sizeof(NUM_STATIC_PCR), array_size);
        rc = TPM_RC_SIZE;
    }

    while (rc == TPM_RC_SUCCESS && !end) {
        if (rc == TPM_RC_SUCCESS) {
            rc = TPM_ALG_ID_Unmarshal(&algid, buffer, size);
        }
        if (rc == TPM_RC_SUCCESS) {
            switch (algid) {
#if ALG_SHA1
            case TPM_ALG_SHA1:
                needed_size = sizeof(data->Sha1);
                t = (BYTE *)&data->Sha1;
            break;
#endif
#if ALG_SHA256
            case TPM_ALG_SHA256:
                needed_size = sizeof(data->Sha256);
                t = (BYTE *)&data->Sha256;
            break;
#endif
#if ALG_SHA384
            case TPM_ALG_SHA384:
                needed_size = sizeof(data->Sha384);
                t = (BYTE *)&data->Sha384;
            break;
#endif
#if ALG_SHA512
            case TPM_ALG_SHA512:
                needed_size = sizeof(data->Sha512);
                t = (BYTE *)&data->Sha512;
            break;
#endif
#if ALG_SM3_256
            case TPM_ALG_SM3_256:
                needed_size = sizeof(data->Sm3_256);
                t = (BYTE *)&data->Sm3_256;
            break;
#endif
#if ALG_SHA3_256 || ALG_SHA3_384 || ALG_SHA3_512 || ALG_SM3_256
#error SHA3 and SM3 are not supported
#endif
            case TPM_ALG_NULL:
                /* end marker */
                end = TRUE;
                t = NULL;
            break;
            default:
                TPMLIB_LogTPM2Error("PCR_SAVE: Unsupported algid %d.",
                                    algid);
                rc = TPM_RC_BAD_PARAMETER;
                t = NULL;
            }
        }
        if (t) {
            if (rc == TPM_RC_SUCCESS) {
                algs_needed &= ~(1 << algid);
                rc = UINT16_Unmarshal(&array_size, buffer, size);
            }
            if (rc == TPM_RC_SUCCESS && array_size != needed_size) {
                TPMLIB_LogTPM2Error("PCR_SAVE: Bad size for PCRs for hash 0x%x; "
                                    "Expected %u, got %d\n",
                                    algid, needed_size, array_size);
                rc = TPM_RC_BAD_PARAMETER;
            }
            if (rc == TPM_RC_SUCCESS) {
                rc = Array_Unmarshal(t, array_size, buffer, size);
            }
        }
    }

    if (rc == TPM_RC_SUCCESS && algs_needed) {
        TPMLIB_LogTPM2Error("PCR_SAVE: Missing data for hash algorithm %d.\n",
                            _ffsll(algs_needed) - 1);
        rc = TPM_RC_BAD_PARAMETER;
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "PCR_SAVE", "version 3 or later");
        /* future versions nest-append here */
    }
skip_future_versions:

    return rc;
}


#ifdef PCR_C

#define PCR_MAGIC 0xe95f0387
#define PCR_VERSION 2
static UINT16
PCR_Marshal(PCR *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    TPM_ALG_ID algid;
    UINT16 array_size;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                PCR_VERSION, PCR_MAGIC, 1);

#if ALG_SHA1
    algid = TPM_ALG_SHA1;
    written += TPM_ALG_ID_Marshal(&algid, buffer, size);

    array_size = sizeof(data->Sha1Pcr);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->Sha1Pcr, array_size,
                            buffer, size);
#endif
#if ALG_SHA256
    algid = TPM_ALG_SHA256;
    written += TPM_ALG_ID_Marshal(&algid, buffer, size);

    array_size = sizeof(data->Sha256Pcr);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->Sha256Pcr, array_size,
                              buffer, size);
#endif
#if ALG_SHA384
    algid = TPM_ALG_SHA384;
    written += TPM_ALG_ID_Marshal(&algid, buffer, size);

    array_size = sizeof(data->Sha384Pcr);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->Sha384Pcr, array_size,
                             buffer, size);
#endif
#if ALG_SHA512
    algid = TPM_ALG_SHA512;
    written += TPM_ALG_ID_Marshal(&algid, buffer, size);

    array_size = sizeof(data->Sha512Pcr);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->Sha512Pcr, array_size,
                             buffer, size);
#endif
#if ALG_SM3_256
    algid = TPM_ALG_SM3_256;
    written += TPM_ALG_ID_Marshal(&algid, buffer, size);

    array_size = sizeof(data->Sm3_256Pcr);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->Sm3_256Pcr, array_size,
                             buffer, size);
#endif
#if ALG_SHA3_256 || ALG_SHA3_384 || ALG_SHA3_512 || ALG_SM3_256
#error SHA3 and SM3 are not supported
#endif

    /* end marker */
    algid = TPM_ALG_NULL;
    written += TPM_ALG_ID_Marshal(&algid, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
PCR_Unmarshal(PCR *data, BYTE **buffer, INT32 *size,
              const TPML_PCR_SELECTION *pcrAllocated)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;
    BOOL end = FALSE;
    BYTE *t = NULL;
    UINT16 needed_size = 0, array_size;
    TPM_ALG_ID algid;
    UINT64 algs_needed = pcrbanks_algs_active(pcrAllocated);

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 PCR_VERSION, PCR_MAGIC);
    }

    while (rc == TPM_RC_SUCCESS && !end) {
        if (rc == TPM_RC_SUCCESS) {
            rc = TPM_ALG_ID_Unmarshal(&algid, buffer, size);
        }
        if (rc == TPM_RC_SUCCESS) {
            switch (algid) {
#if ALG_SHA1
            case TPM_ALG_SHA1:
                needed_size = sizeof(data->Sha1Pcr);
                t = (BYTE *)&data->Sha1Pcr;
            break;
#endif
#if ALG_SHA256
            case TPM_ALG_SHA256:
                needed_size = sizeof(data->Sha256Pcr);
                t = (BYTE *)&data->Sha256Pcr;
            break;
#endif
#if ALG_SHA384
            case TPM_ALG_SHA384:
                needed_size = sizeof(data->Sha384Pcr);
                t = (BYTE *)&data->Sha384Pcr;
            break;
#endif
#if ALG_SHA512
            case TPM_ALG_SHA512:
                needed_size = sizeof(data->Sha512Pcr);
                t = (BYTE *)&data->Sha512Pcr;
            break;
#endif
#if ALG_SM3_256
            case TPM_ALG_SM3_256:
                needed_size = sizeof(data->Sm3_256Pcr);
                t = (BYTE *)&data->Sm3_256Pcr;
            break;
#endif
#if ALG_SHA3_256 || ALG_SHA3_384 || ALG_SHA3_512 || ALG_SM3_256
#error SHA3 and SM3 are not supported
#endif
            case TPM_ALG_NULL:
                /* end marker */
                end = TRUE;
                t = NULL;
            break;
            default:
                TPMLIB_LogTPM2Error("PCR: Unsupported algid %d.",
                                    algid);
                rc = TPM_RC_BAD_PARAMETER;
                t = NULL;
            }
        }
        if (t) {
            if (rc == TPM_RC_SUCCESS) {
                algs_needed &= ~(1 << algid);
                rc = UINT16_Unmarshal(&array_size, buffer, size);
            }
            if (rc == TPM_RC_SUCCESS && array_size != needed_size) {
                TPMLIB_LogTPM2Error("PCR: Bad size for PCR for hash 0x%x; "
                                    "Expected %u, got %d\n",
                                    algid, needed_size, array_size);
                rc = TPM_RC_BAD_PARAMETER;
            }
            if (rc == TPM_RC_SUCCESS) {
                rc = Array_Unmarshal(t, array_size, buffer, size);
            }
        }
    }

    if (rc == TPM_RC_SUCCESS && algs_needed) {
        TPMLIB_LogTPM2Error("PCR: Missing data for hash algorithm %d.\n",
                            _ffsll(algs_needed) - 1);
        rc = TPM_RC_BAD_PARAMETER;
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "PCR", "version 3 or later");
        /* future versions nest-append here */
    }
skip_future_versions:

    return rc;
}
#endif

#define PCR_AUTHVALUE_MAGIC 0x6be82eaf
#define PCR_AUTHVALUE_VERSION 2
static UINT16
PCR_AUTHVALUE_Marshal(PCR_AUTHVALUE *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    size_t i;
    UINT16 array_size;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                PCR_AUTHVALUE_VERSION, PCR_AUTHVALUE_MAGIC, 1);

    array_size = ARRAY_SIZE(data->auth);
    written += UINT16_Marshal(&array_size, buffer, size);
    for (i = 0; i < array_size; i++) {
        written += TPM2B_DIGEST_Marshal(&data->auth[i], buffer, size);
    }

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
PCR_AUTHVALUE_Unmarshal(PCR_AUTHVALUE *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    size_t i;
    NV_HEADER hdr;
    UINT16 array_size = 0;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 PCR_AUTHVALUE_VERSION, PCR_AUTHVALUE_MAGIC);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != ARRAY_SIZE(data->auth)) {
        TPMLIB_LogTPM2Error("PCR_AUTHVALUE: Bad array size for auth; "
                            "expected %zu, got %u\n",
                            ARRAY_SIZE(data->auth), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    if (rc == TPM_RC_SUCCESS) {
        for (i = 0; i < ARRAY_SIZE(data->auth) && rc == TPM_RC_SUCCESS; i++) {
            rc = TPM2B_DIGEST_Unmarshal(&data->auth[i], buffer, size);
        }
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "PCR_AUTHVALUE", "version 3 or later");
        /* future versions nest-append here */
    }
skip_future_versions:

    return rc;
}

#define STATE_CLEAR_DATA_MAGIC  0x98897667
#define STATE_CLEAR_DATA_VERSION 2

static UINT16
STATE_CLEAR_DATA_Marshal(STATE_CLEAR_DATA *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                STATE_CLEAR_DATA_VERSION,
                                STATE_CLEAR_DATA_MAGIC, 1);
    written += BOOL_Marshal(&data->shEnable, buffer, size);
    written += BOOL_Marshal(&data->ehEnable, buffer, size);
    written += BOOL_Marshal(&data->phEnableNV, buffer, size);
    written += UINT16_Marshal(&data->platformAlg, buffer, size);
    written += TPM2B_DIGEST_Marshal(&data->platformPolicy, buffer, size);
    written += TPM2B_AUTH_Marshal(&data->platformAuth, buffer, size);
    written += PCR_SAVE_Marshal(&data->pcrSave, buffer, size);
    written += PCR_AUTHVALUE_Marshal(&data->pcrAuthValues, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
STATE_CLEAR_DATA_Unmarshal(STATE_CLEAR_DATA *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 STATE_CLEAR_DATA_VERSION,
                                 STATE_CLEAR_DATA_MAGIC);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&data->shEnable, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&data->ehEnable, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&data->phEnableNV, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&data->platformAlg, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_DIGEST_Unmarshal(&data->platformPolicy, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_AUTH_Unmarshal(&data->platformAuth, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = PCR_SAVE_Unmarshal(&data->pcrSave, buffer, size, &shadow.pcrAllocated);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = PCR_AUTHVALUE_Unmarshal(&data->pcrAuthValues, buffer, size);
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "STATE_CLEAR_DATA", "version 3 or later");
        /* future versions nest-append here */
    }
skip_future_versions:

    return rc;
}

#define STATE_RESET_DATA_MAGIC  0x01102332
#define STATE_RESET_DATA_VERSION 4

static TPM_RC
STATE_RESET_DATA_Unmarshal(STATE_RESET_DATA *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    BOOL needs_block;
    UINT16 array_size;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 STATE_RESET_DATA_VERSION,
                                 STATE_RESET_DATA_MAGIC);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_PROOF_Unmarshal(&data->nullProof, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_Unmarshal(&data->nullSeed.b, PRIMARY_SEED_SIZE, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->clearCount, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&data->objectContextID, buffer, size);
    }


    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != ARRAY_SIZE(data->contextArray)) {
        TPMLIB_LogTPM2Error("STATE_RESET_DATA: Bad array size for contextArray; "
                            "expected %zu, got %u\n",
                            ARRAY_SIZE(data->contextArray), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    if (rc == TPM_RC_SUCCESS) {
        size_t i;
        if (hdr.version <= 3) {
            /* version <= 3 was writing an array of UINT8 */
            UINT8 element;
            for (i = 0; i < array_size && rc == TPM_RC_SUCCESS; i++) {
                rc = UINT8_Unmarshal(&element, buffer, size);
                data->contextArray[i] = element;
            }
            s_ContextSlotMask = 0xff;
        } else {
            /* version 4 and later an array of UINT16 */
            for (i = 0; i < array_size && rc == TPM_RC_SUCCESS; i++) {
                rc = UINT16_Unmarshal(&data->contextArray[i], buffer, size);
            }
            if (rc == TPM_RC_SUCCESS) {
                rc = UINT16_Unmarshal(&s_ContextSlotMask, buffer, size);
            }
            if (rc == TPM_RC_SUCCESS) {
                if (s_ContextSlotMask != 0xffff && s_ContextSlotMask != 0x00ff) {
                    TPMLIB_LogTPM2Error("STATE_RESET_DATA: s_ContextSlotMask has bad value: 0x%04x\n",
                                        s_ContextSlotMask);
                    rc = TPM_RC_BAD_PARAMETER;
                }
            }
        }
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&data->contextCounter, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_DIGEST_Unmarshal(&data->commandAuditDigest,
                              buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->restartCount, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->pcrCounter, buffer, size);
    }

#if ALG_ECC
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_alg_ecc, needs_block, buffer, size,
                        "STATE_RESET_DATA", "commitCounter");
    }
#if ALG_ECC
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&data->commitCounter, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_AUTH_Unmarshal(&data->commitNonce, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != sizeof(data->commitArray)) {
        TPMLIB_LogTPM2Error("STATE_RESET_DATA: Bad array size for commitArray; "
                            "expected %zu, got %u\n",
                            sizeof(data->commitArray), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = Array_Unmarshal((BYTE *)&data->commitArray, array_size,
                              buffer, size);
    }
#endif
skip_alg_ecc:

    /* default values before conditional block */
    data->nullSeedCompatLevel = SEED_COMPAT_LEVEL_ORIGINAL;

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, hdr.version >= 3, buffer, size,
                        "STATE_RESET_DATA", "version 3 or later");
        if (rc == TPM_RC_SUCCESS) {
            rc = SEED_COMPAT_LEVEL_Unmarshal(&gr.nullSeedCompatLevel,
                                             buffer, size, "nullSeed");
        }

        if (rc == TPM_RC_SUCCESS) {
            BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                            "STATE_RESET_DATA", "version 4 or later");
        }
        /* future versions nest-append here */
    }

skip_future_versions:

    return rc;
}

static UINT16
STATE_RESET_DATA_Marshal(STATE_RESET_DATA *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    BOOL has_block;
    UINT16 array_size;
    BLOCK_SKIP_INIT;
    size_t i;

    written = NV_HEADER_Marshal(buffer, size,
                                STATE_RESET_DATA_VERSION,
                                STATE_RESET_DATA_MAGIC, 4);
    written += TPM2B_PROOF_Marshal(&data->nullProof, buffer, size);
    written += TPM2B_Marshal(&data->nullSeed.b, sizeof(data->nullSeed.t.buffer), buffer, size);
    written += UINT32_Marshal(&data->clearCount, buffer, size);
    written += UINT64_Marshal(&data->objectContextID, buffer, size);

    array_size = ARRAY_SIZE(data->contextArray);
    written += UINT16_Marshal(&array_size, buffer, size);
    for (i = 0; i < array_size; i++)
        written += UINT16_Marshal(&data->contextArray[i], buffer, size);

    if (s_ContextSlotMask != 0x00ff && s_ContextSlotMask != 0xffff) {
        /* TPM wasn't initialized, so s_ContextSlotMask wasn't set */
        s_ContextSlotMask = 0xffff;
    }
    written += UINT16_Marshal(&s_ContextSlotMask, buffer, size);

    written += UINT64_Marshal(&data->contextCounter, buffer, size);
    written += TPM2B_DIGEST_Marshal(&data->commandAuditDigest,
                              buffer, size);
    written += UINT32_Marshal(&data->restartCount, buffer, size);
    written += UINT32_Marshal(&data->pcrCounter, buffer, size);
#if ALG_ECC
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#if ALG_ECC
    written += UINT64_Marshal(&data->commitCounter, buffer, size);
    written += TPM2B_AUTH_Marshal(&data->commitNonce, buffer, size);

    array_size = sizeof(data->commitArray);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->commitArray, array_size,
                             buffer, size);
#endif
    BLOCK_SKIP_WRITE_POP(size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    written += SEED_COMPAT_LEVEL_Marshal(&data->nullSeedCompatLevel,
                                         buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);
    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

#define BN_PRIME_T_MAGIC 0x2fe736ab
#define BN_PRIME_T_VERSION 2
static UINT16
bn_prime_t_Marshal(bn_prime_t *data, BYTE **buffer, INT32 *size)
{
    UINT16 written, numbytes;
    size_t i, idx;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                BN_PRIME_T_VERSION, BN_PRIME_T_MAGIC, 1);

    /* we do not write 'allocated' */
    numbytes = data->size * sizeof(crypt_uword_t);
    written += UINT16_Marshal(&numbytes, buffer, size);

    for (i = 0, idx = 0;
         i < numbytes;
         i += sizeof(crypt_uword_t), idx += 1) {
#if RADIX_BITS == 64
        written += UINT64_Marshal(&data->d[idx], buffer, size);
#elif RADIX_BITS == 32
        written += UINT32_Marshal(&data->d[idx], buffer, size);
#else
#error RADIX_BYTES it no defined
#endif
    }

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
bn_prime_t_Unmarshal(bn_prime_t *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    size_t i, idx;
    UINT16 numbytes = 0;
    UINT32 word;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 BN_PRIME_T_VERSION,
                                 BN_PRIME_T_MAGIC);
    }

    data->allocated = ARRAY_SIZE(data->d);

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&numbytes, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        /* coverity: num_bytes is sanitized here! */
        data->size = (numbytes + sizeof(crypt_uword_t) - 1) / sizeof(crypt_word_t);
        if (data->size > data->allocated) {
            TPMLIB_LogTPM2Error("bn_prime_t: Require size larger %zu than "
                                "allocated %zu\n",
                                (size_t)data->size, (size_t)data->allocated);
            rc = TPM_RC_SIZE;
            data->size = 0;
        }
    }

    if (rc == TPM_RC_SUCCESS) {
        for (i = 0, idx = 0;
             i < numbytes && rc == TPM_RC_SUCCESS;
             i += sizeof(UINT32), idx += 1) {
            rc = UINT32_Unmarshal(&word, buffer, size);
#if RADIX_BITS == 64
            data->d[idx / 2] <<= 32;
            data->d[idx / 2] |= word;
#elif RADIX_BITS == 32
            data->d[idx] = word;
#endif
        }
    }

#if RADIX_BITS == 64
    if (rc == TPM_RC_SUCCESS) {
        if (idx & 1)
            data->d[idx / 2] <<= 32;
    }
#endif

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "BN_PRIME_T", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:

    return rc;
}

#define PRIVATE_EXPONENT_T_MAGIC 0x854eab2
#define PRIVATE_EXPONENT_T_VERSION 2
static UINT16
privateExponent_t_Marshal(privateExponent_t *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                PRIVATE_EXPONENT_T_VERSION,
                                PRIVATE_EXPONENT_T_MAGIC, 1);
#if CRT_FORMAT_RSA == NO
#error Missing code
#else
    written += bn_prime_t_Marshal(&data->Q, buffer, size);
    written += bn_prime_t_Marshal(&data->dP, buffer, size);
    written += bn_prime_t_Marshal(&data->dQ, buffer, size);
    written += bn_prime_t_Marshal(&data->qInv, buffer, size);
#endif

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
privateExponent_t_Unmarshal(privateExponent_t *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 PRIVATE_EXPONENT_T_VERSION,
                                 PRIVATE_EXPONENT_T_MAGIC);
    }

#if CRT_FORMAT_RSA == NO
#error Missing code
#else
    if (rc == TPM_RC_SUCCESS) {
        rc = bn_prime_t_Unmarshal(&data->Q, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = bn_prime_t_Unmarshal(&data->dP, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = bn_prime_t_Unmarshal(&data->dQ, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = bn_prime_t_Unmarshal(&data->qInv, buffer, size);
    }
#endif

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "PRIVATE_EXPONENT_T", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:

    return rc;
}

static UINT16
HASH_STATE_TYPE_Marshal(HASH_STATE_TYPE *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;

    written = UINT8_Marshal(data, buffer, size);

    return written;
}

static UINT16
HASH_STATE_TYPE_Unmarshal(HASH_STATE_TYPE *data, BYTE **buffer, INT32 *size)
{
    return UINT8_Unmarshal(data, buffer, size);
}

static inline UINT16
SHA_LONG_Marshal(SHA_LONG *data, BYTE **buffer, INT32 *size)
{
    return UINT32_Marshal(data, buffer, size);
}

static inline UINT16
SHA_LONG_Unmarshal(SHA_LONG *data, BYTE **buffer, INT32 *size)
{
    return UINT32_Unmarshal(data, buffer, size);
}

static inline UINT16
SHA_LONG64_Marshal(SHA_LONG64 *data, BYTE **buffer, INT32 *size)
{
    assert(sizeof(*data) == 8);
    return UINT64_Marshal((UINT64 *)data, buffer, size);
}

static inline UINT16
SHA_LONG64_Unmarshal(SHA_LONG64 *data, BYTE **buffer, INT32 *size)
{
    assert(sizeof(*data) == 8);
    return UINT64_Unmarshal((UINT64 *)data, buffer, size);
}

#if ALG_SHA1

#define HASH_STATE_SHA1_MAGIC   0x19d46f50
#define HASH_STATE_SHA1_VERSION 2

static UINT16
tpmHashStateSHA1_Marshal(tpmHashStateSHA1_t *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    UINT16 array_size;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                HASH_STATE_SHA1_VERSION,
                                HASH_STATE_SHA1_MAGIC,1);
    written += SHA_LONG_Marshal(&data->h0, buffer, size);
    written += SHA_LONG_Marshal(&data->h1, buffer, size);
    written += SHA_LONG_Marshal(&data->h2, buffer, size);
    written += SHA_LONG_Marshal(&data->h3, buffer, size);
    written += SHA_LONG_Marshal(&data->h4, buffer, size);
    written += SHA_LONG_Marshal(&data->Nl, buffer, size);
    written += SHA_LONG_Marshal(&data->Nh, buffer, size);

    /* data must be written as array */
    array_size = sizeof(data->data);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->data[0], array_size,
                             buffer, size);

    written += UINT32_Marshal(&data->num, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static UINT16
tpmHashStateSHA1_Unmarshal(tpmHashStateSHA1_t *data, BYTE **buffer, INT32 *size)
{
    UINT16 rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;
    UINT16 array_size;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 HASH_STATE_SHA1_VERSION,
                                 HASH_STATE_SHA1_MAGIC);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = SHA_LONG_Unmarshal(&data->h0, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = SHA_LONG_Unmarshal(&data->h1, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = SHA_LONG_Unmarshal(&data->h2, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = SHA_LONG_Unmarshal(&data->h3, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = SHA_LONG_Unmarshal(&data->h4, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = SHA_LONG_Unmarshal(&data->Nl, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = SHA_LONG_Unmarshal(&data->Nh, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != sizeof(data->data)) {
        TPMLIB_LogTPM2Error("HASH_STATE_SHA1: Bad array size for data; "
                            "expected %zu, got %u\n",
                            sizeof(data->data), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = Array_Unmarshal((BYTE *)&data->data[0], array_size,
                             buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->num, buffer, size);
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "HASH_STATE_SHA1", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:

    return rc;
}
#endif

#if ALG_SHA256
#define HASH_STATE_SHA256_MAGIC 0x6ea059d0
#define HASH_STATE_SHA256_VERSION 2

static UINT16
tpmHashStateSHA256_Marshal(tpmHashStateSHA256_t *data, BYTE **buffer, INT32 *size)
{
    UINT16 written = 0;
    UINT16 array_size;
    size_t i;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                HASH_STATE_SHA256_VERSION,
                                HASH_STATE_SHA256_MAGIC, 1);

    array_size = ARRAY_SIZE(data->h);
    written += UINT16_Marshal(&array_size, buffer, size);
    for (i = 0; i < array_size; i++) {
        written += SHA_LONG_Marshal(&data->h[i], buffer, size);
    }
    written += SHA_LONG_Marshal(&data->Nl, buffer, size);
    written += SHA_LONG_Marshal(&data->Nh, buffer, size);

    /* data must be written as array */
    array_size = sizeof(data->data);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal((BYTE *)&data->data[0], array_size,
                             buffer, size);

    written += UINT32_Marshal(&data->num, buffer, size);
    written += UINT32_Marshal(&data->md_len, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static UINT16
tpmHashStateSHA256_Unmarshal(tpmHashStateSHA256_t *data, BYTE **buffer, INT32 *size)
{
    UINT16 rc = TPM_RC_SUCCESS;
    size_t i;
    UINT16 array_size;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 HASH_STATE_SHA256_VERSION,
                                 HASH_STATE_SHA256_MAGIC);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != ARRAY_SIZE(data->h)) {
        TPMLIB_LogTPM2Error("HASH_STATE_SHA256: Bad array size for h; "
                            "expected %zu, got %u\n",
                            ARRAY_SIZE(data->h), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    for (i = 0; rc == TPM_RC_SUCCESS && i < array_size; i++) {
        rc = SHA_LONG_Unmarshal(&data->h[i], buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = SHA_LONG_Unmarshal(&data->Nl, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = SHA_LONG_Unmarshal(&data->Nh, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != sizeof(data->data)) {
        TPMLIB_LogTPM2Error("HASH_STATE_SHA256: Bad array size for data; "
                            "expected %zu, got %u\n",
                            sizeof(data->data), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = Array_Unmarshal((BYTE *)&data->data[0], array_size,
                             buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->num, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->md_len, buffer, size);
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "HASH_STATE_SHA256", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:

    return rc;
}
#endif

#if ALG_SHA384 || ALG_SHA512

#define HASH_STATE_SHA384_MAGIC 0x14814b08
#define HASH_STATE_SHA384_VERSION 2

#define HASH_STATE_SHA512_MAGIC 0x269e8ae0
#define HASH_STATE_SHA512_VERSION 2

static UINT16
tpmHashStateSHA512_Marshal(SHA512_CTX *data, BYTE **buffer, INT32 *size,
                           UINT16 hashAlg)
{
    UINT16 written = 0;
    UINT16 array_size;
    size_t i;
    BLOCK_SKIP_INIT;
    UINT16 version = HASH_STATE_SHA512_VERSION;
    UINT32 magic = HASH_STATE_SHA512_MAGIC;

    if (hashAlg == ALG_SHA384_VALUE) {
        version = HASH_STATE_SHA384_VERSION;
        magic = HASH_STATE_SHA384_MAGIC;
    }

    written = NV_HEADER_Marshal(buffer, size,
                                version, magic, 1);

    array_size = ARRAY_SIZE(data->h);
    written += UINT16_Marshal(&array_size, buffer, size);
    for (i = 0; i < array_size; i++) {
        written += SHA_LONG64_Marshal(&data->h[i], buffer, size);
    }
    written += SHA_LONG64_Marshal(&data->Nl, buffer, size);
    written += SHA_LONG64_Marshal(&data->Nh, buffer, size);

    array_size = sizeof(data->u.p);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal(&data->u.p[0], array_size, buffer, size);

    written += UINT32_Marshal(&data->num, buffer, size);
    written += UINT32_Marshal(&data->md_len, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static UINT16
tpmHashStateSHA512_Unmarshal(SHA512_CTX *data, BYTE **buffer, INT32 *size,
                             UINT16 hashAlg)
{
    UINT16 rc = TPM_RC_SUCCESS;
    size_t i;
    UINT16 array_size;
    NV_HEADER hdr;
    UINT16 version = HASH_STATE_SHA512_VERSION;
    UINT32 magic = HASH_STATE_SHA512_MAGIC;

    if (hashAlg == ALG_SHA384_VALUE) {
        version = HASH_STATE_SHA384_VERSION;
        magic = HASH_STATE_SHA384_MAGIC;
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 version, magic);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != ARRAY_SIZE(data->h)) {
        TPMLIB_LogTPM2Error("HASH_STATE_SHA512: Bad array size for h; "
                            "expected %zu, got %u\n",
                            ARRAY_SIZE(data->h), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    for (i = 0; rc == TPM_RC_SUCCESS && i < array_size; i++) {
        rc = SHA_LONG64_Unmarshal(&data->h[i], buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = SHA_LONG64_Unmarshal(&data->Nl, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = SHA_LONG64_Unmarshal(&data->Nh, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != sizeof(data->u.p)) {
        TPMLIB_LogTPM2Error("HASH_STATE_SHA512: Bad array size for u.p; "
                            "expected %zu, got %u\n",
                            sizeof(data->u.p), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = Array_Unmarshal(&data->u.p[0], array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->num, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->md_len, buffer, size);
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "HASH_STATE_SHA512", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:

    return rc;
}
#endif

#define ANY_HASH_STATE_MAGIC 0x349d494b
#define ANY_HASH_STATE_VERSION 2

static UINT16
ANY_HASH_STATE_Marshal(ANY_HASH_STATE *data, BYTE **buffer, INT32 *size,
                       UINT16 hashAlg)
{
    UINT16 written;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                ANY_HASH_STATE_VERSION,
                                ANY_HASH_STATE_MAGIC, 1);

    switch (hashAlg) {
#if ALG_SHA1
    case ALG_SHA1_VALUE:
        written += tpmHashStateSHA1_Marshal(&data->Sha1, buffer, size);
        break;
#endif
#if ALG_SHA256
    case ALG_SHA256_VALUE:
        written += tpmHashStateSHA256_Marshal(&data->Sha256, buffer, size);
        break;
#endif
#if ALG_SHA384
    case ALG_SHA384_VALUE:
        written += tpmHashStateSHA512_Marshal(&data->Sha384, buffer, size,
                                              ALG_SHA384_VALUE);
        break;
#endif
#if ALG_SHA512
    case ALG_SHA512_VALUE:
        written += tpmHashStateSHA512_Marshal(&data->Sha512, buffer, size,
                                              ALG_SHA512_VALUE);
        break;
#endif
    default:
        break;
    }

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static UINT16
ANY_HASH_STATE_Unmarshal(ANY_HASH_STATE *data, BYTE **buffer, INT32 *size,
                         UINT16 hashAlg)
{
    UINT16 rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 ANY_HASH_STATE_VERSION,
                                 ANY_HASH_STATE_MAGIC);
    }

    switch (hashAlg) {
#if ALG_SHA1
    case ALG_SHA1_VALUE:
        rc = tpmHashStateSHA1_Unmarshal(&data->Sha1, buffer, size);
        break;
#endif
#if ALG_SHA256
    case ALG_SHA256_VALUE:
        rc = tpmHashStateSHA256_Unmarshal(&data->Sha256, buffer, size);
        break;
#endif
#if ALG_SHA384
    case ALG_SHA384_VALUE:
        rc = tpmHashStateSHA512_Unmarshal(&data->Sha384, buffer, size,
                                          ALG_SHA384_VALUE);
        break;
#endif
#if ALG_SHA512
    case ALG_SHA512_VALUE:
        rc = tpmHashStateSHA512_Unmarshal(&data->Sha512, buffer, size,
                                          ALG_SHA512_VALUE);
        break;
#endif
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "ANY_HASH_STATE", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:

    return rc;
}

#define HASH_STATE_MAGIC 0x562878a2
#define HASH_STATE_VERSION 2

static UINT16
HASH_STATE_Marshal(HASH_STATE *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                HASH_STATE_VERSION,
                                HASH_STATE_MAGIC, 1);

    written += HASH_STATE_TYPE_Marshal(&data->type, buffer, size);
    written += TPM_ALG_ID_Marshal(&data->hashAlg, buffer, size);
    /* def does not need to be written */
    written += ANY_HASH_STATE_Marshal(&data->state, buffer, size, data->hashAlg);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static UINT16
HASH_STATE_Unmarshal(HASH_STATE *data, BYTE **buffer, INT32 *size)
{
    UINT16 rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 HASH_STATE_VERSION, HASH_STATE_MAGIC);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = HASH_STATE_TYPE_Unmarshal(&data->type, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc  = TPM_ALG_ID_Unmarshal(&data->hashAlg, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        data->def = CryptGetHashDef(data->hashAlg);
        if (!data->def) {
            TPMLIB_LogTPM2Error("Could not get hash function interface for "
                                "hashAlg 0x%02x\n", data->hashAlg);
            rc = TPM_RC_BAD_PARAMETER;
        }
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = ANY_HASH_STATE_Unmarshal(&data->state, buffer, size, data->hashAlg);
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "HASH_STATE", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:
    return rc;
}

static inline UINT16
TPM2B_HASH_BLOCK_Marshal(TPM2B_HASH_BLOCK *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;

    written = TPM2B_Marshal(&data->b, sizeof(data->t.buffer), buffer, size);

    return written;
}

static inline UINT16
TPM2B_HASH_BLOCK_Unmarshal(TPM2B_HASH_BLOCK *data, BYTE **buffer, INT32 *size)
{
    UINT16 rc;

    rc = TPM2B_Unmarshal(&data->b, sizeof(data->t.buffer), buffer, size);

    return rc;
}

static UINT16
HMAC_STATE_Marshal(HMAC_STATE *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;

    written = HASH_STATE_Marshal(&data->hashState, buffer, size);
    written += TPM2B_HASH_BLOCK_Marshal(&data->hmacKey, buffer, size);

    return written;
}

static UINT16
HMAC_STATE_Unmarshal(HMAC_STATE *data, BYTE **buffer, INT32 *size)
{
    UINT16 rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
        rc = HASH_STATE_Unmarshal(&data->hashState, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_HASH_BLOCK_Unmarshal(&data->hmacKey, buffer, size);
    }

    return rc;
}

#define HASH_OBJECT_MAGIC 0xb874fe38
#define HASH_OBJECT_VERSION 3

static UINT16
HASH_OBJECT_Marshal(HASH_OBJECT *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    size_t i;
    UINT16 array_size;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                HASH_OBJECT_VERSION, HASH_OBJECT_MAGIC, 1);
    written += TPMI_ALG_PUBLIC_Marshal(&data->type, buffer, size);
    written += TPMI_ALG_HASH_Marshal(&data->nameAlg, buffer, size);
    written += TPMA_OBJECT_Marshal(&data->objectAttributes, buffer, size);
    written += TPM2B_AUTH_Marshal(&data->auth, buffer, size);
    if (data->attributes.hashSeq == SET ||
        data->attributes.eventSeq == SET /* since v3 */) {
        array_size = ARRAY_SIZE(data->state.hashState);
        written += UINT16_Marshal(&array_size, buffer, size);
        for (i = 0; i < array_size; i++) {
            written += HASH_STATE_Marshal(&data->state.hashState[i], buffer,
                                          size);
        }
    } else if (data->attributes.hmacSeq == SET) {
        written += HMAC_STATE_Marshal(&data->state.hmacState, buffer, size);
    }

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static UINT16
HASH_OBJECT_Unmarshal(HASH_OBJECT *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    size_t i;
    UINT16 array_size;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 HASH_OBJECT_VERSION, HASH_OBJECT_MAGIC);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPMI_ALG_PUBLIC_Unmarshal(&data->type, buffer, size);
        if (rc == TPM_RC_TYPE)
            rc = TPM_RC_SUCCESS;
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPMI_ALG_HASH_Unmarshal(&data->nameAlg, buffer, size, TRUE);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPMA_OBJECT_Unmarshal(&data->objectAttributes, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_AUTH_Unmarshal(&data->auth, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        /* hashSeq was always written correctly; eventSeq only appeared in v3 */
        if (data->attributes.hashSeq == SET ||
            (data->attributes.eventSeq == SET && hdr.version >= 3)) {
            if (rc == TPM_RC_SUCCESS) {
                rc = UINT16_Unmarshal(&array_size, buffer, size);
            }
            if (rc == TPM_RC_SUCCESS) {
                if (array_size != ARRAY_SIZE(data->state.hashState)) {
                    TPMLIB_LogTPM2Error("HASH_OBJECT: Bad array size for state.hashState; "
                                        "expected %zu, got %u\n",
                                        ARRAY_SIZE(data->state.hashState),
                                        array_size);
                    rc = TPM_RC_SIZE;
                }
            }
            for (i = 0; rc == TPM_RC_SUCCESS && i < array_size; i++) {
                rc = HASH_STATE_Unmarshal(&data->state.hashState[i],
                                          buffer, size);
            }
        } else if (data->attributes.hmacSeq == SET) {
            rc = HMAC_STATE_Unmarshal(&data->state.hmacState, buffer, size);
        }
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "HASH_OBJECT", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:

    return rc;
}

/* Local version of TPMT_SENSITIVE_Marshal handling public keys that don't have much in TPM_SENSITIVE */
static UINT16
NV_TPMT_SENSITIVE_Marshal(TPMT_SENSITIVE *source, BYTE **buffer, INT32 *size)
{
    UINT16 written = 0;

    written += TPM_ALG_ID_Marshal(&source->sensitiveType, buffer, size);
    written += TPM2B_AUTH_Marshal(&source->authValue, buffer, size);
    written += TPM2B_DIGEST_Marshal(&source->seedValue, buffer, size);

    switch (source->sensitiveType) {
    case TPM_ALG_RSA:
    case TPM_ALG_ECC:
    case TPM_ALG_KEYEDHASH:
    case TPM_ALG_SYMCIPHER:
        written += TPMU_SENSITIVE_COMPOSITE_Marshal(&source->sensitive, buffer, size, source->sensitiveType);
        break;
    default:
        /* we wrote these but they must have been 0 in this case */
        pAssert(source->authValue.t.size == 0);
        pAssert(source->seedValue.t.size == 0);
        pAssert(source->sensitiveType == TPM_ALG_ERROR)
        /* public keys */
    }
    return written;
}

/* local version of TPM_SENSITIVE_Unmarshal handling public keys that don't have much in TPMT_SENSITVE */
static TPM_RC
NV_TPMT_SENSITIVE_Unmarshal(TPMT_SENSITIVE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
        /* TPMI_ALG_PUBLIC_Unmarshal would test the sensitiveType; we don't want this */
	rc = TPM_ALG_ID_Unmarshal(&target->sensitiveType, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_AUTH_Unmarshal(&target->authValue, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->seedValue, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        switch (target->sensitiveType) {
        case TPM_ALG_RSA:
        case TPM_ALG_ECC:
        case TPM_ALG_KEYEDHASH:
        case TPM_ALG_SYMCIPHER:
	    rc = TPMU_SENSITIVE_COMPOSITE_Unmarshal(&target->sensitive, buffer, size, target->sensitiveType);
	    break;
	default:
            pAssert(target->authValue.t.size == 0);
            pAssert(target->seedValue.t.size == 0);
            pAssert(target->sensitiveType == TPM_ALG_ERROR)
	    /* nothing do to do */
	}
    }
    return rc;
}

#define OBJECT_MAGIC 0x75be73af
#define OBJECT_VERSION 3

static UINT16
OBJECT_Marshal(OBJECT *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    BOOL has_block;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                OBJECT_VERSION, OBJECT_MAGIC, 3);

    /*
     * attributes are written in ANY_OBJECT_Marshal
     */
    written += TPMT_PUBLIC_Marshal(&data->publicArea, buffer, size);
    written += NV_TPMT_SENSITIVE_Marshal(&data->sensitive, buffer, size);

#if ALG_RSA
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);
#if ALG_RSA
    written += privateExponent_t_Marshal(&data->privateExponent,
                                         buffer, size);
#endif
    BLOCK_SKIP_WRITE_POP(size);

    written += TPM2B_NAME_Marshal(&data->qualifiedName, buffer, size);
    written += TPM_HANDLE_Marshal(&data->evictHandle, buffer, size);
    written += TPM2B_NAME_Marshal(&data->name, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    written += SEED_COMPAT_LEVEL_Marshal(&data->seedCompatLevel,
                                         buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);
    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
OBJECT_Unmarshal(OBJECT *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;
    BOOL needs_block;

    /*
     * attributes are read in ANY_OBJECT_Unmarshal
     */
    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 OBJECT_VERSION, OBJECT_MAGIC);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = TPMT_PUBLIC_Unmarshal(&data->publicArea, buffer, size, TRUE);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = NV_TPMT_SENSITIVE_Unmarshal(&data->sensitive, buffer, size);
    }

#if ALG_RSA
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_alg_rsa, needs_block, buffer, size,
                        "OBJECT", "privateExponent");
    }
#if ALG_RSA
    if (rc == TPM_RC_SUCCESS) {
        rc = privateExponent_t_Unmarshal(&data->privateExponent,
                                         buffer, size);
    }
#endif
skip_alg_rsa:

    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_NAME_Unmarshal(&data->qualifiedName, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM_HANDLE_Unmarshal(&data->evictHandle, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_NAME_Unmarshal(&data->name, buffer, size);
    }

    /* default values before conditional block */
    data->seedCompatLevel = SEED_COMPAT_LEVEL_ORIGINAL;

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, hdr.version >= 3, buffer, size,
                        "OBJECT", "version 3 or later");
        if (rc == TPM_RC_SUCCESS) {
            rc = SEED_COMPAT_LEVEL_Unmarshal(&data->seedCompatLevel,
                                        buffer, size,
                                        "OBJECT seedCompatLevel");
        }

        if (rc == TPM_RC_SUCCESS) {
            BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                            "OBJECT", "version 4 or later");
        }
        /* future versions nest-append here */
    }

skip_future_versions:
    return rc;
}

#define ANY_OBJECT_MAGIC 0xfe9a3974
#define ANY_OBJECT_VERSION 2

UINT16
ANY_OBJECT_Marshal(OBJECT *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    UINT32 *ptr = (UINT32 *)&data->attributes;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                ANY_OBJECT_VERSION, ANY_OBJECT_MAGIC, 1);

    written += UINT32_Marshal(ptr, buffer, size);
    /* the slot must be occupied, otherwise the rest may not be initialized */
    if (data->attributes.occupied) {
        if (ObjectIsSequence(data))
            written += HASH_OBJECT_Marshal((HASH_OBJECT *)data, buffer, size);
        else
            written += OBJECT_Marshal(data, buffer, size);
    }

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

TPM_RC
ANY_OBJECT_Unmarshal(OBJECT *data, BYTE **buffer, INT32 *size, BOOL verbose)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    UINT32 *ptr = (UINT32 *)&data->attributes;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_UnmarshalVerbose(&hdr, buffer, size,
                                        ANY_OBJECT_VERSION, ANY_OBJECT_MAGIC,
                                        verbose);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(ptr, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS && data->attributes.occupied) {
        if (ObjectIsSequence(data))
            rc = HASH_OBJECT_Unmarshal((HASH_OBJECT *)data, buffer, size);
        else
            rc = OBJECT_Unmarshal(data, buffer, size);
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "ANY_OBJECT", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:

    return rc;
}

static UINT16
TPMT_SYM_DEF_Marshal(TPMT_SYM_DEF *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;

    written = UINT16_Marshal(&data->algorithm, buffer, size);
    written += TPMU_SYM_KEY_BITS_Marshal(&data->keyBits, buffer, size, data->algorithm);
    written += TPMU_SYM_MODE_Marshal(&data->mode, buffer, size, data->algorithm);

    return written;
}

#define SESSION_MAGIC 0x44be9f45
#define SESSION_VERSION 2

static UINT16
SESSION_Marshal(SESSION *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    UINT8 clocksize;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                SESSION_VERSION, SESSION_MAGIC, 1);
    written += UINT32_Marshal((UINT32 *)&data->attributes, buffer, size);
    written += UINT32_Marshal(&data->pcrCounter, buffer, size);
    written += UINT64_Marshal(&data->startTime, buffer, size);
    written += UINT64_Marshal(&data->timeout, buffer, size);

#if CLOCK_STOPS
    clocksize = sizeof(UINT64);
    written += UINT8_Marshal(&clocksize, buffer, size);
    written += UINT64_Marshal(&data->epoch, buffer, size);
#else
    clocksize = sizeof(UINT32);
    written += UINT8_Marshal(&clocksize, buffer, size);
    written += UINT32_Marshal(&data->epoch, buffer, size);
#endif

    written += UINT32_Marshal(&data->commandCode, buffer, size);
    written += UINT16_Marshal(&data->authHashAlg, buffer, size);
    written += UINT8_Marshal(&data->commandLocality, buffer, size);
    written += TPMT_SYM_DEF_Marshal(&data->symmetric, buffer, size);
    written += TPM2B_AUTH_Marshal(&data->sessionKey, buffer, size);
    written += TPM2B_NONCE_Marshal(&data->nonceTPM, buffer, size);
    // TPM2B_NAME or TPM2B_DIGEST could be used for marshalling
    written += TPM2B_NAME_Marshal(&data->u1.boundEntity, buffer, size);
    written += TPM2B_DIGEST_Marshal(&data->u2.auditDigest, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
SESSION_Unmarshal(SESSION *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;
    UINT8 clocksize;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 SESSION_VERSION, SESSION_MAGIC);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal((UINT32 *)&data->attributes, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->pcrCounter, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&data->startTime, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&data->timeout, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT8_Unmarshal(&clocksize, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
#if CLOCK_STOPS
        if (clocksize != sizeof(UINT64)) {
            TPMLIB_LogTPM2Error("Unexpected clocksize for epoch; "
                                "Expected %zu, got %u\n",
                                sizeof(UINT64), clocksize);
            rc = TPM_RC_BAD_PARAMETER;
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = UINT64_Unmarshal(&data->epoch, buffer, size);
        }
#else
        if (clocksize != sizeof(UINT32)) {
            TPMLIB_LogTPM2Error("Unexpected clocksize for epoch; "
                                "Expected %zu, got %u\n",
                                sizeof(UINT32), clocksize);
            rc = TPM_RC_BAD_PARAMETER;
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = UINT32_Unmarshal(&data->epoch, buffer, size);
        }
#endif
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->commandCode, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&data->authHashAlg, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT8_Unmarshal(&data->commandLocality, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPMT_SYM_DEF_Unmarshal(&data->symmetric, buffer, size, YES);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_AUTH_Unmarshal(&data->sessionKey, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_NONCE_Unmarshal(&data->nonceTPM, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_NAME_Unmarshal(&data->u1.boundEntity, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_DIGEST_Unmarshal(&data->u2.auditDigest, buffer, size);
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "SESSION", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:
    return rc;
}

#define SESSION_SLOT_MAGIC 0x3664aebc
#define SESSION_SLOT_VERSION 2

static UINT16
SESSION_SLOT_Marshal(SESSION_SLOT *data, BYTE **buffer, INT32* size)
{
    UINT16 written;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                SESSION_SLOT_VERSION,
                                SESSION_SLOT_MAGIC, 1);

    written += BOOL_Marshal(&data->occupied, buffer, size);
    if (!data->occupied)
        return written;

    written += SESSION_Marshal(&data->session, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
SESSION_SLOT_Unmarshal(SESSION_SLOT *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 SESSION_SLOT_VERSION, SESSION_SLOT_MAGIC);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&data->occupied, buffer, size);
    }
    if (!data->occupied)
        return rc;

    if (rc == TPM_RC_SUCCESS) {
        rc = SESSION_Unmarshal(&data->session, buffer, size);
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "SESSION_SLOT", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:
    return rc;
}

#define VOLATILE_STATE_VERSION 4
#define VOLATILE_STATE_MAGIC 0x45637889

UINT16
VolatileState_Marshal(BYTE **buffer, INT32 *size)
{
    UINT16 written;
    size_t i;
    BOOL tpmEst;
    UINT64 tmp_uint64;
    UINT32 tmp_uint32;
    BOOL has_block;
    UINT16 array_size;
    BLOCK_SKIP_INIT;
    PERSISTENT_DATA pd;

    written = NV_HEADER_Marshal(buffer, size,
                                VOLATILE_STATE_VERSION, VOLATILE_STATE_MAGIC,
                                1);

    /* skip g_rcIndex: these are 'constants' */
    written += TPM_HANDLE_Marshal(&g_exclusiveAuditSession, buffer, size); /* line 423 */
    /* g_time: may not be necessary */
    written += UINT64_Marshal(&g_time, buffer, size); /* line 426 */
    /* g_timeEpoch: skipped so far -- needs investigation */
    /* g_phEnable: since we won't call TPM2_Starup, we need to write it */
    written += BOOL_Marshal(&g_phEnable, buffer, size); /* line 439 */
    /* g_pcrReconfig: must write */
    written += BOOL_Marshal(&g_pcrReConfig, buffer, size); /* line 443 */
    /* g_DRTMHandle: must write */
    written += TPM_HANDLE_Marshal(&g_DRTMHandle, buffer, size); /* line 448 */
    /* g_DrtmPreStartup: must write */
    written += BOOL_Marshal(&g_DrtmPreStartup, buffer, size); /* line 453 */
    /* g_StartupLocality3: must write */
    written += BOOL_Marshal(&g_StartupLocality3, buffer, size); /* line 458 */

#if USE_DA_USED
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#if USE_DA_USED
    /* g_daUsed: must write */
    written += BOOL_Marshal(&g_daUsed, buffer, size); /* line 484 */
#endif
    BLOCK_SKIP_WRITE_POP(size);

    /* g_updateNV: can skip since it seems to only be valid during execution of a command*/
    /* g_powerWasLost: must write */
    written += BOOL_Marshal(&g_powerWasLost, buffer, size); /* line 504 */
    /* g_clearOrderly: can skip since it seems to only be valid during execution of a command */
    /* g_prevOrderlyState: must write */
    written += UINT16_Marshal(&g_prevOrderlyState, buffer, size); /* line 516 */
    /* g_nvOk: must write */
    written += BOOL_Marshal(&g_nvOk, buffer, size); /* line 522 */
    /* g_NvStatus: can skip since it seems to only be valid during execution of a command */

#if 0 /* does not exist */
    written += TPM2B_AUTH_Marshal(&g_platformUniqueAuthorities, buffer, size); /* line 535 */
#endif
    written += TPM2B_AUTH_Marshal(&g_platformUniqueDetails, buffer, size); /* line 536 */

    /* gp (persistent_data): skip; we assume its latest states in the persistent data file */

    /* we store the next 3 because they may not have been written to NVRAM */
    written += ORDERLY_DATA_Marshal(&go, buffer, size); /* line 707 */
    written += STATE_CLEAR_DATA_Marshal(&gc, buffer, size); /* line 738 */
    written += STATE_RESET_DATA_Marshal(&gr, buffer, size); /* line 826 */

    /* g_manufactured: must write */
    written += BOOL_Marshal(&g_manufactured, buffer, size); /* line 928 */
    /* g_initialized: must write */
    written += BOOL_Marshal(&g_initialized, buffer, size); /* line 932 */

#if defined SESSION_PROCESS_C || defined GLOBAL_C || defined MANUFACTURE_C
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#if defined SESSION_PROCESS_C || defined GLOBAL_C || defined MANUFACTURE_C
    /*
     * The session related variables may only be valid during the execution
     * of a single command; safer to store
     */
    array_size = ARRAY_SIZE(s_sessionHandles);
    written += UINT16_Marshal(&array_size, buffer, size);

    for (i = 0; i < array_size; i++) {
        written += TPM_HANDLE_Marshal(&s_sessionHandles[i], buffer, size);
        written += TPMA_SESSION_Marshal(&s_attributes[i], buffer, size);
        written += TPM_HANDLE_Marshal(&s_associatedHandles[i], buffer, size);
        written += TPM2B_NONCE_Marshal(&s_nonceCaller[i], buffer, size);
        written += TPM2B_AUTH_Marshal(&s_inputAuthValues[i], buffer, size);
        /* s_usedSessions: cannot serialize this since it is a pointer; also, isn't used */
    }
    written += TPM_HANDLE_Marshal(&s_encryptSessionIndex, buffer, size);
    written += TPM_HANDLE_Marshal(&s_decryptSessionIndex, buffer, size);
    written += TPM_HANDLE_Marshal(&s_auditSessionIndex, buffer, size);

#if CC_GetCommandAuditDigest
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#if CC_GetCommandAuditDigest
    /* s_cpHashForCommandAudit: seems not used; better to write it */
    written += TPM2B_DIGEST_Marshal(&s_cpHashForCommandAudit, buffer, size);
#endif
    BLOCK_SKIP_WRITE_POP(size);

    /* s_DAPendingOnNV: needs investigation ... */
    written += BOOL_Marshal(&s_DAPendingOnNV, buffer, size);
#endif // SESSION_PROCESS_C
    BLOCK_SKIP_WRITE_POP(size);

#if defined DA_C || defined GLOBAL_C || defined MANUFACTURE_C
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#if defined DA_C || defined GLOBAL_C || defined MANUFACTURE_C

#if !ACCUMULATE_SELF_HEAL_TIMER
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#if !ACCUMULATE_SELF_HEAL_TIMER
    written += UINT64_Marshal(&s_selfHealTimer, buffer, size); /* line 975 */
    written += UINT64_Marshal(&s_lockoutTimer, buffer, size); /* line 977 */
#endif // ACCUMULATE_SELF_HEAL_TIMER
    BLOCK_SKIP_WRITE_POP(size);
#endif // DA_C
    BLOCK_SKIP_WRITE_POP(size);

#if defined NV_C || defined GLOBAL_C
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);
    /* s_evictNvEnd set in NvInitStatic called by NvPowerOn in case g_powerWasLost
     * Unless we set g_powerWasLost=TRUE and call NvPowerOn, we have to include it.
     */
#if defined NV_C || defined GLOBAL_C
    written += UINT32_Marshal(&s_evictNvEnd, buffer, size); /* line 984 */
    /* s_indexOrderlyRam read from NVRAM in NvEntityStartup and written to it
     * in NvUpdateIndexOrderlyData called by TPM2_Shutdown and initialized
     * in NvManufacture -- since we don't call TPM2_Shutdown we serialize it here
     */
    array_size = sizeof(s_indexOrderlyRam);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal(s_indexOrderlyRam, array_size, buffer, size);

    written += UINT64_Marshal(&s_maxCounter, buffer, size); /* line 992 */
    /* the following need not be written; NvIndexCacheInit initializes them partly
     * and NvIndexCacheInit() is called during ExecuteCommand()
     * - s_cachedNvIndex
     * - s_cachedNvRef
     * - s_cachedNvRamRef
     */
#endif
    BLOCK_SKIP_WRITE_POP(size);

#if defined OBJECT_C || defined GLOBAL_C
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#if defined OBJECT_C || defined GLOBAL_C
    /* used in many places; it doesn't look like TPM2_Shutdown writes this into
     * persistent memory, so what is lost upon TPM2_Shutdown?
     */
    array_size = ARRAY_SIZE(s_objects);
    written += UINT16_Marshal(&array_size, buffer, size);

    for (i = 0; i < array_size; i++) {
        written += ANY_OBJECT_Marshal(&s_objects[i], buffer, size);
    }
#endif
    BLOCK_SKIP_WRITE_POP(size);

#if defined PCR_C || defined GLOBAL_C
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#if defined PCR_C || defined GLOBAL_C
    /* s_pcrs: Marshal *all* PCRs, even those for which stateSave bit is not set */
    array_size = ARRAY_SIZE(s_pcrs);
    written += UINT16_Marshal(&array_size, buffer, size);

    for (i = 0; i < array_size; i++) {
        written += PCR_Marshal(&s_pcrs[i], buffer, size);
    }
#endif
    BLOCK_SKIP_WRITE_POP(size);

#if defined SESSION_C || defined GLOBAL_C
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#if defined SESSION_C || defined GLOBAL_C
    /* s_sessions: */
    array_size = ARRAY_SIZE(s_sessions);
    written += UINT16_Marshal(&array_size, buffer, size);

    for (i = 0; i < array_size; i++) {
        written += SESSION_SLOT_Marshal(&s_sessions[i], buffer, size);
    }
    /* s_oldestSavedSession: */
    written += UINT32_Marshal(&s_oldestSavedSession, buffer, size);
    /* s_freeSessionSlots: */
    written += UINT32_Marshal((UINT32 *)&s_freeSessionSlots, buffer, size);
#endif
    BLOCK_SKIP_WRITE_POP(size);

#if defined IO_BUFFER_C || defined GLOBAL_C
    /* s_actionInputBuffer: skip; only used during a single command */
    /* s_actionOutputBuffer: skip; only used during a single command */
#endif
    written += BOOL_Marshal(&g_inFailureMode, buffer, size); /* line 1078 */

    /* TPM established bit */
    tpmEst = _rpc__Signal_GetTPMEstablished();
    written += BOOL_Marshal(&tpmEst, buffer, size);

#if defined TPM_FAIL_C || defined GLOBAL_C || 1
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#if defined TPM_FAIL_C || defined GLOBAL_C || 1
    written += UINT32_Marshal(&s_failFunction, buffer, size);
    written += UINT32_Marshal(&s_failLine, buffer, size);
    written += UINT32_Marshal(&s_failCode, buffer, size);
#endif // TPM_FAIL_C
    BLOCK_SKIP_WRITE_POP(size);

#ifndef HARDWARE_CLOCK
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#ifndef HARDWARE_CLOCK
    tmp_uint64 = s_realTimePrevious;
    written += UINT64_Marshal(&tmp_uint64, buffer, size);
    tmp_uint64 = s_tpmTime;
    written += UINT64_Marshal(&tmp_uint64, buffer, size);
#endif
    BLOCK_SKIP_WRITE_POP(size);

    written += BOOL_Marshal(&s_timerReset, buffer, size);
    written += BOOL_Marshal(&s_timerStopped, buffer, size);
    written += UINT32_Marshal(&s_adjustRate, buffer, size);

    tmp_uint64 = ClockGetTime(CLOCK_REALTIME);
    written += UINT64_Marshal(&tmp_uint64, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size); /* v3 */

    /* tie the volatile state to the EP,SP, and PPSeed */
    NvRead(&pd, NV_PERSISTENT_DATA, sizeof(pd));
    written += TPM2B_Marshal(&pd.EPSeed.b, sizeof(pd.EPSeed.t.buffer), buffer, size);
    written += TPM2B_Marshal(&pd.SPSeed.b, sizeof(pd.SPSeed.t.buffer), buffer, size);
    written += TPM2B_Marshal(&pd.PPSeed.b, sizeof(pd.PPSeed.t.buffer), buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size); /* v4 */

    tmp_uint64 = ClockGetTime(CLOCK_MONOTONIC) + s_hostMonotonicAdjustTime;
    written += UINT64_Marshal(&tmp_uint64, buffer, size);

    written += UINT64_Marshal(&s_suspendedElapsedTime, buffer, size);
    written += UINT64_Marshal(&s_lastSystemTime, buffer, size);
    written += UINT64_Marshal(&s_lastReportedTime, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size); /* v5 */
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size); /* v5 */
    BLOCK_SKIP_WRITE_POP(size); /* v4 */
    BLOCK_SKIP_WRITE_POP(size); /* v3 */

    /* keep marker at end */
    tmp_uint32 = VOLATILE_STATE_MAGIC;
    written += UINT32_Marshal(&tmp_uint32, buffer, size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
VolatileState_TailV4_Unmarshal(BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    UINT64 tmp_uint64;

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&tmp_uint64, buffer, size);
        s_hostMonotonicAdjustTime = tmp_uint64 - ClockGetTime(CLOCK_MONOTONIC);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&s_suspendedElapsedTime, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&s_lastSystemTime, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&s_lastReportedTime, buffer, size);
    }

    return rc;
}

static TPM_RC
VolatileState_TailV3_Unmarshal(BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    PERSISTENT_DATA pd;
    TPM2B_SEED seed = {
        .b.size = 0,
    };

    NvRead(&pd, NV_PERSISTENT_DATA, sizeof(pd));

    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_Unmarshal(&seed.b, PRIMARY_SEED_SIZE, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        if (seed.b.size > PRIMARY_SEED_SIZE) /* coverity */
            rc = TPM_RC_SIZE;
    }
    if (rc == TPM_RC_SUCCESS) {
        if (TPM2B_Cmp(&seed.b, &pd.EPSeed.b)) {
            TPMLIB_LogTPM2Error("%s: EPSeed does not match\n",
                                __func__);
            rc = TPM_RC_VALUE;
        }
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_Unmarshal(&seed.b, PRIMARY_SEED_SIZE, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        if (seed.b.size > PRIMARY_SEED_SIZE) /* coverity */
            rc = TPM_RC_SIZE;
    }
    if (rc == TPM_RC_SUCCESS) {
        if (TPM2B_Cmp(&seed.b, &pd.SPSeed.b)) {
            TPMLIB_LogTPM2Error("%s: SPSeed does not match\n",
                                __func__);
            rc = TPM_RC_VALUE;
        }
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_Unmarshal(&seed.b, PRIMARY_SEED_SIZE, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        if (seed.b.size > PRIMARY_SEED_SIZE) /* coverity */
            rc = TPM_RC_SIZE;
    }
    if (rc == TPM_RC_SUCCESS) {
        if (TPM2B_Cmp(&seed.b, &pd.PPSeed.b)) {
            TPMLIB_LogTPM2Error("%s: PPSeed does not match\n",
                                __func__);
            rc = TPM_RC_VALUE;
        }
    }

    return rc;
}

TPM_RC
VolatileState_Unmarshal(BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    size_t i;
    UINT64 tmp_uint64;
    UINT32 tmp_uint32;
    NV_HEADER hdr;
    BOOL needs_block;
    UINT16 array_size = 0;
    UINT64 backthen;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 VOLATILE_STATE_VERSION, VOLATILE_STATE_MAGIC);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM_HANDLE_Unmarshal(&g_exclusiveAuditSession, buffer, size); /* line 423 */
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&g_time, buffer, size); /* line 426 */
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&g_phEnable, buffer, size); /* line 439 */
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&g_pcrReConfig, buffer, size); /* line 443 */
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM_HANDLE_Unmarshal(&g_DRTMHandle, buffer, size); /* line 448 */
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&g_DrtmPreStartup, buffer, size); /* line 453 */
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&g_StartupLocality3, buffer, size); /* line 458 */
    }

#if USE_DA_USED
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_da, needs_block, buffer, size,
                        "Volatile state", "g_daUsed");
    }
#if USE_DA_USED
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&g_daUsed, buffer, size); /* line 484 */
    }
#endif
skip_da:

    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&g_powerWasLost, buffer, size); /* line 504 */
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&g_prevOrderlyState, buffer, size); /* line 516 */
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&g_nvOk, buffer, size); /* line 522 */
    }
#if 0 /* does not exist */
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_AUTH_Unmarshal(&g_platformUniqueAuthorities, buffer, size); /* line 535 */
    }
#endif
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_AUTH_Unmarshal(&g_platformUniqueDetails, buffer, size); /* line 536 */
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = ORDERLY_DATA_Unmarshal(&go, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = STATE_CLEAR_DATA_Unmarshal(&gc, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
       rc = STATE_RESET_DATA_Unmarshal(&gr, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&g_manufactured, buffer, size); /* line 928 */
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&g_initialized, buffer, size); /* line 932 */
    }

#if defined SESSION_PROCESS_C || defined GLOBAL_C || defined MANUFACTURE_C
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_session_process, needs_block, buffer, size,
                        "Volatile state", "s_sessionHandles");
    }
#if defined SESSION_PROCESS_C || defined GLOBAL_C || defined MANUFACTURE_C
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != ARRAY_SIZE(s_sessionHandles)) {
        TPMLIB_LogTPM2Error("Volatile state: Bad array size for s_sessionHandles; "
                            "expected %zu, got %u\n",
                            ARRAY_SIZE(s_sessionHandles), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    for (i = 0; i < array_size && rc == TPM_RC_SUCCESS; i++) {
        if (rc == TPM_RC_SUCCESS) {
            rc = TPM_HANDLE_Unmarshal(&s_sessionHandles[i], buffer, size);
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = TPMA_SESSION_Unmarshal(&s_attributes[i], buffer, size);
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = TPM_HANDLE_Unmarshal(&s_associatedHandles[i], buffer, size);
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = TPM2B_NONCE_Unmarshal(&s_nonceCaller[i], buffer, size);
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = TPM2B_AUTH_Unmarshal(&s_inputAuthValues[i], buffer, size);
        }
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = TPM_HANDLE_Unmarshal(&s_encryptSessionIndex, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM_HANDLE_Unmarshal(&s_decryptSessionIndex, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM_HANDLE_Unmarshal(&s_auditSessionIndex, buffer, size);
    }

#if CC_GetCommandAuditDigest
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_cc_getcommandauditdigest, needs_block, buffer, size,
                        "Volatile state", "s_cpHashForCommandAudit");
    }
#if CC_GetCommandAuditDigest
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_DIGEST_Unmarshal(&s_cpHashForCommandAudit, buffer, size);
    }
#endif
skip_cc_getcommandauditdigest:

    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&s_DAPendingOnNV, buffer, size);
    }
#endif /* SESSION_PROCESS_C */
skip_session_process:

#if defined DA_C || defined GLOBAL_C || defined MANUFACTURE_C
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_accumulate_self_heal_timer_1, needs_block, buffer, size,
                        "Volatile state", "s_selfHealTimer.1");
    }

#if defined DA_C || defined GLOBAL_C || defined MANUFACTURE_C
#if !ACCUMULATE_SELF_HEAL_TIMER
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_accumulate_self_heal_timer_2, needs_block, buffer, size,
                        "Volatile state", "s_selfHealTimer.2");
    }
#if !ACCUMULATE_SELF_HEAL_TIMER
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&s_selfHealTimer, buffer, size); /* line 975 */
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&s_lockoutTimer, buffer, size); /* line 977 */
    }
#endif
skip_accumulate_self_heal_timer_2:
#endif
skip_accumulate_self_heal_timer_1:

#if defined NV_C || defined GLOBAL_C
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_nv, needs_block, buffer, size,
                        "Volatile state", "s_evictNvEnd");
    }

#if defined NV_C || defined GLOBAL_C
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&s_evictNvEnd, buffer, size); /* line 984 */
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != ARRAY_SIZE(s_indexOrderlyRam)) {
        TPMLIB_LogTPM2Error("Volatile state: Bad array size for s_indexOrderlyRam; "
                            "expected %zu, got %u\n",
                            ARRAY_SIZE(s_indexOrderlyRam), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = Array_Unmarshal(s_indexOrderlyRam, array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&s_maxCounter, buffer, size); /* line 992 */
    }
    /* The following are not included:
     * - s_cachedNvIndex
     * - s_cachedNvRef
     * - s_cachedNvRamRef
     */
#endif
skip_nv:

#if defined OBJECT_C || defined GLOBAL_C
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_object, needs_block, buffer, size,
                        "Volatile state", "s_objects");
    }
#if defined OBJECT_C || defined GLOBAL_C
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != ARRAY_SIZE(s_objects)) {
        TPMLIB_LogTPM2Error("Volatile state: Bad array size for s_objects; "
                            "expected %zu, got %u\n",
                            ARRAY_SIZE(s_objects), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    for (i = 0; i < array_size && rc == TPM_RC_SUCCESS; i++) {
        rc = ANY_OBJECT_Unmarshal(&s_objects[i], buffer, size, true);
    }
#endif
skip_object:

#if defined PCR_C || defined GLOBAL_C
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_pcr, needs_block, buffer, size,
                        "Volatile state", "s_pcrs");
    }
#if defined PCR_C || defined GLOBAL_C
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != ARRAY_SIZE(s_pcrs)) {
        TPMLIB_LogTPM2Error("Volatile state: Bad array size for s_pcrs; "
                            "expected %zu, got %u\n",
                            ARRAY_SIZE(s_pcrs), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    for (i = 0; i < array_size && rc == TPM_RC_SUCCESS; i++) {
        rc = PCR_Unmarshal(&s_pcrs[i], buffer, size, &shadow.pcrAllocated);
    }
#endif
skip_pcr:

#if defined SESSION_C || defined GLOBAL_C
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_session, needs_block, buffer, size,
                        "Volatile state", "s_sessions");
    }
#if defined SESSION_C || defined GLOBAL_C
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS &&
        array_size != ARRAY_SIZE(s_sessions)) {
        TPMLIB_LogTPM2Error("Volatile state: Bad array size for s_sessions; "
                            "expected %zu, got %u\n",
                            ARRAY_SIZE(s_sessions), array_size);
        rc = TPM_RC_BAD_PARAMETER;
    }
    /* s_sessions: */
    for (i = 0; i < array_size && rc == TPM_RC_SUCCESS; i++) {
        rc = SESSION_SLOT_Unmarshal(&s_sessions[i], buffer, size);
    }
    /* s_oldestSavedSession: */
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&s_oldestSavedSession, buffer, size);
    }
    /* s_freeSessionSlots: */
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal((UINT32 *)&s_freeSessionSlots, buffer, size);
    }
#endif
skip_session:

    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&g_inFailureMode, buffer, size); /* line 1078 */
    }

    /* TPM established bit */
    if (rc == TPM_RC_SUCCESS) {
        BOOL tpmEst;
        rc = BOOL_Unmarshal(&tpmEst, buffer, size);
        if (rc == TPM_RC_SUCCESS) {
            if (tpmEst)
                _rpc__Signal_SetTPMEstablished();
            else
                _rpc__Signal_ResetTPMEstablished();
        }
    }

#if defined TPM_FAIL_C || defined GLOBAL_C || 1
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_fail, needs_block, buffer, size,
                        "Volatile state", "s_failFunction");
    }

#if defined TPM_FAIL_C || defined GLOBAL_C || 1
    /* appended in v2 */
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&s_failFunction, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&s_failLine, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&s_failCode, buffer, size);
    }
#endif
skip_fail:

#ifndef HARDWARE_CLOCK
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_hardware_clock, needs_block, buffer, size,
                        "Volatile state", "s_realTimePrevious");
    }

#ifndef HARDWARE_CLOCK
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&tmp_uint64, buffer, size);
        s_realTimePrevious = tmp_uint64;
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&tmp_uint64, buffer, size);
        s_tpmTime = tmp_uint64;
    }
#endif
skip_hardware_clock:

    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&s_timerReset, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&s_timerStopped, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
       rc = UINT32_Unmarshal(&s_adjustRate, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&backthen, buffer, size);
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, hdr.version >= 3, buffer, size,
                        "Volatile State", "version 3 or later");
        if (rc == TPM_RC_SUCCESS) {
            rc = VolatileState_TailV3_Unmarshal(buffer, size);
        }
        if (rc == TPM_RC_SUCCESS) {
            BLOCK_SKIP_READ(skip_future_versions, hdr.version >= 4, buffer, size,
                            "Volatile State", "version 4 or later");
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = VolatileState_TailV4_Unmarshal(buffer, size);
        }
        if (rc == TPM_RC_SUCCESS) {
            BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                            "Volatile State", "version 5 or later");
        }
        /* future versions append here */
    }

skip_future_versions:

    /* keep marker at end: */
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&tmp_uint32, buffer, size);
        if (rc == TPM_RC_SUCCESS) {
            if (tmp_uint32 != VOLATILE_STATE_MAGIC) {
                TPMLIB_LogTPM2Error("Invalid volatile state magic. "
                                    "Expected 0x%08x, got 0x%08x\n",
                                    VOLATILE_STATE_MAGIC, tmp_uint32);
                rc = TPM_RC_BAD_TAG;
            }
        }
    }

    if (rc == TPM_RC_SUCCESS) {
        BOOL timesAreRealtime = hdr.version <= 3;
        /* Before Rev148 (header version <= 3), times were reported in
           realtime; we need to account for this now */
        ClockAdjustPostResume(backthen, timesAreRealtime);
    }
    return rc;
}

/********************************************************************
 * The following is a list of compile-time constants that we verify against
 * when state is presented to us. Comparison operators allow us to verify
 * compile time constants' values against what we would accept when reading
 * state. So for example a value of 1024 for a buffer size that is read can
 * be compared against the value that this implementation has been compiled
 * with. In some case a 'less or equal' [LE] (1024 < 2048) may be acceptable
 * but that depends on the purpose of the compile time constant. The most
 * conservative approach is to force that the unmarshalled values are equal
 * [EQ] to the ones of this implementation.
 *
 * Meanings of comparison operators:
 * EQ: The read state must match the state the implementation would produce
 *     The algorithm must have been enabled at the previously implementation
 *     and at the current implementation; or it must have been disabled at
 *     both
 *
 * LE: The read state may have been written by a version that did not
 *     implement an algorithm ('0') but the current implementation does
 *     implement it ('1'); this does NOT allow an implementation to accept
 *     the state anymore if the state was written by an implementation that
 *     implemented it ('1') but the current implementation does not im-
 *     plement it
 *
 * DONTCARE: Implementation that wrote the state can either have implemented
 *           an algorithm or not and implementation reading the state may
 *           also either implement it or not
 */
static const struct _entry {
    UINT32 constant;
    char *name;
    enum CompareOp { EQ, LE, GE, DONTCARE } cmp;
} pa_compile_constants[] = {
#define COMPILE_CONSTANT(CONST, CMP) \
    .constant = CONST, .name = #CONST, .cmp = CMP
    { COMPILE_CONSTANT(ALG_RSA, EQ) },
    { COMPILE_CONSTANT(ALG_SHA1, EQ) },
    { COMPILE_CONSTANT(ALG_HMAC, EQ) },
    { COMPILE_CONSTANT(ALG_TDES, LE) },
    { COMPILE_CONSTANT(ALG_AES, EQ) },
    { COMPILE_CONSTANT(ALG_MGF1, EQ) },
    { COMPILE_CONSTANT(ALG_XOR, EQ) },
    { COMPILE_CONSTANT(ALG_KEYEDHASH, EQ) },
    { COMPILE_CONSTANT(ALG_SHA256, EQ) },
    { COMPILE_CONSTANT(ALG_SHA384, EQ) },
    { COMPILE_CONSTANT(ALG_SHA512, EQ) },
    { COMPILE_CONSTANT(ALG_SM3_256, EQ) },
    { COMPILE_CONSTANT(ALG_SM4, EQ) },
    { COMPILE_CONSTANT(ALG_RSASSA, EQ) },
    { COMPILE_CONSTANT(ALG_RSAES, EQ) },
    { COMPILE_CONSTANT(ALG_RSAPSS, EQ) },
    { COMPILE_CONSTANT(ALG_OAEP, EQ) },
    { COMPILE_CONSTANT(ALG_ECC, EQ) },
    { COMPILE_CONSTANT(ALG_ECDH, EQ) },
    { COMPILE_CONSTANT(ALG_ECDSA, EQ) },
    { COMPILE_CONSTANT(ALG_ECDAA, EQ) },
    { COMPILE_CONSTANT(ALG_SM2, LE) },
    { COMPILE_CONSTANT(ALG_ECSCHNORR, EQ) },
    { COMPILE_CONSTANT(ALG_ECMQV, LE) },
    { COMPILE_CONSTANT(ALG_SYMCIPHER, EQ) },
    { COMPILE_CONSTANT(ALG_KDF1_SP800_56A, EQ) },
    { COMPILE_CONSTANT(ALG_KDF2, LE) },
    { COMPILE_CONSTANT(ALG_KDF1_SP800_108, EQ) },
    { COMPILE_CONSTANT(ALG_CMAC, LE) },
    { COMPILE_CONSTANT(ALG_CTR, EQ) },
    { COMPILE_CONSTANT(ALG_OFB, EQ) },
    { COMPILE_CONSTANT(ALG_CBC, EQ) },
    { COMPILE_CONSTANT(ALG_CFB, EQ) },
    { COMPILE_CONSTANT(ALG_ECB, EQ) },
    { COMPILE_CONSTANT(MAX_RSA_KEY_BITS, LE) }, /* old: 2048 */
    { COMPILE_CONSTANT(MAX_TDES_KEY_BITS, EQ) },
    { COMPILE_CONSTANT(MAX_AES_KEY_BITS, EQ) },
    { COMPILE_CONSTANT(128, EQ) }, /* MAX_SM4_KEY_BITS      in older code was 128 also with SM4 not active */
    { COMPILE_CONSTANT(128, EQ) }, /* MAX_CAMELLIA_KEY_BITS in older code was 128 also with CAMELLIA not active */
    { COMPILE_CONSTANT(ECC_NIST_P192, LE) },
    { COMPILE_CONSTANT(ECC_NIST_P224, LE) },
    { COMPILE_CONSTANT(ECC_NIST_P256, LE) },
    { COMPILE_CONSTANT(ECC_NIST_P384, LE) },
    { COMPILE_CONSTANT(ECC_NIST_P521, LE) },
    { COMPILE_CONSTANT(ECC_BN_P256, LE) },
    { COMPILE_CONSTANT(ECC_BN_P638, LE) },
    { COMPILE_CONSTANT(ECC_SM2_P256, LE) },
    { COMPILE_CONSTANT(MAX_ECC_KEY_BITS, LE) },
    { COMPILE_CONSTANT(4, EQ) }, /* was: HASH_ALIGNMENT, which is not relevant */
    { COMPILE_CONSTANT(SYM_ALIGNMENT, EQ) },
    { COMPILE_CONSTANT(IMPLEMENTATION_PCR, EQ) },
    { COMPILE_CONSTANT(PLATFORM_PCR, EQ) },
    { COMPILE_CONSTANT(DRTM_PCR, EQ) },
    { COMPILE_CONSTANT(HCRTM_PCR, EQ) },
    { COMPILE_CONSTANT(NUM_LOCALITIES, EQ) },
    { COMPILE_CONSTANT(MAX_HANDLE_NUM, EQ) },
    { COMPILE_CONSTANT(MAX_ACTIVE_SESSIONS, EQ) },
    { COMPILE_CONSTANT(MAX_LOADED_SESSIONS, EQ) },
    { COMPILE_CONSTANT(MAX_SESSION_NUM, EQ) },
    { COMPILE_CONSTANT(MAX_LOADED_OBJECTS, EQ) },
    { COMPILE_CONSTANT(MIN_EVICT_OBJECTS, LE) },
    { COMPILE_CONSTANT(NUM_POLICY_PCR_GROUP, EQ) },
    { COMPILE_CONSTANT(NUM_AUTHVALUE_PCR_GROUP, EQ) },
    { COMPILE_CONSTANT(MAX_CONTEXT_SIZE, LE) }, /* old: 2474 */
    { COMPILE_CONSTANT(MAX_DIGEST_BUFFER, EQ) },
    { COMPILE_CONSTANT(MAX_NV_INDEX_SIZE, EQ) },
    { COMPILE_CONSTANT(MAX_NV_BUFFER_SIZE, EQ) },
    { COMPILE_CONSTANT(MAX_CAP_BUFFER, EQ) },
    { COMPILE_CONSTANT(NV_MEMORY_SIZE, LE) },
    { COMPILE_CONSTANT(MIN_COUNTER_INDICES, EQ) },
    { COMPILE_CONSTANT(NUM_STATIC_PCR, EQ) },
    { COMPILE_CONSTANT(MAX_ALG_LIST_SIZE, EQ) },
    { COMPILE_CONSTANT(PRIMARY_SEED_SIZE, EQ) },
#if CONTEXT_ENCRYPT_ALGORITHM == AES
#define CONTEXT_ENCRYPT_ALGORITHM_ TPM_ALG_AES
#endif
    { COMPILE_CONSTANT(CONTEXT_ENCRYPT_ALGORITHM_, EQ) },
    { COMPILE_CONSTANT(NV_CLOCK_UPDATE_INTERVAL, EQ) },
    { COMPILE_CONSTANT(NUM_POLICY_PCR, EQ) },
    { COMPILE_CONSTANT(ORDERLY_BITS, EQ) },
    { COMPILE_CONSTANT(MAX_SYM_DATA, EQ) },
    { COMPILE_CONSTANT(MAX_RNG_ENTROPY_SIZE, EQ) },
    { COMPILE_CONSTANT(RAM_INDEX_SPACE, EQ) },
    { COMPILE_CONSTANT(RSA_DEFAULT_PUBLIC_EXPONENT, EQ) },
    { COMPILE_CONSTANT(ENABLE_PCR_NO_INCREMENT, EQ) },
    { COMPILE_CONSTANT(CRT_FORMAT_RSA, EQ) },
    { COMPILE_CONSTANT(VENDOR_COMMAND_COUNT, EQ) },
    { COMPILE_CONSTANT(MAX_VENDOR_BUFFER_SIZE, EQ) },
    { COMPILE_CONSTANT(TPM_MAX_DERIVATION_BITS, EQ) },
    { COMPILE_CONSTANT(PROOF_SIZE, EQ) },
    { COMPILE_CONSTANT(HASH_COUNT, EQ) },

    /* added for PA_COMPILE_CONSTANTS_VERSION == 3 */
    { COMPILE_CONSTANT(AES_128, LE) },
    { COMPILE_CONSTANT(AES_192, LE) },
    { COMPILE_CONSTANT(AES_256, LE) },
    { COMPILE_CONSTANT(SM4_128, LE) },
    { COMPILE_CONSTANT(ALG_CAMELLIA, LE) },
    { COMPILE_CONSTANT(CAMELLIA_128, LE) },
    { COMPILE_CONSTANT(CAMELLIA_192, LE) },
    { COMPILE_CONSTANT(CAMELLIA_256, LE) },
    { COMPILE_CONSTANT(ALG_SHA3_256, LE) },
    { COMPILE_CONSTANT(ALG_SHA3_384, LE) },
    { COMPILE_CONSTANT(ALG_SHA3_512, LE) },
    { COMPILE_CONSTANT(RSA_1024, LE) },
    { COMPILE_CONSTANT(RSA_2048, LE) },
    { COMPILE_CONSTANT(RSA_3072, LE) },
    { COMPILE_CONSTANT(RSA_4096, LE) },
    { COMPILE_CONSTANT(RSA_16384, LE) },
    { COMPILE_CONSTANT(RH_ACT_0, LE) },
    { COMPILE_CONSTANT(RH_ACT_1, LE) },
    { COMPILE_CONSTANT(RH_ACT_2, LE) },
    { COMPILE_CONSTANT(RH_ACT_3, LE) },
    { COMPILE_CONSTANT(RH_ACT_4, LE) },
    { COMPILE_CONSTANT(RH_ACT_5, LE) },
    { COMPILE_CONSTANT(RH_ACT_6, LE) },
    { COMPILE_CONSTANT(RH_ACT_7, LE) },
    { COMPILE_CONSTANT(RH_ACT_8, LE) },
    { COMPILE_CONSTANT(RH_ACT_9, LE) },
    { COMPILE_CONSTANT(RH_ACT_A, LE) },
    { COMPILE_CONSTANT(RH_ACT_B, LE) },
    { COMPILE_CONSTANT(RH_ACT_C, LE) },
    { COMPILE_CONSTANT(RH_ACT_D, LE) },
    { COMPILE_CONSTANT(RH_ACT_E, LE) },
    { COMPILE_CONSTANT(RH_ACT_F, LE) },
};

static TPM_RC
UINT32_Unmarshal_CheckConstant(BYTE **buffer, INT32 *size, UINT32 constant,
                               const char *name,
                               enum CompareOp cmp, UINT16 struct_version)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    UINT32 value;
    const char *op = NULL;

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&value, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        switch (cmp) {
        case EQ:
            if (!(constant == value))
                op = "=";
            break;
        case LE:
            if (!(value <= constant))
                op = "<=";
            break;
        case GE:
            if (!(value >= constant))
                op = ">=";
            break;
        case DONTCARE:
            break;
        }
        if (op) {
            TPMLIB_LogTPM2Error("Unexpected value for %s; "
                                "its value %d is not %s %d; "
                                "(version: %u)\n",
                                name, value, op, constant,
                                struct_version);
            rc = TPM_RC_BAD_PARAMETER;
        }
    }
    return rc;
}

#define PA_COMPILE_CONSTANTS_MAGIC 0xc9ea6431
#define PA_COMPILE_CONSTANTS_VERSION 3

/* Marshal compile-time constants related to persistent-all state */
static UINT32
PACompileConstants_Marshal(BYTE **buffer, INT32 *size)
{
    unsigned i;
    UINT32 written, tmp_uint32;
    UINT32 array_size = ARRAY_SIZE(pa_compile_constants);
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                PA_COMPILE_CONSTANTS_VERSION,
                                PA_COMPILE_CONSTANTS_MAGIC, 1);

    written += UINT32_Marshal(&array_size, buffer, size);

    for (i = 0; i < array_size; i++) {
        tmp_uint32 = pa_compile_constants[i].constant;
        written += UINT32_Marshal(&tmp_uint32, buffer, size);
    }

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
PACompileConstants_Unmarshal(BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    unsigned i;
    NV_HEADER hdr;
    UINT32 array_size;
    UINT32 exp_array_size = 0;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 PA_COMPILE_CONSTANTS_VERSION,
                                 PA_COMPILE_CONSTANTS_MAGIC);
    }
    if (rc == TPM_RC_SUCCESS) {
        switch (hdr.version) {
        case 1:
        case 2:
            /* PA_COMPILE_CONSTANTS_VERSION 1 and 2 had 88 entries */
            exp_array_size = 88;
            break;
        case 3:
            /* PA_COMPILE_CONSTANTS_VERSION 3 had 104 entries */
            exp_array_size = 120;
            break;
        default:
            /* we don't suport anything newer - no downgrade */
            TPMLIB_LogTPM2Error("Unsupported PA_COMPILE_CONSTANTS version %d. "
                                "Supporting up to version %d.\n",
                                hdr.version, PA_COMPILE_CONSTANTS_VERSION);
            rc = TPM_RC_BAD_VERSION;
        }
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&array_size, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS &&
        array_size != exp_array_size) {
        TPMLIB_LogTPM2Error("PA_COMPILE_CONSTANTS v%d has non-matching number of "
                            "elements; found %u, expected %u\n",
                            hdr.version, array_size, exp_array_size);
    }

    for (i = 0; rc == TPM_RC_SUCCESS && i < exp_array_size; i++)
        rc = UINT32_Unmarshal_CheckConstant(
                                  buffer, size, pa_compile_constants[i].constant,
                                  pa_compile_constants[i].name,
                                  pa_compile_constants[i].cmp, hdr.version);


    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "PA_COMPILE_CONSTANTS", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:

    /* keep marker at end: */
    return rc;
}

#define PERSISTENT_DATA_MAGIC   0x12213443
#define PERSISTENT_DATA_VERSION 4

static UINT16
PERSISTENT_DATA_Marshal(PERSISTENT_DATA *data, BYTE **buffer, INT32 *size)
{
    UINT16 written;
    UINT16 array_size;
    UINT8 clocksize;
    BOOL has_block;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                PERSISTENT_DATA_VERSION,
                                PERSISTENT_DATA_MAGIC, 4);
    written += BOOL_Marshal(&data->disableClear, buffer, size);
    written += TPM_ALG_ID_Marshal(&data->ownerAlg, buffer, size);
    written += TPM_ALG_ID_Marshal(&data->endorsementAlg, buffer, size);
    written += TPM_ALG_ID_Marshal(&data->lockoutAlg, buffer, size);
    written += TPM2B_DIGEST_Marshal(&data->ownerPolicy, buffer, size);
    written += TPM2B_DIGEST_Marshal(&data->endorsementPolicy, buffer, size);
    written += TPM2B_DIGEST_Marshal(&data->lockoutPolicy, buffer, size);
    written += TPM2B_AUTH_Marshal(&data->ownerAuth, buffer, size);
    written += TPM2B_AUTH_Marshal(&data->endorsementAuth, buffer, size);
    written += TPM2B_AUTH_Marshal(&data->lockoutAuth, buffer, size);
    written += TPM2B_Marshal(&data->EPSeed.b, sizeof(data->EPSeed.t.buffer), buffer, size);
    written += TPM2B_Marshal(&data->SPSeed.b, sizeof(data->SPSeed.t.buffer), buffer, size);
    written += TPM2B_Marshal(&data->PPSeed.b, sizeof(data->PPSeed.t.buffer), buffer, size);
    written += TPM2B_PROOF_Marshal(&data->phProof, buffer, size);
    written += TPM2B_PROOF_Marshal(&data->shProof, buffer, size);
    written += TPM2B_PROOF_Marshal(&data->ehProof, buffer, size);
    written += UINT64_Marshal(&data->totalResetCount, buffer, size);
    written += UINT32_Marshal(&data->resetCount, buffer, size);

#if defined NUM_POLICY_PCR_GROUP && NUM_POLICY_PCR_GROUP > 0
    has_block = TRUE;
#else
    has_block = FALSE;
#endif
    written += BLOCK_SKIP_WRITE_PUSH(has_block, buffer, size);

#if defined NUM_POLICY_PCR_GROUP && NUM_POLICY_PCR_GROUP > 0
    written += PCR_POLICY_Marshal(&data->pcrPolicies, buffer, size);
#endif
    BLOCK_SKIP_WRITE_POP(size);

    written += TPML_PCR_SELECTION_Marshal(&data->pcrAllocated, buffer, size);

    /* ppList may grow */
    array_size = sizeof(data->ppList);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal(&data->ppList[0], array_size, buffer, size);

    written += UINT32_Marshal(&data->failedTries, buffer, size);
    written += UINT32_Marshal(&data->maxTries, buffer, size);
    written += UINT32_Marshal(&data->recoveryTime, buffer, size);
    written += UINT32_Marshal(&data->lockoutRecovery, buffer, size);
    written += BOOL_Marshal(&data->lockOutAuthEnabled, buffer, size);
    written += UINT16_Marshal(&data->orderlyState, buffer, size);

    /* auditCommands may grow */
    array_size = sizeof(data->auditCommands);
    written += UINT16_Marshal(&array_size, buffer, size);
    written += Array_Marshal(&data->auditCommands[0], array_size,
                             buffer, size);

    written += TPM_ALG_ID_Marshal(&data->auditHashAlg, buffer, size);
    written += UINT64_Marshal(&data->auditCounter, buffer, size);
    written += UINT32_Marshal(&data->algorithmSet, buffer, size);
    written += UINT32_Marshal(&data->firmwareV1, buffer, size);
    written += UINT32_Marshal(&data->firmwareV2, buffer, size);
#if CLOCK_STOPS
    clocksize = sizeof(UINT64);
    written += UINT8_Marshal(&clocksize, buffer, size);
    written += UINT64_Marshal(&data->timeEpoch, buffer, size);
#else
    clocksize = sizeof(UINT32);
    written += UINT8_Marshal(&clocksize, buffer, size);
    written += UINT32_Marshal(&data->timeEpoch, buffer, size);
#endif

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);

    /* there's a 'shadow' pcrAllocated as well */
    written += TPML_PCR_SELECTION_Marshal(&gp.pcrAllocated, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    written += SEED_COMPAT_LEVEL_Marshal(&data->EPSeedCompatLevel,
                                         buffer, size);
    written += SEED_COMPAT_LEVEL_Marshal(&data->SPSeedCompatLevel,
                                         buffer, size);
    written += SEED_COMPAT_LEVEL_Marshal(&data->PPSeedCompatLevel,
                                         buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);
    BLOCK_SKIP_WRITE_POP(size);
    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
PERSISTENT_DATA_Unmarshal(PERSISTENT_DATA *data, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;
    UINT16 array_size;
    UINT8 clocksize;
    BOOL needs_block;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 PERSISTENT_DATA_VERSION,
                                 PERSISTENT_DATA_MAGIC);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&data->disableClear, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM_ALG_ID_Unmarshal(&data->ownerAlg, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM_ALG_ID_Unmarshal(&data->endorsementAlg, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM_ALG_ID_Unmarshal(&data->lockoutAlg, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_DIGEST_Unmarshal(&data->ownerPolicy, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_DIGEST_Unmarshal(&data->endorsementPolicy, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_DIGEST_Unmarshal(&data->lockoutPolicy, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_AUTH_Unmarshal(&data->ownerAuth, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_AUTH_Unmarshal(&data->endorsementAuth, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_AUTH_Unmarshal(&data->lockoutAuth, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_Unmarshal(&data->EPSeed.b, PRIMARY_SEED_SIZE, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_Unmarshal(&data->SPSeed.b, PRIMARY_SEED_SIZE, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_Unmarshal(&data->PPSeed.b, PRIMARY_SEED_SIZE, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_PROOF_Unmarshal(&data->phProof, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_PROOF_Unmarshal(&data->shProof, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = TPM2B_PROOF_Unmarshal(&data->ehProof, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&data->totalResetCount, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->resetCount, buffer, size);
    }

#if defined NUM_POLICY_PCR_GROUP && NUM_POLICY_PCR_GROUP > 0
    needs_block = TRUE;
#else
    needs_block = FALSE;
#endif
    if (rc == TPM_RC_SUCCESS) {
        BLOCK_SKIP_READ(skip_num_policy_pcr_group, needs_block, buffer, size,
                        "PERSISTENT_DATA", "pcrPolicies");
    }
#if defined NUM_POLICY_PCR_GROUP && NUM_POLICY_PCR_GROUP > 0
    if (rc == TPM_RC_SUCCESS) {
        rc = PCR_POLICY_Unmarshal(&data->pcrPolicies, buffer, size);
    }
#endif
skip_num_policy_pcr_group:

    if (rc == TPM_RC_SUCCESS) {
        rc = TPML_PCR_SELECTION_Unmarshal(&data->pcrAllocated, buffer, size);

        shadow.pcrAllocated = data->pcrAllocated;
        shadow.pcrAllocatedIsNew = TRUE;
    }

    /* ppList array may not be our size */
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
#ifndef VBOX
        BYTE buf[array_size];
        rc = Array_Unmarshal(buf, array_size, buffer, size);
        memcpy(data->ppList, buf, MIN(array_size, sizeof(data->ppList)));
#else
        BYTE *pBuf = (BYTE *)RTMemTmpAlloc(array_size);
        if (RT_LIKELY(pBuf))
        {
            rc = Array_Unmarshal(pBuf, array_size, buffer, size);
            memcpy(data->ppList, pBuf, MIN(array_size, sizeof(data->ppList)));
            RTMemTmpFree(pBuf);
        }
        else
            rc = TPM_RC_SIZE;
#endif
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->failedTries, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->maxTries, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->recoveryTime, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->lockoutRecovery, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = BOOL_Unmarshal(&data->lockOutAuthEnabled, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        /* TPM_SU_Unmarshal returns error if value is 0 */
        rc = UINT16_Unmarshal(&data->orderlyState, buffer, size);
    }

    /* auditCommands array may not be our size */
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT16_Unmarshal(&array_size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
#ifndef VBOX
        BYTE buf[array_size];
        rc = Array_Unmarshal(buf, array_size, buffer, size);
        memcpy(data->auditCommands, buf,
               MIN(array_size, sizeof(data->auditCommands)));
#else
        BYTE *pBuf = (BYTE *)RTMemTmpAlloc(array_size);
        if (RT_LIKELY(pBuf))
        {
            rc = Array_Unmarshal(pBuf, array_size, buffer, size);
            memcpy(data->auditCommands, pBuf, MIN(array_size, sizeof(data->auditCommands)));
            RTMemTmpFree(pBuf);
        }
        else
            rc = TPM_RC_SIZE;
#endif
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = TPM_ALG_ID_Unmarshal(&data->auditHashAlg, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&data->auditCounter, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->algorithmSet, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->firmwareV1, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal(&data->firmwareV2, buffer, size);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = UINT8_Unmarshal(&clocksize, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
#if CLOCK_STOPS
        if (clocksize != sizeof(UINT64)) {
            TPMLIB_LogTPM2Error("Unexpected clocksize for epoch; "
                                "Expected %u, got %u\n",
                                sizeof(UINT64), clocksize);
            rc = TPM_RC_BAD_PARAMETER;
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = UINT64_Unmarshal(&data->timeEpoch, buffer, size);
        }
#else
        if (clocksize != sizeof(UINT32)) {
            TPMLIB_LogTPM2Error("Unexpected clocksize for epoch; "
                                "Expected %zu, got %u\n",
                                sizeof(UINT32), clocksize);
            rc = TPM_RC_BAD_PARAMETER;
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = UINT32_Unmarshal(&data->timeEpoch, buffer, size);
        }
#endif
    }

    /* default values before conditional block */
    data->EPSeedCompatLevel = SEED_COMPAT_LEVEL_ORIGINAL;
    data->SPSeedCompatLevel = SEED_COMPAT_LEVEL_ORIGINAL;
    data->PPSeedCompatLevel = SEED_COMPAT_LEVEL_ORIGINAL;

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, hdr.version >= 3, buffer, size,
                        "PERSISTENT_DATA", "version 3 or later");
        if (rc == TPM_RC_SUCCESS) {
            rc = TPML_PCR_SELECTION_Unmarshal(&shadow.pcrAllocated, buffer, size);
        }

        if (rc == TPM_RC_SUCCESS) {
            BLOCK_SKIP_READ(skip_future_versions, hdr.version >= 4, buffer, size,
                            "PERSISTENT_DATA", "version 4 or later");
        }

        if (rc == TPM_RC_SUCCESS) {
            rc = SEED_COMPAT_LEVEL_Unmarshal(&data->EPSeedCompatLevel,
                                             buffer, size, "EPSeed");
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = SEED_COMPAT_LEVEL_Unmarshal(&data->SPSeedCompatLevel,
                                             buffer, size, "SPSeed");
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = SEED_COMPAT_LEVEL_Unmarshal(&data->PPSeedCompatLevel,
                                             buffer, size, "PPSeed");
        }

        if (rc == TPM_RC_SUCCESS) {
            BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                            "PERSISTENT_DATA", "version 5 or later");
        }
        /* future versions nest-append here */
    }

skip_future_versions:

    if (rc != TPM_RC_SUCCESS) {
        TPMLIB_LogTPM2Error("Failed to unmarshal PERSISTENT_DATA version %u\n",
                            hdr.version);
    }
    return rc;
}

#define INDEX_ORDERLY_RAM_VERSION 2
#define INDEX_ORDERLY_RAM_MAGIC   0x5346feab
static UINT32
INDEX_ORDERLY_RAM_Marshal(void *array, size_t array_size,
                          BYTE **buffer, INT32 *size)
{
    UINT16 written;
    NV_RAM_HEADER nrh, *nrhp;
    UINT16 offset = 0;
    UINT16 datasize;
    UINT32 sourceside_size = array_size;
    BLOCK_SKIP_INIT;

    written = NV_HEADER_Marshal(buffer, size,
                                INDEX_ORDERLY_RAM_VERSION,
                                INDEX_ORDERLY_RAM_MAGIC, 1);

    /* the size of the array we are using here */
    written += UINT32_Marshal(&sourceside_size, buffer, size);

    while (TRUE) {
#ifndef VBOX
        nrhp = array + offset;
#else
        nrhp = (NV_RAM_HEADER *)((uint8_t *)array + offset);
#endif
        /* nrhp may point to misaligned address (ubsan), so use 'nrh'; first access only 'size' */
        memcpy(&nrh, nrhp, sizeof(nrh.size));

        /* write the NVRAM header;
           nrh->size holds the complete size including data;
           nrh->size = 0 indicates the end */
        written += UINT32_Marshal(&nrh.size, buffer, size);
        if (nrh.size == 0)
            break;
        /* copy the entire structure now; ubsan does not allow 'nrh = *nrhp' */
        memcpy(&nrh, nrhp, sizeof(nrh));

        written += TPM_HANDLE_Marshal(&nrh.handle, buffer, size);
        written += TPMA_NV_Marshal(&nrh.attributes, buffer, size);

        if (offset + nrh.size > array_size) {
            TPMLIB_LogTPM2Error("INDEX_ORDERLY_RAM: nrh->size corrupted: %d\n",
                                nrh.size);
            break;
        }
        /* write data size before array */
        if (nrh.size < sizeof(NV_RAM_HEADER)) {
            TPMLIB_LogTPM2Error(
                "INDEX_ORDERLY_RAM: nrh->size < sizeof(NV_RAM_HEADER): %d < %zu\n",
                (int)nrh.size, sizeof(NV_RAM_HEADER));
            break;
        }
        datasize = nrh.size - sizeof(NV_RAM_HEADER);
        written += UINT16_Marshal(&datasize, buffer, size);
        if (datasize > 0) {
            /* append the data */
#ifndef VBOX
            written += Array_Marshal(array + offset + sizeof(NV_RAM_HEADER),
                                     datasize, buffer, size);
#else
            written += Array_Marshal((uint8_t *)array + offset + sizeof(NV_RAM_HEADER),
                                     datasize, buffer, size);
#endif
        }
        offset += nrh.size;
        if (offset + sizeof(NV_RAM_HEADER) > array_size) {
            /* nothing will fit anymore and there won't be a 0-sized
             * terminating node (@1).
             */
            break;
        }
    }

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

static TPM_RC
INDEX_ORDERLY_RAM_Unmarshal(void *array, size_t array_size,
                            BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;
    NV_RAM_HEADER nrh, *nrhp;
    UINT16 offset = 0;
    UINT16 datasize = 0;
    UINT32 sourceside_size;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 INDEX_ORDERLY_RAM_VERSION,
                                 INDEX_ORDERLY_RAM_MAGIC);
    }
    if (rc == TPM_RC_SUCCESS) {
        /* get the size of the array on the source side
           we can accommodate different sizes when rebuilding
           but if it doesn't fit we'll error out and report the sizes */
        rc = UINT32_Unmarshal(&sourceside_size, buffer, size);
    }

    while (rc == TPM_RC_SUCCESS) {
        memset(&nrh, 0, sizeof(nrh)); /* coverity */
        /* nrhp may point to misaligned address (ubsan)
         * we read 'into' nrh and copy to nrhp at end
         */
#ifndef VBOX
        nrhp = array + offset;
#else
        nrhp = (NV_RAM_HEADER *)((uint8_t *)array + offset);
#endif

        if (offset + sizeof(NV_RAM_HEADER) > sourceside_size) {
            /* this case can occur with the previous entry filling up the
             * space; in this case there will not be a 0-sized terminating
             * node (see @1 above). We clear the rest of our space.
             */
            if (array_size > offset)
                memset(nrhp, 0, array_size - offset);
            break;
        }

        /* write the NVRAM header;
           nrh->size holds the complete size including data;
           nrh->size = 0 indicates the end */
        if (offset + sizeof(nrh.size) > array_size) {
            offset += sizeof(nrh.size);
            goto exit_size;
        }

        if (rc == TPM_RC_SUCCESS) {
            rc = UINT32_Unmarshal(&nrh.size, buffer, size);
            if (rc == TPM_RC_SUCCESS && nrh.size == 0) {
                memcpy(nrhp, &nrh, sizeof(nrh.size));
                break;
            }
        }
        if (offset + sizeof(NV_RAM_HEADER) > array_size) {
            offset += sizeof(NV_RAM_HEADER);
            goto exit_size;
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = TPM_HANDLE_Unmarshal(&nrh.handle, buffer, size);
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = TPMA_NV_Unmarshal(&nrh.attributes, buffer, size);
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = UINT16_Unmarshal(&datasize, buffer, size);
        }
        if (offset + sizeof(NV_RAM_HEADER) + datasize > array_size) {
            offset += sizeof(NV_RAM_HEADER) + datasize;
            goto exit_size;
        }
        if (rc == TPM_RC_SUCCESS && datasize > 0) {
            /* append the data */
#ifndef VBOX
            rc = Array_Unmarshal(array + offset + sizeof(NV_RAM_HEADER),
                                 datasize, buffer, size);
#else
            rc = Array_Unmarshal((uint8_t *)array + offset + sizeof(NV_RAM_HEADER),
                                 datasize, buffer, size);
#endif
        }
        if (rc == TPM_RC_SUCCESS) {
            /* fix up size in case it is architecture-dependent */
            nrh.size = sizeof(nrh) + datasize;
            offset += nrh.size;
            /* copy header into possibly misaligned address in NVRAM */
            *nrhp = nrh;
        }
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "INDEX_ORDERLY_RAM", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:
    return rc;

exit_size:
    TPMLIB_LogTPM2Error("INDEX_ORDERLY_RAM:"
                        "Insufficient space to write to offset %u;"
                        "Source had %u bytes, we have %zu bytes.\n",
                        offset, sourceside_size, array_size);
    return TPM_RC_SIZE;
}

static void
USER_NVRAM_Display(const char *msg)
{
    NV_REF entryRef = NV_USER_DYNAMIC;
    UINT32 entrysize;
    UINT64 offset = 0;
    TPM_HANDLE handle;
    UINT32 datasize;
    NV_INDEX nvi;
    OBJECT obj;
    UINT64 maxCount;

    fprintf(stderr, "USER_NVRAM contents %s:\n", msg);

    while (TRUE) {
        /* 1st: entrysize */
        NvRead(&entrysize, entryRef, sizeof(entrysize));
        fprintf(stderr, " offset: %5"PRIu32"   entry size: %5u ",
                (UINT32)(entryRef - NV_USER_DYNAMIC), entrysize);
        offset = sizeof(UINT32);

        if (entrysize == 0)
            break;

        /* 2nd: the handle -- it will tell us what datatype this is */
        NvRead(&handle, entryRef + offset, sizeof(handle));
        fprintf(stderr, "handle: 0x%08x ", handle);

        switch (HandleGetType(handle)) {
        case TPM_HT_NV_INDEX:
            fprintf(stderr, " (NV_INDEX)  ");
            /* NV_INDEX has the index again at offset 0! */
            NvReadNvIndexInfo(entryRef + offset, &nvi);
            offset += sizeof(nvi);
            datasize = entrysize - sizeof(UINT32) - sizeof(nvi);
            fprintf(stderr, " datasize: %u\n",datasize);
            break;
        break;
        case TPM_HT_PERSISTENT:
            fprintf(stderr, " (PERSISTENT)");
            offset += sizeof(handle);

            NvRead(&obj, entryRef + offset, sizeof(obj));
            offset += sizeof(obj);
            fprintf(stderr, " sizeof(obj): %zu\n", sizeof(obj));
        break;
        default:
            TPMLIB_LogTPM2Error("USER_NVRAM: Corrupted handle: %08x\n", handle);
        }
        /* advance to next entry */
        entryRef += entrysize;
    }
    fprintf(stderr, "\n");

    NvRead(&maxCount, entryRef + offset, sizeof(maxCount));
    fprintf(stderr, " maxCount:   %"PRIu64"\n", maxCount);
    fprintf(stderr, "-----------------------------\n");
}

#define USER_NVRAM_VERSION 2
#define USER_NVRAM_MAGIC   0x094f22c3
static UINT32
USER_NVRAM_Marshal(BYTE **buffer, INT32 *size)
{
    UINT32 written;
    UINT32 entrysize;
    UINT64 offset;
    NV_REF entryRef = NV_USER_DYNAMIC;
    NV_INDEX nvi;
    UINT64 maxCount;
    TPM_HANDLE handle;
    OBJECT obj;
    UINT32 datasize;
    UINT64 sourceside_size = NV_USER_DYNAMIC_END - NV_USER_DYNAMIC;
    BLOCK_SKIP_INIT;

    if (FALSE)
        USER_NVRAM_Display("before marshalling");

    written = NV_HEADER_Marshal(buffer, size,
                                USER_NVRAM_VERSION, USER_NVRAM_MAGIC,
                                1);

    written += UINT64_Marshal(&sourceside_size, buffer, size);

    while (TRUE) {
        /* 1st: entrysize */
        NvRead(&entrysize, entryRef, sizeof(entrysize));
        offset = sizeof(UINT32);

        /* entrysize is in native format now */
        written += UINT32_Marshal(&entrysize, buffer, size);
        if (entrysize == 0)
            break;

        /* 2nd: the handle -- it will tell us what datatype this is */
        NvRead(&handle, entryRef + offset, sizeof(handle));
        written += TPM_HANDLE_Marshal(&handle, buffer, size);

        switch (HandleGetType(handle)) {
        case TPM_HT_NV_INDEX:
            /* NV_INDEX has the index again at offset 0! */
            NvReadNvIndexInfo(entryRef + offset, &nvi);
            offset += sizeof(nvi);

            written += NV_INDEX_Marshal(&nvi, buffer, size);
            /* after that: bulk data */
            datasize = entrysize - sizeof(UINT32) - sizeof(nvi);
            written += UINT32_Marshal(&datasize, buffer, size);
            if (datasize > 0) {
#ifndef VBOX
                BYTE buf[datasize];
                NvRead(buf, entryRef + offset, datasize);
                written += Array_Marshal(buf, datasize, buffer, size);
#else
                BYTE *pBuf = (BYTE *)RTMemTmpAlloc(datasize);
                if (RT_LIKELY(pBuf))
                {
                    NvRead(pBuf, entryRef + offset, datasize);
                    written += Array_Marshal(pBuf, datasize, buffer, size);
                    RTMemTmpFree(pBuf);
                }
#endif
            }
        break;
        case TPM_HT_PERSISTENT:
            offset += sizeof(handle);

            NvRead(&obj, entryRef + offset, sizeof(obj));
            offset += sizeof(obj);
            written += ANY_OBJECT_Marshal(&obj, buffer, size);
        break;
        default:
            TPMLIB_LogTPM2Error("USER_NVRAM: Corrupted handle: %08x\n", handle);
        }
        /* advance to next entry */
        entryRef += entrysize;
    }
    NvRead(&maxCount, entryRef + offset, sizeof(maxCount));
    written += UINT64_Marshal(&maxCount, buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

/*
 * USER_NVRAM_Unmarshal:
 *
 * Unmarshal the byte stream directly into the NVRAM. Ensure that the
 * the data fit into the user NVRAM before writing them.
 *
 * This function fails if there's not enough NVRAM to write the data into
 * or if an unknown handle type was encountered.
 */
static TPM_RC
USER_NVRAM_Unmarshal(BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;
    NV_REF entryRef = NV_USER_DYNAMIC;
    UINT32 entrysize;
    UINT64 offset, o = 0;
    NV_INDEX nvi;
    UINT64 maxCount;
    TPM_HANDLE handle;
    OBJECT obj;
    UINT32 datasize = 0;
    UINT64 sourceside_size;
    UINT64 array_size = NV_USER_DYNAMIC_END - NV_USER_DYNAMIC;
    UINT64 entrysize_offset;

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 USER_NVRAM_VERSION,
                                 USER_NVRAM_MAGIC);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&sourceside_size, buffer, size);
    }

    while (rc == TPM_RC_SUCCESS) {
        /* 1st: entrysize */
        if (o + sizeof(UINT32) > array_size) {
            o += sizeof(UINT32);
            goto exit_size;
        }
        if (rc == TPM_RC_SUCCESS) {
            rc = UINT32_Unmarshal(&entrysize, buffer, size);

            /* the entrysize also depends on the sizeof(nvi); we may have to
               update it if sizeof(nvi) changed between versions */
            entrysize_offset = o;
            NvWrite(entryRef + o, sizeof(entrysize), &entrysize);
            offset = sizeof(UINT32);
            if (entrysize == 0)
                break;
        }
        /* 2nd: handle */
        if (rc == TPM_RC_SUCCESS) {
            rc = TPM_HANDLE_Unmarshal(&handle, buffer, size);
        }

        if (rc == TPM_RC_SUCCESS) {
            switch (HandleGetType(handle)) {
            case TPM_HT_NV_INDEX:
                /* we need to read the handle again */
                if (rc == TPM_RC_SUCCESS &&
                    o + offset + sizeof(nvi) > array_size) {
                     o += offset + sizeof(nvi);
                     goto exit_size;
                }
                if (rc == TPM_RC_SUCCESS) {
                    rc = NV_INDEX_Unmarshal(&nvi, buffer, size);
                    NvWrite(entryRef + o + offset, sizeof(nvi), &nvi);
                    offset += sizeof(nvi);
                }
                if (rc == TPM_RC_SUCCESS) {
                    rc = UINT32_Unmarshal(&datasize, buffer, size);
                }
                if (rc == TPM_RC_SUCCESS) {
                    /* datasize cannot exceed 64k + a few bytes */
                    if (datasize > (0x10000 + 0x100)) {
                        TPMLIB_LogTPM2Error("datasize for NV_INDEX too "
                                            "large: %u\n", datasize);
                        rc = TPM_RC_SIZE;
                    }
                }
                if (rc == TPM_RC_SUCCESS &&
                    o + offset + datasize > array_size) {
                    o += offset + datasize;
                    goto exit_size;
                }
                if (rc == TPM_RC_SUCCESS && datasize > 0) {
#ifndef VBOX
                    BYTE buf[datasize];
                    rc = Array_Unmarshal(buf, datasize, buffer, size);
                    NvWrite(entryRef + o + offset, datasize, buf);
#else
                    BYTE *pBuf = (BYTE *)RTMemTmpAlloc(datasize);
                    if (RT_LIKELY(pBuf))
                    {
                        rc = Array_Unmarshal(pBuf, datasize, buffer, size);
                        NvWrite(entryRef + o + offset, datasize, pBuf);
                        RTMemTmpFree(pBuf);
                    }
                    else
                        rc = TPM_RC_SIZE;
#endif
                    offset += datasize;

                    /* update the entry size; account for expanding nvi */
                    entrysize = sizeof(UINT32) + sizeof(nvi) + datasize;
                }
            break;
            case TPM_HT_PERSISTENT:
                if (rc == TPM_RC_SUCCESS &&
                    o + offset + sizeof(TPM_HANDLE) + sizeof(obj) >
                      array_size) {
                    o += offset + sizeof(TPM_HANDLE) + sizeof(obj);
                    goto exit_size;
                }

                if (rc == TPM_RC_SUCCESS) {
                    NvWrite(entryRef + o + offset, sizeof(handle), &handle);
                    offset += sizeof(TPM_HANDLE);

                    memset(&obj, 0, sizeof(obj));
                    rc = ANY_OBJECT_Unmarshal(&obj, buffer, size, true);
                    NvWrite(entryRef + o + offset, sizeof(obj), &obj);
                    offset += sizeof(obj);
                }
                entrysize = sizeof(UINT32) + sizeof(TPM_HANDLE) + sizeof(obj);
            break;
            default:
                TPMLIB_LogTPM2Error("USER_NVRAM: "
                                    "Read handle 0x%08x of unknown type\n",
                                    handle);
                rc = TPM_RC_HANDLE;
            }

            if (rc == TPM_RC_SUCCESS) {
                NvWrite(entryRef + entrysize_offset, sizeof(entrysize), &entrysize);
            }
        }
        if (rc == TPM_RC_SUCCESS) {
            o += offset;
        }
    }
    if (rc == TPM_RC_SUCCESS &&
        o + offset + sizeof(UINT64) > array_size) {
        o += offset + sizeof(UINT64);
        goto exit_size;
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT64_Unmarshal(&maxCount, buffer, size);
        NvWrite(entryRef + o + offset, sizeof(maxCount), &maxCount);
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "USER_NVRAM", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:

    if (FALSE)
        USER_NVRAM_Display("after unmarshalling");

    return rc;

exit_size:
    TPMLIB_LogTPM2Error("USER_NVRAM:"
                        "Insufficient space to write to offset %"PRIu64";"
                        "Source had %"PRIu64" bytes, we have %"PRIu64" "
                        "bytes.\n",
                        o, sourceside_size, array_size);
    return TPM_RC_SIZE;
}

/*
 * Write out all persistent data by reading them from the NVRAM
 * and then writing them out.
 *
 * - PERSISTENT_DATA  (NV_PERSISTENT_DATA)
 * - ORDERLY_DATA     (NV_STATE_RESET_DATA)
 * - STATE_RESET_DATA (NV_STATE_RESET_DATA)
 * - STATE_CLEAR_DATA (NV_STATE_CLEAR_DATA)
 * - indexOrderlyRAM  (NV_INDEX_RAM_DATA)
 * - NVRAM locations  (NV_USER_DYNAMIC)
 */
#define PERSISTENT_ALL_VERSION 3
#define PERSISTENT_ALL_MAGIC   0xab364723
UINT32
PERSISTENT_ALL_Marshal(BYTE **buffer, INT32 *size)
{
    UINT32 magic;
    PERSISTENT_DATA pd;
    ORDERLY_DATA od;
    STATE_RESET_DATA srd;
    STATE_CLEAR_DATA scd;
    UINT32 written = 0;
    BYTE indexOrderlyRam[sizeof(s_indexOrderlyRam)];
    BLOCK_SKIP_INIT;
    BOOL writeSuState;

    NvRead(&pd, NV_PERSISTENT_DATA, sizeof(pd));
    NvRead(&od, NV_ORDERLY_DATA, sizeof(od));
    NvRead(&srd, NV_STATE_RESET_DATA, sizeof(srd));
    NvRead(&scd, NV_STATE_CLEAR_DATA, sizeof(scd));

    /* indexOrderlyRam was never endianess-converted; so it's native */
    NvRead(indexOrderlyRam, NV_INDEX_RAM_DATA, sizeof(indexOrderlyRam));

    written = NV_HEADER_Marshal(buffer, size,
                                PERSISTENT_ALL_VERSION,
                                PERSISTENT_ALL_MAGIC, 3);
    written += PACompileConstants_Marshal(buffer, size);
    written += PERSISTENT_DATA_Marshal(&pd, buffer, size);
    written += ORDERLY_DATA_Marshal(&od, buffer, size);
    writeSuState = (pd.orderlyState & TPM_SU_STATE_MASK) == TPM_SU_STATE;
    /* starting with v3 we only write STATE_RESET and STATE_CLEAR if needed */
    if (writeSuState) {
        written += STATE_RESET_DATA_Marshal(&srd, buffer, size);
        written += STATE_CLEAR_DATA_Marshal(&scd, buffer, size);
    }
    written += INDEX_ORDERLY_RAM_Marshal(indexOrderlyRam, sizeof(indexOrderlyRam),
                                         buffer, size);
    written += USER_NVRAM_Marshal(buffer, size);

    written += BLOCK_SKIP_WRITE_PUSH(TRUE, buffer, size);
    /* future versions append below this line */

    BLOCK_SKIP_WRITE_POP(size);

    magic = PERSISTENT_ALL_MAGIC;
    written += UINT32_Marshal(&magic, buffer, size);

    BLOCK_SKIP_WRITE_CHECK;

    return written;
}

TPM_RC
PERSISTENT_ALL_Unmarshal(BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    NV_HEADER hdr;
    PERSISTENT_DATA pd;
    ORDERLY_DATA od;
    STATE_RESET_DATA srd;
    STATE_CLEAR_DATA scd;
    BYTE indexOrderlyRam[sizeof(s_indexOrderlyRam)];
    BOOL readSuState = false;

    memset(&pd, 0, sizeof(pd));
    memset(&od, 0, sizeof(od));
    memset(&srd, 0, sizeof(srd));
    memset(&scd, 0, sizeof(scd));
    memset(indexOrderlyRam, 0, sizeof(indexOrderlyRam));

    if (rc == TPM_RC_SUCCESS) {
        rc = NV_HEADER_Unmarshal(&hdr, buffer, size,
                                 PERSISTENT_ALL_VERSION,
                                 PERSISTENT_ALL_MAGIC);
    }

    if (rc == TPM_RC_SUCCESS) {
        rc = PACompileConstants_Unmarshal(buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = PERSISTENT_DATA_Unmarshal(&pd, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        if (hdr.version < 3) {
            /* STATE_RESET and STATE_CLEAR were always written before version 3 */
            readSuState = true;
        } else {
            readSuState = (pd.orderlyState & TPM_SU_STATE_MASK) == TPM_SU_STATE;
        }
        rc = ORDERLY_DATA_Unmarshal(&od, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS && readSuState) {
        rc = STATE_RESET_DATA_Unmarshal(&srd, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS && readSuState) {
        rc = STATE_CLEAR_DATA_Unmarshal(&scd, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        rc = INDEX_ORDERLY_RAM_Unmarshal(indexOrderlyRam, sizeof(indexOrderlyRam),
                                         buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
        /* this will write it into NVRAM right away */
        rc = USER_NVRAM_Unmarshal(buffer, size);
        /* if rc == TPM_RC_SUCCESS, we know that there is enough
           NVRAM to fit everything. */
    }

    /* version 2 starts having indicator for next versions that we can skip;
       this allows us to downgrade state */
    if (rc == TPM_RC_SUCCESS && hdr.version >= 2) {
        BLOCK_SKIP_READ(skip_future_versions, FALSE, buffer, size,
                        "USER NVRAM", "version 3 or later");
        /* future versions nest-append here */
    }

skip_future_versions:
    if (rc == TPM_RC_SUCCESS) {
        rc = UINT32_Unmarshal_Check(&hdr.magic,
                               PERSISTENT_ALL_MAGIC, buffer, size,
                               "PERSISTENT_ALL_MAGIC after USER_NVRAM");
    }

    if (rc == TPM_RC_SUCCESS) {
        NvWrite(NV_PERSISTENT_DATA, sizeof(pd), &pd);
        NvWrite(NV_ORDERLY_DATA, sizeof(od), &od);
        NvWrite(NV_STATE_RESET_DATA, sizeof(srd), &srd);
        NvWrite(NV_STATE_CLEAR_DATA, sizeof(scd), &scd);
        NvWrite(NV_INDEX_RAM_DATA, sizeof(indexOrderlyRam), indexOrderlyRam);
    }

    return rc;
}

void
NVShadowRestore(void)
{
    if (shadow.pcrAllocatedIsNew) {
        gp.pcrAllocated = shadow.pcrAllocated;
        shadow.pcrAllocatedIsNew = FALSE;
    }
}
