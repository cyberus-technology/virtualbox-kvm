/* $Id: tpm.h $ */
/** @file
 * IPRT, TPM common definitions (this is actually a protocol and not a format).
 */

/*
 * Copyright (C) 2021-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_formats_tpm_h
#define IPRT_INCLUDED_formats_tpm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/asm.h>
#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/assertcompile.h>
#include <iprt/string.h>


/**
 * TPM request header (everything big endian).
 */
#pragma pack(1)
typedef struct TPMREQHDR
{
    /** The tag for this request. */
    uint16_t            u16Tag;
    /** Size of the request in bytes. */
    uint32_t            cbReq;
    /** The request ordinal to execute. */
    uint32_t            u32Ordinal;
} TPMREQHDR;
#pragma pack()
AssertCompileSize(TPMREQHDR, 2 + 4 + 4);
/** Pointer to a TPM request header. */
typedef TPMREQHDR *PTPMREQHDR;
/** Pointer to a const TPM request header. */
typedef const TPMREQHDR *PCTPMREQHDR;


/** @name TPM 1.2 request tags
 * @{ */
/** Command with no authentication. */
#define TPM_TAG_RQU_COMMAND                 UINT16_C(0x00c1)
/** An authenticated command with one authentication handle. */
#define TPM_TAG_RQU_AUTH1_COMMAND           UINT16_C(0x00c2)
/** An authenticated command with two authentication handles. */
#define TPM_TAG_RQU_AUTH2_COMMAND           UINT16_C(0x00c3)
/** @} */


/** @name TPM 2.0 request/response tags
 * @{ */
/** Command with no associated session. */
#define TPM2_ST_NO_SESSIONS                 UINT16_C(0x8001)
/** Command with an associated session. */
#define TPM2_ST_SESSIONS                    UINT16_C(0x8002)
/** @} */


/** @name TPM 1.2 request ordinals.
 * @{ */
/** Perform a full self test. */
#define TPM_ORD_SELFTESTFULL                UINT32_C(80)
/** Continue the selftest. */
#define TPM_ORD_CONTINUESELFTEST            UINT32_C(83)
/** Return the test result. */
#define TPM_ORD_GETTESTRESULT               UINT32_C(84)
/** Get a capability. */
#define TPM_ORD_GETCAPABILITY               UINT32_C(101)
/** @} */


/** @name TPM 2.0 command codes.
 * @{ */
/** Get a capability. */
#define TPM2_CC_GET_CAPABILITY              UINT32_C(378)
/** @} */


/** @name Defines related to TPM_ORD_GETCAPABILITY.
 * @{ */
/** Return a TPM related property. */
#define TPM_CAP_PROPERTY                    UINT32_C(5)

/** Returns the size of the input buffer. */
#define TPM_CAP_PROP_INPUT_BUFFER           UINT32_C(0x124)

/**
 * TPM_ORD_GETCAPABILITY request.
 */
#pragma pack(1)
typedef struct TPMREQGETCAPABILITY
{
    /** Request header. */
    TPMREQHDR                   Hdr;
    /** The capability group to query. */
    uint32_t                    u32Cap;
    /** Length of the capability. */
    uint32_t                    u32Length;
    /** The sub capability to query. */
    uint32_t                    u32SubCap;
} TPMREQGETCAPABILITY;
#pragma pack()
/** Pointer to a TPM_ORD_GETCAPABILITY request. */
typedef TPMREQGETCAPABILITY *PTPMREQGETCAPABILITY;
/** Pointer to a const TPM_ORD_GETCAPABILITY request. */
typedef const TPMREQGETCAPABILITY *PCTPMREQGETCAPABILITY;
/** @} */


/** @name Defines related to TPM2_CC_GET_CAPABILITY.
 * @{ */
/** Return a TPM related property. */
#define TPM2_CAP_TPM_PROPERTIES             UINT32_C(6)

/** Returns the size of the input buffer. */
#define TPM2_PT_INPUT_BUFFER                UINT32_C(0x10d)

/**
 * TPM2_CC_GET_CAPABILITY request.
 */
#pragma pack(1)
typedef struct TPM2REQGETCAPABILITY
{
    /** Request header. */
    TPMREQHDR                   Hdr;
    /** The capability group to query. */
    uint32_t                    u32Cap;
    /** Property to query. */
    uint32_t                    u32Property;
    /** Number of values to return. */
    uint32_t                    u32Count;
} TPM2REQGETCAPABILITY;
#pragma pack()
/** Pointer to a TPM2_CC_GET_CAPABILITY request. */
typedef TPM2REQGETCAPABILITY *PTPM2REQGETCAPABILITY;
/** Pointer to a const TPM2_CC_GET_CAPABILITY request. */
typedef const TPM2REQGETCAPABILITY *PCTPM2REQGETCAPABILITY;
/** @} */


/**
 * TPM response header (everything big endian).
 */
#pragma pack(1)
typedef struct TPMRESPHDR
{
    /** The tag for this request. */
    uint16_t            u16Tag;
    /** Size of the response in bytes. */
    uint32_t            cbResp;
    /** The error code for the response. */
    uint32_t            u32ErrCode;
} TPMRESPHDR;
#pragma pack()
AssertCompileSize(TPMRESPHDR, 2 + 4 + 4);
/** Pointer to a TPM response header. */
typedef TPMRESPHDR *PTPMRESPHDR;
/** Pointer to a const TPM response header. */
typedef const TPMRESPHDR *PCTPMRESPHDR;


/** @name TPM 1.2 response tags
 * @{ */
/** A response from a command with no authentication. */
#define TPM_TAG_RSP_COMMAND                 UINT16_C(0x00c4)
/** An authenticated response with one authentication handle. */
#define TPM_TAG_RSP_AUTH1_COMMAND           UINT16_C(0x00c5)
/** An authenticated response with two authentication handles. */
#define TPM_TAG_RSP_AUTH2_COMMAND           UINT16_C(0x00c6)
/** @} */


/** @name TPM status codes.
 * @{ */
#ifndef TPM_SUCCESS
/** Request executed successfully. */
# define TPM_SUCCESS                        UINT32_C(0)
#endif
#ifndef TPM_AUTHFAIL
/** Authentication failed. */
# define TPM_AUTHFAIL                       UINT32_C(1)
#endif
#ifndef TPM_BADINDEX
/** An index is malformed. */
# define TPM_BADINDEX                       UINT32_C(2)
#endif
#ifndef TPM_BAD_PARAMETER
/** A request parameter is invalid. */
# define TPM_BAD_PARAMETER                  UINT32_C(3)
#endif
#ifndef TPM_FAIL
/** The TPM failed to execute the request. */
# define TPM_FAIL                           UINT32_C(9)
#endif
/** @todo Extend as need arises. */
/** @} */


/* Some inline helpers to account for the unaligned members of the request and response headers. */

/**
 * Returns the request tag of the given TPM request header.
 *
 * @returns TPM request tag in bytes.
 * @param   pTpmReqHdr          Pointer to the TPM request header.
 */
DECLINLINE(uint16_t) RTTpmReqGetTag(PCTPMREQHDR pTpmReqHdr)
{
    return RT_BE2H_U16(pTpmReqHdr->u16Tag);
}


/**
 * Returns the request size of the given TPM request header.
 *
 * @returns TPM request size in bytes.
 * @param   pTpmReqHdr          Pointer to the TPM request header.
 */
DECLINLINE(size_t) RTTpmReqGetSz(PCTPMREQHDR pTpmReqHdr)
{
    uint32_t cbReq;
    memcpy(&cbReq, &pTpmReqHdr->cbReq, sizeof(pTpmReqHdr->cbReq));
    return RT_BE2H_U32(cbReq);
}


/**
 * Returns the request ordinal of the given TPM request header.
 *
 * @returns TPM request ordinal in bytes.
 * @param   pTpmReqHdr          Pointer to the TPM request header.
 */
DECLINLINE(uint32_t) RTTpmReqGetOrdinal(PCTPMREQHDR pTpmReqHdr)
{
    uint32_t u32Ordinal;
    memcpy(&u32Ordinal, &pTpmReqHdr->u32Ordinal, sizeof(pTpmReqHdr->u32Ordinal));
    return RT_BE2H_U32(u32Ordinal);
}


/**
 * Returns the response tag of the given TPM response header.
 *
 * @returns TPM request tag in bytes.
 * @param   pTpmRespHdr         Pointer to the TPM response header.
 */
DECLINLINE(uint16_t) RTTpmRespGetTag(PCTPMRESPHDR pTpmRespHdr)
{
    return RT_BE2H_U16(pTpmRespHdr->u16Tag);
}


/**
 * Returns the response size included in the given TPM response header.
 *
 * @returns TPM response size in bytes.
 * @param   pTpmRespHdr         Pointer to the TPM response header.
 */
DECLINLINE(size_t) RTTpmRespGetSz(PCTPMRESPHDR pTpmRespHdr)
{
    uint32_t cbResp;
    memcpy(&cbResp, &pTpmRespHdr->cbResp, sizeof(pTpmRespHdr->cbResp));
    return RT_BE2H_U32(cbResp);
}


/**
 * Returns the error code of the given TPM response header.
 *
 * @returns TPM response error code.
 * @param   pTpmRespHdr         Pointer to the TPM response header.
 */
DECLINLINE(uint32_t) RTTpmRespGetErrCode(PCTPMRESPHDR pTpmRespHdr)
{
    uint32_t u32ErrCode;
    memcpy(&u32ErrCode, &pTpmRespHdr->u32ErrCode, sizeof(pTpmRespHdr->u32ErrCode));
    return RT_BE2H_U32(u32ErrCode);
}

#endif /* !IPRT_INCLUDED_formats_tpm_h */

