/********************************************************************************/
/*										*/
/*		Platform specific entry and fail processing	   		*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: RunCommand.c $		*/
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

/* C.11 RunCommand.c */
/* C.11.1. Introduction */
/* This module provides the platform specific entry and fail processing. The _plat__RunCommand()
   function is used to call to ExecuteCommand() in the TPM code. This function does whatever
   processing is necessary to set up the platform in anticipation of the call to the TPM including
   setup for error processing. */
/* The _plat__Fail() function is called when there is a failure in the TPM. The TPM code will have
   set the flag to indicate that the TPM is in failure mode. This call will then recursively call
   ExecuteCommand() in order to build the failure mode response. When ExecuteCommand() returns to
   _plat__Fail(), the platform will do some platform specif operation to return to the environment
   in which the TPM is executing. For a simulator, setjmp/longjmp is used. For an OS, a system exit
   to the OS would be appropriate. */
/* C.11.2. Includes and locals */
#include "Platform.h"
#include <setjmp.h>
#include "ExecCommand_fp.h"
jmp_buf              s_jumpBuffer;
/* C.11.3. Functions */
/* C.11.3.1. _plat__RunCommand() */
/* This version of RunCommand() will set up a jum_buf and call ExecuteCommand(). If the command
   executes without failing, it will return and RunCommand() will return. If there is a failure in
   the command, then _plat__Fail() is called and it will longjump back to RunCommand() which will
   call ExecuteCommand() again. However, this time, the TPM will be in failure mode so
   ExecuteCommand() will simply build a failure response and return. */
LIB_EXPORT void
_plat__RunCommand(
		  uint32_t         requestSize,   // IN: command buffer size
		  unsigned char   *request,       // IN: command buffer
		  uint32_t        *responseSize,  // IN/OUT: response buffer size
		  unsigned char   **response      // IN/OUT: response buffer
		  )
{
    setjmp(s_jumpBuffer);
    ExecuteCommand(requestSize, request, responseSize, response);
}
/* C.11.3.2. _plat__Fail() */
/* This is the platform depended failure exit for the TPM. */
LIB_EXPORT NORETURN void
_plat__Fail(
	    void
	    )
{
    longjmp(&s_jumpBuffer[0], 1);
}
