/********************************************************************************/
/*										*/
/*		Implementation of cryptographic primitives for RSA		*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CryptRsa_fp.h $		*/
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

#ifndef CRYPTRSA_FP_H
#define CRYPTRSA_FP_H

BOOL
CryptRsaInit(
	     void
	     );
BOOL
CryptRsaStartup(
		void
		);
void
RsaInitializeExponent(
		      privateExponent_t      *pExp
		      );
INT16
CryptRsaPssSaltSize(
		    INT16              hashSize,
		    INT16               outSize
		    );
TPMT_RSA_DECRYPT*
CryptRsaSelectScheme(
		     TPMI_DH_OBJECT       rsaHandle,     // IN: handle of an RSA key
		     TPMT_RSA_DECRYPT    *scheme         // IN: a sign or decrypt scheme
		     );
TPM_RC
CryptRsaLoadPrivateExponent(
			    OBJECT          *rsaKey        // IN: the RSA key object
			    );
LIB_EXPORT TPM_RC
CryptRsaEncrypt(
		TPM2B_PUBLIC_KEY_RSA        *cOut,          // OUT: the encrypted data
		TPM2B                       *dIn,           // IN: the data to encrypt
		OBJECT                      *key,           // IN: the key used for encryption
		TPMT_RSA_DECRYPT            *scheme,        // IN: the type of padding and hash
		//     if needed
		const TPM2B                 *label,         // IN: in case it is needed
		RAND_STATE                  *rand           // IN: random number generator
		//     state (mostly for testing)
		);
LIB_EXPORT TPM_RC
CryptRsaDecrypt(
		TPM2B               *dOut,          // OUT: the decrypted data
		TPM2B               *cIn,           // IN: the data to decrypt
		OBJECT              *key,           // IN: the key to use for decryption
		TPMT_RSA_DECRYPT    *scheme,        // IN: the padding scheme
		const TPM2B         *label          // IN: in case it is needed for the scheme
		);
LIB_EXPORT TPM_RC
CryptRsaSign(
	     TPMT_SIGNATURE      *sigOut,
	     OBJECT              *key,           // IN: key to use
	     TPM2B_DIGEST        *hIn,           // IN: the digest to sign
	     RAND_STATE          *rand           // IN: the random number generator
	     //      to use (mostly for testing)
	     );
LIB_EXPORT TPM_RC
CryptRsaValidateSignature(
			  TPMT_SIGNATURE  *sig,           // IN: signature
			  OBJECT          *key,           // IN: public modulus
			  TPM2B_DIGEST    *digest         // IN: The digest being validated
			  );
LIB_EXPORT TPM_RC
CryptRsaGenerateKey(
		    OBJECT              *rsaKey,            // IN/OUT: The object structure in which
		    //          the key is created.
		    RAND_STATE          *rand               // IN: if not NULL, the deterministic
		    //     RNG state
		    );
INT16
MakeDerTag(
	   TPM_ALG_ID   hashAlg,
	   INT16        sizeOfBuffer,
	   BYTE        *buffer
	   );


#endif
