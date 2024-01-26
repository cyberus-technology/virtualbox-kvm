/** @file
 * IPRT Trusted Platform Module API abstracting host specific APIs.
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

#ifndef IPRT_INCLUDED_tpm_h
#define IPRT_INCLUDED_tpm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

#include <iprt/formats/tpm.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_tpm  IPRT Trusted Platform Module API
 * @ingroup grp_rt
 *
 * This API provides a uniform way to access a Trusted Platform Module across all
 * supported hosts.
 *
 * @{
 */


/**
 * TPM version.
 */
typedef enum RTTPMVERSION
{
    /** The usual invalid option. */
    RTTPMVERSION_INVALID = 0,
    /** TPM conforms to version 1.2 of the TCG specification. */
    RTTPMVERSION_1_2,
    /** TPM conforms to version 2.0 of the TCG specification. */
    RTTPMVERSION_2_0,
    /** TPM version couldn't be acquired. */
    RTTPMVERSION_UNKNOWN,
    /** Usual 32bit hack. */
    RTTPMVERSION_32BIT_HACK = 0x7fffffff
} RTTPMVERSION;
/** Pointer to a TPM version. */
typedef RTTPMVERSION *PRTTPMVERSION;

/** TPM handle. */
typedef struct RTTPMINT              *RTTPM;
/** Pointer to a TPM handle. */
typedef RTTPM                        *PRTTPM;
/** NIL TPM handle value. */
#define NIL_RTTPM                    ((RTTPM)0)


/** Default TPM of the host. */
#define RTTPM_ID_DEFAULT             UINT32_C(0xffffffff)

/**
 * Tries to open the given TPM returning a handle.
 *
 * @returns IPRT status code.
 * @param   phTpm               Where to store the handle to the TPM module on success.
 * @param   idTpm               The TPM to open, use RTTPM_ID_DEFAULT for the default TPM of the system.
 */
RTDECL(int) RTTpmOpen(PRTTPM phTpm, uint32_t idTpm);


/**
 * Closes the given TPM handle freeing all allocated resources.
 *
 * @returns IPRT status code.
 * @param   hTpm                Handle of the TPM to close.
 */
RTDECL(int) RTTpmClose(RTTPM hTpm);


/**
 * Returns the version of the TPM for the given handle.
 *
 * @returns Version implemented by the TPM.
 * @param   hTpm                Handle of the TPM.
 */
RTDECL(RTTPMVERSION) RTTpmGetVersion(RTTPM hTpm);


/**
 * Returns the maximum locality supported by the given TPM.
 *
 * @returns Maximum locality supported (0-4).
 * @param   hTpm                Handle of the TPM.
 */
RTDECL(uint32_t) RTTpmGetLocalityMax(RTTPM hTpm);


/**
 * Cancels a currently executed request for the given TPM handle.
 *
 * @returns IPRT status code.
 * @param   hTpm                Handle of the TPM.
 */
RTDECL(int) RTTpmReqCancel(RTTPM hTpm);


/**
 * Executes the given request on the given TPM handle.
 *
 * @returns IPRT status code.
 * @param   hTpm                Handle of the TPM.
 * @param   bLoc                The locality to use (only 0 might be supported on some hosts).
 * @param   pvReq               The request data.
 * @param   cbReq               Size of the request in bytes.
 * @param   pvResp              Where to store the response data.
 * @param   cbRespMax           Size of the response buffer.
 * @param   pcbResp             Where to store the actual size of the response, optional.
 */
RTDECL(int) RTTpmReqExec(RTTPM hTpm, uint8_t bLoc, const void *pvReq, size_t cbReq,
                         void *pvResp, size_t cbRespMax, size_t *pcbResp);


/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_tpm_h */

