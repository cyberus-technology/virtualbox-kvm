/** @file
 * IPRT - Secure Socket Layer (SSL) / Transport Security Layer (TLS)
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_crypto_ssl_h
#define IPRT_INCLUDED_crypto_ssl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/sg.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_crssl  RTCrSsl - Secure Socket Layer (SSL) / Transport Security Layer (TLS)
 * @ingroup grp_rt_crypto
 * @{
 */

/** SSL handle. */
typedef R3PTRTYPE(struct RTCRSSLINT *)                  RTCRSSL;
/** Pointer to a SSL handle. */
typedef RTCRSSL                                        *PRTCRSSL;
/** Nil SSL handle. */
#define NIL_RTCRSSL                                     ((RTCRSSL)0)

/** SSL session handle. */
typedef R3PTRTYPE(struct RTCRSSLSESSIONINT *)           RTCRSSLSESSION;
/** Pointer to a SSL session handle. */
typedef RTCRSSLSESSION                                 *PRTCRSSLSESSION;
/** Nil SSL session handle. */
#define NIL_RTCRSSLSESSION                              ((RTCRSSLSESSION)0)


RTDECL(int) RTCrSslCreate(PRTCRSSL phSsl, uint32_t fFlags);

/**
 * Retains a reference to the SSL handle.
 *
 * @returns New reference count, UINT32_MAX on invalid handle (asserted).
 *
 * @param   hSsl           The SSL handle.
 */
RTDECL(uint32_t) RTCrSslRetain(RTCRSSL hSsl);

/**
 * Release a reference to the SSL handle.
 *
 * @returns New reference count, UINT32_MAX on invalid handle (asserted).
 *
 * @param   hSsl            The SSL handle.  The NIL handle is quietly
 *                          ignored and 0 is returned.
 */
RTDECL(uint32_t) RTCrSslRelease(RTCRSSL hSsl);

#define RTCRSSL_FILE_F_PEM      0
#define RTCRSSL_FILE_F_ASN1     RT_BIT_32(1)

RTDECL(int) RTCrSslSetCertificateFile(RTCRSSL hSsl, const char *pszFile, uint32_t fFlags);
RTDECL(int) RTCrSslSetPrivateKeyFile(RTCRSSL hSsl, const char *pszFile, uint32_t fFlags);
RTDECL(int) RTCrSslLoadTrustedRootCerts(RTCRSSL hSsl, const char *pszFile, const char *pszDir);
RTDECL(int) RTCrSslSetNoPeerVerify(RTCRSSL hSsl);
/** @todo Min/max protocol setters. */



RTDECL(int) RTCrSslCreateSession(RTCRSSL hSsl, RTSOCKET hSocket, uint32_t fFlags, PRTCRSSLSESSION phSslSession);
RTDECL(int) RTCrSslCreateSessionForNativeSocket(RTCRSSL hSsl, RTHCINTPTR hNativeSocket, uint32_t fFlags,
                                              PRTCRSSLSESSION phSslSession);
/** @name RTCRSSLSESSION_F_XXX - Flags for RTCrSslCreateSession and RTCrSslCreateSessionForNativeSocket.
 * @{ */
/** The socket is non-blocking. */
#define RTCRSSLSESSION_F_NON_BLOCKING   RT_BIT_32(0)
/** @} */

/**
 * Retains a reference to the SSL session handle.
 *
 * @returns New reference count, UINT32_MAX on invalid handle (asserted).
 *
 * @param   hSslSession     The SSL session handle.
 */
RTDECL(uint32_t) RTCrSslSessionRetain(RTCRSSLSESSION hSslSession);

/**
 * Release a reference to the SSL handle.
 *
 * @returns New reference count, UINT32_MAX on invalid handle (asserted).
 *
 * @param   hSslSession     The SSL session handle.  The NIL handle is quietly
 *                          ignored and 0 is returned.
 */
RTDECL(uint32_t) RTCrSslSessionRelease(RTCRSSLSESSION hSslSession);

RTDECL(int) RTCrSslSessionAccept(RTCRSSLSESSION hSslSession, uint32_t fFlags);
RTDECL(int) RTCrSslSessionConnect(RTCRSSLSESSION hSslSession, uint32_t fFlags);

RTDECL(const char *) RTCrSslSessionGetVersion(RTCRSSLSESSION hSslSession);
RTDECL(int) RTCrSslSessionGetCertIssuerNameAsString(RTCRSSLSESSION hSslSession, char *pszBuf, size_t cbBuf, size_t *pcbActual);
RTDECL(bool) RTCrSslSessionPending(RTCRSSLSESSION hSslSession);
RTDECL(ssize_t) RTCrSslSessionRead(RTCRSSLSESSION hSslSession, void *pvBuf, size_t cbToRead);
RTDECL(ssize_t) RTCrSslSessionWrite(RTCRSSLSESSION hSslSession, void const *pvBuf, size_t cbToWrite);


/** @} */
RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_ssl_h */

