/********************************************************************************/
/*										*/
/*		DRBG with a behavior according to SP800-90A			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CryptRand_fp.h $		*/
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

#ifndef CRYPTRAND_FP_H
#define CRYPTRAND_FP_H

BOOL
DRBG_GetEntropy(
		UINT32           requiredEntropy,   // IN: requested number of bytes of full
		//     entropy
		BYTE            *entropy            // OUT: buffer to return collected entropy
		);
void
IncrementIv(
	    DRBG_IV         *iv
	    );
BOOL
DRBG_Reseed(
	    DRBG_STATE          *drbgState,         // IN: the state to update
	    DRBG_SEED           *providedEntropy,   // IN: entropy
	    DRBG_SEED           *additionalData     // IN:
	    );
BOOL
DRBG_SelfTest(
	      void
	      );
LIB_EXPORT TPM_RC
CryptRandomStir(
		UINT16           additionalDataSize,
		BYTE            *additionalData
		);
LIB_EXPORT UINT16
CryptRandomGenerate(
		    UINT16           randomSize,
		    BYTE            *buffer
		    );
LIB_EXPORT BOOL
DRBG_InstantiateSeededKdf(
			  KDF_STATE       *state,         // IN: buffer to hold the state
			  TPM_ALG_ID       hashAlg,       // IN: hash algorithm
			  TPM_ALG_ID       kdf,           // IN: the KDF to use
			  TPM2B           *seed,          // IN: the seed to use
			  const TPM2B     *label,         // IN: a label for the generation process.
			  TPM2B           *context,       // IN: the context value
			  UINT32           limit          // IN: Maximum number of bits from the KDF
			  );
LIB_EXPORT void
DRBG_AdditionalData(
		    DRBG_STATE      *drbgState,     // IN:OUT state to update
		    TPM2B           *additionalData // IN: value to incorporate
		    );
LIB_EXPORT TPM_RC
DRBG_InstantiateSeeded(
		       DRBG_STATE      *drbgState,     // IN: buffer to hold the state
		       const TPM2B     *seed,          // IN: the seed to use
		       const TPM2B     *purpose,       // IN: a label for the generation process.
		       const TPM2B     *name,          // IN: name of the object
		       const TPM2B     *additional,    // IN: additional data
		       SEED_COMPAT_LEVEL seedCompatLevel// IN: compatibility level (associated with seed); libtpms added
		       );
LIB_EXPORT BOOL
CryptRandStartup(
		 void
		 );
LIB_EXPORT BOOL
CryptRandInit(
	      void
	      );
LIB_EXPORT UINT16
DRBG_Generate(
	      RAND_STATE      *state,
	      BYTE            *random,        // OUT: buffer to receive the random values
	      UINT16           randomSize     // IN: the number of bytes to generate
	      );
// libtpms added begin
LIB_EXPORT SEED_COMPAT_LEVEL
DRBG_GetSeedCompatLevel(
               RAND_STATE     *state          // IN
              );
// libtpms added end
LIB_EXPORT BOOL
DRBG_Instantiate(
		 DRBG_STATE      *drbgState,         // OUT: the instantiated value
		 UINT16           pSize,             // IN: Size of personalization string
		 BYTE            *personalization    // IN: The personalization string
		 );
LIB_EXPORT TPM_RC
DRBG_Uninstantiate(
		   DRBG_STATE      *drbgState      // IN/OUT: working state to erase
		   );
LIB_EXPORT NUMBYTES
CryptRandMinMax(
		BYTE            *out,
		UINT32           max,
		UINT32           min,
		RAND_STATE      *rand
		);


#endif
