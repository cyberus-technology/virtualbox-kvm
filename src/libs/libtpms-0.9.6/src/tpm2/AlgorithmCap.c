/********************************************************************************/
/*										*/
/*			 Algorithm Property Definitions 			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: AlgorithmCap.c $		*/
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

/* 9.1 AlgorithmCap.c */
/* 9.1.1 Description */
/* This file contains the algorithm property definitions for the algorithms and the code for the
   TPM2_GetCapability() to return the algorithm properties. */
/* 9.1.2 Includes and Defines */
#include "Tpm.h"
typedef struct
{
    TPM_ALG_ID          algID;
    TPMA_ALGORITHM      attributes;
} ALGORITHM;
static const ALGORITHM    s_algorithms[] =
    {
	// The entries in this table need to be in ascending order but the table doesn't
	// need to be full (gaps are allowed). One day, a tool might exist to fill in the
	// table from the TPM_ALG description
#if ALG_RSA
	{TPM_ALG_RSA,           TPMA_ALGORITHM_INITIALIZER(1, 0, 0, 1, 0, 0, 0, 0, 0)},
#endif
#if ALG_TDES
	{TPM_ALG_TDES,          TPMA_ALGORITHM_INITIALIZER(0, 1, 0, 0, 0, 0, 0, 0, 0)},
#endif
#if ALG_SHA1
	{TPM_ALG_SHA1,          TPMA_ALGORITHM_INITIALIZER(0, 0, 1, 0, 0, 0, 0, 0, 0)},
#endif
	{TPM_ALG_HMAC,          TPMA_ALGORITHM_INITIALIZER(0, 0, 1, 0, 0, 1, 0, 0, 0)},
#if ALG_AES
	{TPM_ALG_AES,           TPMA_ALGORITHM_INITIALIZER(0, 1, 0, 0, 0, 0, 0, 0, 0)},
#endif
#if ALG_MGF1
	{TPM_ALG_MGF1,          TPMA_ALGORITHM_INITIALIZER(0, 0, 1, 0, 0, 0, 0, 1, 0)},
#endif
	{TPM_ALG_KEYEDHASH,     TPMA_ALGORITHM_INITIALIZER(0, 0, 1, 1, 0, 1, 1, 0, 0)},
#if ALG_XOR
	{TPM_ALG_XOR,           TPMA_ALGORITHM_INITIALIZER(0, 1, 1, 0, 0, 0, 0, 0, 0)},
#endif
#if ALG_SHA256
	{TPM_ALG_SHA256,        TPMA_ALGORITHM_INITIALIZER(0, 0, 1, 0, 0, 0, 0, 0, 0)},
#endif
#if ALG_SHA384
	{TPM_ALG_SHA384,        TPMA_ALGORITHM_INITIALIZER(0, 0, 1, 0, 0, 0, 0, 0, 0)},
#endif
#if ALG_SHA512
	{TPM_ALG_SHA512,        TPMA_ALGORITHM_INITIALIZER(0, 0, 1, 0, 0, 0, 0, 0, 0)},
#endif
#if ALG_SM3_256
	{TPM_ALG_SM3_256,       TPMA_ALGORITHM_INITIALIZER(0, 0, 1, 0, 0, 0, 0, 0, 0)},
#endif
#if ALG_SM4
	{TPM_ALG_SM4,           TPMA_ALGORITHM_INITIALIZER(0, 1, 0, 0, 0, 0, 0, 0, 0)},
#endif
#if ALG_RSASSA
	{TPM_ALG_RSASSA,        TPMA_ALGORITHM_INITIALIZER(1, 0, 0, 0, 0, 1, 0, 0, 0)},
#endif
#if ALG_RSAES
	{TPM_ALG_RSAES,         TPMA_ALGORITHM_INITIALIZER(1, 0, 0, 0, 0, 0, 1, 0, 0)},
#endif
#if ALG_RSAPSS
	{TPM_ALG_RSAPSS,        TPMA_ALGORITHM_INITIALIZER(1, 0, 0, 0, 0, 1, 0, 0, 0)},
#endif
#if ALG_OAEP
	{TPM_ALG_OAEP,          TPMA_ALGORITHM_INITIALIZER(1, 0, 0, 0, 0, 0, 1, 0, 0)},
#endif
#if ALG_ECDSA
	{TPM_ALG_ECDSA,         TPMA_ALGORITHM_INITIALIZER(1, 0, 0, 0, 0, 1, 0, 0, 0)},
#endif
#if ALG_ECDH
	{TPM_ALG_ECDH,          TPMA_ALGORITHM_INITIALIZER(1, 0, 0, 0, 0, 0, 0, 1, 0)},
#endif
#if ALG_ECDAA
	{TPM_ALG_ECDAA,         TPMA_ALGORITHM_INITIALIZER(1, 0, 0, 0, 0, 1, 0, 0, 0)},
#endif
#if ALG_SM2
	{TPM_ALG_SM2,           TPMA_ALGORITHM_INITIALIZER(1, 0, 0, 0, 0, 1, 0, 1, 0)},
#endif
#if ALG_ECSCHNORR
	{TPM_ALG_ECSCHNORR,      TPMA_ALGORITHM_INITIALIZER(1, 0, 0, 0, 0, 1, 0, 0, 0)},
#endif
#if ALG_ECMQV
	{TPM_ALG_ECMQV,          TPMA_ALGORITHM_INITIALIZER(1, 0, 0, 0, 0, 0, 0, 1, 0)},
#endif
#if ALG_KDF1_SP800_56A
	{TPM_ALG_KDF1_SP800_56A, TPMA_ALGORITHM_INITIALIZER(0, 0, 1, 0, 0, 0, 0, 1, 0)},
#endif
#if ALG_KDF2
	{TPM_ALG_KDF2,           TPMA_ALGORITHM_INITIALIZER(0, 0, 1, 0, 0, 0, 0, 1, 0)},
#endif
#if ALG_KDF1_SP800_108
	{TPM_ALG_KDF1_SP800_108, TPMA_ALGORITHM_INITIALIZER(0, 0, 1, 0, 0, 0, 0, 1, 0)},
#endif
#if ALG_ECC
	{TPM_ALG_ECC,            TPMA_ALGORITHM_INITIALIZER(1, 0, 0, 1, 0, 0, 0, 0, 0)},
#endif
	{TPM_ALG_SYMCIPHER,      TPMA_ALGORITHM_INITIALIZER(0, 0, 0, 1, 0, 0, 0, 0, 0)},
#if ALG_CAMELLIA
	{TPM_ALG_CAMELLIA,       TPMA_ALGORITHM_INITIALIZER(0, 1, 0, 0, 0, 0, 0, 0, 0)},
#endif
#if ALG_CMAC
	{TPM_ALG_CMAC,           TPMA_ALGORITHM_INITIALIZER(0, 1, 0, 0, 0, 1, 0, 0, 0)},
#endif
#if ALG_CTR
	{TPM_ALG_CTR,            TPMA_ALGORITHM_INITIALIZER(0, 1, 0, 0, 0, 0, 1, 0, 0)},
#endif
#if ALG_OFB
	{TPM_ALG_OFB,            TPMA_ALGORITHM_INITIALIZER(0, 1, 0, 0, 0, 0, 1, 0, 0)},
#endif
#if ALG_CBC
	{TPM_ALG_CBC,            TPMA_ALGORITHM_INITIALIZER(0, 1, 0, 0, 0, 0, 1, 0, 0)},
#endif
#if ALG_CFB
	{TPM_ALG_CFB,            TPMA_ALGORITHM_INITIALIZER(0, 1, 0, 0, 0, 0, 1, 0, 0)},
#endif
#if ALG_ECB
	{TPM_ALG_ECB,            TPMA_ALGORITHM_INITIALIZER(0, 1, 0, 0, 0, 0, 1, 0, 0)},
#endif
    };
/* 9.1.3 AlgorithmCapGetImplemented() */
/* This function is used by TPM2_GetCapability() to return a list of the implemented algorithms. */
/* Return Values Meaning */
/* YES more algorithms to report */
/* NO no more algorithms to report */
TPMI_YES_NO
AlgorithmCapGetImplemented(
			   TPM_ALG_ID                   algID,     // IN: the starting algorithm ID
			   UINT32                       count,     // IN: count of returned algorithms
			   TPML_ALG_PROPERTY           *algList    // OUT: algorithm list
			   )
{
    TPMI_YES_NO     more = NO;
    UINT32          i;
    UINT32          algNum;
    // initialize output algorithm list
    algList->count = 0;
    // The maximum count of algorithms we may return is MAX_CAP_ALGS.
    if(count > MAX_CAP_ALGS)
	count = MAX_CAP_ALGS;
    // Compute how many algorithms are defined in s_algorithms array.
    algNum = sizeof(s_algorithms) / sizeof(s_algorithms[0]);
    // Scan the implemented algorithm list to see if there is a match to 'algID'.
    for(i = 0; i < algNum; i++)
	{
	    // If algID is less than the starting algorithm ID, skip it
	    if(s_algorithms[i].algID < algID)
		continue;
	    if(algList->count < count)
		{
		    // If we have not filled up the return list, add more algorithms
		    // to it
		    algList->algProperties[algList->count].alg = s_algorithms[i].algID;
		    algList->algProperties[algList->count].algProperties =
			s_algorithms[i].attributes;
		    algList->count++;
		}
	    else
		{
		    // If the return list is full but we still have algorithms
		    // available, report this and stop scanning.
		    more = YES;
		    break;
		}
	}
    return more;
}
/* 9.1.4 AlgorithmGetImplementedVector()

   This function returns the bit vector of the implemented algorithms.
*/
LIB_EXPORT
void
AlgorithmGetImplementedVector(
			      ALGORITHM_VECTOR    *implemented    // OUT: the implemented bits are SET
			      )
{
    int                      index;
    // Nothing implemented until we say it is
    MemorySet(implemented, 0, sizeof(ALGORITHM_VECTOR));
    // Go through the list of implemented algorithms and SET the corresponding bit in
    // in the implemented vector
    for(index = (sizeof(s_algorithms) / sizeof(s_algorithms[0])) - 1;
	index >= 0; index--)
	SET_BIT(s_algorithms[index].algID, *implemented);
    return;
}
