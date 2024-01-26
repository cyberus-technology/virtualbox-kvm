/********************************************************************************/
/*										*/
/*			Managing and accessing the hierarchy-related values   	*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Hierarchy.c $		*/
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
/* 8.3 Hierarchy.c */
/* 8.3.1 Introduction */
/* This file contains the functions used for managing and accessing the hierarchy-related values. */
/* 8.3.2 Includes */
#include "Tpm.h"
/* 8.3.3 Functions */
/* 8.3.3.1 HierarchyPreInstall() */
/* This function performs the initialization functions for the hierarchy when the TPM is
   simulated. This function should not be called if the TPM is not in a manufacturing mode at the
   manufacturer, or in a simulated environment. */
void
HierarchyPreInstall_Init(
			 void
			 )
{
    // Allow lockout clear command
    gp.disableClear = FALSE;
    // Initialize Primary Seeds
    gp.EPSeed.t.size = sizeof(gp.EPSeed.t.buffer);
    gp.SPSeed.t.size = sizeof(gp.SPSeed.t.buffer);
    gp.PPSeed.t.size = sizeof(gp.PPSeed.t.buffer);
#if (defined USE_PLATFORM_EPS) && (USE_PLATFORM_EPS != NO)
    _plat__GetEPS(gp.EPSeed.t.size, gp.EPSeed.t.buffer);
#else
    CryptRandomGenerate(gp.EPSeed.t.size, gp.EPSeed.t.buffer);
#endif
    CryptRandomGenerate(gp.SPSeed.t.size, gp.SPSeed.t.buffer);
    CryptRandomGenerate(gp.PPSeed.t.size, gp.PPSeed.t.buffer);
    gp.EPSeedCompatLevel = SEED_COMPAT_LEVEL_LAST;   // libtpms added begin
    gp.SPSeedCompatLevel = SEED_COMPAT_LEVEL_LAST;
    gp.PPSeedCompatLevel = SEED_COMPAT_LEVEL_LAST;   // libtpms added end
    // Initialize owner, endorsement and lockout authorization
    gp.ownerAuth.t.size = 0;
    gp.endorsementAuth.t.size = 0;
    gp.lockoutAuth.t.size = 0;
    // Initialize owner, endorsement, and lockout policy
    gp.ownerAlg = TPM_ALG_NULL;
    gp.ownerPolicy.t.size = 0;
    gp.endorsementAlg = TPM_ALG_NULL;
    gp.endorsementPolicy.t.size = 0;
    gp.lockoutAlg = TPM_ALG_NULL;
    gp.lockoutPolicy.t.size = 0;
    // Initialize ehProof, shProof and phProof
    gp.phProof.t.size = sizeof(gp.phProof.t.buffer);
    gp.shProof.t.size = sizeof(gp.shProof.t.buffer);
    gp.ehProof.t.size = sizeof(gp.ehProof.t.buffer);
    CryptRandomGenerate(gp.phProof.t.size, gp.phProof.t.buffer);
    CryptRandomGenerate(gp.shProof.t.size, gp.shProof.t.buffer);
    CryptRandomGenerate(gp.ehProof.t.size, gp.ehProof.t.buffer);
    // Write hierarchy data to NV
    NV_SYNC_PERSISTENT(disableClear);
    NV_SYNC_PERSISTENT(EPSeed);
    NV_SYNC_PERSISTENT(SPSeed);
    NV_SYNC_PERSISTENT(PPSeed);
    NV_SYNC_PERSISTENT(EPSeedCompatLevel);  // libtpms added begin
    NV_SYNC_PERSISTENT(SPSeedCompatLevel);
    NV_SYNC_PERSISTENT(PPSeedCompatLevel);  // libtpms added end
    NV_SYNC_PERSISTENT(ownerAuth);
    NV_SYNC_PERSISTENT(endorsementAuth);
    NV_SYNC_PERSISTENT(lockoutAuth);
    NV_SYNC_PERSISTENT(ownerAlg);
    NV_SYNC_PERSISTENT(ownerPolicy);
    NV_SYNC_PERSISTENT(endorsementAlg);
    NV_SYNC_PERSISTENT(endorsementPolicy);
    NV_SYNC_PERSISTENT(lockoutAlg);
    NV_SYNC_PERSISTENT(lockoutPolicy);
    NV_SYNC_PERSISTENT(phProof);
    NV_SYNC_PERSISTENT(shProof);
    NV_SYNC_PERSISTENT(ehProof);
    return;
}
/* 8.3.3.2 HierarchyStartup() */
/* This function is called at TPM2_Startup() to initialize the hierarchy related values. */
BOOL
HierarchyStartup(
		 STARTUP_TYPE     type           // IN: start up type
		 )
{
    // phEnable is SET on any startup
    g_phEnable = TRUE;
    // Reset platformAuth, platformPolicy; enable SH and EH at TPM_RESET and
    // TPM_RESTART
    if(type != SU_RESUME)
	{
	    gc.platformAuth.t.size = 0;
	    gc.platformPolicy.t.size = 0;
	    gc.platformAlg = TPM_ALG_NULL;
	    // enable the storage and endorsement hierarchies and the platformNV
	    gc.shEnable = gc.ehEnable = gc.phEnableNV = TRUE;
	}
    // nullProof and nullSeed are updated at every TPM_RESET
    if((type != SU_RESTART) && (type != SU_RESUME))
	{
	    gr.nullProof.t.size = sizeof(gr.nullProof.t.buffer);
	    CryptRandomGenerate(gr.nullProof.t.size, gr.nullProof.t.buffer);
	    gr.nullSeed.t.size = sizeof(gr.nullSeed.t.buffer);
	    CryptRandomGenerate(gr.nullSeed.t.size, gr.nullSeed.t.buffer);
	    gr.nullSeedCompatLevel = SEED_COMPAT_LEVEL_LAST;  // libtpms added
	}
    return TRUE;
}
/* 8.3.3.3 HierarchyGetProof() */
/* This function finds the proof value associated with a hierarchy.It returns a pointer to the proof
   value. */
TPM2B_PROOF *
HierarchyGetProof(
		  TPMI_RH_HIERARCHY    hierarchy      // IN: hierarchy constant
		  )
{
    TPM2B_PROOF         *proof = NULL;
    switch(hierarchy)
	{
	  case TPM_RH_PLATFORM:
	    // phProof for TPM_RH_PLATFORM
	    proof = &gp.phProof;
	    break;
	  case TPM_RH_ENDORSEMENT:
	    // ehProof for TPM_RH_ENDORSEMENT
	    proof = &gp.ehProof;
	    break;
	  case TPM_RH_OWNER:
	    // shProof for TPM_RH_OWNER
	    proof = &gp.shProof;
	    break;
	  default:
	    // nullProof for TPM_RH_NULL or anything else
	    proof = &gr.nullProof;
	    break;
	}
    return proof;
}
/* 8.3.3.4 HierarchyGetPrimarySeed() */
/* This function returns the primary seed of a hierarchy. */
TPM2B_SEED *
HierarchyGetPrimarySeed(
			TPMI_RH_HIERARCHY    hierarchy      // IN: hierarchy
			)
{
    TPM2B_SEED          *seed = NULL;
    switch(hierarchy)
	{
	  case TPM_RH_PLATFORM:
	    seed = &gp.PPSeed;
	    break;
	  case TPM_RH_OWNER:
	    seed = &gp.SPSeed;
	    break;
	  case TPM_RH_ENDORSEMENT:
	    seed = &gp.EPSeed;
	    break;
	  default:
	    seed = &gr.nullSeed;
	    break;
	}
    return seed;
}
// libtpms added begin
SEED_COMPAT_LEVEL
HierarchyGetPrimarySeedCompatLevel(
				   TPMI_RH_HIERARCHY    hierarchy     // IN: hierarchy
			           )
{
    switch(hierarchy)
	{
	  case TPM_RH_PLATFORM:
	    return gp.PPSeedCompatLevel;
	    break;
	  case TPM_RH_OWNER:
	    return gp.SPSeedCompatLevel;
	    break;
	  case TPM_RH_ENDORSEMENT:
	    return gp.EPSeedCompatLevel;
	    break;
	  case TPM_RH_NULL:
	    return gr.nullSeedCompatLevel;
	  default:
	    FAIL(FATAL_ERROR_INTERNAL);
	    break;
	}
}
// libtpms added end
/* 8.3.3.5 HierarchyIsEnabled() */
/* This function checks to see if a hierarchy is enabled. */
/* NOTE: The TPM_RH_NULL hierarchy is always enabled. */
/* Return Values Meaning */
/* TRUE hierarchy is enabled */
/* FALSE hierarchy is disabled */
BOOL
HierarchyIsEnabled(
		   TPMI_RH_HIERARCHY    hierarchy      // IN: hierarchy
		   )
{
    BOOL            enabled = FALSE;
    switch(hierarchy)
	{
	  case TPM_RH_PLATFORM:
	    enabled = g_phEnable;
	    break;
	  case TPM_RH_OWNER:
	    enabled = gc.shEnable;
	    break;
	  case TPM_RH_ENDORSEMENT:
	    enabled = gc.ehEnable;
	    break;
	  case TPM_RH_NULL:
	    enabled = TRUE;
	    break;
	  default:
	    enabled = FALSE;
	    break;
	}
    return enabled;
}
