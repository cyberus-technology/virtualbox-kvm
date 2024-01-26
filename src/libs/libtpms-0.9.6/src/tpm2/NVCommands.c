/********************************************************************************/
/*										*/
/*			    Non-Volatile Storage 				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: NVCommands.c $		*/
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
#include "NV_DefineSpace_fp.h"
#if CC_NV_DefineSpace  // Conditional expansion of this file
TPM_RC
TPM2_NV_DefineSpace(
		    NV_DefineSpace_In   *in             // IN: input parameter list
		    )
{
    TPMA_NV         attributes = in->publicInfo.nvPublic.attributes;
    UINT16          nameSize;
    nameSize = CryptHashGetDigestSize(in->publicInfo.nvPublic.nameAlg);
    // Input Validation
    // Checks not specific to type
    // If the UndefineSpaceSpecial command is not implemented, then can't have
    // an index that can only be deleted with policy
#if CC_NV_UndefineSpaceSpecial == NO
    if(IS_ATTRIBUTE(attributes, TPMA_NV, POLICY_DELETE))
	return TPM_RCS_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
#endif
    // check that the authPolicy consistent with hash algorithm
    if(in->publicInfo.nvPublic.authPolicy.t.size != 0
       && in->publicInfo.nvPublic.authPolicy.t.size != nameSize)
	return TPM_RCS_SIZE + RC_NV_DefineSpace_publicInfo;
    // make sure that the authValue is not too large
    if(MemoryRemoveTrailingZeros(&in->auth)
       > CryptHashGetDigestSize(in->publicInfo.nvPublic.nameAlg))
	return TPM_RCS_SIZE + RC_NV_DefineSpace_auth;
    // If an index is being created by the owner and shEnable is
    // clear, then we would not reach this point because ownerAuth
    // can't be given when shEnable is CLEAR. However, if phEnable
    // is SET but phEnableNV is CLEAR, we have to check here
    if(in->authHandle == TPM_RH_PLATFORM && gc.phEnableNV == CLEAR)
	return TPM_RCS_HIERARCHY + RC_NV_DefineSpace_authHandle;
    // Attribute checks
    // Eliminate the unsupported types
    switch(GET_TPM_NT(attributes))
	{
#if CC_NV_Increment == YES
	  case TPM_NT_COUNTER:
#endif
#if CC_NV_SetBits == YES
	  case TPM_NT_BITS:
#endif
#if CC_NV_Extend == YES
	  case TPM_NT_EXTEND:
#endif
#if CC_PolicySecret == YES && defined TPM_NT_PIN_PASS
	  case TPM_NT_PIN_PASS:
	  case TPM_NT_PIN_FAIL:
#endif
	  case TPM_NT_ORDINARY:
	    break;
	  default:
	    return TPM_RCS_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
	    break;
	}
    // Check that the sizes are OK based on the type
    switch(GET_TPM_NT(attributes))
	{
	  case TPM_NT_ORDINARY:
	    // Can't exceed the allowed size for the implementation
	    if(in->publicInfo.nvPublic.dataSize > MAX_NV_INDEX_SIZE)
		return TPM_RCS_SIZE + RC_NV_DefineSpace_publicInfo;
	    break;
	  case TPM_NT_EXTEND:
	    if(in->publicInfo.nvPublic.dataSize != nameSize)
		return TPM_RCS_SIZE + RC_NV_DefineSpace_publicInfo;
	    break;
	  default:
	    // Everything else needs a size of 8
	    if(in->publicInfo.nvPublic.dataSize != 8)
		return TPM_RCS_SIZE + RC_NV_DefineSpace_publicInfo;
	    break;
	}
    // Handle other specifics
    switch(GET_TPM_NT(attributes))
	{
	  case TPM_NT_COUNTER:
	    // Counter can't have TPMA_NV_CLEAR_STCLEAR SET (don't clear counters)
	    if(IS_ATTRIBUTE(attributes, TPMA_NV, CLEAR_STCLEAR))
		return TPM_RCS_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
	    break;
#ifdef TPM_NT_PIN_FAIL
	  case TPM_NT_PIN_FAIL:
	    // NV_NO_DA must be SET and AUTHWRITE must be CLEAR
	    // NOTE: As with a PIN_PASS index, the authValue of the index is not
	    // available until the index is written. If AUTHWRITE is the only way to
	    // write then index, it could never be written. Rather than go through
	    // all of the other possible ways to write the Index, it is simply
	    // prohibited to write the index with the authValue. Other checks
	    // below will insure that there seems to be a way to write the index
	    // (i.e., with platform authorization , owner authorization,
	    // or with policyAuth.)
	    // It is not allowed to create a PIN Index that can't be modified.
	    if(!IS_ATTRIBUTE(attributes, TPMA_NV, NO_DA))
		return TPM_RCS_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
            /* fall through */
#endif
#ifdef TPM_NT_PIN_PASS
	  case TPM_NT_PIN_PASS:
	    // AUTHWRITE must be CLEAR (see note above to TPM_NT_PIN_FAIL)
	    if(IS_ATTRIBUTE(attributes, TPMA_NV, AUTHWRITE)
	       || IS_ATTRIBUTE(attributes, TPMA_NV, GLOBALLOCK)
	       || IS_ATTRIBUTE(attributes, TPMA_NV, WRITEDEFINE))
		return TPM_RCS_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
#endif  // this comes before break because PIN_FAIL falls through
	    break;
	  default:
	    break;
	}
    // Locks may not be SET and written cannot be SET
    if(IS_ATTRIBUTE(attributes, TPMA_NV, WRITTEN)
       || IS_ATTRIBUTE(attributes, TPMA_NV, WRITELOCKED)
       || IS_ATTRIBUTE(attributes, TPMA_NV, READLOCKED))
	return TPM_RCS_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
    // There must be a way to read the index.
    if(!IS_ATTRIBUTE(attributes, TPMA_NV, OWNERREAD)
       && !IS_ATTRIBUTE(attributes, TPMA_NV, PPREAD)
       && !IS_ATTRIBUTE(attributes, TPMA_NV, AUTHREAD)
       && !IS_ATTRIBUTE(attributes, TPMA_NV, POLICYREAD))
	return TPM_RCS_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
    // There must be a way to write the index
    if(!IS_ATTRIBUTE(attributes, TPMA_NV, OWNERWRITE)
       && !IS_ATTRIBUTE(attributes, TPMA_NV, PPWRITE)
       && !IS_ATTRIBUTE(attributes, TPMA_NV, AUTHWRITE)
       && !IS_ATTRIBUTE(attributes, TPMA_NV, POLICYWRITE))
	return TPM_RCS_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
    // An index with TPMA_NV_CLEAR_STCLEAR can't have TPMA_NV_WRITEDEFINE SET
    if(IS_ATTRIBUTE(attributes, TPMA_NV, CLEAR_STCLEAR)
       &&  IS_ATTRIBUTE(attributes, TPMA_NV, WRITEDEFINE))
	return TPM_RCS_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
    // Make sure that the creator of the index can delete the index
    if((IS_ATTRIBUTE(attributes, TPMA_NV, PLATFORMCREATE)
	&& in->authHandle == TPM_RH_OWNER)
       || (!IS_ATTRIBUTE(attributes, TPMA_NV, PLATFORMCREATE)
	   && in->authHandle == TPM_RH_PLATFORM))
	return TPM_RCS_ATTRIBUTES + RC_NV_DefineSpace_authHandle;
    // If TPMA_NV_POLICY_DELETE is SET, then the index must be defined by
    // the platform
    if(IS_ATTRIBUTE(attributes, TPMA_NV, POLICY_DELETE)
       &&  TPM_RH_PLATFORM != in->authHandle)
	return TPM_RCS_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
    // Make sure that the TPMA_NV_WRITEALL is not set if the index size is larger
    // than the allowed NV buffer size.
    if(in->publicInfo.nvPublic.dataSize > MAX_NV_BUFFER_SIZE
       &&  IS_ATTRIBUTE(attributes, TPMA_NV, WRITEALL))
	return TPM_RCS_SIZE + RC_NV_DefineSpace_publicInfo;
    // And finally, see if the index is already defined.
    if(NvIndexIsDefined(in->publicInfo.nvPublic.nvIndex))
	return TPM_RC_NV_DEFINED;
    // Internal Data Update
    // define the space.  A TPM_RC_NV_SPACE error may be returned at this point
    return NvDefineIndex(&in->publicInfo.nvPublic, &in->auth);
}
#endif // CC_NV_DefineSpace
#include "Tpm.h"
#include "NV_UndefineSpace_fp.h"
#if CC_NV_UndefineSpace  // Conditional expansion of this file
TPM_RC
TPM2_NV_UndefineSpace(
		      NV_UndefineSpace_In     *in             // IN: input parameter list
		      )
{
    NV_REF           locator;
    NV_INDEX        *nvIndex = NvGetIndexInfo(in->nvIndex, &locator);
    // Input Validation
    // This command can't be used to delete an index with TPMA_NV_POLICY_DELETE SET
    if(IS_ATTRIBUTE(nvIndex->publicArea.attributes, TPMA_NV, POLICY_DELETE))
	return TPM_RCS_ATTRIBUTES + RC_NV_UndefineSpace_nvIndex;
    // The owner may only delete an index that was defined with ownerAuth. The
    // platform may delete an index that was created with either authorization.
    if(in->authHandle == TPM_RH_OWNER
       && IS_ATTRIBUTE(nvIndex->publicArea.attributes, TPMA_NV, PLATFORMCREATE))
	return TPM_RC_NV_AUTHORIZATION;
    // Internal Data Update
    // Call implementation dependent internal routine to delete NV index
    return NvDeleteIndex(nvIndex, locator);
}
#endif // CC_NV_UndefineSpace
#include "Tpm.h"
#include "NV_UndefineSpaceSpecial_fp.h"
#include "SessionProcess_fp.h"
#if CC_NV_UndefineSpaceSpecial  // Conditional expansion of this file
TPM_RC
TPM2_NV_UndefineSpaceSpecial(
			     NV_UndefineSpaceSpecial_In  *in             // IN: input parameter list
			     )
{
    TPM_RC           result;
    NV_REF           locator;
    NV_INDEX        *nvIndex = NvGetIndexInfo(in->nvIndex, &locator);
    // Input Validation
    // This operation only applies when the TPMA_NV_POLICY_DELETE attribute is SET
    if(!IS_ATTRIBUTE(nvIndex->publicArea.attributes, TPMA_NV, POLICY_DELETE))
	return TPM_RCS_ATTRIBUTES + RC_NV_UndefineSpaceSpecial_nvIndex;
    // Internal Data Update
    // Call implementation dependent internal routine to delete NV index
    result = NvDeleteIndex(nvIndex, locator);
    // If we just removed the index providing the authorization, make sure that the
    // authorization session computation is modified so that it doesn't try to
    // access the authValue of the just deleted index
    if(result == TPM_RC_SUCCESS)
	SessionRemoveAssociationToHandle(in->nvIndex);
    return result;
}
#endif // CC_NV_UndefineSpaceSpecial
#include "Tpm.h"
#include "NV_ReadPublic_fp.h"
#if CC_NV_ReadPublic  // Conditional expansion of this file
TPM_RC
TPM2_NV_ReadPublic(
		   NV_ReadPublic_In    *in,            // IN: input parameter list
		   NV_ReadPublic_Out   *out            // OUT: output parameter list
		   )
{
    NV_INDEX        *nvIndex = NvGetIndexInfo(in->nvIndex, NULL);
    // Command Output
    // Copy index public data to output
    out->nvPublic.nvPublic = nvIndex->publicArea;
    // Compute NV name
    NvGetIndexName(nvIndex, &out->nvName);
    return TPM_RC_SUCCESS;
}
#endif // CC_NV_ReadPublic
#include "Tpm.h"
#include "NV_Write_fp.h"
#if CC_NV_Write  // Conditional expansion of this file
TPM_RC
TPM2_NV_Write(
	      NV_Write_In     *in             // IN: input parameter list
	      )
{
    NV_INDEX        *nvIndex = NvGetIndexInfo(in->nvIndex, NULL);
    TPMA_NV          attributes = nvIndex->publicArea.attributes;
    TPM_RC           result;
    // Input Validation
    // Common access checks, NvWriteAccessCheck() may return TPM_RC_NV_AUTHORIZATION
    // or TPM_RC_NV_LOCKED
    result = NvWriteAccessChecks(in->authHandle,
				 in->nvIndex,
				 attributes);
    if(result != TPM_RC_SUCCESS)
	return result;
    // Bits index, extend index or counter index may not be updated by
    // TPM2_NV_Write
    if(IsNvCounterIndex(attributes)
       || IsNvBitsIndex(attributes)
       || IsNvExtendIndex(attributes))
	return TPM_RC_ATTRIBUTES;
    // Make sure that the offset is not too large
    if(in->offset > nvIndex->publicArea.dataSize)
	return TPM_RCS_VALUE + RC_NV_Write_offset;
    // Make sure that the selection is within the range of the Index
    if(in->data.t.size > (nvIndex->publicArea.dataSize - in->offset))
	return TPM_RC_NV_RANGE;
    // If this index requires a full sized write, make sure that input range is
    // full sized.
    // Note: if the requested size is the same as the Index data size, then offset
    // will have to be zero. Otherwise, the range check above would have failed.
    if(IS_ATTRIBUTE(attributes, TPMA_NV, WRITEALL)
       && in->data.t.size < nvIndex->publicArea.dataSize)
	return TPM_RC_NV_RANGE;
    // Internal Data Update
    // Perform the write.  This called routine will SET the TPMA_NV_WRITTEN
    // attribute if it has not already been SET. If NV isn't available, an error
    // will be returned.
    return NvWriteIndexData(nvIndex, in->offset, in->data.t.size,
			    in->data.t.buffer);
}
#endif // CC_NV_Write
#include "Tpm.h"
#include "NV_Increment_fp.h"
#if CC_NV_Increment  // Conditional expansion of this file
TPM_RC
TPM2_NV_Increment(
		  NV_Increment_In     *in             // IN: input parameter list
		  )
{
    TPM_RC           result;
    NV_REF           locator;
    NV_INDEX        *nvIndex = NvGetIndexInfo(in->nvIndex, &locator);
    UINT64           countValue;
    // Input Validation
    // Common access checks, NvWriteAccessCheck() may return TPM_RC_NV_AUTHORIZATION
    // or TPM_RC_NV_LOCKED
    result = NvWriteAccessChecks(in->authHandle,
				 in->nvIndex,
				 nvIndex->publicArea.attributes);
    if(result != TPM_RC_SUCCESS)
	return result;
    // Make sure that this is a counter
    if(!IsNvCounterIndex(nvIndex->publicArea.attributes))
	return TPM_RCS_ATTRIBUTES + RC_NV_Increment_nvIndex;
    // Internal Data Update
    // If counter index is not been written, initialize it
    if(!IS_ATTRIBUTE(nvIndex->publicArea.attributes, TPMA_NV, WRITTEN))
	countValue = NvReadMaxCount();
    else
	// Read NV data in native format for TPM CPU.
	countValue = NvGetUINT64Data(nvIndex, locator);
    // Do the increment
    countValue++;
    // Write NV data back. A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may
    // be returned at this point. If necessary, this function will set the
    // TPMA_NV_WRITTEN attribute
    result = NvWriteUINT64Data(nvIndex, countValue);
    if(result == TPM_RC_SUCCESS)
	{
	    // If a counter just rolled over, then force the NV update.
	    // Note, if this is an orderly counter, then the write-back needs to be
	    // forced, for other counters, the write-back will happen anyway
	    if(IS_ATTRIBUTE(nvIndex->publicArea.attributes, TPMA_NV, ORDERLY)
	       && (countValue & MAX_ORDERLY_COUNT) == 0 )
		{
		    // Need to force an NV update of orderly data
		    SET_NV_UPDATE(UT_ORDERLY);
		}
	}
    return result;
}
#endif // CC_NV_Increment
#include "Tpm.h"
#include "NV_Extend_fp.h"
#if CC_NV_Extend  // Conditional expansion of this file
TPM_RC
TPM2_NV_Extend(
	       NV_Extend_In    *in             // IN: input parameter list
	       )
{
    TPM_RC                   result;
    NV_REF                   locator;
    NV_INDEX                *nvIndex = NvGetIndexInfo(in->nvIndex, &locator);
    TPM2B_DIGEST            oldDigest;
    TPM2B_DIGEST            newDigest;
    HASH_STATE              hashState;
    // Input Validation
    // Common access checks, NvWriteAccessCheck() may return TPM_RC_NV_AUTHORIZATION
    // or TPM_RC_NV_LOCKED
    result = NvWriteAccessChecks(in->authHandle,
				 in->nvIndex,
				 nvIndex->publicArea.attributes);
    if(result != TPM_RC_SUCCESS)
	return result;
    // Make sure that this is an extend index
    if(!IsNvExtendIndex(nvIndex->publicArea.attributes))
	return TPM_RCS_ATTRIBUTES + RC_NV_Extend_nvIndex;
    // Internal Data Update
    // Perform the write.
    oldDigest.t.size = CryptHashGetDigestSize(nvIndex->publicArea.nameAlg);
    pAssert(oldDigest.t.size <= sizeof(oldDigest.t.buffer));
    if(IS_ATTRIBUTE(nvIndex->publicArea.attributes, TPMA_NV, WRITTEN))
	{
	    NvGetIndexData(nvIndex, locator, 0, oldDigest.t.size, oldDigest.t.buffer);
	}
    else
	{
	    MemorySet(oldDigest.t.buffer, 0, oldDigest.t.size);
	}
    // Start hash
    newDigest.t.size = CryptHashStart(&hashState, nvIndex->publicArea.nameAlg);
    // Adding old digest
    CryptDigestUpdate2B(&hashState, &oldDigest.b);
    // Adding new data
    CryptDigestUpdate2B(&hashState, &in->data.b);
    // Complete hash
    CryptHashEnd2B(&hashState, &newDigest.b);
    // Write extended hash back.
    // Note, this routine will SET the TPMA_NV_WRITTEN attribute if necessary
    return NvWriteIndexData(nvIndex, 0, newDigest.t.size, newDigest.t.buffer);
}
#endif // CC_NV_Extend
#include "Tpm.h"
#include "NV_SetBits_fp.h"
#if CC_NV_SetBits  // Conditional expansion of this file
TPM_RC
TPM2_NV_SetBits(
		NV_SetBits_In   *in             // IN: input parameter list
		)
{
    TPM_RC           result;
    NV_REF           locator;
    NV_INDEX        *nvIndex = NvGetIndexInfo(in->nvIndex, &locator);
    UINT64           oldValue;
    UINT64           newValue;
    // Input Validation
    // Common access checks, NvWriteAccessCheck() may return TPM_RC_NV_AUTHORIZATION
    // or TPM_RC_NV_LOCKED
    result = NvWriteAccessChecks(in->authHandle,
				 in->nvIndex,
				 nvIndex->publicArea.attributes);
    if(result != TPM_RC_SUCCESS)
	return result;
    // Make sure that this is a bit field
    if(!IsNvBitsIndex(nvIndex->publicArea.attributes))
	return TPM_RCS_ATTRIBUTES + RC_NV_SetBits_nvIndex;
    // If index is not been written, initialize it
    if(!IS_ATTRIBUTE(nvIndex->publicArea.attributes, TPMA_NV, WRITTEN))
	oldValue = 0;
    else
	// Read index data
	oldValue = NvGetUINT64Data(nvIndex, locator);
    // Figure out what the new value is going to be
    newValue = oldValue | in->bits;
    // Internal Data Update
    return  NvWriteUINT64Data(nvIndex, newValue);
}
#endif // CC_NV_SetBits
#include "Tpm.h"
#include "NV_WriteLock_fp.h"
#if CC_NV_WriteLock  // Conditional expansion of this file
TPM_RC
TPM2_NV_WriteLock(
		  NV_WriteLock_In     *in             // IN: input parameter list
		  )
{
    TPM_RC           result;
    NV_REF           locator;
    NV_INDEX        *nvIndex = NvGetIndexInfo(in->nvIndex, &locator);
    TPMA_NV          nvAttributes = nvIndex->publicArea.attributes;
    // Input Validation:
    // Common access checks, NvWriteAccessCheck() may return TPM_RC_NV_AUTHORIZATION
    // or TPM_RC_NV_LOCKED
    result = NvWriteAccessChecks(in->authHandle, in->nvIndex, nvAttributes);
    if(result != TPM_RC_SUCCESS)
	{
	    if(result == TPM_RC_NV_AUTHORIZATION)
		return result;
	    // If write access failed because the index is already locked, then it is
	    // no error.
	    return TPM_RC_SUCCESS;
	}
    // if neither TPMA_NV_WRITEDEFINE nor TPMA_NV_WRITE_STCLEAR is set, the index
    // can not be write-locked
    if(!IS_ATTRIBUTE(nvAttributes, TPMA_NV, WRITEDEFINE)
       && !IS_ATTRIBUTE(nvAttributes, TPMA_NV, WRITE_STCLEAR))
	return TPM_RCS_ATTRIBUTES + RC_NV_WriteLock_nvIndex;
    // Internal Data Update
    // Set the WRITELOCK attribute.
    // Note: if TPMA_NV_WRITELOCKED were already SET, then the write access check
    // above would have failed and this code isn't executed.
    SET_ATTRIBUTE(nvAttributes, TPMA_NV, WRITELOCKED);
    // Write index info back
    return NvWriteIndexAttributes(nvIndex->publicArea.nvIndex, locator,
				  nvAttributes);
}
#endif // CC_NV_WriteLock
#include "Tpm.h"
#include "NV_GlobalWriteLock_fp.h"
#if CC_NV_GlobalWriteLock  // Conditional expansion of this file
TPM_RC
TPM2_NV_GlobalWriteLock(
			NV_GlobalWriteLock_In   *in             // IN: input parameter list
			)
{
    // Input parameter (the authorization handle) is not reference in command action.
    NOT_REFERENCED(in);
    // Internal Data Update
    // Implementation dependent method of setting the global lock
    return NvSetGlobalLock();
}
#endif // CC_NV_GlobalWriteLock
#include "Tpm.h"
#include "NV_Read_fp.h"
#if CC_NV_Read  // Conditional expansion of this file
/* TPM_RC_NV_AUTHORIZATION the authorization was valid but the authorizing entity (authHandle) is
   not allowed to read from the Index referenced by nvIndex */
/* TPM_RC_NV_LOCKED the Index referenced by nvIndex is read locked */
/* TPM_RC_NV_RANGE read range defined by size and offset is outside the range of the Index
   referenced by nvIndex */
/* TPM_RC_NV_UNINITIALIZED the Index referenced by nvIndex has not been initialized (written) */
/* TPM_RC_VALUE the read size is larger than the MAX_NV_BUFFER_SIZE */
TPM_RC
TPM2_NV_Read(
	     NV_Read_In      *in,            // IN: input parameter list
	     NV_Read_Out     *out            // OUT: output parameter list
	     )
{
    NV_REF           locator;
    NV_INDEX        *nvIndex = NvGetIndexInfo(in->nvIndex, &locator);
    TPM_RC           result;
    // Input Validation
    // Common read access checks. NvReadAccessChecks() may return
    // TPM_RC_NV_AUTHORIZATION, TPM_RC_NV_LOCKED, or TPM_RC_NV_UNINITIALIZED
    result = NvReadAccessChecks(in->authHandle, in->nvIndex,
				nvIndex->publicArea.attributes);
    if(result != TPM_RC_SUCCESS)
	return result;
    // Make sure the data will fit the return buffer
    if(in->size > MAX_NV_BUFFER_SIZE)
	return TPM_RCS_VALUE + RC_NV_Read_size;
    // Verify that the offset is not too large
    if(in->offset > nvIndex->publicArea.dataSize)
	return TPM_RCS_VALUE + RC_NV_Read_offset;
    // Make sure that the selection is within the range of the Index
    if(in->size > (nvIndex->publicArea.dataSize - in->offset))
	return TPM_RC_NV_RANGE;
    // Command Output
    // Set the return size
    out->data.t.size = in->size;
    // Perform the read
    NvGetIndexData(nvIndex, locator, in->offset, in->size, out->data.t.buffer);
    return TPM_RC_SUCCESS;
}
#endif // CC_NV_Read
#include "Tpm.h"
#include "NV_ReadLock_fp.h"
#if CC_NV_ReadLock  // Conditional expansion of this file
TPM_RC
TPM2_NV_ReadLock(
		 NV_ReadLock_In  *in             // IN: input parameter list
		 )
{
    TPM_RC           result;
    NV_REF           locator;
    // The referenced index has been checked multiple times before this is called
    // so it must be present and will be loaded into cache
    NV_INDEX        *nvIndex = NvGetIndexInfo(in->nvIndex, &locator);
    TPMA_NV          nvAttributes = nvIndex->publicArea.attributes;
    // Input Validation
    // Common read access checks. NvReadAccessChecks() may return
    // TPM_RC_NV_AUTHORIZATION, TPM_RC_NV_LOCKED, or TPM_RC_NV_UNINITIALIZED
    result = NvReadAccessChecks(in->authHandle,
				in->nvIndex,
				nvAttributes);
    if(result == TPM_RC_NV_AUTHORIZATION)
	return TPM_RC_NV_AUTHORIZATION;
    // Index is already locked for write
    else if(result == TPM_RC_NV_LOCKED)
	return TPM_RC_SUCCESS;
    // If NvReadAccessChecks return TPM_RC_NV_UNINITALIZED, then continue.
    // It is not an error to read lock an uninitialized Index.
    // if TPMA_NV_READ_STCLEAR is not set, the index can not be read-locked
    if(!IS_ATTRIBUTE(nvAttributes, TPMA_NV, READ_STCLEAR))
	return TPM_RCS_ATTRIBUTES + RC_NV_ReadLock_nvIndex;
    // Internal Data Update
    // Set the READLOCK attribute
    SET_ATTRIBUTE(nvAttributes, TPMA_NV, READLOCKED);
    // Write NV info back
    return NvWriteIndexAttributes(nvIndex->publicArea.nvIndex,
				  locator,
				  nvAttributes);
}
#endif // CC_NV_ReadLock
#include "Tpm.h"
#include "NV_ChangeAuth_fp.h"
#if CC_NV_ChangeAuth  // Conditional expansion of this file
TPM_RC
TPM2_NV_ChangeAuth(
		   NV_ChangeAuth_In    *in             // IN: input parameter list
		   )
{
    NV_REF           locator;
    NV_INDEX        *nvIndex = NvGetIndexInfo(in->nvIndex, &locator);
    // Input Validation
    // Remove trailing zeros and make sure that the result is not larger than the
    // digest of the nameAlg.
    if(MemoryRemoveTrailingZeros(&in->newAuth)
       > CryptHashGetDigestSize(nvIndex->publicArea.nameAlg))
	return TPM_RCS_SIZE + RC_NV_ChangeAuth_newAuth;
    // Internal Data Update
    // Change authValue
    return NvWriteIndexAuth(locator, &in->newAuth);
}
#endif // CC_NV_ChangeAuth
#include "Tpm.h"
#include "Attest_spt_fp.h"
#include "NV_Certify_fp.h"
#if CC_NV_Certify  // Conditional expansion of this file
TPM_RC
TPM2_NV_Certify(
		NV_Certify_In   *in,            // IN: input parameter list
		NV_Certify_Out  *out            // OUT: output parameter list
		)
{
    TPM_RC                  result;
    NV_REF                   locator;
    NV_INDEX                *nvIndex = NvGetIndexInfo(in->nvIndex, &locator);
    TPMS_ATTEST              certifyInfo;
    OBJECT                 *signObject = HandleToObject(in->signHandle);
    // Input Validation
    if(!IsSigningObject(signObject))
	return TPM_RCS_KEY + RC_NV_Certify_signHandle;
    if(!CryptSelectSignScheme(signObject, &in->inScheme))
	return TPM_RCS_SCHEME + RC_NV_Certify_inScheme;
    // Common access checks, NvWriteAccessCheck() may return TPM_RC_NV_AUTHORIZATION
    // or TPM_RC_NV_LOCKED
    result = NvReadAccessChecks(in->authHandle, in->nvIndex,
				nvIndex->publicArea.attributes);
    if(result != TPM_RC_SUCCESS)
	return result;
    // make sure that the selection is within the range of the Index (cast to avoid
    // any wrap issues with addition)
    if((UINT32)in->size + (UINT32)in->offset > (UINT32)nvIndex->publicArea.dataSize)
	return TPM_RC_NV_RANGE;
    // Make sure the data will fit the return buffer
    // NOTE: This check may be modified if the output buffer will not hold the
    // maximum sized NV buffer as part of the certified data. The difference in
    // size could be substantial if the signature scheme was produced a large
    // signature (e.g., RSA 4096).
    if(in->size > MAX_NV_BUFFER_SIZE)
	return TPM_RCS_VALUE + RC_NV_Certify_size;
    // Command Output
 
    // Fill in attest information common fields
    FillInAttestInfo(in->signHandle, &in->inScheme, &in->qualifyingData,
		     &certifyInfo);
    
    // Get the name of the index
    NvGetIndexName(nvIndex, &certifyInfo.attested.nv.indexName);
    
    // See if this is old format or new format
    if ((in->size != 0) || (in->offset != 0))
	{
	    // NV certify specific fields
	    // Attestation type
	    certifyInfo.type = TPM_ST_ATTEST_NV;
	    
	    // Set the return size
	    certifyInfo.attested.nv.nvContents.t.size = in->size;
	    
	    // Set the offset
	    certifyInfo.attested.nv.offset = in->offset;
	    
	    // Perform the read
	    NvGetIndexData(nvIndex, locator, in->offset, in->size,
			   certifyInfo.attested.nv.nvContents.t.buffer);
	}
    else
	{
	    HASH_STATE                  hashState;
	    // This is to sign a digest of the data
	    certifyInfo.type = TPM_ST_ATTEST_NV_DIGEST;
	    // Initialize the hash before calling the function to add the Index data to
	    // the hash.
	    certifyInfo.attested.nvDigest.nvDigest.t.size =
		CryptHashStart(&hashState, in->inScheme.details.any.hashAlg);
	    NvHashIndexData(&hashState, nvIndex, locator, 0,
			    nvIndex->publicArea.dataSize);
	    CryptHashEnd2B(&hashState, &certifyInfo.attested.nvDigest.nvDigest.b);
	}
    // Sign attestation structure.  A NULL signature will be returned if
    // signObject is NULL.
    return SignAttestInfo(signObject, &in->inScheme, &certifyInfo,
			  &in->qualifyingData, &out->certifyInfo, &out->signature);
}
#endif // CC_NV_Certify
