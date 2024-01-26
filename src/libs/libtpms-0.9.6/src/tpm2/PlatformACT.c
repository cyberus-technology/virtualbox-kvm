/********************************************************************************/
/*										*/
/*		 Platform Authenticated Countdown Timer		  		*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: PlatformACT.c $		*/
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
/*  (c) Copyright IBM Corp. and others, 2019 - 2020				*/
/*										*/
/********************************************************************************/
/* C.16	PlatformACT.c */
/* C.16.1.	Includes */
#include "Platform.h"
#include "PlatformACT_fp.h"
/* C.16.2.	Functions */
/* C.16.2.1.	ActSignal() */
/* Function called when there is an ACT event to signal or unsignal */
#ifndef __ACT_DISABLED	// libtpms added
static void
ActSignal(
	  P_ACT_DATA          actData,
	  int                 on
	  )
{
    if(actData == NULL)
	return;
    // If this is to turn a signal on, don't do anything if it is already on. If this
    // is to turn the signal off, do it anyway because this might be for
    // initialization.
    if(on && (actData->signaled == TRUE))
	return;
    actData->signaled = (uint8_t)on;
    
    // If there is an action, then replace the "Do something" with the correct action.
    // It should test 'on' to see if it is turning the signal on or off.
    switch(actData->number)
	{
#if RH_ACT_0
	  case 0: // Do something
	    return;
#endif
#if RH_ACT_1
	  case 1: // Do something
	    return;
#endif
#if RH_ACT_2
	  case 2: // Do something
	    return;
#endif
#if RH_ACT_3
	  case 3: // Do something
	    return;
#endif
#if RH_ACT_4
	  case 4: // Do something
	    return;
#endif
#if RH_ACT_5
	  case 5: // Do something
	    return;
#endif
#if RH_ACT_6
	  case 6: // Do something
	    return;
#endif
#if RH_ACT_7
	  case 7: // Do something
	    return;
#endif
#if RH_ACT_8
	  case 8: // Do something
	    return;
#endif
#if RH_ACT_9
	  case 9: // Do something
	    return;
#endif
#if RH_ACT_A
	  case 0xA: // Do something
	    return;
#endif
#if RH_ACT_B
	  case 0xB:
	    // Do something
	    return;
#endif
#if RH_ACT_C
	  case 0xC: // Do something
	    return;
#endif
#if RH_ACT_D
	  case 0xD: // Do something
	    return;
#endif
#if RH_ACT_E
	  case 0xE: // Do something
	    return;
#endif
#if RH_ACT_F
	  case 0xF: // Do something
	    return;
#endif
	  default:
	    return;
	}
}
#endif		// libtpms added
/* C.16.2.2.	ActGetDataPointer() */
static P_ACT_DATA
ActGetDataPointer(
		  uint32_t            act
		  )
{
    
#define RETURN_ACT_POINTER(N)  if(0x##N == act) return &ACT_##N;
    
    FOR_EACH_ACT(RETURN_ACT_POINTER)
	
	return (P_ACT_DATA)NULL;
}
/* C.16.2.3.	_plat__ACT_GetImplemented() */
/* This function tests to see if an ACT is implemented. It is a belt and suspenders function because
   the TPM should not be calling to manipulate an ACT that is not implemented. However, this
   could help the simulator code which doesn't necessarily know if an ACT is implemented or not. */
LIB_EXPORT int
_plat__ACT_GetImplemented(
			  uint32_t            act
			  )
{
    return (ActGetDataPointer(act) != NULL);
}
/* C.16.2.4.	_plat__ACT_GetRemaining() */
/* This function returns the remaining time. If an update is pending, newValue is
   returned. Otherwise, the current counter value is returned. Note that since the timers keep
   running, the returned value can get stale immediately. The actual count value will be no greater
   than the returned value. */
LIB_EXPORT uint32_t
_plat__ACT_GetRemaining(
			uint32_t            act             //IN: the ACT selector
			)
{
    P_ACT_DATA              actData = ActGetDataPointer(act);
    uint32_t                remain;
    //
    if(actData == NULL)
	return 0;
    remain = actData->remaining;
    if(actData->pending)
	remain = actData->newValue;
    return remain;
}
/* C.16.2.5.	_plat__ACT_GetSignaled() */
LIB_EXPORT int
_plat__ACT_GetSignaled(
		       uint32_t            act         //IN: number of ACT to check
		       )
{
    P_ACT_DATA              actData = ActGetDataPointer(act);
    //
    if(actData == NULL)
	return 0;
    return (int)actData->signaled;
}
/* C.16.2.6.	_plat__ACT_SetSignaled() */
#ifndef __ACT_DISABLED	// libtpms added
LIB_EXPORT void
_plat__ACT_SetSignaled(
		       uint32_t            act,
		       int                 on
		       )
{
    ActSignal(ActGetDataPointer(act), on);
}
/* C.16.2.7.	_plat__ACT_GetPending() */
LIB_EXPORT int
_plat__ACT_GetPending(
		      uint32_t            act         //IN: number of ACT to check
		      )
{
    P_ACT_DATA              actData = ActGetDataPointer(act);
    //
    if(actData == NULL)
	return 0;
    return (int)actData->pending;
}
/* C.16.2.8.	_plat__ACT_UpdateCounter() */
/* This function is used to write the newValue for the counter. If an update is pending, then no
   update occurs and the function returns FALSE. If setSignaled is TRUE, then the ACT signaled state
   is SET and if newValue is 0, nothing is posted. */
LIB_EXPORT int
_plat__ACT_UpdateCounter(
			 uint32_t            act,        // IN: ACT to update
			 uint32_t            newValue   // IN: the value to post
			 )
{
    P_ACT_DATA          actData = ActGetDataPointer(act);
    //
    if(actData == NULL)
	// actData doesn't exist but pretend update is pending rather than indicate
	// that a retry is necessary.
	return TRUE;
    // if an update is pending then return FALSE so that there will be a retry
    if(actData->pending != 0)
	return FALSE;
    actData->newValue = newValue;
    actData->pending = TRUE;
    
    return TRUE;
}
#endif		// libtpms added
/* C.16.2.9.	_plat__ACT_EnableTicks() */
/* This enables and disables the processing of the once-per-second ticks. This should be turned off
   (enable = FALSE) by _TPM_Init() and turned on (enable = TRUE) by TPM2_Startup() after all the
   initializations have completed. */
LIB_EXPORT void
_plat__ACT_EnableTicks(
		       int 	enable
		       )
{
    actTicksAllowed = enable;
}
/* C.16.2.10.	ActDecrement() */
/* If newValue is non-zero it is copied to remaining and then newValue is set to zero. Then
   remaining is decremented by one if it is not already zero. If the value is decremented to zero,
   then the associated event is signaled. If setting remaining causes it to be greater than 1, then
   the signal associated with the ACT is turned off. */
#ifndef __ACT_DISABLED	// libtpms added
static void
ActDecrement(
	     P_ACT_DATA            actData
	     )
{
    // Check to see if there is an update pending
    if(actData->pending)
	{
	    // If this update will cause the count to go from non-zero to zero, set
	    // the newValue to 1 so that it will timeout when decremented below.
	    if((actData->newValue == 0) && (actData->remaining != 0))
		actData->newValue = 1;
	    actData->remaining = actData->newValue;
	    
	    // Update processed
	    actData->pending = 0;
	}
    // no update so countdown if the count is non-zero but not max
    if((actData->remaining != 0) && (actData->remaining != UINT32_MAX))
	{
	    // If this countdown causes the count to go to zero, then turn the signal for
	    // the ACT on.
	    if((actData->remaining -= 1) == 0)
		ActSignal(actData, TRUE);
	}
    // If the current value of the counter is non-zero, then the signal should be
    // off.
    if(actData->signaled && (actData->remaining > 0))
	ActSignal(actData, FALSE);
}
/* C.16.2.11.	_plat__ACT_Tick() */
/* This processes the once-per-second clock tick from the hardware. This is set up for the simulator to use the control interface to send ticks to the TPM. These ticks do not have to be on a per second basis. They can be as slow or as fast as desired so that the simulation can be tested. */
LIB_EXPORT void
_plat__ACT_Tick(
		void
		)
{
    // Ticks processing is turned off at certain times just to make sure that nothing
    // strange is happening before pointers and things are
    if(actTicksAllowed)
	{
	    // Handle the update for each counter.
#define DECREMENT_COUNT(N)   ActDecrement(&ACT_##N);
	    
	    FOR_EACH_ACT(DECREMENT_COUNT)
		}
}
/* C.16.2.12.	ActZero() */
/* This function initializes a single ACT */
static void
ActZero(
	uint32_t        act,
	P_ACT_DATA      actData
	)
{
    actData->remaining = 0;
    actData->newValue = 0;
    actData->pending = 0;
    actData->number = (uint8_t)act;
    ActSignal(actData, FALSE);
}
#endif			// libtpms added
/* C.16.2.13.	_plat__ACT_Initialize() */
/* This function initializes the ACT hardware and data structures */
LIB_EXPORT int
_plat__ACT_Initialize(
		      void
		      )
{
    actTicksAllowed = 0;
#define ZERO_ACT(N)  ActZero(0x##N, &ACT_##N);
    FOR_EACH_ACT(ZERO_ACT)
	
	return TRUE;
}

