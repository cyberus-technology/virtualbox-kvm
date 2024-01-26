/** @file
 * IPRT - Abstract Syntax Notation One (ASN.1) Definitions.
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

#ifndef IPRT_INCLUDED_formats_asn1_h
#define IPRT_INCLUDED_formats_asn1_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>


/** @defgroup grp_rt_formats_asn1   ASN.1 definitions
 * @ingroup grp_rt_formats
 *
 * @{ */

/** @name Tag classes.
 *  @{ */
#define ASN1_TAGCLASS_UNIVERSAL     UINT8_C(0x00)
#define ASN1_TAGCLASS_APPLICATION   UINT8_C(0x40)
#define ASN1_TAGCLASS_CONTEXT       UINT8_C(0x80)
#define ASN1_TAGCLASS_PRIVATE       UINT8_C(0xc0)
#define ASN1_TAGCLASS_MASK          UINT8_C(0xc0)
/** @} */

/** Primitive encoding. */
#define ASN1_TAGFLAG_PRIMITIVE      UINT8_C(0x00)
/** Constructed encoding, as opposed to primitive. */
#define ASN1_TAGFLAG_CONSTRUCTED    UINT8_C(0x20)

/** The tag value mask. */
#define ASN1_TAG_MASK               UINT8_C(0x1f)

/** @name ASN.1 universal tags.
 * @{ */
#define ASN1_TAG_EOC                UINT8_C(0x00)
#define ASN1_TAG_BOOLEAN            UINT8_C(0x01)
#define ASN1_TAG_INTEGER            UINT8_C(0x02)
#define ASN1_TAG_BIT_STRING         UINT8_C(0x03)
#define ASN1_TAG_OCTET_STRING       UINT8_C(0x04)
#define ASN1_TAG_NULL               UINT8_C(0x05)
#define ASN1_TAG_OID                UINT8_C(0x06)
#define ASN1_TAG_OBJECT_DESCRIPTOR  UINT8_C(0x07)
#define ASN1_TAG_EXTERNAL           UINT8_C(0x08)
#define ASN1_TAG_REAL               UINT8_C(0x09)
#define ASN1_TAG_ENUMERATED         UINT8_C(0x0a)
#define ASN1_TAG_EMBEDDED_PDV       UINT8_C(0x0b)
#define ASN1_TAG_UTF8_STRING        UINT8_C(0x0c)
#define ASN1_TAG_RELATIVE_OID       UINT8_C(0x0d)
#define ASN1_TAG_RESERVED_14        UINT8_C(0x0e)
#define ASN1_TAG_RESERVED_15        UINT8_C(0x0f)
#define ASN1_TAG_SEQUENCE           UINT8_C(0x10)
#define ASN1_TAG_SET                UINT8_C(0x11)
#define ASN1_TAG_NUMERIC_STRING     UINT8_C(0x12)
#define ASN1_TAG_PRINTABLE_STRING   UINT8_C(0x13)
#define ASN1_TAG_T61_STRING         UINT8_C(0x14)
#define ASN1_TAG_VIDEOTEX_STRING    UINT8_C(0x15)
#define ASN1_TAG_IA5_STRING         UINT8_C(0x16)
#define ASN1_TAG_UTC_TIME           UINT8_C(0x17) /**< Century seems to be 1900 if YY < 50, otherwise 2000. Baka ASN.1! */
#define ASN1_TAG_GENERALIZED_TIME   UINT8_C(0x18)
#define ASN1_TAG_GRAPHIC_STRING     UINT8_C(0x19)
#define ASN1_TAG_VISIBLE_STRING     UINT8_C(0x1a)
#define ASN1_TAG_GENERAL_STRING     UINT8_C(0x1b)
#define ASN1_TAG_UNIVERSAL_STRING   UINT8_C(0x1c)
#define ASN1_TAG_CHARACTER_STRING   UINT8_C(0x1d)
#define ASN1_TAG_BMP_STRING         UINT8_C(0x1e)
#define ASN1_TAG_USE_LONG_FORM      UINT8_C(0x1f)
/** @} */


/** @} */

#endif /* !IPRT_INCLUDED_formats_asn1_h */

