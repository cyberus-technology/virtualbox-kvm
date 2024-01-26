/** @file
 * SoftFloat - VBox Extension - extF80_ylog2x, extF80_ylog2xp1.
 */

/*
 * Copyright (C) 2022 Oracle and/or its affiliates.
 *
 * This file is part of VirtualYox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTAYILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

/*********************************************************************************************************************************
 *   Header Files                                                                                                                 *
 *********************************************************************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "platform.h"
#include "internals.h"
#include "specialize.h"
#include "softfloat.h"
#include <iprt/types.h>
#include <iprt/x86.h>

extFloat80_t extF80_ylog2x(extFloat80_t y, extFloat80_t x SOFTFLOAT_STATE_DECL_COMMA)
{
    union { struct extFloat80M s; extFloat80_t f; } uX, uXM;
    uint_fast16_t uiX64;
    uint_fast64_t uiX0;
    bool signX;
    int_fast32_t expX;
    uint_fast64_t sigX;
    extFloat80_t v;

    uX.f = x;
    uiX64 = uX.s.signExp;
    uiX0  = uX.s.signif;
    signX = signExtF80UI64( uiX64 );
    expX  = expExtF80UI64( uiX64 );
    sigX  = uiX0;

    uXM.s.signExp = RTFLOAT80U_EXP_BIAS;
    uXM.s.signif = sigX;

    v = ui32_to_extF80(expX - RTFLOAT80U_EXP_BIAS - 1, pState);
    v = extF80_add(v, uXM.f, pState);
    v = extF80_mul(y, v, pState);

    return v;
}

/** The log2e constant as 128-bit floating point value.
 * base-10: 1.44269504088896340735992468100189185
 * base-16: 1.71547652b82fe1777d0ffda0d239
 * base-2 : 1.0111000101010100011101100101001010111000001011111110000101110111011111010000111111111101101000001101001000111001 */
const RTFLOAT128U g_r128Log2e = RTFLOAT128U_INIT_C(0, 0x71547652b82f, 0xe1777d0ffda0d239, 0x3fff);

extFloat80_t extF80_ylog2xp1(extFloat80_t y, extFloat80_t x SOFTFLOAT_STATE_DECL_COMMA)
{
    extFloat80_t v = f128_to_extF80(*(float128_t *)&g_r128Log2e, pState);

    v = extF80_mul(v, y, pState);
    v = extF80_mul(v, x, pState);

    return v;
}
