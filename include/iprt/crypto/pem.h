/** @file
 * IPRT - Crypto - PEM-file Reader & Writer.
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

#ifndef IPRT_INCLUDED_crypto_pem_h
#define IPRT_INCLUDED_crypto_pem_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/asn1.h>   /* PRTASN1CORE */
#include <iprt/string.h> /* PFNRTSTROUTPUT */


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_spc  RTCrPem - PEM-file Reader & Writer
 * @ingroup grp_rt_crypto
 * @{
 */


/**
 * One PEM marker word (use RT_STR_TUPLE to initialize).
 */
typedef struct RTCRPEMMARKERWORD
{
    /** The word string. */
    const char         *pszWord;
    /** The length. */
    uint32_t            cchWord;
} RTCRPEMMARKERWORD;
/** Pointer to a const marker word. */
typedef RTCRPEMMARKERWORD const *PCRTCRPEMMARKERWORD;


/**
 * A PEM marker.
 *
 * This is an array of words with lengths, optimized for avoid unnecessary
 * strlen() while searching the file content.  It is ASSUMED that all PEM
 * section markers starts with either 'BEGIN' or 'END', followed by the words
 * in the this structure.
 */
typedef struct RTCRPEMMARKER
{
    /** Pointer to an array of marker words. */
    PCRTCRPEMMARKERWORD paWords;
    /** Number of works in the array papszWords points to. */
    uint32_t            cWords;
} RTCRPEMMARKER;
/** Pointer to a const PEM marker. */
typedef RTCRPEMMARKER const *PCRTCRPEMMARKER;


/**
 * A PEM field.
 */
typedef struct RTCRPEMFIELD
{
    /** Pointer to the next field. */
    struct RTCRPEMFIELD const *pNext;
    /** The field value. */
    char const         *pszValue;
    /** The field value length. */
    size_t              cchValue;
    /** The field name length. */
    size_t              cchName;
    /** The field name. */
    RT_FLEXIBLE_ARRAY_EXTENSION
    char                szName[RT_FLEXIBLE_ARRAY];
} RTCRPEMFIELD;
/** Pointer to a PEM field. */
typedef RTCRPEMFIELD *PRTCRPEMFIELD;
/** Pointer to a const PEM field. */
typedef RTCRPEMFIELD const *PCRTCRPEMFIELD;


/**
 * A PEM section.
 *
 * The API works on linked lists of these.
 */
typedef struct RTCRPEMSECTION
{
    /** Pointer to the next file section. */
    struct RTCRPEMSECTION const *pNext;
    /** The marker for this section.  NULL if binary file. */
    PCRTCRPEMMARKER     pMarker;
    /** Pointer to the binary data. */
    uint8_t            *pbData;
    /** The size of the binary data. */
    size_t              cbData;
    /** List of fields, NULL if none. */
    PCRTCRPEMFIELD      pFieldHead;
    /** Set if RTCRPEMREADFILE_F_SENSITIVE was specified. */
    bool                fSensitive;
} RTCRPEMSECTION;
/** Pointer to a PEM section. */
typedef RTCRPEMSECTION *PRTCRPEMSECTION;
/** Pointer to a const PEM section. */
typedef RTCRPEMSECTION const *PCRTCRPEMSECTION;


/**
 * Frees sections returned by RTCrPemReadFile and RTCrPemParseContent.
 * @returns IPRT status code.
 * @param   pSectionHead        The first section.
 */
RTDECL(int) RTCrPemFreeSections(PCRTCRPEMSECTION pSectionHead);

/**
 * Parses the given data and returns a list of binary sections.
 *
 * If the file isn't an ASCII file or if no markers were found, the entire file
 * content is returned as one single section (with pMarker = NULL).
 *
 * @returns IPRT status code.
 * @retval  VINF_EOF if the file is empty.  The @a ppSectionHead value will be
 *          NULL.
 * @retval  VWRN_NOT_FOUND no section was found and RTCRPEMREADFILE_F_ONLY_PEM
 *          is specified.  The @a ppSectionHead value will be NULL.
 *
 * @param   pvContent       The content bytes to parse.
 * @param   cbContent       The number of content bytes.
 * @param   fFlags          RTCRPEMREADFILE_F_XXX.
 * @param   paMarkers       Array of one or more section markers to look for.
 * @param   cMarkers        Number of markers in the array.
 * @param   ppSectionHead   Where to return the head of the section list.  Call
 *                          RTCrPemFreeSections to free.
 * @param   pErrInfo        Where to return extend error info. Optional.
 */
RTDECL(int) RTCrPemParseContent(void const *pvContent, size_t cbContent, uint32_t fFlags,
                                PCRTCRPEMMARKER paMarkers, size_t cMarkers, PCRTCRPEMSECTION *ppSectionHead, PRTERRINFO pErrInfo);

/**
 * Reads the content of the given file and returns a list of binary sections
 * found in the file.
 *
 * If the file isn't an ASCII file or if no markers were found, the entire file
 * content is returned as one single section (with pMarker = NULL).
 *
 * @returns IPRT status code.
 * @retval  VINF_EOF if the file is empty.  The @a ppSectionHead value will be
 *          NULL.
 * @retval  VWRN_NOT_FOUND no section was found and RTCRPEMREADFILE_F_ONLY_PEM
 *          is specified.  The @a ppSectionHead value will be NULL.
 *
 * @param   pszFilename     The path to the file to read.
 * @param   fFlags          RTCRPEMREADFILE_F_XXX.
 * @param   paMarkers       Array of one or more section markers to look for.
 * @param   cMarkers        Number of markers in the array.
 * @param   ppSectionHead   Where to return the head of the section list. Call
 *                          RTCrPemFreeSections to free.
 * @param   pErrInfo        Where to return extend error info. Optional.
 */
RTDECL(int) RTCrPemReadFile(const char *pszFilename, uint32_t fFlags, PCRTCRPEMMARKER paMarkers, size_t cMarkers,
                            PCRTCRPEMSECTION *ppSectionHead, PRTERRINFO pErrInfo);
/** @name RTCRPEMREADFILE_F_XXX - Flags for RTCrPemReadFile and
 *        RTCrPemParseContent.
 * @{ */
/** Continue on encoding error. */
#define RTCRPEMREADFILE_F_CONTINUE_ON_ENCODING_ERROR    RT_BIT(0)
/** Only PEM sections, no binary fallback. */
#define RTCRPEMREADFILE_F_ONLY_PEM                      RT_BIT(1)
/** Sensitive data, use the safer allocator. */
#define RTCRPEMREADFILE_F_SENSITIVE                     RT_BIT(2)
/** Valid flags. */
#define RTCRPEMREADFILE_F_VALID_MASK                    UINT32_C(0x00000007)
/** @} */

/**
 * Finds the beginning of first PEM section using the specified markers.
 *
 * This will not look any further than the first section.  Nor will it check for
 * binaries.
 *
 * @returns Pointer to the "-----BEGIN XXXX" sequence on success.
 *          NULL if not found.
 * @param   pvContent       The content bytes to parse.
 * @param   cbContent       The number of content bytes.
 * @param   paMarkers       Array of one or more section markers to look for.
 * @param   cMarkers        Number of markers in the array.
 */
RTDECL(const char *) RTCrPemFindFirstSectionInContent(void const *pvContent, size_t cbContent,
                                                      PCRTCRPEMMARKER paMarkers, size_t cMarkers);


/**
 * PEM formatter for a binary data blob.
 *
 * @returns Number of output bytes (sum of @a pfnOutput return values).
 * @param   pfnOutput       The output callback function.
 * @param   pvUser          The user argument to the output callback.
 * @param   pvContent       The binary blob to output.
 * @param   cbContent       Size of the binary blob.
 * @param   pszMarker       The PEM marker, .e.g "PRIVATE KEY", "CERTIFICATE" or
 *                          similar.
 * @sa      RTCrPemWriteAsn1, RTCrPemWriteAsn1ToVfsFile,
 *          RTCrPemWriteAsn1ToVfsFile
 */
RTDECL(size_t) RTCrPemWriteBlob(PFNRTSTROUTPUT pfnOutput, void *pvUser,
                                const void *pvContent, size_t cbContent, const char *pszMarker);

RTDECL(ssize_t) RTCrPemWriteBlobToVfsIoStrm(RTVFSIOSTREAM hVfsIos, const void *pvContent, size_t cbContent, const char *pszMarker);
RTDECL(ssize_t) RTCrPemWriteBlobToVfsFile(RTVFSFILE hVfsFile, const void *pvContent, size_t cbContent, const char *pszMarker);

/**
 * PEM formatter for a generic ASN.1 structure.
 *
 * This will call both RTAsn1EncodePrepare() and RTAsn1EncodeWrite() on
 * @a pRoot.  Uses DER encoding.
 *
 * @returns Number of outputted chars (sum of @a pfnOutput return values),
 *          negative values are error status codes from the ASN.1 encoding.
 * @param   pfnOutput       The output callback function.
 * @param   pvUser          The user argument to the output callback.
 * @param   fFlags          Reserved, MBZ.
 * @param   pRoot           The root of the ASN.1 to encode and format as PEM.
 * @param   pszMarker       The PEM marker, .e.g "PRIVATE KEY", "CERTIFICATE" or
 *                          similar.
 * @param   pErrInfo        For encoding errors. Optional.
 * @sa      RTCrPemWriteAsn1ToVfsFile, RTCrPemWriteAsn1ToVfsFile,
 *          RTCrPemWriteBlob
 */
RTDECL(ssize_t) RTCrPemWriteAsn1(PFNRTSTROUTPUT pfnOutput, void *pvUser, PRTASN1CORE pRoot,
                                 uint32_t fFlags, const char *pszMarker, PRTERRINFO pErrInfo);

/**
 * PEM formatter for a generic ASN.1 structure and output it to @a hVfsIos.
 *
 * This will call both RTAsn1EncodePrepare() and RTAsn1EncodeWrite() on
 * @a pRoot.  Uses DER encoding.
 *
 * @returns Number of chars written, negative values are error status codes from
 *          the ASN.1 encoding or from RTVfsIoStrmWrite().
 * @param   hVfsIos         Handle to the I/O stream to write it to.
 * @param   fFlags          Reserved, MBZ.
 * @param   pRoot           The root of the ASN.1 to encode and format as PEM.
 * @param   pszMarker       The PEM marker, .e.g "PRIVATE KEY", "CERTIFICATE" or
 *                          similar.
 * @param   pErrInfo        For encoding errors. Optional.
 * @sa      RTCrPemWriteAsn1ToVfsFile, RTCrPemWriteAsn1, RTCrPemWriteBlob
 */
RTDECL(ssize_t) RTCrPemWriteAsn1ToVfsIoStrm(RTVFSIOSTREAM hVfsIos, PRTASN1CORE pRoot,
                                            uint32_t fFlags, const char *pszMarker, PRTERRINFO pErrInfo);

/**
 * PEM formatter for a generic ASN.1 structure and output it to @a hVfsFile.
 *
 * This will call both RTAsn1EncodePrepare() and RTAsn1EncodeWrite() on
 * @a pRoot.  Uses DER encoding.
 *
 * @returns Number of chars written, negative values are error status codes from
 *          the ASN.1 encoding or from RTVfsIoStrmWrite().
 * @param   hVfsFile        Handle to the file to write it to.
 * @param   fFlags          Reserved, MBZ.
 * @param   pRoot           The root of the ASN.1 to encode and format as PEM.
 * @param   pszMarker       The PEM marker, .e.g "PRIVATE KEY", "CERTIFICATE" or
 *                          similar.
 * @param   pErrInfo        For encoding errors. Optional.
 * @sa      RTCrPemWriteAsn1ToVfsIoStrm, RTCrPemWriteAsn1, RTCrPemWriteBlob
 */
RTDECL(ssize_t) RTCrPemWriteAsn1ToVfsFile(RTVFSFILE hVfsFile, PRTASN1CORE pRoot,
                                          uint32_t fFlags, const char *pszMarker, PRTERRINFO pErrInfo);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_pem_h */

