/********************************************************************************/
/*										*/
/*			   TPM DES Support	  				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: TpmToOsslDesSupport.c $	*/
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

/* B.2.3.1. TpmToOsslDesSupport.c */
/* B.2.3.1.1. Introduction */
/* The functions in this file are used for initialization of the interface to the OpenSSL
   library. */
/* B.2.3.1.2. Defines and Includes */
#include "Tpm.h"
#if (defined SYM_LIB_OSSL) && ALG_TDES
/*     B.2.3.1.3. Functions */
/* B.2.3.1.3.1. TDES_set_encyrpt_key() */
/* This function makes creation of a TDES key look like the creation of a key for any of the other
   OpenSSL block ciphers. It will create three key schedules, one for each of the DES keys. If
   there are only two keys, then the third schedule is a copy of the first. */
void
TDES_set_encrypt_key(
		     const BYTE                  *key,
		     UINT16                       keySizeInBits,
		     tpmKeyScheduleTDES          *keySchedule
		     )
{
    DES_set_key_unchecked((const_DES_cblock *)key, &keySchedule[0]);
    DES_set_key_unchecked((const_DES_cblock *)&key[8], &keySchedule[1]);
    // If is two-key, copy the schedule for K1 into K3, otherwise, compute the
    // the schedule for K3
    if(keySizeInBits == 128)
	keySchedule[2] = keySchedule[0];
    else
	DES_set_key_unchecked((const_DES_cblock *)&key[16],
			      &keySchedule[2]);
}
/* B.2.3.1.3.2. TDES_encyrpt() */
/* The TPM code uses one key schedule. For TDES, the schedule contains three schedules. OpenSSL
   wants the schedules referenced separately. This function does that. */
void TDES_encrypt(
		  const BYTE              *in,
		  BYTE                    *out,
		  tpmKeyScheduleTDES      *ks
		  )
{
    DES_ecb3_encrypt((const_DES_cblock *)in, (DES_cblock *)out,
		     &ks[0], &ks[1], &ks[2],
		     DES_ENCRYPT);
}
#if !USE_OPENSSL_FUNCTIONS_SYMMETRIC
/* B.2.3.1.3.3. TDES_decrypt() */
/* As with TDES_encypt() this function bridges between the TPM single schedule model and the
   OpenSSL three schedule model. */
void TDES_decrypt(
		  const BYTE          *in,
		  BYTE                *out,
		  tpmKeyScheduleTDES   *ks
		  )
{
    DES_ecb3_encrypt((const_DES_cblock *)in, (DES_cblock *)out,
		     &ks[0], &ks[1], &ks[2],
		     DES_DECRYPT);
}
#endif // !USE_OPENSSL_FUNCTIONS_SYMMETRIC
#endif // SYM_LIB_OSSL
