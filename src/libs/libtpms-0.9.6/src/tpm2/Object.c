/********************************************************************************/
/*										*/
/*		Manage the object store of the TPM.    				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Object.c $		*/
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

/* 8.6 Object.c */
/* 8.6.1 Introduction */
/* This file contains the functions that manage the object store of the TPM. */
/* 8.6.2 Includes and Data Definitions */
#define OBJECT_C
#include "Tpm.h"
#include "NVMarshal.h" // libtpms added
#include "BackwardsCompatibilityObject.h" // libtpms added
/* 8.6.3 Functions */
/* 8.6.3.1 ObjectFlush() */
/* This function marks an object slot as available. Since there is no checking of the input
   parameters, it should be used judiciously. */
/* NOTE: This could be converted to a macro. */
void
ObjectFlush(
	    OBJECT          *object
	    )
{
    object->attributes.occupied = CLEAR;
}
/* 8.6.3.2 ObjectSetInUse() */
/* This access function sets the occupied attribute of an object slot. */
void
ObjectSetInUse(
	       OBJECT          *object
	       )
{
    object->attributes.occupied = SET;
}
/* 8.6.3.3 ObjectStartup() */
/* This function is called at TPM2_Startup() to initialize the object subsystem. */
BOOL
ObjectStartup(
	      void
	      )
{
    UINT32      i;
    // object slots initialization
    for(i = 0; i < MAX_LOADED_OBJECTS; i++)
	{
	    //Set the slot to not occupied
	    ObjectFlush(&s_objects[i]);
	}
    return TRUE;
}
/* 8.6.3.4 ObjectCleanupEvict() */
/* In this implementation, a persistent object is moved from NV into an object slot for
   processing. It is flushed after command execution. This function is called from
   ExecuteCommand(). */
void
ObjectCleanupEvict(
		   void
		   )
{
    UINT32      i;
    // This has to be iterated because a command may have two handles
    // and they may both be persistent.
    // This could be made to be more efficient so that a search is not needed.
    for(i = 0; i < MAX_LOADED_OBJECTS; i++)
	{
	    // If an object is a temporary evict object, flush it from slot
	    OBJECT      *object = &s_objects[i];
	    if(object->attributes.evict == SET)
		ObjectFlush(object);
	}
    return;
}
/* 8.6.3.5 IsObjectPresent() */
/* This function checks to see if a transient handle references a loaded object.  This routine
   should not be called if the handle is not a transient handle. The function validates that the
   handle is in the implementation-dependent allowed in range for loaded transient objects. */
/* Return Values Meaning */
/* TRUE if the handle references a loaded object */
/* FALSE if the handle is not an object handle, or it does not reference to a loaded object */
BOOL
IsObjectPresent(
		TPMI_DH_OBJECT   handle         // IN: handle to be checked
		)
{
    UINT32          slotIndex = handle - TRANSIENT_FIRST;
    // Since the handle is just an index into the array that is zero based, any
    // handle value outsize of the range of:
    //    TRANSIENT_FIRST -- (TRANSIENT_FIRST + MAX_LOADED_OBJECT - 1)
    // will now be greater than or equal to MAX_LOADED_OBJECTS
    if(slotIndex >= MAX_LOADED_OBJECTS)
	return FALSE;
    // Indicate if the slot is occupied
    return (s_objects[slotIndex].attributes.occupied == TRUE);
}
/* 8.6.3.6 ObjectIsSequence() */
/* This function is used to check if the object is a sequence object. This function should not be
   called if the handle does not reference a loaded object. */
/* Return Values Meaning */
/* TRUE object is an HMAC, hash, or event sequence object */
/* FALSE object is not an HMAC, hash, or event sequence object */
BOOL
ObjectIsSequence(
		 OBJECT          *object         // IN: handle to be checked
		 )
{
    pAssert(object != NULL);
    return (object->attributes.hmacSeq == SET
	    || object->attributes.hashSeq == SET
	    || object->attributes.eventSeq == SET);
}
/* 8.6.3.7 HandleToObject() */
/* This function is used to find the object structure associated with a handle. */
/* This function requires that handle references a loaded object or a permanent handle. */
OBJECT*
HandleToObject(
	       TPMI_DH_OBJECT   handle         // IN: handle of the object
	       )
{
    UINT32              index;
    // Return NULL if the handle references a permanent handle because there is no
    // associated OBJECT.
    if(HandleGetType(handle) == TPM_HT_PERMANENT)
	return NULL;
    // In this implementation, the handle is determined by the slot occupied by the
    // object.
    index = handle - TRANSIENT_FIRST;
    pAssert(index < MAX_LOADED_OBJECTS);
    pAssert(s_objects[index].attributes.occupied);
    return &s_objects[index];
}
/* 8.6.3.9 GetQualifiedName() */
/* This function returns the Qualified Name of the object. In this implementation, the Qualified
   Name is computed when the object is loaded and is saved in the internal representation of the
   object. The alternative would be to retain the Name of the parent and compute the QN when
   needed. This would take the same amount of space so it is not recommended that the alternate be
   used. */
/* This function requires that handle references a loaded object. */
void
GetQualifiedName(
		 TPMI_DH_OBJECT   handle,        // IN: handle of the object
		 TPM2B_NAME      *qualifiedName  // OUT: qualified name of the object
		 )
{
    OBJECT      *object;
    switch(HandleGetType(handle))
	{
	  case TPM_HT_PERMANENT:
	    qualifiedName->t.size = sizeof(TPM_HANDLE);
	    UINT32_TO_BYTE_ARRAY(handle, qualifiedName->t.name);
	    break;
	  case TPM_HT_TRANSIENT:
	    object = HandleToObject(handle);
	    if(object == NULL || object->publicArea.nameAlg == TPM_ALG_NULL)
		qualifiedName->t.size = 0;
	    else
		// Copy the name
		*qualifiedName = object->qualifiedName;
	    break;
	  default:
	    FAIL(FATAL_ERROR_INTERNAL);
	}
    return;
}
/* 8.6.3.10 ObjectGetHierarchy() */
/* This function returns the handle for the hierarchy of an object. */
TPMI_RH_HIERARCHY
ObjectGetHierarchy(
		   OBJECT          *object         // IN :object
		   )
{
    if(object->attributes.spsHierarchy)
	{
	    return TPM_RH_OWNER;
	}
    else if(object->attributes.epsHierarchy)
	{
	    return TPM_RH_ENDORSEMENT;
	}
    else if(object->attributes.ppsHierarchy)
	{
	    return TPM_RH_PLATFORM;
	}
    else
	{
	    return TPM_RH_NULL;
	}
}
/* 8.6.3.11 GetHierarchy() */
/* This function returns the handle of the hierarchy to which a handle belongs. This function is
   similar to ObjectGetHierarchy() but this routine takes a handle while ObjectGetHierarchy() takes
   a pointer to an object. */
/* This function requires that handle references a loaded object. */
TPMI_RH_HIERARCHY
GetHieriarchy(
	     TPMI_DH_OBJECT   handle         // IN :object handle
	     )
{
    OBJECT          *object = HandleToObject(handle);
    return ObjectGetHierarchy(object);
}
/* 8.6.3.12 FindEmptyObjectSlot() */
/* This function finds an open object slot, if any. It will clear the attributes but will not set
   the occupied attribute. This is so that a slot may be used and discarded if everything does not
   go as planned. */
/* Return Values Meaning */
/* null no open slot found */
/* !=null pointer to available slot */
OBJECT *
FindEmptyObjectSlot(
		    TPMI_DH_OBJECT  *handle         // OUT: (optional)
		    )
{
    UINT32               i;
    OBJECT              *object;
    for(i = 0; i < MAX_LOADED_OBJECTS; i++)
	{
	    object = &s_objects[i];
	    if(object->attributes.occupied == CLEAR)
		{
		    if(handle)
			*handle = i + TRANSIENT_FIRST;
		    // Initialize the object attributes
		    // MemorySet(&object->attributes, 0, sizeof(OBJECT_ATTRIBUTES));
		    MemorySet(object, 0, sizeof(*object)); // libtpms added: Initialize the whole object
		    return object;
		}
	}
    return NULL;
}
/* 8.6.3.13 ObjectAllocateSlot() */
/* This function is used to allocate a slot in internal object array. */
/* Return Values Meaning */
OBJECT *
ObjectAllocateSlot(
		   TPMI_DH_OBJECT  *handle        // OUT: handle of allocated object
		   )
{
    OBJECT          *object = FindEmptyObjectSlot(handle);
    if(object != NULL)
	{
	    // if found, mark as occupied
	    ObjectSetInUse(object);
	}
    return object;
}
/* 8.6.3.14 ObjectSetLoadedAttributes() */
/* This function sets the internal attributes for a loaded object. It is called to finalize the
   OBJECT attributes (not the TPMA_OBJECT attributes) for a loaded object. */
void
ObjectSetLoadedAttributes(
			  OBJECT          *object,        // IN: object attributes to finalize
			  TPM_HANDLE       parentHandle,  // IN: the parent handle
			  SEED_COMPAT_LEVEL seedCompatLevel // IN: seed compat level to use for children
			  )
{
    OBJECT              *parent = HandleToObject(parentHandle);
    TPMA_OBJECT          objectAttributes = object->publicArea.objectAttributes;

    object->seedCompatLevel = seedCompatLevel; // libtpms added
    //
    // Copy the stClear attribute from the public area. This could be overwritten
    // if the parent has stClear SET
    object->attributes.stClear =
	IS_ATTRIBUTE(objectAttributes, TPMA_OBJECT, stClear);
    // If parent handle is a permanent handle, it is a primary (unless it is NULL
    if(parent == NULL)
	{
	    object->attributes.primary = SET;
	    switch(parentHandle)
		{
		  case TPM_RH_ENDORSEMENT:
		    object->attributes.epsHierarchy = SET;
		    break;
		  case TPM_RH_OWNER:
		    object->attributes.spsHierarchy = SET;
		    break;
		  case TPM_RH_PLATFORM:
		    object->attributes.ppsHierarchy = SET;
		    break;
		  default:
		    // Treat the temporary attribute as a hierarchy
		    object->attributes.temporary = SET;
		    object->attributes.primary = CLEAR;
		    break;
		}
	}
    else
	{
	    // is this a stClear object
	    object->attributes.stClear =
		(IS_ATTRIBUTE(objectAttributes, TPMA_OBJECT, stClear)
		 || (parent->attributes.stClear == SET));
	    object->attributes.epsHierarchy = parent->attributes.epsHierarchy;
	    object->attributes.spsHierarchy = parent->attributes.spsHierarchy;
	    object->attributes.ppsHierarchy = parent->attributes.ppsHierarchy;
	    // An object is temporary if its parent is temporary or if the object
	    // is external
	    object->attributes.temporary = parent->attributes.temporary
					   || object->attributes.external;
	}
    // If this is an external object, set the QN == name but don't SET other
    // key properties ('parent' or 'derived')
    if(object->attributes.external)
	object->qualifiedName = object->name;
    else
	{
	    // check attributes for different types of parents
	    if(IS_ATTRIBUTE(objectAttributes, TPMA_OBJECT, restricted)
	       && !object->attributes.publicOnly
	       && IS_ATTRIBUTE(objectAttributes, TPMA_OBJECT, decrypt)
	       && object->publicArea.nameAlg != TPM_ALG_NULL)
		{
		    // This is a parent. If it is not a KEYEDHASH, it is an ordinary parent.
		    // Otherwise, it is a derivation parent.
		    if(object->publicArea.type == TPM_ALG_KEYEDHASH)
			object->attributes.derivation = SET;
		    else
			object->attributes.isParent = SET;
		}
	    ComputeQualifiedName(parentHandle, object->publicArea.nameAlg,
				 &object->name, &object->qualifiedName);
	}
    // Set slot occupied
    ObjectSetInUse(object);
    return;
}
/* 8.6.3.15 ObjectLoad() */
/* Common function to load an object. A loaded object has its public area validated (unless its
   nameAlg is TPM_ALG_NULL). If a sensitive part is loaded, it is verified to be correct and if both
   public and sensitive parts are loaded, then the cryptographic binding between the objects is
   validated. This function does not cause the allocated slot to be marked as in use. */
TPM_RC
ObjectLoad(
	   OBJECT          *object,        // IN: pointer to object slot
	   //     object
	   OBJECT          *parent,        // IN: (optional) the parent object
	   TPMT_PUBLIC     *publicArea,    // IN: public area to be installed in the object
	   TPMT_SENSITIVE  *sensitive,     // IN: (optional) sensitive area to be
	   //      installed in the object
	   TPM_RC           blamePublic,   // IN: parameter number to associate with the
	   //     publicArea errors
	   TPM_RC           blameSensitive,// IN: parameter number to associate with the
	   //     sensitive area errors
	   TPM2B_NAME      *name           // IN: (optional)
	   )
{
    TPM_RC           result = TPM_RC_SUCCESS;
    BOOL             doCheck;
    //
    // Do validations of public area object descriptions
    // Is this public only or a no-name object?
    if(sensitive == NULL || publicArea->nameAlg == TPM_ALG_NULL)
	{
	    // Need to have schemes checked so that we do the right thing with the
	    // public key.
	    result = SchemeChecks(NULL, publicArea);
	}
    else
	{
	    // For any sensitive area, make sure that the seedSize is no larger than the
	    // digest size of nameAlg
	    if(sensitive->seedValue.t.size
	       > CryptHashGetDigestSize(publicArea->nameAlg))
		return TPM_RCS_KEY_SIZE + blameSensitive;
	    // Check attributes and schemes for consistency
	    result = PublicAttributesValidation(parent, publicArea);
	}
    if(result != TPM_RC_SUCCESS)
	return RcSafeAddToResult(result, blamePublic);
    // If object == NULL, then this is am import. For import, load is not called
    // unless the parent is fixedTPM.
    if(object == NULL)
	doCheck = TRUE;// //
    // If the parent is not NULL, then this is an ordinary load and we only check
    // if the parent is not fixedTPM
    else if(parent != NULL)
	doCheck = !IS_ATTRIBUTE(parent->publicArea.objectAttributes,
				TPMA_OBJECT, fixedTPM);
    else
	// This is a loadExternal. Check everything.
	// Note: the check functions will filter things based on the name algorithm
	// and whether or not both parts are loaded.
	doCheck = TRUE;
    // Note: the parent will be NULL if this is a load external. CryptValidateKeys()
    // will only check the parts that need to be checked based on the settings
    // of publicOnly and nameAlg.
    // Note: For an RSA key, the keys sizes are checked but the binding is not
    // checked.
    if(doCheck)
	{
	    // Do the cryptographic key validation
	    result = CryptValidateKeys(publicArea, sensitive, blamePublic,
				       blameSensitive);
	}
    // If this is an import, we are done
    if(object == NULL || result != TPM_RC_SUCCESS)
	return result;
    // Set the name, if one was provided
    if(name != NULL)
	object->name = *name;
    else
	object->name.t.size = 0;
    // Initialize public
    object->publicArea = *publicArea;
    // If there is a sensitive area, load it
    if(sensitive == NULL)
	object->attributes.publicOnly = SET;
    else
	{
	    object->sensitive = *sensitive;
#if ALG_RSA
	    // If this is an RSA key that is not a parent, complete the load by
	    // computing the private exponent.
	    if(publicArea->type == ALG_RSA_VALUE)
		result = CryptRsaLoadPrivateExponent(object);
#endif
	}
    return result;
}
/* 8.6.3.16 AllocateSequenceSlot() */
/* This function allocates a sequence slot and initializes the parts that are used by the normal
   objects so that a sequence object is not inadvertently used for an operation that is not
   appropriate for a sequence. */
static HASH_OBJECT *
AllocateSequenceSlot(
		     TPM_HANDLE      *newHandle,     // OUT: receives the allocated handle
		     TPM2B_AUTH      *auth           // IN: the authValue for the slot
		     )
{
    HASH_OBJECT      *object = (HASH_OBJECT *)ObjectAllocateSlot(newHandle);
    //
    // Validate that the proper location of the hash state data relative to the
    // object state data. It would be good if this could have been done at compile
    // time but it can't so do it in something that can be removed after debug.
    cAssert(offsetof(HASH_OBJECT, auth) == offsetof(OBJECT, publicArea.authPolicy));
    if(object != NULL)
	{
	    // Set the common values that a sequence object shares with an ordinary object
	    // First, clear all attributes
	    MemorySet(&object->objectAttributes, 0, sizeof(TPMA_OBJECT));
	    // The type is TPM_ALG_NULL
	    object->type = TPM_ALG_NULL;
	    // This has no name algorithm and the name is the Empty Buffer
	    object->nameAlg = TPM_ALG_NULL;
	    // A sequence object is considered to be in the NULL hierarchy so it should
	    // be marked as temporary so that it can't be persisted
	    object->attributes.temporary = SET;
	    // A sequence object is DA exempt.
	    SET_ATTRIBUTE(object->objectAttributes, TPMA_OBJECT, noDA);
	    // Copy the authorization value
	    if(auth != NULL)
		object->auth = *auth;
	    else
		object->auth.t.size = 0;
	}
    return object;
}
/* 8.6.3.17 ObjectCreateHMACSequence() */
/* This function creates an internal HMAC sequence object. */
/* Error Returns Meaning */
/* TPM_RC_OBJECT_MEMORY if there is no free slot for an object */
#if CC_HMAC_Start || CC_MAC_Start
TPM_RC
ObjectCreateHMACSequence(
			 TPMI_ALG_HASH    hashAlg,       // IN: hash algorithm
			 OBJECT          *keyObject,     // IN: the object containing the HMAC key
			 TPM2B_AUTH      *auth,          // IN: authValue
			 TPMI_DH_OBJECT  *newHandle      // OUT: HMAC sequence object handle
			 )
{
    HASH_OBJECT         *hmacObject;
    //
    // Try to allocate a slot for new object
    hmacObject = AllocateSequenceSlot(newHandle, auth);
    if(hmacObject == NULL)
	return TPM_RC_OBJECT_MEMORY;
    // Set HMAC sequence bit
    hmacObject->attributes.hmacSeq = SET;
#if !SMAC_IMPLEMENTED
    if(CryptHmacStart(&hmacObject->state.hmacState, hashAlg,
		      keyObject->sensitive.sensitive.bits.b.size,
		      keyObject->sensitive.sensitive.bits.b.buffer) == 0)
#else
	if(CryptMacStart(&hmacObject->state.hmacState,
			 &keyObject->publicArea.parameters,
			 hashAlg, &keyObject->sensitive.sensitive.any.b) == 0)
#endif // SMAC_IMPLEMENTED
	    return TPM_RC_FAILURE;
    return TPM_RC_SUCCESS;
}
#endif

/* 8.6.3.18 ObjectCreateHashSequence() */
/* This function creates a hash sequence object. */
/* Error Returns Meaning */
/* TPM_RC_OBJECT_MEMORY if there is no free slot for an object */
TPM_RC
ObjectCreateHashSequence(
			 TPMI_ALG_HASH    hashAlg,       // IN: hash algorithm
			 TPM2B_AUTH      *auth,          // IN: authValue
			 TPMI_DH_OBJECT  *newHandle      // OUT: sequence object handle
			 )
{
    HASH_OBJECT         *hashObject = AllocateSequenceSlot(newHandle, auth);
    // See if slot allocated
    if(hashObject == NULL)
	return TPM_RC_OBJECT_MEMORY;
    // Set hash sequence bit
    hashObject->attributes.hashSeq = SET;
    // Start hash for hash sequence
    CryptHashStart(&hashObject->state.hashState[0], hashAlg);
    return TPM_RC_SUCCESS;
}
/* 8.6.3.19 ObjectCreateEventSequence() */
/* This function creates an event sequence object. */
/* Error Returns Meaning */
/* TPM_RC_OBJECT_MEMORY if there is no free slot for an object */
TPM_RC
ObjectCreateEventSequence(
			  TPM2B_AUTH      *auth,          // IN: authValue
			  TPMI_DH_OBJECT  *newHandle      // OUT: sequence object handle
			  )
{
    HASH_OBJECT         *hashObject = AllocateSequenceSlot(newHandle, auth);
    UINT32               count;
    TPM_ALG_ID           hash;
    // See if slot allocated
    if(hashObject == NULL)
	return TPM_RC_OBJECT_MEMORY;
    // Set the event sequence attribute
    hashObject->attributes.eventSeq = SET;
    // Initialize hash states for each implemented PCR algorithms
    for(count = 0; (hash = CryptHashGetAlgByIndex(count)) != TPM_ALG_NULL; count++)
	CryptHashStart(&hashObject->state.hashState[count], hash);
    return TPM_RC_SUCCESS;
}
/* 8.6.3.20 ObjectTerminateEvent() */
/* This function is called to close out the event sequence and clean up the hash context states. */
void
ObjectTerminateEvent(
		     void
		     )
{
    HASH_OBJECT         *hashObject;
    int                  count;
    BYTE                 buffer[MAX_DIGEST_SIZE];
    hashObject = (HASH_OBJECT *)HandleToObject(g_DRTMHandle);
    // Don't assume that this is a proper sequence object
    if(hashObject->attributes.eventSeq)
	{
	    // If it is, close any open hash contexts. This is done in case
	    // the cryptographic implementation has some context values that need to be
	    // cleaned up (hygiene).
	    //
	    for(count = 0; CryptHashGetAlgByIndex(count) != TPM_ALG_NULL; count++)
		{
		    CryptHashEnd(&hashObject->state.hashState[count], 0, buffer);
		}
	    // Flush sequence object
	    FlushObject(g_DRTMHandle);
	}
    g_DRTMHandle = TPM_RH_UNASSIGNED;
}
/* 8.6.3.21 ObjectContextLoad() */
/* This function loads an object from a saved object context. */
/* Return Values Meaning */
/* NULL if there is no free slot for an object */
/* NON_NULL points to the loaded object */
#if 0			// libtpms added
OBJECT *
ObjectContextLoad(
		  ANY_OBJECT_BUFFER   *object,        // IN: pointer to object structure in saved
		  //     context
		  TPMI_DH_OBJECT      *handle         // OUT: object handle
		  )
{
    OBJECT      *newObject = ObjectAllocateSlot(handle);
    // Try to allocate a slot for new object
    if(newObject != NULL)
	{
	    // Copy the first part of the object
	    MemoryCopy(newObject, object, offsetof(HASH_OBJECT, state));
	    // See if this is a sequence object
	    if(ObjectIsSequence(newObject))
		{
		    // If this is a sequence object, import the data
		    SequenceDataImport((HASH_OBJECT *)newObject,
				       (HASH_OBJECT_BUFFER *)object);
		}
	    else
		{
		    // Copy input object data to internal structure
		    MemoryCopy(newObject, object, sizeof(OBJECT));
		}
	}
    return newObject;
}
#endif 			// libtpms added begin

OBJECT *
ObjectContextLoadLibtpms(BYTE           *buffer,
                         INT32           size,
                         TPMI_DH_OBJECT *handle
                         )
{
    OBJECT      *newObject = ObjectAllocateSlot(handle);
    TPM_RC       rc;
    BYTE        *mybuf  = buffer;
    INT32        mysize = size;

    pAssert(handle);

    // Try to allocate a slot for new object
    if(newObject != NULL)
	{
	    rc = ANY_OBJECT_Unmarshal(newObject, &mybuf, &mysize, false);
	    if (rc) {
	         /* Attempt to load an old OBJECT that was copied out directly from
	          * an older version of OBJECT.
	          */
		rc = OLD_OBJECTToOBJECT(newObject, buffer, size);
		if (rc) {
		    FlushObject(*handle);
		    newObject = NULL;
		}
	    }
	}
    return newObject;
}			// libtpms added end

/* 8.6.3.22 FlushObject() */
/* This function frees an object slot. */
/* This function requires that the object is loaded. */
void
FlushObject(
	    TPMI_DH_OBJECT   handle         // IN: handle to be freed
	    )
{
    UINT32      index = handle - TRANSIENT_FIRST;
    pAssert(index < MAX_LOADED_OBJECTS);
    // Clear all the object attributes
    MemorySet((BYTE*)&(s_objects[index].attributes),
	      0, sizeof(OBJECT_ATTRIBUTES));
    return;
}
/* 8.6.3.23 ObjectFlushHierarchy() */
/* This function is called to flush all the loaded transient objects associated with a hierarchy
   when the hierarchy is disabled. */
void
ObjectFlushHierarchy(
		     TPMI_RH_HIERARCHY    hierarchy      // IN: hierarchy to be flush
		     )
{
    UINT16          i;
    // iterate object slots
    for(i = 0; i < MAX_LOADED_OBJECTS; i++)
	{
	    if(s_objects[i].attributes.occupied)          // If found an occupied slot
		{
		    switch(hierarchy)
			{
			  case TPM_RH_PLATFORM:
			    if(s_objects[i].attributes.ppsHierarchy == SET)
				s_objects[i].attributes.occupied = FALSE;
			    break;
			  case TPM_RH_OWNER:
			    if(s_objects[i].attributes.spsHierarchy == SET)
				s_objects[i].attributes.occupied = FALSE;
			    break;
			  case TPM_RH_ENDORSEMENT:
			    if(s_objects[i].attributes.epsHierarchy == SET)
				s_objects[i].attributes.occupied = FALSE;
			    break;
			  default:
			    FAIL(FATAL_ERROR_INTERNAL);
			    break;
			}
		}
	}
    return;
}
/* 8.6.3.24 ObjectLoadEvict() */
/* This function loads a persistent object into a transient object slot. */
/* This function requires that handle is associated with a persistent object. */
/* Error Returns Meaning */
/* TPM_RC_HANDLE the persistent object does not exist or the associated hierarchy is disabled. */
/* TPM_RC_OBJECT_MEMORY no object slot */
TPM_RC
ObjectLoadEvict(
		TPM_HANDLE      *handle,        // IN:OUT: evict object handle.  If success, it
		// will be replace by the loaded object handle
		COMMAND_INDEX    commandIndex   // IN: the command being processed
		)
{
    TPM_RC          result;
    TPM_HANDLE      evictHandle = *handle;   // Save the evict handle
    OBJECT          *object;
    // If this is an index that references a persistent object created by
    // the platform, then return TPM_RH_HANDLE if the phEnable is FALSE
    if(*handle >= PLATFORM_PERSISTENT)
	{
	    // belongs to platform
	    if(g_phEnable == CLEAR)
		return TPM_RC_HANDLE;
	}
    // belongs to owner
    else if(gc.shEnable == CLEAR)
	return TPM_RC_HANDLE;
    // Try to allocate a slot for an object
    object = ObjectAllocateSlot(handle);
    if(object == NULL)
	return TPM_RC_OBJECT_MEMORY;
    // Copy persistent object to transient object slot.  A TPM_RC_HANDLE
    // may be returned at this point. This will mark the slot as containing
    // a transient object so that it will be flushed at the end of the
    // command
    result = NvGetEvictObject(evictHandle, object);
    // Bail out if this failed
    if(result != TPM_RC_SUCCESS)
	return result;
    // check the object to see if it is in the endorsement hierarchy
    // if it is and this is not a TPM2_EvictControl() command, indicate
    // that the hierarchy is disabled.
    // If the associated hierarchy is disabled, make it look like the
    // handle is not defined
    if(ObjectGetHierarchy(object) == TPM_RH_ENDORSEMENT
       && gc.ehEnable == CLEAR
       && GetCommandCode(commandIndex) != TPM_CC_EvictControl)
	return TPM_RC_HANDLE;
    return result;
}
/* 8.6.3.25 ObjectComputeName() */
/* This does the name computation from a public area (can be marshaled or not). */
TPM2B_NAME *
ObjectComputeName(
		  UINT32           size,          // IN: the size of the area to digest
		  BYTE            *publicArea,    // IN: the public area to digest
		  TPM_ALG_ID       nameAlg,       // IN: the hash algorithm to use
		  TPM2B_NAME      *name           // OUT: Computed name
		  )
{
    // Hash the publicArea into the name buffer leaving room for the nameAlg
    name->t.size = CryptHashBlock(nameAlg, size, publicArea,
				  sizeof(name->t.name) - 2,
				  &name->t.name[2]);
    // set the nameAlg
    UINT16_TO_BYTE_ARRAY(nameAlg, name->t.name);
    name->t.size += 2;
    return name;
}
/* 8.6.3.26 PublicMarshalAndComputeName() */
/* This function computes the Name of an object from its public area. */
TPM2B_NAME *
PublicMarshalAndComputeName(
			    TPMT_PUBLIC     *publicArea,    // IN: public area of an object
			    TPM2B_NAME      *name           // OUT: name of the object
			    )
{
    // Will marshal a public area into a template. This is because the internal
    // format for a TPM2B_PUBLIC is a structure and not a simple BYTE buffer.
    TPM2B_TEMPLATE       marshaled;     // this is big enough to hold a
    //  marshaled TPMT_PUBLIC
    BYTE                *buffer = (BYTE *)&marshaled.t.buffer;
    // if the nameAlg is NULL then there is no name.
    if(publicArea->nameAlg == TPM_ALG_NULL)
	name->t.size = 0;
    else
	{
	    // Marshal the public area into its canonical form
	    marshaled.t.size = TPMT_PUBLIC_Marshal(publicArea, &buffer, NULL);
	    // and compute the name
	    ObjectComputeName(marshaled.t.size, marshaled.t.buffer,
			      publicArea->nameAlg, name);
	}
    return name;
}
/* 8.6.3.28 ComputeQualifiedName() */
/* This function computes the qualified name of an object. */
void
ComputeQualifiedName(
		     TPM_HANDLE       parentHandle,  // IN: parent's handle
		     TPM_ALG_ID       nameAlg,       // IN: name hash
		     TPM2B_NAME      *name,          // IN: name of the object
		     TPM2B_NAME      *qualifiedName  // OUT: qualified name of the object
		     )
{
    HASH_STATE      hashState;   // hash state
    TPM2B_NAME      parentName;
    if(parentHandle == TPM_RH_UNASSIGNED)
	{
	    MemoryCopy2B(&qualifiedName->b, &name->b, sizeof(qualifiedName->t.name));
	    *qualifiedName = *name;
	}
    else
	{
	    GetQualifiedName(parentHandle, &parentName);
	    //      QN_A = hash_A (QN of parent || NAME_A)
	    // Start hash
	    qualifiedName->t.size = CryptHashStart(&hashState, nameAlg);
	    // Add parent's qualified name
	    CryptDigestUpdate2B(&hashState, &parentName.b);
	    // Add self name
	    CryptDigestUpdate2B(&hashState, &name->b);
	    // Complete hash leaving room for the name algorithm
	    CryptHashEnd(&hashState, qualifiedName->t.size,
			 &qualifiedName->t.name[2]);
	    UINT16_TO_BYTE_ARRAY(nameAlg, qualifiedName->t.name);
	    qualifiedName->t.size += 2;
	}
    return;
}
/* 8.6.3.29 ObjectIsStorage() */
/* This function determines if an object has the attributes associated with a parent. A parent is an
   asymmetric or symmetric block cipher key that has its restricted and decrypt attributes SET, and
   sign CLEAR. */
/* Return Values Meaning */
/* TRUE if the object is a storage key */
/* FALSE if the object is not a storage key */
BOOL
ObjectIsStorage(
		TPMI_DH_OBJECT   handle         // IN: object handle
		)
{
    OBJECT           *object = HandleToObject(handle);
    TPMT_PUBLIC      *publicArea = ((object != NULL) ? &object->publicArea : NULL);
    //
    return (publicArea != NULL
	    && IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT, restricted)
	    && IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT, decrypt)
	    && !IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT, sign)
	    && (object->publicArea.type == ALG_RSA_VALUE
		|| object->publicArea.type == ALG_ECC_VALUE));
}
/* 8.6.3.30 ObjectCapGetLoaded() */
/* This function returns a a list of handles of loaded object, starting from handle. Handle must be
   in the range of valid transient object handles, but does not have to be the handle of a loaded
   transient object. */
/* Return Values Meaning */
/* YES if there are more handles available */
/* NO all the available handles has been returned */
TPMI_YES_NO
ObjectCapGetLoaded(
		   TPMI_DH_OBJECT   handle,        // IN: start handle
		   UINT32           count,         // IN: count of returned handles
		   TPML_HANDLE     *handleList     // OUT: list of handle
		   )
{
    TPMI_YES_NO          more = NO;
    UINT32               i;
    pAssert(HandleGetType(handle) == TPM_HT_TRANSIENT);
    // Initialize output handle list
    handleList->count = 0;
    // The maximum count of handles we may return is MAX_CAP_HANDLES
    if(count > MAX_CAP_HANDLES) count = MAX_CAP_HANDLES;
    // Iterate object slots to get loaded object handles
    for(i = handle - TRANSIENT_FIRST; i < MAX_LOADED_OBJECTS; i++)
	{
	    if(s_objects[i].attributes.occupied == TRUE)
		{
		    // A valid transient object can not be the copy of a persistent object
		    pAssert(s_objects[i].attributes.evict == CLEAR);
		    if(handleList->count < count)
			{
			    // If we have not filled up the return list, add this object
			    // handle to it
			    handleList->handle[handleList->count] = i + TRANSIENT_FIRST;
			    handleList->count++;
			}
		    else
			{
			    // If the return list is full but we still have loaded object
			    // available, report this and stop iterating
			    more = YES;
			    break;
			}
		}
	}
    return more;
}
/* 8.6.3.31 ObjectCapGetTransientAvail() */
/* This function returns an estimate of the number of additional transient objects that could be
   loaded into the TPM. */
UINT32
ObjectCapGetTransientAvail(
			   void
			   )
{
    UINT32      i;
    UINT32      num = 0;
    // Iterate object slot to get the number of unoccupied slots
    for(i = 0; i < MAX_LOADED_OBJECTS; i++)
	{
	    if(s_objects[i].attributes.occupied == FALSE) num++;
	}
    return num;
}
/* 8.6.3.32 ObjectGetPublicAttributes() */
/* Returns the attributes associated with an object handles. */
TPMA_OBJECT
ObjectGetPublicAttributes(
			  TPM_HANDLE       handle
			  )
{
    return HandleToObject(handle)->publicArea.objectAttributes;
}
#if 0 /* libtpms added */
OBJECT_ATTRIBUTES
ObjectGetProperties(
		    TPM_HANDLE       handle
		    )
{
    return HandleToObject(handle)->attributes;
}
#endif /* libtpms added */
