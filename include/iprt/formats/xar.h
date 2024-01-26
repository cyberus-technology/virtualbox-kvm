/** @file
 * IPRT - Extensible Archiver (XAR) format.
 */

/*
 * Copyright (C) 2013-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_formats_xar_h
#define IPRT_INCLUDED_formats_xar_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/assertcompile.h>


/** @defgroup grp_rt_formats_xar   Extensible Archive (XAR) format
 * @ingroup grp_rt_formats
 *
 * @{ */

#pragma pack(4) /* Misdesigned header, not 8-byte aligned size. */
typedef struct XARHEADER
{
    /** The magic number 'xar!' (XAR_HEADER_MAGIC). */
    uint32_t    u32Magic;
    /** The size of this header structure. */
    uint16_t    cbHeader;
    /** The header version structure. */
    uint16_t    uVersion;
    /** The size of the compressed table of content (TOC). */
    uint64_t    cbTocCompressed;
    /** The size of the table of context (TOC) when not compressed. */
    uint64_t    cbTocUncompressed;
    /** Which cryptographic hash function is used (XAR_HASH_XXX). */
    uint32_t    uHashFunction;
} XARHEADER;
#pragma pack()
AssertCompileSize(XARHEADER, 28);
/** Pointer to a XAR header. */
typedef XARHEADER *PXARHEADER;
/** Pointer to a const XAR header. */
typedef XARHEADER const *PCXARHEADER;

/** XAR magic value (on disk endian). */
#define XAR_HEADER_MAGIC        RT_H2LE_U32(RT_MAKE_U32_FROM_U8('x', 'a', 'r', '!'))
/** The current header version value (host endian). */
#define XAR_HEADER_VERSION      1

/** @name XAR hashing functions.
 * @{ */
#define XAR_HASH_NONE           0
#define XAR_HASH_SHA1           1
#define XAR_HASH_MD5            2
#define XAR_HASH_MAX            2
/** @} */

/** @} */

#endif /* !IPRT_INCLUDED_formats_xar_h */

