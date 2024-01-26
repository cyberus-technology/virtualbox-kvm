
/*============================================================================

This C source file is part of the SoftFloat IEEE Floating-Point Arithmetic
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

#include <stdbool.h>
#include <stdint.h>
#include "platform.h"
#include "internals.h"
#include "softfloat.h"
#include <iprt/types.h> /* VBox: RTFLOAT80U_EXP_BIAS_ADJUST */
#include <iprt/assert.h>

static extFloat80_t
 softfloat_roundPackToExtF80Inner(
     bool sign,
     int_fast32_t exp,
     uint_fast64_t sig,
     uint_fast64_t sigExtra,
     uint_fast8_t roundingPrecision
     SOFTFLOAT_STATE_DECL_COMMA
 )
{
    uint_fast8_t roundingMode;
    bool roundNearEven;
    uint_fast64_t roundIncrement, roundMask, roundBits;
    bool isTiny, doIncrement = 0;
    struct uint64_extra sig64Extra;
    union { struct extFloat80M s; extFloat80_t f; } uZ;
    //RTAssertMsg2("softfloat_roundPackToExtF80: exp=%d sig=%RX64 sigExtra=%RX64 rp=%d\n", exp, sig, sigExtra, roundingPrecision);

    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    roundingMode = softfloat_roundingMode;
    roundNearEven = (roundingMode == softfloat_round_near_even);
    if ( roundingPrecision == 80 ) goto precision80;
    if ( roundingPrecision == 64 ) {
        roundIncrement = UINT64_C( 0x0000000000000400 );
        roundMask = UINT64_C( 0x00000000000007FF );
    } else if ( roundingPrecision == 32 ) {
        roundIncrement = UINT64_C( 0x0000008000000000 );
        roundMask = UINT64_C( 0x000000FFFFFFFFFF );
    } else {
        goto precision80;
    }
    sig |= (sigExtra != 0);
    if ( ! roundNearEven && (roundingMode != softfloat_round_near_maxMag) ) {
        roundIncrement =
            (roundingMode
                 == (sign ? softfloat_round_min : softfloat_round_max))
                ? roundMask
                : 0;
    }
    roundBits = sig & roundMask;
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( 0x7FFD <= (uint32_t) (exp - 1) ) {
        if ( exp <= 0 ) {
            /*----------------------------------------------------------------
            *----------------------------------------------------------------*/
            isTiny =
                   (softfloat_detectTininess
                        == softfloat_tininess_beforeRounding)
                || (exp < 0)
                || (sig <= (uint64_t) (sig + roundIncrement));
            sig = softfloat_shiftRightJam64(sig, 1 - exp);
            uint64_t const uOldSig = sig;                                                               /* VBox: C1 */
            roundBits = sig & roundMask;
            if ( roundBits ) {
                if ( isTiny ) softfloat_raiseFlags( softfloat_flag_underflow SOFTFLOAT_STATE_ARG_COMMA );
                softfloat_exceptionFlags |= softfloat_flag_inexact;
#ifdef SOFTFLOAT_ROUND_ODD
                if ( roundingMode == softfloat_round_odd ) {
                    sig |= roundMask + 1;
                }
#endif
            }
            sig += roundIncrement;
            exp = ((sig & UINT64_C( 0x8000000000000000 )) != 0);
            roundIncrement = roundMask + 1;
            if ( roundNearEven && (roundBits<<1 == roundIncrement) ) {
                roundMask |= roundIncrement;
            }
            sig &= ~roundMask;
            if ( sig > uOldSig ) {                                                                      /* VBox: C1 */
                softfloat_exceptionFlags |= softfloat_flag_c1;                                          /* VBox: C1 */
                //RTAssertMsg2("softfloat_roundPackToExtF80: C1 #1\n");                                 /* VBox: C1 */
            }                                                                                           /* VBox: C1 */
            goto packReturn;
        }
        if (
               (0x7FFE < exp)
            || ((exp == 0x7FFE) && ((uint64_t) (sig + roundIncrement) < sig))
        ) {
            goto overflow;
        }
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    {                                                                                                   /* VBox: C1 */
    uint64_t const uOldSig = sig;                                                                       /* VBox: C1 */
    if ( roundBits ) {
        softfloat_exceptionFlags |= softfloat_flag_inexact;
#ifdef SOFTFLOAT_ROUND_ODD
        if ( roundingMode == softfloat_round_odd ) {
            sig = (sig & ~roundMask) | (roundMask + 1);
            if ( sig > uOldSig ) {                                                                      /* VBox: C1 */
                softfloat_exceptionFlags |= softfloat_flag_c1;                                          /* VBox: C1 */
                //RTAssertMsg2("softfloat_roundPackToExtF80: C1 #2\n");                                 /* VBox: C1 */
            }                                                                                           /* VBox: C1 */
            goto packReturn;
        }
#endif
    }
    sig = (uint64_t) (sig + roundIncrement);
    if ( sig < roundIncrement ) {
        ++exp;
        sig = UINT64_C( 0x8000000000000000 );
        softfloat_exceptionFlags |= softfloat_flag_c1;                                                  /* VBox: C1 */
        //RTAssertMsg2("softfloat_roundPackToExtF80: C1 #3\n");                                         /* VBox: C1 */
    }
    roundIncrement = roundMask + 1;
    if ( roundNearEven && (roundBits<<1 == roundIncrement) ) {
        roundMask |= roundIncrement;
    }
    sig &= ~roundMask;
    if ( sig > uOldSig ) {                                                                              /* VBox: C1 */
        softfloat_exceptionFlags |= softfloat_flag_c1;                                                  /* VBox: C1 */
        //RTAssertMsg2("softfloat_roundPackToExtF80: C1 #4\n");                                         /* VBox: C1 */
    }                                                                                                   /* VBox: C1 */
    goto packReturn;
    }                                                                                                   /* VBox: C1 */
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 precision80:
    doIncrement = (UINT64_C( 0x8000000000000000 ) <= sigExtra);
    if ( ! roundNearEven && (roundingMode != softfloat_round_near_maxMag) ) {
        doIncrement =
            (roundingMode
                 == (sign ? softfloat_round_min : softfloat_round_max))
                && sigExtra;
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( 0x7FFD <= (uint32_t) (exp - 1) ) {
        if ( exp <= 0 ) {
            /*----------------------------------------------------------------
            *----------------------------------------------------------------*/
            isTiny =
                   (softfloat_detectTininess
                        == softfloat_tininess_beforeRounding)
                || (exp < 0)
                || ! doIncrement
                || (sig < UINT64_C( 0xFFFFFFFFFFFFFFFF ));
//RTAssertMsg2("softfloat_roundPackToExtF80: #2: sig=%#RX64 sigExtra=%#RX64 isTiny=%d exp=%d 1-exp=%d\n", sig, sigExtra, isTiny, exp, 1-exp);
            sig64Extra =
                softfloat_shiftRightJam64Extra( sig, sigExtra, 1 - exp );
            if (   (exp < -63 || sig64Extra.extra != 0)                                                 /* VBox: Missing inexact result flag */
                && ( sig != 0 || sigExtra != 0 ) /*!zero*/ ) {                                          /* VBox: Missing inexact result flag */
                softfloat_exceptionFlags |= softfloat_flag_inexact;                                     /* VBox: Missing inexact result flag */
            }                                                                                           /* VBox: Missing inexact result flag */
            exp = 0;
            sig = sig64Extra.v;
            sigExtra = sig64Extra.extra;
//RTAssertMsg2("softfloat_roundPackToExtF80: #3: sig=%#RX64 sigExtra=%#RX64 isTiny=%d\n", sig, sigExtra, isTiny);
            if ( sigExtra
                 || (   !(pState->exceptionMask & softfloat_flag_underflow)                             /* VBox: Unmasked underflow conditions differ */
                     && (sig != 0 || sigExtra != 0) /*zero*/ ) ) {                                      /* VBox: Unmasked underflow conditions differ */
                if ( isTiny ) softfloat_raiseFlags( softfloat_flag_underflow SOFTFLOAT_STATE_ARG_COMMA );
#ifdef SOFTFLOAT_ROUND_ODD
                if ( roundingMode == softfloat_round_odd ) {
                    sig |= 1;
                    goto packReturn;
                }
#endif
            }
            doIncrement = (UINT64_C( 0x8000000000000000 ) <= sigExtra);
            if (
                ! roundNearEven
                    && (roundingMode != softfloat_round_near_maxMag)
            ) {
                doIncrement =
                    (roundingMode
                         == (sign ? softfloat_round_min : softfloat_round_max))
                        && sigExtra;
            }
            if ( doIncrement ) {
                uint64_t const uOldSig = sig;                                                           /* VBox: C1 */
                ++sig;
                sig &=
                    ~(uint_fast64_t)
                         (! (sigExtra & UINT64_C( 0x7FFFFFFFFFFFFFFF ))
                              & roundNearEven);
                if (sig > uOldSig) {                                                                    /* VBox: C1 */
                    softfloat_exceptionFlags |= softfloat_flag_c1;                                      /* VBox: C1 */
                    //RTAssertMsg2("softfloat_roundPackToExtF80: C1 #5\n");                             /* VBox: C1 */
                }                                                                                       /* VBox: C1 */
                exp = ((sig & UINT64_C( 0x8000000000000000 )) != 0);
            }
            goto packReturn;
        }
        if (
               (0x7FFE < exp)
            || ((exp == 0x7FFE) && (sig == UINT64_C( 0xFFFFFFFFFFFFFFFF ))
                    && doIncrement)
        ) {
            /*----------------------------------------------------------------
            *----------------------------------------------------------------*/
            roundMask = 0;
 overflow:
            softfloat_raiseFlags(
                softfloat_flag_overflow | softfloat_flag_inexact
                SOFTFLOAT_STATE_ARG_COMMA );
            if (
                   roundNearEven
                || (roundingMode == softfloat_round_near_maxMag)
                || (roundingMode
                        == (sign ? softfloat_round_min : softfloat_round_max))
            ) {
                exp = 0x7FFF;
                sig = UINT64_C( 0x8000000000000000 );
                softfloat_exceptionFlags |= softfloat_flag_c1; /* Inf means rounding up */              /* VBox: C1 */
                //RTAssertMsg2("softfloat_roundPackToExtF80: C1 #6\n");                                 /* VBox: C1 */
            } else {
                exp = 0x7FFE;
                sig = ~roundMask;
            }
            goto packReturn;
        }
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
    if ( sigExtra ) {
        softfloat_exceptionFlags |= softfloat_flag_inexact;
#ifdef SOFTFLOAT_ROUND_ODD
        if ( roundingMode == softfloat_round_odd ) {
            sig |= 1;
            goto packReturn;
        }
#endif
    }
    if ( doIncrement ) {
        uint64_t const uOldSig = sig; /* VBox */
        ++sig;
        if ( ! sig ) {
            ++exp;
            sig = UINT64_C( 0x8000000000000000 );
            softfloat_exceptionFlags |= softfloat_flag_c1;                                              /* VBox: C1 */
            //RTAssertMsg2("softfloat_roundPackToExtF80: C1 #7\n");                                     /* VBox: C1 */
        } else {
            sig &=
                ~(uint_fast64_t)
                     (! (sigExtra & UINT64_C( 0x7FFFFFFFFFFFFFFF ))
                          & roundNearEven);
            if ( sig > uOldSig ) {                                                                      /* VBox: C1 */
                softfloat_exceptionFlags |= softfloat_flag_c1;                                          /* VBox: C1 */
                //RTAssertMsg2("softfloat_roundPackToExtF80: C1 #8\n");                                 /* VBox: C1 */
            }
        }
    }
    /*------------------------------------------------------------------------
    *------------------------------------------------------------------------*/
 packReturn:
    uZ.s.signExp = packToExtF80UI64( sign, exp );
    uZ.s.signif = sig;
    return uZ.f;

}

/** 
 * VBox: Wrapper for implementing underflow and overflow bias adjustment.
 */
extFloat80_t
 softfloat_roundPackToExtF80(
     bool sign,
     int_fast32_t exp,
     uint_fast64_t sig,
     uint_fast64_t sigExtra,
     uint_fast8_t roundingPrecision
     SOFTFLOAT_STATE_DECL_COMMA
 )
{
    static union extF80M_extF80 const s_aExtF80Zero[2]     =
    {
        EXTF80M_EXTF80_INIT3_C( 0, 0, 0 ), EXTF80M_EXTF80_INIT3_C( 1, 0, 0 ),
    };
    static union extF80M_extF80 const s_aExtF80Infinity[2] =
    {
        EXTF80M_EXTF80_INIT3( 0, RT_BIT_64( 63 ), RTFLOAT80U_EXP_MAX ),
        EXTF80M_EXTF80_INIT3( 1, RT_BIT_64( 63 ), RTFLOAT80U_EXP_MAX ),
    };

    uint8_t const exceptionFlagsSaved = softfloat_exceptionFlags;
    softfloat_exceptionFlags = 0;
    extFloat80_t r80Result = softfloat_roundPackToExtF80Inner( sign, exp, sig, sigExtra, roundingPrecision, pState );
    if ( !(softfloat_exceptionFlags & ~pState->exceptionMask & (softfloat_flag_underflow | softfloat_flag_overflow)) ) {
        /* Denormals are fun, because they don't cause #U when masked, and the inner
           code here assume it's always masked. So, detect denormals and check if it
           was masked or not, in the latter case do bias adjust. */
        if (    (r80Result.signif & RT_BIT_64( 63 ))
             || !r80Result.signif
             || (pState->exceptionMask & softfloat_flag_underflow) ) {
            softfloat_exceptionFlags |= exceptionFlagsSaved;
            return r80Result;
        }

        /* Denormal and underflow not masked, need to adjust the exponent bias
           to match 387 behaviour. */
        Assert( expExtF80UI64( r80Result.signExp ) == 0 );
        softfloat_exceptionFlags |= softfloat_flag_underflow;
    }

    /* On Intel 10980XE the FSCALE instruction can cause really large exponents
       and the rounding changes when we exceed the bias adjust. */
    if (exp >= RTFLOAT80U_EXP_BIAS_ADJUST + RTFLOAT80U_EXP_MAX) {
        Assert( softfloat_exceptionFlags & softfloat_flag_overflow );
        softfloat_exceptionFlags |= softfloat_flag_inexact | softfloat_flag_c1;
        r80Result = s_aExtF80Infinity[sign].f;
    } else if (exp <= -RTFLOAT80U_EXP_BIAS_ADJUST) {
        Assert( softfloat_exceptionFlags & softfloat_flag_underflow );
        softfloat_exceptionFlags &= ~softfloat_flag_c1;
        softfloat_exceptionFlags |= softfloat_flag_inexact;
        r80Result = s_aExtF80Zero[sign].f;
    } else {
        /* Redo the conversion with the bias applied.  */
        softfloat_exceptionFlags &= softfloat_flag_underflow | softfloat_flag_overflow;
        if ( softfloat_exceptionFlags & softfloat_flag_underflow ) {
            exp += RTFLOAT80U_EXP_BIAS_ADJUST;
            Assert( exp > 0 );
        } else {
            exp -= RTFLOAT80U_EXP_BIAS_ADJUST;
            Assert( exp < RTFLOAT80U_EXP_MAX );
        }
        r80Result = softfloat_roundPackToExtF80Inner( sign, exp, sig, sigExtra, roundingPrecision, pState );
    }
    softfloat_exceptionFlags |= exceptionFlagsSaved;
    return r80Result;
}

