/** @file
 * IPRT / No-CRT - string.h.
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

#ifndef IPRT_INCLUDED_nocrt_string_h
#define IPRT_INCLUDED_nocrt_string_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

/* Dummy for now, fix when/if needed. */
typedef struct RT_NOCRT(mbstate_t)
{
    unsigned long whatever;
} RT_NOCRT(mbstate_t);


RT_C_DECLS_BEGIN

void       *RT_NOCRT(memchr)(const void *pv, int ch, size_t cb);
int         RT_NOCRT(memcmp)(const void *pv1, const void *pv2, size_t cb);
void       *RT_NOCRT(memcpy)(void *pvDst, const void *pvSrc, size_t cb);
void       *RT_NOCRT(mempcpy)(void *pvDst, const void *pvSrc, size_t cb);
void       *RT_NOCRT(memrchr)(const void *pv, int ch, size_t cb);
void       *RT_NOCRT(memmove)(void *pvDst, const void *pvSrc, size_t cb);
void       *RT_NOCRT(memset)(void *pvDst, int ch, size_t cb);

char       *RT_NOCRT(strcat)(char *pszDst, const char *pszSrc);
char       *RT_NOCRT(strncat)(char *pszDst, const char *pszSrc, size_t cch);
char       *RT_NOCRT(strchr)(const char *psz, int ch);
char       *RT_NOCRT(strrchr)(const char *psz, int ch);
int         RT_NOCRT(strcmp)(const char *psz1, const char *psz2);
int         RT_NOCRT(strncmp)(const char *psz1, const char *psz2, size_t cch);
int         RT_NOCRT(stricmp)(const char *psz1, const char *psz2);
int         RT_NOCRT(strnicmp)(const char *psz1, const char *psz2, size_t cch);
int         RT_NOCRT(strcmpcase)(const char *psz1, const char *psz2, size_t cch);
int         RT_NOCRT(strcoll)(const char *psz1, const char *psz2);
char       *RT_NOCRT(strcpy)(char *pszDst, const char *pszSrc);
char       *RT_NOCRT(strncpy)(char *pszDst, const char *pszSrc, size_t cch);
char       *RT_NOCRT(strcat)(char *pszDst, const char *pszSrc);
char       *RT_NOCRT(strncat)(char *pszDst, const char *pszSrc, size_t cch);
size_t      RT_NOCRT(strlen)(const char *psz);
size_t      RT_NOCRT(strnlen)(const char *psz, size_t cch);
size_t      RT_NOCRT(strspn)(const char *psz, const char *pszBreakChars);
size_t      RT_NOCRT(strcspn)(const char *psz, const char *pszBreakChars);
char       *RT_NOCRT(strpbrk)(const char *psz, const char *pszBreakChars);
char       *RT_NOCRT(strstr)(const char *psz, const char *pszSub);
char       *RT_NOCRT(strtok)(char *psz, const char *pszDelim);
char       *RT_NOCRT(strtok_r)(char *psz, const char *pszDelim, char **ppszSave);
#if 0 /* C++11:  */
char       *RT_NOCRT(strtok_s)(char *psz, /*rsize_t*/ size_t cchMax, const char *pszDelim, char **ppszSave);
#else /* Microsoft: */
char       *RT_NOCRT(strtok_s)(char *psz, const char *pszDelim, char **ppszSave);
#endif
size_t      RT_NOCRT(strxfrm)(char *pszDst, const char *pszSrc, size_t cch);

size_t      RT_NOCRT(wcslen)(const wchar_t *pwsz);
wchar_t    *RT_NOCRT(wcscat)(wchar_t *pwszDst, const wchar_t *pwszSrc);
wchar_t    *RT_NOCRT(wcschr)(const wchar_t *pwsz, wchar_t wc);
wchar_t    *RT_NOCRT(wcscpy)(wchar_t *pwszDst, const wchar_t *pwszSrc);
int         RT_NOCRT(wcsicmp)(const wchar_t *pwsz1, const wchar_t *pwsz2);
size_t      RT_NOCRT(wcstombs)(char *pszDst, const wchar_t *pszSrc, size_t cbDst);


/* Underscored versions for MSC compatibility (mesa #defines regular to _regular
   a lot, which is why we really need these prototypes). */
void       *RT_NOCRT(_memchr)(const void *pv, int ch, size_t cb);
int         RT_NOCRT(_memcmp)(const void *pv1, const void *pv2, size_t cb);
void       *RT_NOCRT(_memcpy)(void *pvDst, const void *pvSrc, size_t cb);
void       *RT_NOCRT(_mempcpy)(void *pvDst, const void *pvSrc, size_t cb);
void       *RT_NOCRT(_memrchr)(const void *pv, int ch, size_t cb);
void       *RT_NOCRT(_memmove)(void *pvDst, const void *pvSrc, size_t cb);
void       *RT_NOCRT(_memset)(void *pvDst, int ch, size_t cb);

char       *RT_NOCRT(_strcat)(char *pszDst, const char *pszSrc);
char       *RT_NOCRT(_strncat)(char *pszDst, const char *pszSrc, size_t cch);
char       *RT_NOCRT(_strchr)(const char *psz, int ch);
char       *RT_NOCRT(_strrchr)(const char *psz, int ch);
int         RT_NOCRT(_strcmp)(const char *psz1, const char *psz2);
int         RT_NOCRT(_strncmp)(const char *psz1, const char *psz2, size_t cch);
int         RT_NOCRT(_stricmp)(const char *psz1, const char *psz2);
int         RT_NOCRT(_strnicmp)(const char *psz1, const char *psz2, size_t cch);
int         RT_NOCRT(_strcmpcase)(const char *psz1, const char *psz2, size_t cch);
int         RT_NOCRT(_strcoll)(const char *psz1, const char *psz2);
char       *RT_NOCRT(_strcpy)(char *pszDst, const char *pszSrc);
char       *RT_NOCRT(_strncpy)(char *pszDst, const char *pszSrc, size_t cch);
char       *RT_NOCRT(_strcat)(char *pszDst, const char *pszSrc);
char       *RT_NOCRT(_strncat)(char *pszDst, const char *pszSrc, size_t cch);
size_t      RT_NOCRT(_strlen)(const char *psz);
size_t      RT_NOCRT(_strnlen)(const char *psz, size_t cch);
size_t      RT_NOCRT(_strspn)(const char *psz, const char *pszBreakChars);
size_t      RT_NOCRT(_strcspn)(const char *psz, const char *pszBreakChars);
char       *RT_NOCRT(_strpbrk)(const char *psz, const char *pszBreakChars);
char       *RT_NOCRT(_strstr)(const char *psz, const char *pszSub);
char       *RT_NOCRT(_strtok)(char *psz, const char *pszDelim);
char       *RT_NOCRT(_strtok_r)(char *psz, const char *pszDelim, char **ppszSave);
#if 0 /* C++11:  */
char       *RT_NOCRT(_strtok_s)(char *psz, /*rsize_t*/ size_t cchMax, const char *pszDelim, char **ppszSave);
#else /* Microsoft: */
char       *RT_NOCRT(_strtok_s)(char *psz, const char *pszDelim, char **ppszSave);
#endif
size_t      RT_NOCRT(_strxfrm)(char *pszDst, const char *pszSrc, size_t cch);

size_t      RT_NOCRT(_wcslen)(const wchar_t *pwsz);
wchar_t    *RT_NOCRT(_wcscat)(wchar_t *pwszDst, const wchar_t *pwszSrc);
wchar_t    *RT_NOCRT(_wcschr)(const wchar_t *pwsz, wchar_t wc);
wchar_t    *RT_NOCRT(_wcscpy)(wchar_t *pwszDst, const wchar_t *pwszSrc);
int         RT_NOCRT(_wcsicmp)(const wchar_t *pwsz1, const wchar_t *pwsz2);
size_t      RT_NOCRT(_wcstombs)(char *pszDst, const wchar_t *pszSrc, size_t cbDst);


#if !defined(RT_WITHOUT_NOCRT_WRAPPERS) && !defined(RT_WITHOUT_NOCRT_WRAPPER_ALIASES)
# define memchr         RT_NOCRT(memchr)
# define memcmp         RT_NOCRT(memcmp)
# define memcpy         RT_NOCRT(memcpy)
# define mempcpy        RT_NOCRT(mempcpy)
# define memrchr        RT_NOCRT(memrchr)
# define memmove        RT_NOCRT(memmove)
# define memset         RT_NOCRT(memset)

# define strcat         RT_NOCRT(strcat)
# define strncat        RT_NOCRT(strncat)
# define strchr         RT_NOCRT(strchr)
# define strrchr        RT_NOCRT(strrchr)
# define strcmp         RT_NOCRT(strcmp)
# define strncmp        RT_NOCRT(strncmp)
# define stricmp        RT_NOCRT(stricmp)
# define strnicmp       RT_NOCRT(strnicmp)
# define strcmpcase     RT_NOCRT(strcmpcase)
# define strcoll        RT_NOCRT(strcoll)
# define strcpy         RT_NOCRT(strcpy)
# define strncpy        RT_NOCRT(strncpy)
# define strcat         RT_NOCRT(strcat)
# define strncat        RT_NOCRT(strncat)
# define strlen         RT_NOCRT(strlen)
# define strnlen        RT_NOCRT(strnlen)
# define strspn         RT_NOCRT(strspn)
# define strcspn        RT_NOCRT(strcspn)
# define strpbrk        RT_NOCRT(strpbrk)
# define strstr         RT_NOCRT(strstr)
# define strtok         RT_NOCRT(strtok)
# define strtok_r       RT_NOCRT(strtok_r)
# define strtok_s       RT_NOCRT(strtok_s)
# define strxfrm        RT_NOCRT(strxfrm)

# define wcslen         RT_NOCRT(wcslen)
# define wcscat         RT_NOCRT(wcscat)
# define wcschr         RT_NOCRT(wcschr)
# define wcscpy         RT_NOCRT(wcscpy)
# define wcsicmp        RT_NOCRT(wcsicmp)
# define wcstombs       RT_NOCRT(wcstombs)

/* Underscored: */
# define _memchr        RT_NOCRT(memchr)
# define _memcmp        RT_NOCRT(memcmp)
# define _memcpy        RT_NOCRT(memcpy)
# define _mempcpy       RT_NOCRT(mempcpy)
# define _memrchr       RT_NOCRT(memrchr)
# define _memmove       RT_NOCRT(memmove)
# define _memset        RT_NOCRT(memset)

# define _strcat        RT_NOCRT(strcat)
# define _strncat       RT_NOCRT(strncat)
# define _strchr        RT_NOCRT(strchr)
# define _strrchr       RT_NOCRT(strrchr)
# define _strcmp        RT_NOCRT(strcmp)
# define _strncmp       RT_NOCRT(strncmp)
# define _stricmp       RT_NOCRT(stricmp)
# define _strnicmp      RT_NOCRT(strnicmp)
# define _strcmpcase    RT_NOCRT(strcmpcase)
# define _strcoll       RT_NOCRT(strcoll)
# define _strcpy        RT_NOCRT(strcpy)
# define _strncpy       RT_NOCRT(strncpy)
# define _strcat        RT_NOCRT(strcat)
# define _strncat       RT_NOCRT(strncat)
# define _strlen        RT_NOCRT(strlen)
# define _strnlen       RT_NOCRT(strnlen)
# define _strspn        RT_NOCRT(strspn)
# define _strcspn       RT_NOCRT(strcspn)
# define _strpbrk       RT_NOCRT(strpbrk)
# define _strstr        RT_NOCRT(strstr)
# define _strtok        RT_NOCRT(strtok)
# define _strtok_r      RT_NOCRT(strtok_r)
# define _strtok_s      RT_NOCRT(strtok_s)
# define _strxfrm       RT_NOCRT(strxfrm)

# define _wcslen        RT_NOCRT(wcslen)
# define _wcscat        RT_NOCRT(wcscat)
# define _wcschr        RT_NOCRT(wcschr)
# define _wcscpy        RT_NOCRT(wcscpy)
# define _wcsicmp       RT_NOCRT(wcsicmp)
# define _wcstombs      RT_NOCRT(wcstombs)
#endif


#ifdef IPRT_NO_CRT_FOR_3RD_PARTY
/*
 * Only for external libraries and such.
 */

const char *RT_NOCRT(strerror)(int iErrNo);
char       *RT_NOCRT(strdup)(const char *pszSrc);

/* Underscored: */
const char *RT_NOCRT(_strerror)(int iErrNo);
char       *RT_NOCRT(_strdup)(const char *pszSrc);

# if !defined(RT_WITHOUT_NOCRT_WRAPPERS) && !defined(RT_WITHOUT_NOCRT_WRAPPER_ALIASES)
#  define strerror  RT_NOCRT(strerror)
#  define strdup    RT_NOCRT(strdup)

/* Underscored: */
#  define _strerror RT_NOCRT(strerror)
#  define _strdup   RT_NOCRT(strdup)
# endif

#endif /* IPRT_NO_CRT_FOR_3RD_PARTY */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_nocrt_string_h */
