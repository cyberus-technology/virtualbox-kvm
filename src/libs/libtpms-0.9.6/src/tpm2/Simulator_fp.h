/********************************************************************************/
/*										*/
/*			     				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Simulator_fp.h $		*/
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

/* D.2 Simulator_fp.h */
#ifndef    _SIMULATOR_FP_H_
#define    _SIMULATOR_FP_H_

#include <stdbool.h>

#ifdef TPM_WINDOWS
#include <windows.h>
#include <winsock.h>
#endif

#ifdef TPM_POSIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#endif

/* D.2.1. From TcpServer.c */
/* D.2.1.1. PlatformServer() */
/* Moved to TpmServer_fp.h because it's different for Windows and Unix */

/* D.2.2. From TPMCmdp.c */
/* D.2.2.1. Signal_PowerOn() */
/* This function processes a power-on indication. Among other things, it calls the _TPM_Init()
   handler. */
void
_rpc__Signal_PowerOn(
		     bool        isReset
		     );
/* D.2.2.2. Signal_Restart() */
/* This function processes the clock restart indication. All it does is call the platform
   function. */
void
_rpc__Signal_Restart(
		     void
		     );
/* D.2.2.3. Signal_PowerOff() */
/* This function processes the power off indication. Its primary function is to set a flag
   indicating that the next power on indication should cause _TPM_Init() to be called. */
void
_rpc__Signal_PowerOff(
		      void
		      );
#if 0 /* libtpms added */
/* D.2.2.4. _rpc__ForceFailureMode() */
/* This function is used to debug the Failure Mode logic of the TPM. It will set a flag in the TPM
   code such that the next call to TPM2_SelfTest() will result in a failure, putting the TPM into
   Failure Mode. */
void
_rpc__ForceFailureMode(
		       void
		       );
/* D.2.2.5. _rpc__Signal_PhysicalPresenceOn() */
/* This function is called to simulate activation of the physical presence pin. */
void
_rpc__Signal_PhysicalPresenceOn(
				void
				);
/* D.2.2.6. _rpc__Signal_PhysicalPresenceOff() */
/* This function is called to simulate deactivation of the physical presence pin. */
void
_rpc__Signal_PhysicalPresenceOff(
				 void
				 );
/* D.2.2.7. _rpc__Signal_Hash_Start() */
/* This function is called to simulate a _TPM_Hash_Start() event. It will call */
void
_rpc__Signal_Hash_Start(
			void
			);
/* D.2.2.8. _rpc__Signal_Hash_Data() */
/* This function is called to simulate a _TPM_Hash_Data() event. */
void
_rpc__Signal_Hash_Data(
		       _IN_BUFFER       input
		       );
/* D.2.2.9. _rpc__Signal_HashEnd() */
/* This function is called to simulate a _TPM_Hash_End() event. */
void
_rpc__Signal_HashEnd(
		     void
		     );
#endif /* libtpms added */
/* rpc__Send_Command() This is the interface to the TPM code. */
void
_rpc__Send_Command(
		   unsigned char    locality,
		   _IN_BUFFER       request,
		   _OUT_BUFFER     *response
		   );

/* D.2.2.10. _rpc__Signal_CancelOn() */
/* This function is used to turn on the indication to cancel a command in process. An executing
   command is not interrupted. The command code may periodically check this indication to see if it
   should abort the current command processing and returned TPM_RC_CANCELLED. */
void
_rpc__Signal_CancelOn(
		      void
		      );
/* D.2.2.11. _rpc__Signal_CancelOff() */
/* This function is used to turn off the indication to cancel a command in process. */
void
_rpc__Signal_CancelOff(
		       void
		       );
/* D.2.2.12. _rpc__Signal_NvOn() */
/* In a system where the NV memory used by the TPM is not within the TPM, the NV may not always be
   available. This function turns on the indicator that indicates that NV is available. */
void
_rpc__Signal_NvOn(
		  void
		  );
#if 0 /* libtpms added */
/* D.2.2.13. _rpc__Signal_NvOff() */
/* This function is used to set the indication that NV memory is no longer available. */
void
_rpc__Signal_NvOff(
		   void
		   );
/* D.2.2.14. _rpc__RsaKeyCacheControl() */
/* This function is used to enable/disable the use of the RSA key cache during simulation. */
void
_rpc__RsaKeyCacheControl(
			 int              state
			 );
/* D.4.2.15.	_rpc__ACT_GetSignaled() */
bool
_rpc__ACT_GetSignaled(
		      uint32_t actHandle
		      );

/* D.2.3. From TPMCmds.c */
/* D.2.3.1. main() */
/* This is the main entry point for the simulator. */
int
main(
     int              argc,
     char            *argv[]
     );
#endif /* libtpms added */
/* libtpms added begin */
void _rpc__Signal_SetTPMEstablished(void);
bool _rpc__Signal_GetTPMEstablished(void);
void _rpc__Signal_ResetTPMEstablished(void);
bool _rpc__Signal_IsPowerOn(void);
/* libtpms added end */

#endif  // _SIMULATOR_FP_H_

