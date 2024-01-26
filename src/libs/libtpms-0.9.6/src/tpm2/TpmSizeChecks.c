/********************************************************************************/
/*										*/
/*			     TPM Size Checks					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: TpmSizeChecks.c $		*/
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

//** Includes, Defines, and Types
#include    "Tpm.h"
#include    "PlatformACT_fp.h"		/* kgold */
#include    "TpmSizeChecks_fp.h"
#include    <stdio.h>
#include    <assert.h>

#if RUNTIME_SIZE_CHECKS

#if TABLE_DRIVEN_MARSHAL
extern uint32_t    MarshalDataSize;
#endif

static      int once = 0;

//** TpmSizeChecks()
// This function is used during the development process to make sure that the
// vendor-specific values result in a consistent implementation. When possible,
// the code contains #if to do compile-time checks. However, in some cases, the
// values require the use of "sizeof()" and that can't be used in an #if.
BOOL
TpmSizeChecks(
	      void
	      )
{
    BOOL        PASS = TRUE;
#if DEBUG
    //
    if(once++ != 0)
        return 1;
    {
        UINT32      maxAsymSecurityStrength = MAX_ASYM_SECURITY_STRENGTH;
        UINT32      maxHashSecurityStrength = MAX_HASH_SECURITY_STRENGTH;
        UINT32      maxSymSecurityStrength = MAX_SYM_SECURITY_STRENGTH;
        UINT32      maxSecurityStrengthBits = MAX_SECURITY_STRENGTH_BITS;
        UINT32      proofSize = PROOF_SIZE;
        UINT32      compliantProofSize = COMPLIANT_PROOF_SIZE;
        UINT32      compliantPrimarySeedSize = COMPLIANT_PRIMARY_SEED_SIZE;
        UINT32      primarySeedSize = PRIMARY_SEED_SIZE;

        UINT32      cmacState = sizeof(tpmCmacState_t);
        UINT32      hashState = sizeof(HASH_STATE);
        UINT32      keyScheduleSize = sizeof(tpmCryptKeySchedule_t);
	//
        NOT_REFERENCED(cmacState);
        NOT_REFERENCED(hashState);
        NOT_REFERENCED(keyScheduleSize);
        NOT_REFERENCED(maxAsymSecurityStrength);
        NOT_REFERENCED(maxHashSecurityStrength);
        NOT_REFERENCED(maxSymSecurityStrength);
        NOT_REFERENCED(maxSecurityStrengthBits);
        NOT_REFERENCED(proofSize);
        NOT_REFERENCED(compliantProofSize);
        NOT_REFERENCED(compliantPrimarySeedSize);
        NOT_REFERENCED(primarySeedSize);


        {
            TPMT_SENSITIVE           *p;
            // This assignment keeps compiler from complaining about a conditional
            // comparison being between two constants
            UINT16                    max_rsa_key_bytes = MAX_RSA_KEY_BYTES;
            if((max_rsa_key_bytes / 2) != (sizeof(p->sensitive.rsa.t.buffer) / 5))
		{
		    printf("Sensitive part of TPMT_SENSITIVE is undersized. May be caused"
			   " by use of wrong version of Part 2.\n");
		    PASS = FALSE;
		}
        }
#if TABLE_DRIVEN_MARSHAL
        printf("sizeof(MarshalData) = %zu\n", sizeof(MarshalData_st));
#endif

        printf("Size of OBJECT = %zu\n", sizeof(OBJECT));
        printf("Size of components in TPMT_SENSITIVE = %zu\n", sizeof(TPMT_SENSITIVE));
        printf("    TPMI_ALG_PUBLIC                 %zu\n", sizeof(TPMI_ALG_PUBLIC));
        printf("    TPM2B_AUTH                      %zu\n", sizeof(TPM2B_AUTH));
        printf("    TPM2B_DIGEST                    %zu\n", sizeof(TPM2B_DIGEST));
        printf("    TPMU_SENSITIVE_COMPOSITE        %zu\n",
               sizeof(TPMU_SENSITIVE_COMPOSITE));
    }
    // Make sure that the size of the context blob is large enough for the largest
    // context
    // TPMS_CONTEXT_DATA contains two TPM2B values. That is not how this is
    // implemented. Rather, the size field of the TPM2B_CONTEXT_DATA is used to
    // determine the amount of data in the encrypted data. That part is not
    // independently sized. This makes the actual size 2 bytes smaller than
    // calculated using Part 2. Since this is opaque to the caller, it is not
    // necessary to fix. The actual size is returned by TPM2_GetCapabilties().

    // Initialize output handle.  At the end of command action, the output
    // handle of an object will be replaced, while the output handle
    // for a session will be the same as input

    // Get the size of fingerprint in context blob.  The sequence value in
    // TPMS_CONTEXT structure is used as the fingerprint
    {
        UINT32  fingerprintSize = sizeof(UINT64);
        UINT32  integritySize = sizeof(UINT16)
				+ CryptHashGetDigestSize(CONTEXT_INTEGRITY_HASH_ALG);
        UINT32  biggestObject = MAX(MAX(sizeof(HASH_OBJECT), sizeof(OBJECT)),
                                    sizeof(SESSION));
        UINT32  biggestContext = fingerprintSize + integritySize + biggestObject;

        // round required size up to nearest 8 byte boundary.
        biggestContext = 8 * ((biggestContext + 7) / 8);

        if(MAX_CONTEXT_SIZE < biggestContext)
	    {
		printf("MAX_CONTEXT_SIZE needs to be increased to at least to %d (%d)\n",
		       biggestContext, MAX_CONTEXT_SIZE);
		PASS = FALSE;
	    }
	else if (MAX_CONTEXT_SIZE > biggestContext)
	    {
		printf("MAX_CONTEXT_SIZE can be reduced to %d (%d)\n",
		       biggestContext, MAX_CONTEXT_SIZE);
	    }
    }
    {
        union u
        {
            TPMA_OBJECT             attributes;
            UINT32                  uint32Value;
        } u;
        // these are defined so that compiler doesn't complain about conditional
        // expressions comparing two constants.
        int                         aSize = sizeof(u.attributes);
        int                         uSize = sizeof(u.uint32Value);
        u.uint32Value = 0;
        SET_ATTRIBUTE(u.attributes, TPMA_OBJECT, fixedTPM);
        if(u.uint32Value != 2)
	    {
		printf("The bit allocation in a TPMA_OBJECT is not as expected");
		PASS = FALSE;
	    }
        if(aSize != uSize)  // comparison of two sizeof() values annoys compiler
	    {
		printf("A TPMA_OBJECT is not the expected size.");
		PASS = FALSE;
	    }
    }
    // Check that the platform implements each of the ACT that the TPM thinks are present
    {
        uint32_t            act;
        for(act = 0; act < 16; act++)
	    {
		switch(act)
		    {
			FOR_EACH_ACT(CASE_ACT_NUMBER)
			    if(!_plat__ACT_GetImplemented(act))
				{
				    printf("TPM_RH_ACT_%1X is not implemented by platform\n",
					   act);
				    PASS = FALSE;
				}
		      default:
			break;
		    }
	    }
    }
#endif // DEBUG
    return (PASS);
}

#endif // RUNTIME_SIZE_CHECKS


