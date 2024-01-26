/********************************************************************************/
/*										*/
/*			     Object Commands					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: ObjectCommands.c $	*/
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

#include "Tpm.h"
#include "Object_spt_fp.h"
#include "Create_fp.h"
#if CC_Create  // Conditional expansion of this file
TPM_RC
TPM2_Create(
	    Create_In       *in,            // IN: input parameter list
	    Create_Out      *out            // OUT: output parameter list
	    )
{
    TPM_RC                   result = TPM_RC_SUCCESS;
    OBJECT                  *parentObject;
    OBJECT                  *newObject;
    TPMT_PUBLIC             *publicArea;
    // Input Validation
    parentObject = HandleToObject(in->parentHandle);
    pAssert(parentObject != NULL);
    // Does parent have the proper attributes?
    if(!ObjectIsParent(parentObject))
	return TPM_RCS_TYPE + RC_Create_parentHandle;
    // Get a slot for the creation
    newObject = FindEmptyObjectSlot(NULL);
    if(newObject == NULL)
	return TPM_RC_OBJECT_MEMORY;
    // If the TPM2B_PUBLIC was passed as a structure, marshal it into is canonical
    // form for processing
    // to save typing.
    publicArea = &newObject->publicArea;
    // Copy the input structure to the allocated structure
    *publicArea = in->inPublic.publicArea;
    // Check attributes in input public area. CreateChecks() checks the things that
    // are unique to creation and then validates the attributes and values that are
    // common to create and load.
    result = CreateChecks(parentObject, publicArea,
			  in->inSensitive.sensitive.data.t.size);
    if(result != TPM_RC_SUCCESS)
	return RcSafeAddToResult(result, RC_Create_inPublic);
    // Clean up the authValue if necessary
    if(!AdjustAuthSize(&in->inSensitive.sensitive.userAuth, publicArea->nameAlg))
	return TPM_RCS_SIZE + RC_Create_inSensitive;
    // Command Output
    // Create the object using the default TPM random-number generator
    result = CryptCreateObject(newObject, &in->inSensitive.sensitive, NULL);
    if(result != TPM_RC_SUCCESS)
	return result;
    // Fill in creation data
    FillInCreationData(in->parentHandle, publicArea->nameAlg,
		       &in->creationPCR, &in->outsideInfo,
		       &out->creationData, &out->creationHash);
    // Compute creation ticket
    TicketComputeCreation(EntityGetHierarchy(in->parentHandle), &newObject->name,
			  &out->creationHash, &out->creationTicket);
    // Prepare output private data from sensitive
    SensitiveToPrivate(&newObject->sensitive, &newObject->name, parentObject,
		       publicArea->nameAlg,
		       &out->outPrivate);
    // Finish by copying the remaining return values
    out->outPublic.publicArea = newObject->publicArea;
    return TPM_RC_SUCCESS;
}
#endif // CC_Create
#include "Tpm.h"
#include "Load_fp.h"
#if CC_Load  // Conditional expansion of this file
#include "Object_spt_fp.h"
TPM_RC
TPM2_Load(
	  Load_In         *in,            // IN: input parameter list
	  Load_Out        *out            // OUT: output parameter list
	  )
{
    TPM_RC                   result = TPM_RC_SUCCESS;
    TPMT_SENSITIVE           sensitive = {0}; // libtpms changed (valgrind)
    OBJECT                  *parentObject;
    OBJECT                  *newObject;
    // Input Validation
    // Don't get invested in loading if there is no place to put it.
    newObject = FindEmptyObjectSlot(&out->objectHandle);
    if(newObject == NULL)
	return TPM_RC_OBJECT_MEMORY;
    if(in->inPrivate.t.size == 0)
	return TPM_RCS_SIZE + RC_Load_inPrivate;
    parentObject = HandleToObject(in->parentHandle);
    pAssert(parentObject != NULL);
    // Is the object that is being used as the parent actually a parent.
    if(!ObjectIsParent(parentObject))
	return TPM_RCS_TYPE + RC_Load_parentHandle;
    // Compute the name of object. If there isn't one, it is because the nameAlg is
    // not valid.
    PublicMarshalAndComputeName(&in->inPublic.publicArea, &out->name);
    if(out->name.t.size == 0)
	return TPM_RCS_HASH + RC_Load_inPublic;
    // Retrieve sensitive data.
    result = PrivateToSensitive(&in->inPrivate.b, &out->name.b, parentObject,
				in->inPublic.publicArea.nameAlg,
				&sensitive);
    if(result != TPM_RC_SUCCESS)
	return RcSafeAddToResult(result, RC_Load_inPrivate);
    // Internal Data Update
    // Load and validate object
    result = ObjectLoad(newObject, parentObject,
			&in->inPublic.publicArea, &sensitive,
			RC_Load_inPublic, RC_Load_inPrivate,
			&out->name);
    if(result == TPM_RC_SUCCESS)
	{
	    // Set the common OBJECT attributes for a loaded object.
	    ObjectSetLoadedAttributes(newObject, in->parentHandle,
	                              parentObject->seedCompatLevel); // libtpms added
	}
    return result;
}
#endif // CC_Load
#include "Tpm.h"
#include "LoadExternal_fp.h"
#if CC_LoadExternal  // Conditional expansion of this file
#include "Object_spt_fp.h"
TPM_RC
TPM2_LoadExternal(
		  LoadExternal_In     *in,            // IN: input parameter list
		  LoadExternal_Out    *out            // OUT: output parameter list
		  )
{
    TPM_RC               result;
    OBJECT              *object;
    TPMT_SENSITIVE      *sensitive = NULL;
    // Input Validation
    // Don't get invested in loading if there is no place to put it.
    object = FindEmptyObjectSlot(&out->objectHandle);
    if(object == NULL)
	return TPM_RC_OBJECT_MEMORY;
    // If the hierarchy to be associated with this object is turned off, the object
    // cannot be loaded.
    if(!HierarchyIsEnabled(in->hierarchy))
	return TPM_RCS_HIERARCHY + RC_LoadExternal_hierarchy;
    // For loading an object with both public and sensitive
    if(in->inPrivate.size != 0)
	{
	    // An external object with a sensitive area can only be loaded in the
	    // NULL hierarchy
	    if(in->hierarchy != TPM_RH_NULL)
		return TPM_RCS_HIERARCHY + RC_LoadExternal_hierarchy;
	    // An external object with a sensitive area must have fixedTPM == CLEAR
	    // fixedParent == CLEAR so that it does not appear to be a key created by
	    // this TPM.
	    if(IS_ATTRIBUTE(in->inPublic.publicArea.objectAttributes, TPMA_OBJECT, fixedTPM)
	       || IS_ATTRIBUTE(in->inPublic.publicArea.objectAttributes, TPMA_OBJECT,
			       fixedParent)
	       || IS_ATTRIBUTE(in->inPublic.publicArea.objectAttributes, TPMA_OBJECT,
			       restricted))
		return TPM_RCS_ATTRIBUTES + RC_LoadExternal_inPublic;
	    // Have sensitive point to something other than NULL so that object
	    // initialization will load the sensitive part too
	    sensitive = &in->inPrivate.sensitiveArea;
	}
    // Need the name to initialize the object structure
    PublicMarshalAndComputeName(&in->inPublic.publicArea, &out->name);
    // Load and validate key
    result = ObjectLoad(object, NULL,
			&in->inPublic.publicArea, sensitive,
			RC_LoadExternal_inPublic, RC_LoadExternal_inPrivate,
			&out->name);
    if(result == TPM_RC_SUCCESS)
	{
	    object->attributes.external = SET;
	    // Set the common OBJECT attributes for a loaded object.
	    ObjectSetLoadedAttributes(object, in->hierarchy,
                                      // if anything can be derived from an external object,
                                      // we make sure it always uses the old algorithm
				      SEED_COMPAT_LEVEL_ORIGINAL); // libtpms added
	}
    return result;
}
#endif // CC_LoadExternal
#include "Tpm.h"
#include "ReadPublic_fp.h"
#if CC_ReadPublic  // Conditional expansion of this file
TPM_RC
TPM2_ReadPublic(
		ReadPublic_In   *in,            // IN: input parameter list
		ReadPublic_Out  *out            // OUT: output parameter list
		)
{
    OBJECT                  *object = HandleToObject(in->objectHandle);
    // Input Validation
    // Can not read public area of a sequence object
    if(ObjectIsSequence(object))
	return TPM_RC_SEQUENCE;
    // Command Output
    out->outPublic.publicArea = object->publicArea;
    out->name = object->name;
    out->qualifiedName = object->qualifiedName;
    return TPM_RC_SUCCESS;
}
#endif // CC_ReadPublic
#include "Tpm.h"
#include "ActivateCredential_fp.h"
#if CC_ActivateCredential  // Conditional expansion of this file
#include "Object_spt_fp.h"
TPM_RC
TPM2_ActivateCredential(
			ActivateCredential_In   *in,            // IN: input parameter list
			ActivateCredential_Out  *out            // OUT: output parameter list
			)
{
    TPM_RC                   result = TPM_RC_SUCCESS;
    OBJECT                  *object;            // decrypt key
    OBJECT                  *activateObject;    // key associated with credential
    TPM2B_DATA               data;          // credential data
    // Input Validation
    // Get decrypt key pointer
    object = HandleToObject(in->keyHandle);
    // Get certificated object pointer
    activateObject = HandleToObject(in->activateHandle);
    // input decrypt key must be an asymmetric, restricted decryption key
    if(!CryptIsAsymAlgorithm(object->publicArea.type)
       || !IS_ATTRIBUTE(object->publicArea.objectAttributes, TPMA_OBJECT, decrypt)
       || !IS_ATTRIBUTE(object->publicArea.objectAttributes,
			TPMA_OBJECT, restricted))
	return TPM_RCS_TYPE + RC_ActivateCredential_keyHandle;
    // Command output
    // Decrypt input credential data via asymmetric decryption.  A
    // TPM_RC_VALUE, TPM_RC_KEY or unmarshal errors may be returned at this
    // point
    result = CryptSecretDecrypt(object, NULL, IDENTITY_STRING, &in->secret, &data);
    if(result != TPM_RC_SUCCESS)
	{
	    if(result == TPM_RC_KEY)
		return TPM_RC_FAILURE;
	    return RcSafeAddToResult(result, RC_ActivateCredential_secret);
	}
    // Retrieve secret data.  A TPM_RC_INTEGRITY error or unmarshal
    // errors may be returned at this point
    result = CredentialToSecret(&in->credentialBlob.b,
				&activateObject->name.b,
				&data.b,
				object,
				&out->certInfo);
    if(result != TPM_RC_SUCCESS)
	return RcSafeAddToResult(result, RC_ActivateCredential_credentialBlob);
    return TPM_RC_SUCCESS;
}
#endif // CC_ActivateCredential
#include "Tpm.h"
#include "MakeCredential_fp.h"
#if CC_MakeCredential  // Conditional expansion of this file
#include "Object_spt_fp.h"
TPM_RC
TPM2_MakeCredential(
		    MakeCredential_In   *in,            // IN: input parameter list
		    MakeCredential_Out  *out            // OUT: output parameter list
		    )
{
    TPM_RC               result = TPM_RC_SUCCESS;
    OBJECT              *object;
    TPM2B_DATA           data;
    // Input Validation
    // Get object pointer
    object = HandleToObject(in->handle);
    // input key must be an asymmetric, restricted decryption key
    // NOTE: Needs to be restricted to have a symmetric value.
    if(!CryptIsAsymAlgorithm(object->publicArea.type)
       || !IS_ATTRIBUTE(object->publicArea.objectAttributes, TPMA_OBJECT, decrypt)
       || !IS_ATTRIBUTE(object->publicArea.objectAttributes, TPMA_OBJECT, restricted))
	return TPM_RCS_TYPE + RC_MakeCredential_handle;
    // The credential information may not be larger than the digest size used for
    // the Name of the key associated with handle.
    if(in->credential.t.size > CryptHashGetDigestSize(object->publicArea.nameAlg))
	return TPM_RCS_SIZE + RC_MakeCredential_credential;
    // Command Output
    // Make encrypt key and its associated secret structure.
    out->secret.t.size = sizeof(out->secret.t.secret);
    result = CryptSecretEncrypt(object, IDENTITY_STRING, &data, &out->secret);
    if(result != TPM_RC_SUCCESS)
	return result;
    // Prepare output credential data from secret
    SecretToCredential(&in->credential, &in->objectName.b, &data.b,
		       object, &out->credentialBlob);
    return TPM_RC_SUCCESS;
}
#endif // CC_MakeCredential
#include "Tpm.h"
#include "Unseal_fp.h"
#if CC_Unseal  // Conditional expansion of this file
TPM_RC
TPM2_Unseal(
	    Unseal_In           *in,
	    Unseal_Out          *out
	    )
{
    OBJECT                  *object;
    // Input Validation
    // Get pointer to loaded object
    object = HandleToObject(in->itemHandle);
    // Input handle must be a data object
    if(object->publicArea.type != TPM_ALG_KEYEDHASH)
	return TPM_RCS_TYPE + RC_Unseal_itemHandle;
    if(IS_ATTRIBUTE(object->publicArea.objectAttributes, TPMA_OBJECT, decrypt)
       || IS_ATTRIBUTE(object->publicArea.objectAttributes, TPMA_OBJECT, sign)
       || IS_ATTRIBUTE(object->publicArea.objectAttributes, TPMA_OBJECT, restricted))
	return TPM_RCS_ATTRIBUTES + RC_Unseal_itemHandle;
    // Command Output
    // Copy data
    out->outData = object->sensitive.sensitive.bits;
    return TPM_RC_SUCCESS;
}
#endif // CC_Unseal
#include "Tpm.h"
#include "ObjectChangeAuth_fp.h"
#if CC_ObjectChangeAuth  // Conditional expansion of this file
#include "Object_spt_fp.h"
TPM_RC
TPM2_ObjectChangeAuth(
		      ObjectChangeAuth_In     *in,            // IN: input parameter list
		      ObjectChangeAuth_Out    *out            // OUT: output parameter list
		      )
{
    TPMT_SENSITIVE           sensitive;
    OBJECT                  *object = HandleToObject(in->objectHandle);
    TPM2B_NAME               QNCompare;
    // Input Validation
    // Can not change authorization on sequence object
    if(ObjectIsSequence(object))
	return TPM_RCS_TYPE + RC_ObjectChangeAuth_objectHandle;
    // Make sure that the authorization value is consistent with the nameAlg
    if(!AdjustAuthSize(&in->newAuth, object->publicArea.nameAlg))
	return TPM_RCS_SIZE + RC_ObjectChangeAuth_newAuth;
    // Parent handle should be the parent of object handle.  In this
    // implementation we verify this by checking the QN of object.  Other
    // implementation may choose different method to verify this attribute.
    ComputeQualifiedName(in->parentHandle,
			 object->publicArea.nameAlg,
			 &object->name, &QNCompare);
    if(!MemoryEqual2B(&object->qualifiedName.b, &QNCompare.b))
	return TPM_RCS_TYPE + RC_ObjectChangeAuth_parentHandle;
    // Command Output
    // Prepare the sensitive area with the new authorization value
    sensitive = object->sensitive;
    sensitive.authValue = in->newAuth;
    // Protect the sensitive area
    SensitiveToPrivate(&sensitive, &object->name, HandleToObject(in->parentHandle),
		       object->publicArea.nameAlg,
		       &out->outPrivate);
    return TPM_RC_SUCCESS;
}
#endif // CC_ObjectChangeAuth
#include "Tpm.h"
#include "CreateLoaded_fp.h"
#if CC_CreateLoaded  // Conditional expansion of this file
TPM_RC
TPM2_CreateLoaded(
		  CreateLoaded_In    *in,            // IN: input parameter list
		  CreateLoaded_Out   *out            // OUT: output parameter list
		  )
{
    TPM_RC                       result = TPM_RC_SUCCESS;
    OBJECT                      *parent = HandleToObject(in->parentHandle);
    OBJECT                      *newObject;
    BOOL                         derivation;
    TPMT_PUBLIC                 *publicArea;
    RAND_STATE                   randState;
    RAND_STATE                  *rand = &randState;
    TPMS_DERIVE                  labelContext;
    SEED_COMPAT_LEVEL            seedCompatLevel = SEED_COMPAT_LEVEL_LAST; // libtpms added
    // Input Validation
    // How the public area is unmarshaled is determined by the parent, so
    // see if parent is a derivation parent
    derivation = (parent != NULL && parent->attributes.derivation);
    // If the parent is an object, then make sure that it is either a parent or
    // derivation parent
    if(parent != NULL && !parent->attributes.isParent && !derivation)
	return TPM_RCS_TYPE + RC_CreateLoaded_parentHandle;
    // Get a spot in which to create the newObject
    newObject = FindEmptyObjectSlot(&out->objectHandle);
    if(newObject == NULL)
	return TPM_RC_OBJECT_MEMORY;
    // Do this to save typing
    publicArea = &newObject->publicArea;
    // Unmarshal the template into the object space. TPM2_Create() and
    // TPM2_CreatePrimary() have the publicArea unmarshaled by CommandDispatcher.
    // This command is different because of an unfortunate property of the
    // unique field of an ECC key. It is a structure rather than a single TPM2B. If
    // if had been a TPM2B, then the label and context could be within a TPM2B and
    // unmarshaled like other public areas. Since it is not, this command needs its
    // on template that is a TPM2B that is unmarshaled as a BYTE array with a
    // its own unmarshal function.
    result = UnmarshalToPublic(publicArea, &in->inPublic, derivation,
			       &labelContext);
    if(result != TPM_RC_SUCCESS)
	return result + RC_CreateLoaded_inPublic;
    // Validate that the authorization size is appropriate
    if(!AdjustAuthSize(&in->inSensitive.sensitive.userAuth, publicArea->nameAlg))
	return TPM_RCS_SIZE + RC_CreateLoaded_inSensitive;
    // Command output
    if(derivation)
	{
	    TPMT_KEYEDHASH_SCHEME       *scheme;
	    scheme = &parent->publicArea.parameters.keyedHashDetail.scheme;
	    // SP800-108 is the only KDF supported by this implementation and there is
	    // no default hash algorithm.
	    pAssert(scheme->details.xorr.hashAlg != TPM_ALG_NULL
		    && scheme->details.xorr.kdf == TPM_ALG_KDF1_SP800_108);
	    // Don't derive RSA keys
	    if(publicArea->type == TPM_ALG_RSA)
		return TPM_RCS_TYPE + RC_CreateLoaded_inPublic;
	    // sensitiveDataOrigin has to be CLEAR in a derived object. Since this
	    // is specific to a derived object, it is checked here.
	    if(IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT,
			    sensitiveDataOrigin))
		return TPM_RCS_ATTRIBUTES;
	    // Check the reset of the attributes
	    result = PublicAttributesValidation(parent, publicArea);
	    if(result != TPM_RC_SUCCESS)
		return RcSafeAddToResult(result, RC_CreateLoaded_inPublic);
	    // Process the template and sensitive areas to get the actual 'label' and
	    // 'context' values to be used for this derivation.
	    result = SetLabelAndContext(&labelContext, &in->inSensitive.sensitive.data);
	    if(result != TPM_RC_SUCCESS)
		return result;
	    // Set up the KDF for object generation
	    DRBG_InstantiateSeededKdf((KDF_STATE *)rand,
				      scheme->details.xorr.hashAlg,
				      scheme->details.xorr.kdf,
				      &parent->sensitive.sensitive.bits.b,
				      &labelContext.label.b,
				      &labelContext.context.b,
				      TPM_MAX_DERIVATION_BITS);
	    // Clear the sensitive size so that the creation functions will not try
	    // to use this value.
	    in->inSensitive.sensitive.data.t.size = 0;
	    seedCompatLevel = parent->seedCompatLevel;               // libtpms added
	}
    else
	{
	    // Check attributes in input public area. CreateChecks() checks the things
	    // that are unique to creation and then validates the attributes and values
	    // that are common to create and load.
	    result = CreateChecks(parent, publicArea,
				  in->inSensitive.sensitive.data.t.size);
	    if(result != TPM_RC_SUCCESS)
		return RcSafeAddToResult(result, RC_CreateLoaded_inPublic);
	    // Creating a primary object
	    if(parent == NULL)
		{
		    TPM2B_NAME              name;
	            newObject->attributes.primary = SET;
	            if(in->parentHandle == TPM_RH_ENDORSEMENT)
	                newObject->attributes.epsHierarchy = SET;
		    seedCompatLevel =
		        HierarchyGetPrimarySeedCompatLevel(in->parentHandle); // libtpms added
	            // If so, use the primary seed and the digest of the template
	            // to seed the DRBG
		    result = DRBG_InstantiateSeeded((DRBG_STATE *)rand,
						    &HierarchyGetPrimarySeed(in->parentHandle)->b,
						    PRIMARY_OBJECT_CREATION,
						    (TPM2B *)PublicMarshalAndComputeName(publicArea,&name),
						    &in->inSensitive.sensitive.data.b,
						    seedCompatLevel);        // libtpms added
		    if (result != TPM_RC_SUCCESS)
			return result;
	        }
	    else
		// This is an ordinary object so use the normal random number generator
		rand = NULL;
	}
    // Internal data update
    // Create the object
    result = CryptCreateObject(newObject, &in->inSensitive.sensitive, rand);
    if(result != TPM_RC_SUCCESS)
	return result;
    // if this is not a Primary key and not a derived key, then return the sensitive
    // area
    if(parent != NULL && !derivation)
	// Prepare output private data from sensitive
	SensitiveToPrivate(&newObject->sensitive, &newObject->name,
			   parent, newObject->publicArea.nameAlg,
			   &out->outPrivate);
    else
	out->outPrivate.t.size = 0;
    // Set the remaining return values
    out->outPublic.publicArea = newObject->publicArea;
    out->name = newObject->name;
    // Set the remaining attributes for a loaded object
    ObjectSetLoadedAttributes(newObject, in->parentHandle,
                              seedCompatLevel); // libtpms added
    return result;
}
#endif // CC_CreateLoaded
