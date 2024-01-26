/********************************************************************************/
/*										*/
/*		Implementation of cryptographic functions for hashing.		*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CryptHash_fp.h $		*/
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

#ifndef CRYPTHASH_FP_H
#define CRYPTHASH_FP_H

BOOL
CryptHashInit(
	      void
	      );
BOOL
CryptHashStartup(
		 void
		 );
PHASH_DEF
CryptGetHashDef(
		TPM_ALG_ID       hashAlg
		);
BOOL
CryptHashIsValidAlg(
		    TPM_ALG_ID       hashAlg,
		    BOOL             flag
		    );
LIB_EXPORT TPM_ALG_ID
CryptHashGetAlgByIndex(
		       UINT32           index          // IN: the index
		       );
LIB_EXPORT UINT16
CryptHashGetDigestSize(
		       TPM_ALG_ID       hashAlg        // IN: hash algorithm to look up
		       );
LIB_EXPORT UINT16
CryptHashGetBlockSize(
		      TPM_ALG_ID       hashAlg        // IN: hash algorithm to look up
		      );
LIB_EXPORT const BYTE *
CryptHashGetOid(
		TPM_ALG_ID      hashAlg
		);
TPM_ALG_ID
CryptHashGetContextAlg(
		       PHASH_STATE      state          // IN: the context to check
		       );
LIB_EXPORT void
CryptHashCopyState(
		   HASH_STATE          *out,           // OUT: destination of the state
		   const HASH_STATE    *in             // IN: source of the state
		   );
void
CryptHashExportState(
		     PCHASH_STATE         internalFmt,   // IN: the hash state formatted for use by
		     //     library
		     PEXPORT_HASH_STATE   externalFmt    // OUT: the exported hash state
		     );
void
CryptHashImportState(
		     PHASH_STATE          internalFmt,   // OUT: the hash state formatted for use by
		     //     the library
		     PCEXPORT_HASH_STATE  externalFmt    // IN: the exported hash state
		     );
LIB_EXPORT UINT16
CryptHashStart(
	       PHASH_STATE      hashState,     // OUT: the running hash state
	       TPM_ALG_ID       hashAlg        // IN: hash algorithm
	       );
LIB_EXPORT void
CryptDigestUpdate(
		  PHASH_STATE      hashState,     // IN: the hash context information
		  UINT32           dataSize,      // IN: the size of data to be added
		  const BYTE      *data           // IN: data to be hashed
		  );
LIB_EXPORT UINT16
CryptHashEnd(
	     PHASH_STATE      hashState,     // IN: the state of hash stack
	     UINT32           dOutSize,      // IN: size of digest buffer
	     BYTE            *dOut           // OUT: hash digest
	     );
LIB_EXPORT UINT16
CryptHashBlock(
	       TPM_ALG_ID       hashAlg,       // IN: The hash algorithm
	       UINT32           dataSize,      // IN: size of buffer to hash
	       const BYTE      *data,          // IN: the buffer to hash
	       UINT32           dOutSize,      // IN: size of the digest buffer
	       BYTE            *dOut           // OUT: digest buffer
	       );
LIB_EXPORT void
CryptDigestUpdate2B(
		    PHASH_STATE      state,         // IN: the digest state
		    const TPM2B     *bIn            // IN: 2B containing the data
		    );
LIB_EXPORT UINT16
CryptHashEnd2B(
	       PHASH_STATE      state,         // IN: the hash state
	       P2B              digest         // IN: the size of the buffer Out: requested
	       //     number of bytes
	       );
LIB_EXPORT void
CryptDigestUpdateInt(
		     void            *state,         // IN: the state of hash stack
		     UINT32           intSize,       // IN: the size of 'intValue' in bytes
		     UINT64           intValue       // IN: integer value to be hashed
		     );
LIB_EXPORT UINT16
CryptHmacStart(
	       PHMAC_STATE      state,         // IN/OUT: the state buffer
	       TPM_ALG_ID       hashAlg,       // IN: the algorithm to use
	       UINT16           keySize,       // IN: the size of the HMAC key
	       const BYTE      *key            // IN: the HMAC key
	       );
LIB_EXPORT UINT16
CryptHmacEnd(
	     PHMAC_STATE      state,         // IN: the hash state buffer
	     UINT32           dOutSize,      // IN: size of digest buffer
	     BYTE            *dOut           // OUT: hash digest
	     );
LIB_EXPORT UINT16
CryptHmacStart2B(
		 PHMAC_STATE      hmacState,     // OUT: the state of HMAC stack. It will be used
		 //     in HMAC update and completion
		 TPMI_ALG_HASH    hashAlg,       // IN: hash algorithm
		 P2B              key            // IN: HMAC key
		 );
LIB_EXPORT UINT16
CryptHmacEnd2B(
	       PHMAC_STATE      hmacState,     // IN: the state of HMAC stack
	       P2B              digest         // OUT: HMAC
	       );
LIB_EXPORT UINT16
CryptMGF_KDF(
	  UINT32           mSize,         // IN: length of the mask to be produced
	  BYTE            *mask,          // OUT: buffer to receive the mask
	  TPM_ALG_ID       hashAlg,       // IN: hash to use
	  UINT32           seedSize,      // IN: size of the seed
	  BYTE            *seed,          // IN: seed size
	  UINT32           counter        // IN: counter initial value
	  );
LIB_EXPORT UINT16
CryptKDFa(
	  TPM_ALG_ID       hashAlg,       // IN: hash algorithm used in HMAC
	  const TPM2B     *key,           // IN: HMAC key
	  const TPM2B     *label,         // IN: a label for the KDF
	  const TPM2B     *contextU,      // IN: context U
	  const TPM2B     *contextV,      // IN: context V
	  UINT32           sizeInBits,    // IN: size of generated key in bits
	  BYTE            *keyStream,     // OUT: key buffer
	  UINT32          *counterInOut,  // IN/OUT: caller may provide the iteration
	  UINT16           blocks         // IN: If non-zero, this is the maximum number
	  );
LIB_EXPORT UINT16
CryptKDFe(
	  TPM_ALG_ID       hashAlg,       // IN: hash algorithm used in HMAC
	  TPM2B           *Z,             // IN: Z
	  const TPM2B     *label,         // IN: a label value for the KDF
	  TPM2B           *partyUInfo,    // IN: PartyUInfo
	  TPM2B           *partyVInfo,    // IN: PartyVInfo
	  UINT32           sizeInBits,    // IN: size of generated key in bits
	  BYTE            *keyStream      // OUT: key buffer
	  );


#endif
