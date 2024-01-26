/** @file
 * IPRT - Uniform Resource Identifier handling.
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_uri_h
#define IPRT_INCLUDED_uri_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_uri    RTUri - Uri parsing and creation
 *
 * URI parsing and creation based on RFC-3986.
 *
 * @remarks The whole specification isn't implemented and we only provide scheme
 *          specific special APIs for "file://".
 *
 * @ingroup grp_rt
 * @{
 */


/**
 * Parsed URI.
 *
 * @remarks This structure is subject to change.
 */
typedef struct RTURIPARSED
{
    /** Magic value (for internal use only). */
    uint32_t    u32Magic;
    /** RTURIPARSED_F_XXX. */
    uint32_t    fFlags;

    /** The length of the scheme. */
    size_t      cchScheme;

    /** The offset into the string of the authority. */
    size_t      offAuthority;
    /** The authority length.
     * @remarks The authority component can be zero length, so to check whether
     *          it's there or not consult RTURIPARSED_F_HAVE_AUTHORITY. */
    size_t      cchAuthority;

    /** The offset into the string of the path. */
    size_t      offPath;
    /** The length of the path. */
    size_t      cchPath;

    /** The offset into the string of the query. */
    size_t      offQuery;
    /** The length of the query. */
    size_t      cchQuery;

    /** The offset into the string of the fragment. */
    size_t      offFragment;
    /** The length of the fragment. */
    size_t      cchFragment;

    /** @name Authority subdivisions
     * @{ */
    /** If there is a userinfo part, this is the start of it. Otherwise it's the
     * same as offAuthorityHost. */
    size_t      offAuthorityUsername;
    /** The length of the username (zero if not present). */
    size_t      cchAuthorityUsername;
    /** If there is a userinfo part containing a password, this is the start of it.
     * Otherwise it's the same as offAuthorityHost. */
    size_t      offAuthorityPassword;
    /** The length of the password (zero if not present). */
    size_t      cchAuthorityPassword;
    /** The offset of the host part of the authority. */
    size_t      offAuthorityHost;
    /** The length of the host part of the authority. */
    size_t      cchAuthorityHost;
    /** The authority port number, UINT32_MAX if not present or empty. */
    uint32_t    uAuthorityPort;
    /** @} */
} RTURIPARSED;
/** Pointer to a parsed URI. */
typedef RTURIPARSED *PRTURIPARSED;
/** Pointer to a const parsed URI. */
typedef RTURIPARSED const *PCRTURIPARSED;

/** @name  RTURIPARSED_F_XXX - RTURIPARSED::fFlags
 * @{  */
/** Set if the URI contains escaped characters. */
#define RTURIPARSED_F_CONTAINS_ESCAPED_CHARS        UINT32_C(0x00000001)
/** Set if the URI has an authority component.  Necessary since the authority
 * component can have a zero length. */
#define RTURIPARSED_F_HAS_AUTHORITY                 UINT32_C(0x00000002)
/** Set if there is a port component. */
#define RTURIPARSED_F_HAS_PORT                      UINT32_C(0x00000004)
/** @} */

/**
 * Parses a URI.
 *
 * @returns IPRT status code.
 * @param   pszUri              The URI to parse.
 * @param   pParsed             Where to return the details.  This can be handed
 *                              to the RTUriParsed* APIs for retriving
 *                              information.
 */
RTDECL(int) RTUriParse(const char *pszUri, PRTURIPARSED pParsed);

/**
 * Extract the scheme out of a parsed URI.
 *
 * @returns the scheme if the URI is valid, NULL otherwise.
 * @param   pszUri              The URI passed to RTUriParse when producing the
 *                              info in @a pParsed.
 * @param   pParsed             Pointer to the RTUriParse output.
 */
RTDECL(char *) RTUriParsedScheme(const char *pszUri, PCRTURIPARSED pParsed);

/**
 * Extract the authority out of a parsed URI.
 *
 * @returns the authority if the URI contains one, NULL otherwise.
 * @param   pszUri              The URI passed to RTUriParse when producing the
 *                              info in @a pParsed.
 * @param   pParsed             Pointer to the RTUriParse output.
 * @remarks The authority can have a zero length.
 */
RTDECL(char *) RTUriParsedAuthority(const char *pszUri, PCRTURIPARSED pParsed);

/**
 * Extract the username out of the authority component in a parsed URI.
 *
 * @returns The username if the URI contains one, otherwise NULL.
 * @param   pszUri              The URI passed to RTUriParse when producing the
 *                              info in @a pParsed.
 * @param   pParsed             Pointer to the RTUriParse output.
 *
 * @todo    This may currently be returning NULL when it maybe would be more
 *          appropriate to return an empty string...
 */
RTDECL(char *) RTUriParsedAuthorityUsername(const char *pszUri, PCRTURIPARSED pParsed);

/**
 * Extract the password out of the authority component in a parsed URI.
 *
 * @returns The password if the URI contains one, otherwise NULL.
 * @param   pszUri              The URI passed to RTUriParse when producing the
 *                              info in @a pParsed.
 * @param   pParsed             Pointer to the RTUriParse output.
 *
 * @todo    This may currently be returning NULL when it maybe would be more
 *          appropriate to return an empty string...
 */
RTDECL(char *) RTUriParsedAuthorityPassword(const char *pszUri, PCRTURIPARSED pParsed);

/**
 * Extract the host out of the authority component in a parsed URI.
 *
 * @returns The host if the URI contains one, otherwise NULL.
 * @param   pszUri              The URI passed to RTUriParse when producing the
 *                              info in @a pParsed.
 * @param   pParsed             Pointer to the RTUriParse output.
 *
 * @todo    This may currently be returning NULL when it maybe would be more
 *          appropriate to return an empty string...
 */
RTDECL(char *) RTUriParsedAuthorityHost(const char *pszUri, PCRTURIPARSED pParsed);

/**
 * Extract the port number out of the authority component in a parsed URI.
 *
 * @returns The port number if the URI contains one, otherwise UINT32_MAX.
 * @param   pszUri              The URI passed to RTUriParse when producing the
 *                              info in @a pParsed.
 * @param   pParsed             Pointer to the RTUriParse output.
 */
RTDECL(uint32_t) RTUriParsedAuthorityPort(const char *pszUri, PCRTURIPARSED pParsed);

/**
 * Extract the path out of a parsed URI.
 *
 * @returns the path if the URI contains one, NULL otherwise.
 * @param   pszUri              The URI passed to RTUriParse when producing the
 *                              info in @a pParsed.
 * @param   pParsed             Pointer to the RTUriParse output.
 */
RTDECL(char *) RTUriParsedPath(const char *pszUri, PCRTURIPARSED pParsed);

/**
 * Extract the query out of a parsed URI.
 *
 * @returns the query if the URI contains one, NULL otherwise.
 * @param   pszUri              The URI passed to RTUriParse when producing the
 *                              info in @a pParsed.
 * @param   pParsed             Pointer to the RTUriParse output.
 */
RTDECL(char *) RTUriParsedQuery(const char *pszUri, PCRTURIPARSED pParsed);

/**
 * Extract the fragment out of a parsed URI.
 *
 * @returns the fragment if the URI contains one, NULL otherwise.
 * @param   pszUri              The URI passed to RTUriParse when producing the
 *                              info in @a pParsed.
 * @param   pParsed             Pointer to the RTUriParse output.
 */
RTDECL(char *) RTUriParsedFragment(const char *pszUri, PCRTURIPARSED pParsed);



/**
 * Creates a generic URI.
 *
 * The returned pointer must be freed using RTStrFree().
 *
 * @returns the new URI on success, NULL otherwise.
 * @param   pszScheme           The URI scheme.
 * @param   pszAuthority        The authority part of the URI (optional).
 * @param   pszPath             The path part of the URI (optional).
 * @param   pszQuery            The query part of the URI (optional).
 * @param   pszFragment         The fragment part of the URI (optional).
 */
RTDECL(char *) RTUriCreate(const char *pszScheme, const char *pszAuthority, const char *pszPath, const char *pszQuery,
                           const char *pszFragment);

/**
 * Check whether the given scheme matches that of the URI.
 *
 * This does not validate the URI, it just compares the scheme, no more, no
 * less.  Thus it's much faster than using RTUriParsedScheme.
 *
 * @returns true if the scheme match, false if not.
 * @param   pszUri              The URI to check.
 * @param   pszScheme           The scheme to compare with.
 */
RTDECL(bool)   RTUriIsSchemeMatch(const char *pszUri, const char *pszScheme);

/** @defgroup grp_rt_uri_file   RTUriFile - Uri file parsing and creation
 *
 * Implements basic "file:" scheme support to the generic RTUri interface.  This
 * is partly documented in RFC-1738.
 *
 * @{
 */

/**
 * Creates a file URI.
 *
 * The returned pointer must be freed using RTStrFree().
 *
 * @returns The new URI on success, NULL otherwise.  Free With RTStrFree.
 * @param   pszPath         The path to create an 'file://' URI for.  This is
 *                          assumed to be using the default path style of the
 *                          system.
 *
 * @sa      RTUriFileCreateEx, RTUriCreate
 */
RTDECL(char *) RTUriFileCreate(const char *pszPath);

/**
 * Creates an file URL for the given path.
 *
 * This API works like RTStrToUtf16Ex with regard to result allocation or
 * buffering (i.e. it's a bit complicated but very flexible).
 *
 * @returns iprt status code.
 * @param   pszPath         The path to convert to a file:// URL.
 * @param   fPathStyle      The input path style, exactly one of
 *                          RTPATH_STR_F_STYLE_HOST, RTPATH_STR_F_STYLE_DOS and
 *                          RTPATH_STR_F_STYLE_UNIX.  Must include iprt/path.h.
 * @param   ppszUri         If cbUri is non-zero, this must either be pointing
 *                          to pointer to a buffer of the specified size, or
 *                          pointer to a NULL pointer.  If *ppszUri is NULL or
 *                          cbUri is zero a buffer of at least cbUri chars will
 *                          be allocated to hold the URI.  If a buffer was
 *                          requested it must be freed using RTStrFree().
 * @param   cbUri           The buffer size in bytes (includes terminator).
 * @param   pcchUri         Where to store the length of the URI string,
 *                          excluding the terminator. (Optional)
 *
 *                          This may be set under some error conditions,
 *                          however, only for VERR_BUFFER_OVERFLOW and
 *                          VERR_NO_STR_MEMORY will it contain a valid string
 *                          length that can be used to resize the buffer.
 * @sa      RTUriCreate, RTUriFileCreate
 */
RTDECL(int) RTUriFileCreateEx(const char *pszPath, uint32_t fPathStyle, char **ppszUri, size_t cbUri, size_t *pcchUri);

/**
 * Returns the file path encoded in the file URI.
 *
 * This differs a quite a bit from RTUriParsedPath in that it tries to be
 * compatible with URL produced by older windows version.  This API is basically
 * producing the same results as the PathCreateFromUrl API on Windows.
 *
 * @returns The path if the URI contains one, system default path style,
 *          otherwise NULL.
 * @param   pszUri          The alleged 'file://' URI to extract the path from.
 *
 * @sa      RTUriParsedPath, RTUriFilePathEx
 */
RTDECL(char *) RTUriFilePath(const char *pszUri);

/**
 * Queries the file path for the given file URI.
 *
 * This API works like RTStrToUtf16Ex with regard to result allocation or
 * buffering (i.e. it's a bit complicated but very flexible).
 *
 * This differs a quite a bit from RTUriParsedPath in that it tries to be
 * compatible with URL produced by older windows version.  This API is basically
 * producing the same results as the PathCreateFromUrl API on Windows.
 *
 * @returns IPRT status code.
 * @retval  VERR_URI_NOT_FILE_SCHEME if not file scheme.
 *
 * @param   pszUri          The alleged file:// URI to extract the path from.
 * @param   fPathStyle      The output path style, exactly one of
 *                          RTPATH_STR_F_STYLE_HOST, RTPATH_STR_F_STYLE_DOS and
 *                          RTPATH_STR_F_STYLE_UNIX.  Must include iprt/path.h.
 * @param   ppszPath        If cbPath is non-zero, this must either be pointing
 *                          to pointer to a buffer of the specified size, or
 *                          pointer to a NULL pointer.  If *ppszPath is NULL or
 *                          cbPath is zero a buffer of at least cbPath chars
 *                          will be allocated to hold the path.  If a buffer was
 *                          requested it must be freed using RTStrFree().
 * @param   cbPath          The buffer size in bytes (includes terminator).
 * @param   pcchPath        Where to store the length of the path string,
 *                          excluding the terminator. (Optional)
 *
 *                          This may be set under some error conditions,
 *                          however, only for VERR_BUFFER_OVERFLOW and
 *                          VERR_NO_STR_MEMORY will it contain a valid string
 *                          length that can be used to resize the buffer.
 * @sa      RTUriParsedPath, RTUriFilePath
 */
RTDECL(int) RTUriFilePathEx(const char *pszUri, uint32_t fPathStyle, char **ppszPath, size_t cbPath, size_t *pcchPath);

/** @} */

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_uri_h */

