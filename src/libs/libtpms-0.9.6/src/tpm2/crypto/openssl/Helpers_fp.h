/********************************************************************************/
/*										*/
/*			       OpenSSL helper functions				*/
/*			     Written by Stefan Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
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
/*  (c) Copyright IBM Corp. and others, 2019					*/
/*										*/
/********************************************************************************/

#ifndef HELPERS_FP_H
#define HELPERS_FP_H

#include "TpmTypes.h"

#include <openssl/evp.h>

#if USE_OPENSSL_FUNCTIONS_SYMMETRIC
TPM_RC
OpenSSLCryptGenerateKeyDes(
                           TPMT_SENSITIVE *sensitive    // OUT: sensitive area
                          );

typedef const EVP_CIPHER *(*evpfunc)(void);

evpfunc GetEVPCipher(TPM_ALG_ID    algorithm,       // IN
                     UINT16        keySizeInBits,   // IN
                     TPM_ALG_ID    mode,            // IN
                     const BYTE   *key,             // IN
                     BYTE         *keyToUse,        // OUT same as key or stretched key
                     UINT16       *keyToUseLen      // IN/OUT
                     );
#endif

#if USE_OPENSSL_FUNCTIONS_EC
BOOL OpenSSLEccGetPrivate(
                          bigNum             dOut,   // OUT: the qualified random value
                          const EC_GROUP    *G,      // IN:  the EC_GROUP to use
                          const UINT32       requestedBits // IN: if not 0, then dOut must have that many bits
                         );
#endif

#if USE_OPENSSL_FUNCTIONS_RSA

const char *GetDigestNameByHashAlg(const TPM_ALG_ID hashAlg);

LIB_EXPORT TPM_RC
OpenSSLCryptRsaGenerateKey(
		    OBJECT              *rsaKey,            // IN/OUT: The object structure in which
		    //          the key is created.
		    UINT32               e,
		    int                  keySizeInBits
		    );

LIB_EXPORT TPM_RC
InitOpenSSLRSAPublicKey(OBJECT    *key,   // IN
                        EVP_PKEY **pkey   //OUT
                       );

LIB_EXPORT TPM_RC
InitOpenSSLRSAPrivateKey(OBJECT     *rsaKey,   // IN
                         EVP_PKEY  **pkey      // OUT
                        );

#endif // USE_OPENSSL_FUNCTIONS_RSA

#endif  /* HELPERS_FP_H */
