/********************************************************************************/
/*										*/
/*		Command code attribute array for GetCapability	    		*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CommandAttributeData.h $	*/
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

/* 5.6	CommandAttributeData.h */
/* This file should only be included by CommandCodeAttibutes.c */

#ifdef _COMMAND_CODE_ATTRIBUTES_
#include "CommandAttributes.h"
#if COMPRESSED_LISTS
#   define      PAD_LIST    0
#else
#   define      PAD_LIST    1
#endif

/* This is the command code attribute array for GetCapability(). Both this array and
   s_commandAttributes provides command code attributes, but tuned for different purpose */

const TPMA_CC    s_ccAttr [] = {
#if (PAD_LIST  || CC_NV_UndefineSpaceSpecial)
    TPMA_CC_INITIALIZER(0x011f, 0, 1, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_EvictControl)
    TPMA_CC_INITIALIZER(0x0120, 0, 1, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_HierarchyControl)
    TPMA_CC_INITIALIZER(0x0121, 0, 1, 1, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_UndefineSpace)
    TPMA_CC_INITIALIZER(0x0122, 0, 1, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST )
    TPMA_CC_INITIALIZER(0x0123, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ChangeEPS)
    TPMA_CC_INITIALIZER(0x0124, 0, 1, 1, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ChangePPS)
    TPMA_CC_INITIALIZER(0x0125, 0, 1, 1, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Clear)
    TPMA_CC_INITIALIZER(0x0126, 0, 1, 1, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ClearControl)
    TPMA_CC_INITIALIZER(0x0127, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ClockSet)
    TPMA_CC_INITIALIZER(0x0128, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_HierarchyChangeAuth)
    TPMA_CC_INITIALIZER(0x0129, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_DefineSpace)
    TPMA_CC_INITIALIZER(0x012a, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PCR_Allocate)
    TPMA_CC_INITIALIZER(0x012b, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PCR_SetAuthPolicy)
    TPMA_CC_INITIALIZER(0x012c, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PP_Commands)
    TPMA_CC_INITIALIZER(0x012d, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_SetPrimaryPolicy)
    TPMA_CC_INITIALIZER(0x012e, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_FieldUpgradeStart)
    TPMA_CC_INITIALIZER(0x012f, 0, 0, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ClockRateAdjust)
    TPMA_CC_INITIALIZER(0x0130, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_CreatePrimary)
    TPMA_CC_INITIALIZER(0x0131, 0, 0, 0, 0, 1, 1, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_GlobalWriteLock)
    TPMA_CC_INITIALIZER(0x0132, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_GetCommandAuditDigest)
    TPMA_CC_INITIALIZER(0x0133, 0, 1, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_Increment)
    TPMA_CC_INITIALIZER(0x0134, 0, 1, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_SetBits)
    TPMA_CC_INITIALIZER(0x0135, 0, 1, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_Extend)
    TPMA_CC_INITIALIZER(0x0136, 0, 1, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_Write)
    TPMA_CC_INITIALIZER(0x0137, 0, 1, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_WriteLock)
    TPMA_CC_INITIALIZER(0x0138, 0, 1, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_DictionaryAttackLockReset)
    TPMA_CC_INITIALIZER(0x0139, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_DictionaryAttackParameters)
    TPMA_CC_INITIALIZER(0x013a, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_ChangeAuth)
    TPMA_CC_INITIALIZER(0x013b, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PCR_Event)
    TPMA_CC_INITIALIZER(0x013c, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PCR_Reset)
    TPMA_CC_INITIALIZER(0x013d, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_SequenceComplete)
    TPMA_CC_INITIALIZER(0x013e, 0, 0, 0, 1, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_SetAlgorithmSet)
    TPMA_CC_INITIALIZER(0x013f, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_SetCommandCodeAuditStatus)
    TPMA_CC_INITIALIZER(0x0140, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_FieldUpgradeData)
    TPMA_CC_INITIALIZER(0x0141, 0, 1, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_IncrementalSelfTest)
    TPMA_CC_INITIALIZER(0x0142, 0, 1, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_SelfTest)
    TPMA_CC_INITIALIZER(0x0143, 0, 1, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Startup)
    TPMA_CC_INITIALIZER(0x0144, 0, 1, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Shutdown)
    TPMA_CC_INITIALIZER(0x0145, 0, 1, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_StirRandom)
    TPMA_CC_INITIALIZER(0x0146, 0, 1, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ActivateCredential)
    TPMA_CC_INITIALIZER(0x0147, 0, 0, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Certify)
    TPMA_CC_INITIALIZER(0x0148, 0, 0, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyNV)
    TPMA_CC_INITIALIZER(0x0149, 0, 0, 0, 0, 3, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_CertifyCreation)
    TPMA_CC_INITIALIZER(0x014a, 0, 0, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Duplicate)
    TPMA_CC_INITIALIZER(0x014b, 0, 0, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_GetTime)
    TPMA_CC_INITIALIZER(0x014c, 0, 0, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_GetSessionAuditDigest)
    TPMA_CC_INITIALIZER(0x014d, 0, 0, 0, 0, 3, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_Read)
    TPMA_CC_INITIALIZER(0x014e, 0, 0, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_ReadLock)
    TPMA_CC_INITIALIZER(0x014f, 0, 1, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ObjectChangeAuth)
    TPMA_CC_INITIALIZER(0x0150, 0, 0, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicySecret)
    TPMA_CC_INITIALIZER(0x0151, 0, 0, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Rewrap)
    TPMA_CC_INITIALIZER(0x0152, 0, 0, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Create)
    TPMA_CC_INITIALIZER(0x0153, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ECDH_ZGen)
    TPMA_CC_INITIALIZER(0x0154, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || (CC_HMAC || CC_MAC))
    TPMA_CC_INITIALIZER(0x0155, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Import)
    TPMA_CC_INITIALIZER(0x0156, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Load)
    TPMA_CC_INITIALIZER(0x0157, 0, 0, 0, 0, 1, 1, 0, 0),
#endif
#if (PAD_LIST  || CC_Quote)
    TPMA_CC_INITIALIZER(0x0158, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_RSA_Decrypt)
    TPMA_CC_INITIALIZER(0x0159, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST )
    TPMA_CC_INITIALIZER(0x015a, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || (CC_HMAC_Start || CC_MAC_Start))
    TPMA_CC_INITIALIZER(0x015b, 0, 0, 0, 0, 1, 1, 0, 0),
#endif
#if (PAD_LIST  || CC_SequenceUpdate)
    TPMA_CC_INITIALIZER(0x015c, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Sign)
    TPMA_CC_INITIALIZER(0x015d, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Unseal)
    TPMA_CC_INITIALIZER(0x015e, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST )
    TPMA_CC_INITIALIZER(0x015f, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicySigned)
    TPMA_CC_INITIALIZER(0x0160, 0, 0, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ContextLoad)
    TPMA_CC_INITIALIZER(0x0161, 0, 0, 0, 0, 0, 1, 0, 0),
#endif
#if (PAD_LIST  || CC_ContextSave)
    TPMA_CC_INITIALIZER(0x0162, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ECDH_KeyGen)
    TPMA_CC_INITIALIZER(0x0163, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_EncryptDecrypt)
    TPMA_CC_INITIALIZER(0x0164, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_FlushContext)
    TPMA_CC_INITIALIZER(0x0165, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST )
    TPMA_CC_INITIALIZER(0x0166, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_LoadExternal)
    TPMA_CC_INITIALIZER(0x0167, 0, 0, 0, 0, 0, 1, 0, 0),
#endif
#if (PAD_LIST  || CC_MakeCredential)
    TPMA_CC_INITIALIZER(0x0168, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_ReadPublic)
    TPMA_CC_INITIALIZER(0x0169, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyAuthorize)
    TPMA_CC_INITIALIZER(0x016a, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyAuthValue)
    TPMA_CC_INITIALIZER(0x016b, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyCommandCode)
    TPMA_CC_INITIALIZER(0x016c, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyCounterTimer)
    TPMA_CC_INITIALIZER(0x016d, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyCpHash)
    TPMA_CC_INITIALIZER(0x016e, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyLocality)
    TPMA_CC_INITIALIZER(0x016f, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyNameHash)
    TPMA_CC_INITIALIZER(0x0170, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyOR)
    TPMA_CC_INITIALIZER(0x0171, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyTicket)
    TPMA_CC_INITIALIZER(0x0172, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ReadPublic)
    TPMA_CC_INITIALIZER(0x0173, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_RSA_Encrypt)
    TPMA_CC_INITIALIZER(0x0174, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST )
    TPMA_CC_INITIALIZER(0x0175, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_StartAuthSession)
    TPMA_CC_INITIALIZER(0x0176, 0, 0, 0, 0, 2, 1, 0, 0),
#endif
#if (PAD_LIST  || CC_VerifySignature)
    TPMA_CC_INITIALIZER(0x0177, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ECC_Parameters)
    TPMA_CC_INITIALIZER(0x0178, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_FirmwareRead)
    TPMA_CC_INITIALIZER(0x0179, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_GetCapability)
    TPMA_CC_INITIALIZER(0x017a, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_GetRandom)
    TPMA_CC_INITIALIZER(0x017b, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_GetTestResult)
    TPMA_CC_INITIALIZER(0x017c, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Hash)
    TPMA_CC_INITIALIZER(0x017d, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PCR_Read)
    TPMA_CC_INITIALIZER(0x017e, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyPCR)
    TPMA_CC_INITIALIZER(0x017f, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyRestart)
    TPMA_CC_INITIALIZER(0x0180, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ReadClock)
    TPMA_CC_INITIALIZER(0x0181, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PCR_Extend)
    TPMA_CC_INITIALIZER(0x0182, 0, 1, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PCR_SetAuthValue)
    TPMA_CC_INITIALIZER(0x0183, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_NV_Certify)
    TPMA_CC_INITIALIZER(0x0184, 0, 0, 0, 0, 3, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_EventSequenceComplete)
    TPMA_CC_INITIALIZER(0x0185, 0, 1, 0, 1, 2, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_HashSequenceStart)
    TPMA_CC_INITIALIZER(0x0186, 0, 0, 0, 0, 0, 1, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyPhysicalPresence)
    TPMA_CC_INITIALIZER(0x0187, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyDuplicationSelect)
    TPMA_CC_INITIALIZER(0x0188, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyGetDigest)
    TPMA_CC_INITIALIZER(0x0189, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_TestParms)
    TPMA_CC_INITIALIZER(0x018a, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Commit)
    TPMA_CC_INITIALIZER(0x018b, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyPassword)
    TPMA_CC_INITIALIZER(0x018c, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_ZGen_2Phase)
    TPMA_CC_INITIALIZER(0x018d, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_EC_Ephemeral)
    TPMA_CC_INITIALIZER(0x018e, 0, 0, 0, 0, 0, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyNvWritten)
    TPMA_CC_INITIALIZER(0x018f, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyTemplate)
    TPMA_CC_INITIALIZER(0x0190, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_CreateLoaded)
    TPMA_CC_INITIALIZER(0x0191, 0, 0, 0, 0, 1, 1, 0, 0),
#endif
#if (PAD_LIST  || CC_PolicyAuthorizeNV)
    TPMA_CC_INITIALIZER(0x0192, 0, 0, 0, 0, 3, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_EncryptDecrypt2)
    TPMA_CC_INITIALIZER(0x0193, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_AC_GetCapability)
    TPMA_CC_INITIALIZER(0x0194, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_AC_Send)
    TPMA_CC_INITIALIZER(0x0195, 0, 0, 0, 0, 3, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Policy_AC_SendSelect)
    TPMA_CC_INITIALIZER(0x0196, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST || CC_CertifyX509)
    TPMA_CC_INITIALIZER(0x0197, 0, 0, 0, 0, 2, 0, 0, 0),
#endif
#if (PAD_LIST || CC_ACT_SetTimeout)
    TPMA_CC_INITIALIZER(0x0198, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST || CC_ECC_Encrypt)
    TPMA_CC_INITIALIZER(0x0199, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST || CC_ECC_Decrypt)
    TPMA_CC_INITIALIZER(0x019A, 0, 0, 0, 0, 1, 0, 0, 0),
#endif
#if (PAD_LIST  || CC_Vendor_TCG_Test)
    TPMA_CC_INITIALIZER(0x0000, 0, 0, 0, 0, 0, 0, 1, 0),
#endif

    TPMA_ZERO_INITIALIZER()
};

/* This is the command code attribute structure. */

const COMMAND_ATTRIBUTES    s_commandAttributes [] = {
#if (PAD_LIST  || CC_NV_UndefineSpaceSpecial)
    (COMMAND_ATTRIBUTES)(CC_NV_UndefineSpaceSpecial     *  // 0x011f
			 (IS_IMPLEMENTED+HANDLE_1_ADMIN+HANDLE_2_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_EvictControl)
    (COMMAND_ATTRIBUTES)(CC_EvictControl                *  // 0x0120
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_HierarchyControl)
    (COMMAND_ATTRIBUTES)(CC_HierarchyControl            *  // 0x0121
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_NV_UndefineSpace)
    (COMMAND_ATTRIBUTES)(CC_NV_UndefineSpace            *  // 0x0122
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST )
    (COMMAND_ATTRIBUTES)(0),                               // 0x0123
#endif
#if (PAD_LIST  || CC_ChangeEPS)
    (COMMAND_ATTRIBUTES)(CC_ChangeEPS                   *  // 0x0124
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_ChangePPS)
    (COMMAND_ATTRIBUTES)(CC_ChangePPS                   *  // 0x0125
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_Clear)
    (COMMAND_ATTRIBUTES)(CC_Clear                       *  // 0x0126
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_ClearControl)
    (COMMAND_ATTRIBUTES)(CC_ClearControl                *  // 0x0127
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_ClockSet)
    (COMMAND_ATTRIBUTES)(CC_ClockSet                    *  // 0x0128
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_HierarchyChangeAuth)
    (COMMAND_ATTRIBUTES)(CC_HierarchyChangeAuth         *  // 0x0129
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_NV_DefineSpace)
    (COMMAND_ATTRIBUTES)(CC_NV_DefineSpace              *  // 0x012a
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_PCR_Allocate)
    (COMMAND_ATTRIBUTES)(CC_PCR_Allocate                *  // 0x012b
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_PCR_SetAuthPolicy)
    (COMMAND_ATTRIBUTES)(CC_PCR_SetAuthPolicy           *  // 0x012c
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_PP_Commands)
    (COMMAND_ATTRIBUTES)(CC_PP_Commands                 *  // 0x012d
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_REQUIRED)),
#endif
#if (PAD_LIST  || CC_SetPrimaryPolicy)
    (COMMAND_ATTRIBUTES)(CC_SetPrimaryPolicy            *  // 0x012e
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_FieldUpgradeStart)
    (COMMAND_ATTRIBUTES)(CC_FieldUpgradeStart           *  // 0x012f
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_ADMIN+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_ClockRateAdjust)
    (COMMAND_ATTRIBUTES)(CC_ClockRateAdjust             *  // 0x0130
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_CreatePrimary)
    (COMMAND_ATTRIBUTES)(CC_CreatePrimary               *  // 0x0131
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+PP_COMMAND+ENCRYPT_2+R_HANDLE)),
#endif
#if (PAD_LIST  || CC_NV_GlobalWriteLock)
    (COMMAND_ATTRIBUTES)(CC_NV_GlobalWriteLock          *  // 0x0132
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_GetCommandAuditDigest)
    (COMMAND_ATTRIBUTES)(CC_GetCommandAuditDigest       *  // 0x0133
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+HANDLE_2_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_NV_Increment)
    (COMMAND_ATTRIBUTES)(CC_NV_Increment                *  // 0x0134
			 (IS_IMPLEMENTED+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_NV_SetBits)
    (COMMAND_ATTRIBUTES)(CC_NV_SetBits                  *  // 0x0135
			 (IS_IMPLEMENTED+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_NV_Extend)
    (COMMAND_ATTRIBUTES)(CC_NV_Extend                   *  // 0x0136
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_NV_Write)
    (COMMAND_ATTRIBUTES)(CC_NV_Write                    *  // 0x0137
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_NV_WriteLock)
    (COMMAND_ATTRIBUTES)(CC_NV_WriteLock                *  // 0x0138
			 (IS_IMPLEMENTED+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_DictionaryAttackLockReset)
    (COMMAND_ATTRIBUTES)(CC_DictionaryAttackLockReset   *  // 0x0139
			 (IS_IMPLEMENTED+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_DictionaryAttackParameters)
    (COMMAND_ATTRIBUTES)(CC_DictionaryAttackParameters  *  // 0x013a
			 (IS_IMPLEMENTED+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_NV_ChangeAuth)
    (COMMAND_ATTRIBUTES)(CC_NV_ChangeAuth               *  // 0x013b
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_ADMIN)),
#endif
#if (PAD_LIST  || CC_PCR_Event)
    (COMMAND_ATTRIBUTES)(CC_PCR_Event                   *  // 0x013c
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_PCR_Reset)
    (COMMAND_ATTRIBUTES)(CC_PCR_Reset                   *  // 0x013d
			 (IS_IMPLEMENTED+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_SequenceComplete)
    (COMMAND_ATTRIBUTES)(CC_SequenceComplete            *  // 0x013e
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_SetAlgorithmSet)
    (COMMAND_ATTRIBUTES)(CC_SetAlgorithmSet             *  // 0x013f
			 (IS_IMPLEMENTED+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_SetCommandCodeAuditStatus)
    (COMMAND_ATTRIBUTES)(CC_SetCommandCodeAuditStatus   *  // 0x0140
			 (IS_IMPLEMENTED+HANDLE_1_USER+PP_COMMAND)),
#endif
#if (PAD_LIST  || CC_FieldUpgradeData)
    (COMMAND_ATTRIBUTES)(CC_FieldUpgradeData            *  // 0x0141
			 (IS_IMPLEMENTED+DECRYPT_2)),
#endif
#if (PAD_LIST  || CC_IncrementalSelfTest)
    (COMMAND_ATTRIBUTES)(CC_IncrementalSelfTest         *  // 0x0142
			 (IS_IMPLEMENTED)),
#endif
#if (PAD_LIST  || CC_SelfTest)
    (COMMAND_ATTRIBUTES)(CC_SelfTest                    *  // 0x0143
			 (IS_IMPLEMENTED)),
#endif
#if (PAD_LIST  || CC_Startup)
    (COMMAND_ATTRIBUTES)(CC_Startup                     *  // 0x0144
			 (IS_IMPLEMENTED+NO_SESSIONS)),
#endif
#if (PAD_LIST  || CC_Shutdown)
    (COMMAND_ATTRIBUTES)(CC_Shutdown                    *  // 0x0145
			 (IS_IMPLEMENTED)),
#endif
#if (PAD_LIST  || CC_StirRandom)
    (COMMAND_ATTRIBUTES)(CC_StirRandom                  *  // 0x0146
			 (IS_IMPLEMENTED+DECRYPT_2)),
#endif
#if (PAD_LIST  || CC_ActivateCredential)
    (COMMAND_ATTRIBUTES)(CC_ActivateCredential          *  // 0x0147
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_ADMIN+HANDLE_2_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_Certify)
    (COMMAND_ATTRIBUTES)(CC_Certify                     *  // 0x0148
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_ADMIN+HANDLE_2_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_PolicyNV)
    (COMMAND_ATTRIBUTES)(CC_PolicyNV                    *  // 0x0149
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_CertifyCreation)
    (COMMAND_ATTRIBUTES)(CC_CertifyCreation             *  // 0x014a
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_Duplicate)
    (COMMAND_ATTRIBUTES)(CC_Duplicate                   *  // 0x014b
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_DUP+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_GetTime)
    (COMMAND_ATTRIBUTES)(CC_GetTime                     *  // 0x014c
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+HANDLE_2_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_GetSessionAuditDigest)
    (COMMAND_ATTRIBUTES)(CC_GetSessionAuditDigest       *  // 0x014d
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+HANDLE_2_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_NV_Read)
    (COMMAND_ATTRIBUTES)(CC_NV_Read                     *  // 0x014e
			 (IS_IMPLEMENTED+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_NV_ReadLock)
    (COMMAND_ATTRIBUTES)(CC_NV_ReadLock                 *  // 0x014f
			 (IS_IMPLEMENTED+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_ObjectChangeAuth)
    (COMMAND_ATTRIBUTES)(CC_ObjectChangeAuth            *  // 0x0150
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_ADMIN+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_PolicySecret)
    (COMMAND_ATTRIBUTES)(CC_PolicySecret                *  // 0x0151
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ALLOW_TRIAL+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_Rewrap)
    (COMMAND_ATTRIBUTES)(CC_Rewrap                      *  // 0x0152
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_Create)
    (COMMAND_ATTRIBUTES)(CC_Create                      *  // 0x0153
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_ECDH_ZGen)
    (COMMAND_ATTRIBUTES)(CC_ECDH_ZGen                   *  // 0x0154
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || (CC_HMAC || CC_MAC))
    (COMMAND_ATTRIBUTES)((CC_HMAC || CC_MAC)            *  // 0x0155
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_Import)
    (COMMAND_ATTRIBUTES)(CC_Import                      *  // 0x0156
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_Load)
    (COMMAND_ATTRIBUTES)(CC_Load                        *  // 0x0157
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2+R_HANDLE)),
#endif
#if (PAD_LIST  || CC_Quote)
    (COMMAND_ATTRIBUTES)(CC_Quote                       *  // 0x0158
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_RSA_Decrypt)
    (COMMAND_ATTRIBUTES)(CC_RSA_Decrypt                 *  // 0x0159
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST )
    (COMMAND_ATTRIBUTES)(0),                               // 0x015a
#endif
#if (PAD_LIST  || (CC_HMAC_Start || CC_MAC_Start))
    (COMMAND_ATTRIBUTES)((CC_HMAC_Start || CC_MAC_Start) *  // 0x015b
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+R_HANDLE)),
#endif
#if (PAD_LIST  || CC_SequenceUpdate)
    (COMMAND_ATTRIBUTES)(CC_SequenceUpdate              *  // 0x015c
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_Sign)
    (COMMAND_ATTRIBUTES)(CC_Sign                        *  // 0x015d
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_Unseal)
    (COMMAND_ATTRIBUTES)(CC_Unseal                      *  // 0x015e
			 (IS_IMPLEMENTED+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST )
    (COMMAND_ATTRIBUTES)(0),                               // 0x015f
#endif
#if (PAD_LIST  || CC_PolicySigned)
    (COMMAND_ATTRIBUTES)(CC_PolicySigned                *  // 0x0160
			 (IS_IMPLEMENTED+DECRYPT_2+ALLOW_TRIAL+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_ContextLoad)
    (COMMAND_ATTRIBUTES)(CC_ContextLoad                 *  // 0x0161
			 (IS_IMPLEMENTED+NO_SESSIONS+R_HANDLE)),
#endif
#if (PAD_LIST  || CC_ContextSave)
    (COMMAND_ATTRIBUTES)(CC_ContextSave                 *  // 0x0162
			 (IS_IMPLEMENTED+NO_SESSIONS)),
#endif
#if (PAD_LIST  || CC_ECDH_KeyGen)
    (COMMAND_ATTRIBUTES)(CC_ECDH_KeyGen                 *  // 0x0163
			 (IS_IMPLEMENTED+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_EncryptDecrypt)
    (COMMAND_ATTRIBUTES)(CC_EncryptDecrypt              *  // 0x0164
			 (IS_IMPLEMENTED+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_FlushContext)
    (COMMAND_ATTRIBUTES)(CC_FlushContext                *  // 0x0165
			 (IS_IMPLEMENTED+NO_SESSIONS)),
#endif
#if (PAD_LIST )
    (COMMAND_ATTRIBUTES)(0),                               // 0x0166
#endif
#if (PAD_LIST  || CC_LoadExternal)
    (COMMAND_ATTRIBUTES)(CC_LoadExternal                *  // 0x0167
			 (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2+R_HANDLE)),
#endif
#if (PAD_LIST  || CC_MakeCredential)
    (COMMAND_ATTRIBUTES)(CC_MakeCredential              *  // 0x0168
			 (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_NV_ReadPublic)
    (COMMAND_ATTRIBUTES)(CC_NV_ReadPublic               *  // 0x0169
			 (IS_IMPLEMENTED+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_PolicyAuthorize)
    (COMMAND_ATTRIBUTES)(CC_PolicyAuthorize             *  // 0x016a
			 (IS_IMPLEMENTED+DECRYPT_2+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_PolicyAuthValue)
    (COMMAND_ATTRIBUTES)(CC_PolicyAuthValue             *  // 0x016b
			 (IS_IMPLEMENTED+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_PolicyCommandCode)
    (COMMAND_ATTRIBUTES)(CC_PolicyCommandCode           *  // 0x016c
			 (IS_IMPLEMENTED+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_PolicyCounterTimer)
    (COMMAND_ATTRIBUTES)(CC_PolicyCounterTimer          *  // 0x016d
			 (IS_IMPLEMENTED+DECRYPT_2+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_PolicyCpHash)
    (COMMAND_ATTRIBUTES)(CC_PolicyCpHash                *  // 0x016e
			 (IS_IMPLEMENTED+DECRYPT_2+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_PolicyLocality)
    (COMMAND_ATTRIBUTES)(CC_PolicyLocality              *  // 0x016f
			 (IS_IMPLEMENTED+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_PolicyNameHash)
    (COMMAND_ATTRIBUTES)(CC_PolicyNameHash              *  // 0x0170
			 (IS_IMPLEMENTED+DECRYPT_2+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_PolicyOR)
    (COMMAND_ATTRIBUTES)(CC_PolicyOR                    *  // 0x0171
			 (IS_IMPLEMENTED+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_PolicyTicket)
    (COMMAND_ATTRIBUTES)(CC_PolicyTicket                *  // 0x0172
			 (IS_IMPLEMENTED+DECRYPT_2+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_ReadPublic)
    (COMMAND_ATTRIBUTES)(CC_ReadPublic                  *  // 0x0173
			 (IS_IMPLEMENTED+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_RSA_Encrypt)
    (COMMAND_ATTRIBUTES)(CC_RSA_Encrypt                 *  // 0x0174
			 (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2)),
#endif
#if (PAD_LIST )
    (COMMAND_ATTRIBUTES)(0),                               // 0x0175
#endif
#if (PAD_LIST  || CC_StartAuthSession)
    (COMMAND_ATTRIBUTES)(CC_StartAuthSession            *  // 0x0176
			 (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2+R_HANDLE)),
#endif
#if (PAD_LIST  || CC_VerifySignature)
    (COMMAND_ATTRIBUTES)(CC_VerifySignature             *  // 0x0177
			 (IS_IMPLEMENTED+DECRYPT_2)),
#endif
#if (PAD_LIST  || CC_ECC_Parameters)
    (COMMAND_ATTRIBUTES)(CC_ECC_Parameters              *  // 0x0178
			 (IS_IMPLEMENTED)),
#endif
#if (PAD_LIST  || CC_FirmwareRead)
    (COMMAND_ATTRIBUTES)(CC_FirmwareRead                *  // 0x0179
			 (IS_IMPLEMENTED+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_GetCapability)
    (COMMAND_ATTRIBUTES)(CC_GetCapability               *  // 0x017a
			 (IS_IMPLEMENTED)),
#endif
#if (PAD_LIST  || CC_GetRandom)
    (COMMAND_ATTRIBUTES)(CC_GetRandom                   *  // 0x017b
			 (IS_IMPLEMENTED+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_GetTestResult)
    (COMMAND_ATTRIBUTES)(CC_GetTestResult               *  // 0x017c
			 (IS_IMPLEMENTED+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_Hash)
    (COMMAND_ATTRIBUTES)(CC_Hash                        *  // 0x017d
			 (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_PCR_Read)
    (COMMAND_ATTRIBUTES)(CC_PCR_Read                    *  // 0x017e
			 (IS_IMPLEMENTED)),
#endif
#if (PAD_LIST  || CC_PolicyPCR)
    (COMMAND_ATTRIBUTES)(CC_PolicyPCR                   *  // 0x017f
			 (IS_IMPLEMENTED+DECRYPT_2+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_PolicyRestart)
    (COMMAND_ATTRIBUTES)(CC_PolicyRestart               *  // 0x0180
			 (IS_IMPLEMENTED+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_ReadClock)
    (COMMAND_ATTRIBUTES)(CC_ReadClock                   *  // 0x0181
			 (IS_IMPLEMENTED)),
#endif
#if (PAD_LIST  || CC_PCR_Extend)
    (COMMAND_ATTRIBUTES)(CC_PCR_Extend                  *  // 0x0182
			 (IS_IMPLEMENTED+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_PCR_SetAuthValue)
    (COMMAND_ATTRIBUTES)(CC_PCR_SetAuthValue            *  // 0x0183
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER)),
#endif
#if (PAD_LIST  || CC_NV_Certify)
    (COMMAND_ATTRIBUTES)(CC_NV_Certify                  *  // 0x0184
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+HANDLE_2_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_EventSequenceComplete)
    (COMMAND_ATTRIBUTES)(CC_EventSequenceComplete       *  // 0x0185
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+HANDLE_2_USER)),
#endif
#if (PAD_LIST  || CC_HashSequenceStart)
    (COMMAND_ATTRIBUTES)(CC_HashSequenceStart           *  // 0x0186
			 (IS_IMPLEMENTED+DECRYPT_2+R_HANDLE)),
#endif
#if (PAD_LIST  || CC_PolicyPhysicalPresence)
    (COMMAND_ATTRIBUTES)(CC_PolicyPhysicalPresence      *  // 0x0187
			 (IS_IMPLEMENTED+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_PolicyDuplicationSelect)
    (COMMAND_ATTRIBUTES)(CC_PolicyDuplicationSelect     *  // 0x0188
			 (IS_IMPLEMENTED+DECRYPT_2+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_PolicyGetDigest)
    (COMMAND_ATTRIBUTES)(CC_PolicyGetDigest             *  // 0x0189
			 (IS_IMPLEMENTED+ALLOW_TRIAL+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_TestParms)
    (COMMAND_ATTRIBUTES)(CC_TestParms                   *  // 0x018a
			 (IS_IMPLEMENTED)),
#endif
#if (PAD_LIST  || CC_Commit)
    (COMMAND_ATTRIBUTES)(CC_Commit                      *  // 0x018b
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_PolicyPassword)
    (COMMAND_ATTRIBUTES)(CC_PolicyPassword              *  // 0x018c
			 (IS_IMPLEMENTED+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_ZGen_2Phase)
    (COMMAND_ATTRIBUTES)(CC_ZGen_2Phase                 *  // 0x018d
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_EC_Ephemeral)
    (COMMAND_ATTRIBUTES)(CC_EC_Ephemeral                *  // 0x018e
			 (IS_IMPLEMENTED+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_PolicyNvWritten)
    (COMMAND_ATTRIBUTES)(CC_PolicyNvWritten             *  // 0x018f
			 (IS_IMPLEMENTED+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_PolicyTemplate)
    (COMMAND_ATTRIBUTES)(CC_PolicyTemplate              *  // 0x0190
			 (IS_IMPLEMENTED+DECRYPT_2+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_CreateLoaded)
    (COMMAND_ATTRIBUTES)(CC_CreateLoaded                *  // 0x0191
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+PP_COMMAND+ENCRYPT_2+R_HANDLE)),
#endif
#if (PAD_LIST  || CC_PolicyAuthorizeNV)
    (COMMAND_ATTRIBUTES)(CC_PolicyAuthorizeNV           *  // 0x0192
			 (IS_IMPLEMENTED+HANDLE_1_USER+ALLOW_TRIAL)),
#endif
#if (PAD_LIST  || CC_EncryptDecrypt2)
    (COMMAND_ATTRIBUTES)(CC_EncryptDecrypt2             *  // 0x0193
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_AC_GetCapability)
    (COMMAND_ATTRIBUTES)(CC_AC_GetCapability            *  // 0x0194
			 (IS_IMPLEMENTED)),
#endif
#if (PAD_LIST  || CC_AC_Send)
    (COMMAND_ATTRIBUTES)(CC_AC_Send                     *  // 0x0195
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_DUP+HANDLE_2_USER)),
#endif
#if (PAD_LIST  || CC_Policy_AC_SendSelect)
    (COMMAND_ATTRIBUTES)(CC_Policy_AC_SendSelect        *  // 0x0196
			 (IS_IMPLEMENTED+DECRYPT_2+ALLOW_TRIAL)),
#endif
#if (PAD_LIST || CC_CertifyX509)
    (COMMAND_ATTRIBUTES)(CC_CertifyX509                 *  // 0x0197
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_ADMIN+HANDLE_2_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST || CC_ACT_SetTimeout)
    (COMMAND_ATTRIBUTES)(CC_ACT_SetTimeout              *  // 0x0198
			 (IS_IMPLEMENTED+HANDLE_1_USER)),
#endif
#if (PAD_LIST || CC_ECC_Encrypt)
    (COMMAND_ATTRIBUTES)(CC_ECC_Encrypt                 *  // 0x0199
			 (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2)),
#endif
#if (PAD_LIST || CC_ECC_Decrypt)
    (COMMAND_ATTRIBUTES)(CC_ECC_Decrypt                 *  // 0x019A
			 (IS_IMPLEMENTED+DECRYPT_2+HANDLE_1_USER+ENCRYPT_2)),
#endif
#if (PAD_LIST  || CC_Vendor_TCG_Test)
    (COMMAND_ATTRIBUTES)(CC_Vendor_TCG_Test             *  // 0x0000
			 (IS_IMPLEMENTED+DECRYPT_2+ENCRYPT_2)),
#endif

    0
};

#endif  // _COMMAND_CODE_ATTRIBUTES_
