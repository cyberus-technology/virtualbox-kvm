/********************************************************************************/
/*										*/
/*			     Hierarchy Commands					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: HierarchyCommands.c $	*/
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

#include "Tpm.h"
#include "CreatePrimary_fp.h"
#if CC_CreatePrimary  // Conditional expansion of this file
TPM_RC
TPM2_CreatePrimary(
		   CreatePrimary_In    *in,            // IN: input parameter list
		   CreatePrimary_Out   *out            // OUT: output parameter list
		   )
{
    TPM_RC              result = TPM_RC_SUCCESS;
    TPMT_PUBLIC         *publicArea;
    DRBG_STATE           rand;
    OBJECT              *newObject;
    TPM2B_NAME           name;
    // Input Validation
    // Will need a place to put the result
    newObject = FindEmptyObjectSlot(&out->objectHandle);
    if(newObject == NULL)
	return TPM_RC_OBJECT_MEMORY;
    // Get the address of the public area in the new object
    // (this is just to save typing)
    publicArea = &newObject->publicArea;
    *publicArea = in->inPublic.publicArea;
    // Check attributes in input public area. CreateChecks() checks the things that
    // are unique to creation and then validates the attributes and values that are
    // common to create and load.
    result = CreateChecks(NULL, publicArea,
			  in->inSensitive.sensitive.data.t.size);
    if(result != TPM_RC_SUCCESS)
	return RcSafeAddToResult(result, RC_CreatePrimary_inPublic);
    // Validate the sensitive area values
    if(!AdjustAuthSize(&in->inSensitive.sensitive.userAuth,
		       publicArea->nameAlg))
	return TPM_RCS_SIZE + RC_CreatePrimary_inSensitive;
    // Command output
    // Compute the name using out->name as a scratch area (this is not the value
    // that ultimately will be returned, then instantiate the state that will be
    // used as a random number generator during the object creation.
    // The caller does not know the seed values so the actual name does not have
    // to be over the input, it can be over the unmarshaled structure.
    result = DRBG_InstantiateSeeded(&rand,
				    &HierarchyGetPrimarySeed(in->primaryHandle)->b,
				    PRIMARY_OBJECT_CREATION,
				    (TPM2B *)PublicMarshalAndComputeName(publicArea, &name),
				    &in->inSensitive.sensitive.data.b,
				    HierarchyGetPrimarySeedCompatLevel(in->primaryHandle)); // libtpms added
    if (result == TPM_RC_SUCCESS)
	{
	    newObject->attributes.primary = SET;
	    if(in->primaryHandle == TPM_RH_ENDORSEMENT)
		newObject->attributes.epsHierarchy = SET;
	    // Create the primary object.
	    result = CryptCreateObject(newObject, &in->inSensitive.sensitive,
				       (RAND_STATE *)&rand);
	}
    if(result != TPM_RC_SUCCESS)
	return result;
    // Set the publicArea and name from the computed values
    out->outPublic.publicArea = newObject->publicArea;
    out->name = newObject->name;
    // Fill in creation data
    FillInCreationData(in->primaryHandle, publicArea->nameAlg,
		       &in->creationPCR, &in->outsideInfo, &out->creationData,
		       &out->creationHash);
    // Compute creation ticket
    TicketComputeCreation(EntityGetHierarchy(in->primaryHandle), &out->name,
			  &out->creationHash, &out->creationTicket);
    // Set the remaining attributes for a loaded object
    ObjectSetLoadedAttributes(newObject, in->primaryHandle,
                              HierarchyGetPrimarySeedCompatLevel(in->primaryHandle)); // libtpms added
    return result;
}
#endif // CC_CreatePrimary
#include "Tpm.h"
#include "HierarchyControl_fp.h"
#if CC_HierarchyControl  // Conditional expansion of this file
TPM_RC
TPM2_HierarchyControl(
		      HierarchyControl_In     *in             // IN: input parameter list
		      )
{
    BOOL        select = (in->state == YES);
    BOOL        *selected = NULL;
    // Input Validation
    switch(in->enable)
	{
	    // Platform hierarchy has to be disabled by PlatformAuth
	    // If the platform hierarchy has already been disabled, only a reboot
	    // can enable it again
	  case TPM_RH_PLATFORM:
	  case TPM_RH_PLATFORM_NV:
	    if(in->authHandle != TPM_RH_PLATFORM)
		return TPM_RC_AUTH_TYPE;
	    break;
	    // ShEnable may be disabled if PlatformAuth/PlatformPolicy or
	    // OwnerAuth/OwnerPolicy is provided.  If ShEnable is disabled, then it
	    // may only be enabled if PlatformAuth/PlatformPolicy is provided.
	  case TPM_RH_OWNER:
	    if(in->authHandle != TPM_RH_PLATFORM
	       && in->authHandle != TPM_RH_OWNER)
		return TPM_RC_AUTH_TYPE;
	    if(gc.shEnable == FALSE && in->state == YES
	       && in->authHandle != TPM_RH_PLATFORM)
		return TPM_RC_AUTH_TYPE;
	    break;
	    // EhEnable may be disabled if either PlatformAuth/PlatformPolicy or
	    // EndosementAuth/EndorsementPolicy is provided.  If EhEnable is disabled,
	    // then it may only be enabled if PlatformAuth/PlatformPolicy is
	    // provided.
	  case TPM_RH_ENDORSEMENT:
	    if(in->authHandle != TPM_RH_PLATFORM
	       && in->authHandle != TPM_RH_ENDORSEMENT)
		return TPM_RC_AUTH_TYPE;
	    if(gc.ehEnable == FALSE && in->state == YES
	       && in->authHandle != TPM_RH_PLATFORM)
		return TPM_RC_AUTH_TYPE;
	    break;
	  default:
	    FAIL(FATAL_ERROR_INTERNAL);
	    break;
	}
    // Internal Data Update
    // Enable or disable the selected hierarchy
    // Note: the authorization processing for this command may keep these
    // command actions from being executed. For example, if phEnable is
    // CLEAR, then platformAuth cannot be used for authorization. This
    // means that would not be possible to use platformAuth to change the
    // state of phEnable from CLEAR to SET.
    // If it is decided that platformPolicy can still be used when phEnable
    // is CLEAR, then this code could SET phEnable when proper platform
    // policy is provided.
    switch(in->enable)
	{
	  case TPM_RH_OWNER:
	    selected = &gc.shEnable;
	    break;
	  case TPM_RH_ENDORSEMENT:
	    selected = &gc.ehEnable;
	    break;
	  case TPM_RH_PLATFORM:
	    selected = &g_phEnable;
	    break;
	  case TPM_RH_PLATFORM_NV:
	    selected = &gc.phEnableNV;
	    break;
	  default:
	    FAIL(FATAL_ERROR_INTERNAL);
	    break;
	}
    if(selected != NULL && *selected != select)
	{
	    // Before changing the internal state, make sure that NV is available.
	    // Only need to update NV if changing the orderly state
	    RETURN_IF_ORDERLY;
	    // state is changing and NV is available so modify
	    *selected = select;
	    // If a hierarchy was just disabled, flush it
	    if(select == CLEAR && in->enable != TPM_RH_PLATFORM_NV)
	        // Flush hierarchy
		ObjectFlushHierarchy(in->enable);
	    // orderly state should be cleared because of the update to state clear data
	    // This gets processed in ExecuteCommand() on the way out.
	    g_clearOrderly = TRUE;
	}
    return TPM_RC_SUCCESS;
}
#endif // CC_HierarchyControl
#include "Tpm.h"
#include "SetPrimaryPolicy_fp.h"
#if CC_SetPrimaryPolicy  // Conditional expansion of this file
TPM_RC
TPM2_SetPrimaryPolicy(
		      SetPrimaryPolicy_In     *in             // IN: input parameter list
		      )
{
    // Input Validation
    // Check the authPolicy consistent with hash algorithm. If the policy size is
    // zero, then the algorithm is required to be TPM_ALG_NULL
    if(in->authPolicy.t.size != CryptHashGetDigestSize(in->hashAlg))
	return TPM_RCS_SIZE + RC_SetPrimaryPolicy_authPolicy;
    // The command need NV update for OWNER and ENDORSEMENT hierarchy, and
    // might need orderlyState update for PLATFORM hierarchy.
    // Check if NV is available.  A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE
    // error may be returned at this point
    RETURN_IF_NV_IS_NOT_AVAILABLE;
    // Internal Data Update
    // Set hierarchy policy
    switch(in->authHandle)
	{
	  case TPM_RH_OWNER:
	    gp.ownerAlg = in->hashAlg;
	    gp.ownerPolicy = in->authPolicy;
	    NV_SYNC_PERSISTENT(ownerAlg);
	    NV_SYNC_PERSISTENT(ownerPolicy);
	    break;
	  case TPM_RH_ENDORSEMENT:
	    gp.endorsementAlg = in->hashAlg;
	    gp.endorsementPolicy = in->authPolicy;
	    NV_SYNC_PERSISTENT(endorsementAlg);
	    NV_SYNC_PERSISTENT(endorsementPolicy);
	    break;
	  case TPM_RH_PLATFORM:
	    gc.platformAlg = in->hashAlg;
	    gc.platformPolicy = in->authPolicy;
	    // need to update orderly state
	    g_clearOrderly = TRUE;
	    break;
	  case TPM_RH_LOCKOUT:
	    gp.lockoutAlg = in->hashAlg;
	    gp.lockoutPolicy = in->authPolicy;
	    NV_SYNC_PERSISTENT(lockoutAlg);
	    NV_SYNC_PERSISTENT(lockoutPolicy);
	    break;

#define SET_ACT_POLICY(N)						\
	    case TPM_RH_ACT_##N:					\
	      go.ACT_##N.hashAlg = in->hashAlg;				\
	      go.ACT_##N.authPolicy = in->authPolicy;			\
	      g_clearOrderly = TRUE;					\
	      break;
	    
	    FOR_EACH_ACT(SET_ACT_POLICY)
	    
	  default:
	    FAIL(FATAL_ERROR_INTERNAL);
	    break;
	}
    return TPM_RC_SUCCESS;
}
#endif // CC_SetPrimaryPolicy
#include "Tpm.h"
#include "ChangePPS_fp.h"
#if CC_ChangePPS  // Conditional expansion of this file
TPM_RC
TPM2_ChangePPS(
	       ChangePPS_In    *in             // IN: input parameter list
	       )
{
    UINT32          i;
    // Check if NV is available.  A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE
    // error may be returned at this point
    RETURN_IF_NV_IS_NOT_AVAILABLE;
    // Input parameter is not reference in command action
    NOT_REFERENCED(in);
    // Internal Data Update
    // Reset platform hierarchy seed from RNG
    CryptRandomGenerate(sizeof(gp.PPSeed.t.buffer), gp.PPSeed.t.buffer);
    gp.PPSeedCompatLevel = SEED_COMPAT_LEVEL_LAST; // libtpms added
    // Create a new phProof value from RNG to prevent the saved platform
    // hierarchy contexts being loaded
    CryptRandomGenerate(sizeof(gp.phProof.t.buffer), gp.phProof.t.buffer);
    // Set platform authPolicy to null
    gc.platformAlg = TPM_ALG_NULL;
    gc.platformPolicy.t.size = 0;
    // Flush loaded object in platform hierarchy
    ObjectFlushHierarchy(TPM_RH_PLATFORM);
    // Flush platform evict object and index in NV
    NvFlushHierarchy(TPM_RH_PLATFORM);
    // Save hierarchy changes to NV
    NV_SYNC_PERSISTENT(PPSeed);
    NV_SYNC_PERSISTENT(PPSeedCompatLevel); // libtpms added
    NV_SYNC_PERSISTENT(phProof);
    // Re-initialize PCR policies
#if defined NUM_POLICY_PCR_GROUP && NUM_POLICY_PCR_GROUP > 0
    for(i = 0; i < NUM_POLICY_PCR_GROUP; i++)
	{
	    gp.pcrPolicies.hashAlg[i] = TPM_ALG_NULL;
	    gp.pcrPolicies.policy[i].t.size = 0;
	}
    NV_SYNC_PERSISTENT(pcrPolicies);
#endif
    // orderly state should be cleared because of the update to state clear data
    g_clearOrderly = TRUE;
    return TPM_RC_SUCCESS;
}
#endif // CC_ChangePPS
#include "Tpm.h"
#include "ChangeEPS_fp.h"
#if CC_ChangeEPS  // Conditional expansion of this file
TPM_RC
TPM2_ChangeEPS(
	       ChangeEPS_In    *in             // IN: input parameter list
	       )
{
    // The command needs NV update.  Check if NV is available.
    // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
    // this point
    RETURN_IF_NV_IS_NOT_AVAILABLE;
    // Input parameter is not reference in command action
    NOT_REFERENCED(in);
    // Internal Data Update
    // Reset endorsement hierarchy seed from RNG
    CryptRandomGenerate(sizeof(gp.EPSeed.t.buffer), gp.EPSeed.t.buffer);
    gp.EPSeedCompatLevel = SEED_COMPAT_LEVEL_LAST; // libtpms added
    // Create new ehProof value from RNG
    CryptRandomGenerate(sizeof(gp.ehProof.t.buffer), gp.ehProof.t.buffer);
    // Enable endorsement hierarchy
    gc.ehEnable = TRUE;
    // set authValue buffer to zeros
    MemorySet(gp.endorsementAuth.t.buffer, 0, gp.endorsementAuth.t.size);
    // Set endorsement authValue to null
    gp.endorsementAuth.t.size = 0;
    // Set endorsement authPolicy to null
    gp.endorsementAlg = TPM_ALG_NULL;
    gp.endorsementPolicy.t.size = 0;
    // Flush loaded object in endorsement hierarchy
    ObjectFlushHierarchy(TPM_RH_ENDORSEMENT);
    // Flush evict object of endorsement hierarchy stored in NV
    NvFlushHierarchy(TPM_RH_ENDORSEMENT);
    // Save hierarchy changes to NV
    NV_SYNC_PERSISTENT(EPSeed);
    NV_SYNC_PERSISTENT(EPSeedCompatLevel); // libtpms added
    NV_SYNC_PERSISTENT(ehProof);
    NV_SYNC_PERSISTENT(endorsementAuth);
    NV_SYNC_PERSISTENT(endorsementAlg);
    NV_SYNC_PERSISTENT(endorsementPolicy);
    // orderly state should be cleared because of the update to state clear data
    g_clearOrderly = TRUE;
    return TPM_RC_SUCCESS;
}
#endif // CC_ChangeEPS
#include "Tpm.h"
#include "Clear_fp.h"
#if CC_Clear  // Conditional expansion of this file
TPM_RC
TPM2_Clear(
	   Clear_In        *in             // IN: input parameter list
	   )
{
    // Input parameter is not reference in command action
    NOT_REFERENCED(in);
    // The command needs NV update.  Check if NV is available.
    // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
    // this point
    RETURN_IF_NV_IS_NOT_AVAILABLE;
    // Input Validation
    // If Clear command is disabled, return an error
    if(gp.disableClear)
	return TPM_RC_DISABLED;
    // Internal Data Update
    // Reset storage hierarchy seed from RNG
    CryptRandomGenerate(sizeof(gp.SPSeed.t.buffer), gp.SPSeed.t.buffer);
    gp.SPSeedCompatLevel = SEED_COMPAT_LEVEL_LAST; // libtpms added
    // Create new shProof and ehProof value from RNG
    CryptRandomGenerate(sizeof(gp.shProof.t.buffer), gp.shProof.t.buffer);
    CryptRandomGenerate(sizeof(gp.ehProof.t.buffer), gp.ehProof.t.buffer);
    // Enable storage and endorsement hierarchy
    gc.shEnable = gc.ehEnable = TRUE;
    // set the authValue buffers to zero
    MemorySet(&gp.ownerAuth, 0, sizeof(gp.ownerAuth));
    MemorySet(&gp.endorsementAuth, 0, sizeof(gp.endorsementAuth));
    MemorySet(&gp.lockoutAuth, 0, sizeof(gp.lockoutAuth));
    // Set storage, endorsement, and lockout authPolicy to null
    gp.ownerAlg = gp.endorsementAlg = gp.lockoutAlg = TPM_ALG_NULL;
    MemorySet(&gp.ownerPolicy, 0, sizeof(gp.ownerPolicy));
    MemorySet(&gp.endorsementPolicy, 0, sizeof(gp.endorsementPolicy));
    MemorySet(&gp.lockoutPolicy, 0, sizeof(gp.lockoutPolicy));
    // Flush loaded object in storage and endorsement hierarchy
    ObjectFlushHierarchy(TPM_RH_OWNER);
    ObjectFlushHierarchy(TPM_RH_ENDORSEMENT);
    // Flush owner and endorsement object and owner index in NV
    NvFlushHierarchy(TPM_RH_OWNER);
    NvFlushHierarchy(TPM_RH_ENDORSEMENT);
    // Initialize dictionary attack parameters
    DAPreInstall_Init();
    // Reset clock
    go.clock = 0;
    go.clockSafe = YES;
    NvWrite(NV_ORDERLY_DATA, sizeof(ORDERLY_DATA), &go);
    // Reset counters
    gp.resetCount = gr.restartCount = gr.clearCount = 0;
    gp.auditCounter = 0;
    // Save persistent data changes to NV
    // Note: since there are so many changes to the persistent data structure, the
    // entire PERSISTENT_DATA structure is written as a unit
    NvWrite(NV_PERSISTENT_DATA, sizeof(PERSISTENT_DATA), &gp);
    // Reset the PCR authValues (this does not change the PCRs)
    PCR_ClearAuth();
    // Bump the PCR counter
    PCRChanged(0);
    // orderly state should be cleared because of the update to state clear data
    g_clearOrderly = TRUE;
    return TPM_RC_SUCCESS;
}
#endif // CC_Clear
#include "Tpm.h"
#include "ClearControl_fp.h"
#if CC_ClearControl  // Conditional expansion of this file
TPM_RC
TPM2_ClearControl(
		  ClearControl_In     *in             // IN: input parameter list
		  )
{
    // The command needs NV update.
    RETURN_IF_NV_IS_NOT_AVAILABLE;
    // Input Validation
    // LockoutAuth may be used to set disableLockoutClear to TRUE but not to FALSE
    if(in->auth == TPM_RH_LOCKOUT && in->disable == NO)
	return TPM_RC_AUTH_FAIL;
    // Internal Data Update
    if(in->disable == YES)
	gp.disableClear = TRUE;
    else
	gp.disableClear = FALSE;
    // Record the change to NV
    NV_SYNC_PERSISTENT(disableClear);
    return TPM_RC_SUCCESS;
}
#endif // CC_ClearControl
#include "Tpm.h"
#include "HierarchyChangeAuth_fp.h"
#if CC_HierarchyChangeAuth  // Conditional expansion of this file
#include "Object_spt_fp.h"
TPM_RC
TPM2_HierarchyChangeAuth(
			 HierarchyChangeAuth_In  *in             // IN: input parameter list
			 )
{
    // The command needs NV update.
    RETURN_IF_NV_IS_NOT_AVAILABLE;
    // Make sure that the authorization value is a reasonable size (not larger than
    // the size of the digest produced by the integrity hash. The integrity
    // hash is assumed to produce the longest digest of any hash implemented
    // on the TPM. This will also remove trailing zeros from the authValue.
    if(MemoryRemoveTrailingZeros(&in->newAuth) > CONTEXT_INTEGRITY_HASH_SIZE)
	return TPM_RCS_SIZE + RC_HierarchyChangeAuth_newAuth;
    // Set hierarchy authValue
    switch(in->authHandle)
	{
	  case TPM_RH_OWNER:
	    gp.ownerAuth = in->newAuth;
	    NV_SYNC_PERSISTENT(ownerAuth);
	    break;
	  case TPM_RH_ENDORSEMENT:
	    gp.endorsementAuth = in->newAuth;
	    NV_SYNC_PERSISTENT(endorsementAuth);
	    break;
	  case TPM_RH_PLATFORM:
	    gc.platformAuth = in->newAuth;
	    // orderly state should be cleared
	    g_clearOrderly = TRUE;
	    break;
	  case TPM_RH_LOCKOUT:
	    gp.lockoutAuth = in->newAuth;
	    NV_SYNC_PERSISTENT(lockoutAuth);
	    break;
	  default:
	    FAIL(FATAL_ERROR_INTERNAL);
	    break;
	}
    return TPM_RC_SUCCESS;
}
#endif // CC_HierarchyChangeAuth
