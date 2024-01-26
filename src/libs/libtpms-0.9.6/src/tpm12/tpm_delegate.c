/********************************************************************************/
/*										*/
/*				Delegate Handler				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_delegate.c $		*/
/*										*/
/* (c) Copyright IBM Corporation 2006, 2010.					*/
/*										*/
/* All rights reserved.								*/
/* 										*/
/* Redistribution and use in source and binary forms, with or without		*/
/* modification, are permitted provided that the following conditions are	*/
/* met:										*/
/* 										*/
/* Redistributions of source code must retain the above copyright notice,	*/
/* this list of conditions and the following disclaimer.			*/
/* 										*/
/* Redistributions in binary form must reproduce the above copyright		*/
/* notice, this list of conditions and the following disclaimer in the		*/
/* documentation and/or other materials provided with the distribution.		*/
/* 										*/
/* Neither the names of the IBM Corporation nor the names of its		*/
/* contributors may be used to endorse or promote products derived from		*/
/* this software without specific prior written permission.			*/
/* 										*/
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS		*/
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT		*/
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR	*/
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT		*/
/* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,	*/
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT		*/
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,	*/
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY	*/
/* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT		*/
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE	*/
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.		*/
/********************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "tpm_auth.h"
#include "tpm_crypto.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_key.h"
#include "tpm_pcr.h"
#include "tpm_permanent.h"
#include "tpm_process.h"
#include "tpm_secret.h"

#include "tpm_delegate.h"

/*
  TPM_DELEGATE_PUBLIC
*/

/* TPM_DelegatePublic_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DelegatePublic_Init(TPM_DELEGATE_PUBLIC *tpm_delegate_public)
{
    printf(" TPM_DelegatePublic_Init:\n");
    tpm_delegate_public->rowLabel = 0;
    TPM_PCRInfoShort_Init(&(tpm_delegate_public->pcrInfo));
    TPM_Delegations_Init(&(tpm_delegate_public->permissions));
    tpm_delegate_public->familyID = 0;
    tpm_delegate_public->verificationCount = 0;
    return;
}

/* TPM_DelegatePublic_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes

   After use, call TPM_DelegatePublic_Delete() to free memory
*/

TPM_RESULT TPM_DelegatePublic_Load(TPM_DELEGATE_PUBLIC *tpm_delegate_public,
				   unsigned char **stream,
				   uint32_t *stream_size)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_DelegatePublic_Load:\n");
    /* check the tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_DELEGATE_PUBLIC, stream, stream_size);
    }
    /* load rowLabel */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_delegate_public->rowLabel), stream, stream_size);
    }
    /* load pcrInfo */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_Load(&(tpm_delegate_public->pcrInfo), stream, stream_size, FALSE);
    }
    /* load permissions */
    if (rc == 0) {
	rc = TPM_Delegations_Load(&(tpm_delegate_public->permissions), stream, stream_size);
    }
    /* load the familyID */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_delegate_public->familyID), stream, stream_size);
    }
    /* load the verificationCount */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_delegate_public->verificationCount), stream, stream_size);
    }
    return rc;
}

/* TPM_DelegatePublic_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DelegatePublic_Store(TPM_STORE_BUFFER *sbuffer,
				    const TPM_DELEGATE_PUBLIC *tpm_delegate_public)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_DelegatePublic_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_DELEGATE_PUBLIC);
    }
    /* store rowLabel */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_delegate_public->rowLabel),
			       sizeof(TPM_DELEGATE_LABEL)); 
    }
    /* store pcrInfo */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_Store(sbuffer, &(tpm_delegate_public->pcrInfo), FALSE); 
    } 
    /* store permissions */
    if (rc == 0) {
	rc = TPM_Delegations_Store(sbuffer, &(tpm_delegate_public->permissions)); 
    }
    /* store familyID */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_delegate_public->familyID);
    }
    /* store verificationCount */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_delegate_public->verificationCount);
    }
    return rc;
}

/* TPM_DelegatePublic_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DelegatePublic_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_DelegatePublic_Delete(TPM_DELEGATE_PUBLIC *tpm_delegate_public)
{
    printf(" TPM_DeleteDelegatePublic:\n");
    if (tpm_delegate_public != NULL) {
	TPM_PCRInfoShort_Delete(&(tpm_delegate_public->pcrInfo));
	TPM_Delegations_Delete(&(tpm_delegate_public->permissions));
	TPM_DelegatePublic_Init(tpm_delegate_public);
    }
    return;
}

/* TPM_DelegatePublic_Copy() copies the 'src' to the 'dest' structure

*/

TPM_RESULT TPM_DelegatePublic_Copy(TPM_DELEGATE_PUBLIC *dest,
				   TPM_DELEGATE_PUBLIC *src)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_DelegatePublic_Copy:\n");
    if (rc == 0) {
	/* copy rowLabel */
	dest->rowLabel = src->rowLabel;
	/* copy pcrInfo */
	rc = TPM_PCRInfoShort_Copy(&(dest->pcrInfo), &(src->pcrInfo));
    }
    if (rc == 0) {
	/* copy permissions */
	TPM_Delegations_Copy(&(dest->permissions), &(src->permissions));
	/* copy familyID */
	dest->familyID = src->familyID;
	/* copy verificationCount */
	dest->verificationCount = src->verificationCount;
    }
    return rc;
}

/*
  TPM_DELEGATE_SENSITIVE
*/

/* TPM_DelegateSensitive_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DelegateSensitive_Init(TPM_DELEGATE_SENSITIVE *tpm_delegate_sensitive)
{
    printf(" TPM_DelegateSensitive_Init:\n");
    TPM_Secret_Init(tpm_delegate_sensitive->authValue);
    return;
}

/* TPM_DelegateSensitive_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes

   After use, call TPM_DelegateSensitive_Delete() to free memory
*/

TPM_RESULT TPM_DelegateSensitive_Load(TPM_DELEGATE_SENSITIVE *tpm_delegate_sensitive,
				      unsigned char **stream,
				      uint32_t *stream_size)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_DelegateSensitive_Load:\n");
    /* check the tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_DELEGATE_SENSITIVE, stream, stream_size);
    }
    /* load authValue */
    if (rc == 0) {
	rc = TPM_Secret_Load(tpm_delegate_sensitive->authValue, stream, stream_size);
    }
    return rc;
}

/* TPM_DelegateSensitive_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DelegateSensitive_Store(TPM_STORE_BUFFER *sbuffer,
				       const TPM_DELEGATE_SENSITIVE *tpm_delegate_sensitive)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_DelegateSensitive_Store:\n");
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_DELEGATE_SENSITIVE);
    }
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_delegate_sensitive->authValue);
    }
    return rc;
}

/* TPM_DelegateSensitive_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DelegateSensitive_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_DelegateSensitive_Delete(TPM_DELEGATE_SENSITIVE *tpm_delegate_sensitive)
{
    printf(" TPM_DeleteDelegateSensitive:\n");
    if (tpm_delegate_sensitive != NULL) {
	TPM_DelegateSensitive_Init(tpm_delegate_sensitive);
    }
    return;
}

/* TPM_DelegateSensitive_DecryptEncData() decrypts 'sensitiveArea' to a stream using 'delegateKey'
   and then deserializes the stream to a TPM_DELEGATE_SENSITIVE
*/

TPM_RESULT TPM_DelegateSensitive_DecryptEncData(TPM_DELEGATE_SENSITIVE *tpm_delegate_sensitive,
						TPM_SIZED_BUFFER *sensitiveArea,
						TPM_SYMMETRIC_KEY_TOKEN delegateKey)
{
    TPM_RESULT	rc = 0;
    unsigned char		*s1;			/* decrypted sensitive data */
    uint32_t			s1_length;	
    unsigned char		*stream;		/* temp input stream */
    uint32_t			stream_size;

    printf(" TPM_DelegateSensitive_DecryptEncData:\n");
    s1 = NULL;						/* freed @1 */
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_Decrypt(&s1,		/* decrypted data */
					  &s1_length,	/* length decrypted data */
					  sensitiveArea->buffer, 
					  sensitiveArea->size,
					  delegateKey);
    }
    if (rc == 0) {
	stream = s1;
	stream_size = s1_length;
	rc = TPM_DelegateSensitive_Load(tpm_delegate_sensitive, &stream, &stream_size);
    }
    free(s1);		/* @1 */
    return rc;
}

/*
  TPM_DELEGATIONS
*/

/* TPM_Delegations_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_Delegations_Init(TPM_DELEGATIONS *tpm_delegations)
{
    printf(" TPM_Delegations_Init:\n");
    tpm_delegations->delegateType = TPM_DEL_KEY_BITS;	/* any legal value */
    tpm_delegations->per1 = 0;
    tpm_delegations->per2 = 0;
    return;
}

/* TPM_Delegations_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes

   After use, call TPM_DeleteDelegations() to free memory
*/

TPM_RESULT TPM_Delegations_Load(TPM_DELEGATIONS *tpm_delegations,
				unsigned char **stream,
				uint32_t *stream_size)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_Delegations_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_DELEGATIONS, stream, stream_size);
    }
    /* load delegateType */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_delegations->delegateType), stream, stream_size);
    }
    /* load per1 */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_delegations->per1), stream, stream_size);
    }
    /* load per2 */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_delegations->per2), stream, stream_size);
    }
    /* validate parameters */
    if (rc == 0) {
	if (tpm_delegations->delegateType == TPM_DEL_OWNER_BITS) {
	    if (tpm_delegations->per1 & ~TPM_DELEGATE_PER1_MASK) {
		printf("TPM_Delegations_Load: Error, owner per1 %08x\n", tpm_delegations->per1);
		rc = TPM_BAD_PARAMETER;
	    }
	    if (tpm_delegations->per2 & ~TPM_DELEGATE_PER2_MASK) {
		printf("TPM_Delegations_Load: Error, owner per2 %08x\n", tpm_delegations->per2);
		rc = TPM_BAD_PARAMETER;
	    }
	}
	else if (tpm_delegations->delegateType == TPM_DEL_KEY_BITS) {
	    if (tpm_delegations->per1 & ~TPM_KEY_DELEGATE_PER1_MASK) {
		printf("TPM_Delegations_Load: Error, key per1 %08x\n", tpm_delegations->per1);
		rc = TPM_BAD_PARAMETER;
	    }
	    if (tpm_delegations->per2 & ~TPM_KEY_DELEGATE_PER2_MASK) {
		printf("TPM_Delegations_Load: Error, key per2 %08x\n", tpm_delegations->per2);
		rc = TPM_BAD_PARAMETER;
	    }
	}
	else {
	    printf("TPM_Delegations_Load: Error, delegateType %08x\n",
		   tpm_delegations->delegateType);
	    rc = TPM_BAD_PARAMETER;
	}
    }	 
    return rc;
}

/* TPM_Delegations_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_Delegations_Store(TPM_STORE_BUFFER *sbuffer,
				 const TPM_DELEGATIONS *tpm_delegations)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_Delegations_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_DELEGATIONS);
    }
    /* store delegateType */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_delegations->delegateType);
    }
    /* store per1 */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_delegations->per1);
    }
    /* store per2 */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_delegations->per2);
    }
    return rc;
}

/* TPM_Delegations_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_Delegations_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_Delegations_Delete(TPM_DELEGATIONS *tpm_delegations)
{
    printf(" TPM_Delegations_Delete:\n");
    if (tpm_delegations != NULL) {
	TPM_Delegations_Init(tpm_delegations);
    }
    return;
}

/* TPM_Delegations_Copy() copies the source to the destination
 */

void TPM_Delegations_Copy(TPM_DELEGATIONS *dest,
			  TPM_DELEGATIONS *src)
{
    dest->delegateType = src->delegateType;
    dest->per1 = src->per1;
    dest->per2 = src->per2;
    return;
}

/* TPM_Delegations_CheckPermissionDelegation() verifies that the new delegation bits do not grant
   more permissions then currently delegated.  Otherwise return error TPM_AUTHFAIL.

   An error occurs if a bit is set in newDelegations -> per and clear in currentDelegations -> per
*/

TPM_RESULT TPM_Delegations_CheckPermissionDelegation(TPM_DELEGATIONS *newDelegations,
						     TPM_DELEGATIONS *currentDelegations)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_Delegations_CheckPermissionDelegation:\n");
    /* check per1 */
    if (rc == 0) {
	if (newDelegations->per1 & ~currentDelegations->per1) {
	    printf("TPM_Delegations_CheckPermissionDelegation: Error, "
		   "new per1 %08x current per1 %08x\n",
		   newDelegations->per1, currentDelegations->per1);
	    rc = TPM_AUTHFAIL;
	}
    }
    /* check per2 */
    if (rc == 0) {
	if (newDelegations->per2 & ~currentDelegations->per2) {
	    printf("TPM_Delegations_CheckPermissionDelegation: Error, "
		   "new per1 %08x current per1 %08x\n",
		   newDelegations->per1, currentDelegations->per1);
	    rc = TPM_AUTHFAIL;
	}
    }
    return rc;
}

/* TPM_Delegations_CheckPermission() verifies that the 'ordinal' has been delegated for execution
   based on the TPM_DELEGATE_PUBLIC.

   It verifies that the TPM_DELEGATIONS is appropriate for the entityType.  Currently, only key or
   owner authorization can be delegated.

   It verifies that the TPM_DELEGATE_PUBLIC PCR's allow the delegation.
*/

TPM_RESULT TPM_Delegations_CheckPermission(tpm_state_t *tpm_state,
					   TPM_DELEGATE_PUBLIC *delegatePublic,
					   TPM_ENT_TYPE entityType,		/* required */
					   TPM_COMMAND_CODE ordinal)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_Delegations_CheckPermission: ordinal %08x\n", ordinal);
    if (rc == 0) {
	switch (entityType) {
	  case TPM_ET_KEYHANDLE:
	    rc = TPM_Delegations_CheckKeyPermission(&(delegatePublic->permissions), ordinal);
	    break;
	  case TPM_ET_OWNER:
	    rc = TPM_Delegations_CheckOwnerPermission(&(delegatePublic->permissions), ordinal);
	    break;
	  default:
	    printf("TPM_Delegations_CheckPermission: Error, "
		   "DSAP session does not support entity type %02x\n",
		   entityType);
	    rc = TPM_AUTHFAIL;
	    break;
	}
    }
    /* check that the TPM_DELEGATE_PUBLIC PCR's allow the delegation */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_CheckDigest(&(delegatePublic->pcrInfo),
					  tpm_state->tpm_stclear_data.PCRS,
					  tpm_state->tpm_stany_flags.localityModifier);
    }
    return rc;
}

/* TPM_Delegations_CheckOwnerPermission() verifies that the 'ordinal' has been delegated for
   execution based on the TPM_DELEGATIONS.
*/

TPM_RESULT TPM_Delegations_CheckOwnerPermission(TPM_DELEGATIONS *tpm_delegations,
						TPM_COMMAND_CODE ordinal)
{
    TPM_RESULT	rc = 0;
    uint16_t	ownerPermissionBlock;		/* 0:unused, 1:per1 2:per2 */
    uint32_t	ownerPermissionPosition;	/* owner permission bit position */

    printf(" TPM_Delegations_CheckOwnerPermission: ordinal %08x\n", ordinal);
    /* check that the TPM_DELEGATIONS structure is the correct type */
    if (rc == 0) {
	if (tpm_delegations->delegateType != TPM_DEL_OWNER_BITS) {
	    printf("TPM_Delegations_CheckOwnerPermission: Error,"
		   "Ordinal requires owner auth but delegateType is %08x\n",
		   tpm_delegations->delegateType);
	    rc = TPM_AUTHFAIL;
	}
    }
    /* get the block and position in the block from the ordinals table */
    if (rc == 0) {
	rc = TPM_OrdinalTable_GetOwnerPermission(&ownerPermissionBlock,
						 &ownerPermissionPosition,
						 ordinal);
    }
    /* check that the permission bit is set in the TPM_DELEGATIONS bit map */
    if (rc == 0) {
	printf("  TPM_Delegations_CheckOwnerPermission: block %u position %u\n",
	       ownerPermissionBlock, ownerPermissionPosition);
	switch (ownerPermissionBlock) {
	  case 1:	/* per1 */
	    if (!(tpm_delegations->per1 & (1 << ownerPermissionPosition))) {
		printf("TPM_Delegations_CheckOwnerPermission: Error, per1 %08x\n",
		       tpm_delegations->per1);
		rc = TPM_AUTHFAIL;
	    }
	    break;
	  case 2:	/* per2 */
	    if (!(tpm_delegations->per2 & (1 << ownerPermissionPosition))) {
		printf("TPM_Delegations_CheckOwnerPermission: Error, per2 %08x\n",
		       tpm_delegations->per2);
		rc = TPM_AUTHFAIL;
	    }
	    break;
	  default:
	    printf("TPM_Delegations_CheckOwnerPermission: Error, block not 1 or 2\n");
	    rc = TPM_AUTHFAIL;
	    break;
	}
    }
    return rc;
}

/* TPM_Delegations_CheckKeyPermission() verifies that the 'ordinal' has been delegated for
   execution based on the TPM_DELEGATIONS.
*/

TPM_RESULT TPM_Delegations_CheckKeyPermission(TPM_DELEGATIONS *tpm_delegations,
					      TPM_COMMAND_CODE ordinal)
{
    TPM_RESULT	rc = 0;
    uint16_t	keyPermissionBlock;		/* 0:unused, 1:per1 2:per2 */
    uint32_t	keyPermissionPosition;		/* key permission bit position */

    printf(" TPM_Delegations_CheckKeyPermission: ordinal %08x\n", ordinal);
    /* check that the TPM_DELEGATIONS structure is the correct type */
    if (rc == 0) {
	if (tpm_delegations->delegateType != TPM_DEL_KEY_BITS) {
	    printf("TPM_Delegations_CheckKeyPermission: Error,"
		   "Ordinal requires key auth but delegateType is %08x\n",
		   tpm_delegations->delegateType);
	    rc = TPM_AUTHFAIL;
	}
    }
    /* get the block and position in the block from the ordinals table */
    if (rc == 0) {
	rc = TPM_OrdinalTable_GetKeyPermission(&keyPermissionBlock,
					       &keyPermissionPosition,
					       ordinal);
    }
    /* check that the permission bit is set in the TPM_DELEGATIONS bit map */
    if (rc == 0) {
	printf("  TPM_Delegations_CheckKeyPermission: block %u position %u\n",
	       keyPermissionBlock, keyPermissionPosition);
	switch (keyPermissionBlock) {
	  case 1:	/* per1 */
	    if (!(tpm_delegations->per1 & (1 << keyPermissionPosition))) {
		printf("TPM_Delegations_CheckKeyPermission: Error, per1 %08x\n",
		       tpm_delegations->per1);
		rc = TPM_AUTHFAIL;
	    }
	    break;
	  case 2:	/* per2 */
	    if (!(tpm_delegations->per2 & (1 << keyPermissionPosition))) {
		printf("TPM_Delegations_CheckKeyPermission: Error, per2 %08x\n",
		       tpm_delegations->per2);
		rc = TPM_AUTHFAIL;
	    }
	    break;
	  default:
	    printf("TPM_Delegations_CheckKeyPermission: Error, block not 1 or 2\n");
	    rc = TPM_AUTHFAIL;
	    break;
	}
    }
    return rc;
}

/*
  TPM_DELEGATE_OWNER_BLOB
*/

/* TPM_DelegateOwnerBlob_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DelegateOwnerBlob_Init(TPM_DELEGATE_OWNER_BLOB *tpm_delegate_owner_blob)
{
    printf(" TPM_DelegateOwnerBlob_Init:\n");
    TPM_DelegatePublic_Init(&(tpm_delegate_owner_blob->pub));
    TPM_Digest_Init(tpm_delegate_owner_blob->integrityDigest);
    TPM_SizedBuffer_Init(&(tpm_delegate_owner_blob->additionalArea));
    TPM_SizedBuffer_Init(&(tpm_delegate_owner_blob->sensitiveArea));
    return;
}

/* TPM_DelegateOwnerBlob_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_DelegateOwnerBlob_Init()
   After use, call TPM_DelegateOwnerBlob_Delete() to free memory
*/

TPM_RESULT TPM_DelegateOwnerBlob_Load(TPM_DELEGATE_OWNER_BLOB *tpm_delegate_owner_blob,
				      unsigned char **stream,
				      uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DelegateOwnerBlob_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_DELEGATE_OWNER_BLOB, stream, stream_size);
    }
    /* load pub */
    if (rc == 0) {
	rc = TPM_DelegatePublic_Load(&(tpm_delegate_owner_blob->pub), stream, stream_size);
    }
    /* check that permissions are owner */
    if (rc == 0) {
	if (tpm_delegate_owner_blob->pub.permissions.delegateType != TPM_DEL_OWNER_BITS) {
	    printf("TPM_DelegateOwnerBlob_Load: Error, delegateType expected %08x found %08x\n",
		   TPM_DEL_OWNER_BITS, tpm_delegate_owner_blob->pub.permissions.delegateType);
	    rc = TPM_INVALID_STRUCTURE;
	}
    }
    /* load integrityDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_delegate_owner_blob->integrityDigest, stream, stream_size);
    }
    /* load additionalArea */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_delegate_owner_blob->additionalArea), stream, stream_size);
    }
    /* load sensitiveArea */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_delegate_owner_blob->sensitiveArea), stream, stream_size);
    }
    return rc;
}

/* TPM_DelegateOwnerBlob_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DelegateOwnerBlob_Store(TPM_STORE_BUFFER *sbuffer,
				       const TPM_DELEGATE_OWNER_BLOB *tpm_delegate_owner_blob)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DelegateOwnerBlob_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_DELEGATE_OWNER_BLOB);
    }
    /* store pub */
    if (rc == 0) {
	rc = TPM_DelegatePublic_Store(sbuffer, &(tpm_delegate_owner_blob->pub));
    }
    /* store integrityDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_delegate_owner_blob->integrityDigest);
    }
    /* store additionalArea */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_delegate_owner_blob->additionalArea));
    }
    /* store sensitiveArea */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_delegate_owner_blob->sensitiveArea));
    }
    return rc;
}

/* TPM_DelegateOwnerBlob_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DelegateOwnerBlob_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_DelegateOwnerBlob_Delete(TPM_DELEGATE_OWNER_BLOB *tpm_delegate_owner_blob)
{
    printf(" TPM_DelegateOwnerBlob_Delete:\n");
    if (tpm_delegate_owner_blob != NULL) {
	TPM_DelegatePublic_Delete(&(tpm_delegate_owner_blob->pub));
	TPM_SizedBuffer_Delete(&(tpm_delegate_owner_blob->additionalArea));
	TPM_SizedBuffer_Delete(&(tpm_delegate_owner_blob->sensitiveArea));
	TPM_DelegateOwnerBlob_Init(tpm_delegate_owner_blob);
    }
    return;
}

/*
  TPM_DELEGATE_KEY_BLOB
*/

/* TPM_DelegateKeyBlob_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DelegateKeyBlob_Init(TPM_DELEGATE_KEY_BLOB *tpm_delegate_key_blob)
{
    printf(" TPM_DelegateKeyBlob_Init:\n");
    TPM_DelegatePublic_Init(&(tpm_delegate_key_blob->pub));
    TPM_Digest_Init(tpm_delegate_key_blob->integrityDigest);
    TPM_Digest_Init(tpm_delegate_key_blob->pubKeyDigest);
    TPM_SizedBuffer_Init(&(tpm_delegate_key_blob->additionalArea));
    TPM_SizedBuffer_Init(&(tpm_delegate_key_blob->sensitiveArea));
    return;
}

/* TPM_DelegateKeyBlob_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_DelegateKeyBlob_Init()
   After use, call TPM_DelegateKeyBlob_Delete() to free memory
*/

TPM_RESULT TPM_DelegateKeyBlob_Load(TPM_DELEGATE_KEY_BLOB *tpm_delegate_key_blob,
				    unsigned char **stream,
				    uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DelegateKeyBlob_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_DELG_KEY_BLOB, stream, stream_size);
    }
    /* load pub */
    if (rc == 0) {
	rc = TPM_DelegatePublic_Load(&(tpm_delegate_key_blob->pub), stream, stream_size);
    }
    /* check that permissions are key */
    if (rc == 0) {
	if (tpm_delegate_key_blob->pub.permissions.delegateType != TPM_DEL_KEY_BITS) {
	    printf("TPM_DelegateKeyBlob_Load: Error, delegateType expected %08x found %08x\n",
		   TPM_DEL_KEY_BITS, tpm_delegate_key_blob->pub.permissions.delegateType);
	    rc = TPM_INVALID_STRUCTURE;
	}
    }
    /* load integrityDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_delegate_key_blob->integrityDigest, stream, stream_size);
    }
    /* load pubKeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_delegate_key_blob->pubKeyDigest, stream, stream_size);
    }
    /* load additionalArea */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_delegate_key_blob->additionalArea), stream, stream_size);
    }
    /* load sensitiveArea */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Load(&(tpm_delegate_key_blob->sensitiveArea), stream, stream_size);
    }
    return rc;
}

/* TPM_DelegateKeyBlob_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DelegateKeyBlob_Store(TPM_STORE_BUFFER *sbuffer,
				     const TPM_DELEGATE_KEY_BLOB *tpm_delegate_key_blob)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DelegateKeyBlob_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_DELG_KEY_BLOB);
    }
    /* store pub */
    if (rc == 0) {
	rc = TPM_DelegatePublic_Store(sbuffer, &(tpm_delegate_key_blob->pub));
    }
    /* store integrityDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_delegate_key_blob->integrityDigest);
    }
    /* store pubKeyDigest */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_delegate_key_blob->pubKeyDigest);
    }
    /* store additionalArea */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_delegate_key_blob->additionalArea));
    }
    /* store sensitiveArea */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_delegate_key_blob->sensitiveArea));
    }
    return rc;
}

/* TPM_DelegateKeyBlob_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DelegateKeyBlob_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_DelegateKeyBlob_Delete(TPM_DELEGATE_KEY_BLOB *tpm_delegate_key_blob)
{
    printf(" TPM_DelegateKeyBlob_Delete:\n");
    if (tpm_delegate_key_blob != NULL) {
	TPM_DelegatePublic_Delete(&(tpm_delegate_key_blob->pub));
	TPM_SizedBuffer_Delete(&(tpm_delegate_key_blob->additionalArea));
	TPM_SizedBuffer_Delete(&(tpm_delegate_key_blob->sensitiveArea));
	TPM_DelegateKeyBlob_Init(tpm_delegate_key_blob);
    }
    return;
}

/*
  TPM_FAMILY_TABLE
*/

/* TPM_FamilyTable_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_FamilyTable_Init(TPM_FAMILY_TABLE *tpm_family_table)
{
    size_t i;

    printf(" TPM_FamilyTable_Init: Qty %u\n", TPM_NUM_FAMILY_TABLE_ENTRY_MIN);
    for (i = 0 ; i < TPM_NUM_FAMILY_TABLE_ENTRY_MIN ; i++) {
	TPM_FamilyTableEntry_Init(&(tpm_family_table->famTableRow[i]));
    }
    return;
}

/* TPM_FamilyTable_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_FamilyTable_Init()
   After use, call TPM_FamilyTable_Delete() to free memory
*/

TPM_RESULT TPM_FamilyTable_Load(TPM_FAMILY_TABLE *tpm_family_table,
				unsigned char **stream,
				uint32_t *stream_size)
{
    TPM_RESULT	rc = 0;
    size_t	i;

    printf(" TPM_FamilyTable_Load: Qty %u\n", TPM_NUM_FAMILY_TABLE_ENTRY_MIN);
    for (i = 0 ; (rc == 0) && (i < TPM_NUM_FAMILY_TABLE_ENTRY_MIN) ; i++) {
	rc = TPM_FamilyTableEntry_Load(&(tpm_family_table->famTableRow[i]),
				       stream,
				       stream_size);
    }
    return rc;
}

/* TPM_FamilyTable_Store()
   
   If store_tag is TRUE, the TPM_FAMILY_TABLE_ENTRY tag is stored.

   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_FamilyTable_Store(TPM_STORE_BUFFER *sbuffer,
				 const TPM_FAMILY_TABLE *tpm_family_table,
				 TPM_BOOL store_tag)
{
    TPM_RESULT		rc = 0;
    size_t i;

    printf(" TPM_FamilyTable_Store: Qty %u\n", TPM_NUM_FAMILY_TABLE_ENTRY_MIN);
    for (i = 0 ; (rc == 0) && (i < TPM_NUM_FAMILY_TABLE_ENTRY_MIN) ; i++) {
	rc = TPM_FamilyTableEntry_Store(sbuffer,
					&(tpm_family_table->famTableRow[i]), store_tag);
    }
    return rc;
}

/* TPM_FamilyTable_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_FamilyTable_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_FamilyTable_Delete(TPM_FAMILY_TABLE *tpm_family_table)
{
    size_t i;

    printf(" TPM_FamilyTable_Delete: Qty %u\n", TPM_NUM_FAMILY_TABLE_ENTRY_MIN);
    if (tpm_family_table != NULL) {
	for (i = 0 ; i < TPM_NUM_FAMILY_TABLE_ENTRY_MIN ; i++) {
	    TPM_FamilyTableEntry_Delete(&(tpm_family_table->famTableRow[i]));
	}
	TPM_FamilyTable_Init(tpm_family_table);
    }
    return;
}

/* TPM_FamilyTable_GetEntry() searches all entries for the entry matching the familyID, and returns
   the TPM_FAMILY_TABLE_ENTRY associated with the familyID.

   Returns
	0 for success
	TPM_BADINDEX if the familyID is not found
*/

TPM_RESULT TPM_FamilyTable_GetEntry(TPM_FAMILY_TABLE_ENTRY **tpm_family_table_entry, /* output */
				    TPM_FAMILY_TABLE *tpm_family_table,
				    TPM_FAMILY_ID familyID)
{
    TPM_RESULT	rc = 0;
    size_t	i;
    TPM_BOOL	found;

    printf(" TPM_FamilyTable_GetEntry: familyID %08x\n", familyID);
    for (i = 0, found = FALSE ; (i < TPM_NUM_FAMILY_TABLE_ENTRY_MIN) && !found ; i++) {
	if (tpm_family_table->famTableRow[i].valid &&
	    (tpm_family_table->famTableRow[i].familyID == familyID)) {	/* found */
	    found = TRUE;
	    *tpm_family_table_entry = &(tpm_family_table->famTableRow[i]);
	}
    }
    if (!found) {
	printf("TPM_FamilyTable_GetEntry: Error, familyID %08x not found\n", familyID);
	rc = TPM_BADINDEX;
    }
    return rc;
}

/* TPM_FamilyTable_GetEnabledEntry() searches all entries for the entry matching the familyID, and
   returns the TPM_FAMILY_TABLE_ENTRY associated with the familyID.

   Similar to TPM_FamilyTable_GetEntry() but returns an error if the entry is disabled.
  
   Returns
	0 for success
	TPM_BADINDEX if the familyID is not found
	TPM_DISABLED_CMD if the TPM_FAMILY_TABLE_ENTRY -> TPM_FAMFLAG_ENABLED is FALSE 
*/

TPM_RESULT TPM_FamilyTable_GetEnabledEntry(TPM_FAMILY_TABLE_ENTRY **tpm_family_table_entry,
					   TPM_FAMILY_TABLE *tpm_family_table,
					   TPM_FAMILY_ID familyID)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_FamilyTable_GetEnabledEntry: familyID %08x\n", familyID);
    if (rc == 0) {
	rc = TPM_FamilyTable_GetEntry(tpm_family_table_entry,
				      tpm_family_table,
				      familyID);
    }
    if (rc == 0) {
	if (!((*tpm_family_table_entry)->flags & TPM_FAMFLAG_ENABLED)) {
	    printf("TPM_FamilyTable_GetEnabledEntry: Error, family %08x disabled\n", familyID);
	    rc = TPM_DISABLED_CMD;
	}
    }
    return rc;
}

/* TPM_FamilyTable_IsSpace() returns success if an entry is available, an error if not.

   If success, 'family_table_entry' holds the first free family table row.
*/

TPM_RESULT TPM_FamilyTable_IsSpace(TPM_FAMILY_TABLE_ENTRY **tpm_family_table_entry, /* output */
				   TPM_FAMILY_TABLE *tpm_family_table)
{
    size_t	i;
    TPM_BOOL	isSpace;
    TPM_RESULT	rc = 0;
    
    
    printf(" TPM_FamilyTable_IsSpace:\n");
    for (i = 0, isSpace = FALSE ; i < TPM_NUM_FAMILY_TABLE_ENTRY_MIN; i++) {
	*tpm_family_table_entry = &(tpm_family_table->famTableRow[i]);
	if (!((*tpm_family_table_entry)->valid)) {
	    printf("  TPM_FamilyTable_IsSpace: Found space at %lu\n", (unsigned long)i);
	    isSpace = TRUE;
	    break;
	}	    
    }
    if (!isSpace) {
	printf("  TPM_FamilyTable_IsSpace: Error, no space found\n");
	rc = TPM_RESOURCES;
    }
    return rc;
}

/* TPM_FamilyTable_StoreValid() stores only the valid (occupied) entries

   If store_tag is TRUE, the TPM_FAMILY_TABLE_ENTRY tag is stored.
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_FamilyTable_StoreValid(TPM_STORE_BUFFER *sbuffer,
				      const TPM_FAMILY_TABLE *tpm_family_table,
				      TPM_BOOL store_tag)
{
    TPM_RESULT	rc = 0;
    size_t	i;

    printf(" TPM_FamilyTable_StoreValid: \n");
    for (i = 0 ; (rc == 0) && (i < TPM_NUM_FAMILY_TABLE_ENTRY_MIN) ; i++) {
	/* store only the valid rows */
	if (tpm_family_table->famTableRow[i].valid) {
	    /* store only the publicly visible members */
	    printf("  TPM_FamilyTable_StoreValid: Entry %lu is valid\n", (unsigned long)i);
	    printf("  TPM_FamilyTable_StoreValid: Entry family ID is %08x\n",
		   tpm_family_table->famTableRow[i].familyID);
	    rc = TPM_FamilyTableEntry_StorePublic(sbuffer,
						  &(tpm_family_table->famTableRow[i]), store_tag);
	}
    }
    return rc;
}

/*
  TPM_FAMILY_TABLE_ENTRY
*/

/* TPM_FamilyTableEntry_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_FamilyTableEntry_Init(TPM_FAMILY_TABLE_ENTRY *tpm_family_table_entry)
{
    printf(" TPM_FamilyTableEntry_Init:\n");
    tpm_family_table_entry->familyLabel = 0;
    tpm_family_table_entry->familyID = 0;
    tpm_family_table_entry->verificationCount = 0;
    tpm_family_table_entry->flags = 0;
    tpm_family_table_entry->valid = FALSE;
    return;
}

/* TPM_FamilyTableEntry_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_FamilyTableEntry_Init()
   After use, call TPM_FamilyTableEntry_Delete() to free memory
*/

TPM_RESULT TPM_FamilyTableEntry_Load(TPM_FAMILY_TABLE_ENTRY *tpm_family_table_entry,
				     unsigned char **stream,
				     uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_FamilyTableEntry_Load:\n");
    /* load tag */
    /* the tag is not serialized when storing TPM_PERMANENT_DATA, to save NV space */
    /* load familyLabel */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_family_table_entry->familyLabel), stream, stream_size);
    }
    /* load familyID */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_family_table_entry->familyID), stream, stream_size);
    }
    /* load verificationCount */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_family_table_entry->verificationCount), stream, stream_size);
    }
    /* load flags */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_family_table_entry->flags), stream, stream_size);
    }
    /* load valid */
    if (rc == 0) {
	rc = TPM_LoadBool(&(tpm_family_table_entry->valid), stream, stream_size);
    }
    if (rc == 0) {
	printf("  TPM_FamilyTableEntry_Load: label %02x familyID %08x valid %u\n",
	       tpm_family_table_entry->familyLabel,
	       tpm_family_table_entry->familyID,
	       tpm_family_table_entry->valid);
    }
    return rc;
}

/* TPM_FamilyTableEntry_Store() stores all members of the structure

   If store_tag is TRUE, the TPM_FAMILY_TABLE_ENTRY tag is stored.
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_FamilyTableEntry_Store(TPM_STORE_BUFFER *sbuffer,
				      const TPM_FAMILY_TABLE_ENTRY *tpm_family_table_entry,
				      TPM_BOOL store_tag)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_FamilyTableEntry_Store:\n");
    /* store public, visible members */
    if (rc == 0) {
	rc = TPM_FamilyTableEntry_StorePublic(sbuffer, tpm_family_table_entry, store_tag); 
    }
    /* store valid */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_family_table_entry->valid),
				sizeof(TPM_BOOL));
    }
    return rc;
}

/* TPM_FamilyTableEntry_StorePublic() stores only the public, visible members of the structure
   
   If store_tag is TRUE, the TPM_FAMILY_TABLE_ENTRY tag is stored.

   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_FamilyTableEntry_StorePublic(TPM_STORE_BUFFER *sbuffer,
					    const TPM_FAMILY_TABLE_ENTRY *tpm_family_table_entry,
					    TPM_BOOL store_tag)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_FamilyTableEntry_StorePublic:\n");
    /* store tag */
    if ((rc == 0) && (store_tag)) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_FAMILY_TABLE_ENTRY); 
    }
    /* store familyLabel */
    if (rc == 0) {
	TPM_Sbuffer_Append(sbuffer, &(tpm_family_table_entry->familyLabel),
			   sizeof(TPM_FAMILY_LABEL));
    }
    /* store familyID */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_family_table_entry->familyID); 
    }					  
    /* store verificationCount */
    if (rc == 0) {			  
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_family_table_entry->verificationCount); 
    }					  
    /* store flags */
    if (rc == 0) {			  
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_family_table_entry->flags); 
    }
    return rc;
}

/* TPM_FamilyTableEntry_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_FamilyTableEntry_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_FamilyTableEntry_Delete(TPM_FAMILY_TABLE_ENTRY *tpm_family_table_entry)
{
    printf(" TPM_FamilyTableEntry_Delete:\n");
    if (tpm_family_table_entry != NULL) {
	TPM_FamilyTableEntry_Init(tpm_family_table_entry);
    }
    return;
}

/*
  TPM_DELEGATE_TABLE
*/

/* TPM_DelegateTable_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DelegateTable_Init(TPM_DELEGATE_TABLE *tpm_delegate_table)
{
    size_t i;

    printf(" TPM_DelegateTable_Init: Qty %u\n", TPM_NUM_DELEGATE_TABLE_ENTRY_MIN);
    for (i = 0 ; i < TPM_NUM_DELEGATE_TABLE_ENTRY_MIN ; i++) {
	TPM_DelegateTableRow_Init(&(tpm_delegate_table->delRow[i]));
    }
    return;
}

/* TPM_DelegateTable_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_DelegateTable_Init()
   After use, call TPM_DelegateTable_Delete() to free memory
*/

TPM_RESULT TPM_DelegateTable_Load(TPM_DELEGATE_TABLE *tpm_delegate_table,
				  unsigned char **stream,
				  uint32_t *stream_size)
{
    TPM_RESULT	rc = 0;
    size_t	i;

    printf(" TPM_DelegateTable_Load: Qty %u\n", TPM_NUM_DELEGATE_TABLE_ENTRY_MIN);
    for (i = 0 ; (rc == 0) && (i < TPM_NUM_DELEGATE_TABLE_ENTRY_MIN)  ; i++) {
	rc = TPM_DelegateTableRow_Load(&(tpm_delegate_table->delRow[i]),
					 stream,
					 stream_size);
    }
    return rc;
}

/* TPM_DelegateTable_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DelegateTable_Store(TPM_STORE_BUFFER *sbuffer,
				   const TPM_DELEGATE_TABLE *tpm_delegate_table)
{
    TPM_RESULT		rc = 0;
    size_t i;

    printf(" TPM_DelegateTable_Store: Qty %u\n", TPM_NUM_DELEGATE_TABLE_ENTRY_MIN);
    for (i = 0 ; (rc == 0) && (i < TPM_NUM_DELEGATE_TABLE_ENTRY_MIN) ; i++) {
	rc = TPM_DelegateTableRow_Store(sbuffer, &(tpm_delegate_table->delRow[i]));
    }
    return rc;
}

/* TPM_DelegateTable_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DelegateTable_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_DelegateTable_Delete(TPM_DELEGATE_TABLE *tpm_delegate_table)
{
    size_t i;

    printf(" TPM_DelegateTable_Delete: Qty %u\n", TPM_NUM_DELEGATE_TABLE_ENTRY_MIN);
    if (tpm_delegate_table != NULL) {
	for (i = 0 ; i < TPM_NUM_DELEGATE_TABLE_ENTRY_MIN ; i++) {
	    TPM_DelegateTableRow_Delete(&(tpm_delegate_table->delRow[i]));
	}
	TPM_DelegateTable_Init(tpm_delegate_table);
    }
    return;
}

/* TPM_DelegateTable_StoreValid() store only the valid (occupied) entries.  Each entry is prepended
   with it's index.
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DelegateTable_StoreValid(TPM_STORE_BUFFER *sbuffer,
					const TPM_DELEGATE_TABLE *tpm_delegate_table)
{
    TPM_RESULT	rc = 0;
    uint32_t	i;

    printf(" TPM_DelegateTable_StoreValid:\n");
    for (i = 0 ; (rc == 0) && (i < TPM_NUM_DELEGATE_TABLE_ENTRY_MIN) ; i++) {
	/* store only the valid rows */
	if (tpm_delegate_table->delRow[i].valid) {
	    /* a. Write the TPM_DELEGATE_INDEX to delegateTable */
	    printf("  TPM_DelegateTable_StoreValid: Entry %u is valid\n", i);
	    printf("  TPM_DelegateTable_StoreValid: Entry family ID is %08x\n",
		   tpm_delegate_table->delRow[i].pub.familyID);
	    if (rc == 0) {
		rc = TPM_Sbuffer_Append32(sbuffer, i);
	    }
	    /* b. Copy the TPM_DELEGATE_PUBLIC to delegateTable */
	    if (rc == 0) {
		rc = TPM_DelegatePublic_Store(sbuffer, &(tpm_delegate_table->delRow[i].pub));
	    }
	}
    }
    return rc;
}

/* TPM_DelegateTable_GetRow() maps 'rowIndex' to a TPM_DELEGATE_TABLE_ROW in the delegate table.

   The row may not have valid data.
 */

TPM_RESULT TPM_DelegateTable_GetRow(TPM_DELEGATE_TABLE_ROW **delegateTableRow,
				    TPM_DELEGATE_TABLE *tpm_delegate_table,
				    uint32_t rowIndex)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_DelegateTable_GetRow: index %u\n", rowIndex);
    if (rc == 0) {
	if (rowIndex >= TPM_NUM_DELEGATE_TABLE_ENTRY_MIN) {
	    printf("TPM_DelegateTable_GetRow: index %u out of range\n", rowIndex);
	    rc = TPM_BADINDEX;
	}
    }
    if (rc == 0) {
	*delegateTableRow = &(tpm_delegate_table->delRow[rowIndex]);
    }
    return rc;
}

/* TPM_DelegateTable_GetValidRow() maps 'rowIndex' to a TPM_DELEGATE_TABLE_ROW in the delegate
   table.

   The row must have valid data.
 */

TPM_RESULT TPM_DelegateTable_GetValidRow(TPM_DELEGATE_TABLE_ROW **delegateTableRow,
					 TPM_DELEGATE_TABLE *tpm_delegate_table,
					 uint32_t rowIndex)
{
    TPM_RESULT	rc = 0;

    if (rc == 0) {
	rc = TPM_DelegateTable_GetRow(delegateTableRow,
				      tpm_delegate_table,
				      rowIndex);
    }
    if (rc == 0) {
	*delegateTableRow = &(tpm_delegate_table->delRow[rowIndex]);
	if (!(*delegateTableRow)->valid) {
	    printf("TPM_DelegateTable_GetValidRow: index %u invalid\n", rowIndex);
	    rc = TPM_BADINDEX;
	}
    }
    return rc;
}

/*
  TPM_DELEGATE_TABLE_ROW
*/

/* TPM_DelegateTableRow_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_DelegateTableRow_Init(TPM_DELEGATE_TABLE_ROW *tpm_delegate_table_row)
{
    printf(" TPM_DelegateTableRow_Init:\n");
    TPM_DelegatePublic_Init(&(tpm_delegate_table_row->pub));
    TPM_Secret_Init(tpm_delegate_table_row->authValue);
    tpm_delegate_table_row->valid = FALSE;
    return;
}

/* TPM_DelegateTableRow_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_DelegateTableRow_Init()
   After use, call TPM_DelegateTableRow_Delete() to free memory
*/

TPM_RESULT TPM_DelegateTableRow_Load(TPM_DELEGATE_TABLE_ROW *tpm_delegate_table_row,
				     unsigned char **stream,
				     uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DelegateTableRow_Load:\n");
    /* check the tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_DELEGATE_TABLE_ROW, stream, stream_size);
    }
    /* load pub */
    if (rc == 0) {
	rc = TPM_DelegatePublic_Load(&(tpm_delegate_table_row->pub), stream, stream_size);
    }
    /* load authValue */
    if (rc == 0) {
	rc = TPM_Secret_Load(tpm_delegate_table_row->authValue, stream, stream_size);
    }
    /* load valid */
    if (rc == 0) {
	rc = TPM_LoadBool(&(tpm_delegate_table_row->valid), stream, stream_size);
    }
    return rc;
}

/* TPM_DelegateTableRow_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_DelegateTableRow_Store(TPM_STORE_BUFFER *sbuffer,
				      const TPM_DELEGATE_TABLE_ROW *tpm_delegate_table_row)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_DelegateTableRow_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_DELEGATE_TABLE_ROW);
    }
    /* store pub */
    if (rc == 0) {
	rc = TPM_DelegatePublic_Store(sbuffer, &(tpm_delegate_table_row->pub)); 
    }
    /* store authValue */
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_delegate_table_row->authValue);
    }
    /* store valid */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_delegate_table_row->valid), sizeof(TPM_BOOL));
    }
    return rc;
}

/* TPM_DelegateTableRow_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_DelegateTableRow_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_DelegateTableRow_Delete(TPM_DELEGATE_TABLE_ROW *tpm_delegate_table_row)
{
    printf(" TPM_DelegateTableRow_Delete:\n");
    if (tpm_delegate_table_row != NULL) {
	TPM_DelegatePublic_Delete(&(tpm_delegate_table_row->pub));
	TPM_DelegateTableRow_Init(tpm_delegate_table_row);
    }
    return;
}

/*
  Processing Functions
*/

/* 19.1 TPM_Delegate_Manage rev 115
   
   TPM_Delegate_Manage is the fundamental process for managing the Family tables, including
   enabling/disabling Delegation for a selected Family. Normally TPM_Delegate_Manage must be
   executed at least once (to create Family tables for a particular family) before any other type of
   Delegation command in that family can succeed.

   Delegate_Manage is authorized by the TPM Owner if an Owner is installed, because changing a table
   is a privileged Owner operation. If no Owner is installed, Delegate_Manage requires no privilege
   to execute. This does not disenfranchise an Owner, since there is no Owner, and simplifies
   loading of tables during platform manufacture or on first-boot. Burn-out of TPM non-volatile
   storage by inappropriate use is mitigated by the TPM's normal limits on NV-writes in the absence
   of an Owner. Tables can be locked after loading, to prevent subsequent tampering, and only
   unlocked by the Owner, his delegate, or the act of removing the Owner (even if there is no
   Owner).

   TPM_Delegate_Manage command is customized by opcode:

   (1) TPM_FAMILY_ENABLE enables/disables use of a family and all the rows of the delegate table
   belonging to that family,

   (2) TPM_FAMILY_ADMIN can be used to prevent further management of the Tables until an Owner is
   installed, or until the Owner is removed from the TPM. (Note that the Physical Presence command
   TPM_ForceClear always enables further management, even if TPM_ForceClear is used when no Owner is
   installed.)

   (3) TPM_FAMILY_CREATE creates a new family.

   (4) TPM_FAMILY_INVALIDATE invalidates an existing family.
*/

TPM_RESULT TPM_Process_DelegateManage(tpm_state_t *tpm_state,
				      TPM_STORE_BUFFER *response,
				      TPM_TAG tag,
				      uint32_t paramSize,
				      TPM_COMMAND_CODE ordinal,
				      unsigned char *command,
				      TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_FAMILY_ID		familyID;	/* The familyID that is to be managed */
    TPM_FAMILY_OPERATION	opCode = 0;	/* Operation to be performed by this command. */
    TPM_SIZED_BUFFER		opData;		/* Data necessary to implement opCode */
    TPM_AUTHHANDLE		authHandle;	/* The authorization session handle used for owner
						   authentication. */
    TPM_NONCE			nonceOdd;	/* Nonce generated by system associated with
						   authHandle */
    TPM_BOOL		continueAuthSession = TRUE;	/* The continue use flag for the
						   authorization session handle */
    TPM_AUTHDATA		ownerAuth;	/* HMAC key: ownerAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey;
    TPM_SECRET			savedAuth;		/* saved copy for response */
    TPM_DELEGATE_PUBLIC		*delegatePublic;	/* from DSAP session */
    TPM_FAMILY_TABLE_ENTRY	*familyRow = NULL;	/* family table row containing familyID */
    uint32_t			nv1 = tpm_state->tpm_permanent_data.noOwnerNVWrite;
							/* temp for noOwnerNVWrite, initialize to
							   silence compiler */
    TPM_BOOL			nv1Incremented = FALSE;	/* flag that nv1 was incremented */
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back data */

    /* output parameters  */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_STORE_BUFFER		retData;	/* Returned data */

    printf("TPM_Process_DelegateManage: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&opData);		/* freed @1 */
    TPM_Sbuffer_Init(&retData);			/* freed @2 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get familyID parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&familyID, &command, &paramSize);
    }
    /* get opCode parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DelegateManage: familyID %08x\n", familyID);
	returnCode = TPM_Load32(&opCode, &command, &paramSize);
    }
    /* get opData parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DelegateManage: opCode %u\n", opCode);
	returnCode = TPM_SizedBuffer_Load(&opData, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALLOW_NO_OWNER);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag10(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_DelegateManage: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. If opCode != TPM_FAMILY_CREATE */
    /* a. Locate familyID in the TPM_FAMILY_TABLE and set familyRow to indicate row, return
       TPM_BADINDEX if not found */
    /* b. Set FR, a TPM_FAMILY_TABLE_ENTRY, to TPM_FAMILY_TABLE. famTableRow[familyRow] */
    if ((returnCode == TPM_SUCCESS) && (opCode != TPM_FAMILY_CREATE)) {
	printf("TPM_Process_DelegateManage: Not creating, get entry for familyID %08x\n",
	       familyID);
	returnCode = TPM_FamilyTable_GetEntry(&familyRow,
					      &(tpm_state->tpm_permanent_data.familyTable),
					      familyID);
    }
    /* 2. If tag = TPM_TAG_RQU_AUTH1_COMMAND */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	/* a. Validate the command and parameters using ownerAuth, return TPM_AUTHFAIL on error */
	returnCode =
	    TPM_AuthSessions_GetData(&auth_session_data,
				     &hmacKey,
				     tpm_state,
				     authHandle,
				     TPM_PID_NONE,
				     TPM_ET_OWNER,
				     ordinal,
				     NULL,
				     &(tpm_state->tpm_permanent_data.ownerAuth),/* OIAP */
				     tpm_state->tpm_permanent_data.ownerAuth);	/* OSAP */
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	/* make a copy of the HMAC key for the response, since it MAY be invalidated */
	TPM_Secret_Copy(savedAuth, *hmacKey);
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* owner HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					ownerAuth);		/* Authorization digest for input */
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	/* b. If the command is delegated (authHandle session type is TPM_PID_DSAP or through
	   ownerReference delegation) */
	if ((auth_session_data->protocolID == TPM_PID_DSAP) ||
	    (tpm_state->tpm_stclear_data.ownerReference != TPM_KH_OWNER)) {
	    /* i. If opCode = TPM_FAMILY_CREATE */
	    /* (1) The TPM MUST ignore familyID */
	    /* ii. Else */
	    if (opCode != TPM_FAMILY_CREATE) {
		/* get the TPM_DELEGATE_PUBLIC from the DSAP session */
		if (returnCode == TPM_SUCCESS) {
		    returnCode = TPM_AuthSessionData_GetDelegatePublic(&delegatePublic,
								       auth_session_data);
		}
		/* (1) Verify that the familyID associated with authHandle matches the familyID
		   parameter, return TPM_DELEGATE_FAMILY on error */
		if (returnCode == TPM_SUCCESS) {
		    if (delegatePublic->familyID != familyID) {
			printf("TPM_Process_DelegateManage: Error, familyID %08x should be %08x\n",
			       familyID, delegatePublic->familyID);
			returnCode = TPM_DELEGATE_FAMILY;
		    }
		}
	    }
	}
    }
    /* 3. Else */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_AUTH1_COMMAND)) {
	/* a. If TPM_PERMANENT_DATA -> ownerAuth is valid, return TPM_AUTHFAIL */
	if (tpm_state->tpm_permanent_data.ownerInstalled) {
	    printf("TPM_Process_DelegateManage: Error, owner installed but no authorization\n");
	    returnCode = TPM_AUTHFAIL ;
	}
    }
    /* b. If opCode != TPM_FAMILY_CREATE and FR -> flags -> TPM_DELEGATE_ADMIN_LOCK is TRUE, return
       TPM_DELEGATE_LOCK */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_AUTH1_COMMAND)) {
	if ((opCode != TPM_FAMILY_CREATE) && (familyRow->flags & TPM_DELEGATE_ADMIN_LOCK)) {
	    printf("TPM_Process_DelegateManage: Error, row locked\n");
	    returnCode = TPM_DELEGATE_LOCK;
	}
    }
    /* c. Validate max NV writes without an owner */
    if ((returnCode == TPM_SUCCESS) && (tag != TPM_TAG_RQU_AUTH1_COMMAND)) {
	/* i. Set NV1 to TPM_PERMANENT_DATA -> noOwnerNVWrite */
	nv1 = tpm_state->tpm_permanent_data.noOwnerNVWrite;
	/* ii. Increment NV1 by 1 */
	nv1++;
	/* iii. If NV1 > TPM_MAX_NV_WRITE_NOOWNER return TPM_MAXNVWRITES */
	if (nv1 > TPM_MAX_NV_WRITE_NOOWNER) {
	    printf("TPM_Process_DelegateManage: Error, max NV writes %d w/o owner reached\n",
		   tpm_state->tpm_permanent_data.noOwnerNVWrite);
	    returnCode = TPM_MAXNVWRITES;
	}
	if (returnCode == TPM_SUCCESS){
	    /* iv. Set TPM_PERMANENT_DATA -> noOwnerNVWrite to NV1 */
	    /* NOTE Don't update the noOwnerNVWrite value until determining that the write will be
	       performed */
	    nv1Incremented = TRUE;
	}
    }	
    /* 4. The TPM invalidates sessions */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DelegateManage: Invalidate sessions\n");
	/* a. MUST invalidate all DSAP sessions */
	/* b. MUST invalidate all OSAP sessions associated with the delegation table */
	/* d. MAY invalidate any other session */
	TPM_AuthSessions_TerminatexSAP(&continueAuthSession,
				       authHandle,
				       tpm_state->tpm_stclear_data.authSessions);
	/* c. MUST set TPM_STCLEAR_DATA -> ownerReference to TPM_KH_OWNER */
	tpm_state->tpm_stclear_data.ownerReference = TPM_KH_OWNER;
    }
    /*
      5. If opCode == TPM_FAMILY_CREATE
    */
    if ((returnCode == TPM_SUCCESS) && (opCode == TPM_FAMILY_CREATE)) {
	printf("TPM_Process_DelegateManage: Processing TPM_FAMILY_CREATE\n");
	/* a. Validate that sufficient space exists within the TPM to store an additional family and
	   map F2 to the newly allocated space. */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_FamilyTable_IsSpace(&familyRow,	/* output */
						 &(tpm_state->tpm_permanent_data.familyTable));
	}
	/* b. Validate that opData is a TPM_FAMILY_LABEL */
	if (returnCode == TPM_SUCCESS) {
	    /* i. If opDataSize != sizeof(TPM_FAMILY_LABEL) return TPM_BAD_PARAM_SIZE */
	    if (opData.size != sizeof(TPM_FAMILY_LABEL)) {
		printf("TPM_Process_DelegateManage: Error, invalid opDataSize %u\n", opData.size);
		returnCode = TPM_BAD_PARAM_SIZE;
	    }
	}
	/* c. Map F2 to a TPM_FAMILY_TABLE_ENTRY */
	/* NOTE Done by TPM_FamilyTable_IsSpace() */
	/* i. Set F2 -> tag to TPM_TAG_FAMILY_TABLE_ENTRY */
	/* NOTE Done by TPM_FamilyTableEntry_Init() */
	if (returnCode == TPM_SUCCESS) {
	    /* ii. Set F2 -> familyLabel to opData */
	    familyRow->familyLabel = *(opData.buffer);
	    /* d. Increment TPM_PERMANENT_DATA -> lastFamilyID by 1 */
	    tpm_state->tpm_permanent_data.lastFamilyID++;
	    /* must write TPM_PERMANENT_DATA back to NVRAM, set this flag after NVRAM is written */
	    writeAllNV = TRUE;
	    /* e. Set F2 -> familyID = TPM_PERMANENT_DATA -> lastFamilyID */
	    familyRow->familyID = tpm_state->tpm_permanent_data.lastFamilyID;
	    /* f. Set F2 -> verificationCount = 1 */
	    familyRow->verificationCount = 1;
	    /* g. Set F2 -> flags -> TPM_FAMFLAG_ENABLED to FALSE */
	    familyRow->flags &= ~TPM_FAMFLAG_ENABLED;
	    /* h. Set F2 -> flags -> TPM_DELEGATE_ADMIN_LOCK to FALSE */
	    familyRow->flags &= ~TPM_DELEGATE_ADMIN_LOCK;
	    /* i. Set retDataSize = 4 */
	    /* j. Set retData = F2 -> familyID */
	    printf("TPM_Process_DelegateManage: Created familyID %08x\n", familyRow->familyID);
	    familyRow->valid = TRUE;
	    returnCode = TPM_Sbuffer_Append32(&retData, familyRow->familyID);
	}
	/* k. Return TPM_SUCCESS */
    }
    /* 6. If authHandle is of type DSAP then continueAuthSession MUST set to FALSE */
    if ((returnCode == TPM_SUCCESS) && (opCode != TPM_FAMILY_CREATE) &&
	(tag == TPM_TAG_RQU_AUTH1_COMMAND)) {	/* only if auth-1 */

	if (auth_session_data->protocolID == TPM_PID_DSAP) {
	    continueAuthSession = FALSE;
	}
    }
    /* 7. If opCode == TPM_FAMILY_ADMIN */
    if ((returnCode == TPM_SUCCESS) && (opCode == TPM_FAMILY_ADMIN)) {
	printf("TPM_Process_DelegateManage: Processing TPM_FAMILY_ADMIN\n");
	/* a. Validate that opDataSize == 1, and that opData is a Boolean value. */
	if (returnCode == TPM_SUCCESS) {
	    if (opData.size != sizeof(TPM_BOOL)) {
		printf("TPM_Process_DelegateManage: Error, invalid opDataSize %u\n", opData.size);
		returnCode = TPM_BAD_PARAM_SIZE;
	    }
	}
	/* b. Set (FR -> flags -> TPM_DELEGATE_ADMIN_LOCK) = opData */
	if (returnCode == TPM_SUCCESS) {
	    printf("TPM_Process_DelegateManage: TPM_FAMILY_ADMIN opData %02x\n",
		   opData.buffer[0]);
	    if (*(TPM_BOOL *)(opData.buffer)) {
		familyRow->flags |= TPM_DELEGATE_ADMIN_LOCK;
	    }
	    else {
		familyRow->flags &= ~TPM_DELEGATE_ADMIN_LOCK;
	    }
	    printf("TPM_Process_DelegateManage: new TPM_FAMILY_TABLE_ENTRY.flags %08x\n",
		   familyRow->flags);
	    /* c. Set retDataSize = 0 */
	    /* NOTE Done by TPM_Sbuffer_Init() */
	    /* d. Return TPM_SUCCESS */
	}
	if (returnCode == TPM_SUCCESS) {
	    writeAllNV = TRUE;
	}
    }
    /* 8. else If opflag == TPM_FAMILY_ENABLE */
    if ((returnCode == TPM_SUCCESS) && (opCode == TPM_FAMILY_ENABLE)) {
	printf("TPM_Process_DelegateManage: Processing TPM_FAMILY_ENABLE\n");
	/* a. Validate that opDataSize == 1, and that opData is a Boolean value. */
	if (returnCode == TPM_SUCCESS) {
	    if (opData.size != sizeof(TPM_BOOL)) {
		printf("TPM_Process_DelegateManage: Error, invalid opDataSize %u\n", opData.size);
		returnCode = TPM_BAD_PARAM_SIZE;
	    }
	}
	/* b. Set FR -> flags-> TPM_FAMFLAG_ENABLED = opData */
	if (returnCode == TPM_SUCCESS) {
	    printf("TPM_Process_DelegateManage: TPM_FAMILY_ENABLE opData %02x\n",
		   opData.buffer[0]);
	    if (*(TPM_BOOL *)(opData.buffer)) {
		familyRow->flags |= TPM_FAMFLAG_ENABLED;	
	    }
	    else {
		familyRow->flags &= ~TPM_FAMFLAG_ENABLED;
	    }
	    printf("TPM_Process_DelegateManage: new TPM_FAMILY_TABLE_ENTRY.flags %08x\n",
		   familyRow->flags);
	    /* c. Set retDataSize = 0 */
	    /* NOTE Done by TPM_Sbuffer_Init() */
	    /* d. Return TPM_SUCCESS */
	}
	if (returnCode == TPM_SUCCESS) {
	    writeAllNV = TRUE;
	}
    }
    /* 9. else If opflag == TPM_FAMILY_INVALIDATE */
    if ((returnCode == TPM_SUCCESS) && (opCode == TPM_FAMILY_INVALIDATE)) {
	printf("TPM_Process_DelegateManage: Processing TPM_FAMILY_INVALIDATE\n");
	/* a. Invalidate all data associated with familyRow */
	/* i. All data is all information pointed to by FR */
	/* ii. return TPM_SELFTEST_FAILED on failure */
	TPM_FamilyTableEntry_Delete(familyRow);
	/* b.The TPM MAY invalidate delegate rows that contain the same familyID. */
	/* c. Set retDataSize = 0 */
	/* NOTE Done by TPM_Sbuffer_Init() */
	/* d. Return TPM_SUCCESS */
	writeAllNV = TRUE;
    }
    /* 10. Else return TPM_BAD_PARAMETER */
    if (returnCode == TPM_SUCCESS) {
	if ((opCode != TPM_FAMILY_CREATE) &&
	    (opCode != TPM_FAMILY_ADMIN) &&
	    (opCode != TPM_FAMILY_ENABLE) &&
	    (opCode != TPM_FAMILY_INVALIDATE)) {
	    printf("TPM_Process_DelegateManage: Error, bad opCode %08x\n", opCode);
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /* if writing NV and this is a no owner NV write, update the count with the previously
       incremented value */
    if (returnCode == TPM_SUCCESS) {
	if (writeAllNV && nv1Incremented) {
	    printf("TPM_Process_DelegateManage: noOwnerNVWrite %u\n", nv1);
	    tpm_state->tpm_permanent_data.noOwnerNVWrite = nv1;
	}
    }
    /* write back TPM_PERMANENT_DATA if required */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV,
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DelegateManage: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* append retDataSize and retData */
	    returnCode = TPM_Sbuffer_AppendAsSizedBuffer(response, &retData);
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	    returnCode = TPM_AuthParams_Set(response,
					    savedAuth,		/* saved HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&opData);	/* @1 */
    TPM_Sbuffer_Delete(&retData);	/* @2 */
    return rcf;
}

/* 19.2 TPM_Delegate_CreateKeyDelegation rev 109
   
  This command delegates privilege to use a key by creating a blob that can be used by TPM_DSAP.

  There is no check for appropriateness of the key's key usage against the key permission
  settings. If the key usage is incorrect, this command succeeds, but the delegated command will
  fail.

  These blobs CANNOT be used as input data for TPM_LoadOwnerDelegation because the internal TPM
  delegate table can store owner delegations only.
  
  (TPM_Delegate_CreateOwnerDelegation must be used to delegate Owner privilege.)

  The use restrictions that may be present on the key pointed to by keyHandle are not enforced for
  this command. Stated another way CreateKeyDelegation is not a use of the key.

  The publicInfo -> familyID can specify a disabled family row.	 The family row is checked when the
  key delegation is used in a DSAP session, not when it is created.
*/

TPM_RESULT TPM_Process_DelegateCreateKeyDelegation(tpm_state_t *tpm_state,
						   TPM_STORE_BUFFER *response,
						   TPM_TAG tag,
						   uint32_t paramSize,
						   TPM_COMMAND_CODE ordinal,
						   unsigned char *command,
						   TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_KEY_HANDLE	keyHandle;	/* The keyHandle identifier of a loaded key. */
    TPM_DELEGATE_PUBLIC publicInfo;	/* The public information necessary to fill in the blob */
    TPM_ENCAUTH		delAuth;	/* The encrypted new AuthData for the blob. The encryption
					   key is the shared secret from the authorization session
					   protocol.*/
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for keyHandle
					   authorization */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* Ignored */
    TPM_AUTHDATA	privAuth;	/* The authorization session digest that authorizes the use
					   of keyHandle. HMAC key: key.usageAuth */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey;
    TPM_BOOL			parentPCRStatus;
    TPM_KEY			*key = NULL;			/* the key specified by keyHandle */
    TPM_SECRET			*keyUsageAuth;
    TPM_DELEGATE_PUBLIC		*delegatePublic;	/* from DSAP session */
    TPM_FAMILY_TABLE_ENTRY	*familyRow;		/* family table row containing familyID */
    TPM_DIGEST			a1Auth;
    TPM_DELEGATE_SENSITIVE	m1DelegateSensitive;
    TPM_STORE_BUFFER		delegateSensitive_sbuffer;
    TPM_DELEGATE_KEY_BLOB	p1DelegateKeyBlob;


    /* output parameters  */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_STORE_BUFFER		blobSbuffer;	/* The partially encrypted delegation information.
						   */

    printf("TPM_Process_DelegateCreateKeyDelegation: Ordinal Entry\n");
    TPM_DelegatePublic_Init(&publicInfo);		/* freed @1 */
    TPM_DelegateSensitive_Init(&m1DelegateSensitive);	/* freed @2 */
    TPM_Sbuffer_Init(&delegateSensitive_sbuffer);	/* freed @3 */
    TPM_DelegateKeyBlob_Init(&p1DelegateKeyBlob);	/* freed @4 */
    TPM_Sbuffer_Init(&blobSbuffer);			/* freed @5 */
    /*
      get inputs
    */
    /* get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get publicInfo parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DelegateCreateKeyDelegation: keyHandle %08x\n", keyHandle);
	returnCode = TPM_DelegatePublic_Load(&publicInfo, &command, &paramSize);
    }
    /* get delAuth parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Load(delAuth, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALL);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag1(tag);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					privAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_DelegateCreateKeyDelegation: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Verify AuthData for the command and parameters using privAuth */
    /* get the key corresponding to the keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&key, &parentPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not read-only */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    /* get keyHandle -> usageAuth */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_GetUsageAuth(&keyUsageAuth, key);
    }	 
    /* get the session data */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_OSAP,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      key,
					      NULL,			/* OIAP */
					      key->tpm_store_asymkey->pubDataDigest);	/* OSAP */
    }
    /* Validate the authorization */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					privAuth);		/* Authorization digest for input */
    }
    /* 2. Locate publicInfo -> familyID in the TPM_FAMILY_TABLE and set familyRow to indicate row,
       return TPM_BADINDEX if not found */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_FamilyTable_GetEntry(&familyRow,
					      &(tpm_state->tpm_permanent_data.familyTable),
					      publicInfo.familyID);
    }
    /* 3. If the key authentication is in fact a delegation, then the TPM SHALL validate the command
       and parameters using Delegation authorisation, then */
    if ((returnCode == TPM_SUCCESS) && (auth_session_data->protocolID == TPM_PID_DSAP)) {
	printf("TPM_Process_DelegateCreateKeyDelegation: Authentication is a delegation\n");
	/* get the TPM_DELEGATE_PUBLIC from the DSAP session */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthSessionData_GetDelegatePublic(&delegatePublic,
							       auth_session_data);
	}
	/* a. Validate that authHandle -> familyID equals publicInfo -> familyID return
	   TPM_DELEGATE_FAMILY on error */
	if (returnCode == TPM_SUCCESS) {
	    if (publicInfo.familyID != delegatePublic->familyID) {
		printf("TPM_Process_DelegateCreateKeyDelegation: Error, "
		       "familyID %u should be %u\n",
		       publicInfo.familyID, delegatePublic->familyID);
		returnCode = TPM_DELEGATE_FAMILY;
	    }
	}
	/* b. If TPM_FAMILY_TABLE.famTableRow[ authHandle -> familyID] -> flags ->
	   TPM_FAMFLAG_ENABLED is FALSE, return error TPM_DISABLED_CMD. */
	if (returnCode == TPM_SUCCESS) {
	    if (!(familyRow->flags & TPM_FAMFLAG_ENABLED)) {
		printf("TPM_Process_DelegateCreateKeyDelegation: Error, family %u disabled\n",
		       publicInfo.familyID);
		returnCode = TPM_DISABLED_CMD;
	    }
	}
	/* c. Verify that the delegation bits in publicInfo do not grant more permissions then
	   currently delegated.	 Otherwise return error TPM_AUTHFAIL */
	if (returnCode == TPM_SUCCESS) {
	    returnCode =
		TPM_Delegations_CheckPermissionDelegation(&(publicInfo.permissions),
							  &(delegatePublic->permissions));
	}
    }
    /* 4. Check that publicInfo -> delegateType is TPM_DEL_KEY_BITS */
    if (returnCode == TPM_SUCCESS) {
	if (publicInfo.permissions.delegateType	 != TPM_DEL_KEY_BITS) {
	    printf("TPM_Process_DelegateCreateKeyDelegation: Error, "
		   "delegateType %08x not a key delegation\n",
		   publicInfo.permissions.delegateType);
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /* 5. Verify that authHandle indicates an OSAP or DSAP session return TPM_INVALID_AUTHHANDLE on
	  error */
    /* NOTE Done by TPM_AuthSessions_GetData() */
    /* 6. Create a1 by decrypting delAuth according to the ADIP indicated by authHandle. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessionData_Decrypt(a1Auth,
						 NULL,
						 delAuth,
						 auth_session_data,
						 NULL,
						 NULL,
						 FALSE);	/* even and odd */
    }
    /* 7. Create h1 the SHA-1 of TPM_STORE_PUBKEY structure of the key pointed to by keyHandle */
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_DelegateCreateKeyDelegation: Decrypted a1", a1Auth);
	returnCode = TPM_SHA1_GenerateStructure(p1DelegateKeyBlob.pubKeyDigest,
						&(key->pubKey),
						(TPM_STORE_FUNCTION_T)TPM_SizedBuffer_Store);
    }
    /* 8. Create M1 a TPM_DELEGATE_SENSITIVE structure */
    /* a. Set M1 -> tag to TPM_TAG_DELEGATE_SENSITIVE */
    /* NOTE Done by TPM_DelegateSensitive_Init() */
    /* b. Set M1 -> authValue to a1 */
    if (returnCode == TPM_SUCCESS) {
	TPM_Secret_Copy(m1DelegateSensitive.authValue, a1Auth);
	/* c. The TPM MAY add additional information of a sensitive nature relative to the
	   delegation */
	/* 9. Create M2 the encryption of M1 using TPM_DELEGATE_KEY */
	/* serialize M1 */
	returnCode = TPM_DelegateSensitive_Store(&delegateSensitive_sbuffer, &m1DelegateSensitive);
    }
    /* encrypt with delegate key */
    if (returnCode == TPM_SUCCESS) {
	    printf("TPM_Process_DelegateCreateKeyDelegation: Encrypting TPM_DELEGATE_SENSITIVE\n");
	    returnCode =
		TPM_SymmetricKeyData_EncryptSbuffer(&(p1DelegateKeyBlob.sensitiveArea),
						    &delegateSensitive_sbuffer,
						    tpm_state->tpm_permanent_data.delegateKey);
    }
    /* 10. Create P1 a TPM_DELEGATE_KEY_BLOB */
    /* a. Set P1 -> tag to TPM_TAG_DELG_KEY_BLOB */
    /* NOTE Done by TPM_DelegateKeyBlob_Init() */
    /* b. Set P1 -> pubKeyDigest to H1 */
    /* NOTE Done by TPM_StorePubkey_GenerateDigest() */
    /* c. Set P1 -> pub to PublicInfo */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_DelegatePublic_Copy(&(p1DelegateKeyBlob.pub), &publicInfo);
    }
    /* d. Set P1 -> pub -> verificationCount to familyRow -> verificationCount */
    if (returnCode == TPM_SUCCESS) {
	p1DelegateKeyBlob.pub.verificationCount = familyRow->verificationCount;
	/* e. Set P1 -> integrityDigest to NULL */
	/* NOTE Done by TPM_DelegateKeyBlob_Init() */
	/* f. The TPM sets additionalArea and additionalAreaSize appropriate for this TPM. The
	   information MAY include symmetric IV, symmetric mode of encryption and other data that
	   allows the TPM to process the blob in the future. */
	/* g. Set P1 -> sensitiveSize to the size of M2 */
	/* h. Set P1 -> sensitiveArea to M2 */
	/* NOTE Encrypted directly into p1DelegateKeyBlob.sensitiveArea */
	/* 11. Calculate H2 the HMAC of P1 using tpmProof as the secret */
	/* 12. Set P1 -> integrityDigest to H2 */
	/* NOTE It is safe to HMAC directly into TPM_DELEGATE_KEY_BLOB, since the structure
	   is serialized before the HMAC is performed */
	returnCode = TPM_HMAC_GenerateStructure
		     (p1DelegateKeyBlob.integrityDigest,		/* HMAC */
		      tpm_state->tpm_permanent_data.tpmProof,		/* HMAC key */
		      &p1DelegateKeyBlob,				/* structure */
		      (TPM_STORE_FUNCTION_T)TPM_DelegateKeyBlob_Store); /* store function */
    }
    /* 13. Ignore continueAuthSession on input set continueAuthSession to FALSE on output */
    if (returnCode == TPM_SUCCESS) {
	continueAuthSession = FALSE;
    }
    /* 14. Return P1 as blob */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_DelegateKeyBlob_Store(&blobSbuffer, &p1DelegateKeyBlob);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DelegateCreateKeyDelegation: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return blobSize and blob */
	    returnCode = TPM_Sbuffer_AppendAsSizedBuffer(response, &blobSbuffer);
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_DelegatePublic_Delete(&publicInfo);		/* @1 */
    TPM_DelegateSensitive_Delete(&m1DelegateSensitive); /* @2 */
    TPM_Sbuffer_Delete(&delegateSensitive_sbuffer);	/* @3 */
    TPM_DelegateKeyBlob_Delete(&p1DelegateKeyBlob);	/* @4 */
    TPM_Sbuffer_Delete(&blobSbuffer);			/* @5 */
    return rcf;
}

/* 19.3 TPM_Delegate_CreateOwnerDelegation rev 98

   TPM_Delegate_CreateOwnerDelegation delegates the Owner's privilege to use a set of command
   ordinals, by creating a blob. Such blobs can be used as input data for TPM_DSAP or
   TPM_Delegate_LoadOwnerDelegation.

   TPM_Delegate_CreateOwnerDelegation includes the ability to void all existing delegations (by
   incrementing the verification count) before creating the new delegation. This ensures that the
   new delegation will be the only delegation that can operate at Owner privilege in this
   family. This new delegation could be used to enable a security monitor (a local separate entity,
   or remote separate entity, or local host entity) to reinitialize a family and perhaps perform
   external verification of delegation settings. Normally the ordinals for a delegated security
   monitor would include TPM_Delegate_CreateOwnerDelegation (this command) in order to permit the
   monitor to create further delegations, and TPM_Delegate_UpdateVerification to reactivate some
   previously voided delegations.
   
   If the verification count is incremented and the new delegation does not delegate any privileges
   (to any ordinals) at all, or uses an authorisation value that is then discarded, this family's
   delegations are all void and delegation must be managed using actual Owner authorisation.
   
   (TPM_Delegate_CreateKeyDelegation must be used to delegate privilege to use a key.)
*/

TPM_RESULT TPM_Process_DelegateCreateOwnerDelegation(tpm_state_t *tpm_state,
						     TPM_STORE_BUFFER *response,
						     TPM_TAG tag,
						     uint32_t paramSize,
						     TPM_COMMAND_CODE ordinal,
						     unsigned char *command,
						     TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_BOOL		increment;	/* Flag dictates whether verificationCount will be
					   incremented */
    TPM_DELEGATE_PUBLIC publicInfo;	/* The public parameters for the blob */
    TPM_ENCAUTH		delAuth;	/* The encrypted new AuthData for the blob. The encryption
					   key is the shared secret from the OSAP protocol.*/
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle TPM Owner authentication
					   */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* Ignored */
    TPM_AUTHDATA	ownerAuth;	/* The authorization session digest. HMAC key:ownerAuth */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus = FALSE;	/* audit the ordinal */
    TPM_BOOL			transportEncrypt = FALSE;	/* wrapped in encrypted transport
								   session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey;
    TPM_SECRET			savedAuth;		/* saved copy for response */
    TPM_FAMILY_TABLE_ENTRY	*familyRow;		/* family table row containing familyID */
    TPM_DELEGATE_PUBLIC		*delegatePublic;	/* from DSAP session */
    TPM_BOOL			writeAllNV = FALSE;
    TPM_DIGEST			a1Auth;
    TPM_DELEGATE_SENSITIVE	m1DelegateSensitive;
    TPM_STORE_BUFFER		delegateSensitive_sbuffer; /* serialization of delegateSensitive */
    TPM_DELEGATE_OWNER_BLOB	b1DelegateOwnerBlob;
    
    /* output parameters  */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_STORE_BUFFER		blobSbuffer;	/* The partially encrypted delegation
						   information. */

    printf("TPM_Process_DelegateCreateOwnerDelegation: Ordinal Entry\n");
    TPM_DelegatePublic_Init(&publicInfo);		/* freed @1 */
    TPM_DelegateSensitive_Init(&m1DelegateSensitive);	/* freed @2 */
    TPM_Sbuffer_Init(&delegateSensitive_sbuffer);	/* freed @3 */
    TPM_DelegateOwnerBlob_Init(&b1DelegateOwnerBlob);	/* freed @4 */
    TPM_Sbuffer_Init(&blobSbuffer);			/* freed @5 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get increment parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_LoadBool(&increment, &command, &paramSize);
    }
    /* get publicInfo parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DelegateCreateOwnerDelegation: increment %02x\n", increment);
	returnCode = TPM_DelegatePublic_Load(&publicInfo, &command, &paramSize);
    }
    /* get delAuth parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Load(delAuth, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALL);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag1(tag);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_DelegateCreateOwnerDelegation: Error, "
		   "command has %u extra bytes\n", paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. The TPM SHALL authenticate the command using TPM Owner authentication. Return TPM_AUTHFAIL
       on failure. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_OSAP,
					      TPM_ET_OWNER,
					      ordinal,
					      NULL,
					      NULL,					/* OIAP */
					      tpm_state->tpm_permanent_data.ownerAuth); /* OSAP */
    }
    if (returnCode == TPM_SUCCESS) {
	/* make a copy of the HMAC key for the response, since it MAY be invalidated */
	TPM_Secret_Copy(savedAuth, *hmacKey);
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* owner HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					ownerAuth);		/* Authorization digest for input */
    }
    /* 2. Locate publicInfo -> familyID in the TPM_FAMILY_TABLE and set familyRow to indicate the
	  row return TPM_BADINDEX if not found */
    /* a. Set FR to TPM_FAMILY_TABLE.famTableRow[familyRow] */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_FamilyTable_GetEntry(&familyRow,
					      &(tpm_state->tpm_permanent_data.familyTable),
					      publicInfo.familyID);
    }
    /* 3. If the TPM Owner authentication is in fact a delegation, then the TPM SHALL validate the
       command and parameters using Delegation authorisation, then */
    if ((returnCode == TPM_SUCCESS) && (auth_session_data->protocolID == TPM_PID_DSAP)) {
	/* get the TPM_DELEGATE_PUBLIC from the DSAP session */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthSessionData_GetDelegatePublic(&delegatePublic,
							       auth_session_data);
	}
	/* a. Validate that authHandle -> familyID equals publicInfo -> familyID return
	   TPM_DELEGATE_FAMILY */
	if (returnCode == TPM_SUCCESS) {
	    if (publicInfo.familyID != delegatePublic->familyID) {
		printf("TPM_Process_DelegateCreateOwnerDelegation: Error, "
		       "familyID %u should be %u\n",
		       publicInfo.familyID, delegatePublic->familyID);
		returnCode = TPM_DELEGATE_FAMILY;
	    }
	}
	/* b. If FR -> flags -> TPM_FAMFLAG_ENABLED is FALSE, return error TPM_DISABLED_CMD. */
	if (returnCode == TPM_SUCCESS) {
	    if (!(familyRow->flags & TPM_FAMFLAG_ENABLED)) {
		printf("TPM_Process_DelegateCreateOwnerDelegation: Error, family %u disabled\n",
		       publicInfo.familyID);
		returnCode = TPM_DISABLED_CMD;
	    }
	}
	/* c. Verify that the delegation bits in publicInfo do not grant more permissions then
	   currently delegated.	 Otherwise return error TPM_AUTHFAIL */
	if (returnCode == TPM_SUCCESS) {
	    returnCode =
		TPM_Delegations_CheckPermissionDelegation(&(publicInfo.permissions),
							  &(delegatePublic->permissions));
	}
    }
    /* 4. Check that publicInfo -> delegateType is TPM_DEL_OWNER_BITS */
    if (returnCode == TPM_SUCCESS) {
	if (publicInfo.permissions.delegateType != TPM_DEL_OWNER_BITS) {
	    printf("TPM_Process_DelegateCreateOwnerDelegation: Error, bad delegateType %08x\n",
		   publicInfo.permissions.delegateType);
	    returnCode = TPM_BAD_PARAMETER;
	}
    }
    /* 5. Verify that authHandle indicates an OSAP or DSAP session return TPM_INVALID_AUTHHANDLE on
       error */
    /* NOTE Done by TPM_AuthSessions_GetData() */
    /* 7. Create a1 by decrypting delAuth according to the ADIP indicated by authHandle */
    /* NOTE 7. moved before 6. because it needs the session data */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessionData_Decrypt(a1Auth,
						 NULL,
						 delAuth,
						 auth_session_data,
						 NULL,
						 NULL,
						 FALSE);	/* even and odd */
    }
    /* 6. If increment == TRUE */
    if ((returnCode == TPM_SUCCESS) && increment) {
	/* a. Increment FR -> verificationCount */
	familyRow->verificationCount++;
	writeAllNV = TRUE;
	/* b. Set TPM_STCLEAR_DATA -> ownerReference to TPM_KH_OWNER */
	tpm_state->tpm_stclear_data.ownerReference = TPM_KH_OWNER;
	/* c. The TPM invalidates sessions */
	/* i. MUST invalidate all DSAP sessions */
	/* ii. MUST invalidate all OSAP sessions associated with the delegation table */
	/* iii. MAY invalidate any other session */
	TPM_AuthSessions_TerminatexSAP(&continueAuthSession,
				       authHandle,
				       tpm_state->tpm_stclear_data.authSessions);
    }
    /* 8. Create M1 a TPM_DELEGATE_SENSITIVE structure */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DelegateCreateOwnerDelegation: Creating TPM_DELEGATE_SENSITIVE\n");
	/* a. Set M1 -> tag to TPM_TAG_DELEGATE_SENSITIVE */
	/* NOTE Done by TPM_DelegateSensitive_Init() */
	/* b. Set M1 -> authValue to a1 */
	TPM_Secret_Copy(m1DelegateSensitive.authValue, a1Auth);
	/* c. Set other M1 fields as determined by the TPM vendor */
    }
    /* 9. Create M2 the encryption of M1 using TPM_DELEGATE_KEY */
    /* serialize M1 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_DelegateSensitive_Store(&delegateSensitive_sbuffer, &m1DelegateSensitive);
    }
    /* encrypt with delegate key */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DelegateCreateOwnerDelegation: Encrypting TPM_DELEGATE_SENSITIVE\n");
	returnCode =
	    TPM_SymmetricKeyData_EncryptSbuffer(&(b1DelegateOwnerBlob.sensitiveArea),
						&delegateSensitive_sbuffer,
						tpm_state->tpm_permanent_data.delegateKey);
    }
    /* 10. Create B1 a TPM_DELEGATE_OWNER_BLOB */
    /* a. Set B1 -> tag to TPM_TAG_DELG_OWNER_BLOB */
    /* NOTE Done by TPM_DelegateOwnerBlob_Init() */
    /* b. Set B1 -> pub to publicInfo */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DelegateCreateOwnerDelegation: Creating TPM_DELEGATE_OWNER_BLOB\n");
	returnCode = TPM_DelegatePublic_Copy(&(b1DelegateOwnerBlob.pub), &publicInfo);
    }
    /* c. Set B1 -> sensitiveSize to the size of M2 */
    /* d. Set B1 -> sensitiveArea to M2 */
    /* NOTE Encrypted directly into b1DelegateOwnerBlob */
    /* e. Set B1 -> integrityDigest to NULL */
    /* NOTE Done by TPM_DelegateOwnerBlob_Init() */
    if (returnCode == TPM_SUCCESS) {
	/* f. Set B1 pub -> verificationCount to FR -> verificationCount */
	b1DelegateOwnerBlob.pub.verificationCount = familyRow->verificationCount;
	/* 11. The TPM sets additionalArea and additionalAreaSize appropriate for this TPM. The
	   information MAY include symmetric IV, symmetric mode of encryption and other data that
	   allows the TPM to process the blob in the future. */
	/* 12. Create H1 the HMAC of B1 using tpmProof as the secret */
	/* 13. Set B1 -> integrityDigest to H1 */
	/* NOTE It is safe to HMAC directly into TPM_DELEGATE_OWNER_BLOB, since the structure
	   is serialized before the HMAC is performed */
	returnCode = TPM_HMAC_GenerateStructure
		     (b1DelegateOwnerBlob.integrityDigest,		/* HMAC */
		      tpm_state->tpm_permanent_data.tpmProof,		/* HMAC key */
		      &b1DelegateOwnerBlob,				/* structure */
		      (TPM_STORE_FUNCTION_T)TPM_DelegateOwnerBlob_Store); /* store function */
    }
    /* 14. Ignore continueAuthSession on input set continueAuthSession to FALSE on output */
    if (returnCode == TPM_SUCCESS) {
	continueAuthSession = FALSE;
    }
    /* 15. Return B1 as blob */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_DelegateOwnerBlob_Store(&blobSbuffer, &b1DelegateOwnerBlob);
    }
    /* write back TPM_PERMANENT_DATA if required */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV,
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DelegateCreateOwnerDelegation: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return blobSize and blob */
	    returnCode = TPM_Sbuffer_AppendAsSizedBuffer(response, &blobSbuffer);
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    savedAuth,		/* saved HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_DelegatePublic_Delete(&publicInfo);		/* @1 */
    TPM_DelegateSensitive_Delete(&m1DelegateSensitive); /* @2 */
    TPM_Sbuffer_Delete(&delegateSensitive_sbuffer);	/* @3 */
    TPM_DelegateOwnerBlob_Delete(&b1DelegateOwnerBlob); /* @4 */
    TPM_Sbuffer_Delete(&blobSbuffer);			/* @5 */
    return rcf;
}

/* 19.4 TPM_Delegate_LoadOwnerDelegation rev 109
   
  This command loads a delegate table row blob into a non-volatile delegate table row.
  Delegate_LoadOwnerDelegation can be used during manufacturing or on first boot (when no Owner is
  installed), or after an Owner is installed. If an Owner is installed, Delegate_LoadOwnerDelegation
  requires Owner authorisation, and sensitive information must be encrypted.

  Burn-out of TPM non-volatile storage by inappropriate use is mitigated by the TPM's normal limits
  on NV- writes in the absence of an Owner. Tables can be locked after loading using
  TPM_Delegate_Manage, to prevent subsequent tampering.

  A management system outside the TPM is expected to manage the delegate table rows stored on the
  TPM, and can overwrite any previously stored data.  There is no way to explicitly delete a
  delegation entry.  A new entry can overwrite an invalid entry.

  This command cannot be used to load key delegation blobs into the TPM
*/

TPM_RESULT TPM_Process_DelegateLoadOwnerDelegation(tpm_state_t *tpm_state,
						   TPM_STORE_BUFFER *response,
						   TPM_TAG tag,
						   uint32_t paramSize,
						   TPM_COMMAND_CODE ordinal,
						   unsigned char *command,
						   TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_DELEGATE_INDEX		index;		/* The index of the delegate row to be written */
    uint32_t			blobSize;	/* The size of the delegate blob */
    TPM_DELEGATE_OWNER_BLOB	d1Blob;		/* Delegation information, including encrypted
						   portions as appropriate */
    TPM_AUTHHANDLE		authHandle;	/* The authorization session handle TPM Owner
						   authentication */
    TPM_NONCE			nonceOdd;	/* Nonce generated by system associated with
						   authHandle */
    TPM_BOOL		continueAuthSession = TRUE;	/* The continue use flag for the
							   authorization session handle */
    TPM_AUTHDATA		ownerAuth;	/* The authorization session digest. HMAC
						   key:ownerAuth */
    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus = FALSE;	/* audit the ordinal */
    TPM_BOOL			transportEncrypt = FALSE;	/* wrapped in encrypted transport
								   session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey;
    TPM_SECRET			savedAuth;		/* saved copy for response */
    TPM_DELEGATE_PUBLIC		*delegatePublic;	/* from DSAP session */
    TPM_FAMILY_TABLE_ENTRY	*familyRow;		/* family table row containing familyID */
    TPM_DELEGATE_SENSITIVE	s1DelegateSensitive;
    TPM_DELEGATE_TABLE_ROW	*delegateTableRow;
    unsigned char		*stream;
    uint32_t			stream_size;
    uint32_t			nv1 = tpm_state->tpm_permanent_data.noOwnerNVWrite;
							/* temp for noOwnerNVWrite, initialize to
							   silence compiler */
    TPM_BOOL			nv1Incremented = FALSE;	/* flag that nv1 was incremented */
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back data */

    /* output parameters  */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;

    printf("TPM_Process_DelegateLoadOwnerDelegation: Ordinal Entry\n");
    TPM_DelegateOwnerBlob_Init(&d1Blob);		/* freed @1 */
    TPM_DelegateSensitive_Init(&s1DelegateSensitive);	/* freed @2 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get index parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&index, &command, &paramSize);
    }
    /* get blobSize parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DelegateLoadOwnerDelegation: index %u\n", index);
	returnCode = TPM_Load32(&blobSize, &command, &paramSize);
    }
    /* get blob parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_DelegateOwnerBlob_Load(&d1Blob, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALLOW_NO_OWNER);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag10(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_DelegateLoadOwnerDelegation: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Map blob to D1 a TPM_DELEGATE_OWNER_BLOB. */
    /* a. Validate that D1 -> tag == TPM_TAG_DELEGATE_OWNER_BLOB */
    /* Done by TPM_DelegateOwnerBlob_Load() */
    /* 2. Locate D1 -> pub -> familyID in the TPM_FAMILY_TABLE and set familyRow to indicate row,
       return TPM_BADINDEX if not found */
    /* 3. Set FR to TPM_FAMILY_TABLE -> famTableRow[familyRow] */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_FamilyTable_GetEnabledEntry(&familyRow,
						     &(tpm_state->tpm_permanent_data.familyTable),
						     d1Blob.pub.familyID);
    }
    /* 4. If TPM Owner is installed */
    if ((returnCode == TPM_SUCCESS) && tpm_state->tpm_permanent_data.ownerInstalled) {
	/* a. Validate the command and parameters using TPM Owner authorization, return
	   TPM_AUTHFAIL on error */
	if (returnCode == TPM_SUCCESS) {
	    if (tag != TPM_TAG_RQU_AUTH1_COMMAND) {
		printf("TPM_Process_DelegateLoadOwnerDelegation: Error, "
		       "owner installed but no authorization\n");
		returnCode = TPM_AUTHFAIL;
	    }
	}
	if (returnCode == TPM_SUCCESS) {
	    returnCode =
		TPM_AuthSessions_GetData(&auth_session_data,
					 &hmacKey,
					 tpm_state,
					 authHandle,
					 TPM_PID_NONE,
					 TPM_ET_OWNER,
					 ordinal,
					 NULL,
					 &(tpm_state->tpm_permanent_data.ownerAuth),/* OIAP */
					 tpm_state->tpm_permanent_data.ownerAuth);  /* OSAP */
	}
	if (returnCode == TPM_SUCCESS) {
	    /* make a copy of the HMAC key for the response, since it MAY be invalidated */
	    TPM_Secret_Copy(savedAuth, *hmacKey);
	    returnCode = TPM_Authdata_Check(tpm_state,
					    *hmacKey,		/* owner HMAC key */
					    inParamDigest,
					    auth_session_data,	/* authorization session */
					    nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					    continueAuthSession,
					    ownerAuth);		/* Authorization digest for input */
	}
	/* b. If the command is delegated (authHandle session type is TPM_PID_DSAP or through
	   ownerReference delegation), verify that D1 -> pub -> familyID matches authHandle ->
	   familyID, on error return TPM_DELEGATE_FAMILY */
	if ((returnCode == TPM_SUCCESS) &&
	    ((auth_session_data->protocolID == TPM_PID_DSAP) ||
	     (tpm_state->tpm_stclear_data.ownerReference != TPM_KH_OWNER))) {
	    /* get the TPM_DELEGATE_PUBLIC from the DSAP session */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_AuthSessionData_GetDelegatePublic(&delegatePublic,
								   auth_session_data);
	    }
	    if (returnCode == TPM_SUCCESS) {
		if (d1Blob.pub.familyID != delegatePublic->familyID) {
		    printf("TPM_Process_DelegateLoadOwnerDelegation: Error, "
			   "familyID %u should be %u\n",
			   d1Blob.pub.familyID, delegatePublic->familyID);
		    returnCode = TPM_DELEGATE_FAMILY;
		}
	    }
	}
    }
    /* 5. Else */
    if ((returnCode == TPM_SUCCESS) && !tpm_state->tpm_permanent_data.ownerInstalled) {
	/* a. If FR -> flags -> TPM_DELEGATE_ADMIN_LOCK is TRUE return TPM_DELEGATE_LOCK */
	if (returnCode == TPM_SUCCESS) {
	    if (familyRow->flags & TPM_DELEGATE_ADMIN_LOCK) {
		printf("TPM_Process_DelegateLoadOwnerDelegation: Error, row locked\n");
		returnCode = TPM_DELEGATE_LOCK;
	    }
	}
	/* b. Validate max NV writes without an owner */
	if (returnCode == TPM_SUCCESS) {
	    /* i. Set NV1 to PD -> noOwnerNVWrite */
	    nv1 = tpm_state->tpm_permanent_data.noOwnerNVWrite;
	    /* ii. Increment NV1 by 1 */
	    nv1++;
	    /* iii. If NV1 > TPM_MAX_NV_WRITE_NOOWNER return TPM_MAXNVWRITES */
	    if (nv1 > TPM_MAX_NV_WRITE_NOOWNER) {
		printf("TPM_Process_DelegateLoadOwnerDelegation: Error, "
		       "max NV writes %d w/o owner reached\n",
		       tpm_state->tpm_permanent_data.noOwnerNVWrite);
		returnCode = TPM_MAXNVWRITES;
	    }
	}
	/* iv. Set PD -> noOwnerNVWrite to NV1 */
	if (returnCode == TPM_SUCCESS) {
	    /* NOTE Don't update the noOwnerNVWrite value until determining that the write will be
	       performed */
	    nv1Incremented = TRUE;
	}
    }
    /* 6. If FR -> flags -> TPM_FAMFLAG_ENABLED is FALSE, return TPM_DISABLED_CMD */
    /* NOTE Done by TPM_FamilyTable_GetEnabledEntry() */
    /* 7. If TPM Owner is installed, validate the integrity of the blob */
    if ((returnCode == TPM_SUCCESS) && tpm_state->tpm_permanent_data.ownerInstalled) {
	printf("TPM_Process_DelegateLoadOwnerDelegation: Checking integrityDigest\n");
	/* a. Copy D1 -> integrityDigest to H2 */
	/* b. Set D1 -> integrityDigest to NULL */
	/* c. Create H3 the HMAC of D1 using tpmProof as the secret */
	/* d. Compare H2 to H3, return TPM_AUTHFAIL on mismatch */
	returnCode = TPM_HMAC_CheckStructure
		     (tpm_state->tpm_permanent_data.tpmProof,		/* key */
		      &d1Blob,						/* structure */
		      d1Blob.integrityDigest,				/* expected */
		      (TPM_STORE_FUNCTION_T)TPM_DelegateOwnerBlob_Store, /* store function */
		      TPM_AUTHFAIL);					/* error code */
    }
    /* 8. If TPM Owner is installed, create S1 a TPM_DELEGATE_SENSITIVE area by decrypting D1 ->
       sensitiveArea using TPM_DELEGATE_KEY. */
    if ((returnCode == TPM_SUCCESS) && tpm_state->tpm_permanent_data.ownerInstalled) {
	if (returnCode == TPM_SUCCESS) {
	    printf("TPM_Process_DelegateLoadOwnerDelegation: Decrypting sensitiveArea\n");
	    returnCode =
		TPM_DelegateSensitive_DecryptEncData(&s1DelegateSensitive,	/* decrypted data */
						     &(d1Blob.sensitiveArea),	/* encrypted */
						     tpm_state->tpm_permanent_data.delegateKey);
	}
    }
    /* 8. Otherwise set S1 = D1 -> sensitiveArea */
    if ((returnCode == TPM_SUCCESS) && !tpm_state->tpm_permanent_data.ownerInstalled) {
	stream = d1Blob.sensitiveArea.buffer;
	stream_size = d1Blob.sensitiveArea.size;
	returnCode = TPM_DelegateSensitive_Load(&s1DelegateSensitive, &stream, &stream_size);
    }
    /* 9. Validate S1 */
    /* a. Validate that S1 -> tag == TPM_TAG_DELEGATE_SENSITIVE, return TPM_INVALID_STRUCTURE on
       error */
    /* NOTE Done by TPM_DelegateSensitive_Load() */
    /* 10. Validate that index is a valid value for delegateTable, return TPM_BADINDEX on error */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_DelegateTable_GetRow(&delegateTableRow,
					      &(tpm_state->tpm_permanent_data.delegateTable),
					      index);
    }
    /* 11. The TPM invalidates sessions */
    if (returnCode == TPM_SUCCESS) {
	/* a. MUST invalidate all DSAP sessions */
	/* b. MUST invalidate all OSAP sessions associated with the delegation table */
	/* c. MAY invalidate any other session */
	TPM_AuthSessions_TerminatexSAP(&continueAuthSession,
				       authHandle,
				       tpm_state->tpm_stclear_data.authSessions);
    }
    /* 12. Copy data to the delegate table row */
    if (returnCode == TPM_SUCCESS) {
	/* a. Copy the TPM_DELEGATE_PUBLIC from D1 -> pub to TPM_DELEGATE_TABLE -> delRow[index] ->
	   pub. */
	returnCode = TPM_DelegatePublic_Copy(&delegateTableRow->pub, &(d1Blob.pub));
	writeAllNV = TRUE;
    }
    if (returnCode == TPM_SUCCESS) {
	delegateTableRow->valid = TRUE;
	/* b. Copy the TPM_SECRET from S1 -> authValue to TPM_DELEGATE_TABLE -> delRow[index] ->
	   authValue. */
	TPM_Secret_Copy(delegateTableRow->authValue, s1DelegateSensitive.authValue);
	/* c. Set TPM_STCLEAR_DATA-> ownerReference to TPM_KH_OWNER */
	tpm_state->tpm_stclear_data.ownerReference = TPM_KH_OWNER;
    }
    if ((returnCode == TPM_SUCCESS) && tpm_state->tpm_permanent_data.ownerInstalled) {
	/* d. If authHandle is of type DSAP then continueAuthSession MUST set to FALSE */
	if (auth_session_data->protocolID == TPM_PID_DSAP) {
	    continueAuthSession = FALSE;
	}
    }
    /* if writing NV and this is a no owner NV write, update the count with the previously
       incremented value */
    if (returnCode == TPM_SUCCESS) {
	if (writeAllNV && nv1Incremented) {
	    printf("TPM_Process_DelegateLoadOwnerDelegation: noOwnerNVWrite %u\n", nv1);
	    tpm_state->tpm_permanent_data.noOwnerNVWrite = nv1;
	}
    }
    /* write back TPM_PERMANENT_DATA */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV,
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DelegateLoadOwnerDelegation: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	    returnCode = TPM_AuthParams_Set(response,
					    savedAuth,		/* saved HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_DelegateOwnerBlob_Delete(&d1Blob);		/* @1 */
    TPM_DelegateSensitive_Delete(&s1DelegateSensitive); /* @2 */
    return rcf;
}

/* 19.5 TPM_Delegate_ReadTable rev 87

   This command is used to read from the TPM the public contents of the family and delegate tables
   that are stored on the TPM. Such data is required during external verification of tables.
  
   There are no restrictions on the execution of this command; anyone can read this information
   regardless of the state of the PCRs, regardless of whether they know any specific AuthData value
   and regardless of whether or not the enable and admin bits are set one way or the other.
*/

TPM_RESULT TPM_Process_DelegateReadTable(tpm_state_t *tpm_state,
					 TPM_STORE_BUFFER *response,
					 TPM_TAG tag,
					 uint32_t paramSize,
					 TPM_COMMAND_CODE ordinal,
					 unsigned char *command,
					 TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters  */
    uint32_t			outParamStart;		/* starting point of outParam's */
    uint32_t			outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_STORE_BUFFER		familySbuffer;		/* Array of TPM_FAMILY_TABLE_ENTRY
							   elements */
    TPM_STORE_BUFFER		delegateSbuffer;	/* Array of TPM_DELEGATE_INDEX and
							   TPM_DELEGATE_PUBLIC elements */

    printf("TPM_Process_DelegateReadTable: Ordinal Entry\n");
    TPM_Sbuffer_Init(&familySbuffer);		/* freed @1 */
    TPM_Sbuffer_Init(&delegateSbuffer);		/* freed @2 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALLOW_NO_OWNER);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_DelegateReadTable: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Set familyTableSize to the number of valid families on the TPM times
       sizeof(TPM_FAMILY_TABLE_ELEMENT). */
    /* NOTE Done below by TPM_Sbuffer_AppendAsSizedBuffer() */
    /* 2. Copy the valid entries in the internal family table to the output array familyTable */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_FamilyTable_StoreValid(&familySbuffer,
						&(tpm_state->tpm_permanent_data.familyTable),
						TRUE);		/* standard, store the tag */
    }
    /* 3. Set delegateTableSize to the number of valid delegate table entries on the TPM times
       (sizeof(TPM_DELEGATE_PUBLIC) + 4). */
    /* NOTE Done below by TPM_Sbuffer_AppendAsSizedBuffer()  */
    /* 4. For each valid entry */
    /* a. Write the TPM_DELEGATE_INDEX to delegateTable */
    /* b. Copy the TPM_DELEGATE_PUBLIC to delegateTable */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_DelegateTable_StoreValid(&delegateSbuffer,
						  &(tpm_state->tpm_permanent_data.delegateTable));
    }
    /* 5. Return TPM_SUCCESS */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DelegateReadTable: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* append familyTableSize and familyTable */
	    returnCode = TPM_Sbuffer_AppendAsSizedBuffer(response, &familySbuffer);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* append delegateTableSize and delegateTable */
	    returnCode = TPM_Sbuffer_AppendAsSizedBuffer(response, &delegateSbuffer);
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /*
      cleanup
    */
    TPM_Sbuffer_Delete(&familySbuffer);		/* @1 */
    TPM_Sbuffer_Delete(&delegateSbuffer);	/* @2 */
    return rcf;
}
    
/* 19.6 TPM_Delegate_UpdateVerification rev 87
   
   UpdateVerification sets the verificationCount in an entity (a blob or a delegation row) to the
   current family value, in order that the delegations represented by that entity will continue to
   be accepted by the TPM.
*/

TPM_RESULT TPM_Process_DelegateUpdateVerification(tpm_state_t *tpm_state,
						  TPM_STORE_BUFFER *response,
						  TPM_TAG tag,
						  uint32_t paramSize,
						  TPM_COMMAND_CODE ordinal,
						  unsigned char *command,
						  TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_SIZED_BUFFER	inputData;	/* TPM_DELEGATE_KEY_BLOB or TPM_DELEGATE_OWNER_BLOB or 
					   TPM_DELEGATE_INDEX */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for owner
					   authentication. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA	ownerAuth;	/* Authorization HMAC key: ownerAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus = FALSE;	/* audit the ordinal */
    TPM_BOOL			transportEncrypt = FALSE;	/* wrapped in encrypted transport
								   session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey = NULL;
    unsigned char		*stream;		/* temp input stream */
    uint32_t			stream_size;
    TPM_STRUCTURE_TAG		d1Tag;			/* input structure tag */
    TPM_DELEGATE_INDEX		d1DelegateIndex;
    TPM_DELEGATE_OWNER_BLOB	d1DelegateOwnerBlob;
    TPM_DELEGATE_KEY_BLOB	d1DelegateKeyBlob;
    TPM_DELEGATE_TABLE_ROW	*d1DelegateTableRow = NULL;
    TPM_FAMILY_ID		familyID = 0;
    TPM_FAMILY_TABLE_ENTRY	*familyRow;		/* family table row containing familyID */
    TPM_DELEGATE_PUBLIC		*delegatePublic;	/* from DSAP session */
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back NV */

    /* output parameters  */
    uint32_t			outParamStart;		/* starting point of outParam's */
    uint32_t			outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    TPM_STORE_BUFFER		outputDataSbuffer;	/* TPM_DELEGATE_KEY_BLOB or
							   TPM_DELEGATE_OWNER_BLOB */

    printf("TPM_Process_DelegateUpdateVerification: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&inputData);			/* freed @1 */
    TPM_DelegateOwnerBlob_Init(&d1DelegateOwnerBlob);	/* freed @2 */
    TPM_DelegateKeyBlob_Init(&d1DelegateKeyBlob);	/* freed @3 */
    TPM_Sbuffer_Init(&outputDataSbuffer);		/* freed @4 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get inputData parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&inputData, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALL);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag1(tag);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_DelegateUpdateVerification: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Verify the TPM Owner, directly or indirectly through delegation, authorizes the command
       and parameters, on error return TPM_AUTHFAIL */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_OWNER,
					      ordinal,
					      NULL,
					      &(tpm_state->tpm_permanent_data.ownerAuth), /* OIAP */
					      tpm_state->tpm_permanent_data.ownerAuth);	  /* OSAP */
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* owner HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					ownerAuth);		/* Authorization digest for input */
    }
    /* 2. Determine the type of inputData (TPM_DELEGATE_TABLE_ROW or TPM_DELEGATE_OWNER_BLOB
       or TPM_DELEGATE_KEY_BLOB) and map D1 to that structure */
    if (returnCode == TPM_SUCCESS) {
	/* use a temporary copy so the original values are not moved */
	stream = inputData.buffer;
	stream_size = inputData.size;
	/* the inputData is either a table index or a blob */
	if (inputData.size == sizeof(TPM_DELEGATE_INDEX)) {
	    /* if it's an index, get the index */
	    returnCode = TPM_Load32(&d1DelegateIndex, &stream, &stream_size); 
	}
	else {
	    /* if it's a blob, get the blob structure tag to determine the blob type */
	    returnCode = TPM_Load16(&d1Tag, &stream, &stream_size);
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* use a temporary copy so the original values are not moved */
	stream = inputData.buffer;
	stream_size = inputData.size;
	/* if inputData is a table index */
	if (inputData.size == sizeof(TPM_DELEGATE_INDEX)) {
	    /* a. Mapping to TPM_DELEGATE_TABLE_ROW requires taking inputData as a tableIndex and
	       locating the appropriate row in the table */
	    returnCode =
		TPM_DelegateTable_GetValidRow(&d1DelegateTableRow,
					      &(tpm_state->tpm_permanent_data.delegateTable),
					      d1DelegateIndex);
	    familyID = d1DelegateTableRow->pub.familyID;
	}
	/* if inputData is a blob */
	else {
	    switch (d1Tag) {
	      case TPM_TAG_DELEGATE_OWNER_BLOB:
		returnCode = TPM_DelegateOwnerBlob_Load(&d1DelegateOwnerBlob,
							&stream, &stream_size);
		familyID = d1DelegateOwnerBlob.pub.familyID;
		break;
	      case TPM_TAG_DELG_KEY_BLOB:
		returnCode = TPM_DelegateKeyBlob_Load(&d1DelegateKeyBlob,
						      &stream, &stream_size);
		familyID = d1DelegateKeyBlob.pub.familyID;
		break;
	      default:
		printf("TPM_Process_DelegateUpdateVerification: Error, invalid tag %04hx\n", d1Tag);
		returnCode = TPM_BAD_PARAMETER;
		break;
	    }
	}
    }
    /* 3. If D1 is TPM_DELEGATE_OWNER_BLOB or TPM_DELEGATE_KEY_BLOB Validate the integrity of
       D1 */
    if ((returnCode == TPM_SUCCESS) && (inputData.size != sizeof(TPM_DELEGATE_INDEX))) {
	/* a. Copy D1 -> integrityDigest to H2 */
	/* b. Set D1 -> integrityDigest to NULL */
	/* c. Create H3 the HMAC of D1 using tpmProof as the secret */
	/* d. Compare H2 to H3 return TPM_AUTHFAIL on mismatch */
	switch (d1Tag) {
	  case TPM_TAG_DELEGATE_OWNER_BLOB:
	    returnCode = TPM_HMAC_CheckStructure
			 (tpm_state->tpm_permanent_data.tpmProof,		/* key */
			  &d1DelegateOwnerBlob,					/* structure */
			  d1DelegateOwnerBlob.integrityDigest,			/* expected */
			  (TPM_STORE_FUNCTION_T)TPM_DelegateOwnerBlob_Store,	/* store function */
			  TPM_AUTHFAIL);					/* error code */
	    break;
	  case TPM_TAG_DELG_KEY_BLOB:
	    returnCode = TPM_HMAC_CheckStructure
			 (tpm_state->tpm_permanent_data.tpmProof,		/* key */
			  &d1DelegateKeyBlob,					/* structure */
			  d1DelegateKeyBlob.integrityDigest,			/* expected */
			  (TPM_STORE_FUNCTION_T)TPM_DelegateKeyBlob_Store,	/* store function */
			  TPM_AUTHFAIL);					/* error code */
	    break;
	    /* default error tested above */
	}
    }
    /* 4. Locate (D1 -> pub -> familyID) in the TPM_FAMILY_TABLE and set familyRow to indicate row,
       return TPM_BADINDEX if not found */
    /* 5. Set FR to TPM_FAMILY_TABLE.famTableRow[familyRow] */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_FamilyTable_GetEntry(&familyRow,
					      &(tpm_state->tpm_permanent_data.familyTable),
					      familyID);
    }
    if ((returnCode == TPM_SUCCESS) && (auth_session_data->protocolID == TPM_PID_DSAP)) {
	/* get the TPM_DELEGATE_PUBLIC from the DSAP session */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthSessionData_GetDelegatePublic(&delegatePublic,
							       auth_session_data);
	}
	/* 6. If delegated, verify that family of the delegated Owner-auth is the same as D1:
	   (authHandle -> familyID) == (D1 -> pub -> familyID); otherwise return error
	   TPM_DELEGATE_FAMILY */
	if (returnCode == TPM_SUCCESS) {
	    if (familyID != delegatePublic->familyID) {
		printf("TPM_Process_DelegateUpdateVerification: Error, "
		       "familyID %u should be %u\n",
		       familyID, delegatePublic->familyID);
		returnCode = TPM_DELEGATE_FAMILY;
	    }
	}
	/* 7. If delegated, verify that the family of the delegated Owner-auth is enabled: if
	   (authHandle -> familyID -> flags TPM_FAMFLAG_ENABLED) is FALSE, return
	   TPM_DISABLED_CMD */
	if (returnCode == TPM_SUCCESS) {
	    if (!(familyRow->flags & TPM_FAMFLAG_ENABLED)) {
		printf("TPM_Process_DelegateUpdateVerification: Error, family %u disabled\n",
		       familyID);
		returnCode = TPM_DISABLED_CMD;
	    }
	}
    }
    /* 8. Set D1 -> verificationCount to FR -> verificationCount */
    if (returnCode == TPM_SUCCESS) {
	if (inputData.size == sizeof(TPM_DELEGATE_INDEX)) {
	    d1DelegateTableRow->pub.verificationCount = familyRow->verificationCount;
	    writeAllNV = TRUE;
	}
	else {
	    switch (d1Tag) {
	      case TPM_TAG_DELEGATE_OWNER_BLOB:
		d1DelegateOwnerBlob.pub.verificationCount = familyRow->verificationCount;
		break;
	      case TPM_TAG_DELG_KEY_BLOB:
		d1DelegateKeyBlob.pub.verificationCount = familyRow->verificationCount;
		break;
		/* default error tested above */
	    }
	}
    }
    /* 9. If D1 is TPM_DELEGATE_OWNER_BLOB or TPM_DELEGATE_KEY_BLOB set the integrity of D1 */
    if ((returnCode == TPM_SUCCESS) && (inputData.size != sizeof(TPM_DELEGATE_INDEX))) {
	/* a. Set D1 -> integrityDigest to NULL */
	/* NOTE Done by TPM_HMAC_GenerateStructure() */
	/* b. Create H1 the HMAC of D1 using tpmProof as the secret */
	/* c. Set D1 -> integrityDigest to H1 */
	/* NOTE It is safe to HMAC directly into the blob, since the structure is serialized before
	   the HMAC is performed */
	switch (d1Tag) {
	  case TPM_TAG_DELEGATE_OWNER_BLOB:
	    returnCode = TPM_HMAC_GenerateStructure
			 (d1DelegateOwnerBlob.integrityDigest,		/* HMAC */
			  tpm_state->tpm_permanent_data.tpmProof,		/* HMAC key */
			  &d1DelegateOwnerBlob,				/* structure */
			  (TPM_STORE_FUNCTION_T)TPM_DelegateOwnerBlob_Store); /* store function */
	    break;
	  case TPM_TAG_DELG_KEY_BLOB:
	    returnCode = TPM_HMAC_GenerateStructure
			 (d1DelegateKeyBlob.integrityDigest,		/* HMAC */
			  tpm_state->tpm_permanent_data.tpmProof,	/* HMAC key */
			  &d1DelegateKeyBlob,				/* structure */
			  (TPM_STORE_FUNCTION_T)TPM_DelegateKeyBlob_Store);	/* store function */
	    break;
	}
    }
    /* If updating a delegate row, write back TPM_PERMANENT_DATA */
    if (inputData.size == sizeof(TPM_DELEGATE_INDEX)) {
	returnCode = TPM_PermanentAll_NVStore(tpm_state,
					      writeAllNV,
					      returnCode);
    }
    /* 10. If D1 is a blob recreate the blob and return it */
    else {
	if (returnCode == TPM_SUCCESS) {
	    switch (d1Tag) {
	      case TPM_TAG_DELEGATE_OWNER_BLOB:
		returnCode = TPM_DelegateOwnerBlob_Store(&outputDataSbuffer,
							 &d1DelegateOwnerBlob);
		break;
	      case TPM_TAG_DELG_KEY_BLOB:
		returnCode = TPM_DelegateKeyBlob_Store(&outputDataSbuffer,
						       &d1DelegateKeyBlob);
		break;
		/* default error tested above */
	    }
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DelegateUpdateVerification: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return outputSize and outputData */
	    returnCode = TPM_Sbuffer_AppendAsSizedBuffer(response, &outputDataSbuffer);
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&inputData);			/* @1 */
    TPM_DelegateOwnerBlob_Delete(&d1DelegateOwnerBlob); /* @2 */
    TPM_DelegateKeyBlob_Delete(&d1DelegateKeyBlob);	/* @3 */
    TPM_Sbuffer_Delete(&outputDataSbuffer);		/* @4 */
    return rcf;
}

/* 19.7 TPM_Delegate_VerifyDelegation rev 105

   VerifyDelegation interprets a delegate blob and returns success or failure, depending on whether
   the blob is currently valid. The delegate blob is NOT loaded into the TPM.
*/

TPM_RESULT TPM_Process_DelegateVerifyDelegation(tpm_state_t *tpm_state,
						TPM_STORE_BUFFER *response,
						TPM_TAG tag,
						uint32_t paramSize,
						TPM_COMMAND_CODE ordinal,
						unsigned char *command,
						TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_SIZED_BUFFER	delegation;	/* TPM_DELEGATE_KEY_BLOB or TPM_DELEGATE_OWNER_BLOB */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    unsigned char		*stream;		/* temp input stream */
    uint32_t			stream_size;
    TPM_STRUCTURE_TAG		d1Tag;			/* input structure tag */
    TPM_DELEGATE_OWNER_BLOB	d1DelegateOwnerBlob;
    TPM_DELEGATE_KEY_BLOB	d1DelegateKeyBlob;
    TPM_FAMILY_TABLE_ENTRY	*familyRow;		/* family table row containing familyID */
    TPM_FAMILY_ID		familyID;
    TPM_FAMILY_VERIFICATION	verificationCount = 0;
    TPM_DELEGATE_SENSITIVE	s1DelegateSensitive;
    
    /* output parameters  */
    uint32_t			outParamStart;		/* starting point of outParam's */
    uint32_t			outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;

    printf("TPM_Process_DelegateVerifyDelegation: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&delegation);			/* freed @1 */
    TPM_DelegateOwnerBlob_Init(&d1DelegateOwnerBlob);	/* freed @2 */
    TPM_DelegateKeyBlob_Init(&d1DelegateKeyBlob);	/* freed @3 */
    TPM_DelegateSensitive_Init(&s1DelegateSensitive);	/* freed @4 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get delegation parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&delegation, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALL);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_DelegateVerifyDelegation: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Determine the type of blob */
    if (returnCode == TPM_SUCCESS) {
	/* use a temporary copy so the original values are not moved */
	stream = delegation.buffer;
	stream_size = delegation.size;
	returnCode = TPM_Load16(&d1Tag, &stream, &stream_size); 
    }
    if (returnCode == TPM_SUCCESS) {
	/* use a temporary copy so the original values are not moved */
	stream = delegation.buffer;
	stream_size = delegation.size;
	switch (d1Tag) {
	    /* 1. If delegation -> tag is equal to TPM_TAG_DELEGATE_OWNER_BLOB then */
	  case TPM_TAG_DELEGATE_OWNER_BLOB:
	    /* a. Map D1 a TPM_DELEGATE_BLOB_OWNER to delegation */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_DelegateOwnerBlob_Load(&d1DelegateOwnerBlob,
							&stream, &stream_size);
	    }
	    if (returnCode == TPM_SUCCESS) {
		familyID = d1DelegateOwnerBlob.pub.familyID;
		verificationCount = d1DelegateOwnerBlob.pub.verificationCount;
	    }
	    break;
	    /* 2. Else if delegation -> tag = TPM_TAG_DELG_KEY_BLOB */
	  case TPM_TAG_DELG_KEY_BLOB:
	    /* a. Map D1 a TPM_DELEGATE_KEY_BLOB to delegation */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_DelegateKeyBlob_Load(&d1DelegateKeyBlob, &stream, &stream_size);
	    }
	    if (returnCode == TPM_SUCCESS) {
		familyID = d1DelegateKeyBlob.pub.familyID;
		verificationCount = d1DelegateKeyBlob.pub.verificationCount;
	    }
	    break;
	    /* 3. Else return TPM_BAD_PARAMETER */
	  default:
	    printf("TPM_Process_DelegateVerifyDelegation: Error, invalid tag %04hx\n", d1Tag);
	    returnCode = TPM_BAD_PARAMETER;
	    break;
	}
    }
    /* 4. Locate D1 -> familyID in the TPM_FAMILY_TABLE and set familyRow to indicate row, return
       TPM_BADINDEX if not found */
    /* 5. Set FR to TPM_FAMILY_TABLE.famTableRow[familyRow] */
    /* 6. If FR -> flags TPM_FAMFLAG_ENABLED is FALSE, return TPM_DISABLED_CMD */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_FamilyTable_GetEnabledEntry(&familyRow,
						     &(tpm_state->tpm_permanent_data.familyTable),
						     familyID);
    }
    /* 7. Validate that D1 -> pub -> verificationCount matches FR -> verificationCount, on mismatch
       return TPM_FAMILYCOUNT */
    if (returnCode == TPM_SUCCESS) {
	if (verificationCount != familyRow->verificationCount) {
	    printf("TPM_Process_DelegateVerifyDelegation: Error, "
		   "verificationCount mismatch %u %u\n",
		   verificationCount, familyRow->verificationCount);
	    returnCode = TPM_FAMILYCOUNT;
	}
    }
    /* 8. Validate the integrity of D1 */
    /* a. Copy D1 -> integrityDigest to H2 */
    /* b. Set D1 -> integrityDigest to NULL */
    /* c. Create H3 the HMAC of D1 using tpmProof as the secret */
    /* d. Compare H2 to H3 return TPM_AUTHFAIL on mismatch */
    if (returnCode == TPM_SUCCESS) {
	if (d1Tag == TPM_TAG_DELEGATE_OWNER_BLOB) {
	    returnCode = TPM_HMAC_CheckStructure
			 (tpm_state->tpm_permanent_data.tpmProof,		/* key */
			  &d1DelegateOwnerBlob,					/* structure */
			  d1DelegateOwnerBlob.integrityDigest,			/* expected */
			  (TPM_STORE_FUNCTION_T)TPM_DelegateOwnerBlob_Store,	/* store function */
			  TPM_AUTHFAIL);					/* error code */
	}
	else {
	    returnCode = TPM_HMAC_CheckStructure
			 (tpm_state->tpm_permanent_data.tpmProof,		/* key */
			  &d1DelegateKeyBlob,					/* structure */
			  d1DelegateKeyBlob.integrityDigest,			/* expected */
			  (TPM_STORE_FUNCTION_T)TPM_DelegateKeyBlob_Store,	/* store function */
			  TPM_AUTHFAIL);					/* error code */
	}
    }
    /* 9. Create S1 a TPM_DELEGATE_SENSITIVE area by decrypting D1 -> sensitiveArea using
       TPM_DELEGATE_KEY */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DelegateVerifyDelegation: Decrypting sensitiveArea\n");
	if (d1Tag == TPM_TAG_DELEGATE_OWNER_BLOB) {
	    returnCode =
		TPM_DelegateSensitive_DecryptEncData(&s1DelegateSensitive, 
						     &(d1DelegateOwnerBlob.sensitiveArea),
						     tpm_state->tpm_permanent_data.delegateKey);
	}
	else {
	    returnCode =
		TPM_DelegateSensitive_DecryptEncData(&s1DelegateSensitive, 
						     &(d1DelegateKeyBlob.sensitiveArea),
						     tpm_state->tpm_permanent_data.delegateKey);
	}
    }
    /* 10. Validate S1 values */
    /* a. S1 -> tag is TPM_TAG_DELEGATE_SENSITIVE */
    /* NOTE Done by TPM_DelegateSensitive_DecryptEncData() */
    /* b. Return TPM_BAD_PARAMETER on error */
    /* 11. Return TPM_SUCCESS */
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DelegateVerifyDelegation: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&delegation);		/* @1 */
    TPM_DelegateOwnerBlob_Delete(&d1DelegateOwnerBlob); /* @2 */
    TPM_DelegateKeyBlob_Delete(&d1DelegateKeyBlob);	/* @3 */
    TPM_DelegateSensitive_Delete(&s1DelegateSensitive); /* @4 */
    return rcf;
}
