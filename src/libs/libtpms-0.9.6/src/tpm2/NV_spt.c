/********************************************************************************/
/*										*/
/*			     				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: NV_spt.c $			*/
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
/*  (c) Copyright IBM Corp. and others, 2016, 2017				*/
/*										*/
/********************************************************************************/

/* 7.5 NV Command Support (NV_spt.c) */
/* 7.5.1 Includes */
#include "Tpm.h"
#include "NV_spt_fp.h"
/* 7.5.2 Functions */
/* 7.5.2.1 NvReadAccessChecks() */
/* Common routine for validating a read Used by TPM2_NV_Read(), TPM2_NV_ReadLock() and
   TPM2_PolicyNV() */
/* Error Returns Meaning */
/* TPM_RC_NV_AUTHORIZATION autHandle is not allowed to authorize read of the index */
/* TPM_RC_NV_LOCKED Read locked */
/* TPM_RC_NV_UNINITIALIZED Try to read an uninitialized index */
TPM_RC
NvReadAccessChecks(
		   TPM_HANDLE       authHandle,    // IN: the handle that provided the
		   //     authorization
		   TPM_HANDLE       nvHandle,      // IN: the handle of the NV index to be read
		   TPMA_NV          attributes     // IN: the attributes of 'nvHandle'
		   )
{
    // If data is read locked, returns an error
    if(IS_ATTRIBUTE(attributes, TPMA_NV, READLOCKED))
	return TPM_RC_NV_LOCKED;
    // If the authorization was provided by the owner or platform, then check
    // that the attributes allow the read.  If the authorization handle
    // is the same as the index, then the checks were made when the authorization
    // was checked..
    if(authHandle == TPM_RH_OWNER)
	{
	    // If Owner provided authorization then ONWERWRITE must be SET
	    if(!IS_ATTRIBUTE(attributes, TPMA_NV, OWNERREAD))
		return TPM_RC_NV_AUTHORIZATION;
	}
    else if(authHandle == TPM_RH_PLATFORM)
	{
	    // If Platform provided authorization then PPWRITE must be SET
	    if(!IS_ATTRIBUTE(attributes, TPMA_NV, PPREAD))
		return TPM_RC_NV_AUTHORIZATION;
	}
    // If neither Owner nor Platform provided authorization, make sure that it was
    // provided by this index.
    else if(authHandle != nvHandle)
	return TPM_RC_NV_AUTHORIZATION;
    // If the index has not been written, then the value cannot be read
    // NOTE: This has to come after other access checks to make sure that
    // the proper authorization is given to TPM2_NV_ReadLock()
    if(!IS_ATTRIBUTE(attributes, TPMA_NV, WRITTEN))
	return TPM_RC_NV_UNINITIALIZED;
    return TPM_RC_SUCCESS;
}
/* 7.5.2.2 NvWriteAccessChecks() */
/* Common routine for validating a write Used by TPM2_NV_Write(), TPM2_NV_Increment(),
   TPM2_SetBits(), and TPM2_NV_WriteLock() */
/* Error Returns Meaning */
/* TPM_RC_NV_AUTHORIZATION Authorization fails */
/* TPM_RC_NV_LOCKED Write locked */
TPM_RC
NvWriteAccessChecks(
		    TPM_HANDLE       authHandle,    // IN: the handle that provided the
		    //     authorization
		    TPM_HANDLE       nvHandle,      // IN: the handle of the NV index to be written
		    TPMA_NV          attributes     // IN: the attributes of 'nvHandle'
		    )
{
    // If data is write locked, returns an error
    if(IS_ATTRIBUTE(attributes, TPMA_NV, WRITELOCKED))
	return TPM_RC_NV_LOCKED;
    // If the authorization was provided by the owner or platform, then check
    // that the attributes allow the write.  If the authorization handle
    // is the same as the index, then the checks were made when the authorization
    // was checked..
    if(authHandle == TPM_RH_OWNER)
	{
	    // If Owner provided authorization then ONWERWRITE must be SET
	    if(!IS_ATTRIBUTE(attributes, TPMA_NV, OWNERWRITE))
		return TPM_RC_NV_AUTHORIZATION;
	}
    else if(authHandle == TPM_RH_PLATFORM)
	{
	    // If Platform provided authorization then PPWRITE must be SET
	    if(!IS_ATTRIBUTE(attributes, TPMA_NV, PPWRITE))
		return TPM_RC_NV_AUTHORIZATION;
	}
    // If neither Owner nor Platform provided authorization, make sure that it was
    // provided by this index.
    else if(authHandle != nvHandle)
	return TPM_RC_NV_AUTHORIZATION;
    return TPM_RC_SUCCESS;
}
/* 7.5.2.3 NvClearOrderly() */
/* This function is used to cause gp.orderlyState to be cleared to the non-orderly state. */
TPM_RC
NvClearOrderly(
	       void
	       )
{
    if(gp.orderlyState < SU_DA_USED_VALUE)
	RETURN_IF_NV_IS_NOT_AVAILABLE;
    g_clearOrderly = TRUE;
    return TPM_RC_SUCCESS;
}
/* 7.5.2.4 NvIsPinPassIndex() */
/* Function to check to see if an NV index is a PIN Pass Index */
/* Return Value Meaning */
/* TRUE is pin pass */
/* FALSE is not pin pass */
BOOL
NvIsPinPassIndex(
		 TPM_HANDLE          index       // IN: Handle to check
		 )
{
    if(HandleGetType(index) == TPM_HT_NV_INDEX)
	{
	    NV_INDEX                *nvIndex = NvGetIndexInfo(index, NULL);
	    return IsNvPinPassIndex(nvIndex->publicArea.attributes);
	}
    return FALSE;
}
