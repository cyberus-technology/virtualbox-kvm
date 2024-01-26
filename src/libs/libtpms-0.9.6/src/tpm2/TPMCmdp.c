/********************************************************************************/
/*										*/
/*			  Process the commands    				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: TPMCmdp.c $		*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2021				*/
/*										*/
/********************************************************************************/

/* D.4 TPMCmdp.c */
/* D.4.1. Description */
/* This file contains the functions that process the commands received on the control port or the
   command port of the simulator. The control port is used to allow simulation of hardware events
   (such as, _TPM_Hash_Start()) to test the simulated TPM's reaction to those events. This improves
   code coverage of the testing. */
/* D.4.2. Includes and Data Definitions */
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdbool.h>
#include "TpmBuildSwitches.h"
#ifndef VBOX
#ifdef TPM_WINDOWS
#include <windows.h>
#include <winsock.h>
#endif
#endif
#include "Platform_fp.h"
#include "PlatformACT_fp.h"
#include "ExecCommand_fp.h"
#include "Manufacture_fp.h"
#include "_TPM_Init_fp.h"
#include "_TPM_Hash_Start_fp.h"
#include "_TPM_Hash_Data_fp.h"
#include "_TPM_Hash_End_fp.h"
#include "TpmFail_fp.h"
#include "TpmTcpProtocol.h"
#include "Simulator_fp.h"
#ifndef VBOX
#ifdef TPM_WINDOWS
#include "TcpServer_fp.h"	/* kgold */
#endif
#ifdef TPM_POSIX
#include "TcpServerPosix_fp.h"	/* kgold */
#endif
#endif
#include "TpmProfile.h"		/* kgold */

static bool     s_isPowerOn = false;
/* D.4.3. Functions */
/* D.4.3.1. Signal_PowerOn() */
/* This function processes a power-on indication. Among other things, it calls the _TPM_Init()
   handler. */
void
_rpc__Signal_PowerOn(
		     bool        isReset
		     )
{
    // if power is on and this is not a call to do TPM reset then return
    if(s_isPowerOn && !isReset)
	return;
    // If this is a reset but power is not on, then return
    if(isReset && !s_isPowerOn)
	return;
    // Unless this is just a reset, pass power on signal to platform
    if(!isReset)
	_plat__Signal_PowerOn();
    // Power on and reset both lead to _TPM_Init()
    _plat__Signal_Reset();
    // Set state as power on
    s_isPowerOn = true;
}
#if 0 /* libtpms added */
/* D.4.3.2. Signal_Restart() */
/* This function processes the clock restart indication. All it does is call the platform
   function. */
void
_rpc__Signal_Restart(
		     void
		     )
{
    _plat__TimerRestart();
}
#endif /* libtpms added */
/* D.4.3.3. Signal_PowerOff() */
/* This function processes the power off indication. Its primary function is to set a flag
   indicating that the next power on indication should cause _TPM_Init() to be called. */
void
_rpc__Signal_PowerOff(
		      void
		      )
{
    if(s_isPowerOn)
	// Pass power off signal to platform
	_plat__Signal_PowerOff();
    // This could be redundant, but...
    s_isPowerOn = false;
    return;
}
#if 0 /* libtpms added */
/* D.4.3.4. _rpc__ForceFailureMode() */
/* This function is used to debug the Failure Mode logic of the TPM. It will set a flag in the TPM
   code such that the next call to TPM2_SelfTest() will result in a failure, putting the TPM into
   Failure Mode. */
void
_rpc__ForceFailureMode(
		       void
		       )
{
    SetForceFailureMode();
    return;
}
/* D.4.3.5. _rpc__Signal_PhysicalPresenceOn() */
/* This function is called to simulate activation of the physical presence pin. */
void
_rpc__Signal_PhysicalPresenceOn(
				void
				)
{
    // If TPM power is on
    if(s_isPowerOn)
	// Pass physical presence on to platform
	_plat__Signal_PhysicalPresenceOn();
    return;
}
/* D.4.3.6. _rpc__Signal_PhysicalPresenceOff() */
/* This function is called to simulate deactivation of the physical presence pin. */
void
_rpc__Signal_PhysicalPresenceOff(
				 void
				 )
{
    // If TPM power is on
    if(s_isPowerOn)
	// Pass physical presence off to platform
	_plat__Signal_PhysicalPresenceOff();
    return;
}
/* D.4.3.7. _rpc__Signal_Hash_Start() */
/* This function is called to simulate a _TPM_Hash_Start() event. It will call */
void
_rpc__Signal_Hash_Start(
			void
			)
{
    // If TPM power is on
    if(s_isPowerOn)
	// Pass _TPM_Hash_Start signal to TPM
	_TPM_Hash_Start();
    return;
}
/* D.4.3.8. _rpc__Signal_Hash_Data() */
/* This function is called to simulate a _TPM_Hash_Data() event. */
void
_rpc__Signal_Hash_Data(
		       _IN_BUFFER       input
		       )
{
    // If TPM power is on
    if(s_isPowerOn)
	// Pass _TPM_Hash_Data signal to TPM
	_TPM_Hash_Data(input.BufferSize, input.Buffer);
    return;
}
/* D.4.3.9. _rpc__Signal_HashEnd() */
/* This function is called to simulate a _TPM_Hash_End() event. */
void
_rpc__Signal_HashEnd(
		     void
		     )
{
    // If TPM power is on
    if(s_isPowerOn)
	// Pass _TPM_HashEnd signal to TPM
	_TPM_Hash_End();
    return;
}
#endif /* libtpms added */
/* D.4.3.10. rpc_Send_Command() */
/* This is the interface to the TPM code. */
void
_rpc__Send_Command(
		   unsigned char    locality,
		   _IN_BUFFER       request,
		   _OUT_BUFFER     *response
		   )
{
    // If TPM is power off, reject any commands.
    if(!s_isPowerOn)
	{
	    response->BufferSize = 0;
	    return;
	}
    // Set the locality of the command so that it doesn't change during the command
    _plat__LocalitySet(locality);
    // Do implementation-specific command dispatch
    _plat__RunCommand(request.BufferSize, request.Buffer,
		      &response->BufferSize, &response->Buffer);
    return;
}
/* D.4.3.10. _rpc__Signal_CancelOn() */
/* This function is used to turn on the indication to cancel a command in process. An executing
   command is not interrupted. The command code may periodically check this indication to see if it
   should abort the current command processing and returned TPM_RC_CANCELLED. */
void
_rpc__Signal_CancelOn(
		      void
		      )
{
    // If TPM is power off, reject this signal
    if(s_isPowerOn)
	// Set the platform canceling flag.
	_plat__SetCancel();
    return;
}
/* D.4.3.11. _rpc__Signal_CancelOff() */
/* This function is used to turn off the indication to cancel a command in process. */
void
_rpc__Signal_CancelOff(
		       void
		       )
{
    // If TPM power is n
    if(s_isPowerOn)
	// Set the platform canceling flag.
	_plat__ClearCancel();
    return;
}
/* D.4.3.12. _rpc__Signal_NvOn() */
/* In a system where the NV memory used by the TPM is not within the TPM, the NV may not always be
   available. This function turns on the indicator that indicates that NV is available. */
void
_rpc__Signal_NvOn(
		  void
		  )
{
    // If TPM power is on
    if(s_isPowerOn)
	// Make the NV available
	_plat__SetNvAvail();
    return;
}
#if 0 /* libtpms added */
/* D.4.3.13. _rpc__Signal_NvOff() */
/* This function is used to set the indication that NV memory is no longer available. */
void
_rpc__Signal_NvOff(
		   void
		   )
{
    // If TPM power is on
    if(s_isPowerOn)
	// Make NV not available
	_plat__ClearNvAvail();
    return;
}
void RsaKeyCacheControl(int state);
/* D.4.3.14. _rpc__RsaKeyCacheControl() */
/* This function is used to enable/disable the use of the RSA key cache during simulation. */
void
_rpc__RsaKeyCacheControl(
			 int              state
			 )
{
#if USE_RSA_KEY_CACHE
    RsaKeyCacheControl(state);
#else
    NOT_REFERENCED(state);
#endif
}

#define TPM_RH_ACT_0        0x40000110

/* D.4.2.15.	_rpc__ACT_GetSignaled() */
/* This function is used to count the ACT second tick. */
bool
_rpc__ACT_GetSignaled(
		      uint32_t actHandle
		      )
{
    // If TPM power is on
    if (s_isPowerOn)
	// Query the platform
	return _plat__ACT_GetSignaled(actHandle - TPM_RH_ACT_0);
    return false;
}
#endif /* libtpms added */

/* libtpms added begin */
static bool tpmEstablished;

void
_rpc__Signal_SetTPMEstablished(void)
{
    tpmEstablished = TRUE;
}

void
_rpc__Signal_ResetTPMEstablished(void)
{
    /* check for locality 3 or 4 already done by caller */
    tpmEstablished = FALSE;
}

bool
_rpc__Signal_GetTPMEstablished(void)
{
    return tpmEstablished;
}

bool
_rpc__Signal_IsPowerOn(void)
{
    return s_isPowerOn;
}
/* libtpms added end */
