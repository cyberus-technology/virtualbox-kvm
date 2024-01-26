/** @file
 * SoftFloat - VBox Extension - extF80_tan.
 */

/*
 * Copyright (C) 2022 Oracle and/or its affiliates.
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

extern const RTFLOAT128U g_r128pi2;

extFloat80_t extF80_tan( extFloat80_t x SOFTFLOAT_STATE_DECL_COMMA )
{
    int32_t fSign = 0;
    extFloat80_t v, f80Zero, f80One, f80Pi2;

    f80Zero = ui32_to_extF80(0, pState);
    f80One  = ui32_to_extF80(1, pState);
    f80Pi2  = f128_to_extF80(*(float128_t *)&g_r128pi2, pState);

    if (extF80_le(x, f80Zero, pState))
    {
        x = extF80_sub(f80Zero, x, pState);
        fSign = 1;
    }

    uint16_t fCxFlags = 0;
    extFloat80_t rem = extF80_partialRem(x, f80Pi2, pState->roundingMode, &fCxFlags, pState);
    int32_t const quo = X86_FSW_CX_TO_QUOTIENT(fCxFlags);

    v = extF80_div(rem, f80Pi2, pState);
    v = extF80_mul(v, v, pState);
    v = extF80_sub(f80One, v, pState);
    v = extF80_div(rem, v, pState);

    if (quo % 2)
    {
        v = extF80_div(f80One, v, pState);
        v = extF80_sub(f80Zero, v, pState);
    }

    if (fSign)
        v = extF80_sub(f80Zero, v, pState);

    return v;
}
