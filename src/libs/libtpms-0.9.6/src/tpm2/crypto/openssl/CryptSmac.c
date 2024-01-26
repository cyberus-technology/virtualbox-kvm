/********************************************************************************/
/*										*/
/*		Message Authentication Codes Based on a Symmetric Block Cipher	*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CryptSmac.c $		*/
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

/* 10.2.20	CryptSmac.c */
/* 10.2.20.1	Introduction */
/* This file contains the implementation of the message authentication codes based on a symmetric
   block cipher. These functions only use the single block encryption functions of the selected
   symmetric cryptographic library. */
/* 10.2.20.2	Includes, Defines, and Typedefs */
#define _CRYPT_HASH_C_
#include "Tpm.h"
#if SMAC_IMPLEMENTED
    /* 10.2.20.2.1	CryptSmacStart() */
    /* Function to start an SMAC. */
UINT16
CryptSmacStart(
	       HASH_STATE              *state,
	       TPMU_PUBLIC_PARMS       *keyParameters,
	       TPM_ALG_ID               macAlg,          // IN: the type of MAC
	       TPM2B                   *key
	       )
{
    UINT16                  retVal = 0;
    //
    // Make sure that the key size is correct. This should have been checked
    // at key load, but...
    if(BITS_TO_BYTES(keyParameters->symDetail.sym.keyBits.sym) == key->size)
	{
	    switch(macAlg)
		{
#if ALG_CMAC
		  case TPM_ALG_CMAC:
		    retVal = CryptCmacStart(&state->state.smac, keyParameters,
					    macAlg, key);
		    break;
#endif
		  default:
		    break;
		}
	}
    state->type = (retVal != 0) ? HASH_STATE_SMAC : HASH_STATE_EMPTY;
    return retVal;
}
/* 10.2.20.2.2	CryptMacStart() */
/* Function to start either an HMAC or an SMAC. Cannot reuse the CryptHmacStart() function because
   of the difference in number of parameters. */
UINT16
CryptMacStart(
	      HMAC_STATE              *state,
	      TPMU_PUBLIC_PARMS       *keyParameters,
	      TPM_ALG_ID               macAlg,          // IN: the type of MAC
	      TPM2B                   *key
	      )
{
    MemorySet(state, 0, sizeof(HMAC_STATE));
    if(CryptHashIsValidAlg(macAlg, FALSE))
	{
	    return CryptHmacStart(state, macAlg, key->size, key->buffer);
	}
    else if(CryptSmacIsValidAlg(macAlg, FALSE))
	{
	    return CryptSmacStart(&state->hashState, keyParameters, macAlg, key);
	}
    else
	return 0;
}
/* 10.2.20.2.3	CryptMacEnd() */
/* Dispatch to the MAC end function using a size and buffer pointer. */
UINT16
CryptMacEnd(
	    HMAC_STATE          *state,
	    UINT32               size,
	    BYTE                *buffer
	    )
{
    UINT16              retVal = 0;
    if(state->hashState.type == HASH_STATE_SMAC)
	retVal = (state->hashState.state.smac.smacMethods.end)(
							       &state->hashState.state.smac.state, size, buffer);
    else if(state->hashState.type == HASH_STATE_HMAC)
	retVal = CryptHmacEnd(state, size, buffer);
    state->hashState.type = HASH_STATE_EMPTY;
    return retVal;
}
#if 0 /* libtpms added */
/* 10.2.20.2.4	CryptMacEnd2B() */
/* Dispatch to the MAC end function using a 2B. */
UINT16
CryptMacEnd2B (
	       HMAC_STATE          *state,
	       TPM2B               *data
	       )
{
    return CryptMacEnd(state, data->size, data->buffer);
}
#endif /* libtpms added */
#endif // SMAC_IMPLEMENTED

