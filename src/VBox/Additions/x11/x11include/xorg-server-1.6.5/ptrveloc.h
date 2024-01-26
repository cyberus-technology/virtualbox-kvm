/*
 *
 * Copyright © 2006-2008 Simon Thum             simon dot thum at gmx dot de
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef POINTERVELOCITY_H
#define POINTERVELOCITY_H

#include <input.h> /* DeviceIntPtr */

/* maximum number of filters to approximate velocity.
 * ABI-breaker!
 */
#define MAX_VELOCITY_FILTERS 8

/* constants for acceleration profiles;
 * see  */

#define AccelProfileClassic  0
#define AccelProfileDeviceSpecific 1
#define AccelProfilePolynomial 2
#define AccelProfileSmoothLinear 3
#define AccelProfileSimple 4
#define AccelProfilePower 5
#define AccelProfileLinear 6
#define AccelProfileReserved 7

/* fwd */
struct _DeviceVelocityRec;

/**
 * profile
 * returns actual acceleration depending on velocity, acceleration control,...
 */
typedef float (*PointerAccelerationProfileFunc)
              (struct _DeviceVelocityRec* /*pVel*/,
               float /*velocity*/, float /*threshold*/, float /*acc*/);

/**
 * a filter stage contains the data for adaptive IIR filtering.
 * To improve results, one may run several parallel filters
 * which have different decays. Since more integration means more
 * delay, a given filter only does good matches in a specific phase of
 * a stroke.
 *
 * Basically, the coupling feature makes one filter fairly enough,
 * so that is the default.
 */
typedef struct _FilterStage {
    float*  fading_lut;     /* lookup for adaptive IIR filter */
    int     fading_lut_size; /* size of lookup table */
    float   rdecay;     /* reciprocal weighting halflife in ms */
    float   current;
} FilterStage, *FilterStagePtr;

/**
 * Contains all data needed to implement mouse ballistics
 */
typedef struct _DeviceVelocityRec {
    FilterStage filters[MAX_VELOCITY_FILTERS];
    float   velocity;       /* velocity as guessed by algorithm */
    float   last_velocity;  /* previous velocity estimate */
    int     lrm_time;       /* time the last motion event was processed  */
    int     last_dx, last_dy; /* last motion delta */
    int     last_diff;      /* last time-difference */
    Bool    last_reset;     /* whether a nv-reset occurred just before */
    float   corr_mul;       /* config: multiply this into velocity */
    float   const_acceleration;  /* config: (recipr.) const deceleration */
    float   min_acceleration;    /* config: minimum acceleration */
    short   reset_time;     /* config: reset non-visible state after # ms */
    short   use_softening;  /* config: use softening of mouse values */
    float   coupling;       /* config: max. divergence before coupling */
    Bool    average_accel;  /* config: average acceleration over velocity */
    PointerAccelerationProfileFunc Profile;
    PointerAccelerationProfileFunc deviceSpecificProfile;
    void*   profile_private;/* extended data, see  SetAccelerationProfile() */
    struct {   /* to be able to query this information */
        int     profile_number;
        int     filter_usecount[MAX_VELOCITY_FILTERS +1];
    } statistics;
} DeviceVelocityRec, *DeviceVelocityPtr;


extern void
InitVelocityData(DeviceVelocityPtr s);

extern void
InitFilterChain(DeviceVelocityPtr s, float rdecay, float degression,
                int lutsize, int stages);

extern int
SetAccelerationProfile(DeviceVelocityPtr s, int profile_num);

extern DeviceVelocityPtr
GetDevicePredictableAccelData(DeviceIntPtr pDev);

extern void
SetDeviceSpecificAccelerationProfile(DeviceVelocityPtr s,
                                     PointerAccelerationProfileFunc profile);

extern void
AccelerationDefaultCleanup(DeviceIntPtr pDev);

extern void
acceleratePointerPredictable(DeviceIntPtr pDev, int first_valuator,
                             int num_valuators, int *valuators, int evtime);

extern void
acceleratePointerLightweight(DeviceIntPtr pDev, int first_valuator,
                         int num_valuators, int *valuators, int ignore);

#endif  /* POINTERVELOCITY_H */
