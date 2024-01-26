/********************************************************************************/
/*										*/
/*			 Authenticated Countdown Timer				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id$		*/
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

// 5.24	ACT.h

#ifndef _ACT_H_
#define _ACT_H_
#include "TpmProfile.h"
#if !(defined RH_ACT_0) || (RH_ACT_0 != YES)
#   undef   RH_ACT_0
#   define  RH_ACT_0 NO
#   define IF_ACT_0_IMPLEMENTED(op)
#else
#   define IF_ACT_0_IMPLEMENTED(op) op(0)
#endif
#if !(defined RH_ACT_1) || (RH_ACT_1 != YES)
#   undef   RH_ACT_1
#   define  RH_ACT_1 NO
#   define IF_ACT_1_IMPLEMENTED(op)
#else
#   define IF_ACT_1_IMPLEMENTED(op) op(1)
#endif
#if !(defined RH_ACT_2) || (RH_ACT_2 != YES)
#   undef   RH_ACT_2
#   define  RH_ACT_2 NO
#   define IF_ACT_2_IMPLEMENTED(op)
#else
#   define IF_ACT_2_IMPLEMENTED(op) op(2)
#endif
#if !(defined RH_ACT_3) || (RH_ACT_3 != YES)
#   undef   RH_ACT_3
#   define  RH_ACT_3 NO
#   define IF_ACT_3_IMPLEMENTED(op)
#else
#   define IF_ACT_3_IMPLEMENTED(op) op(3)
#endif
#if !(defined RH_ACT_4) || (RH_ACT_4 != YES)
#   undef   RH_ACT_4
#   define  RH_ACT_4 NO
#   define IF_ACT_4_IMPLEMENTED(op)
#else
#   define IF_ACT_4_IMPLEMENTED(op) op(4)
#endif
#if !(defined RH_ACT_5) || (RH_ACT_5 != YES)
#   undef   RH_ACT_5
#   define  RH_ACT_5 NO
#   define IF_ACT_5_IMPLEMENTED(op)
#else
#   define IF_ACT_5_IMPLEMENTED(op) op(5)
#endif
#if !(defined RH_ACT_6) || (RH_ACT_6 != YES)
#   undef   RH_ACT_6
#   define  RH_ACT_6 NO
#   define IF_ACT_6_IMPLEMENTED(op)
#else
#   define IF_ACT_6_IMPLEMENTED(op) op(6)
#endif
#if !(defined RH_ACT_7) || (RH_ACT_7 != YES)
#   undef   RH_ACT_7
#   define  RH_ACT_7 NO
#   define IF_ACT_7_IMPLEMENTED(op)
#else
#   define IF_ACT_7_IMPLEMENTED(op) op(7)
#endif
#if !(defined RH_ACT_8) || (RH_ACT_8 != YES)
#   undef   RH_ACT_8
#   define  RH_ACT_8 NO
#   define IF_ACT_8_IMPLEMENTED(op)
#else
#   define IF_ACT_8_IMPLEMENTED(op) op(8)
#endif
#if !(defined RH_ACT_9) || (RH_ACT_9 != YES)
#   undef   RH_ACT_9
#   define  RH_ACT_9 NO
#   define IF_ACT_9_IMPLEMENTED(op)
#else
#   define IF_ACT_9_IMPLEMENTED(op) op(9)
#endif
#if !(defined RH_ACT_A) || (RH_ACT_A != YES)
#   undef   RH_ACT_A
#   define  RH_ACT_A NO
#   define IF_ACT_A_IMPLEMENTED(op)
#else
#   define IF_ACT_A_IMPLEMENTED(op) op(A)
#endif
#if !(defined RH_ACT_B) || (RH_ACT_B != YES)
#   undef   RH_ACT_B
#   define  RH_ACT_B NO
#   define IF_ACT_B_IMPLEMENTED(op)
#else
#   define IF_ACT_B_IMPLEMENTED(op) op(B)
#endif
#if !(defined RH_ACT_C) || (RH_ACT_C != YES)
#   undef   RH_ACT_C
#   define  RH_ACT_C NO
#   define IF_ACT_C_IMPLEMENTED(op)
#else
#   define IF_ACT_C_IMPLEMENTED(op) op(C)
#endif
#if !(defined RH_ACT_D) || (RH_ACT_D != YES)
#   undef   RH_ACT_D
#   define  RH_ACT_D NO
#   define IF_ACT_D_IMPLEMENTED(op)
#else
#   define IF_ACT_D_IMPLEMENTED(op) op(D)
#endif
#if !(defined RH_ACT_E) || (RH_ACT_E != YES)
#   undef   RH_ACT_E
#   define  RH_ACT_E NO
#   define IF_ACT_E_IMPLEMENTED(op)
#else
#   define IF_ACT_E_IMPLEMENTED(op) op(E)
#endif
#if !(defined RH_ACT_F) || (RH_ACT_F != YES)
#   undef   RH_ACT_F
#   define  RH_ACT_F NO
#   define IF_ACT_F_IMPLEMENTED(op)
#else
#   define IF_ACT_F_IMPLEMENTED(op) op(F)
#endif
#ifndef TPM_RH_ACT_0
#error Need numeric definition for TPM_RH_ACT_0
#endif
#ifndef TPM_RH_ACT_1
#   define TPM_RH_ACT_1    (TPM_RH_ACT_0 + 1)
#endif
#ifndef TPM_RH_ACT_2
#   define TPM_RH_ACT_2    (TPM_RH_ACT_0 + 2)
#endif
#ifndef TPM_RH_ACT_3
#   define TPM_RH_ACT_3    (TPM_RH_ACT_0 + 3)
#endif
#ifndef TPM_RH_ACT_4
#   define TPM_RH_ACT_4    (TPM_RH_ACT_0 + 4)
#endif
#ifndef TPM_RH_ACT_5
#   define TPM_RH_ACT_5    (TPM_RH_ACT_0 + 5)
#endif
#ifndef TPM_RH_ACT_6
#   define TPM_RH_ACT_6    (TPM_RH_ACT_0 + 6)
#endif
#ifndef TPM_RH_ACT_7
#   define TPM_RH_ACT_7    (TPM_RH_ACT_0 + 7)
#endif
#ifndef TPM_RH_ACT_8
#   define TPM_RH_ACT_8    (TPM_RH_ACT_0 + 8)
#endif
#ifndef TPM_RH_ACT_9
#   define TPM_RH_ACT_9    (TPM_RH_ACT_0 + 9)
#endif
#ifndef TPM_RH_ACT_A
#   define TPM_RH_ACT_A    (TPM_RH_ACT_0 + 0xA)
#endif
#ifndef TPM_RH_ACT_B
#   define TPM_RH_ACT_B    (TPM_RH_ACT_0 + 0xB)
#endif
#ifndef TPM_RH_ACT_C
#   define TPM_RH_ACT_C    (TPM_RH_ACT_0 + 0xC)
#endif
#ifndef TPM_RH_ACT_D
#   define TPM_RH_ACT_D    (TPM_RH_ACT_0 + 0xD)
#endif
#ifndef TPM_RH_ACT_E
#   define TPM_RH_ACT_E    (TPM_RH_ACT_0 + 0xE)
#endif
#ifndef TPM_RH_ACT_F
#   define TPM_RH_ACT_F    (TPM_RH_ACT_0 + 0xF)
#endif
#define FOR_EACH_ACT(op)						\
    IF_ACT_0_IMPLEMENTED(op)						\
    IF_ACT_1_IMPLEMENTED(op)						\
    IF_ACT_2_IMPLEMENTED(op)						\
    IF_ACT_3_IMPLEMENTED(op)						\
    IF_ACT_4_IMPLEMENTED(op)						\
    IF_ACT_5_IMPLEMENTED(op)						\
    IF_ACT_6_IMPLEMENTED(op)						\
    IF_ACT_7_IMPLEMENTED(op)						\
    IF_ACT_8_IMPLEMENTED(op)						\
    IF_ACT_9_IMPLEMENTED(op)						\
    IF_ACT_A_IMPLEMENTED(op)						\
    IF_ACT_B_IMPLEMENTED(op)						\
    IF_ACT_C_IMPLEMENTED(op)						\
    IF_ACT_D_IMPLEMENTED(op)						\
    IF_ACT_E_IMPLEMENTED(op)						\
    IF_ACT_F_IMPLEMENTED(op)

// This is the mask for ACT that are implemented

//#define ACT_MASK(N)     | (1 << 0x##N)
//#define ACT_IMPLEMENTED_MASK    (0 FOR_EACH_ACT(ACT_MASK))
#define CASE_ACT_HANDLE(N)     case TPM_RH_ACT_##N:
#define CASE_ACT_NUMBER(N)     case 0x##N:
typedef struct ACT_STATE
{
    UINT32          remaining;
    TPM_ALG_ID      hashAlg;
    TPM2B_DIGEST    authPolicy;
} ACT_STATE, *P_ACT_STATE;
#endif // _ACT_H_
