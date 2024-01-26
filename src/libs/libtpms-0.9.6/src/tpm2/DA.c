/********************************************************************************/
/*										*/
/*			   Dictionary Attack Logic.  				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: DA.c $			*/
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

/* 8.2 DA.c */
/* 8.2.1 Introduction */
/* This file contains the functions and data definitions relating to the dictionary attack logic. */
/* 8.2.2 Includes and Data Definitions */
#define DA_C
#include "Tpm.h"
/* 8.2.3 Functions */
/* 8.2.3.1 DAPreInstall_Init() */
/* This function initializes the DA parameters to their manufacturer-default values. The default
   values are determined by a platform-specific specification. */
/* This function should not be called outside of a manufacturing or simulation environment. */
/* The DA parameters will be restored to these initial values by TPM2_Clear(). */
void
DAPreInstall_Init(
		  void
		  )
{
    gp.failedTries = 0;
    gp.maxTries = 3;
    gp.recoveryTime = 1000;         // in seconds (~16.67 minutes)
    gp.lockoutRecovery = 1000;      // in seconds
    gp.lockOutAuthEnabled = TRUE;   // Use of lockoutAuth is enabled
    // Record persistent DA parameter changes to NV
    NV_SYNC_PERSISTENT(failedTries);
    NV_SYNC_PERSISTENT(maxTries);
    NV_SYNC_PERSISTENT(recoveryTime);
    NV_SYNC_PERSISTENT(lockoutRecovery);
    NV_SYNC_PERSISTENT(lockOutAuthEnabled);
    return;
}
/* 8.2.3.2 DAStartup() */
/* This function is called by TPM2_Startup() to initialize the DA parameters. In the case of
   Startup(CLEAR), use of lockoutAuth will be enabled if the lockout recovery time is 0. Otherwise,
   lockoutAuth will not be enabled until the TPM has been continuously powered for the
   lockoutRecovery time. */
/* This function requires that NV be available and not rate limiting. */
BOOL
DAStartup(
	  STARTUP_TYPE     type           // IN: startup type
	  )
{
    NOT_REFERENCED(type);
#if !ACCUMULATE_SELF_HEAL_TIMER
    _plat__TimerWasReset();
    s_selfHealTimer = 0;
    s_lockoutTimer = 0;
#else
    if(_plat__TimerWasReset())
	{
	    if(!NV_IS_ORDERLY)
		{
		    // If shutdown was not orderly, then don't really know if go.time has
		    // any useful value so reset the timer to 0. This is what the tick
		    // was reset to
		    s_selfHealTimer = 0;
		    s_lockoutTimer = 0;
		}
	    else
		{
		    // If we know how much time was accumulated at the last orderly shutdown
		    // subtract that from the saved timer values so that they effectively
		    // have the accumulated values
		    s_selfHealTimer -= go.time;
		    s_lockoutTimer -= go.time;
		}
	}
#endif
    // For any Startup(), if lockoutRecovery is 0, enable use of lockoutAuth.
    if(gp.lockoutRecovery == 0)
	{
	    gp.lockOutAuthEnabled = TRUE;
	    // Record the changes to NV
	    NV_SYNC_PERSISTENT(lockOutAuthEnabled);
	}
    // If DA has not been disabled and the previous shutdown is not orderly
    // failedTries is not already at its maximum then increment 'failedTries'
    if(gp.recoveryTime != 0
       && gp.failedTries < gp.maxTries
       && !IS_ORDERLY(g_prevOrderlyState))
	{
#if USE_DA_USED
	    gp.failedTries += g_daUsed;
	    g_daUsed = FALSE;
#else
	    gp.failedTries++;
#endif
	    // Record the change to NV
	    NV_SYNC_PERSISTENT(failedTries);
	}
    // Before Startup, the TPM will not do clock updates. At startup, need to
    // do a time update which will do the DA update.
    TimeUpdate();
    return TRUE;
}
/* 8.2.3.3 DARegisterFailure() */
/* This function is called when an authorization failure occurs on an entity that is subject to
   dictionary-attack protection. When a DA failure is triggered, register the failure by resetting
   the relevant self-healing timer to the current time. */
void
DARegisterFailure(
		  TPM_HANDLE       handle         // IN: handle for failure
		  )
{
    // Reset the timer associated with lockout if the handle is the lockoutAuth.
    if(handle == TPM_RH_LOCKOUT)
	s_lockoutTimer = g_time;
    else
	s_selfHealTimer = g_time;
    return;
}
/* 8.2.3.4 DASelfHeal() */
/* This function is called to check if sufficient time has passed to allow decrement of failedTries
   or to re-enable use of lockoutAuth. */
/* This function should be called when the time interval is updated. */
void
DASelfHeal(
	   void
	   )
{
    // Regular authorization self healing logic
    // If no failed authorization tries, do nothing.  Otherwise, try to
    // decrease failedTries
    if(gp.failedTries != 0)
	{
	    // if recovery time is 0, DA logic has been disabled.  Clear failed tries
	    // immediately
	    if(gp.recoveryTime == 0)
		{
		    gp.failedTries = 0;
		    // Update NV record
		    NV_SYNC_PERSISTENT(failedTries);
		}
	    else
		{
		    UINT64          decreaseCount;
#if 0 // Errata eliminates this code
		    // In the unlikely event that failedTries should become larger than
		    // maxTries
		    if(gp.failedTries > gp.maxTries)
			gp.failedTries = gp.maxTries;
#endif
		    // How much can failedTries be decreased
		    // Cast s_selfHealTimer to an int in case it became negative at
		    // startup
		    decreaseCount = ((g_time - (INT64)s_selfHealTimer) / 1000)
				    / gp.recoveryTime;
		    if(gp.failedTries <= (UINT32)decreaseCount)
			// should not set failedTries below zero
			gp.failedTries = 0;
		    else
			gp.failedTries -= (UINT32)decreaseCount;
		    // the cast prevents overflow of the product
		    s_selfHealTimer += (decreaseCount * (UINT64)gp.recoveryTime) * 1000;
		    if(decreaseCount != 0)
			// If there was a change to the failedTries, record the changes
			// to NV
			NV_SYNC_PERSISTENT(failedTries);
		}
	}
    // LockoutAuth self healing logic
    // If lockoutAuth is enabled, do nothing.  Otherwise, try to see if we
    // may enable it
    if(!gp.lockOutAuthEnabled)
	{
	    // if lockout authorization recovery time is 0, a reboot is required to
	    // re-enable use of lockout authorization.  Self-healing would not
	    // apply in this case.
	    if(gp.lockoutRecovery != 0)
		{
		    if(((g_time - (INT64)s_lockoutTimer) / 1000) >= gp.lockoutRecovery)
			{
			    gp.lockOutAuthEnabled = TRUE;
			    // Record the changes to NV
			    NV_SYNC_PERSISTENT(lockOutAuthEnabled);
			}
		}
	}
    return;
}
