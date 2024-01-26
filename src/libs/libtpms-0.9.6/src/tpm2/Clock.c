/********************************************************************************/
/*										*/
/*		 Used by the simulator to mimic a hardware clock  		*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Clock.c $			*/
/*										*/
/*  Licenses and Notices							*/
/*										*/
/*  1. Copyright Licenses:							*/
/*										*/
/*  - Trusted Computing Group (TCG) grants to the user of the source code in	*/
/*    this specification (the "Source Code") a worldwide, irrevocable, 		*/
/*    nonexclusive, royalty free, copyright license to reproduce, create 	*/
/*    derivative works, distribute, display and perform the Source Code and	*/
/*    derivative works thereof, and to grant others the rights granted herein.	*/
/*										*/
/*  - The TCG grants to the user of the other parts of the specification 	*/
/*    (other than the Source Code) the rights to reproduce, distribute, 	*/
/*    display, and perform the specification solely for the purpose of 		*/
/*    developing products based on such documents.				*/
/*										*/
/*  2. Source Code Distribution Conditions:					*/
/*										*/
/*  - Redistributions of Source Code must retain the above copyright licenses, 	*/
/*    this list of conditions and the following disclaimers.			*/
/*										*/
/*  - Redistributions in binary form must reproduce the above copyright 	*/
/*    licenses, this list of conditions	and the following disclaimers in the 	*/
/*    documentation and/or other materials provided with the distribution.	*/
/*										*/
/*  3. Disclaimers:								*/
/*										*/
/*  - THE COPYRIGHT LICENSES SET FORTH ABOVE DO NOT REPRESENT ANY FORM OF	*/
/*  LICENSE OR WAIVER, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, WITH	*/
/*  RESPECT TO PATENT RIGHTS HELD BY TCG MEMBERS (OR OTHER THIRD PARTIES)	*/
/*  THAT MAY BE NECESSARY TO IMPLEMENT THIS SPECIFICATION OR OTHERWISE.		*/
/*  Contact TCG Administration (admin@trustedcomputinggroup.org) for 		*/
/*  information on specification licensing rights available through TCG 	*/
/*  membership agreements.							*/
/*										*/
/*  - THIS SPECIFICATION IS PROVIDED "AS IS" WITH NO EXPRESS OR IMPLIED 	*/
/*    WARRANTIES WHATSOEVER, INCLUDING ANY WARRANTY OF MERCHANTABILITY OR 	*/
/*    FITNESS FOR A PARTICULAR PURPOSE, ACCURACY, COMPLETENESS, OR 		*/
/*    NONINFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS, OR ANY WARRANTY 		*/
/*    OTHERWISE ARISING OUT OF ANY PROPOSAL, SPECIFICATION OR SAMPLE.		*/
/*										*/
/*  - Without limitation, TCG and its members and licensors disclaim all 	*/
/*    liability, including liability for infringement of any proprietary 	*/
/*    rights, relating to use of information in this specification and to the	*/
/*    implementation of this specification, and TCG disclaims all liability for	*/
/*    cost of procurement of substitute goods or services, lost profits, loss 	*/
/*    of use, loss of data or any incidental, consequential, direct, indirect, 	*/
/*    or special damages, whether under contract, tort, warranty or otherwise, 	*/
/*    arising in any way out of use or reliance upon this specification or any 	*/
/*    information herein.							*/
/*										*/
/*  (c) Copyright IBM Corp. and others, 2016 - 2019				*/
/*										*/
/********************************************************************************/

/* added for portability because Linux clock is 32 bits */

#include <stdint.h>
#include <stdio.h>
#ifndef VBOX
#include <time.h>
#else
# include <iprt/time.h>
#endif

/* C.3 Clock.c */
/* C.3.1. Description */
/* This file contains the routines that are used by the simulator to mimic a hardware clock on a
   TPM. In this implementation, all the time values are measured in millisecond. However, the
   precision of the clock functions may be implementation dependent. */
/* C.3.2. Includes and Data Definitions */
#include <assert.h>
#include "Platform.h"
#include "TpmFail_fp.h"

/* libtpms added begin */
/* ClockGetTime -- get time given a specified clock type */
uint64_t
ClockGetTime(
#ifndef VBOX
               clockid_t clk_id
#else
               TPM_CLOCK_ID clk_id
#endif
               )
{
#ifndef VBOX
    uint64_t           time;
#ifdef TPM_WINDOWS
#error Not supported for TPM_WINDOWS
#else
    struct timespec     systime;

    clock_gettime(clk_id, &systime);
    time = (uint64_t)systime.tv_sec * 1000 + (systime.tv_nsec / 1000000);
#endif

    return time;
#else
    if (clk_id == CLOCK_REALTIME)
    {
        RTTIMESPEC Timespec;

        RTTimeNow(&Timespec);
        return RTTimeSpecGetMilli(&Timespec);
    }
    else if (clk_id == CLOCK_MONOTONIC)
        return RTTimeMilliTS();

    return 0;
#endif
}

/* ClockAdjustPostResume -- adjust time parameters post resume */
#include "Tpm.h"
void
ClockAdjustPostResume(UINT64 backthen, BOOL timesAreRealtime)
{
    UINT64 now = ClockGetTime(CLOCK_REALTIME);
    INT64 timediff = now - backthen;

    if (timesAreRealtime) {
        /* g_time, s_realTimePrevious, s_tpmTime are all in real time */
        s_suspendedElapsedTime = now;
#ifndef VBOX
        s_hostMonotonicAdjustTime = -ClockGetTime(CLOCK_MONOTONIC);
#else
        s_hostMonotonicAdjustTime = -(int64_t)ClockGetTime(CLOCK_MONOTONIC);
#endif

        /* s_lastSystemTime & s_lastReportTime need to be set as well */
        s_lastSystemTime = now;
        s_lastReportedTime = now;
    } else if (timediff >= 0) {
        s_suspendedElapsedTime += timediff;
    }
}
/* libtpms added end */

/* C.3.3. Simulator Functions */
/* C.3.3.1. Introduction */
/* This set of functions is intended to be called by the simulator environment in order to simulate
   hardware events. */
/* C.3.3.2. _plat__TimerReset() */
/* This function sets current system clock time as t0 for counting TPM time. This function is called
   at a power on event to reset the clock.  When the clock is reset, the indication that the clock
   was stopped is also set. */
LIB_EXPORT void
_plat__TimerReset(
		  void
		  )
{
    s_lastSystemTime = 0;
    s_tpmTime = 0;
    s_adjustRate = CLOCK_NOMINAL;
    s_timerReset = TRUE;
    s_timerStopped = TRUE;
    s_hostMonotonicAdjustTime = 0; /* libtpms added */
    s_suspendedElapsedTime = 0; /* libtpms added */
    return;
}
/* C.3.3.3. _plat__TimerRestart() */
/* This function should be called in order to simulate the restart of the timer should it be stopped
   while power is still applied. */
LIB_EXPORT void
_plat__TimerRestart(
		    void
		    )
{
    s_timerStopped = TRUE;
    return;
}

/* C.3.4. Functions Used by TPM */
/* C.3.4.1. Introduction */
/* These functions are called by the TPM code. They should be replaced by appropriated hardware
   functions. */

clock_t     debugTime;
/* C.3.4.2.	_plat__Time() */
/* This is another, probably futile, attempt to define a portable function that will return a 64-bit
   clock value that has mSec resolution. */
LIB_EXPORT uint64_t
_plat__RealTime(
		void
		)
{
    clock64_t           time;
    //#ifdef _MSC_VER	kgold
#ifndef VBOX
#ifdef TPM_WINDOWS
    #include <sys/timeb.h>
    struct _timeb       sysTime;
    //
    _ftime(&sysTime);	/* kgold, mingw doesn't have _ftime_s */
    time = (clock64_t)(sysTime.time) * 1000 + sysTime.millitm;
    // set the time back by one hour if daylight savings
    if(sysTime.dstflag)
	time -= 1000 * 60 * 60;  // mSec/sec * sec/min * min/hour = ms/hour
#else
    // hopefully, this will work with most UNIX systems
    struct timespec     systime;
    //
    clock_gettime(CLOCK_MONOTONIC, &systime);
    time = (clock64_t)systime.tv_sec * 1000 + (systime.tv_nsec / 1000000);
#endif
#else
    time = (clock64_t)RTTimeMilliTS();
#endif
    /* libtpms added begin */
    /* We have to make sure that this function returns monotonically increasing time
       also when a vTPM has been suspended and the host has been rebooted.
       Example:
         - The vTPM is suspended at systime '5'
         - The vTPM is resumed   at systime '1' after a host reboot
         -> we now need to add '4' to the time
         Besides this we want to account for the time a vTPM was suspended.
         If it was suspended for 10 time units, we need to add '10' here.
     */
    time += s_hostMonotonicAdjustTime + s_suspendedElapsedTime;
    /* libtpms added end */
    return time;
}



/* C.3.4.3. _plat__TimerRead() */
/* This function provides access to the tick timer of the platform. The TPM code uses this value to
   drive the TPM Clock. */
/* The tick timer is supposed to run when power is applied to the device. This timer should not be
   reset by time events including _TPM_Init(). It should only be reset when TPM power is
   re-applied. */
/* If the TPM is run in a protected environment, that environment may provide the tick time to the
   TPM as long as the time provided by the environment is not allowed to go backwards. If the time
   provided by the system can go backwards during a power discontinuity, then the
   _plat__Signal_PowerOn() should call _plat__TimerReset(). */
LIB_EXPORT uint64_t
_plat__TimerRead(
		 void
		 )
{
#ifdef HARDWARE_CLOCK
#error      "need a definition for reading the hardware clock"
    return HARDWARE_CLOCK
#else
    clock64_t         timeDiff;
    clock64_t         adjustedTimeDiff;
    clock64_t         timeNow;
    clock64_t         readjustedTimeDiff;
    // This produces a timeNow that is basically locked to the system clock.
    timeNow = _plat__RealTime();
    // if this hasn't been initialized, initialize it
    if(s_lastSystemTime == 0)
	{
	    s_lastSystemTime = timeNow;
	    debugTime = clock();
	    s_lastReportedTime = 0;
	    s_realTimePrevious = 0;
	}
    // The system time can bounce around and that's OK as long as we don't allow
    // time to go backwards. When the time does appear to go backwards, set
    // lastSystemTime to be the new value and then update the reported time.
    if(timeNow < s_lastReportedTime)
	s_lastSystemTime = timeNow;
    s_lastReportedTime = s_lastReportedTime + timeNow - s_lastSystemTime;
    s_lastSystemTime = timeNow;
    timeNow = s_lastReportedTime;
    // The code above produces a timeNow that is similar to the value returned
    // by Clock(). The difference is that timeNow does not max out, and it is
    // at a ms. rate rather than at a CLOCKS_PER_SEC rate. The code below
    // uses that value and does the rate adjustment on the time value.
    // If there is no difference in time, then skip all the computations
    if(s_realTimePrevious >= timeNow)
	return s_tpmTime;
    // Compute the amount of time since the last update of the system clock
    timeDiff = timeNow - s_realTimePrevious;
    // Do the time rate adjustment and conversion from CLOCKS_PER_SEC to mSec
    adjustedTimeDiff = (timeDiff * CLOCK_NOMINAL) / ((uint64_t)s_adjustRate);
    // update the TPM time with the adjusted timeDiff
    s_tpmTime += (clock64_t)adjustedTimeDiff;
    // Might have some rounding error that would loose CLOCKS. See what is not
    // being used. As mentioned above, this could result in putting back more than
    // is taken out. Here, we are trying to recreate timeDiff.
    readjustedTimeDiff = (adjustedTimeDiff * (uint64_t)s_adjustRate )
			 / CLOCK_NOMINAL;
    // adjusted is now converted back to being the amount we should advance the
    // previous sampled time. It should always be less than or equal to timeDiff.
    // That is, we could not have use more time than we started with.
    s_realTimePrevious = s_realTimePrevious + readjustedTimeDiff;
#ifdef  DEBUGGING_TIME
    // Put this in so that TPM time will pass much faster than real time when
    // doing debug.
    // A value of 1000 for DEBUG_TIME_MULTIPLER will make each ms into a second
    // A good value might be 100
    return (s_tpmTime * DEBUG_TIME_MULTIPLIER);
#endif
    return s_tpmTime;
#endif
}


/* C.3.4.3. _plat__TimerWasReset() */
/* This function is used to interrogate the flag indicating if the tick timer has been reset. */
/* If the resetFlag parameter is SET, then the flag will be CLEAR before the function returns. */
LIB_EXPORT int
_plat__TimerWasReset(
		     void
		     )
{
    int retVal = s_timerReset;
    s_timerReset = FALSE;
    return retVal;
}
/* C.3.4.4. _plat__TimerWasStopped() */
/* This function is used to interrogate the flag indicating if the tick timer has been stopped. If
   so, this is typically a reason to roll the nonce. */
/* This function will CLEAR the s_timerStopped flag before returning. This provides functionality
   that is similar to status register that is cleared when read. This is the model used here because
   it is the one that has the most impact on the TPM code as the flag can only be accessed by one
   entity in the TPM. Any other implementation of the hardware can be made to look like a read-once
   register. */
LIB_EXPORT int
_plat__TimerWasStopped(
		       void
		       )
{
    BOOL         retVal = s_timerStopped;
    s_timerStopped = FALSE;
    return retVal;
}
/* C.3.4.5. _plat__ClockAdjustRate() */
/* Adjust the clock rate */
LIB_EXPORT void
_plat__ClockAdjustRate(
		       int	adjust         // IN: the adjust number.  It could be positive
		       //     or negative
		       )
{
    // We expect the caller should only use a fixed set of constant values to
    // adjust the rate
    switch(adjust)
	{
	  case CLOCK_ADJUST_COARSE:
	    s_adjustRate += CLOCK_ADJUST_COARSE;
	    break;
	  case -CLOCK_ADJUST_COARSE:
	    s_adjustRate -= CLOCK_ADJUST_COARSE;
	    break;
	  case CLOCK_ADJUST_MEDIUM:
	    s_adjustRate += CLOCK_ADJUST_MEDIUM;
	    break;
	  case -CLOCK_ADJUST_MEDIUM:
	    s_adjustRate -= CLOCK_ADJUST_MEDIUM;
	    break;
	  case CLOCK_ADJUST_FINE:
	    s_adjustRate += CLOCK_ADJUST_FINE;
	    break;
	  case -CLOCK_ADJUST_FINE:
	    s_adjustRate -= CLOCK_ADJUST_FINE;
	    break;
	  default:
	    // ignore any other values;
	    break;
	}
    if(s_adjustRate > (CLOCK_NOMINAL + CLOCK_ADJUST_LIMIT))
	s_adjustRate = CLOCK_NOMINAL + CLOCK_ADJUST_LIMIT;
    if(s_adjustRate < (CLOCK_NOMINAL - CLOCK_ADJUST_LIMIT))
	s_adjustRate = CLOCK_NOMINAL - CLOCK_ADJUST_LIMIT;
    return;
}
