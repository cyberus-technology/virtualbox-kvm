/********************************************************************************/
/*										*/
/*			Miscellaneous Management Functions	   		*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: ManagementCommands.c $	*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2018				*/
/*										*/
/********************************************************************************/

#include "Tpm.h"
#include "PP_Commands_fp.h"
#if CC_PP_Commands  // Conditional expansion of this file
TPM_RC
TPM2_PP_Commands(
		 PP_Commands_In  *in             // IN: input parameter list
		 )
{
    UINT32          i;
    // The command needs NV update.  Check if NV is available.
    // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
    // this point
    RETURN_IF_NV_IS_NOT_AVAILABLE;
    // Internal Data Update
    // Process set list
    for(i = 0; i < in->setList.count; i++)
	// If command is implemented, set it as PP required.  If the input
	// command is not a PP command, it will be ignored at
	// PhysicalPresenceCommandSet().
	// Note: PhysicalPresenceCommandSet() checks if the command is implemented.
	PhysicalPresenceCommandSet(in->setList.commandCodes[i]);
    // Process clear list
    for(i = 0; i < in->clearList.count; i++)
	// If command is implemented, clear it as PP required.  If the input
	// command is not a PP command, it will be ignored at
	// PhysicalPresenceCommandClear().  If the input command is
	// TPM2_PP_Commands, it will be ignored as well
	PhysicalPresenceCommandClear(in->clearList.commandCodes[i]);
    // Save the change of PP list
    NV_SYNC_PERSISTENT(ppList);
    return TPM_RC_SUCCESS;
}
#endif // CC_PP_Commands
#include "Tpm.h"
#include "SetAlgorithmSet_fp.h"
#if CC_SetAlgorithmSet  // Conditional expansion of this file
TPM_RC
TPM2_SetAlgorithmSet(
		     SetAlgorithmSet_In  *in             // IN: input parameter list
		     )
{
    // The command needs NV update.  Check if NV is available.
    // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
    // this point
    RETURN_IF_NV_IS_NOT_AVAILABLE;
    // Internal Data Update
    gp.algorithmSet = in->algorithmSet;
    // Write the algorithm set changes to NV
    NV_SYNC_PERSISTENT(algorithmSet);
    return TPM_RC_SUCCESS;
}
#endif // CC_SetAlgorithmSet
