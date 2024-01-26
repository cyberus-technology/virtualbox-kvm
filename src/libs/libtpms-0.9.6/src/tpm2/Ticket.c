/********************************************************************************/
/*										*/
/*		Functions used for ticket computations.				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Ticket.c $		*/
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

/* 10.2.23 Ticket.c */
/* 10.2.23.1 Introduction */
/* This clause contains the functions used for ticket computations. */
/* 10.2.23.2 Includes */
#include "Tpm.h"
/* 10.2.23.3 Functions */
/* 10.2.23.3.1 TicketIsSafe() */
/* This function indicates if producing a ticket is safe. It checks if the leading bytes of an input
   buffer is TPM_GENERATED_VALUE or its substring of canonical form.  If so, it is not safe to
   produce ticket for an input buffer claiming to be TPM generated buffer */
/* Return Values Meaning */
/* TRUE It is safe to produce ticket */
/* FALSE It is not safe to produce ticket */
BOOL
TicketIsSafe(
	     TPM2B           *buffer
	     )
{
    TPM_CONSTANTS32 valueToCompare = TPM_GENERATED_VALUE;
    BYTE            bufferToCompare[sizeof(valueToCompare)];
    BYTE            *marshalBuffer;
    //
    // If the buffer size is less than the size of TPM_GENERATED_VALUE, assume
    // it is not safe to generate a ticket
    if(buffer->size < sizeof(valueToCompare))
	return FALSE;
    marshalBuffer = bufferToCompare;
    TPM_CONSTANTS32_Marshal(&valueToCompare, &marshalBuffer, NULL);
    if(MemoryEqual(buffer->buffer, bufferToCompare, sizeof(valueToCompare)))
	return FALSE;
    else
	return TRUE;
}
/* 10.2.23.3.2 TicketComputeVerified() */
/* This function creates a TPMT_TK_VERIFIED ticket. */
void
TicketComputeVerified(
		      TPMI_RH_HIERARCHY    hierarchy,     // IN: hierarchy constant for ticket
		      TPM2B_DIGEST        *digest,        // IN: digest
		      TPM2B_NAME          *keyName,       // IN: name of key that signed the values
		      TPMT_TK_VERIFIED    *ticket         // OUT: verified ticket
		      )
{
    TPM2B_PROOF          *proof;
    HMAC_STATE           hmacState;
    //
    // Fill in ticket fields
    ticket->tag = TPM_ST_VERIFIED;
    ticket->hierarchy = hierarchy;
    proof = HierarchyGetProof(hierarchy);
    // Start HMAC using the proof value of the hierarchy as the HMAC key
    ticket->digest.t.size = CryptHmacStart2B(&hmacState, CONTEXT_INTEGRITY_HASH_ALG,
					     &proof->b);
    //  TPM_ST_VERIFIED
    CryptDigestUpdateInt(&hmacState, sizeof(TPM_ST), ticket->tag);
    //  digest
    CryptDigestUpdate2B(&hmacState.hashState, &digest->b);
    // key name
    CryptDigestUpdate2B(&hmacState.hashState, &keyName->b);
    // done
    CryptHmacEnd2B(&hmacState, &ticket->digest.b);
    return;
}
/* 10.2.23.3.3 TicketComputeAuth() */
/* This function creates a TPMT_TK_AUTH ticket. */
void
TicketComputeAuth(
		  TPM_ST               type,          // IN: the type of ticket.
		  TPMI_RH_HIERARCHY    hierarchy,     // IN: hierarchy constant for ticket
		  UINT64               timeout,       // IN: timeout
		  BOOL                 expiresOnReset,// IN: flag to indicate if ticket expires on
		  //      TPM Reset
		  TPM2B_DIGEST        *cpHashA,       // IN: input cpHashA
		  TPM2B_NONCE         *policyRef,     // IN: input policyRef
		  TPM2B_NAME          *entityName,    // IN: name of entity
		  TPMT_TK_AUTH        *ticket         // OUT: Created ticket
		  )
{
    TPM2B_PROOF          *proof;
    HMAC_STATE           hmacState;
    //
    // Get proper proof
    proof = HierarchyGetProof(hierarchy);
    // Fill in ticket fields
    ticket->tag = type;
    ticket->hierarchy = hierarchy;
    // Start HMAC with hierarchy proof as the HMAC key
    ticket->digest.t.size = CryptHmacStart2B(&hmacState, CONTEXT_INTEGRITY_HASH_ALG,
					     &proof->b);
    //  TPM_ST_AUTH_SECRET or TPM_ST_AUTH_SIGNED,
    CryptDigestUpdateInt(&hmacState, sizeof(UINT16), ticket->tag);
    // cpHash
    CryptDigestUpdate2B(&hmacState.hashState, &cpHashA->b);
    //  policyRef
    CryptDigestUpdate2B(&hmacState.hashState, &policyRef->b);
    //  keyName
    CryptDigestUpdate2B(&hmacState.hashState, &entityName->b);
    //  timeout
    CryptDigestUpdateInt(&hmacState, sizeof(timeout), timeout);
    if(timeout != 0)
	{
	    //  epoch
	    CryptDigestUpdateInt(&hmacState.hashState, sizeof(CLOCK_NONCE),
				 g_timeEpoch);
	    // reset count
	    if(expiresOnReset)
		CryptDigestUpdateInt(&hmacState.hashState, sizeof(gp.totalResetCount),
				     gp.totalResetCount);
	}
    // done
    CryptHmacEnd2B(&hmacState, &ticket->digest.b);
    return;
}
/* 10.2.23.3.4 TicketComputeHashCheck() */
/* This function creates a TPMT_TK_HASHCHECK ticket. */
void
TicketComputeHashCheck(
		       TPMI_RH_HIERARCHY    hierarchy,     // IN: hierarchy constant for ticket
		       TPM_ALG_ID           hashAlg,       // IN: the hash algorithm for 'digest'
		       TPM2B_DIGEST        *digest,        // IN: input digest
		       TPMT_TK_HASHCHECK   *ticket         // OUT: Created ticket
		       )
{
    TPM2B_PROOF          *proof;
    HMAC_STATE           hmacState;
    //
    // Get proper proof
    proof = HierarchyGetProof(hierarchy);
    // Fill in ticket fields
    ticket->tag = TPM_ST_HASHCHECK;
    ticket->hierarchy = hierarchy;
    // Start HMAC using hierarchy proof as HMAC key
    ticket->digest.t.size = CryptHmacStart2B(&hmacState, CONTEXT_INTEGRITY_HASH_ALG,
					     &proof->b);
    //  TPM_ST_HASHCHECK
    CryptDigestUpdateInt(&hmacState, sizeof(TPM_ST), ticket->tag);
    //  hash algorithm
    CryptDigestUpdateInt(&hmacState, sizeof(hashAlg), hashAlg);
    //  digest
    CryptDigestUpdate2B(&hmacState.hashState, &digest->b);
    // done
    CryptHmacEnd2B(&hmacState, &ticket->digest.b);
    return;
}
/* 10.2.23.3.5 TicketComputeCreation() */
/* This function creates a TPMT_TK_CREATION ticket. */
void
TicketComputeCreation(
		      TPMI_RH_HIERARCHY    hierarchy,     // IN: hierarchy for ticket
		      TPM2B_NAME          *name,          // IN: object name
		      TPM2B_DIGEST        *creation,      // IN: creation hash
		      TPMT_TK_CREATION    *ticket         // OUT: created ticket
		      )
{
    TPM2B_PROOF          *proof;
    HMAC_STATE           hmacState;
    // Get proper proof
    proof = HierarchyGetProof(hierarchy);
    // Fill in ticket fields
    ticket->tag = TPM_ST_CREATION;
    ticket->hierarchy = hierarchy;
    // Start HMAC using hierarchy proof as HMAC key
    ticket->digest.t.size = CryptHmacStart2B(&hmacState, CONTEXT_INTEGRITY_HASH_ALG,
					     &proof->b);
    //  TPM_ST_CREATION
    CryptDigestUpdateInt(&hmacState, sizeof(TPM_ST), ticket->tag);
    //  name if provided
    if(name != NULL)
	CryptDigestUpdate2B(&hmacState.hashState, &name->b);
    //  creation hash
    CryptDigestUpdate2B(&hmacState.hashState, &creation->b);
    // Done
    CryptHmacEnd2B(&hmacState, &ticket->digest.b);
    return;
}
