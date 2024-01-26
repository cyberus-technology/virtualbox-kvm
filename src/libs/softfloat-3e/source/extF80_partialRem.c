
/*============================================================================

This C source file is part of the SoftFloat IEEE Floating-Point Arithmetic
Package, Release 3e, by John R. Hauser.

Copyright 2011, 2012, 2013, 2014, 2015, 2016, 2017 The Regents of the
University of California.  All rights reserved.

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

#include <stdbool.h>
#include <stdint.h>
#include "platform.h"
#include "internals.h"
#include "specialize.h"
#include "softfloat.h"
#include <iprt/cdefs.h> /* RT_BIT_64 */
#include <iprt/x86.h>   /* X86_FSW_C? */
#include <iprt/assert.h>

/** VBox: Copy of extF80_rem modified to fit FPREM and FPREM1. */
extFloat80_t extF80_partialRem( extFloat80_t a, extFloat80_t b, uint8_t roundingMode,
                                uint16_t *pfCxFlags, softfloat_state_t *pState )
{
    union { struct extFloat80M s; extFloat80_t f; } uA;
    uint_fast16_t uiA64;
    uint_fast64_t uiA0;
    bool signA;
    int_fast32_t expA;
    uint_fast64_t sigA;
    union { struct extFloat80M s; extFloat80_t f; } uB;
    uint_fast16_t uiB64;
    uint_fast64_t uiB0;
    int_fast32_t expB;
    uint_fast64_t sigB;
    struct exp32_sig64 normExpSig;
    int_fast32_t expDiff;
    struct uint128 rem, shiftedSigB;
    uint_fast32_t q, recip32;
    uint_fast64_t q64;
    struct uint128 term, altRem, meanRem;
    bool signRem;
    struct uint128 uiZ;
    uint_fast16_t uiZ64;
    uint_fast64_t uiZ0;
    union { struct extFloat80M s; extFloat80_t f; } uZ;

    *pfCxFlags = 0; /* C2=0 - complete */                                                               /* VBox */

    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    uA.f = a;
    uiA64 = uA.s.signExp;
    uiA0  = uA.s.signif;
    signA = signExtF80UI64( uiA64 );
    expA  = expExtF80UI64( uiA64 );
    sigA  = uiA0;
    uB.f = b;
    uiB64 = uB.s.signExp;
    uiB0  = uB.s.signif;
    expB  = expExtF80UI64( uiB64 );
    sigB  = uiB0;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( expA == 0x7FFF ) {
        if (
               (sigA & UINT64_C( 0x7FFFFFFFFFFFFFFF ))                          /* VBox: NaN or Indefinite */
            || ((expB == 0x7FFF) && (sigB & UINT64_C( 0x7FFFFFFFFFFFFFFF )))    /* VBox: NaN or Indefinite */
        ) {
            goto propagateNaN;
        }
        goto invalid;                                                           /* VBox: Infinity */
    }
    if ( expB == 0x7FFF ) {
        if ( sigB & UINT64_C( 0x7FFFFFFFFFFFFFFF ) ) goto propagateNaN;         /* VBox: NaN or Indefinite */
        /*--------------------------------------------------------------------
        | Argument b is an infinity.  Doubling `expB' is an easy way to ensure
        | that `expDiff' later is less than -1, which will result in returning
        | a canonicalized version of argument a.
        *--------------------------------------------------------------------*/
        expB += expB;
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( ! expB ) expB = 1;
    if ( ! (sigB & UINT64_C( 0x8000000000000000 )) ) {
        if ( ! sigB ) goto invalid;                                             /* VBox: Zero -> /0 -> invalid. */
        normExpSig = softfloat_normSubnormalExtF80Sig( sigB );
        expB += normExpSig.exp;
        sigB = normExpSig.sig;
    }
    if ( ! expA ) expA = 1;
    if ( ! (sigA & UINT64_C( 0x8000000000000000 )) ) {
        if ( ! sigA ) {
#if 0 /* A is zero. VBox: Don't mix denormals and zero returns! */                                      /* VBox */
            expA = 0;
            goto copyA;
#else                                                                                                   /* VBox */
            uiZ64 = packToExtF80UI64( signA, 0);                                                        /* VBox */
            uiZ0  = 0;                                                                                  /* VBox */
            goto uiZ;                                                                                   /* VBox */
#endif                                                                                                  /* VBox */
        }
        normExpSig = softfloat_normSubnormalExtF80Sig( sigA );
        expA += normExpSig.exp;
        sigA = normExpSig.sig;
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    expDiff = expA - expB;

    /*------------------------------------------------------------------------                             VBox
    | Do at most 63 rounds. If exponent difference is 64 or higher, return                                 VBox
    | a partial remainder.                                                                                 VBox
    *------------------------------------------------------------------------*/                         /* VBox */
    bool const fPartial = expDiff >= 64;                                                                /* VBox */
    if ( fPartial ) { /* VBox */                                                                        /* VBox */
        unsigned N = 32 + ((unsigned)expDiff % 32); /* (Amount of work documented by AMD.) */           /* VBox */
        expB         = expA - N;                                                                        /* VBox */
        expDiff      = N;                                                                               /* VBox */
        roundingMode = softfloat_round_minMag;                                                          /* VBox */
    }                                                                                                   /* VBox */

    /*------------------------------------------------------------------------
    | The final rounds.
    *------------------------------------------------------------------------*/
    if ( expDiff < -1 ) goto copyA;
    rem = softfloat_shortShiftLeft128( 0, sigA, 32 );
    shiftedSigB = softfloat_shortShiftLeft128( 0, sigB, 32 );
    uint64_t quotient = 0;                                                                              /* VBox */
    if ( expDiff < 1 ) {
        if ( expDiff ) {
            --expB;
            shiftedSigB = softfloat_shortShiftLeft128( 0, sigB, 33 );
            q = 0;
        } else {
            q = (sigB <= sigA);
            quotient = q;                                                                               /* VBox */
            if ( q ) {
                rem =
                    softfloat_sub128(
                        rem.v64, rem.v0, shiftedSigB.v64, shiftedSigB.v0 );
            }
        }
    } else {
        recip32 = softfloat_approxRecip32_1( sigB>>32 );
        expDiff -= 30;
        for (;;) {
            q64 = (uint_fast64_t) (uint32_t) (rem.v64>>2) * recip32;
            if ( expDiff < 0 ) break;
            q = (q64 + 0x80000000)>>32;
            quotient = (quotient << 29) + q;                                                            /* VBox */
            rem = softfloat_shortShiftLeft128( rem.v64, rem.v0, 29 );
            term = softfloat_mul64ByShifted32To128( sigB, q );
            rem = softfloat_sub128( rem.v64, rem.v0, term.v64, term.v0 );
            if ( rem.v64 & UINT64_C( 0x8000000000000000 ) ) {
                rem =
                    softfloat_add128(
                        rem.v64, rem.v0, shiftedSigB.v64, shiftedSigB.v0 );
                quotient--;                                                                             /* VBox */
            }
            expDiff -= 29;
        }
        /*--------------------------------------------------------------------
        | (`expDiff' cannot be less than -29 here.)
        *--------------------------------------------------------------------*/
        q = (uint32_t) (q64>>32)>>(~expDiff & 31);
        quotient = (quotient << (expDiff + 30)) + q;                                                    /* VBox */
        rem = softfloat_shortShiftLeft128( rem.v64, rem.v0, expDiff + 30 );
        term = softfloat_mul64ByShifted32To128( sigB, q );
        rem = softfloat_sub128( rem.v64, rem.v0, term.v64, term.v0 );
        if ( rem.v64 & UINT64_C( 0x8000000000000000 ) ) {
            altRem =
                softfloat_add128(
                    rem.v64, rem.v0, shiftedSigB.v64, shiftedSigB.v0 );
            goto selectRem;
        }
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    do {
        altRem = rem;
        ++q;
        quotient++;                                                                                     /* VBox */
        rem =
            softfloat_sub128(
                rem.v64, rem.v0, shiftedSigB.v64, shiftedSigB.v0 );
    } while ( ! (rem.v64 & UINT64_C( 0x8000000000000000 )) );
 selectRem:
    if (roundingMode == softfloat_round_near_even) {                                                    /* VBox */
        meanRem = softfloat_add128( rem.v64, rem.v0, altRem.v64, altRem.v0 );
        if (
            (meanRem.v64 & UINT64_C( 0x8000000000000000 ))
                || (! (meanRem.v64 | meanRem.v0) && (q & 1))
        ) {
            rem = altRem;
            quotient--;                                                                                 /* VBox */
        }
    }                                                                                                   /* VBox */
    signRem = signA;
    if ( rem.v64 & UINT64_C( 0x8000000000000000 ) ) {
        if (roundingMode != softfloat_round_near_even) {                                                /* VBox */
            rem = altRem;                                                                               /* VBox */
            quotient--;                                                                                 /* VBox */
        } else {                                                                                        /* VBox */
            signRem = ! signRem;
            rem = softfloat_sub128( 0, 0, rem.v64, rem.v0 );
            Assert(!fPartial);                                                                          /* VBox */
        }                                                                                               /* VBox */
    } else Assert(roundingMode == softfloat_round_near_even);                                           /* VBox */

    /* VBox: Set pfCxFlags */                                                                           /* VBox */
    if ( fPartial ) {                                                                                   /* VBox */
        *pfCxFlags = X86_FSW_C2;                            /* C2 = 1 - incomplete */                   /* VBox */
    } else {                                                                                            /* VBox */
        *pfCxFlags = X86_FSW_CX_FROM_QUOTIENT( quotient );  /* C2 = 0 - complete */                     /* VBox */
    }                                                                                                   /* VBox */

    return
        softfloat_normRoundPackToExtF80(
            signRem, rem.v64 | rem.v0 ? expB + 32 : 0, rem.v64, rem.v0, 80 SOFTFLOAT_STATE_ARG_COMMA );
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 propagateNaN:
    uiZ = softfloat_propagateNaNExtF80UI( uiA64, uiA0, uiB64, uiB0 SOFTFLOAT_STATE_ARG_COMMA );
    uiZ64 = uiZ.v64;
    uiZ0  = uiZ.v0;
    goto uiZ;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 invalid:
    softfloat_raiseFlags( softfloat_flag_invalid SOFTFLOAT_STATE_ARG_COMMA );
    uiZ64 = defaultNaNExtF80UI64;
    uiZ0  = defaultNaNExtF80UI0;
    goto uiZ;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 copyA:
    if ( expA < 1 ) {
#if 0                                                                                                   /* VBox */
        sigA >>= 1 - expA;
        expA = 0;
#else                                                                                                   /* VBox */
        Assert(sigA != 0); /* We don't get here for zero values, only denormals. */                     /* VBox */
        /* Apply the bias adjust if underflows exceptions aren't masked, unless                            VBox
           the divisor is +/-Infinity.                                                                     VBox
           Note! extB has been tweaked, so don't use if for Inf classification. */                      /* VBox */
        if (    (pState->exceptionMask & softfloat_flag_underflow)                                      /* VBox */
             || (expExtF80UI64( b.signExp ) == 0x7fff && !(sigB & (RT_BIT_64( 63 ) - 1))) ) {           /* VBox */
            sigA >>= 1 - expA;                                                                          /* VBox */
            expA = 0;                                                                                   /* VBox */
        } else {                                                                                        /* VBox */
            softfloat_raiseFlags( softfloat_flag_underflow SOFTFLOAT_STATE_ARG_COMMA );                 /* VBox */
            expA = (expA + RTFLOAT80U_EXP_BIAS_ADJUST) & RTFLOAT80U_EXP_MAX;                            /* VBox */
        }                                                                                               /* VBox */
#endif                                                                                                  /* VBox */
    }
    uiZ64 = packToExtF80UI64( signA, expA );
    uiZ0  = sigA;
 uiZ:
    uZ.s.signExp = uiZ64;
    uZ.s.signif  = uiZ0;
    return uZ.f;

}

