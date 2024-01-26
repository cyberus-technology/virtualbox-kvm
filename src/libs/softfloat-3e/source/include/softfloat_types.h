
/*============================================================================

This C header file is part of the SoftFloat IEEE Floating-Point Arithmetic
Package, Release 3e, by John R. Hauser.

Copyright 2011, 2012, 2013, 2014, 2015, 2017 The Regents of the University of
California.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions, and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions, and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 3. Neither the name of the University nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS", AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE
DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

#ifndef softfloat_types_h
#define softfloat_types_h 1

#include <stdint.h>

/*----------------------------------------------------------------------------
| Types used to pass 16-bit, 32-bit, 64-bit, and 128-bit floating-point
| arguments and results to/from functions.  These types must be exactly
| 16 bits, 32 bits, 64 bits, and 128 bits in size, respectively.  Where a
| platform has "native" support for IEEE-Standard floating-point formats,
| the types below may, if desired, be defined as aliases for the native types
| (typically 'float' and 'double', and possibly 'long double').
*----------------------------------------------------------------------------*/
typedef struct { uint16_t v; } float16_t;
typedef struct { uint32_t v; } float32_t;
typedef struct { uint64_t v; } float64_t;
typedef struct { uint64_t v[2]; } float128_t;

/*----------------------------------------------------------------------------
| The format of an 80-bit extended floating-point number in memory.  This
| structure must contain a 16-bit field named 'signExp' and a 64-bit field
| named 'signif'.
*----------------------------------------------------------------------------*/
#ifdef LITTLEENDIAN
struct extFloat80M { uint64_t signif; uint16_t signExp; };
# define EXTFLOAT80M_INIT( a_signExp, a_signif )    { a_signif, a_signExp }                             /* VBox */
#else
struct extFloat80M { uint16_t signExp; uint64_t signif; };
# define EXTFLOAT80M_INIT( a_signExp, a_signif )    { a_signExp, a_signif }                             /* VBox */
#endif
#define EXTFLOAT80M_INIT_C( a_signExp, a_signif )   EXTFLOAT80M_INIT( a_signExp, UINT64_C( a_signif ) ) /* VBox */
#define EXTFLOAT80M_INIT3( a_sign, a_signif, a_exp ) \
    EXTFLOAT80M_INIT( packToExtF80UI64( a_sign, a_exp ), a_signif)                                      /* VBox */
#define EXTFLOAT80M_INIT3_C( a_sign, a_signif, a_exp ) \
    EXTFLOAT80M_INIT3( a_sign, UINT64_C( a_signif ), a_exp )                                            /* VBox */

/*----------------------------------------------------------------------------
| The type used to pass 80-bit extended floating-point arguments and
| results to/from functions.  This type must have size identical to
| 'struct extFloat80M'.  Type 'extFloat80_t' can be defined as an alias for
| 'struct extFloat80M'.  Alternatively, if a platform has "native" support
| for IEEE-Standard 80-bit extended floating-point, it may be possible,
| if desired, to define 'extFloat80_t' as an alias for the native type
| (presumably either 'long double' or a nonstandard compiler-intrinsic type).
| In that case, the 'signif' and 'signExp' fields of 'struct extFloat80M'
| must align exactly with the locations in memory of the sign, exponent, and
| significand of the native type.
*----------------------------------------------------------------------------*/
typedef struct extFloat80M extFloat80_t;

/*----------------------------------------------------------------------------
| VBox: The four globals as non-globals.
*----------------------------------------------------------------------------*/
#ifndef VBOX_WITH_SOFTFLOAT_GLOBALS
# define VBOX_WITHOUT_SOFTFLOAT_GLOBALS
# define SOFTFLOAT_STATE_ARG            pState
# define SOFTFLOAT_STATE_ARG_COMMA      , pState
# define SOFTFLOAT_STATE_DECL           softfloat_state_t *pState
# define SOFTFLOAT_STATE_DECL_COMMA     , softfloat_state_t *pState
# define SOFTFLOAT_STATE_NOREF()        (void)pState
typedef struct softfloat_state
{
    /* softfloat_tininess_beforeRounding or softfloat_tininess_afterRounding */
    uint8_t detectTininess;
    /* softfloat_round_near_even and friends. */
    uint8_t roundingMode;
    /* softfloat_flag_inexact and friends. */
    uint8_t exceptionFlags;
    /** Masked exceptions (only underflow is relevant). */
    uint8_t exceptionMask;
    /* extF80: rounding precsision: 32, 64 or 80 */
    uint8_t roundingPrecision;
} softfloat_state_t;
# define SOFTFLOAT_STATE_INIT_DEFAULTS() { softfloat_tininess_afterRounding, softfloat_round_near_even, 0, 0x3f, 80 }
#else
# undef  VBOX_WITHOUT_SOFTFLOAT_GLOBALS
# define SOFTFLOAT_STATE_ARG
# define SOFTFLOAT_STATE_ARG_COMMA
# define SOFTFLOAT_STATE_DECL
# define SOFTFLOAT_STATE_DECL_COMMA
# define SOFTFLOAT_STATE_NOREF()        (void)0
#endif

#endif

