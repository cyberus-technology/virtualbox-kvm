/********************************************************************************/
/*										*/
/*			 	Ephemeral EC Keys    				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: EphemeralCommands.c $	*/
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
#include "Commit_fp.h"
#if CC_Commit  // Conditional expansion of this file
TPM_RC
TPM2_Commit(
	    Commit_In       *in,            // IN: input parameter list
	    Commit_Out      *out            // OUT: output parameter list
	    )
{
    OBJECT                  *eccKey;
    TPMS_ECC_POINT           P2;
    TPMS_ECC_POINT          *pP2 = NULL;
    TPMS_ECC_POINT          *pP1 = NULL;
    TPM2B_ECC_PARAMETER      r;
    TPM2B_ECC_PARAMETER      p;
    TPM_RC                   result;
    TPMS_ECC_PARMS          *parms;
    // Input Validation
    eccKey = HandleToObject(in->signHandle);
    parms = &eccKey->publicArea.parameters.eccDetail;
    // Input key must be an ECC key
    if(eccKey->publicArea.type != TPM_ALG_ECC)
	return TPM_RCS_KEY + RC_Commit_signHandle;
    // This command may only be used with a sign-only key using an anonymous
    // scheme.
    // NOTE: a sign + decrypt key has no scheme so it will not be an anonymous one
    // and an unrestricted sign key might no have a signing scheme but it can't
    // be use in Commit()
    if(!CryptIsSchemeAnonymous(parms->scheme.scheme))
	return TPM_RCS_SCHEME + RC_Commit_signHandle;
    // Make sure that both parts of P2 are present if either is present
    if((in->s2.t.size == 0) != (in->y2.t.size == 0))
	return TPM_RCS_SIZE + RC_Commit_y2;
    // Get prime modulus for the curve. This is needed later but getting this now
    // allows confirmation that the curve exists.
    if(!CryptEccGetParameter(&p, 'p', parms->curveID))
	return TPM_RCS_KEY + RC_Commit_signHandle;
    // Get the random value that will be used in the point multiplications
    // Note: this does not commit the count.
    if(!CryptGenerateR(&r, NULL, parms->curveID, &eccKey->name))
	return TPM_RC_NO_RESULT;
    // Set up P2 if s2 and Y2 are provided
    if(in->s2.t.size != 0)
	{
	    TPM2B_DIGEST             x2;
	    pP2 = &P2;
	    // copy y2 for P2
	    P2.y = in->y2;
	    // Compute x2  HnameAlg(s2) mod p
	    //      do the hash operation on s2 with the size of curve 'p'
	    x2.t.size = CryptHashBlock(eccKey->publicArea.nameAlg,
				       in->s2.t.size,
				       in->s2.t.buffer,
				       sizeof(x2.t.buffer),
				       x2.t.buffer);
	    // If there were error returns in the hash routine, indicate a problem
	    // with the hash algorithm selection
	    if(x2.t.size == 0)
		return TPM_RCS_HASH + RC_Commit_signHandle;
	    // The size of the remainder will be same as the size of p. DivideB() will
	    // pad the results (leading zeros) if necessary to make the size the same
	    P2.x.t.size = p.t.size;
	    //  set p2.x = hash(s2) mod p
	    if(DivideB(&x2.b, &p.b, NULL, &P2.x.b) != TPM_RC_SUCCESS)
		return TPM_RC_NO_RESULT;
	    if(!CryptEccIsPointOnCurve(parms->curveID, pP2))
		return TPM_RCS_ECC_POINT + RC_Commit_s2;
	    if(eccKey->attributes.publicOnly == SET)
		return TPM_RCS_KEY + RC_Commit_signHandle;
	}
    // If there is a P1, make sure that it is on the curve
    // NOTE: an "empty" point has two UINT16 values which are the size values
    // for each of the coordinates.
    if(in->P1.size > 4)
	{
	    pP1 = &in->P1.point;
	    if(!CryptEccIsPointOnCurve(parms->curveID, pP1))
		return TPM_RCS_ECC_POINT + RC_Commit_P1;
	}
    // Pass the parameters to CryptCommit.
    // The work is not done in-line because it does several point multiplies
    // with the same curve.  It saves work by not having to reload the curve
    // parameters multiple times.
    result = CryptEccCommitCompute(&out->K.point,
				   &out->L.point,
				   &out->E.point,
				   parms->curveID,
				   pP1,
				   pP2,
				   &eccKey->sensitive.sensitive.ecc,
				   &r);
    if(result != TPM_RC_SUCCESS)
	return result;
    // The commit computation was successful so complete the commit by setting
    // the bit
    out->counter = CryptCommit();
    return TPM_RC_SUCCESS;
}
#endif // CC_Commit
#include "Tpm.h"
#include "EC_Ephemeral_fp.h"
#if CC_EC_Ephemeral  // Conditional expansion of this file
TPM_RC
TPM2_EC_Ephemeral(
		  EC_Ephemeral_In     *in,            // IN: input parameter list
		  EC_Ephemeral_Out    *out            // OUT: output parameter list
		  )
{
    TPM2B_ECC_PARAMETER      r;
    TPM_RC                   result;
    //
    do
	{
	    // Get the random value that will be used in the point multiplications
	    // Note: this does not commit the count.
	    if(!CryptGenerateR(&r, NULL, in->curveID, NULL))
		return TPM_RC_NO_RESULT;
	    // do a point multiply
	    result = CryptEccPointMultiply(&out->Q.point, in->curveID, NULL, &r,
					   NULL, NULL);
	    // commit the count value if either the r value results in the point at
	    // infinity or if the value is good. The commit on the r value for infinity
	    // is so that the r value will be skipped.
	    if((result == TPM_RC_SUCCESS) || (result == TPM_RC_NO_RESULT))
		out->counter = CryptCommit();
	} while(result == TPM_RC_NO_RESULT);
    return TPM_RC_SUCCESS;
}
#endif // CC_EC_Ephemeral
