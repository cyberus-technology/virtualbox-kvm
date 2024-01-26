/********************************************************************************/
/*										*/
/*			Context Management Command Support   			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Context_spt.c $		*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2020				*/
/*										*/
/********************************************************************************/

/* 7.3.1 Includes */
#include "Tpm.h"
#include "Context_spt_fp.h"
/* 7.3.2 Functions */
/* 7.3.2.1 ComputeContextProtectionKey() */
/* This function retrieves the symmetric protection key for context encryption It is used by
   TPM2_ContextSave() and TPM2_ContextLoad() to create the symmetric encryption key and iv */
void
ComputeContextProtectionKey(
			    TPMS_CONTEXT    *contextBlob,   // IN: context blob
			    TPM2B_SYM_KEY   *symKey,        // OUT: the symmetric key
			    TPM2B_IV        *iv             // OUT: the IV.
			    )
{
    UINT16           symKeyBits;    // number of bits in the parent's
    //   symmetric key
    TPM2B_PROOF      *proof = NULL;  // the proof value to use. Is null for
    //   everything but a primary object in
    //   the Endorsement Hierarchy
    BYTE             kdfResult[sizeof(TPMU_HA) * 2];// Value produced by the KDF
    TPM2B_DATA       sequence2B, handle2B;
    // Get proof value
    proof = HierarchyGetProof(contextBlob->hierarchy);

    // Get sequence value in 2B format
    sequence2B.t.size = sizeof(contextBlob->sequence);
    cAssert(sequence2B.t.size <= sizeof(sequence2B.t.buffer));
    MemoryCopy(sequence2B.t.buffer, &contextBlob->sequence, sequence2B.t.size);
    
    // Get handle value in 2B format
    handle2B.t.size = sizeof(contextBlob->savedHandle);
    cAssert(handle2B.t.size <= sizeof(handle2B.t.buffer));
    MemoryCopy(handle2B.t.buffer, &contextBlob->savedHandle, handle2B.t.size);

    // Get the symmetric encryption key size
    symKey->t.size = CONTEXT_ENCRYPT_KEY_BYTES;
    symKeyBits = CONTEXT_ENCRYPT_KEY_BITS;
    // Get the size of the IV for the algorithm
    iv->t.size = CryptGetSymmetricBlockSize(CONTEXT_ENCRYPT_ALG, symKeyBits);
    // KDFa to generate symmetric key and IV value
    CryptKDFa(CONTEXT_INTEGRITY_HASH_ALG, &proof->b, CONTEXT_KEY, &sequence2B.b,
	      &handle2B.b, (symKey->t.size + iv->t.size) * 8, kdfResult, NULL,
	      FALSE);
    // Copy part of the returned value as the key
    pAssert(symKey->t.size <= sizeof(symKey->t.buffer));
    MemoryCopy(symKey->t.buffer, kdfResult, symKey->t.size);
    // Copy the rest as the IV
    pAssert(iv->t.size <= sizeof(iv->t.buffer));
    MemoryCopy(iv->t.buffer, &kdfResult[symKey->t.size], iv->t.size);
    return;
}
/* 7.3.2.2 ComputeContextIntegrity() */
/* Generate the integrity hash for a context It is used by TPM2_ContextSave() to create an integrity
   hash and by TPM2_ContextLoad() to compare an integrity hash */
void
ComputeContextIntegrity(
			TPMS_CONTEXT    *contextBlob,   // IN: context blob
			TPM2B_DIGEST    *integrity      // OUT: integrity
			)
{
    HMAC_STATE          hmacState;
    TPM2B_PROOF         *proof;
    UINT16              integritySize;
    // Get proof value
    proof = HierarchyGetProof(contextBlob->hierarchy);
    // Start HMAC
    integrity->t.size = CryptHmacStart2B(&hmacState, CONTEXT_INTEGRITY_HASH_ALG,
					 &proof->b);
    // Compute integrity size at the beginning of context blob
    integritySize = sizeof(integrity->t.size) + integrity->t.size;
    // Adding total reset counter so that the context cannot be
    // used after a TPM Reset
    CryptDigestUpdateInt(&hmacState.hashState, sizeof(gp.totalResetCount),
			 gp.totalResetCount);
    // If this is a ST_CLEAR object, add the clear count
    // so that this contest cannot be loaded after a TPM Restart
    if(contextBlob->savedHandle == 0x80000002)
	CryptDigestUpdateInt(&hmacState.hashState, sizeof(gr.clearCount),
			     gr.clearCount);
    // Adding sequence number to the HMAC to make sure that it doesn't
    // get changed
    CryptDigestUpdateInt(&hmacState.hashState, sizeof(contextBlob->sequence),
			 contextBlob->sequence);
    // Protect the handle
    CryptDigestUpdateInt(&hmacState.hashState, sizeof(contextBlob->savedHandle),
			 contextBlob->savedHandle);
    // Adding sensitive contextData, skip the leading integrity area
    CryptDigestUpdate(&hmacState.hashState,
		      contextBlob->contextBlob.t.size - integritySize,
		      contextBlob->contextBlob.t.buffer + integritySize);
    // Complete HMAC
    CryptHmacEnd2B(&hmacState, &integrity->b);
    return;
}
#if 0
/* 7.3.2.3 SequenceDataExport() */
/* This function is used scan through the sequence object and either modify the hash state data for
   export (contextSave) or to import it into the internal format (contextLoad). This function should
   only be called after the sequence object has been copied to the context buffer (contextSave) or
   from the context buffer into the sequence object. The presumption is that the context buffer
   version of the data is the same size as the internal representation so nothing outsize of the
   hash context area gets modified. */
void
SequenceDataExport(
		   HASH_OBJECT         *object,        // IN: an internal hash object
		   HASH_OBJECT_BUFFER  *exportObject   // OUT: a sequence context in a buffer
		   )
{
    // If the hash object is not an event, then only one hash context is needed
    int                   count = (object->attributes.eventSeq) ? HASH_COUNT : 1;
    for(count--; count >= 0; count--)
	{
	    HASH_STATE          *hash = &object->state.hashState[count];
	    size_t               offset = (BYTE *)hash - (BYTE *)object;
	    BYTE                *exportHash = &((BYTE *)exportObject)[offset];
	    CryptHashExportState(hash, (EXPORT_HASH_STATE *)exportHash);
	}
}
/* 7.3.2.4 SequenceDataImport() */
/* This function is used scan through the sequence object and either modify the hash state data for
   export (contextSave) or to import it into the internal format (contextLoad). This function should
   only be called after the sequence object has been copied to the context buffer (contextSave) or
   from the context buffer into the sequence object. The presumption is that the context buffer
   version of the data is the same size as the internal representation so nothing outsize of the
   hash context area gets modified. */
void
SequenceDataImport(
		   HASH_OBJECT         *object,        // IN/OUT: an internal hash object
		   HASH_OBJECT_BUFFER  *exportObject   // IN/OUT: a sequence context in a buffer
		   )
{
    // If the hash object is not an event, then only one hash context is needed
    int                   count = (object->attributes.eventSeq) ? HASH_COUNT : 1;
    for(count--; count >= 0; count--)
	{
	    HASH_STATE          *hash = &object->state.hashState[count];
	    size_t               offset = (BYTE *)hash - (BYTE *)object;
	    BYTE                *importHash = &((BYTE *)exportObject)[offset];
	    //
	    CryptHashImportState(hash, (EXPORT_HASH_STATE *)importHash);
	}
}
#endif
