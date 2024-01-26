/** @file
 * IPRT / No-CRT - math.h.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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
 * --------------------------------------------------------------------
 *
 * This code is based on:
 *
 * from: @(#)fdlibm.h 5.1 93/09/24
 * $FreeBSD: src/lib/msun/src/math.h,v 1.61 2005/04/16 21:12:47 das Exp $
 * FreeBSD HEAD 2005-06-xx
 *
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#ifndef IPRT_INCLUDED_nocrt_math_h
#define IPRT_INCLUDED_nocrt_math_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
/*#include <machine/_limits.h>*/

/* from sys/cdefs.h */
#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define __GNUC_PREREQ__(ma, mi) \
    (__GNUC__ > (ma) || __GNUC__ == (ma) && __GNUC_MINOR__ >= (mi))
#else
#define __GNUC_PREREQ__(ma, mi) 0
#endif
#undef  __pure2 /* darwin: avoid conflict with system headers when doing syntax checking of the headers */
#define __pure2


/*
 * ANSI/POSIX
 */
extern const union __infinity_un {
    RTFLOAT64U      __uu;
    double          __ud;
} RT_NOCRT(__infinity);

extern const union __nanf_un {
    RTFLOAT32U      __uu;
    float           __uf;
} RT_NOCRT(__nanf);

#if __GNUC_PREREQ__(3, 3) || (defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 800)
#define __MATH_BUILTIN_CONSTANTS
#endif

#if __GNUC_PREREQ__(3, 0) && !defined(__INTEL_COMPILER)
#define __MATH_BUILTIN_RELOPS
#endif

#ifndef IPRT_NOCRT_WITHOUT_CONFLICTING_CONSTANTS

# if defined(__MATH_BUILTIN_CONSTANTS) \
 || (RT_MSC_PREREQ(RT_MSC_VER_VC140) && defined(__cplusplus)) /** @todo when was this added exactly? 2015, 2017 & 2019 has it for C++. */
#  define HUGE_VAL   __builtin_huge_val()
# else
#  define HUGE_VAL   (RT_NOCRT(__infinity).__ud)
# endif

/*
 * XOPEN/SVID
 */
# if 1/* __BSD_VISIBLE || __XSI_VISIBLE*/
# define M_E     2.7182818284590452354   /* e */
# define M_LOG2E     1.4426950408889634074   /* log 2e */
# define M_LOG10E    0.43429448190325182765  /* log 10e */
# define M_LN2       0.69314718055994530942  /* log e2 */
# define M_LN10      2.30258509299404568402  /* log e10 */
# define M_PI        3.14159265358979323846  /* pi */
# define M_PI_2      1.57079632679489661923  /* pi/2 */
# define M_PI_4      0.78539816339744830962  /* pi/4 */
# define M_1_PI      0.31830988618379067154  /* 1/pi */
# define M_2_PI      0.63661977236758134308  /* 2/pi */
# define M_2_SQRTPI  1.12837916709551257390  /* 2/sqrt(pi) */
# define M_SQRT2     1.41421356237309504880  /* sqrt(2) */
# define M_SQRT1_2   0.70710678118654752440  /* 1/sqrt(2) */

# define MAXFLOAT    ((float)3.40282346638528860e+38)
extern int RT_NOCRT(signgam);
# endif /* __BSD_VISIBLE || __XSI_VISIBLE */

# if 1/* __BSD_VISIBLE*/
#  if 0
/* Old value from 4.4BSD-Lite math.h; this is probably better. */
#   define HUGE        HUGE_VAL
#  else
#   define HUGE        MAXFLOAT
#  endif
# endif /* __BSD_VISIBLE */

#endif /* !IPRT_NOCRT_WITHOUT_MATH_CONSTANTS */

/*
 * Most of these functions depend on the rounding mode and have the side
 * effect of raising floating-point exceptions, so they are not declared
 * as __pure2.  In C99, FENV_ACCESS affects the purity of these functions.
 */
RT_C_DECLS_BEGIN
/*
 * ANSI/POSIX
 */
int RT_NOCRT(__fpclassifyd)(double) __pure2;
int RT_NOCRT(__fpclassifyf)(float) __pure2;
int RT_NOCRT(__fpclassifyl)(long double) __pure2;
int RT_NOCRT(__isfinitef)(float) __pure2;
int RT_NOCRT(__isfinite)(double) __pure2;
int RT_NOCRT(__isfinitel)(long double) __pure2;
int RT_NOCRT(__isinff)(float) __pure2;
int RT_NOCRT(__isinfl)(long double) __pure2;
int RT_NOCRT(__isnanl)(long double) __pure2;
int RT_NOCRT(__isnormalf)(float) __pure2;
int RT_NOCRT(__isnormal)(double) __pure2;
int RT_NOCRT(__isnormall)(long double) __pure2;
int RT_NOCRT(__signbit)(double) __pure2;
int RT_NOCRT(__signbitf)(float) __pure2;
int RT_NOCRT(__signbitl)(long double) __pure2;

double  RT_NOCRT(acos)(double);
double  RT_NOCRT(asin)(double);
double  RT_NOCRT(atan)(double);
double  RT_NOCRT(atan2)(double, double);
double  RT_NOCRT(cos)(double);
double  RT_NOCRT(sin)(double);
double  RT_NOCRT(tan)(double);

double  RT_NOCRT(cosh)(double);
double  RT_NOCRT(sinh)(double);
double  RT_NOCRT(tanh)(double);

double  RT_NOCRT(exp)(double);
double  RT_NOCRT(frexp)(double, int *);   /* fundamentally !__pure2 */
double  RT_NOCRT(ldexp)(double, int);
double  RT_NOCRT(log)(double);
double  RT_NOCRT(log10)(double);
double  RT_NOCRT(modf)(double, double *); /* fundamentally !__pure2 */

double  RT_NOCRT(pow)(double, double);
double  RT_NOCRT(sqrt)(double);

double  RT_NOCRT(ceil)(double);
double  RT_NOCRT(fabs)(double) __pure2;
double  RT_NOCRT(floor)(double);
double  RT_NOCRT(fmod)(double, double);

/*
 * These functions are not in C90.
 */
#if 1 /*__BSD_VISIBLE || __ISO_C_VISIBLE >= 1999 || __XSI_VISIBLE*/
double  RT_NOCRT(acosh)(double);
double  RT_NOCRT(asinh)(double);
double  RT_NOCRT(atanh)(double);
double  RT_NOCRT(cbrt)(double);
double  RT_NOCRT(erf)(double);
double  RT_NOCRT(erfc)(double);
double  RT_NOCRT(exp2)(double);
double  RT_NOCRT(expm1)(double);
double  RT_NOCRT(fma)(double, double, double);
double  RT_NOCRT(hypot)(double, double);
int     RT_NOCRT(ilogb)(double) __pure2;
int     RT_NOCRT(isinf)(double) __pure2;
int     RT_NOCRT(isnan)(double) __pure2;
double  RT_NOCRT(lgamma)(double);
long long RT_NOCRT(llrint)(double);
long long RT_NOCRT(llround)(double);
double  RT_NOCRT(log1p)(double);
double  RT_NOCRT(logb)(double);
long    RT_NOCRT(lrint)(double);
long    RT_NOCRT(lround)(double);
double  RT_NOCRT(nextafter)(double, double);
double  RT_NOCRT(remainder)(double, double);
double  RT_NOCRT(remquo)(double, double, int *);
double  RT_NOCRT(rint)(double);
#endif /* __BSD_VISIBLE || __ISO_C_VISIBLE >= 1999 || __XSI_VISIBLE */

#if 1/* __BSD_VISIBLE || __XSI_VISIBLE*/
double  RT_NOCRT(j0)(double);
double  RT_NOCRT(j1)(double);
double  RT_NOCRT(jn)(int, double);
double  RT_NOCRT(scalb)(double, double);
double  RT_NOCRT(y0)(double);
double  RT_NOCRT(y1)(double);
double  RT_NOCRT(yn)(int, double);

#if 1/* __XSI_VISIBLE <= 500 || __BSD_VISIBLE*/
double  RT_NOCRT(gamma)(double);
#endif
#endif /* __BSD_VISIBLE || __XSI_VISIBLE */

#if 1/* __BSD_VISIBLE || __ISO_C_VISIBLE >= 1999*/
double  RT_NOCRT(copysign)(double, double) __pure2;
double  RT_NOCRT(fdim)(double, double);
double  RT_NOCRT(fmax)(double, double) __pure2;
double  RT_NOCRT(fmin)(double, double) __pure2;
double  RT_NOCRT(nearbyint)(double);
double  RT_NOCRT(round)(double);
double  RT_NOCRT(scalbln)(double, long);
double  RT_NOCRT(scalbn)(double, int);
double  RT_NOCRT(tgamma)(double);
double  RT_NOCRT(trunc)(double);
#endif

/*
 * BSD math library entry points
 */
#if 1/* __BSD_VISIBLE*/
double  RT_NOCRT(drem)(double, double);
int RT_NOCRT(finite)(double) __pure2;
int RT_NOCRT(isnanf)(float) __pure2;

/*
 * Reentrant version of gamma & lgamma; passes signgam back by reference
 * as the second argument; user must allocate space for signgam.
 */
double  RT_NOCRT(gamma_r)(double, int *);
double  RT_NOCRT(lgamma_r)(double, int *);

/*
 * IEEE Test Vector
 */
double  RT_NOCRT(significand)(double);
#endif /* __BSD_VISIBLE */

/* float versions of ANSI/POSIX functions */
#if 1/* __ISO_C_VISIBLE >= 1999*/
float   RT_NOCRT(acosf)(float);
float   RT_NOCRT(asinf)(float);
float   RT_NOCRT(atanf)(float);
float   RT_NOCRT(atan2f)(float, float);
float   RT_NOCRT(cosf)(float);
float   RT_NOCRT(sinf)(float);
float   RT_NOCRT(tanf)(float);

float   RT_NOCRT(coshf)(float);
float   RT_NOCRT(sinhf)(float);
float   RT_NOCRT(tanhf)(float);

float   RT_NOCRT(exp2f)(float);
float   RT_NOCRT(expf)(float);
float   RT_NOCRT(expm1f)(float);
float   RT_NOCRT(frexpf)(float, int *);   /* fundamentally !__pure2 */
int RT_NOCRT(ilogbf)(float) __pure2;
float   RT_NOCRT(ldexpf)(float, int);
float   RT_NOCRT(log10f)(float);
float   RT_NOCRT(log1pf)(float);
float   RT_NOCRT(logf)(float);
float   RT_NOCRT(modff)(float, float *);  /* fundamentally !__pure2 */

float   RT_NOCRT(powf)(float, float);
float   RT_NOCRT(sqrtf)(float);

float   RT_NOCRT(ceilf)(float);
float   RT_NOCRT(fabsf)(float) __pure2;
float   RT_NOCRT(floorf)(float);
float   RT_NOCRT(fmodf)(float, float);
float   RT_NOCRT(roundf)(float);

float   RT_NOCRT(erff)(float);
float   RT_NOCRT(erfcf)(float);
float   RT_NOCRT(hypotf)(float, float);
float   RT_NOCRT(lgammaf)(float);

float   RT_NOCRT(acoshf)(float);
float   RT_NOCRT(asinhf)(float);
float   RT_NOCRT(atanhf)(float);
float   RT_NOCRT(cbrtf)(float);
float   RT_NOCRT(logbf)(float);
float   RT_NOCRT(copysignf)(float, float) __pure2;
long long RT_NOCRT(llrintf)(float);
long long RT_NOCRT(llroundf)(float);
long    RT_NOCRT(lrintf)(float);
long    RT_NOCRT(lroundf)(float);
float   RT_NOCRT(nearbyintf)(float);
float   RT_NOCRT(nextafterf)(float, float);
float   RT_NOCRT(remainderf)(float, float);
float   RT_NOCRT(remquof)(float, float, int *);
float   RT_NOCRT(rintf)(float);
float   RT_NOCRT(scalblnf)(float, long);
float   RT_NOCRT(scalbnf)(float, int);
float   RT_NOCRT(truncf)(float);

float   RT_NOCRT(fdimf)(float, float);
float   RT_NOCRT(fmaf)(float, float, float);
float   RT_NOCRT(fmaxf)(float, float) __pure2;
float   RT_NOCRT(fminf)(float, float) __pure2;
#endif

/*
 * float versions of BSD math library entry points
 */
#if 1/* __BSD_VISIBLE*/
float   RT_NOCRT(dremf)(float, float);
int RT_NOCRT(finitef)(float) __pure2;
float   RT_NOCRT(gammaf)(float);
float   RT_NOCRT(j0f)(float);
float   RT_NOCRT(j1f)(float);
float   RT_NOCRT(jnf)(int, float);
float   RT_NOCRT(scalbf)(float, float);
float   RT_NOCRT(y0f)(float);
float   RT_NOCRT(y1f)(float);
float   RT_NOCRT(ynf)(int, float);

/*
 * Float versions of reentrant version of gamma & lgamma; passes
 * signgam back by reference as the second argument; user must
 * allocate space for signgam.
 */
float   RT_NOCRT(gammaf_r)(float, int *);
float   RT_NOCRT(lgammaf_r)(float, int *);

/*
 * float version of IEEE Test Vector
 */
float   RT_NOCRT(significandf)(float);
#endif  /* __BSD_VISIBLE */

/*
 * long double versions of ISO/POSIX math functions
 */
#if 1/* __ISO_C_VISIBLE >= 1999*/
#if 1  /* bird: we've got these */
long double RT_NOCRT(acoshl)(long double);
long double RT_NOCRT(acosl)(long double);
long double RT_NOCRT(asinhl)(long double);
long double RT_NOCRT(asinl)(long double);
long double RT_NOCRT(atan2l)(long double, long double);
long double RT_NOCRT(atanhl)(long double);
long double RT_NOCRT(atanl)(long double);
long double RT_NOCRT(cbrtl)(long double);
#endif
long double RT_NOCRT(ceill)(long double);
long double RT_NOCRT(copysignl)(long double, long double) __pure2;
#if 1 /* bird */
long double RT_NOCRT(coshl)(long double);
long double RT_NOCRT(cosl)(long double);
long double RT_NOCRT(erfcl)(long double);
long double RT_NOCRT(erfl)(long double);
long double RT_NOCRT(exp2l)(long double);
long double RT_NOCRT(expl)(long double);
long double RT_NOCRT(expm1l)(long double);
#endif
long double RT_NOCRT(fabsl)(long double) __pure2;
long double RT_NOCRT(fdiml)(long double, long double);
long double RT_NOCRT(floorl)(long double);
long double RT_NOCRT(fmal)(long double, long double, long double);
long double RT_NOCRT(fmaxl)(long double, long double) __pure2;
long double RT_NOCRT(fminl)(long double, long double) __pure2;
#if 1 /* bird */
long double RT_NOCRT(fmodl)(long double, long double);
#endif
long double RT_NOCRT(frexpl)(long double value, int *); /* fundamentally !__pure2 */
#if 1 /* bird */
long double RT_NOCRT(hypotl)(long double, long double);
#endif
int     RT_NOCRT(ilogbl)(long double) __pure2;
long double RT_NOCRT(ldexpl)(long double, int);
#if 1 /* bird */
long double RT_NOCRT(lgammal)(long double);
long long   RT_NOCRT(llrintl)(long double);
#endif
long long   RT_NOCRT(llroundl)(long double);
#if 1 /* bird */
long double RT_NOCRT(log10l)(long double);
long double RT_NOCRT(log1pl)(long double);
long double RT_NOCRT(log2l)(long double);
long double RT_NOCRT(logbl)(long double);
long double RT_NOCRT(logl)(long double);
long        RT_NOCRT(lrintl)(long double);
#endif
long        RT_NOCRT(lroundl)(long double);
#if 1 /* bird */
long double RT_NOCRT(modfl)(long double, long double *); /* fundamentally !__pure2 */
long double RT_NOCRT(nanl)(const char *) __pure2;
long double RT_NOCRT(nearbyintl)(long double);
#endif
long double RT_NOCRT(nextafterl)(long double, long double);
double      RT_NOCRT(nexttoward)(double, long double);
float       RT_NOCRT(nexttowardf)(float, long double);
long double RT_NOCRT(nexttowardl)(long double, long double);
#if 1 /* bird */
long double RT_NOCRT(powl)(long double, long double);
long double RT_NOCRT(remainderl)(long double, long double);
long double RT_NOCRT(remquol)(long double, long double, int *);
long double RT_NOCRT(rintl)(long double);
#endif
long double RT_NOCRT(roundl)(long double);
long double RT_NOCRT(scalblnl)(long double, long);
long double RT_NOCRT(scalbnl)(long double, int);
#if 1 /* bird: we 've got most of these. */
long double RT_NOCRT(sinhl)(long double);
long double RT_NOCRT(sinl)(long double);
long double RT_NOCRT(sqrtl)(long double);
long double RT_NOCRT(tanhl)(long double);
long double RT_NOCRT(tanl)(long double);
long double RT_NOCRT(tgammal)(long double);
#endif
long double RT_NOCRT(truncl)(long double);

/* bird: these were missing, gcc apparently inlines them. */
double          RT_NOCRT(nan)(const char *);
float           RT_NOCRT(nanf)(const char *);

#endif /* __ISO_C_VISIBLE >= 1999 */

#ifndef IPRT_NOCRT_WITHOUT_CONFLICTING_CONSTANTS /*def __USE_GNU*/
/*
 * In GLIBC there are long variants of the XOPEN/SVID constant
 * block some pages ago. We need this to get the math tests going.
 */
# define M_El                2.7182818284590452353602874713526625L
# define M_LOG2El            1.4426950408889634073599246810018921L
# define M_LOG10El           0.4342944819032518276511289189166051L
# define M_LN2l              0.6931471805599453094172321214581766L
# define M_LN10l             2.3025850929940456840179914546843642L
# define M_PIl               3.1415926535897932384626433832795029L
# define M_PI_2l             1.5707963267948966192313216916397514L
# define M_PI_4l             0.7853981633974483096156608458198757L
# define M_1_PIl             0.3183098861837906715377675267450287L
# define M_2_PIl             0.6366197723675813430755350534900574L
# define M_2_SQRTPIl         1.1283791670955125738961589031215452L
# define M_SQRT2l            1.4142135623730950488016887242096981L
# define M_SQRT1_2l          0.7071067811865475244008443621048490L
#endif /* !IPRT_NOCRT_WITHOUT_MATH_CONSTANTS */

#if 1/*def __USE_GNU*/

void RT_NOCRT(sincos)(double, double *, double *);
void RT_NOCRT(sincosf)(float, float *, float *);
void RT_NOCRT(sincosl)(long double, long double *, long double *);
float RT_NOCRT(exp10f)(float);
double RT_NOCRT(exp10)(double);
long double RT_NOCRT(exp10l)(long double);
float RT_NOCRT(log2f)(float);
double RT_NOCRT(log2)(double);
long double RT_NOCRT(log2l)(long double);
float RT_NOCRT(tgammaf)(float);
long double RT_NOCRT(significandl)(long double);
long double RT_NOCRT(j0l)(long double);
long double RT_NOCRT(j1l)(long double);
long double RT_NOCRT(jnl)(int, long double);
long double RT_NOCRT(scalbl)(long double, long double);
long double RT_NOCRT(y0l)(long double);
long double RT_NOCRT(y1l)(long double);
long double RT_NOCRT(ynl)(int, long double);
long double RT_NOCRT(lgammal_r)(long double,int *);
long double RT_NOCRT(gammal)(long double);
#endif


RT_C_DECLS_END


/** @name fpclassify return values
 * @{  */
#define RT_NOCRT_FP_INFINITE    0x01
#define RT_NOCRT_FP_NAN         0x02
#define RT_NOCRT_FP_NORMAL      0x04
#define RT_NOCRT_FP_SUBNORMAL   0x08
#define RT_NOCRT_FP_ZERO        0x10
/** @} */

/* bird 2022-08-03: moved this block down so we can prototype isnan & isinf without runnning into the macro forms. */
#ifndef IPRT_NOCRT_WITHOUT_CONFLICTING_CONSTANTS /* __ISO_C_VISIBLE >= 1999*/
# define FP_ILOGB0   (-__INT_MAX)
# define FP_ILOGBNAN __INT_MAX

# ifdef __MATH_BUILTIN_CONSTANTS
#  define HUGE_VALF   __builtin_huge_valf()
#  define HUGE_VALL   __builtin_huge_vall()
#  define INFINITY    __builtin_inf()
#  define NAN         __builtin_nan("")
# elif RT_MSC_PREREQ(RT_MSC_VER_VC140) && defined(__cplusplus)
/** @todo When were these introduced exactly? 2015, 2017 & 2019 has them.
 * However, they only work in C++ even if the c1.dll includes the strings. Oh, well. */
#  define HUGE_VALF   __builtin_huge_valf()
#  define HUGE_VALL   __builtin_huge_val()
#  define INFINITY    __builtin_huge_val()
#  define NAN         __builtin_nan("0")     /* same as we use in climits */
# else
#  define HUGE_VALF   (float)HUGE_VAL
#  define HUGE_VALL   (long double)HUGE_VAL
#  define INFINITY    HUGE_VALF
#  define NAN         (__nanf.__uf)
# endif /* __MATH_BUILTIN_CONSTANTS */

# ifndef IPRT_NO_CRT
#  define MATH_ERRNO  1
# endif
# define MATH_ERREXCEPT  2
# define math_errhandling    MATH_ERREXCEPT

/* XXX We need a <machine/math.h>. */
# if defined(__ia64__) || defined(__sparc64__)
#  define FP_FAST_FMA
# endif
# ifdef __ia64__
#  define FP_FAST_FMAL
# endif
# define FP_FAST_FMAF

/* Symbolic constants to classify floating point numbers. */
# define FP_INFINITE            RT_NOCRT_FP_INFINITE
# define FP_NAN                 RT_NOCRT_FP_NAN
# define FP_NORMAL              RT_NOCRT_FP_NORMAL
# define FP_SUBNORMAL           RT_NOCRT_FP_SUBNORMAL
# define FP_ZERO                RT_NOCRT_FP_ZERO
# define fpclassify(x) \
    ((sizeof (x) == sizeof (float)) ? RT_NOCRT(__fpclassifyf)(x) \
    : (sizeof (x) == sizeof (double)) ? RT_NOCRT(__fpclassifyd)(x) \
    : RT_NOCRT(__fpclassifyl)(x))

# define isfinite(x)                 \
    ((sizeof (x) == sizeof (float)) ? RT_NOCRT(__isfinitef)(x)    \
    : (sizeof (x) == sizeof (double)) ? RT_NOCRT(__isfinite)(x)   \
    : RT_NOCRT(__isfinitel)(x))
# define isinf(x)                    \
    ((sizeof (x) == sizeof (float)) ? RT_NOCRT(__isinff)(x)   \
    : (sizeof (x) == sizeof (double)) ? RT_NOCRT(isinf)(x)    \
    : RT_NOCRT(__isinfl)(x))
# define isnan(x)                    \
    ((sizeof (x) == sizeof (float)) ? RT_NOCRT(isnanf)(x)     \
    : (sizeof (x) == sizeof (double)) ? RT_NOCRT(isnan)(x)    \
    : RT_NOCRT(__isnanl)(x))
# define isnormal(x)                 \
    ((sizeof (x) == sizeof (float)) ? RT_NOCRT(__isnormalf)(x)    \
    : (sizeof (x) == sizeof (double)) ? RT_NOCRT(__isnormal)(x)   \
    : RT_NOCRT(__isnormall)(x))

# ifdef __MATH_BUILTIN_RELOPS
#  define isgreater(x, y)     __builtin_isgreater((x), (y))
#  define isgreaterequal(x, y)    __builtin_isgreaterequal((x), (y))
#  define isless(x, y)        __builtin_isless((x), (y))
#  define islessequal(x, y)   __builtin_islessequal((x), (y))
#  define islessgreater(x, y) __builtin_islessgreater((x), (y))
#  define isunordered(x, y)   __builtin_isunordered((x), (y))
# else
#  define isgreater(x, y)     (!isunordered((x), (y)) && (x) > (y))
#  define isgreaterequal(x, y)    (!isunordered((x), (y)) && (x) >= (y))
#  define isless(x, y)        (!isunordered((x), (y)) && (x) < (y))
#  define islessequal(x, y)   (!isunordered((x), (y)) && (x) <= (y))
#  define islessgreater(x, y) (!isunordered((x), (y)) && \
                    ((x) > (y) || (y) > (x)))
#  define isunordered(x, y)   (isnan(x) || isnan(y))
# endif /* __MATH_BUILTIN_RELOPS */

# define signbit(x)                  \
    ((sizeof (x) == sizeof (float)) ? RT_NOCRT(__signbitf)(x) \
    : (sizeof (x) == sizeof (double)) ? RT_NOCRT(__signbit)(x)    \
    : RT_NOCRT(__signbitl)(x))

typedef double  double_t;
typedef float   float_t;
#endif /* !IPRT_NOCRT_WITHOUT_MATH_CONSTANTS */ /* __ISO_C_VISIBLE >= 1999 */


#if !defined(RT_WITHOUT_NOCRT_WRAPPERS) && !defined(RT_WITHOUT_NOCRT_WRAPPER_ALIASES)
/* sed -e "/#/d" -e "/RT_NOCRT/!d" -e "s/^.*RT_NOCRT(\([a-z0-9_]*\)).*$/# define \1 RT_NOCRT(\1)/" */
# define __fpclassifyf RT_NOCRT(__fpclassifyf)
# define __fpclassifyd RT_NOCRT(__fpclassifyd)
# define __fpclassifyl RT_NOCRT(__fpclassifyl)
# define __isfinitef RT_NOCRT(__isfinitef)
# define __isfinite RT_NOCRT(__isfinite)
# define __isfinitel RT_NOCRT(__isfinitel)
# define __isinff RT_NOCRT(__isinff)
# define __isinfl RT_NOCRT(__isinfl)
# define __isnanl RT_NOCRT(__isnanl)
# define __isnormalf RT_NOCRT(__isnormalf)
# define __isnormal RT_NOCRT(__isnormal)
# define __isnormall RT_NOCRT(__isnormall)
# define __signbitf RT_NOCRT(__signbitf)
# define __signbit RT_NOCRT(__signbit)
# define __signbitl RT_NOCRT(__signbitl)
# define signgam RT_NOCRT(signgam)
# define __fpclassifyd RT_NOCRT(__fpclassifyd)
# define __fpclassifyf RT_NOCRT(__fpclassifyf)
# define __fpclassifyl RT_NOCRT(__fpclassifyl)
# define __isfinitef RT_NOCRT(__isfinitef)
# define __isfinite RT_NOCRT(__isfinite)
# define __isfinitel RT_NOCRT(__isfinitel)
# define __isinff RT_NOCRT(__isinff)
# define __isinfl RT_NOCRT(__isinfl)
# define __isnanl RT_NOCRT(__isnanl)
# define __isnormalf RT_NOCRT(__isnormalf)
# define __isnormal RT_NOCRT(__isnormal)
# define __isnormall RT_NOCRT(__isnormall)
# define __signbit RT_NOCRT(__signbit)
# define __signbitf RT_NOCRT(__signbitf)
# define __signbitl RT_NOCRT(__signbitl)
# define acos RT_NOCRT(acos)
# define asin RT_NOCRT(asin)
# define atan RT_NOCRT(atan)
# define atan2 RT_NOCRT(atan2)
# define cos RT_NOCRT(cos)
# define sin RT_NOCRT(sin)
# define tan RT_NOCRT(tan)
# define cosh RT_NOCRT(cosh)
# define sinh RT_NOCRT(sinh)
# define tanh RT_NOCRT(tanh)
# define exp RT_NOCRT(exp)
# define frexp RT_NOCRT(frexp)
# define ldexp RT_NOCRT(ldexp)
# define log RT_NOCRT(log)
# define log10 RT_NOCRT(log10)
# define modf RT_NOCRT(modf)
# define pow RT_NOCRT(pow)
# define sqrt RT_NOCRT(sqrt)
# define ceil RT_NOCRT(ceil)
# define fabs RT_NOCRT(fabs)
# define floor RT_NOCRT(floor)
# define fmod RT_NOCRT(fmod)
# define acosh RT_NOCRT(acosh)
# define asinh RT_NOCRT(asinh)
# define atanh RT_NOCRT(atanh)
# define cbrt RT_NOCRT(cbrt)
# define erf RT_NOCRT(erf)
# define erfc RT_NOCRT(erfc)
# define exp2 RT_NOCRT(exp2)
# define expm1 RT_NOCRT(expm1)
# define fma RT_NOCRT(fma)
# define hypot RT_NOCRT(hypot)
# define ilogb RT_NOCRT(ilogb)
# define lgamma RT_NOCRT(lgamma)
# define llrint RT_NOCRT(llrint)
# define llround RT_NOCRT(llround)
# define log1p RT_NOCRT(log1p)
# define logb RT_NOCRT(logb)
# define lrint RT_NOCRT(lrint)
# define lround RT_NOCRT(lround)
# define nextafter RT_NOCRT(nextafter)
# define remainder RT_NOCRT(remainder)
# define remquo RT_NOCRT(remquo)
# define rint RT_NOCRT(rint)
# define j0 RT_NOCRT(j0)
# define j1 RT_NOCRT(j1)
# define jn RT_NOCRT(jn)
# define scalb RT_NOCRT(scalb)
# define y0 RT_NOCRT(y0)
# define y1 RT_NOCRT(y1)
# define yn RT_NOCRT(yn)
# define gamma RT_NOCRT(gamma)
# define copysign RT_NOCRT(copysign)
# define fdim RT_NOCRT(fdim)
# define fmax RT_NOCRT(fmax)
# define fmin RT_NOCRT(fmin)
# define nearbyint RT_NOCRT(nearbyint)
# define round RT_NOCRT(round)
# define scalbln RT_NOCRT(scalbln)
# define scalbn RT_NOCRT(scalbn)
# define tgamma RT_NOCRT(tgamma)
# define trunc RT_NOCRT(trunc)
# define drem RT_NOCRT(drem)
# define finite RT_NOCRT(finite)
/*# define isinf RT_NOCRT(isinf) - already a macro */
/*# define isnan RT_NOCRT(isnan) - already a macro */
# define isnanf RT_NOCRT(isnanf)
# define gamma_r RT_NOCRT(gamma_r)
# define lgamma_r RT_NOCRT(lgamma_r)
# define significand RT_NOCRT(significand)
# define acosf RT_NOCRT(acosf)
# define asinf RT_NOCRT(asinf)
# define atanf RT_NOCRT(atanf)
# define atan2f RT_NOCRT(atan2f)
# define cosf RT_NOCRT(cosf)
# define sinf RT_NOCRT(sinf)
# define tanf RT_NOCRT(tanf)
# define coshf RT_NOCRT(coshf)
# define sinhf RT_NOCRT(sinhf)
# define tanhf RT_NOCRT(tanhf)
# define exp2f RT_NOCRT(exp2f)
# define expf RT_NOCRT(expf)
# define expm1f RT_NOCRT(expm1f)
# define frexpf RT_NOCRT(frexpf)
# define ilogbf RT_NOCRT(ilogbf)
# define ldexpf RT_NOCRT(ldexpf)
# define log10f RT_NOCRT(log10f)
# define log1pf RT_NOCRT(log1pf)
# define logf RT_NOCRT(logf)
# define modff RT_NOCRT(modff)
# define powf RT_NOCRT(powf)
# define sqrtf RT_NOCRT(sqrtf)
# define ceilf RT_NOCRT(ceilf)
# define fabsf RT_NOCRT(fabsf)
# define floorf RT_NOCRT(floorf)
# define fmodf RT_NOCRT(fmodf)
# define roundf RT_NOCRT(roundf)
# define erff RT_NOCRT(erff)
# define erfcf RT_NOCRT(erfcf)
# define hypotf RT_NOCRT(hypotf)
# define lgammaf RT_NOCRT(lgammaf)
# define acoshf RT_NOCRT(acoshf)
# define asinhf RT_NOCRT(asinhf)
# define atanhf RT_NOCRT(atanhf)
# define cbrtf RT_NOCRT(cbrtf)
# define logbf RT_NOCRT(logbf)
# define copysignf RT_NOCRT(copysignf)
# define llrintf RT_NOCRT(llrintf)
# define llroundf RT_NOCRT(llroundf)
# define lrintf RT_NOCRT(lrintf)
# define lroundf RT_NOCRT(lroundf)
# define nearbyintf RT_NOCRT(nearbyintf)
# define nextafterf RT_NOCRT(nextafterf)
# define remainderf RT_NOCRT(remainderf)
# define remquof RT_NOCRT(remquof)
# define rintf RT_NOCRT(rintf)
# define scalblnf RT_NOCRT(scalblnf)
# define scalbnf RT_NOCRT(scalbnf)
# define truncf RT_NOCRT(truncf)
# define fdimf RT_NOCRT(fdimf)
# define fmaf RT_NOCRT(fmaf)
# define fmaxf RT_NOCRT(fmaxf)
# define fminf RT_NOCRT(fminf)
# define dremf RT_NOCRT(dremf)
# define finitef RT_NOCRT(finitef)
# define gammaf RT_NOCRT(gammaf)
# define j0f RT_NOCRT(j0f)
# define j1f RT_NOCRT(j1f)
# define jnf RT_NOCRT(jnf)
# define scalbf RT_NOCRT(scalbf)
# define y0f RT_NOCRT(y0f)
# define y1f RT_NOCRT(y1f)
# define ynf RT_NOCRT(ynf)
# define gammaf_r RT_NOCRT(gammaf_r)
# define lgammaf_r RT_NOCRT(lgammaf_r)
# define significandf RT_NOCRT(significandf)
# define acoshl RT_NOCRT(acoshl)
# define acosl RT_NOCRT(acosl)
# define asinhl RT_NOCRT(asinhl)
# define asinl RT_NOCRT(asinl)
# define atan2l RT_NOCRT(atan2l)
# define atanhl RT_NOCRT(atanhl)
# define atanl RT_NOCRT(atanl)
# define cbrtl RT_NOCRT(cbrtl)
# define ceill RT_NOCRT(ceill)
# define copysignl RT_NOCRT(copysignl)
# define coshl RT_NOCRT(coshl)
# define cosl RT_NOCRT(cosl)
# define erfcl RT_NOCRT(erfcl)
# define erfl RT_NOCRT(erfl)
# define exp2l RT_NOCRT(exp2l)
# define expl RT_NOCRT(expl)
# define expm1l RT_NOCRT(expm1l)
# define fabsl RT_NOCRT(fabsl)
# define fdiml RT_NOCRT(fdiml)
# define floorl RT_NOCRT(floorl)
# define fmal RT_NOCRT(fmal)
# define fmaxl RT_NOCRT(fmaxl)
# define fminl RT_NOCRT(fminl)
# define fmodl RT_NOCRT(fmodl)
# define frexpl RT_NOCRT(frexpl)
# define hypotl RT_NOCRT(hypotl)
# define ilogbl RT_NOCRT(ilogbl)
# define ldexpl RT_NOCRT(ldexpl)
# define lgammal RT_NOCRT(lgammal)
# define llrintl RT_NOCRT(llrintl)
# define llroundl RT_NOCRT(llroundl)
# define log10l RT_NOCRT(log10l)
# define log1pl RT_NOCRT(log1pl)
# define log2l RT_NOCRT(log2l)
# define logbl RT_NOCRT(logbl)
# define logl RT_NOCRT(logl)
# define lrintl RT_NOCRT(lrintl)
# define lroundl RT_NOCRT(lroundl)
# define modfl RT_NOCRT(modfl)
# define nanl RT_NOCRT(nanl)
# define nearbyintl RT_NOCRT(nearbyintl)
# define nextafterl RT_NOCRT(nextafterl)
# define nexttoward RT_NOCRT(nexttoward)
# define nexttowardf RT_NOCRT(nexttowardf)
# define nexttowardl RT_NOCRT(nexttowardl)
# define powl RT_NOCRT(powl)
# define remainderl RT_NOCRT(remainderl)
# define remquol RT_NOCRT(remquol)
# define rintl RT_NOCRT(rintl)
# define roundl RT_NOCRT(roundl)
# define scalblnl RT_NOCRT(scalblnl)
# define scalbnl RT_NOCRT(scalbnl)
# define sinhl RT_NOCRT(sinhl)
# define sinl RT_NOCRT(sinl)
# define sqrtl RT_NOCRT(sqrtl)
# define tanhl RT_NOCRT(tanhl)
# define tanl RT_NOCRT(tanl)
# define tgammal RT_NOCRT(tgammal)
# define truncl RT_NOCRT(truncl)
# define nan RT_NOCRT(nan)
# define nanf RT_NOCRT(nanf)
# define sincos RT_NOCRT(sincos)
# define sincosf RT_NOCRT(sincosf)
# define sincosl RT_NOCRT(sincosl)
# define exp10f RT_NOCRT(exp10f)
# define exp10 RT_NOCRT(exp10)
# define exp10l RT_NOCRT(exp10l)
# define log2f RT_NOCRT(log2f)
# define log2 RT_NOCRT(log2)
# define log2l RT_NOCRT(log2l)
# define tgammaf RT_NOCRT(tgammaf)
# define significandl RT_NOCRT(significandl)
# define j0l RT_NOCRT(j0l)
# define j1l RT_NOCRT(j1l)
# define jnl RT_NOCRT(jnl)
# define scalbl RT_NOCRT(scalbl)
# define y0l RT_NOCRT(y0l)
# define y1l RT_NOCRT(y1l)
# define ynl RT_NOCRT(ynl)
# define lgammal_r RT_NOCRT(lgammal_r)
# define gammal RT_NOCRT(gammal)
#endif

/*
 * Include inlined implementations.
 */
#ifdef RT_ARCH_AMD64
# include <iprt/nocrt/amd64/math.h>
#elif defined(RT_ARCH_X86)
# include <iprt/nocrt/x86/math.h>
#endif

#endif /* !IPRT_INCLUDED_nocrt_math_h */

