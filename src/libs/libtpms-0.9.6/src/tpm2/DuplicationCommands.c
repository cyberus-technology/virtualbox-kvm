/********************************************************************************/
/*										*/
/*			    Duplication Commands 				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: DuplicationCommands.c $	*/
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
#include "Duplicate_fp.h"
#if CC_Duplicate  // Conditional expansion of this file
#include "Object_spt_fp.h"
TPM_RC
TPM2_Duplicate(
	       Duplicate_In    *in,            // IN: input parameter list
	       Duplicate_Out   *out            // OUT: output parameter list
	       )
{
    TPM_RC                  result = TPM_RC_SUCCESS;
    TPMT_SENSITIVE          sensitive;
    UINT16                  innerKeySize = 0; // encrypt key size for inner wrap
    OBJECT                  *object;
    OBJECT                  *newParent;
    TPM2B_DATA              data;
    // Input Validation
    // Get duplicate object pointer
    object = HandleToObject(in->objectHandle);
    // Get new parent
    newParent = HandleToObject(in->newParentHandle);
    // duplicate key must have fixParent bit CLEAR.
    if(IS_ATTRIBUTE(object->publicArea.objectAttributes, TPMA_OBJECT, fixedParent))
	return TPM_RCS_ATTRIBUTES + RC_Duplicate_objectHandle;
    // Do not duplicate object with NULL nameAlg
    if(object->publicArea.nameAlg == TPM_ALG_NULL)
	return TPM_RCS_TYPE + RC_Duplicate_objectHandle;
    // new parent key must be a storage object or TPM_RH_NULL
    if(in->newParentHandle != TPM_RH_NULL
       && !ObjectIsStorage(in->newParentHandle))
	return TPM_RCS_TYPE + RC_Duplicate_newParentHandle;
    // If the duplicated object has encryptedDuplication SET, then there must be
    // an inner wrapper and the new parent may not be TPM_RH_NULL
    if(IS_ATTRIBUTE(object->publicArea.objectAttributes, TPMA_OBJECT,
		    encryptedDuplication))
	{
	    if(in->symmetricAlg.algorithm == TPM_ALG_NULL)
		return TPM_RCS_SYMMETRIC + RC_Duplicate_symmetricAlg;
	    if(in->newParentHandle == TPM_RH_NULL)
		return TPM_RCS_HIERARCHY + RC_Duplicate_newParentHandle;
	}
    if(in->symmetricAlg.algorithm == TPM_ALG_NULL)
	{
	    // if algorithm is TPM_ALG_NULL, input key size must be 0
	    if(in->encryptionKeyIn.t.size != 0)
		return TPM_RCS_SIZE + RC_Duplicate_encryptionKeyIn;
	}
    else
	{
	    // Get inner wrap key size
	    innerKeySize = in->symmetricAlg.keyBits.sym;
	    // If provided the input symmetric key must match the size of the algorithm
	    if(in->encryptionKeyIn.t.size != 0
	       && in->encryptionKeyIn.t.size != (innerKeySize + 7) / 8)
		return TPM_RCS_SIZE + RC_Duplicate_encryptionKeyIn;
	}
    // Command Output
    if(in->newParentHandle != TPM_RH_NULL)
	{
	    // Make encrypt key and its associated secret structure.  A TPM_RC_KEY
	    // error may be returned at this point
	    out->outSymSeed.t.size = sizeof(out->outSymSeed.t.secret);
	    result = CryptSecretEncrypt(newParent, DUPLICATE_STRING, &data,
					&out->outSymSeed);
	    if(result != TPM_RC_SUCCESS)
		return result;
	}
    else
	{
	    // Do not apply outer wrapper
	    data.t.size = 0;
	    out->outSymSeed.t.size = 0;
	}
    // Copy sensitive area
    sensitive = object->sensitive;
    // Prepare output private data from sensitive.
    // Note: If there is no encryption key, one will be provided by
    // SensitiveToDuplicate(). This is why the assignment of encryptionKeyIn to
    // encryptionKeyOut will work properly and is not conditional.
    SensitiveToDuplicate(&sensitive, &object->name.b, newParent,
			 object->publicArea.nameAlg, &data.b,
			 &in->symmetricAlg, &in->encryptionKeyIn,
			 &out->duplicate);
    out->encryptionKeyOut = in->encryptionKeyIn;
    return TPM_RC_SUCCESS;
}
#endif // CC_Duplicate
#include "Tpm.h"
#include "Rewrap_fp.h"
#if CC_Rewrap  // Conditional expansion of this file
#include "Object_spt_fp.h"
TPM_RC
TPM2_Rewrap(
	    Rewrap_In       *in,            // IN: input parameter list
	    Rewrap_Out      *out            // OUT: output parameter list
	    )
{
    TPM_RC                  result = TPM_RC_SUCCESS;
    TPM2B_DATA              data;               // symmetric key
    UINT16                  hashSize = 0;
    TPM2B_PRIVATE           privateBlob;        // A temporary private blob
    // to transit between old
    // and new wrappers
    // Input Validation
    if((in->inSymSeed.t.size == 0 && in->oldParent != TPM_RH_NULL)
       || (in->inSymSeed.t.size != 0 && in->oldParent == TPM_RH_NULL))
	return TPM_RCS_HANDLE + RC_Rewrap_oldParent;
    if(in->oldParent != TPM_RH_NULL)
	{
	    OBJECT              *oldParent = HandleToObject(in->oldParent);
	    // old parent key must be a storage object
	    if(!ObjectIsStorage(in->oldParent))
		return TPM_RCS_TYPE + RC_Rewrap_oldParent;
	    // Decrypt input secret data via asymmetric decryption.  A
	    // TPM_RC_VALUE, TPM_RC_KEY or unmarshal errors may be returned at this
	    // point
	    result = CryptSecretDecrypt(oldParent, NULL, DUPLICATE_STRING,
					&in->inSymSeed, &data);
	    if(result != TPM_RC_SUCCESS)
		return TPM_RCS_VALUE + RC_Rewrap_inSymSeed;
	    // Unwrap Outer
	    result = UnwrapOuter(oldParent, &in->name.b,
				 oldParent->publicArea.nameAlg, &data.b,
				 FALSE,
				 in->inDuplicate.t.size, in->inDuplicate.t.buffer);
	    if(result != TPM_RC_SUCCESS)
		return RcSafeAddToResult(result, RC_Rewrap_inDuplicate);
	    // Copy unwrapped data to temporary variable, remove the integrity field
	    hashSize = sizeof(UINT16) +
		       CryptHashGetDigestSize(oldParent->publicArea.nameAlg);
	    privateBlob.t.size = in->inDuplicate.t.size - hashSize;
	    pAssert(privateBlob.t.size <= sizeof(privateBlob.t.buffer));
	    MemoryCopy(privateBlob.t.buffer, in->inDuplicate.t.buffer + hashSize,
		       privateBlob.t.size);
	}
    else
	{
	    // No outer wrap from input blob.  Direct copy.
	    privateBlob = in->inDuplicate;
	}
    if(in->newParent != TPM_RH_NULL)
	{
	    OBJECT          *newParent;
	    newParent = HandleToObject(in->newParent);
	    // New parent must be a storage object
	    if(!ObjectIsStorage(in->newParent))
		return TPM_RCS_TYPE + RC_Rewrap_newParent;
	    // Make new encrypt key and its associated secret structure.  A
	    // TPM_RC_VALUE error may be returned at this point if RSA algorithm is
	    // enabled in TPM
	    out->outSymSeed.t.size = sizeof(out->outSymSeed.t.secret);
	    result = CryptSecretEncrypt(newParent, DUPLICATE_STRING, &data,
					&out->outSymSeed);
	    if(result != TPM_RC_SUCCESS)
		return result;
	    // Copy temporary variable to output, reserve the space for integrity
	    hashSize = sizeof(UINT16) +
		       CryptHashGetDigestSize(newParent->publicArea.nameAlg);
	    // Make sure that everything fits into the output buffer
	    // Note: this is mostly only an issue if there was no outer wrapper on
	    // 'inDuplicate'. It could be as large as a TPM2B_PRIVATE buffer. If we add
	    // a digest for an outer wrapper, it won't fit anymore.
	    if((privateBlob.t.size + hashSize) > sizeof(out->outDuplicate.t.buffer))
		return TPM_RCS_VALUE + RC_Rewrap_inDuplicate;
	    // Command output
	    out->outDuplicate.t.size = privateBlob.t.size;
	    pAssert(privateBlob.t.size
		    <= sizeof(out->outDuplicate.t.buffer) - hashSize);
	    MemoryCopy(out->outDuplicate.t.buffer + hashSize, privateBlob.t.buffer,
		       privateBlob.t.size);
	    // Produce outer wrapper for output
	    out->outDuplicate.t.size = ProduceOuterWrap(newParent, &in->name.b,
							newParent->publicArea.nameAlg,
							&data.b,
							FALSE,
							out->outDuplicate.t.size,
							out->outDuplicate.t.buffer);
	}
    else  // New parent is a null key so there is no seed
	{
	    out->outSymSeed.t.size = 0;
	    // Copy privateBlob directly
	    out->outDuplicate = privateBlob;
	}
    return TPM_RC_SUCCESS;
}
#endif // CC_Rewrap
#include "Tpm.h"
#include "Import_fp.h"
#if CC_Import  // Conditional expansion of this file
#include "Object_spt_fp.h"
TPM_RC
TPM2_Import(
	    Import_In       *in,            // IN: input parameter list
	    Import_Out      *out            // OUT: output parameter list
	    )
{
    TPM_RC                   result = TPM_RC_SUCCESS;
    OBJECT                  *parentObject;
    TPM2B_DATA               data;                   // symmetric key
    TPMT_SENSITIVE           sensitive;
    TPM2B_NAME               name;
    TPMA_OBJECT              attributes;
    UINT16                   innerKeySize = 0;       // encrypt key size for inner
    // wrapper
    // Input Validation
    // to save typing
    attributes = in->objectPublic.publicArea.objectAttributes;
    // FixedTPM and fixedParent must be CLEAR
    if(IS_ATTRIBUTE(attributes, TPMA_OBJECT, fixedTPM)
       || IS_ATTRIBUTE(attributes, TPMA_OBJECT, fixedParent))
	return TPM_RCS_ATTRIBUTES + RC_Import_objectPublic;
    // Get parent pointer
    parentObject = HandleToObject(in->parentHandle);
    if(!ObjectIsParent(parentObject))
	return TPM_RCS_TYPE + RC_Import_parentHandle;
    if(in->symmetricAlg.algorithm != TPM_ALG_NULL)
	{
	    // Get inner wrap key size
	    innerKeySize = in->symmetricAlg.keyBits.sym;
	    // Input symmetric key must match the size of algorithm.
	    if(in->encryptionKey.t.size != (innerKeySize + 7) / 8)
		return TPM_RCS_SIZE + RC_Import_encryptionKey;
	}
    else
	{
	    // If input symmetric algorithm is NULL, input symmetric key size must
	    // be 0 as well
	    if(in->encryptionKey.t.size != 0)
		return TPM_RCS_SIZE + RC_Import_encryptionKey;
	    // If encryptedDuplication is SET, then the object must have an inner
	    // wrapper
	    if(IS_ATTRIBUTE(attributes, TPMA_OBJECT, encryptedDuplication))
		return TPM_RCS_ATTRIBUTES + RC_Import_encryptionKey;
	}
    // See if there is an outer wrapper
    if(in->inSymSeed.t.size != 0)
	{
	    // in->inParentHandle is a parent, but in order to decrypt an outer wrapper,
	    // it must be able to do key exchange and a symmetric key can't do that.
	    if(parentObject->publicArea.type == TPM_ALG_SYMCIPHER)
		return TPM_RCS_TYPE + RC_Import_parentHandle;
	    // Decrypt input secret data via asymmetric decryption. TPM_RC_ATTRIBUTES,
	    // TPM_RC_ECC_POINT, TPM_RC_INSUFFICIENT, TPM_RC_KEY, TPM_RC_NO_RESULT,
	    // TPM_RC_SIZE, TPM_RC_VALUE may be returned at this point
	    result = CryptSecretDecrypt(parentObject, NULL, DUPLICATE_STRING,
					&in->inSymSeed, &data);
	    pAssert(result != TPM_RC_BINDING);
	    if(result != TPM_RC_SUCCESS)
		return RcSafeAddToResult(result, RC_Import_inSymSeed);
	}
    else
	{
	    // If encrytpedDuplication is set, then the object must have an outer
	    // wrapper
	    if(IS_ATTRIBUTE(attributes, TPMA_OBJECT, encryptedDuplication))
		return TPM_RCS_ATTRIBUTES + RC_Import_inSymSeed;
	    data.t.size = 0;
	}
    // Compute name of object
    PublicMarshalAndComputeName(&(in->objectPublic.publicArea), &name);
    if(name.t.size == 0)
	return TPM_RCS_HASH + RC_Import_objectPublic;
    // Retrieve sensitive from private.
    // TPM_RC_INSUFFICIENT, TPM_RC_INTEGRITY, TPM_RC_SIZE may be returned here.
    result = DuplicateToSensitive(&in->duplicate.b, &name.b, parentObject,
				  in->objectPublic.publicArea.nameAlg,
				  &data.b, &in->symmetricAlg,
				  &in->encryptionKey.b, &sensitive);
    if(result != TPM_RC_SUCCESS)
	return RcSafeAddToResult(result, RC_Import_duplicate);
    // If the parent of this object has fixedTPM SET, then validate this
    // object as if it were being loaded so that validation can be skipped
    // when it is actually loaded.
    if(IS_ATTRIBUTE(parentObject->publicArea.objectAttributes, TPMA_OBJECT, fixedTPM))
	{
	    result = ObjectLoad(NULL, NULL, &in->objectPublic.publicArea,
				&sensitive, RC_Import_objectPublic, RC_Import_duplicate,
				NULL);
	}
    // Command output
    if(result == TPM_RC_SUCCESS)
	{
	    // Prepare output private data from sensitive
	    SensitiveToPrivate(&sensitive, &name, parentObject,
			       in->objectPublic.publicArea.nameAlg,
			       &out->outPrivate);
	}
    return result;
}
#endif // CC_Import
