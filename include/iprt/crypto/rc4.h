/** @file
 * IPRT - Crypto - Alleged RC4 Cipher.
 */

/*
 * Copyright (C) 2018-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_crypto_rc4_h
#define IPRT_INCLUDED_crypto_rc4_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_cr_rc4  RTCrRc4 - Alleged RC4 Cipher.
 * @ingroup grp_rt_crypto
 * @{
 */

/** RC4 key structure. */
typedef union RTCRRC4KEY
{
    uint64_t    au64Padding[(2 + 256) / 2];
#ifdef HEADER_RC4_H
    RC4_KEY     Ossl;
#endif
} RTCRRC4KEY;
/** Pointer to a RC4 key structure. */
typedef RTCRRC4KEY *PRTCRRC4KEY;
/** Pointer to a const RC4 key structure. */
typedef RTCRRC4KEY const *PCRTCRRC4KEY;

RTDECL(void) RTCrRc4SetKey(PRTCRRC4KEY pKey, size_t cbData, void const *pvData);
RTDECL(void) RTCrRc4(PRTCRRC4KEY pKey, size_t cbData, void const *pvDataIn, void *pvDataOut);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_rc4_h */

