/********************************************************************************/
/*										*/
/*			Hash/HMAC/Event Sequences	     			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: HashCommands.c $		*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2018				*/
/*										*/
/********************************************************************************/

#include "Tpm.h"
#include "HMAC_Start_fp.h"
#if CC_HMAC_Start  // Conditional expansion of this file
TPM_RC
TPM2_HMAC_Start(
		HMAC_Start_In   *in,            // IN: input parameter list
		HMAC_Start_Out  *out            // OUT: output parameter list
		)
{
    OBJECT                  *keyObject;
    TPMT_PUBLIC             *publicArea;
    TPM_ALG_ID               hashAlg;
    // Input Validation
    // Get HMAC key object and public area pointers
    keyObject = HandleToObject(in->handle);
    publicArea = &keyObject->publicArea;
    // Make sure that the key is an HMAC key
    if(publicArea->type != TPM_ALG_KEYEDHASH)
	return TPM_RCS_TYPE + RC_HMAC_Start_handle;
    // and that it is unrestricted
    if (IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT, restricted))
	return TPM_RCS_ATTRIBUTES + RC_HMAC_Start_handle;
    // and that it is a signing key
    if (!IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT, sign))
	return TPM_RCS_KEY + RC_HMAC_Start_handle;
    // See if the key has a default
    if(publicArea->parameters.keyedHashDetail.scheme.scheme == TPM_ALG_NULL)
	// it doesn't so use the input value
	hashAlg = in->hashAlg;
    else
	{
	    // key has a default so use it
	    hashAlg
		= publicArea->parameters.keyedHashDetail.scheme.details.hmac.hashAlg;
	    // and verify that the input was either the  TPM_ALG_NULL or the default
	    if(in->hashAlg != TPM_ALG_NULL && in->hashAlg != hashAlg)
		hashAlg = TPM_ALG_NULL;
	}
    // if we ended up without a hash algorithm then return an error
    if(hashAlg == TPM_ALG_NULL)
	return TPM_RCS_VALUE + RC_HMAC_Start_hashAlg;
    // Internal Data Update
    // Create a HMAC sequence object. A TPM_RC_OBJECT_MEMORY error may be
    // returned at this point
    return ObjectCreateHMACSequence(hashAlg,
				    keyObject,
				    &in->auth,
				    &out->sequenceHandle);
}
#endif // CC_HMAC_Start
#include "Tpm.h"
#include "MAC_Start_fp.h"
#if CC_MAC_Start  // Conditional expansion of this file
/* Error Returns Meaning */
/* TPM_RC_ATTRIBUTES key referenced by handle is not a signing key or is restricted */
/* TPM_RC_OBJECT_MEMORY no space to create an internal object */
/* TPM_RC_KEY key referenced by handle is not an HMAC key */
/* TPM_RC_VALUE hashAlg is not compatible with the hash algorithm of the scheme of the object
   referenced by handle */
TPM_RC
TPM2_MAC_Start(
	       MAC_Start_In   *in,            // IN: input parameter list
	       MAC_Start_Out  *out            // OUT: output parameter list
	       )
{
    OBJECT                  *keyObject;
    TPMT_PUBLIC             *publicArea;
    TPM_RC                   result;
    // Input Validation
    // Get HMAC key object and public area pointers
    keyObject = HandleToObject(in->handle);
    publicArea = &keyObject->publicArea;
    // Make sure that the key can do what is required
    result = CryptSelectMac(publicArea, &in->inScheme);
    // If the key is not able to do a MAC, indicate that the handle selects an
    // object that can't do a MAC
    if(result == TPM_RCS_TYPE)
	return TPM_RCS_TYPE + RC_MAC_Start_handle;
    // If there is another error type, indicate that the scheme and key are not
    // compatible
    if(result != TPM_RC_SUCCESS)
	return RcSafeAddToResult(result, RC_MAC_Start_inScheme);
    // Make sure that the key is not restricted
    if(IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT, restricted))
	return TPM_RCS_ATTRIBUTES + RC_MAC_Start_handle;
    // and that it is a signing key
    if(!IS_ATTRIBUTE(publicArea->objectAttributes, TPMA_OBJECT, sign))
	return TPM_RCS_KEY + RC_MAC_Start_handle;
    // Internal Data Update
    // Create a HMAC sequence object. A TPM_RC_OBJECT_MEMORY error may be
    // returned at this point
    return ObjectCreateHMACSequence(in->inScheme,
				    keyObject,
				    &in->auth,
				    &out->sequenceHandle);
}
#endif // CC_MAC_Start
#include "Tpm.h"
#include "HashSequenceStart_fp.h"
#if CC_HashSequenceStart  // Conditional expansion of this file
TPM_RC
TPM2_HashSequenceStart(
		       HashSequenceStart_In    *in,            // IN: input parameter list
		       HashSequenceStart_Out   *out            // OUT: output parameter list
		       )
{
    // Internal Data Update
    if(in->hashAlg == TPM_ALG_NULL)
	// Start a event sequence.  A TPM_RC_OBJECT_MEMORY error may be
	// returned at this point
	return ObjectCreateEventSequence(&in->auth, &out->sequenceHandle);
    // Start a hash sequence.  A TPM_RC_OBJECT_MEMORY error may be
    // returned at this point
    return ObjectCreateHashSequence(in->hashAlg, &in->auth, &out->sequenceHandle);
}
#endif // CC_HashSequenceStart
#include "Tpm.h"
#include "SequenceUpdate_fp.h"
#if CC_SequenceUpdate  // Conditional expansion of this file
TPM_RC
TPM2_SequenceUpdate(
		    SequenceUpdate_In   *in             // IN: input parameter list
		    )
{
    OBJECT                  *object;
    HASH_OBJECT             *hashObject;
    // Input Validation
    // Get sequence object pointer
    object = HandleToObject(in->sequenceHandle);
    hashObject = (HASH_OBJECT *)object;
    // Check that referenced object is a sequence object.
    if(!ObjectIsSequence(object))
	return TPM_RCS_MODE + RC_SequenceUpdate_sequenceHandle;
    // Internal Data Update
    if(object->attributes.eventSeq == SET)
	{
	    // Update event sequence object
	    UINT32           i;
	    for(i = 0; i < HASH_COUNT; i++)
	        {
	            // Update sequence object
	            CryptDigestUpdate2B(&hashObject->state.hashState[i], &in->buffer.b);
	        }
	}
    else
	{
	    // Update hash/HMAC sequence object
	    if(hashObject->attributes.hashSeq == SET)
	        {
	            // Is this the first block of the sequence
	            if(hashObject->attributes.firstBlock == CLEAR)
			{
			    // If so, indicate that first block was received
			    hashObject->attributes.firstBlock = SET;
			    // Check the first block to see if the first block can contain
			    // the TPM_GENERATED_VALUE.  If it does, it is not safe for
			    // a ticket.
			    if(TicketIsSafe(&in->buffer.b))
				hashObject->attributes.ticketSafe = SET;
			}
	            // Update sequence object hash/HMAC stack
	            CryptDigestUpdate2B(&hashObject->state.hashState[0], &in->buffer.b);
	        }
	    else if(object->attributes.hmacSeq == SET)
	        {
	            // Update sequence object HMAC stack
	            CryptDigestUpdate2B(&hashObject->state.hmacState.hashState,
	                                &in->buffer.b);
	        }
	}
    return TPM_RC_SUCCESS;
}
#endif // CC_SequenceUpdate
#include "Tpm.h"
#include "SequenceComplete_fp.h"
#if CC_SequenceComplete  // Conditional expansion of this file
/* Error Returns Meaning */
/* TPM_RC_MODE sequenceHandle does not reference a hash or HMAC sequence object */
TPM_RC
TPM2_SequenceComplete(
		      SequenceComplete_In     *in,            // IN: input parameter list
		      SequenceComplete_Out    *out            // OUT: output parameter list
		      )
{
    HASH_OBJECT                      *hashObject;
    // Input validation
    // Get hash object pointer
    hashObject = (HASH_OBJECT *)HandleToObject(in->sequenceHandle);
    // input handle must be a hash or HMAC sequence object.
    if(hashObject->attributes.hashSeq == CLEAR
       && hashObject->attributes.hmacSeq == CLEAR)
	return TPM_RCS_MODE + RC_SequenceComplete_sequenceHandle;
    // Command Output
    if(hashObject->attributes.hashSeq == SET)           // sequence object for hash
	{
	    // Get the hash algorithm before the algorithm is lost in CryptHashEnd
	    TPM_ALG_ID       hashAlg = hashObject->state.hashState[0].hashAlg;
	    // Update last piece of the data
	    CryptDigestUpdate2B(&hashObject->state.hashState[0], &in->buffer.b);
	    // Complete hash
	    out->result.t.size = CryptHashEnd(&hashObject->state.hashState[0],
					      sizeof(out->result.t.buffer),
					      out->result.t.buffer);
	    // Check if the first block of the sequence has been received
	    if(hashObject->attributes.firstBlock == CLEAR)
		{
		    // If not, then this is the first block so see if it is 'safe'
		    // to sign.
		    if(TicketIsSafe(&in->buffer.b))
			hashObject->attributes.ticketSafe = SET;
		}
	    // Output ticket
	    out->validation.tag = TPM_ST_HASHCHECK;
	    out->validation.hierarchy = in->hierarchy;
	    if(in->hierarchy == TPM_RH_NULL)
		{
		    // Ticket is not required
		    out->validation.digest.t.size = 0;
		}
	    else if(hashObject->attributes.ticketSafe == CLEAR)
		{
		    // Ticket is not safe to generate
		    out->validation.hierarchy = TPM_RH_NULL;
		    out->validation.digest.t.size = 0;
		}
	    else
		{
		    // Compute ticket
		    TicketComputeHashCheck(out->validation.hierarchy, hashAlg,
					   &out->result, &out->validation);
		}
	}
    else
	{
	    //   Update last piece of data
	    CryptDigestUpdate2B(&hashObject->state.hmacState.hashState, &in->buffer.b);
#if !SMAC_IMPLEMENTED
	    // Complete HMAC
	    out->result.t.size = CryptHmacEnd(&(hashObject->state.hmacState),
					      sizeof(out->result.t.buffer),
					      out->result.t.buffer);
#else
	    // Complete the MAC
	    out->result.t.size = CryptMacEnd(&hashObject->state.hmacState,
					     sizeof(out->result.t.buffer),
					     out->result.t.buffer);
#endif
	    // No ticket is generated for HMAC sequence
	    out->validation.tag = TPM_ST_HASHCHECK;
	    out->validation.hierarchy = TPM_RH_NULL;
	    out->validation.digest.t.size = 0;
	}
    // Internal Data Update
    // mark sequence object as evict so it will be flushed on the way out
    hashObject->attributes.evict = SET;
    return TPM_RC_SUCCESS;
}
#endif // CC_SequenceComplete
#include "Tpm.h"
#include "EventSequenceComplete_fp.h"
#if CC_EventSequenceComplete  // Conditional expansion of this file
TPM_RC
TPM2_EventSequenceComplete(
			   EventSequenceComplete_In    *in,            // IN: input parameter list
			   EventSequenceComplete_Out   *out            // OUT: output parameter list
			   )
{
    HASH_OBJECT         *hashObject;
    UINT32               i;
    TPM_ALG_ID           hashAlg;
    // Input validation
    // get the event sequence object pointer
    hashObject = (HASH_OBJECT *)HandleToObject(in->sequenceHandle);
    // input handle must reference an event sequence object
    if(hashObject->attributes.eventSeq != SET)
	return TPM_RCS_MODE + RC_EventSequenceComplete_sequenceHandle;
    // see if a PCR extend is requested in call
    if(in->pcrHandle != TPM_RH_NULL)
	{
	    // see if extend of the PCR is allowed at the locality of the command,
	    if(!PCRIsExtendAllowed(in->pcrHandle))
		return TPM_RC_LOCALITY;
	    // if an extend is going to take place, then check to see if there has
	    // been an orderly shutdown. If so, and the selected PCR is one of the
	    // state saved PCR, then the orderly state has to change. The orderly state
	    // does not change for PCR that are not preserved.
	    // NOTE: This doesn't just check for Shutdown(STATE) because the orderly
	    // state will have to change if this is a state-saved PCR regardless
	    // of the current state. This is because a subsequent Shutdown(STATE) will
	    // check to see if there was an orderly shutdown and not do anything if
	    // there was. So, this must indicate that a future Shutdown(STATE) has
	    // something to do.
	    if(PCRIsStateSaved(in->pcrHandle))
		RETURN_IF_ORDERLY;
	}
    // Command Output
    out->results.count = 0;
    for(i = 0; i < HASH_COUNT; i++)
	{
	    hashAlg = CryptHashGetAlgByIndex(i);
	    // Update last piece of data
	    CryptDigestUpdate2B(&hashObject->state.hashState[i], &in->buffer.b);
	    // Complete hash
	    out->results.digests[out->results.count].hashAlg = hashAlg;
	    CryptHashEnd(&hashObject->state.hashState[i],
			 CryptHashGetDigestSize(hashAlg),
			 (BYTE *)&out->results.digests[out->results.count].digest);
	    // Extend PCR
	    if(in->pcrHandle != TPM_RH_NULL)
		PCRExtend(in->pcrHandle, hashAlg,
			  CryptHashGetDigestSize(hashAlg),
			  (BYTE *)&out->results.digests[out->results.count].digest);
	    out->results.count++;
	}
    // Internal Data Update
    // mark sequence object as evict so it will be flushed on the way out
    hashObject->attributes.evict = SET;
    return TPM_RC_SUCCESS;
}
#endif // CC_EventSequenceComplete
