/********************************************************************************/
/*										*/
/*			  	Capability Commands   				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CapabilityCommands.c $	*/
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
#include "GetCapability_fp.h"
#if CC_GetCapability  // Conditional expansion of this file
TPM_RC
TPM2_GetCapability(
		   GetCapability_In    *in,            // IN: input parameter list
		   GetCapability_Out   *out            // OUT: output parameter list
		   )
{
    TPMU_CAPABILITIES   *data = &out->capabilityData.data;
    // Command Output
    // Set output capability type the same as input type
    out->capabilityData.capability = in->capability;
    switch(in->capability)
	{
	  case TPM_CAP_ALGS:
	    out->moreData = AlgorithmCapGetImplemented((TPM_ALG_ID)in->property,
						       in->propertyCount,
						       &data->algorithms);
	    break;
	  case TPM_CAP_HANDLES:
	    switch(HandleGetType((TPM_HANDLE)in->property))
		{
		  case TPM_HT_TRANSIENT:
		    // Get list of handles of loaded transient objects
		    out->moreData = ObjectCapGetLoaded((TPM_HANDLE)in->property,
						       in->propertyCount,
						       &data->handles);
		    break;
		  case TPM_HT_PERSISTENT:
		    // Get list of handles of persistent objects
		    out->moreData = NvCapGetPersistent((TPM_HANDLE)in->property,
						       in->propertyCount,
						       &data->handles);
		    break;
		  case TPM_HT_NV_INDEX:
		    // Get list of defined NV index
		    out->moreData = NvCapGetIndex((TPM_HANDLE)in->property,
						  in->propertyCount,
						  &data->handles);
		    break;
		  case TPM_HT_LOADED_SESSION:
		    // Get list of handles of loaded sessions
		    out->moreData = SessionCapGetLoaded((TPM_HANDLE)in->property,
							in->propertyCount,
							&data->handles);
		    break;
#ifdef TPM_HT_SAVED_SESSION
		  case TPM_HT_SAVED_SESSION:
#else
		  case TPM_HT_ACTIVE_SESSION:
#endif
	            // Get list of handles of
		    out->moreData = SessionCapGetSaved((TPM_HANDLE)in->property,
						       in->propertyCount,
						       &data->handles);
		    break;
		  case TPM_HT_PCR:
		    // Get list of handles of PCR
		    out->moreData = PCRCapGetHandles((TPM_HANDLE)in->property,
						     in->propertyCount,
						     &data->handles);
		    break;
		  case TPM_HT_PERMANENT:
		    // Get list of permanent handles
		    out->moreData = PermanentCapGetHandles((TPM_HANDLE)in->property,
							   in->propertyCount,
							   &data->handles);
		    break;
		  default:
		    // Unsupported input handle type
		    return TPM_RCS_HANDLE + RC_GetCapability_property;
		    break;
		}
	    break;
	  case TPM_CAP_COMMANDS:
	    out->moreData = CommandCapGetCCList((TPM_CC)in->property,
						in->propertyCount,
						&data->command);
	    break;
	  case TPM_CAP_PP_COMMANDS:
	    out->moreData = PhysicalPresenceCapGetCCList((TPM_CC)in->property,
							 in->propertyCount,
							 &data->ppCommands);
	    break;
	  case TPM_CAP_AUDIT_COMMANDS:
	    out->moreData = CommandAuditCapGetCCList((TPM_CC)in->property,
						     in->propertyCount,
						     &data->auditCommands);
	    break;
	  case TPM_CAP_PCRS:
	    // Input property must be 0
	    if(in->property != 0)
		return TPM_RCS_VALUE + RC_GetCapability_property;
	    out->moreData = PCRCapGetAllocation(in->propertyCount,
						&data->assignedPCR);
	    break;
	  case TPM_CAP_PCR_PROPERTIES:
	    out->moreData = PCRCapGetProperties((TPM_PT_PCR)in->property,
						in->propertyCount,
						&data->pcrProperties);
	    break;
	  case TPM_CAP_TPM_PROPERTIES:
	    out->moreData = TPMCapGetProperties((TPM_PT)in->property,
						in->propertyCount,
						&data->tpmProperties);
	    break;
#if ALG_ECC
	  case TPM_CAP_ECC_CURVES:
	    out->moreData = CryptCapGetECCCurve((TPM_ECC_CURVE)in->property,
						in->propertyCount,
						&data->eccCurves);
	    break;
#endif // TPM_ALG_ECC
	  case TPM_CAP_AUTH_POLICIES:
	    if(HandleGetType((TPM_HANDLE)in->property) != TPM_HT_PERMANENT)
		return TPM_RCS_VALUE + RC_GetCapability_property;
	    out->moreData = PermanentHandleGetPolicy((TPM_HANDLE)in->property,
						     in->propertyCount,
						     &data->authPolicies);
	    break;
	  case TPM_CAP_ACT:
	    if(((TPM_RH)in->property < TPM_RH_ACT_0)
	       || ((TPM_RH)in->property > TPM_RH_ACT_F))
		return TPM_RCS_VALUE + RC_GetCapability_property;
	    out->moreData = ActGetCapabilityData((TPM_HANDLE)in->property,
						 in->propertyCount,
						 &data->actData);
	    break;
	  case TPM_CAP_VENDOR_PROPERTY:
	    // vendor property is not implemented
	  default:
	    // Unsupported TPM_CAP value
	    return TPM_RCS_VALUE + RC_GetCapability_capability;
	    break;
	}
    return TPM_RC_SUCCESS;
}
#endif // CC_GetCapability
#include "Tpm.h"
#include "TestParms_fp.h"
#if CC_TestParms  // Conditional expansion of this file
TPM_RC
TPM2_TestParms(
	       TestParms_In    *in             // IN: input parameter list
	       )
{
    // Input parameter is not reference in command action
    NOT_REFERENCED(in);
    // The parameters are tested at unmarshal process.  We do nothing in command
    // action
    return TPM_RC_SUCCESS;
}
#endif // CC_TestParms
