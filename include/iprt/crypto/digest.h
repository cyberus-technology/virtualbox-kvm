/** @file
 * IPRT - Crypto - Cryptographic Hash / Message Digest.
 */

/*
 * Copyright (C) 2014-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_crypto_digest_h
#define IPRT_INCLUDED_crypto_digest_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/asn1.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_crdigest RTCrDigest - Crypographic Hash / Message Digest API.
 * @ingroup grp_rt
 * @{
 */

/**
 * Cryptographic hash / message digest provider descriptor.
 *
 * This gives the basic details and identifiers of the algorithm as well as
 * function pointers to the implementation.
 */
typedef struct RTCRDIGESTDESC
{
    /** The message digest provider name. */
    const char         *pszName;
    /** The object ID string. */
    const char         *pszObjId;
    /** Pointer to a NULL terminated table of alias object IDs (optional). */
    const char * const *papszObjIdAliases;
    /** The IPRT digest type. */
    RTDIGESTTYPE        enmType;
    /** The max size of the final hash (binary). */
    uint32_t            cbHash;
    /** The size of the state. */
    uint32_t            cbState;
    /** Flags, RTCRDIGESTDESC_F_XXX. */
    uint32_t            fFlags;

    /**
     * Allocates the digest data.
     */
    DECLCALLBACKMEMBER(void *, pfnNew,(void));

    /**
     * Frees the digest data.
     *
     * @param   pvState     The opaque message digest state.
     */
    DECLCALLBACKMEMBER(void, pfnFree,(void *pvState));

    /**
     * Updates the digest with more data.
     *
     * @param   pvState     The opaque message digest state.
     * @param   pvData      The data to add to the digest.
     * @param   cbData      The amount of data to add to the digest.
     */
    DECLCALLBACKMEMBER(void, pfnUpdate,(void *pvState, const void *pvData, size_t cbData));

    /**
     * Finalizes the digest calculation.
     *
     * @param   pvState     The opaque message digest state.
     * @param   pbHash      Where to store the output digest.  This buffer is at
     *                      least RTCRDIGESTDESC::cbHash bytes large.
     */
    DECLCALLBACKMEMBER(void, pfnFinal,(void *pvState, uint8_t *pbHash));

    /**
     * (Re-)Initializes the digest. Optional.
     *
     * Optional, RT_BZERO will be used if NULL.
     *
     * @returns IPRT status code.
     * @param   pvState     The opaque message digest state.
     * @param   pvOpaque    Opaque algortihm specific parameter.
     * @param   fReInit     Set if this is a re-init call.
     */
    DECLCALLBACKMEMBER(int, pfnInit,(void *pvState, void *pvOpaque, bool fReInit));

    /**
     * Deletes the message digest state.
     *
     * Optional, memset will be used if NULL.
     *
     * @param   pvState     The opaque message digest state.
     */
    DECLCALLBACKMEMBER(void, pfnDelete,(void *pvState));

    /**
     * Clones the message digest state.
     *
     * Optional, memcpy will be used if NULL.
     *
     * @returns IPRT status code.
     * @param   pvState     The opaque message digest state (destination).
     * @param   pvSrcState  The opaque message digest state to clone (source).
     */
    DECLCALLBACKMEMBER(int, pfnClone,(void *pvState, void const *pvSrcState));

    /**
     * Gets the hash size.
     *
     * Optional, if not provided RTCRDIGESTDESC::cbHash will be returned.  If
     * provided though, RTCRDIGESTDESC::cbHash must be set to the largest possible
     * hash size.
     *
     * @returns The hash size.
     * @param   pvState     The opaque message digest state.
     */
    DECLCALLBACKMEMBER(uint32_t, pfnGetHashSize,(void *pvState));

    /**
     * Gets the digest type (when enmType is RTDIGESTTYPE_UNKNOWN).
     *
     * @returns The hash size.
     * @param   pvState     The opaque message digest state.
     */
    DECLCALLBACKMEMBER(RTDIGESTTYPE, pfnGetDigestType,(void *pvState));
} RTCRDIGESTDESC;
/** Pointer to const message digest details and vtable. */
typedef RTCRDIGESTDESC const *PCRTCRDIGESTDESC;

/** @name RTCRDIGESTDESC_F_XXX
 * @{ */
/** Digest is deprecated. */
#define RTCRDIGESTDESC_F_DEPRECATED             RT_BIT_32(0)
/** Digest is compromised. */
#define RTCRDIGESTDESC_F_COMPROMISED            RT_BIT_32(1)
/** Digest is severely compromised. */
#define RTCRDIGESTDESC_F_SERVERELY_COMPROMISED  RT_BIT_32(2)
/** @} */

/**
 * Finds a cryptographic hash / message digest descriptor by object identifier
 * string.
 *
 * @returns Pointer to the message digest details & vtable if found.  NULL if
 *          not found.
 * @param   pszObjId        The dotted object identifier string of the message
 *                          digest algorithm.
 * @param   ppvOpaque       Where to return an opaque implementation specfici
 *                          sub-type indicator that can be passed to
 *                          RTCrDigestCreate.  This is optional, fewer
 *                          algortihms are available if not specified.
 */
RTDECL(PCRTCRDIGESTDESC) RTCrDigestFindByObjIdString(const char *pszObjId, void **ppvOpaque);

/**
 * Finds a cryptographic hash / message digest descriptor by object identifier
 * ASN.1 object.
 *
 * @returns Pointer to the message digest details & vtable if found.  NULL if
 *          not found.
 * @param   pObjId          The ASN.1 object ID of the message digest algorithm.
 * @param   ppvOpaque       Where to return an opaque implementation specfici
 *                          sub-type indicator that can be passed to
 *                          RTCrDigestCreate.  This is optional, fewer
 *                          algortihms are available if not specified.
 */
RTDECL(PCRTCRDIGESTDESC) RTCrDigestFindByObjId(PCRTASN1OBJID pObjId, void **ppvOpaque);

RTDECL(PCRTCRDIGESTDESC) RTCrDigestFindByType(RTDIGESTTYPE enmDigestType);
RTDECL(int)             RTCrDigestCreateByObjIdString(PRTCRDIGEST phDigest, const char *pszObjId);
RTDECL(int)             RTCrDigestCreateByObjId(PRTCRDIGEST phDigest, PCRTASN1OBJID pObjId);
RTDECL(int)             RTCrDigestCreateByType(PRTCRDIGEST phDigest, RTDIGESTTYPE enmDigestType);


/**
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VINF_CR_DIGEST_DEPRECATED on success from a deprecated hash algorithm.
 * @retval  VINF_CR_DIGEST_COMPROMISED on success from a compromised hash algorithm.
 * @retval  VINF_CR_DIGEST_SEVERELY_COMPROMISED on success from a severely compromised hash algorithm.
 */
RTDECL(int)             RTCrDigestCreate(PRTCRDIGEST phDigest, PCRTCRDIGESTDESC pDesc, void *pvOpaque);
/**
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VINF_CR_DIGEST_DEPRECATED on success from a deprecated hash algorithm.
 * @retval  VINF_CR_DIGEST_COMPROMISED on success from a compromised hash algorithm.
 * @retval  VINF_CR_DIGEST_SEVERELY_COMPROMISED on success from a severely compromised hash algorithm.
 */
RTDECL(int)             RTCrDigestClone(PRTCRDIGEST phDigest, RTCRDIGEST hSrc);
/**
 * Resets the digest to start calculating a new digest.
 */
RTDECL(int)             RTCrDigestReset(RTCRDIGEST hDigest);

/**
 * Retains a references to the digest.
 *
 * @returns New reference count. UINT32_MAX if invalid handle.
 * @param   hDigest     Handle to the digest.
 */
RTDECL(uint32_t)        RTCrDigestRetain(RTCRDIGEST hDigest);
/**
 * Releases a references to the digest.
 *
 * @returns New reference count. UINT32_MAX if invalid handle.
 * @param   hDigest     Handle to the digest.  NIL is ignored (returns 0).
 */
RTDECL(uint32_t)        RTCrDigestRelease(RTCRDIGEST hDigest);

/**
 * Updates the digest with more message data.
 *
 * @returns IPRT status code.
 * @param   hDigest     Handle to the digest.
 * @param   pvData      Pointer to the message data.
 * @param   cbData      The number of bytes of data @a pvData points to.
 */
RTDECL(int)             RTCrDigestUpdate(RTCRDIGEST hDigest, void const *pvData, size_t cbData);

/**
 * Updates the digest with more message data from the given VFS file handle.
 *
 * @returns IPRT status code.
 * @param   hDigest     Handle to the digest.
 * @param   hVfsFile    Handle to the VFS file.
 * @param   fRewindFile Rewind to the start of the file if @a true, start
 *                      consumption at the current file position if @a false.
 */
RTDECL(int)             RTCrDigestUpdateFromVfsFile(RTCRDIGEST hDigest, RTVFSFILE hVfsFile, bool fRewindFile);

/**
 * Finalizes the hash calculation, copying out the resulting hash value.
 *
 * This can be called more than once and will always return the same result.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VINF_CR_DIGEST_DEPRECATED on success from a deprecated hash algorithm.
 * @retval  VINF_CR_DIGEST_COMPROMISED on success from a compromised hash algorithm.
 * @retval  VINF_CR_DIGEST_SEVERELY_COMPROMISED on success from a severely compromised hash algorithm.
 * @retval  VINF_BUFFER_UNDERFLOW if the supplied buffer is too big.
 * @retval  VERR_BUFFER_OVERFLOW if the supplied buffer is too small.
 * @retval  VERR_INVALID_STATE if there is nothing to finalize.
 *
 * @param   hDigest     The digest handle.
 * @param   pvHash      Where to return the hash. Optional.
 * @param   cbHash      The hash size.  Optional.
 */
RTDECL(int)             RTCrDigestFinal(RTCRDIGEST hDigest, void *pvHash, size_t cbHash);

RTDECL(bool)            RTCrDigestMatch(RTCRDIGEST hDigest, void const *pvHash, size_t cbHash);
RTDECL(uint8_t const *) RTCrDigestGetHash(RTCRDIGEST hDigest);
RTDECL(uint32_t)        RTCrDigestGetHashSize(RTCRDIGEST hDigest);
RTDECL(uint64_t)        RTCrDigestGetConsumedSize(RTCRDIGEST hDigest);
RTDECL(bool)            RTCrDigestIsFinalized(RTCRDIGEST hDigest);
RTDECL(RTDIGESTTYPE)    RTCrDigestGetType(RTCRDIGEST hDigest);
RTDECL(const char *)    RTCrDigestGetAlgorithmOid(RTCRDIGEST hDigest);

/**
 * Gets the flags for the algorithm.
 *
 * @returns RTCRDIGESTDESC_F_XXX, UINT32_MAX on invalid handle.
 * @param   hDigest     The digest handle.
 */
RTDECL(uint32_t)        RTCrDigestGetFlags(RTCRDIGEST hDigest);


/**
 * Translates an IPRT digest type value to an OID.
 *
 * @returns Dotted OID string on success, NULL if not translatable.
 * @param       enmDigestType       The IPRT digest type value to convert.
 */
RTDECL(const char *) RTCrDigestTypeToAlgorithmOid(RTDIGESTTYPE enmDigestType);

/**
 * Translates an IPRT digest type value to a name/descriptive string.
 *
 * The purpose here is for human readable output rather than machine readable
 * output, i.e. the names aren't set in stone.
 *
 * @returns Pointer to read-only string, NULL if unknown type.
 * @param       enmDigestType       The IPRT digest type value to convert.
 */
RTDECL(const char *) RTCrDigestTypeToName(RTDIGESTTYPE enmDigestType);

/**
 * Translates an IPRT digest type value to a hash size.
 *
 * @returns Hash size (in bytes).
 * @param       enmDigestType       The IPRT digest type value to convert.
 */
RTDECL(uint32_t) RTCrDigestTypeToHashSize(RTDIGESTTYPE enmDigestType);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_digest_h */

