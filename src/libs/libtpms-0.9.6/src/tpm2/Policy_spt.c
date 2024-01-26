/********************************************************************************/
/*										*/
/*			  Policy Command Support    				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Policy_spt.c $		*/
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

/* 7.4 Policy Command Support (Policy_spt.c) */
#include "Tpm.h"
#include "Policy_spt_fp.h"
#include "PolicySigned_fp.h"
#include "PolicySecret_fp.h"
#include "PolicyTicket_fp.h"
/* 7.4.1 PolicyParameterChecks() */
/* This function validates the common parameters of TPM2_PolicySiged() and TPM2_PolicySecret(). The
   common parameters are nonceTPM, expiration, and cpHashA. */
TPM_RC
PolicyParameterChecks(
		      SESSION         *session,
		      UINT64           authTimeout,
		      TPM2B_DIGEST    *cpHashA,
		      TPM2B_NONCE     *nonce,
		      TPM_RC           blameNonce,
		      TPM_RC           blameCpHash,
		      TPM_RC           blameExpiration
		      )
{
    // Validate that input nonceTPM is correct if present
    if(nonce != NULL && nonce->t.size != 0)
	{
	    if(!MemoryEqual2B(&nonce->b, &session->nonceTPM.b))
		return TPM_RCS_NONCE + blameNonce;
	}
    // If authTimeout is set (expiration != 0...
    if(authTimeout != 0)
	{
	    // Validate input expiration.
	    // Cannot compare time if clock stop advancing.  A TPM_RC_NV_UNAVAILABLE
	    // or TPM_RC_NV_RATE error may be returned here.
	    RETURN_IF_NV_IS_NOT_AVAILABLE;
	    // if the time has already passed or the time epoch has changed then the
	    // time value is no longer good.
	    if((authTimeout < g_time)
	       || (session->epoch != g_timeEpoch))
		return TPM_RCS_EXPIRED + blameExpiration;
	}
    // If the cpHash is present, then check it
    if(cpHashA != NULL && cpHashA->t.size != 0)
	{
	    // The cpHash input has to have the correct size
	    if(cpHashA->t.size != session->u2.policyDigest.t.size)
		return TPM_RCS_SIZE + blameCpHash;
	    // If the cpHash has already been set, then this input value
	    // must match the current value.
	    if(session->u1.cpHash.b.size != 0
	       && !MemoryEqual2B(&cpHashA->b, &session->u1.cpHash.b))
		return TPM_RC_CPHASH;
	}
    return TPM_RC_SUCCESS;
}
/* 7.4.2 PolicyContextUpdate() */
/* Update policy hash Update the policyDigest in policy session by extending policyRef and
   objectName to it. This will also update the cpHash if it is present. */
void
PolicyContextUpdate(
		    TPM_CC           commandCode,   // IN: command code
		    TPM2B_NAME      *name,          // IN: name of entity
		    TPM2B_NONCE     *ref,           // IN: the reference data
		    TPM2B_DIGEST    *cpHash,        // IN: the cpHash (optional)
		    UINT64           policyTimeout, // IN: the timeout value for the policy
		    SESSION         *session        // IN/OUT: policy session to be updated
		    )
{
    HASH_STATE           hashState;
    // Start hash
    CryptHashStart(&hashState, session->authHashAlg);
    // policyDigest size should always be the digest size of session hash algorithm.
    pAssert(session->u2.policyDigest.t.size
	    == CryptHashGetDigestSize(session->authHashAlg));
    // add old digest
    CryptDigestUpdate2B(&hashState, &session->u2.policyDigest.b);
    // add commandCode
    CryptDigestUpdateInt(&hashState, sizeof(commandCode), commandCode);
    // add name if applicable
    if(name != NULL)
	CryptDigestUpdate2B(&hashState, &name->b);
    // Complete the digest and get the results
    CryptHashEnd2B(&hashState, &session->u2.policyDigest.b);
    // If the policy reference is not null, do a second update to the digest.
    if(ref != NULL)
	{
	    // Start second hash computation
	    CryptHashStart(&hashState, session->authHashAlg);
	    // add policyDigest
	    CryptDigestUpdate2B(&hashState, &session->u2.policyDigest.b);
	    // add policyRef
	    CryptDigestUpdate2B(&hashState, &ref->b);
	    // Complete second digest
	    CryptHashEnd2B(&hashState, &session->u2.policyDigest.b);
	}
    // Deal with the cpHash. If the cpHash value is present
    // then it would have already been checked to make sure that
    // it is compatible with the current value so all we need
    // to do here is copy it and set the isCpHashDefined attribute
    if(cpHash != NULL && cpHash->t.size != 0)
	{
	    session->u1.cpHash = *cpHash;
	    session->attributes.isCpHashDefined = SET;
	}
    // update the timeout if it is specified
    if(policyTimeout != 0)
	{
	    // If the timeout has not been set, then set it to the new value
	    // than the current timeout then set it to the new value
	    if(session->timeout == 0 || session->timeout > policyTimeout)
		session->timeout = policyTimeout;
	}
    return;
}
/* 7.4.2.1 ComputeAuthTimeout() */
/* This function is used to determine what the authorization timeout value for the session should
   be. */
UINT64
ComputeAuthTimeout(
		   SESSION         *session,               // IN: the session containing the time
		   //     values
		   INT32            expiration,            // IN: either the number of seconds from
		   //     the start of the session or the
		   //     time in g_timer;
		   TPM2B_NONCE     *nonce                  // IN: indicator of the time base
		   )
{
    UINT64           policyTime;
    // If no expiration, policy time is 0
    if(expiration == 0)
	policyTime = 0;
    else
	{
	    if(expiration < 0) {
	        if (expiration == (INT32)0x80000000) /* libtpms changed begin; ubsan */
	            expiration++;                    /* libtpms changed end */
		expiration = -expiration;
	    }
	    if(nonce->t.size == 0)
		// The input time is absolute Time (not Clock), but it is expressed
		// in seconds. To make sure that we don't time out too early, take the
		// current value of milliseconds in g_time and add that to the input
		// seconds value.
		policyTime = (((UINT64)expiration) * 1000) + g_time % 1000;
	    else
		// The policy timeout is the absolute value of the expiration in seconds
		// added to the start time of the policy.
		policyTime = session->startTime + (((UINT64)expiration) * 1000);
	}
    return policyTime;
}
/* 7.4.2.2 PolicyDigestClear() */
/* Function to reset the policyDigest of a session */
void
PolicyDigestClear(
		  SESSION         *session
		  )
{
    session->u2.policyDigest.t.size = CryptHashGetDigestSize(session->authHashAlg);
    MemorySet(session->u2.policyDigest.t.buffer, 0,
	      session->u2.policyDigest.t.size);
}

/* 7.4.2.5	PolicySptCheckCondition() */
/* Checks to see if the condition in the policy is satisfied. */

BOOL
PolicySptCheckCondition(
			TPM_EO          operation,
			BYTE            *opA,
			BYTE            *opB,
			UINT16           size
			)
{
    // Arithmetic Comparison
    switch(operation)
	{
	  case TPM_EO_EQ:
	    // compare A = B
	    return (UnsignedCompareB(size, opA, size, opB) == 0);
	    break;
	  case TPM_EO_NEQ:
	    // compare A != B
	    return (UnsignedCompareB(size, opA, size, opB) != 0);
	    break;
	  case TPM_EO_SIGNED_GT:
	    // compare A > B signed
	    return (SignedCompareB(size, opA, size, opB) > 0);
	    break;
	  case TPM_EO_UNSIGNED_GT:
	    // compare A > B unsigned
	    return (UnsignedCompareB(size, opA, size, opB) > 0);
	    break;
	  case TPM_EO_SIGNED_LT:
	    // compare A < B signed
	    return (SignedCompareB(size, opA, size, opB) < 0);
	    break;
	  case TPM_EO_UNSIGNED_LT:
	    // compare A < B unsigned
	    return (UnsignedCompareB(size, opA, size, opB) < 0);
	    break;
	  case TPM_EO_SIGNED_GE:
	    // compare A >= B signed
	    return (SignedCompareB(size, opA, size, opB) >= 0);
	    break;
	  case TPM_EO_UNSIGNED_GE:
	    // compare A >= B unsigned
	    return (UnsignedCompareB(size, opA, size, opB) >= 0);
	    break;
	  case TPM_EO_SIGNED_LE:
	    // compare A <= B signed
	    return (SignedCompareB(size, opA, size, opB) <= 0);
	    break;
	  case TPM_EO_UNSIGNED_LE:
	    // compare A <= B unsigned
	    return (UnsignedCompareB(size, opA, size, opB) <= 0);
	    break;
	  case TPM_EO_BITSET:
	    // All bits SET in B are SET in A. ((A&B)=B)
	      {
		  UINT32 i;
		  for(i = 0; i < size; i++)
		      if((opA[i] & opB[i]) != opB[i])
			  return FALSE;
	      }
	      break;
	  case TPM_EO_BITCLEAR:
	    // All bits SET in B are CLEAR in A. ((A&B)=0)
	      {
		  UINT32 i;
		  for(i = 0; i < size; i++)
		      if((opA[i] & opB[i]) != 0)
			  return FALSE;
	      }
	      break;
	  default:
	    FAIL(FATAL_ERROR_INTERNAL);
	    break;
	}
    return TRUE;
}
