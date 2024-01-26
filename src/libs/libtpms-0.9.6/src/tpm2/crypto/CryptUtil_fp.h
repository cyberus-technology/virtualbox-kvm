/********************************************************************************/
/*										*/
/*			  Interfaces to the CryptoEngine			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CryptUtil_fp.h $		*/
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

#ifndef CRYPTUTIL_FP_H
#define CRYPTUTIL_FP_H

BOOL
CryptIsSchemeAnonymous(
		       TPM_ALG_ID       scheme         // IN: the scheme algorithm to test
		       );
void
ParmDecryptSym(
	       TPM_ALG_ID       symAlg,        // IN: the symmetric algorithm
	       TPM_ALG_ID       hash,          // IN: hash algorithm for KDFa
	       UINT16           keySizeInBits, // IN: the key size in bits
	       TPM2B           *key,           // IN: KDF HMAC key
	       TPM2B           *nonceCaller,   // IN: nonce caller
	       TPM2B           *nonceTpm,      // IN: nonce TPM
	       UINT32           dataSize,      // IN: size of parameter buffer
	       BYTE            *data           // OUT: buffer to be decrypted
	       );
void
ParmEncryptSym(
	       TPM_ALG_ID       symAlg,        // IN: symmetric algorithm
	       TPM_ALG_ID       hash,          // IN: hash algorithm for KDFa
	       UINT16           keySizeInBits, // IN: AES key size in bits
	       TPM2B           *key,           // IN: KDF HMAC key
	       TPM2B           *nonceCaller,   // IN: nonce caller
	       TPM2B           *nonceTpm,      // IN: nonce TPM
	       UINT32           dataSize,      // IN: size of parameter buffer
	       BYTE            *data           // OUT: buffer to be encrypted
	       );
void
CryptXORObfuscation(
		    TPM_ALG_ID       hash,          // IN: hash algorithm for KDF
		    TPM2B           *key,           // IN: KDF key
		    TPM2B           *contextU,      // IN: contextU
		    TPM2B           *contextV,      // IN: contextV
		    UINT32           dataSize,      // IN: size of data buffer
		    BYTE            *data           // IN/OUT: data to be XORed in place
		    );
BOOL
CryptInit(
	  void
	  );
BOOL
CryptStartup(
	     STARTUP_TYPE     type           // IN: the startup type
	     );
BOOL
CryptIsAsymAlgorithm(
		     TPM_ALG_ID       algID          // IN: algorithm ID
		     );
TPM_RC
CryptSecretEncrypt(
		   OBJECT                  *encryptKey,    // IN: encryption key object
		   const TPM2B             *label,         // IN: a null-terminated string as L
		   TPM2B_DATA              *data,          // OUT: secret value
		   TPM2B_ENCRYPTED_SECRET  *secret         // OUT: secret structure
		   );
TPM_RC
CryptSecretDecrypt(
		   OBJECT                 *decryptKey,    // IN: decrypt key
		   TPM2B_NONCE             *nonceCaller,   // IN: nonceCaller.  It is needed for
		   //     symmetric decryption.  For
		   //     asymmetric decryption, this
		   //     parameter is NULL
		   const TPM2B             *label,         // IN: a value for L
		   TPM2B_ENCRYPTED_SECRET  *secret,        // IN: input secret
		   TPM2B_DATA              *data           // OUT: decrypted secret value
		   );
void
CryptParameterEncryption(
			 TPM_HANDLE       handle,            // IN: encrypt session handle
			 TPM2B           *nonceCaller,       // IN: nonce caller
			 UINT16           leadingSizeInByte, // IN: the size of the leading size field in
			 //     bytes
			 TPM2B_AUTH      *extraKey,          // IN: additional key material other than
			 //     sessionAuth
			 BYTE            *buffer             // IN/OUT: parameter buffer to be encrypted
			 );
TPM_RC
CryptParameterDecryption(
			 TPM_HANDLE       handle,            // IN: encrypted session handle
			 TPM2B           *nonceCaller,       // IN: nonce caller
			 UINT32           bufferSize,        // IN: size of parameter buffer
			 UINT16           leadingSizeInByte, // IN: the size of the leading size field in
			 //     byte
			 TPM2B_AUTH      *extraKey,          // IN: the authValue
			 BYTE            *buffer             // IN/OUT: parameter buffer to be decrypted
			 );
void
CryptComputeSymmetricUnique(
			    TPMT_PUBLIC     *publicArea,    // IN: the object's public area
			    TPMT_SENSITIVE  *sensitive,     // IN: the associated sensitive area
			    TPM2B_DIGEST    *unique         // OUT: unique buffer
			    );
TPM_RC
CryptCreateObject(
		  OBJECT                  *object,            // IN: new object structure pointer
		  TPMS_SENSITIVE_CREATE   *sensitiveCreate,   // IN: sensitive creation
		  RAND_STATE              *rand               // IN: the random number generator
		  //      to use
		  );
TPMI_ALG_HASH
CryptGetSignHashAlg(
		    TPMT_SIGNATURE  *auth           // IN: signature
		    );
BOOL
CryptIsSplitSign(
		 TPM_ALG_ID       scheme         // IN: the algorithm selector
		 );
BOOL
CryptIsAsymSignScheme(
		      TPMI_ALG_PUBLIC          publicType,        // IN: Type of the object
		      TPMI_ALG_ASYM_SCHEME     scheme             // IN: the scheme
		      );
BOOL
CryptIsAsymDecryptScheme(
			 TPMI_ALG_PUBLIC          publicType,        // IN: Type of the object
			 TPMI_ALG_ASYM_SCHEME     scheme             // IN: the scheme
			 );
BOOL
CryptSelectSignScheme(
		      OBJECT              *signObject,    // IN: signing key
		      TPMT_SIG_SCHEME     *scheme         // IN/OUT: signing scheme
		      );
TPM_RC
CryptSign(
	  OBJECT              *signKey,       // IN: signing key
	  TPMT_SIG_SCHEME     *signScheme,    // IN: sign scheme.
	  TPM2B_DIGEST        *digest,        // IN: The digest being signed
	  TPMT_SIGNATURE      *signature      // OUT: signature
	  );
TPM_RC
CryptValidateSignature(
		       TPMI_DH_OBJECT   keyHandle,     // IN: The handle of sign key
		       TPM2B_DIGEST    *digest,        // IN: The digest being validated
		       TPMT_SIGNATURE  *signature      // IN: signature
		       );
TPM_RC
CryptGetTestResult(
		   TPM2B_MAX_BUFFER    *outData        // OUT: test result data
		   );
TPM_RC
CryptValidateKeys(
		  TPMT_PUBLIC      *publicArea,
		  TPMT_SENSITIVE   *sensitive,
		  TPM_RC            blamePublic,
		  TPM_RC            blameSensitive
		  );
TPM_RC
CryptSelectMac(
	       TPMT_PUBLIC             *publicArea,
	       TPMI_ALG_MAC_SCHEME     *inMac
	       );
BOOL
CryptMacIsValidForKey(
		      TPM_ALG_ID          keyType,
		      TPM_ALG_ID          macAlg,
		      BOOL                flag
		      );
BOOL
CryptSmacIsValidAlg(
		    TPM_ALG_ID      alg,
		    BOOL            FLAG        // IN: Indicates if TPM_ALG_NULL is valid
		    );
BOOL
CryptSymModeIsValid(
		    TPM_ALG_ID          mode,
		    BOOL                flag
		    );

#endif
