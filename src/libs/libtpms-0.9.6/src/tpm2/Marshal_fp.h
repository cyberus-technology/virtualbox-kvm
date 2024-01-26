/********************************************************************************/
/*										*/
/*			   Parameter Marshaling  				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Marshal_fp.h $		*/
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

#ifndef MARSHAL_FP_H
#define MARSHAL_FP_H

#include "TpmTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

    UINT16
    UINT8_Marshal(UINT8 *source, BYTE **buffer, INT32 *size);
    UINT16
    UINT16_Marshal(UINT16 *source, BYTE **buffer, INT32 *size);
    UINT16
    UINT32_Marshal(UINT32 *source, BYTE **buffer, INT32 *size);
    UINT16
    UINT64_Marshal(UINT64 *source, BYTE **buffer, INT32 *size);
    UINT16
    Array_Marshal(BYTE *sourceBuffer, UINT16 sourceSize, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_Marshal(TPM2B *source, UINT32 maxSize, BYTE **buffer, INT32 *size); // libtpms changed
    UINT16
    TPM_KEY_BITS_Marshal(TPM_KEY_BITS *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM_CONSTANTS32_Marshal(TPM_CONSTANTS32 *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM_ALG_ID_Marshal(TPM_ALG_ID *source, BYTE **buffer, INT32 *size);    
    UINT16
    TPM_ECC_CURVE_Marshal(TPM_ECC_CURVE *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM_CC_Marshal(TPM_CC *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM_RC_Marshal(TPM_RC *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM_ST_Marshal(TPM_ST *source, BYTE **buffer, INT32 *size);
    INT16
    TPM_CAP_Marshal(TPM_CAP *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM_PT_Marshal(TPM_PT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM_PT_PCR_Marshal(TPM_PT_PCR *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM_HANDLE_Marshal(TPM_HANDLE *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMA_ALGORITHM_Marshal(TPMA_ALGORITHM *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMA_OBJECT_Marshal(TPMA_OBJECT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMA_SESSION_Marshal(TPMA_SESSION *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMA_LOCALITY_Marshal(TPMA_LOCALITY *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMA_CC_Marshal(TPMA_CC *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_YES_NO_Marshal(TPMI_YES_NO *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMA_ACT_Marshal(TPMA_ACT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_DH_SAVED_Marshal(TPMI_DH_CONTEXT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_RH_HIERARCHY_Marshal(TPMI_RH_HIERARCHY *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_RH_NV_INDEX_Marshal(TPMI_RH_NV_INDEX *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_ALG_HASH_Marshal(TPMI_ALG_HASH *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_ALG_SYM_OBJECT_Marshal(TPMI_ALG_SYM_OBJECT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMU_SYM_KEY_BITS_Marshal(TPMU_SYM_KEY_BITS *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMI_ALG_SYM_MODE_Marshal(TPMI_ALG_SYM_MODE *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_ALG_KDF_Marshal(TPMI_ALG_KDF *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_ALG_SIG_SCHEME_Marshal(TPMI_ALG_SIG_SCHEME *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMU_HA_Marshal(TPMU_HA *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMT_HA_Marshal(TPMT_HA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_DIGEST_Marshal(TPM2B_DIGEST *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_DATA_Marshal(TPM2B_DATA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_NONCE_Marshal(TPM2B_NONCE *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_AUTH_Marshal(TPM2B_AUTH *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_MAX_BUFFER_Marshal(TPM2B_MAX_BUFFER *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_MAX_NV_BUFFER_Marshal(TPM2B_MAX_NV_BUFFER *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_TIMEOUT_Marshal(TPM2B_TIMEOUT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_IV_Marshal(TPM2B_IV *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_NAME_Marshal(TPM2B_NAME *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_PCR_SELECTION_Marshal(TPMS_PCR_SELECTION *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMT_TK_CREATION_Marshal(TPMT_TK_CREATION *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMT_TK_VERIFIED_Marshal(TPMT_TK_VERIFIED *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMT_TK_AUTH_Marshal(TPMT_TK_AUTH *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMT_TK_HASHCHECK_Marshal(TPMT_TK_HASHCHECK *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_ALG_PROPERTY_Marshal(TPMS_ALG_PROPERTY *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_TAGGED_PCR_SELECT_Marshal(TPMS_TAGGED_PCR_SELECT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_TAGGED_POLICY_Marshal(TPMS_TAGGED_POLICY *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_ACT_DATA_Marshal(TPMS_ACT_DATA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_TAGGED_PROPERTY_Marshal(TPMS_TAGGED_PROPERTY *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_CC_Marshal(TPML_CC *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_CCA_Marshal(TPML_CCA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_ALG_Marshal(TPML_ALG *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_DIGEST_Marshal(TPML_DIGEST *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_HANDLE_Marshal(TPML_HANDLE *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_DIGEST_VALUES_Marshal(TPML_DIGEST_VALUES *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_PCR_SELECTION_Marshal(TPML_PCR_SELECTION *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_ALG_PROPERTY_Marshal(TPML_ALG_PROPERTY *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_TAGGED_TPM_PROPERTY_Marshal(TPML_TAGGED_TPM_PROPERTY *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_TAGGED_PCR_PROPERTY_Marshal(TPML_TAGGED_PCR_PROPERTY *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_ECC_CURVE_Marshal(TPML_ECC_CURVE *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_TAGGED_POLICY_Marshal(TPML_TAGGED_POLICY *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_ACT_DATA_Marshal(TPML_ACT_DATA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMU_CAPABILITIES_Marshal(TPMU_CAPABILITIES *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMS_CAPABILITY_DATA_Marshal(TPMS_CAPABILITY_DATA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_CLOCK_INFO_Marshal(TPMS_CLOCK_INFO *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_TIME_INFO_Marshal(TPMS_TIME_INFO *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_TIME_ATTEST_INFO_Marshal(TPMS_TIME_ATTEST_INFO *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_CERTIFY_INFO_Marshal(TPMS_CERTIFY_INFO *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_QUOTE_INFO_Marshal(TPMS_QUOTE_INFO *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_COMMAND_AUDIT_INFO_Marshal(TPMS_COMMAND_AUDIT_INFO *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SESSION_AUDIT_INFO_Marshal(TPMS_SESSION_AUDIT_INFO *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_CREATION_INFO_Marshal(TPMS_CREATION_INFO *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_NV_CERTIFY_INFO_Marshal(TPMS_NV_CERTIFY_INFO *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_NV_DIGEST_CERTIFY_INFO_Marshal(TPMS_NV_DIGEST_CERTIFY_INFO *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM_ST_ATTEST_NV_DIGEST_Marshal(TPMS_NV_DIGEST_CERTIFY_INFO *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_ST_ATTEST_Marshal(TPMI_ST_ATTEST *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMU_ATTEST_Marshal(TPMU_ATTEST  *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMS_ATTEST_Marshal(TPMS_ATTEST  *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_ATTEST_Marshal(TPM2B_ATTEST *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_AES_KEY_BITS_Marshal(TPMI_AES_KEY_BITS *source, BYTE **buffer, INT32 *size);
    UINT16			// libtpms added
    TPMI_TDES_KEY_BITS_Marshal(TPMI_TDES_KEY_BITS *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_CAMELLIA_KEY_BITS_Marshal(TPMI_CAMELLIA_KEY_BITS *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMU_SYM_KEY_BITS_Marshal(TPMU_SYM_KEY_BITS *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMU_SYM_MODE_Marshal(TPMU_SYM_MODE *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMT_SYM_DEF_OBJECT_Marshal(TPMT_SYM_DEF_OBJECT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_SYM_KEY_Marshal(TPM2B_SYM_KEY *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SYMCIPHER_PARMS_Marshal(TPMS_SYMCIPHER_PARMS *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_SENSITIVE_DATA_Marshal(TPM2B_SENSITIVE_DATA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SCHEME_HASH_Marshal(TPMS_SCHEME_HASH *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SCHEME_ECDAA_Marshal(TPMS_SCHEME_ECDAA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_ALG_KEYEDHASH_SCHEME_Marshal(TPMI_ALG_KEYEDHASH_SCHEME *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SCHEME_HMAC_Marshal(TPMS_SCHEME_HMAC *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SCHEME_XOR_Marshal(TPMS_SCHEME_XOR *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMT_KEYEDHASH_SCHEME_Marshal(TPMT_KEYEDHASH_SCHEME *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIG_SCHEME_RSASSA_Marshal(TPMS_SIG_SCHEME_RSASSA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIG_SCHEME_RSAPSS_Marshal(TPMS_SIG_SCHEME_RSAPSS *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIG_SCHEME_ECDSA_Marshal(TPMS_SIG_SCHEME_ECDSA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIG_SCHEME_SM2_Marshal(TPMS_SIG_SCHEME_SM2 *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIG_SCHEME_ECSCHNORR_Marshal(TPMS_SIG_SCHEME_ECSCHNORR *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIG_SCHEME_ECDAA_Marshal(TPMS_SIG_SCHEME_ECDAA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_ENC_SCHEME_OAEP_Marshal(TPMS_ENC_SCHEME_OAEP *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_ENC_SCHEME_RSAES_Marshal(TPMS_ENC_SCHEME_RSAES *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMU_SCHEME_KEYEDHASH_Marshal(TPMU_SCHEME_KEYEDHASH *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMS_KEY_SCHEME_ECDH_Marshal(TPMS_KEY_SCHEME_ECDH *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_KEY_SCHEME_ECMQV_Marshal(TPMS_KEY_SCHEME_ECMQV*source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_KDF_SCHEME_MGF1_Marshal(TPMS_KDF_SCHEME_MGF1 *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_KDF_SCHEME_KDF1_SP800_56A_Marshal(TPMS_KDF_SCHEME_KDF1_SP800_56A *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_KDF_SCHEME_KDF2_Marshal(TPMS_KDF_SCHEME_KDF2 *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_KDF_SCHEME_KDF1_SP800_108_Marshal(TPMS_KDF_SCHEME_KDF1_SP800_108 *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMU_KDF_SCHEME_Marshal(TPMU_KDF_SCHEME *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMT_KDF_SCHEME_Marshal(TPMT_KDF_SCHEME *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMU_ASYM_SCHEME_Marshal(TPMU_ASYM_SCHEME  *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMI_ALG_RSA_SCHEME_Marshal(TPMI_ALG_RSA_SCHEME *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMT_RSA_SCHEME_Marshal(TPMT_RSA_SCHEME *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_PUBLIC_KEY_RSA_Marshal(TPM2B_PUBLIC_KEY_RSA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_RSA_KEY_BITS_Marshal(TPMI_RSA_KEY_BITS *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_PRIVATE_KEY_RSA_Marshal(TPM2B_PRIVATE_KEY_RSA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_ECC_PARAMETER_Marshal(TPM2B_ECC_PARAMETER *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_ECC_POINT_Marshal(TPMS_ECC_POINT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_ECC_POINT_Marshal(TPM2B_ECC_POINT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_ALG_ECC_SCHEME_Marshal(TPMI_ALG_ECC_SCHEME *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_ECC_CURVE_Marshal(TPMI_ECC_CURVE *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMT_ECC_SCHEME_Marshal(TPMT_ECC_SCHEME *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_ALGORITHM_DETAIL_ECC_Marshal(TPMS_ALGORITHM_DETAIL_ECC *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIGNATURE_RSA_Marshal(TPMS_SIGNATURE_RSA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIGNATURE_RSASSA_Marshal(TPMS_SIGNATURE_RSASSA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIGNATURE_RSAPSS_Marshal(TPMS_SIGNATURE_RSAPSS *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIGNATURE_ECC_Marshal(TPMS_SIGNATURE_ECC *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIGNATURE_ECDSA_Marshal(TPMS_SIGNATURE_ECDSA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIGNATURE_ECDAA_Marshal(TPMS_SIGNATURE_ECDAA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIGNATURE_SM2_Marshal(TPMS_SIGNATURE_SM2 *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_SIGNATURE_ECSCHNORR_Marshal(TPMS_SIGNATURE_ECSCHNORR *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMU_SIGNATURE_Marshal(TPMU_SIGNATURE *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMT_SIGNATURE_Marshal(TPMT_SIGNATURE *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_ENCRYPTED_SECRET_Marshal(TPM2B_ENCRYPTED_SECRET *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMI_ALG_PUBLIC_Marshal(TPMI_ALG_PUBLIC *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMU_PUBLIC_ID_Marshal(TPMU_PUBLIC_ID *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMS_KEYEDHASH_PARMS_Marshal(TPMS_KEYEDHASH_PARMS *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_RSA_PARMS_Marshal(TPMS_RSA_PARMS *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_ECC_PARMS_Marshal(TPMS_ECC_PARMS *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMU_PUBLIC_PARMS_Marshal(TPMU_PUBLIC_PARMS *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMT_PUBLIC_Marshal(TPMT_PUBLIC *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_PUBLIC_Marshal(TPM2B_PUBLIC *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMU_SENSITIVE_COMPOSITE_Marshal(TPMU_SENSITIVE_COMPOSITE *source, BYTE **buffer, INT32 *size, UINT32 selector);
    UINT16
    TPMT_SENSITIVE_Marshal(TPMT_SENSITIVE *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_PRIVATE_Marshal(TPM2B_PRIVATE *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_ID_OBJECT_Marshal(TPM2B_ID_OBJECT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMA_NV_Marshal(TPMA_NV *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_NV_PUBLIC_Marshal(TPMS_NV_PUBLIC *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_NV_PUBLIC_Marshal(TPM2B_NV_PUBLIC *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_CONTEXT_DATA_Marshal(TPM2B_CONTEXT_DATA  *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_CONTEXT_Marshal(TPMS_CONTEXT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_CREATION_DATA_Marshal(TPMS_CREATION_DATA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM2B_CREATION_DATA_Marshal(TPM2B_CREATION_DATA *source, BYTE **buffer, INT32 *size);
    UINT16
    TPM_AT_Marshal(TPM_AT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPMS_AC_OUTPUT_Marshal(TPMS_AC_OUTPUT *source, BYTE **buffer, INT32 *size);
    UINT16
    TPML_AC_CAPABILITIES_Marshal(TPML_AC_CAPABILITIES *source, BYTE **buffer, INT32 *size);

#ifdef __cplusplus
}
#endif

#endif
