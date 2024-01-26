/********************************************************************************/
/*										*/
/*	Message Authentication Codes Based on a Symmetric Block Cipher		*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CryptCmac.c $		*/
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
/*  (c) Copyright IBM Corp. and others, 2018 - 2021				*/
/*										*/
/********************************************************************************/

/* 10.2.6	CryptCmac.c */
/* 10.2.6.1	Introduction */
/* This file contains the implementation of the message authentication codes based on a symmetric
   block cipher. These functions only use the single block encryption functions of the selected
   symmetric cryptographic library. */
/* 10.2.6.2	Includes, Defines, and Typedefs */
#define _CRYPT_HASH_C_
#include "Tpm.h"
#include "CryptSym.h"
#if ALG_CMAC
    /* 10.2.6.3	Functions */
    /* 10.2.6.3.1	CryptCmacStart() */
    /* This is the function to start the CMAC sequence operation. It initializes the dispatch
       functions for the data and end operations for CMAC and initializes the parameters that are
       used for the processing of data, including the key, key size and block cipher algorithm. */
UINT16
CryptCmacStart(
	       SMAC_STATE          *state,
	       TPMU_PUBLIC_PARMS   *keyParms,
	       TPM_ALG_ID           macAlg,
	       TPM2B               *key
	       )
{
    tpmCmacState_t      *cState = &state->state.cmac;
    TPMT_SYM_DEF_OBJECT *def = &keyParms->symDetail.sym;
    //
    if(macAlg != TPM_ALG_CMAC)
	return 0;
    MemorySet(cState, 0, sizeof(*cState));  // libtpms bugfix
    // set up the encryption algorithm and parameters
    cState->symAlg = def->algorithm;
    cState->keySizeBits = def->keyBits.sym;
    cState->iv.t.size = CryptGetSymmetricBlockSize(def->algorithm,
						   def->keyBits.sym);
    MemoryCopy2B(&cState->symKey.b, key, sizeof(cState->symKey.t.buffer));
    // Set up the dispatch methods for the CMAC
    state->smacMethods.data = CryptCmacData;
    state->smacMethods.end = CryptCmacEnd;
    return cState->iv.t.size;
}

/* 10.2.5.3.2	CryptCmacData() */
/* This function is used to add data to the CMAC sequence computation. The function will XOR new
   data into the IV. If the buffer is full, and there is additional input data, the data is
   encrypted into the IV buffer, the new data is then XOR into the IV. When the data runs out, the
   function returns without encrypting even if the buffer is full. The last data block of a sequence
   will not be encrypted until the call to CryptCmacEnd(). This is to allow the proper subkey to be
   computed and applied before the last block is encrypted. */
void
CryptCmacData(
	      SMAC_STATES         *state,
	      UINT32               size,
	      const BYTE          *buffer
	      )
{
    tpmCmacState_t          *cmacState = &state->cmac;
    TPM_ALG_ID               algorithm = cmacState->symAlg;
    BYTE                    *key = cmacState->symKey.t.buffer;
    UINT16                   keySizeInBits = cmacState->keySizeBits;
    tpmCryptKeySchedule_t    keySchedule;
    TpmCryptSetSymKeyCall_t  encrypt;
    //
    memset(&keySchedule, 0, sizeof(keySchedule)); /* libtpms added: coverity */
    // Set up the encryption values based on the algorithm
    switch (algorithm)
	{
	    FOR_EACH_SYM(ENCRYPT_CASE)
	  default:
	    FAIL(FATAL_ERROR_INTERNAL);
	}
    while(size > 0)
	{
	    if(cmacState->bcount == cmacState->iv.t.size)
	        {
	            ENCRYPT(&keySchedule, cmacState->iv.t.buffer, cmacState->iv.t.buffer);
	            cmacState->bcount = 0;
	        }
	    for(;(size > 0) && (cmacState->bcount < cmacState->iv.t.size);
		size--, cmacState->bcount++)
	        {
	            cmacState->iv.t.buffer[cmacState->bcount] ^= *buffer++;
	        }
	}
}

/* 10.2.6.3.3	CryptCmacEnd() */
/* This is the completion function for the CMAC. It does padding, if needed, and selects the subkey
   to be applied before the last block is encrypted. */
UINT16
CryptCmacEnd(
	     SMAC_STATES             *state,
	     UINT32                   outSize,
	     BYTE                    *outBuffer
	     )
{
    tpmCmacState_t          *cState = &state->cmac;
    // Need to set algorithm, key, and keySizeInBits in the local context so that
    // the SELECT and ENCRYPT macros will work here
    TPM_ALG_ID               algorithm = cState->symAlg;
    BYTE                    *key = cState->symKey.t.buffer;
    UINT16                   keySizeInBits = cState->keySizeBits;
    tpmCryptKeySchedule_t    keySchedule;
    TpmCryptSetSymKeyCall_t  encrypt;
    TPM2B_IV                 subkey = {{0, {0}}};
    BOOL                     xorVal;
    UINT16                   i;
    memset(&keySchedule, 0, sizeof(keySchedule)); /* libtpms added: coverity */

    subkey.t.size = cState->iv.t.size;
    // Encrypt a block of zero
    // Set up the encryption values based on the algorithm
    switch (algorithm)
	{
	    FOR_EACH_SYM(ENCRYPT_CASE)
	  default:
	    return 0;
	}
    ENCRYPT(&keySchedule, subkey.t.buffer, subkey.t.buffer);

    // shift left by 1 and XOR with 0x0...87 if the MSb was 0
    xorVal = ((subkey.t.buffer[0] & 0x80) == 0) ? 0 : 0x87;
    ShiftLeft(&subkey.b);
    subkey.t.buffer[subkey.t.size - 1] ^= xorVal;
    // this is a sanity check to make sure that the algorithm is working properly.
    // remove this check when debug is done
    pAssert(cState->bcount <= cState->iv.t.size);
    // If the buffer is full then no need to compute subkey 2.
    if(cState->bcount < cState->iv.t.size)
	{
	    //Pad the data
	    cState->iv.t.buffer[cState->bcount++] ^= 0x80;
	    // The rest of the data is a pad of zero which would simply be XORed
	    // with the iv value so nothing to do...
	    // Now compute K2
	    xorVal = ((subkey.t.buffer[0] & 0x80) == 0) ? 0 : 0x87;
	    ShiftLeft(&subkey.b);
	    subkey.t.buffer[subkey.t.size - 1] ^= xorVal;
	}
    // XOR the subkey into the IV
    for(i = 0; i < subkey.t.size; i++)
	cState->iv.t.buffer[i] ^= subkey.t.buffer[i];
    ENCRYPT(&keySchedule, cState->iv.t.buffer, cState->iv.t.buffer);
    i = (UINT16)MIN(cState->iv.t.size, outSize);
    MemoryCopy(outBuffer, cState->iv.t.buffer, i);

    return i;
}

#endif
