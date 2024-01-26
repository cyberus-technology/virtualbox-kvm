/********************************************************************************/
/*										*/
/*			   Attestation Commands  				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: AttestationCommands.c $	*/
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
#include "Attest_spt_fp.h"
#include "Certify_fp.h"
#if CC_Certify  // Conditional expansion of this file
TPM_RC
TPM2_Certify(
	     Certify_In      *in,            // IN: input parameter list
	     Certify_Out     *out            // OUT: output parameter list
	     )
{
    TPMS_ATTEST             certifyInfo;
    OBJECT                  *signObject = HandleToObject(in->signHandle);
    OBJECT                  *certifiedObject = HandleToObject(in->objectHandle);
    // Input validation
    if(!IsSigningObject(signObject))
	return TPM_RCS_KEY + RC_Certify_signHandle;
    if(!CryptSelectSignScheme(signObject, &in->inScheme))
	return TPM_RCS_SCHEME + RC_Certify_inScheme;
    // Command Output
    // Filling in attest information
    // Common fields
    FillInAttestInfo(in->signHandle, &in->inScheme, &in->qualifyingData,
		     &certifyInfo);
    // Certify specific fields
    certifyInfo.type = TPM_ST_ATTEST_CERTIFY;
    // NOTE: the certified object is not allowed to be TPM_ALG_NULL so
    // 'certifiedObject' will never be NULL
    certifyInfo.attested.certify.name = certifiedObject->name;

    // When using an anonymous signing scheme, need to set the qualified Name to the
    // empty buffer to avoid correlation between keys
    if(CryptIsSchemeAnonymous(in->inScheme.scheme))
	certifyInfo.attested.certify.qualifiedName.t.size = 0;
    else
	certifyInfo.attested.certify.qualifiedName = certifiedObject->qualifiedName;
    
    // Sign attestation structure.  A NULL signature will be returned if
    // signHandle is TPM_RH_NULL.  A TPM_RC_NV_UNAVAILABLE, TPM_RC_NV_RATE,
    // TPM_RC_VALUE, TPM_RC_SCHEME or TPM_RC_ATTRIBUTES error may be returned
    // by SignAttestInfo()
    return SignAttestInfo(signObject, &in->inScheme, &certifyInfo,
			  &in->qualifyingData, &out->certifyInfo, &out->signature);
}
#endif // CC_Certify
#include "Tpm.h"
#include "Attest_spt_fp.h"
#include "CertifyCreation_fp.h"
#if CC_CertifyCreation  // Conditional expansion of this file
TPM_RC
TPM2_CertifyCreation(
		     CertifyCreation_In      *in,            // IN: input parameter list
		     CertifyCreation_Out     *out            // OUT: output parameter list
		     )
{
    TPMT_TK_CREATION        ticket;
    TPMS_ATTEST             certifyInfo;
    OBJECT                  *certified = HandleToObject(in->objectHandle);
    OBJECT                  *signObject = HandleToObject(in->signHandle);
    // Input Validation
    if(!IsSigningObject(signObject))
	return TPM_RCS_KEY + RC_CertifyCreation_signHandle;
    if(!CryptSelectSignScheme(signObject, &in->inScheme))
	return TPM_RCS_SCHEME + RC_CertifyCreation_inScheme;
    // CertifyCreation specific input validation
    // Re-compute ticket
    TicketComputeCreation(in->creationTicket.hierarchy, &certified->name,
			  &in->creationHash, &ticket);
    // Compare ticket
    if(!MemoryEqual2B(&ticket.digest.b, &in->creationTicket.digest.b))
	return TPM_RCS_TICKET + RC_CertifyCreation_creationTicket;
    // Command Output
    // Common fields
    FillInAttestInfo(in->signHandle, &in->inScheme, &in->qualifyingData,
		     &certifyInfo);
    // CertifyCreation specific fields
    // Attestation type
    certifyInfo.type = TPM_ST_ATTEST_CREATION;
    certifyInfo.attested.creation.objectName = certified->name;
    // Copy the creationHash
    certifyInfo.attested.creation.creationHash = in->creationHash;
    // Sign attestation structure.  A NULL signature will be returned if
    // signObject is TPM_RH_NULL.  A TPM_RC_NV_UNAVAILABLE, TPM_RC_NV_RATE,
    // TPM_RC_VALUE, TPM_RC_SCHEME or TPM_RC_ATTRIBUTES error may be returned at
    // this point
    return SignAttestInfo(signObject, &in->inScheme, &certifyInfo,
			  &in->qualifyingData, &out->certifyInfo,
			  &out->signature);
}
#endif // CC_CertifyCreation
#include "Tpm.h"
#include "Attest_spt_fp.h"
#include "Quote_fp.h"
#if CC_Quote  // Conditional expansion of this file
TPM_RC
TPM2_Quote(
	   Quote_In        *in,            // IN: input parameter list
	   Quote_Out       *out            // OUT: output parameter list
	   )
{
    TPMI_ALG_HASH            hashAlg;
    TPMS_ATTEST              quoted;
    OBJECT                 *signObject = HandleToObject(in->signHandle);
    // Input Validation
    if(!IsSigningObject(signObject))
	return TPM_RCS_KEY + RC_Quote_signHandle;
    if(!CryptSelectSignScheme(signObject, &in->inScheme))
	return TPM_RCS_SCHEME + RC_Quote_inScheme;
    // Command Output
    // Filling in attest information
    // Common fields
    // FillInAttestInfo may return TPM_RC_SCHEME or TPM_RC_KEY
    FillInAttestInfo(in->signHandle, &in->inScheme, &in->qualifyingData, &quoted);
    // Quote specific fields
    // Attestation type
    quoted.type = TPM_ST_ATTEST_QUOTE;
    // Get hash algorithm in sign scheme.  This hash algorithm is used to
    // compute PCR digest. If there is no algorithm, then the PCR cannot
    // be digested and this command returns TPM_RC_SCHEME
    hashAlg = in->inScheme.details.any.hashAlg;
    if(hashAlg == TPM_ALG_NULL)
	return TPM_RCS_SCHEME + RC_Quote_inScheme;
    // Compute PCR digest
    PCRComputeCurrentDigest(hashAlg, &in->PCRselect,
			    &quoted.attested.quote.pcrDigest);
    // Copy PCR select.  "PCRselect" is modified in PCRComputeCurrentDigest
    // function
    quoted.attested.quote.pcrSelect = in->PCRselect;
    // Sign attestation structure.  A NULL signature will be returned if
    // signObject is NULL.
    return SignAttestInfo(signObject, &in->inScheme, &quoted, &in->qualifyingData,
			  &out->quoted, &out->signature);
}
#endif // CC_Quote
#include "Tpm.h"
#include "Attest_spt_fp.h"
#include "GetSessionAuditDigest_fp.h"
#if CC_GetSessionAuditDigest  // Conditional expansion of this file
TPM_RC
TPM2_GetSessionAuditDigest(
			   GetSessionAuditDigest_In    *in,            // IN: input parameter list
			   GetSessionAuditDigest_Out   *out            // OUT: output parameter list
			   )
{
    SESSION                 *session = SessionGet(in->sessionHandle);
    TPMS_ATTEST              auditInfo;
    OBJECT                 *signObject = HandleToObject(in->signHandle);
    // Input Validation
    if(!IsSigningObject(signObject))
	return TPM_RCS_KEY + RC_GetSessionAuditDigest_signHandle;
    if(!CryptSelectSignScheme(signObject, &in->inScheme))
	return TPM_RCS_SCHEME + RC_GetSessionAuditDigest_inScheme;
    // session must be an audit session
    if(session->attributes.isAudit == CLEAR)
	return TPM_RCS_TYPE + RC_GetSessionAuditDigest_sessionHandle;
    // Command Output
    // Fill in attest information common fields
    FillInAttestInfo(in->signHandle, &in->inScheme, &in->qualifyingData,
		     &auditInfo);
    // SessionAuditDigest specific fields
    auditInfo.type = TPM_ST_ATTEST_SESSION_AUDIT;
    auditInfo.attested.sessionAudit.sessionDigest = session->u2.auditDigest;
    // Exclusive audit session
    auditInfo.attested.sessionAudit.exclusiveSession
	= (g_exclusiveAuditSession == in->sessionHandle);
    // Sign attestation structure.  A NULL signature will be returned if
    // signObject is NULL.
    return SignAttestInfo(signObject, &in->inScheme, &auditInfo,
			  &in->qualifyingData, &out->auditInfo,
			  &out->signature);
}
#endif // CC_GetSessionAuditDigest
#include "Tpm.h"
#include "Attest_spt_fp.h"
#include "GetCommandAuditDigest_fp.h"
#if CC_GetCommandAuditDigest  // Conditional expansion of this file
TPM_RC
TPM2_GetCommandAuditDigest(
			   GetCommandAuditDigest_In    *in,            // IN: input parameter list
			   GetCommandAuditDigest_Out   *out            // OUT: output parameter list
			   )
{
    TPM_RC                  result;
    TPMS_ATTEST             auditInfo;
    OBJECT                 *signObject = HandleToObject(in->signHandle);
    // Input validation
    if(!IsSigningObject(signObject))
	return TPM_RCS_KEY + RC_GetCommandAuditDigest_signHandle;
    if(!CryptSelectSignScheme(signObject, &in->inScheme))
	return TPM_RCS_SCHEME + RC_GetCommandAuditDigest_inScheme;
    // Command Output
    // Fill in attest information common fields
    FillInAttestInfo(in->signHandle, &in->inScheme, &in->qualifyingData,
		     &auditInfo);
    // CommandAuditDigest specific fields
    auditInfo.type = TPM_ST_ATTEST_COMMAND_AUDIT;
    auditInfo.attested.commandAudit.digestAlg = gp.auditHashAlg;
    auditInfo.attested.commandAudit.auditCounter = gp.auditCounter;
    // Copy command audit log
    auditInfo.attested.commandAudit.auditDigest = gr.commandAuditDigest;
    CommandAuditGetDigest(&auditInfo.attested.commandAudit.commandDigest);
    // Sign attestation structure.  A NULL signature will be returned if
    // signHandle is TPM_RH_NULL.  A TPM_RC_NV_UNAVAILABLE, TPM_RC_NV_RATE,
    // TPM_RC_VALUE, TPM_RC_SCHEME or TPM_RC_ATTRIBUTES error may be returned at
    // this point
    result = SignAttestInfo(signObject, &in->inScheme, &auditInfo,
			    &in->qualifyingData, &out->auditInfo,
			    &out->signature);
    // Internal Data Update
    if(result == TPM_RC_SUCCESS && in->signHandle != TPM_RH_NULL)
	// Reset log
	gr.commandAuditDigest.t.size = 0;
    return result;
}
#endif // CC_GetCommandAuditDigest
#include "Tpm.h"
#include "Attest_spt_fp.h"
#include "GetTime_fp.h"
#if CC_GetTime  // Conditional expansion of this file
TPM_RC
TPM2_GetTime(
	     GetTime_In      *in,            // IN: input parameter list
	     GetTime_Out     *out            // OUT: output parameter list
	     )
{
    TPMS_ATTEST             timeInfo;
    OBJECT                 *signObject = HandleToObject(in->signHandle);
    // Input Validation
    if(!IsSigningObject(signObject))
	return TPM_RCS_KEY + RC_GetTime_signHandle;
    if(!CryptSelectSignScheme(signObject, &in->inScheme))
	return TPM_RCS_SCHEME + RC_GetTime_inScheme;
    // Command Output
    // Fill in attest common fields
    FillInAttestInfo(in->signHandle, &in->inScheme, &in->qualifyingData, &timeInfo);
    // GetClock specific fields
    timeInfo.type = TPM_ST_ATTEST_TIME;
    timeInfo.attested.time.time.time = g_time;
    TimeFillInfo(&timeInfo.attested.time.time.clockInfo);
    // Firmware version in plain text
    timeInfo.attested.time.firmwareVersion
	= (((UINT64)gp.firmwareV1) << 32) + gp.firmwareV2;
    // Sign attestation structure.  A NULL signature will be returned if
    // signObject is NULL.
    return SignAttestInfo(signObject, &in->inScheme, &timeInfo, &in->qualifyingData,
			  &out->timeInfo, &out->signature);
}
#endif // CC_GetTime
#include "Tpm.h"
#include "CertifyX509_fp.h"
#include "X509.h"
#include "TpmAsn1_fp.h"
#include "X509_spt_fp.h"
#include "Attest_spt_fp.h"
#include "Platform_fp.h"
#if CC_CertifyX509 // Conditional expansion of this file
#if CERTIFYX509_DEBUG
#include "DebugHelpers_fp.h"
#endif

/* Error Returns	Meaning*/
/* TPM_RC_ATTRIBUTES	the attributes of objectHandle are not compatible with the KeyUsage() or TPMA_OBJECT values in the extensions fields */
/* TPM_RC_BINDING	the public and private portions of the key are not properly bound. */
/* TPM_RC_HASH	the hash algorithm in the scheme is not supported */
/* TPM_RC_KEY	signHandle does not reference a signing key; */
/* TPM_RC_SCHEME	the scheme is not compatible with sign key type, or input scheme is not compatible with default scheme, or the chosen scheme is not a valid sign scheme */
    /* TPM_RC_VALUE	most likely a problem with the format of partialCertificate */
TPM_RC
TPM2_CertifyX509(
		 CertifyX509_In          *in,          // IN: input parameter list
		 CertifyX509_Out         *out            // OUT: output parameter list
		 )
{
    TPM_RC                   result;
    OBJECT                  *signKey = HandleToObject(in->signHandle);
    OBJECT                  *object = HandleToObject(in->objectHandle);
    HASH_STATE               hash;
    INT16                    length;        // length for a tagged element
    ASN1UnmarshalContext     ctx;
    ASN1MarshalContext       ctxOut;
    // certTBS holds an array of pointers and lengths. Each entry references the
    // corresponding value in a TBSCertificate structure. For example, the 1th
    // element references the version number
    stringRef                certTBS[REF_COUNT] = {{0}};
#define ALLOWED_SEQUENCES   (SUBJECT_PUBLIC_KEY_REF - SIGNATURE_REF)
    stringRef                partial[ALLOWED_SEQUENCES] = {{0}};
    INT16                    countOfSequences = 0;
    INT16                    i;
    //
#if CERTIFYX509_DEBUG
    DebugFileInit();
    DebugDumpBuffer(in->partialCertificate.t.size, in->partialCertificate.t.buffer,
		    "partialCertificate");
#endif
    
    // Input Validation
    if(in->reserved.b.size != 0)
	return TPM_RC_SIZE + RC_CertifyX509_reserved;
    // signing key must be able to sign
    if(!IsSigningObject(signKey))
	return TPM_RCS_KEY + RC_CertifyX509_signHandle;
    // Pick a scheme for sign.  If the input sign scheme is not compatible with
    // the default scheme, return an error.
    if(!CryptSelectSignScheme(signKey, &in->inScheme))
	return TPM_RCS_SCHEME + RC_CertifyX509_inScheme;
    // Make sure that the public Key encoding is known
    if(X509AddPublicKey(NULL, object) == 0)
	return TPM_RCS_ASYMMETRIC + RC_CertifyX509_objectHandle;
    // Unbundle 'partialCertificate'.
    // Initialize the unmarshaling context
    if(!ASN1UnmarshalContextInitialize(&ctx, in->partialCertificate.t.size,
				       in->partialCertificate.t.buffer))
	return TPM_RCS_VALUE + RC_CertifyX509_partialCertificate;
    // Make sure that this is a constructed SEQUENCE
    length = ASN1NextTag(&ctx);
    // Must be a constructed SEQUENCE that uses all of the input parameter
    if((ctx.tag != (ASN1_CONSTRUCTED_SEQUENCE))
       || ((ctx.offset + length) != in->partialCertificate.t.size))
	return TPM_RCS_SIZE + RC_CertifyX509_partialCertificate;
    
    // This scans through the contents of the outermost SEQUENCE. This would be the
    // 'issuer', 'validity', 'subject', 'issuerUniqueID' (optional),
    // 'subjectUniqueID' (optional), and 'extensions.'
    while(ctx.offset < ctx.size)
	{
	    INT16           startOfElement = ctx.offset;
	    //
	    // Read the next tag and length field.
	    length = ASN1NextTag(&ctx);
	    if(length < 0)
		break;
	    if(ctx.tag == ASN1_CONSTRUCTED_SEQUENCE)
	        {
	            partial[countOfSequences].buf = &ctx.buffer[startOfElement];
	            ctx.offset += length;
	            partial[countOfSequences].len = (INT16)ctx.offset - startOfElement;
	            if(++countOfSequences > ALLOWED_SEQUENCES)
	                break;
	        }
	    else if(ctx.tag  == X509_EXTENSIONS)
	        {
	            if(certTBS[EXTENSIONS_REF].len != 0)
	                return TPM_RCS_VALUE + RC_CertifyX509_partialCertificate;
	            certTBS[EXTENSIONS_REF].buf = &ctx.buffer[startOfElement];
	            ctx.offset += length;
	            certTBS[EXTENSIONS_REF].len =
	                (INT16)ctx.offset - startOfElement;
	        }
	    else
		return TPM_RCS_VALUE + RC_CertifyX509_partialCertificate;
	}
    // Make sure that we used all of the data and found at least the required
    // number of elements.
    if((ctx.offset != ctx.size) || (countOfSequences < 3)
       || (countOfSequences > 4)
       || (certTBS[EXTENSIONS_REF].buf == NULL))
	return TPM_RCS_VALUE + RC_CertifyX509_partialCertificate;
    // Now that we know how many sequences there were, we can put them where they
    // belong
    for(i = 0; i < countOfSequences; i++)
	certTBS[SUBJECT_KEY_REF - i] = partial[countOfSequences - 1 - i];
    
    // If only three SEQUENCES, then the TPM needs to produce the signature algorithm.
    // See if it can
    if((countOfSequences == 3) &&
       (X509AddSigningAlgorithm(NULL, signKey, &in->inScheme) == 0))
	return TPM_RCS_SCHEME + RC_CertifyX509_signHandle;
    
    // Process the extensions
    result = X509ProcessExtensions(object, &certTBS[EXTENSIONS_REF]);
    if(result != TPM_RC_SUCCESS)
	// If the extension has the TPMA_OBJECT extension and the attributes don't
	// match, then the error code will be TPM_RCS_ATTRIBUTES. Otherwise, the error
	// indicates a malformed partialCertificate.
	return result + ((result == TPM_RCS_ATTRIBUTES)
			 ? RC_CertifyX509_objectHandle
			 : RC_CertifyX509_partialCertificate);
    // Command Output
    // Create the addedToCertificate values
    
    // Build the addedToCertificate from the bottom up.
    // Initialize the context structure
    ASN1InitialializeMarshalContext(&ctxOut, sizeof(out->addedToCertificate.t.buffer),
				    out->addedToCertificate.t.buffer);
    // Place a marker for the overall context
    ASN1StartMarshalContext(&ctxOut);  // SEQUENCE for addedToCertificate
    
    // Add the subject public key descriptor
    certTBS[SUBJECT_PUBLIC_KEY_REF].len = X509AddPublicKey(&ctxOut, object);
    certTBS[SUBJECT_PUBLIC_KEY_REF].buf = ctxOut.buffer + ctxOut.offset;
    // If the caller didn't provide the algorithm identifier, create it
    if(certTBS[SIGNATURE_REF].len == 0)
	{
	    certTBS[SIGNATURE_REF].len = X509AddSigningAlgorithm(&ctxOut, signKey,
								 &in->inScheme);
	    certTBS[SIGNATURE_REF].buf = ctxOut.buffer + ctxOut.offset;
	}
    // Create the serial number value. Use the out->tbsDigest as scratch.
    {
	TPM2B                   *digest = &out->tbsDigest.b;
	//
	digest->size = (INT16)CryptHashStart(&hash, signKey->publicArea.nameAlg);
	pAssert(digest->size != 0);
	
	// The serial number size is the smaller of the digest and the vendor-defined
	// value
	digest->size = MIN(digest->size, SIZE_OF_X509_SERIAL_NUMBER);
	// Add all the parts of the certificate other than the serial number
	// and version number
	for(i = SIGNATURE_REF; i < REF_COUNT; i++)
	    CryptDigestUpdate(&hash, certTBS[i].len, certTBS[i].buf);
	// throw in the Name of the signing key...
	CryptDigestUpdate2B(&hash, &signKey->name.b);
	// ...and the Name of the signed key.
	CryptDigestUpdate2B(&hash, &object->name.b);
	// Done
	CryptHashEnd2B(&hash, digest);
    }
    
    // Add the serial number
    certTBS[SERIAL_NUMBER_REF].len =
	ASN1PushInteger(&ctxOut, out->tbsDigest.t.size, out->tbsDigest.t.buffer);
    certTBS[SERIAL_NUMBER_REF].buf = ctxOut.buffer + ctxOut.offset;
    
    // Add the static version number
    ASN1StartMarshalContext(&ctxOut);
    ASN1PushUINT(&ctxOut, 2);
    certTBS[VERSION_REF].len =
	ASN1EndEncapsulation(&ctxOut, ASN1_APPLICAIION_SPECIFIC);
    certTBS[VERSION_REF].buf = ctxOut.buffer + ctxOut.offset;
    
    // Create a fake tag and length for the TBS in the space used for
    // 'addedToCertificate'
    {
	for(length = 0, i = 0; i < REF_COUNT; i++)
	    length += certTBS[i].len;
	// Put a fake tag and length into the buffer for use in the tbsDigest
	certTBS[ENCODED_SIZE_REF].len =
	    ASN1PushTagAndLength(&ctxOut, ASN1_CONSTRUCTED_SEQUENCE, length);
	certTBS[ENCODED_SIZE_REF].buf = ctxOut.buffer + ctxOut.offset;
	// Restore the buffer pointer to add back the number of octets used for the
	// tag and length
	ctxOut.offset += certTBS[ENCODED_SIZE_REF].len;
    }
    // sanity check
    if(ctxOut.offset < 0)
	return TPM_RC_FAILURE;
    // Create the tbsDigest to sign
    out->tbsDigest.t.size = CryptHashStart(&hash, in->inScheme.details.any.hashAlg);
    for(i = 0; i < REF_COUNT; i++)
	CryptDigestUpdate(&hash, certTBS[i].len, certTBS[i].buf);
    CryptHashEnd2B(&hash, &out->tbsDigest.b);
    
#if CERTIFYX509_DEBUG
    {
	BYTE                 fullTBS[4096];
	BYTE                *fill = fullTBS;
	int                  j;
	for (j = 0; j < REF_COUNT; j++)
	    {
		MemoryCopy(fill, certTBS[j].buf, certTBS[j].len);
		fill += certTBS[j].len;
	    }
	DebugDumpBuffer((int)(fill - &fullTBS[0]), fullTBS, "\nfull TBS");
    }
#endif
    
    // Finish up the processing of addedToCertificate
    // Create the actual tag and length for the addedToCertificate structure
    out->addedToCertificate.t.size =
	ASN1EndEncapsulation(&ctxOut, ASN1_CONSTRUCTED_SEQUENCE);
    // Now move all the addedToContext to the start of the buffer
    MemoryCopy(out->addedToCertificate.t.buffer, ctxOut.buffer + ctxOut.offset,
	       out->addedToCertificate.t.size);
#if CERTIFYX509_DEBUG
    DebugDumpBuffer(out->addedToCertificate.t.size, out->addedToCertificate.t.buffer,
		    "\naddedToCertificate");
#endif
    // only thing missing is the signature
    result = CryptSign(signKey, &in->inScheme, &out->tbsDigest, &out->signature);
    
    return result;
}
#endif // CC_CertifyX509
