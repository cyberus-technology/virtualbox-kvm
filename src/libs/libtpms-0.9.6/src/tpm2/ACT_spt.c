/********************************************************************************/
/*										*/
/*			    ACT Command Support 				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Object_spt.c $		*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2020				*/
/*										*/
/********************************************************************************/

/* 7.8	ACT Support (ACT_spt.c) */
/* 7.8.1	Introduction */
/* This code implements the ACT update code. It does not use a mutex. This code uses a platform
   service (_plat__ACT_UpdateCounter()) that returns false if the update is not accepted. If this
   occurs, then TPM_RC_RETRY should be sent to the caller so that they can retry the operation
   later. The implementation of this is platform dependent but the reference uses a simple flag to
   indicate that an update is pending and the only process that can clear that flag is the process
   that does the actual update. */

/* 7.8.2	Includes */

#include "Tpm.h"
#include "ACT_spt_fp.h"
#include "Platform_fp.h"
#include "PlatformACT_fp.h"		/* added kgold */

/* 7.8.3	Functions */
/* 7.8.3.1	_ActResume() */
/* This function does the resume processing for an ACT. It updates the saved count and turns
   signaling back on if necessary. */
#ifndef __ACT_DISABLED	// libtpms added
static void
_ActResume(
	   UINT32              act,            //IN: the act number
	   ACT_STATE          *actData         //IN: pointer to the saved ACT data
	   )
{
    // If the act was non-zero, then restore the counter value.
    if(actData->remaining > 0)
	_plat__ACT_UpdateCounter(act, actData->remaining);
    // if the counter was zero and the ACT signaling, enable the signaling.
    else if(go.signaledACT & (1 << act))
	_plat__ACT_SetSignaled(act, TRUE);
}
#endif			// libtpms added
/* 7.8.3.2	ActStartup() */
/* This function is called by TPM2_Startup() to initialize the ACT counter values. */
BOOL
ActStartup(
	   STARTUP_TYPE        type
	   )
{
    // Reset all the ACT hardware
    _plat__ACT_Initialize();

    // If this not a cold start, copy all the current 'signaled' settings to
    // 'preservedSignaled'.
#ifndef __ACT_DISABLED	// libtpms added
    if (g_powerWasLost)
	go.preservedSignaled = 0;
    else
	go.preservedSignaled |= go.signaledACT;
#endif			// libtpms added
    
    // For TPM_RESET or TPM_RESTART, the ACTs will all be disabled and the output
    // de-asserted.
    if(type != SU_RESUME)
	{
#ifndef __ACT_DISABLED	// libtpms added
	    go.signaledACT = 0;
#endif			// libtpms added
#define CLEAR_ACT_POLICY(N)						\
	    go.ACT_##N.hashAlg = TPM_ALG_NULL;				\
	    go.ACT_##N.authPolicy.b.size = 0;
	    
	    FOR_EACH_ACT(CLEAR_ACT_POLICY)
		
		}
    else
	{
	    // Resume each of the implemented ACT
#define RESUME_ACT(N)   _ActResume(0x##N, &go.ACT_##N);
	    
	    FOR_EACH_ACT(RESUME_ACT)
		}
    // set no ACT updated since last startup. This is to enable the halving of the
    // timeout value
    s_ActUpdated = 0;
    _plat__ACT_EnableTicks(TRUE);
    return TRUE;
}
/* 7.8.3.3	_ActSaveState() */
/* Get the counter state and the signaled state for an ACT. If the ACT has not been updated since
   the last time it was saved, then divide the count by 2. */
#ifndef __ACT_DISABLED	// libtpms added
static void
_ActSaveState(
	      UINT32              act,
	      P_ACT_STATE         actData
	      )
{
    actData->remaining = _plat__ACT_GetRemaining(act);
    // If the ACT hasn't been updated since the last startup, then it should be
    // be halved.
    if((s_ActUpdated & (1 << act)) == 0)
	{
	    // Don't halve if the count is set to max or if halving would make it zero
	    if((actData->remaining != UINT32_MAX) && (actData->remaining > 1))
		actData->remaining /= 2;
	}
    if(_plat__ACT_GetSignaled(act))
	go.signaledACT |= (1 << act);
}
/* 7.8.3.4	ActGetSignaled() */
/* This function returns the state of the signaled flag associated with an ACT. */
BOOL
ActGetSignaled(
	       TPM_RH              actHandle
	       )
{
    UINT32              act = actHandle - TPM_RH_ACT_0;
    //
    return _plat__ACT_GetSignaled(act);
}
#endif			// libtpms added
/* 7.8.3.5	ActShutdown() */
/* This function saves the current state of the counters */
BOOL
ActShutdown(
	    TPM_SU              state       //IN: the type of the shutdown.
	    )
{
    // if this is not shutdown state, then the only type of startup is TPM_RESTART
    // so the timer values will be cleared. If this is shutdown state, get the current
    // countdown and signaled values. Plus, if the counter has not been updated
    // since the last restart, divide the time by 2 so that there is no attack on the
    // countdown by saving the countdown state early and then not using the TPM.
    if(state == TPM_SU_STATE)
	{
	    // This will be populated as each of the ACT is queried
#ifndef __ACT_DISABLED		// libtpms added
	    go.signaledACT = 0;
#endif				// libtpms added
	    // Get the current count and the signaled state
#define SAVE_ACT_STATE(N) _ActSaveState(0x##N, &go.ACT_##N);
	    
	    FOR_EACH_ACT(SAVE_ACT_STATE);
	}
    return TRUE;
}
/* 7.8.3.6	ActIsImplemented() */
/* This function determines if an ACT is implemented in both the TPM and the platform code. */
BOOL
ActIsImplemented(
		 UINT32          act
		 )
{
#define CASE_ACT_
    // This switch accounts for the TPM implemented values.
    switch(act)
	{
#ifndef __ACT_DISABLED	// libtpms added
	    FOR_EACH_ACT(CASE_ACT_NUMBER)
		// This ensures that the platorm implemented the values implemented by
		// the TPM
		return _plat__ACT_GetImplemented(act);
#endif			// libtpms added
	  default:
	    break;
	}
    return FALSE;
}
/* 7.8.3.7	ActCounterUpdate() */
/* This function updates the ACT counter. If the counter already has a pending update, it returns
   TPM_RC_RETRY so that the update can be tried again later. */
#if CC_ACT_SetTimeout	// libtpms added
TPM_RC
ActCounterUpdate(
		 TPM_RH          handle,         //IN: the handle of the act
		 UINT32          newValue        //IN: the value to set in the ACT
		 )
{
    UINT32          act;
    TPM_RC          result;
    //
    act = handle - TPM_RH_ACT_0;
    // This should never fail, but...
    if(!_plat__ACT_GetImplemented(act))
	result = TPM_RC_VALUE;
    else
	{
	    // Will need to clear orderly so fail if we are orderly and NV is not available
	    if(NV_IS_ORDERLY)
		RETURN_IF_NV_IS_NOT_AVAILABLE;
	    // if the attempt to update the counter fails, it means that there is an
	    // update pending so wait until it has occurred and then do an update.
	    if(!_plat__ACT_UpdateCounter(act, newValue))
		result = TPM_RC_RETRY;
	    else
	        {
	            // Indicate that the ACT has been updated since last TPM2_Startup().
	            s_ActUpdated |= (UINT16)(1 << act);

		    // Clear the preservedSignaled attribute.
		    go.preservedSignaled &= ~((UINT16)(1 << act));
		    
	            // Need to clear the orderly flag
	            g_clearOrderly = TRUE;
		    
	            result = TPM_RC_SUCCESS;
	        }
	}
    return result;
}
#endif		// libtpms added
/* 7.8.3.8	ActGetCapabilityData() */
/* This function returns the list of ACT data */
/* Return Value	Meaning */
/* YES	if more ACT data is available */
/* NO	if no more ACT data to */
TPMI_YES_NO
ActGetCapabilityData(
		     TPM_HANDLE       actHandle,     // IN: the handle for the starting ACT
		     UINT32           maxCount,      // IN: maximum allowed return values
		     TPML_ACT_DATA   *actList        // OUT: ACT data list
		     )
{
    // Initialize output property list
    actList->count = 0;
    
    // Make sure that the starting handle value is in range (again)
    if((actHandle < TPM_RH_ACT_0) || (actHandle > TPM_RH_ACT_F))
	return FALSE;
    // The maximum count of curves we may return is MAX_ECC_CURVES
    if(maxCount > MAX_ACT_DATA)
	maxCount = MAX_ACT_DATA;
    // Scan the ACT data from the starting ACT
    for(; actHandle <= TPM_RH_ACT_F; actHandle++)
	{
	    UINT32          act = actHandle - TPM_RH_ACT_0;
	    if(actList->count < maxCount)
	        {
	            if(ActIsImplemented(act))
			{
			    TPMS_ACT_DATA    *actData = &actList->actData[actList->count];
			    //
			    memset(&actData->attributes, 0, sizeof(actData->attributes));
			    actData->handle = actHandle;
			    actData->timeout = _plat__ACT_GetRemaining(act);
			    if (_plat__ACT_GetSignaled(act))
				SET_ATTRIBUTE(actData->attributes, TPMA_ACT, signaled);
			    else
				CLEAR_ATTRIBUTE(actData->attributes, TPMA_ACT, signaled);
			    actList->count++;
			}
		}
	    else
		{
		    if(_plat__ACT_GetImplemented(act))
			return YES;
		}
	}
    // If we get here, either all of the ACT values were put in the list, or the list
    // was filled and there are no more ACT values to return
    return NO;
}
