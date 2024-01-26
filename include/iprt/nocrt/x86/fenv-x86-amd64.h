/** @file
 * IPRT / No-CRT - x86 & AMD64 fenv.h.
 */

/*
 * Copyright (C) 2022-2023 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_nocrt_x86_fenv_x86_amd64_h
#define IPRT_INCLUDED_nocrt_x86_fenv_x86_amd64_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>

typedef struct RTNOCRTFENV
{
    /** The FPU environment. */
    union
    {
        uint32_t        au32[28/4];
#ifdef IPRT_INCLUDED_x86_h
        X86FSTENV32P    Env;
#endif
    } fpu;
    /** The SSE control & status register. */
    uint32_t            fMxCsr;
} RTNOCRTFENV;

/** Exception flags/mask. */
typedef uint16_t RTNOCRTFEXCEPT;

#ifndef IPRT_NOCRT_WITHOUT_CONFLICTING_TYPES
typedef RTNOCRTFENV     fenv_t;
typedef RTNOCRTFEXCEPT  fexcept_t;
#endif

/** @name Exception flags (same as X86_FCW_xM, X86_FSW_xE, X86_MXCSR_xE)
 * @note The X86_FSW_SF is not covered here as it is more of a sub-type of
 *       invalid operand exception, and it is not part of MXCSR.
 * @{ */
#define RT_NOCRT_FE_INVALID      0x0001
#define RT_NOCRT_FE_DENORMAL     0x0002
#define RT_NOCRT_FE_DIVBYZERO    0x0004
#define RT_NOCRT_FE_OVERFLOW     0x0008
#define RT_NOCRT_FE_UNDERFLOW    0x0010
#define RT_NOCRT_FE_INEXACT      0x0020
#define RT_NOCRT_FE_ALL_EXCEPT   0x003f
#ifndef IPRT_NOCRT_WITHOUT_MATH_CONSTANTS
# define FE_INVALID              RT_NOCRT_FE_INVALID
# define FE_DENORMAL             RT_NOCRT_FE_DENORMAL
# define FE_DIVBYZERO            RT_NOCRT_FE_DIVBYZERO
# define FE_OVERFLOW             RT_NOCRT_FE_OVERFLOW
# define FE_UNDERFLOW            RT_NOCRT_FE_UNDERFLOW
# define FE_INEXACT              RT_NOCRT_FE_INEXACT
# define FE_ALL_EXCEPT           RT_NOCRT_FE_ALL_EXCEPT
#endif
/** @} */

/** @name Rounding Modes (same X86_FCW_RC_XXX)
 * @{ */
#define RT_NOCRT_FE_TONEAREST    0x0000
#define RT_NOCRT_FE_DOWNWARD     0x0400
#define RT_NOCRT_FE_UPWARD       0x0800
#define RT_NOCRT_FE_TOWARDZERO   0x0c00
#define RT_NOCRT_FE_ROUND_MASK   0x0c00
#ifndef IPRT_NOCRT_WITHOUT_MATH_CONSTANTS
# define FE_TONEAREST            RT_NOCRT_FE_TONEAREST
# define FE_DOWNWARD             RT_NOCRT_FE_DOWNWARD
# define FE_UPWARD               RT_NOCRT_FE_UPWARD
# define FE_TOWARDZERO           RT_NOCRT_FE_TOWARDZERO
#endif
/** @} */


/** @name x87 Precision (same X86_FCW_PC_XXX)
 * @{ */
#define RT_NOCRT_PC_FLOAT        0x0000
#define RT_NOCRT_PC_RSVD         0x0100
#define RT_NOCRT_PC_DOUBLE       0x0200
#define RT_NOCRT_PC_EXTENDED     0x0300
#define RT_NOCRT_PC_MASK         0x0300
/** @} */


/** @name Special environment pointer values.
 * @note Only valid with fesetenv and feupdateenv.
 * @note Defined as constants in fesetenv.asm.
 * @{ */
/** The default FPU+SSE environment set, all exceptions disabled (masked). */
#define RT_NOCRT_FE_DFL_ENV     ((RTNOCRTFENV const *)(intptr_t)1)
/** The default FPU+SSE environment set, but all exceptions enabled (unmasked)
 *  except for RT_NOCRT_FE_DENORMAL. */
#define RT_NOCRT_FE_NOMASK_ENV  ((RTNOCRTFENV const *)(intptr_t)2)
/** The default FPU+SSE environment set, all exceptions disabled (masked),
 *  double precision (53 bit mantissa). */
#define RT_NOCRT_FE_PC53_ENV    ((RTNOCRTFENV const *)(intptr_t)3)
/** The default FPU+SSE environment set, all exceptions disabled (masked),
 *  extended double precision (64 bit mantissa). */
#define RT_NOCRT_FE_PC64_ENV    ((RTNOCRTFENV const *)(intptr_t)4)
#ifndef IPRT_NOCRT_WITHOUT_MATH_CONSTANTS
# define FE_DFL_ENV              RT_NOCRT_FE_DFL_ENV
# define FE_NOMASK_ENV           RT_NOCRT_FE_NOMASK_ENV
# define FE_PC53_ENV             RT_NOCRT_FE_PC53_ENV
# define FE_PC64_ENV             RT_NOCRT_FE_PC64_ENV
#endif
/** @} */

RT_C_DECLS_BEGIN

int     RT_NOCRT(fegetenv)(RTNOCRTFENV *);
int     RT_NOCRT(fesetenv)(RTNOCRTFENV const *);
int     RT_NOCRT(feholdexcept)(RTNOCRTFENV *);
int     RT_NOCRT(feupdateenv)(RTNOCRTFENV const *);

int     RT_NOCRT(fegetround)(void);
int     RT_NOCRT(fesetround)(int);

int     RT_NOCRT(fegetexcept)(void);
int     RT_NOCRT(feenableexcept)(int);
int     RT_NOCRT(fedisableexcept)(int);

int     RT_NOCRT(feclearexcept)(int);
int     RT_NOCRT(fetestexcept)(int);
int     RT_NOCRT(fegetexceptflag)(RTNOCRTFEXCEPT *, int);
int     RT_NOCRT(fesetexceptflag)(RTNOCRTFEXCEPT const *, int);

int     RT_NOCRT(feraiseexcept)(int);

/* IPRT addition:  */
int     RT_NOCRT(fegetx87precision)(void);
int     RT_NOCRT(fesetx87precision)(int);

/* Underscored variants: */
int     RT_NOCRT(_fegetenv)(RTNOCRTFENV *);
int     RT_NOCRT(_fesetenv)(RTNOCRTFENV const *);
int     RT_NOCRT(_feholdexcept)(RTNOCRTFENV *);
int     RT_NOCRT(_feupdateenv)(RTNOCRTFENV const *);

int     RT_NOCRT(_fegetround)(void);
int     RT_NOCRT(_fesetround)(int);

int     RT_NOCRT(_fegetexcept)(void);
int     RT_NOCRT(_feenableexcept)(int);
int     RT_NOCRT(_fedisableexcept)(int);

int     RT_NOCRT(_feclearexcept)(int);
int     RT_NOCRT(_fetestexcept)(int);
int     RT_NOCRT(_fegetexceptflag)(RTNOCRTFEXCEPT *, int);
int     RT_NOCRT(_fesetexceptflag)(RTNOCRTFEXCEPT const *, int);

int     RT_NOCRT(_feraiseexcept)(int);

/* Aliases: */
#if !defined(RT_WITHOUT_NOCRT_WRAPPERS) && !defined(RT_WITHOUT_NOCRT_WRAPPER_ALIASES)
# define fegetenv           RT_NOCRT(fegetenv)
# define fesetenv           RT_NOCRT(fesetenv)
# define feholdexcept       RT_NOCRT(feholdexcept)
# define feupdateenv        RT_NOCRT(feupdateenv)
# define fegetround         RT_NOCRT(fegetround)
# define fesetround         RT_NOCRT(fesetround)
# define fegetexcept        RT_NOCRT(fegetexcept)
# define feenableexcept     RT_NOCRT(feenableexcept)
# define fedisableexcept    RT_NOCRT(fedisableexcept)
# define feclearexcept      RT_NOCRT(feclearexcept)
# define fetestexcept       RT_NOCRT(fetestexcept)
# define fegetexceptflag    RT_NOCRT(fegetexceptflag)
# define fesetexceptflag    RT_NOCRT(fesetexceptflag)
# define feraiseexcept      RT_NOCRT(feraiseexcept)

/* Underscored variants: */
# define _fegetenv          RT_NOCRT(fegetenv)
# define _fesetenv          RT_NOCRT(fesetenv)
# define _feholdexcept      RT_NOCRT(feholdexcept)
# define _feupdateenv       RT_NOCRT(feupdateenv)
# define _fegetround        RT_NOCRT(fegetround)
# define _fesetround        RT_NOCRT(fesetround)
# define _fegetexcept       RT_NOCRT(fegetexcept)
# define _feenableexcept    RT_NOCRT(feenableexcept)
# define _fedisableexcept   RT_NOCRT(fedisableexcept)
# define _feclearexcept     RT_NOCRT(feclearexcept)
# define _fetestexcept      RT_NOCRT(fetestexcept)
# define _fegetexceptflag   RT_NOCRT(fegetexceptflag)
# define _fesetexceptflag   RT_NOCRT(fesetexceptflag)
# define _feraiseexcept     RT_NOCRT(feraiseexcept)
#endif

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_nocrt_x86_fenv_x86_amd64_h */

