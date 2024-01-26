/** @file
 * IPRT - Crypto - Miscellaneous.
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

#ifndef IPRT_INCLUDED_crypto_misc_h
#define IPRT_INCLUDED_crypto_misc_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_crmisc RTCrMisc - Miscellaneous
 * @ingroup grp_rt_crypto
 * @{
 */

/**
 * Get cryptographically strong random bytes.
 *
 * The buffer will always be filled with random bytes, however only
 * on @c VINF_SUCCESS is it guaranteed to be strong random bytes.
 *
 * @retval  VINF_SUCCESS
 * @retval  VERR_CR_RANDOM_FAILED if insufficient strong random bytes or some similar failure.
 * @retval  VERR_CR_RANDOM_SETUP_FAILED if setting up strong random failed
 *          and no strong bytes returned.
 *
 * @param   pvDst               Where to return the random bytes.
 * @param   cbDst               How many random bytes to return.
 */
RTDECL(int) RTCrRandBytes(void *pvDst, size_t cbDst);

RTDECL(int) RTCrPkcs5Pbkdf2Hmac(void const *pvInput, size_t cbInput, void const *pvSalt, size_t cbSalt, uint32_t cIterations,
                                RTDIGESTTYPE enmDigestType, size_t cbKeyLen, void *pvOutput);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_misc_h */

