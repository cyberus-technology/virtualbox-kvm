/********************************************************************************/
/*										*/
/*			     Parameter Unmarshaling				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Unmarshal.c $		*/
/*										*/
/* (c) Copyright IBM Corporation 2015 - 2018					*/
/*										*/
/* All rights reserved.								*/
/* 										*/
/* Redistribution and use in source and binary forms, with or without		*/
/* modification, are permitted provided that the following conditions are	*/
/* met:										*/
/* 										*/
/* Redistributions of source code must retain the above copyright notice,	*/
/* this list of conditions and the following disclaimer.			*/
/* 										*/
/* Redistributions in binary form must reproduce the above copyright		*/
/* notice, this list of conditions and the following disclaimer in the		*/
/* documentation and/or other materials provided with the distribution.		*/
/* 										*/
/* Neither the names of the IBM Corporation nor the names of its		*/
/* contributors may be used to endorse or promote products derived from		*/
/* this software without specific prior written permission.			*/
/* 										*/
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS		*/
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT		*/
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR	*/
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT		*/
/* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,	*/
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT		*/
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,	*/
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY	*/
/* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT		*/
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE	*/
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.		*/
/********************************************************************************/

/* rev 136 */

#include <string.h>

#include "Unmarshal_fp.h"
#include "CryptEccMain_fp.h"	// libtpms added

TPM_RC
UINT8_Unmarshal(UINT8 *target, BYTE **buffer, INT32 *size)
{
    if ((UINT32)*size < sizeof(UINT8)) {
	return TPM_RC_INSUFFICIENT;
    }
    *target = (*buffer)[0];
    *buffer += sizeof(UINT8);
    *size -= sizeof(UINT8);
    return TPM_RC_SUCCESS;
}

TPM_RC
INT8_Unmarshal(INT8 *target, BYTE **buffer, INT32 *size)
{
    return UINT8_Unmarshal((UINT8 *)target, buffer, size);
}

TPM_RC
UINT16_Unmarshal(UINT16 *target, BYTE **buffer, INT32 *size)
{
    if ((UINT32)*size < sizeof(UINT16)) {
	return TPM_RC_INSUFFICIENT;
    }
    *target = ((UINT16)((*buffer)[0]) << 8) |
	      ((UINT16)((*buffer)[1]) << 0);
    *buffer += sizeof(UINT16);
    *size -= sizeof(UINT16);
    return TPM_RC_SUCCESS;
}

TPM_RC
UINT32_Unmarshal(UINT32 *target, BYTE **buffer, INT32 *size)
{
    if ((UINT32)*size < sizeof(UINT32)) {
	return TPM_RC_INSUFFICIENT;
    }
    *target = ((UINT32)((*buffer)[0]) << 24) |
	      ((UINT32)((*buffer)[1]) << 16) |
	      ((UINT32)((*buffer)[2]) <<  8) |
	      ((UINT32)((*buffer)[3]) <<  0);
    *buffer += sizeof(UINT32);
    *size -= sizeof(UINT32);
    return TPM_RC_SUCCESS;
}

TPM_RC
UINT64_Unmarshal(UINT64 *target, BYTE **buffer, INT32 *size)
{
    if ((UINT32)*size < sizeof(UINT64)) {
	return TPM_RC_INSUFFICIENT;
    }
    *target = ((UINT64)((*buffer)[0]) << 56) |
	      ((UINT64)((*buffer)[1]) << 48) |
	      ((UINT64)((*buffer)[2]) << 40) |
	      ((UINT64)((*buffer)[3]) << 32) |
	      ((UINT64)((*buffer)[4]) << 24) |
	      ((UINT64)((*buffer)[5]) << 16) |
	      ((UINT64)((*buffer)[6]) <<  8) |
	      ((UINT64)((*buffer)[7]) <<  0);
    *buffer += sizeof(UINT64);
    *size -= sizeof(UINT64);
    return TPM_RC_SUCCESS;
}

TPM_RC
Array_Unmarshal(BYTE *targetBuffer, UINT16 targetSize, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (targetSize > *size) {
	rc = TPM_RC_INSUFFICIENT;
    }
    else {
	memcpy(targetBuffer, *buffer, targetSize);
	*buffer += targetSize;
	*size -= targetSize;
    }
    return rc;
}

TPM_RC
TPM2B_Unmarshal(TPM2B *target, UINT16 targetSize, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size > targetSize) {
	    rc = TPM_RC_SIZE;
	    target->size = 0; // libtpms added
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = Array_Unmarshal(target->buffer, target->size, buffer, size);
    }
    return rc;
}

/* Table 5 - Definition of Types for Documentation Clarity */

TPM_RC
TPM_KEY_BITS_Unmarshal(TPM_KEY_BITS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(target, buffer, size);  
    }
    return rc;
}

/* Table 7 - Definition of (UINT32) TPM_GENERATED Constants <O> */

#if 0
TPM_RC
TPM_GENERATED_Unmarshal(TPM_GENERATED *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPM_GENERATED orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	if (*target != TPM_GENERATED_VALUE) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}
#endif

/* Table 9 - Definition of (UINT16) TPM_ALG_ID Constants <IN/OUT, S> */

TPM_RC
TPM_ALG_ID_Unmarshal(TPM_ALG_ID *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(target, buffer, size);  
    }
    return rc;
}

/* Table 10 - Definition of (UINT16) {ECC} TPM_ECC_CURVE Constants <IN/OUT, S> */

#if ALG_ECC
TPM_RC
TPM_ECC_CURVE_Unmarshal(TPM_ECC_CURVE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPM_ECC_CURVE orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(target, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_ECC_NONE:
	  case TPM_ECC_NIST_P192:
	  case TPM_ECC_NIST_P224:
	  case TPM_ECC_NIST_P256:
	  case TPM_ECC_NIST_P384:
	  case TPM_ECC_NIST_P521:
	  case TPM_ECC_BN_P256:
	  case TPM_ECC_BN_P638:
	  case TPM_ECC_SM2_P256:
	    break;
	  default:
	    rc = TPM_RC_CURVE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}
#endif

/* Table 13 - Definition of (UINT32) TPM_CC Constants (Numeric Order) <IN/OUT, S> */

TPM_RC
TPM_CC_Unmarshal(TPM_RC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(target, buffer, size);  
    }
    return rc;
}

/* Table 18 - Definition of (INT8) TPM_CLOCK_ADJUST Constants <IN> */

TPM_RC
TPM_CLOCK_ADJUST_Unmarshal(TPM_CLOCK_ADJUST *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPM_CLOCK_ADJUST orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = INT8_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_CLOCK_COARSE_SLOWER:
	  case TPM_CLOCK_MEDIUM_SLOWER:
	  case TPM_CLOCK_FINE_SLOWER:
	  case TPM_CLOCK_NO_CHANGE:
	  case TPM_CLOCK_FINE_FASTER:
	  case TPM_CLOCK_MEDIUM_FASTER:
	  case TPM_CLOCK_COARSE_FASTER:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 19 - Definition of (UINT16) TPM_EO Constants <IN/OUT> */

TPM_RC
TPM_EO_Unmarshal(TPM_EO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPM_EO orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_EO_EQ:
	  case TPM_EO_NEQ:
	  case TPM_EO_SIGNED_GT:
	  case TPM_EO_UNSIGNED_GT:
	  case TPM_EO_SIGNED_LT:
	  case TPM_EO_UNSIGNED_LT:
	  case TPM_EO_SIGNED_GE:
	  case TPM_EO_UNSIGNED_GE:
	  case TPM_EO_SIGNED_LE:
	  case TPM_EO_UNSIGNED_LE:
	  case TPM_EO_BITSET:
	  case TPM_EO_BITCLEAR:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 20 - Definition of (UINT16) TPM_ST Constants <IN/OUT, S> */

TPM_RC
TPM_ST_Unmarshal(TPM_ST *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPM_ST orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_ST_RSP_COMMAND:
	  case TPM_ST_NULL:
	  case TPM_ST_NO_SESSIONS:
	  case TPM_ST_SESSIONS:
	  case TPM_ST_ATTEST_NV:
	  case TPM_ST_ATTEST_COMMAND_AUDIT:
	  case TPM_ST_ATTEST_SESSION_AUDIT:
	  case TPM_ST_ATTEST_CERTIFY:
	  case TPM_ST_ATTEST_QUOTE:
	  case TPM_ST_ATTEST_TIME:
	  case TPM_ST_ATTEST_CREATION:
	  case TPM_ST_CREATION:
	  case TPM_ST_VERIFIED:
	  case TPM_ST_AUTH_SECRET:
	  case TPM_ST_HASHCHECK:
	  case TPM_ST_AUTH_SIGNED:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 20 - Definition of (UINT16) TPM_SU Constants <IN> */

TPM_RC
TPM_SU_Unmarshal(TPM_SU *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPM_SU orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_SU_CLEAR:
	  case TPM_SU_STATE:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 21 - Definition of (UINT8) TPM_SE Constants <IN> */

TPM_RC
TPM_SE_Unmarshal(TPM_SE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPM_SE orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT8_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_SE_HMAC:
	  case TPM_SE_POLICY:
	  case TPM_SE_TRIAL:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 22 - Definition of (UINT32) TPM_CAP Constants  */

TPM_RC
TPM_CAP_Unmarshal(TPM_CAP *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPM_CAP orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_CAP_ALGS:
	  case TPM_CAP_HANDLES:
	  case TPM_CAP_COMMANDS:
	  case TPM_CAP_PP_COMMANDS:
	  case TPM_CAP_AUDIT_COMMANDS:
	  case TPM_CAP_PCRS:
	  case TPM_CAP_TPM_PROPERTIES:
	  case TPM_CAP_PCR_PROPERTIES:
	  case TPM_CAP_ECC_CURVES:
	  case TPM_CAP_AUTH_POLICIES:
	  case TPM_CAP_ACT:
	  case TPM_CAP_VENDOR_PROPERTY:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 24 - Definition of (UINT32) TPM_PT Constants <IN/OUT, S> */

TPM_RC
TPM_PT_Unmarshal(TPM_HANDLE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(target, buffer, size);  
    }
    return rc;
}

/* Table 25 - Definition of (UINT32) TPM_PT_PCR Constants <IN/OUT, S> */

TPM_RC
TPM_PT_PCR_Unmarshal(TPM_PT_PCR *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(target, buffer, size);  
    }
    return rc;
}

/* Table 27 - Definition of Types for Handles */

TPM_RC
TPM_HANDLE_Unmarshal(TPM_HANDLE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(target, buffer, size);  
    }
    return rc;
}

/* Table 31 - Definition of (UINT32) TPMA_ALGORITHM Bits */

TPM_RC
TPMA_ALGORITHM_Unmarshal(TPMA_ALGORITHM *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMA_ALGORITHM orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal((UINT32 *)target, buffer, size); /* libtpms changed */
    }
    if (rc == TPM_RC_SUCCESS) {
	if (*target & TPMA_ALGORITHM_reserved) {
	    rc = TPM_RC_RESERVED_BITS;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 32 - Definition of (UINT32) TPMA_OBJECT Bits */

TPM_RC
TPMA_OBJECT_Unmarshal(TPMA_OBJECT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMA_OBJECT orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal((UINT32 *)target, buffer, size); /* libtpms changed */
    }
    if (rc == TPM_RC_SUCCESS) {
	if (*target & TPMA_OBJECT_reserved) {
	    rc = TPM_RC_RESERVED_BITS;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 33 - Definition of (UINT8) TPMA_SESSION Bits <IN/OUT> */

TPM_RC
TPMA_SESSION_Unmarshal(TPMA_SESSION *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMA_SESSION orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT8_Unmarshal((UINT8 *)target, buffer, size);  /* libtpms changed */
    }
    if (rc == TPM_RC_SUCCESS) {
	if (*target & TPMA_SESSION_reserved) {
	    rc = TPM_RC_RESERVED_BITS;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 34 - Definition of (UINT8) TPMA_LOCALITY Bits <IN/OUT> */

TPM_RC
TPMA_LOCALITY_Unmarshal(TPMA_LOCALITY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT8_Unmarshal((UINT8 *)target, buffer, size);  /* libtpms changed */
    }
    return rc;
}

/* Table 38 - Definition of (TPM_CC) TPMA_CC Bits <OUT> */

TPM_RC
TPMA_CC_Unmarshal(TPMA_CC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMA_CC orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal((UINT32 *)target, buffer, size); /* libtpms changed */
    }
    if (rc == TPM_RC_SUCCESS) {
	if (*target & TPMA_CC_reserved) {
	    rc = TPM_RC_RESERVED_BITS;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 39 - Definition of (BYTE) TPMI_YES_NO Type */

TPM_RC
TPMI_YES_NO_Unmarshal(TPMI_YES_NO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_YES_NO orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT8_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case NO:
	  case YES:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 40 - Definition of (TPM_HANDLE) TPMI_DH_OBJECT Type */

TPM_RC
TPMI_DH_OBJECT_Unmarshal(TPMI_DH_OBJECT *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_DH_OBJECT orig_target = *target; // libtpms added
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotTransient = (*target < TRANSIENT_FIRST) || (*target > TRANSIENT_LAST);
	BOOL isNotPersistent = (*target < PERSISTENT_FIRST) || (*target > PERSISTENT_LAST);
	BOOL isNotLegalNull = (*target != TPM_RH_NULL) || !allowNull;
	if (isNotTransient &&
	    isNotPersistent &&
	    isNotLegalNull) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 2:41 - Definition of TPMI_DH_PARENT Type (InterfaceTable()) */

TPM_RC
TPMI_DH_PARENT_Unmarshal(TPMI_DH_PARENT *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_DH_PARENT orig_target = *target; // libtpms added
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotTransient = (*target < TRANSIENT_FIRST) || (*target > TRANSIENT_LAST);
	BOOL isNotPersistent = (*target < PERSISTENT_FIRST) || (*target > PERSISTENT_LAST);
	BOOL isNotOwner = *target != TPM_RH_OWNER;
	BOOL isNotPlatform = *target != TPM_RH_PLATFORM;
	BOOL isNotEndorsement = *target != TPM_RH_ENDORSEMENT;
	BOOL isNotLegalNull = (*target != TPM_RH_NULL) || !allowNull;
	if (isNotTransient &&
	    isNotPersistent &&
	    isNotOwner &&
	    isNotPlatform &&
	    isNotEndorsement && 
	    isNotLegalNull) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 41 - Definition of (TPM_HANDLE) TPMI_DH_PERSISTENT Type */

TPM_RC
TPMI_DH_PERSISTENT_Unmarshal(TPMI_DH_PERSISTENT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_DH_PERSISTENT orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotPersistent = (*target < PERSISTENT_FIRST) || (*target > PERSISTENT_LAST);
	if (isNotPersistent) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 42 - Definition of (TPM_HANDLE) TPMI_DH_ENTITY Type <IN> */

TPM_RC
TPMI_DH_ENTITY_Unmarshal(TPMI_DH_ENTITY *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_DH_ENTITY orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotOwner = *target != TPM_RH_OWNER;
	BOOL isNotEndorsement = *target != TPM_RH_ENDORSEMENT;
	BOOL isNotPlatform = *target != TPM_RH_PLATFORM;
	BOOL isNotLockout = *target != TPM_RH_LOCKOUT;
	BOOL isNotTransient = (*target < TRANSIENT_FIRST) || (*target > TRANSIENT_LAST);
	BOOL isNotPersistent = (*target < PERSISTENT_FIRST) || (*target > PERSISTENT_LAST);
	BOOL isNotNv = (*target < NV_INDEX_FIRST) || (*target > NV_INDEX_LAST);
	BOOL isNotPcr = (*target > PCR_LAST);
	BOOL isNotAuth = (*target < TPM_RH_AUTH_00) || (*target > TPM_RH_AUTH_FF);
	BOOL isNotLegalNull = (*target != TPM_RH_NULL) || !allowNull;
	if (isNotOwner &&
	    isNotEndorsement &&
	    isNotPlatform &&
	    isNotLockout &&
	    isNotTransient &&
	    isNotPersistent &&
	    isNotNv &&
	    isNotPcr &&
	    isNotAuth &&
	    isNotLegalNull) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 43 - Definition of (TPM_HANDLE) TPMI_DH_PCR Type <IN> */

TPM_RC
TPMI_DH_PCR_Unmarshal(TPMI_DH_PCR *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_DH_PCR orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotPcr = (*target > PCR_LAST);
	BOOL isNotLegalNull = (*target != TPM_RH_NULL) || !allowNull;
	if (isNotPcr &&
	    isNotLegalNull) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 44 - Definition of (TPM_HANDLE) TPMI_SH_AUTH_SESSION Type <IN/OUT> */

TPM_RC
TPMI_SH_AUTH_SESSION_Unmarshal(TPMI_SH_AUTH_SESSION *target, BYTE **buffer, INT32 *size, BOOL allowPwd)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_SH_AUTH_SESSION orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotHmacSession = (*target < HMAC_SESSION_FIRST ) || (*target > HMAC_SESSION_LAST);
	BOOL isNotPolicySession = (*target < POLICY_SESSION_FIRST) || (*target > POLICY_SESSION_LAST);
	BOOL isNotLegalPwd = (*target != TPM_RS_PW) || !allowPwd;
	if (isNotHmacSession &&
	    isNotPolicySession &&
	    isNotLegalPwd) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 45 - Definition of (TPM_HANDLE) TPMI_SH_HMAC Type <IN/OUT> */

TPM_RC
TPMI_SH_HMAC_Unmarshal(TPMI_SH_HMAC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_SH_HMAC orig_target = *target; // libtpms added
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotHmacSession = (*target < HMAC_SESSION_FIRST ) || (*target > HMAC_SESSION_LAST);
	if (isNotHmacSession) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 46 - Definition of (TPM_HANDLE) TPMI_SH_POLICY Type <IN/OUT> */

TPM_RC
TPMI_SH_POLICY_Unmarshal(TPMI_SH_POLICY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_SH_POLICY orig_target = *target; // libtpms added
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotPolicySession = (*target < POLICY_SESSION_FIRST) || (*target > POLICY_SESSION_LAST);
	if (isNotPolicySession) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 47 - Definition of (TPM_HANDLE) TPMI_DH_CONTEXT Type  */

TPM_RC
TPMI_DH_CONTEXT_Unmarshal(TPMI_DH_CONTEXT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_DH_CONTEXT orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotHmacSession = (*target < HMAC_SESSION_FIRST ) || (*target > HMAC_SESSION_LAST);
	BOOL isNotPolicySession = (*target < POLICY_SESSION_FIRST) || (*target > POLICY_SESSION_LAST);
	BOOL isNotTransient = (*target < TRANSIENT_FIRST) || (*target > TRANSIENT_LAST);
	if (isNotHmacSession &&
	    isNotPolicySession &&
	    isNotTransient) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 49 - Definition of (TPM_HANDLE) TPMI_DH_SAVED Type */

TPM_RC
TPMI_DH_SAVED_Unmarshal(TPMI_DH_SAVED *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_DH_SAVED orig_target = *target; // libtpms added
    allowNull = allowNull;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotHmacSession = (*target < HMAC_SESSION_FIRST ) || (*target > HMAC_SESSION_LAST);
	BOOL isNotPolicySession = (*target < POLICY_SESSION_FIRST) || (*target > POLICY_SESSION_LAST);
	BOOL isNotTransientObject = (*target != 0x80000000);
	BOOL isNotSequenceObject = (*target != 0x80000001);
	BOOL isNotTransientStClear = (*target != 0x80000002);
	if (isNotHmacSession &&
	    isNotPolicySession &&
	    isNotTransientObject &&
	    isNotSequenceObject &&
	    isNotTransientStClear) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 48 - Definition of (TPM_HANDLE) TPMI_RH_HIERARCHY Type  */

TPM_RC
TPMI_RH_HIERARCHY_Unmarshal(TPMI_RH_HIERARCHY *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_HIERARCHY orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_OWNER:
	  case TPM_RH_PLATFORM:
	  case TPM_RH_ENDORSEMENT:
	    break;
	  case TPM_RH_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 49 - Definition of (TPM_HANDLE) TPMI_RH_ENABLES Type */

TPM_RC
TPMI_RH_ENABLES_Unmarshal(TPMI_RH_ENABLES *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_ENABLES orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_OWNER:
	  case TPM_RH_PLATFORM:
	  case TPM_RH_ENDORSEMENT:
	  case TPM_RH_PLATFORM_NV:
	    break;
	  case TPM_RH_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 50 - Definition of (TPM_HANDLE) TPMI_RH_HIERARCHY_AUTH Type <IN> */

TPM_RC
TPMI_RH_HIERARCHY_AUTH_Unmarshal(TPMI_RH_HIERARCHY_AUTH *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_HIERARCHY_AUTH orig_target = *target; // libtpms added
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_OWNER:
	  case TPM_RH_PLATFORM:
	  case TPM_RH_ENDORSEMENT:
	  case TPM_RH_LOCKOUT:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 50 - Definition of (TPM_HANDLE) TPMI_RH_HIERARCHY_POLICY Type <IN> */

TPM_RC
TPMI_RH_HIERARCHY_POLICY_Unmarshal(TPMI_RH_HIERARCHY_POLICY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_HIERARCHY_POLICY orig_target = *target; // libtpms added
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_OWNER:
	  case TPM_RH_PLATFORM:
	  case TPM_RH_ENDORSEMENT:
	  case TPM_RH_LOCKOUT:
	    break;
	  default:
	      {
		  BOOL isNotHP =  (*target < TPM_RH_ACT_0) || (*target > TPM_RH_ACT_F);
		  if (isNotHP) {
		      rc = TPM_RC_VALUE;
		      *target = orig_target; // libtpms added
		  }
	      }
	}
    }
    return rc;
}

/* Table 51 - Definition of (TPM_HANDLE) TPMI_RH_PLATFORM Type <IN> */

TPM_RC
TPMI_RH_PLATFORM_Unmarshal(TPMI_RH_PLATFORM *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_PLATFORM orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_PLATFORM:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 53 - Definition of (TPM_HANDLE) TPMI_RH_ENDORSEMENT Type <IN> */

TPM_RC
TPMI_RH_ENDORSEMENT_Unmarshal(TPMI_RH_ENDORSEMENT *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_ENDORSEMENT orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_ENDORSEMENT:
	    break;
	  case TPM_RH_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 54 - Definition of (TPM_HANDLE) TPMI_RH_PROVISION Type <IN> */

TPM_RC
TPMI_RH_PROVISION_Unmarshal(TPMI_RH_PROVISION *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_PROVISION orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_OWNER:
	  case TPM_RH_PLATFORM:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 55 - Definition of (TPM_HANDLE) TPMI_RH_CLEAR Type <IN> */

TPM_RC
TPMI_RH_CLEAR_Unmarshal(TPMI_RH_CLEAR *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_CLEAR orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_LOCKOUT:
	  case TPM_RH_PLATFORM:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 56 - Definition of (TPM_HANDLE) TPMI_RH_NV_AUTH Type <IN> */

TPM_RC
TPMI_RH_NV_AUTH_Unmarshal(TPMI_RH_NV_AUTH *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_NV_AUTH orig_target = *target; // libtpms added
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_OWNER:
	  case TPM_RH_PLATFORM:
	    break;
	  default:
	      {
		  BOOL isNotNv = (*target < NV_INDEX_FIRST) || (*target > NV_INDEX_LAST);
		  if (isNotNv) {
		      rc = TPM_RC_VALUE;
		      *target = orig_target; // libtpms added
		  }
	      }
	}
    }
    return rc;
}

/* Table 57 - Definition of (TPM_HANDLE) TPMI_RH_LOCKOUT Type <IN> */

TPM_RC
TPMI_RH_LOCKOUT_Unmarshal(TPMI_RH_LOCKOUT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_LOCKOUT orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_RH_LOCKOUT:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 58 - Definition of (TPM_HANDLE) TPMI_RH_NV_INDEX Type <IN/OUT> */

TPM_RC
TPMI_RH_NV_INDEX_Unmarshal(TPMI_RH_NV_INDEX *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_NV_INDEX orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotNv = (*target < NV_INDEX_FIRST) || (*target > NV_INDEX_LAST);
	if (isNotNv) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 64 - Definition of (TPM_HANDLE) TPMI_RH_AC Type <IN> */

TPM_RC
TPMI_RH_AC_Unmarshal(TPMI_RH_AC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_AC orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotAC = (*target < AC_FIRST) || (*target > AC_LAST);
	if (isNotAC) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 65 - Definition of (TPM_HANDLE) TPMI_RH_ACT Type <IN> */

TPM_RC
TPMI_RH_ACT_Unmarshal( TPMI_RH_ACT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RH_ACT orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	BOOL isNotACT = (*target < TPM_RH_ACT_0) || (*target > TPM_RH_ACT_F);
	if (isNotACT) {
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 59 - Definition of (TPM_ALG_ID) TPMI_ALG_HASH Type  */

TPM_RC
TPMI_ALG_HASH_Unmarshal(TPMI_ALG_HASH *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_HASH orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_SHA1
	  case TPM_ALG_SHA1:
#endif
#if ALG_SHA256	
	  case TPM_ALG_SHA256:
#endif
#if ALG_SHA384
	  case 	TPM_ALG_SHA384:
#endif
#if ALG_SHA512
	  case 	TPM_ALG_SHA512:
#endif
#if ALG_SM3_256
	  case TPM_ALG_SM3_256:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_HASH;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 61 - Definition of (TPM_ALG_ID) TPMI_ALG_SYM Type */

TPM_RC
TPMI_ALG_SYM_Unmarshal(TPMI_ALG_SYM *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_SYM orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_AES
	  case TPM_ALG_AES:
#endif
#if ALG_SM4
	  case TPM_ALG_SM4:		
#endif
#if ALG_CAMELLIA
	  case TPM_ALG_CAMELLIA:	
#endif
#if ALG_TDES		// libtpms added begin
	  case TPM_ALG_TDES:
#endif			// libtpms added end
#if ALG_XOR
	  case TPM_ALG_XOR:		
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_SYMMETRIC;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 62 - Definition of (TPM_ALG_ID) TPMI_ALG_SYM_OBJECT Type */

TPM_RC
TPMI_ALG_SYM_OBJECT_Unmarshal(TPMI_ALG_SYM_OBJECT *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_SYM_OBJECT orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_AES
	  case TPM_ALG_AES:
#endif
#if ALG_SM4
	  case TPM_ALG_SM4:		
#endif
#if ALG_CAMELLIA
	  case TPM_ALG_CAMELLIA:	
#endif
#if ALG_TDES		// libtpms added begin
          case TPM_ALG_TDES:
#endif			// iibtpms added end
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_SYMMETRIC;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 63 - Definition of (TPM_ALG_ID) TPMI_ALG_SYM_MODE Type */

TPM_RC
TPMI_ALG_SYM_MODE_Unmarshal(TPMI_ALG_SYM_MODE *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_SYM_MODE orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_CTR	
	  case TPM_ALG_CTR:
#endif
#if ALG_OFB	
	  case TPM_ALG_OFB:
#endif
#if ALG_CBC
	  case TPM_ALG_CBC:
#endif
#if ALG_CFB	
	  case TPM_ALG_CFB:
#endif
#if ALG_ECB	
	  case TPM_ALG_ECB:
#endif
#if ALG_CMAC
	  case TPM_ALG_CMAC:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_MODE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 64 - Definition of (TPM_ALG_ID) TPMI_ALG_KDF Type */

TPM_RC
TPMI_ALG_KDF_Unmarshal(TPMI_ALG_KDF *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_KDF orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_MGF1
	  case TPM_ALG_MGF1:
#endif
#if ALG_KDF1_SP800_56A
	  case TPM_ALG_KDF1_SP800_56A:	
#endif
#if ALG_KDF2
	  case TPM_ALG_KDF2:			
#endif
#if ALG_KDF1_SP800_108
	  case TPM_ALG_KDF1_SP800_108:	
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_KDF;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}
    
/* Table 65 - Definition of (TPM_ALG_ID) TPMI_ALG_SIG_SCHEME Type */

TPM_RC
TPMI_ALG_SIG_SCHEME_Unmarshal(TPMI_ALG_SIG_SCHEME *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_SIG_SCHEME orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_HMAC		
	  case TPM_ALG_HMAC:
#endif
#if ALG_RSASSA	
	  case TPM_ALG_RSASSA:
#endif
#if ALG_RSAPSS	
	  case TPM_ALG_RSAPSS:
#endif
#if ALG_ECDSA		
	  case TPM_ALG_ECDSA:
#endif
#if ALG_ECDAA		
	  case TPM_ALG_ECDAA:
#endif
#if ALG_SM2		
	  case TPM_ALG_SM2:
#endif
#if ALG_ECSCHNORR	
	  case TPM_ALG_ECSCHNORR:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_SCHEME;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 66 - Definition of (TPM_ALG_ID) TPMI_ECC_KEY_EXCHANGE Type */

TPM_RC
TPMI_ECC_KEY_EXCHANGE_Unmarshal(TPMI_ECC_KEY_EXCHANGE *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ECC_KEY_EXCHANGE orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_ECDH
	  case TPM_ALG_ECDH:
#endif
#if ALG_ECMQV
	  case TPM_ALG_ECMQV:
#endif
#if ALG_SM2
	  case TPM_ALG_SM2:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_SCHEME;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}


/* Table 67 - Definition of (TPM_ST) TPMI_ST_COMMAND_TAG Type */

TPM_RC
TPMI_ST_COMMAND_TAG_Unmarshal(TPMI_ST_COMMAND_TAG *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ST_COMMAND_TAG orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ST_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_ST_NO_SESSIONS:
	  case TPM_ST_SESSIONS:
	    break;
	  default:
	    rc = TPM_RC_BAD_TAG;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 70 TPMI_ALG_MAC_SCHEME */

TPM_RC
TPMI_ALG_MAC_SCHEME_Unmarshal(TPMI_ALG_MAC_SCHEME *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_MAC_SCHEME orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_SHA1
	  case TPM_ALG_SHA1:
#endif
#if ALG_SHA256	
	  case TPM_ALG_SHA256:
#endif
#if ALG_SHA384
	  case 	TPM_ALG_SHA384:
#endif
#if ALG_SHA512
	  case 	TPM_ALG_SHA512:
#endif
#if ALG_SM3_256
	  case TPM_ALG_SM3_256:
#endif
#if ALG_CMAC
	  case TPM_ALG_CMAC:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_SYMMETRIC;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}
    
/* Table 70 TPMI_ALG_CIPHER_MODE */

TPM_RC
TPMI_ALG_CIPHER_MODE_Unmarshal(TPMI_ALG_CIPHER_MODE*target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_CIPHER_MODE orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_CTR	
	  case TPM_ALG_CTR:
#endif
#if ALG_OFB	
	  case TPM_ALG_OFB:
#endif
#if ALG_CBC
	  case TPM_ALG_CBC:
#endif
#if ALG_CFB	
	  case TPM_ALG_CFB:
#endif
#if ALG_ECB	
	  case TPM_ALG_ECB:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_MODE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 68 - Definition of TPMS_EMPTY Structure <IN/OUT> */

TPM_RC
TPMS_EMPTY_Unmarshal(TPMS_EMPTY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    target = target;
    buffer = buffer;
    size = size;
    return rc;
}

/* Table 70 - Definition of TPMU_HA Union <IN/OUT, S> */

TPM_RC
TPMU_HA_Unmarshal(TPMU_HA *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
#if ALG_SHA1
      case TPM_ALG_SHA1:
	rc = Array_Unmarshal(target->sha1, SHA1_DIGEST_SIZE, buffer, size);
	break;
#endif
#if ALG_SHA256
      case TPM_ALG_SHA256:
	rc = Array_Unmarshal(target->sha256, SHA256_DIGEST_SIZE, buffer, size);
	break;
#endif
#if ALG_SHA384
      case TPM_ALG_SHA384:
	rc = Array_Unmarshal(target->sha384, SHA384_DIGEST_SIZE, buffer, size);
	break;
#endif
#if ALG_SHA512
      case TPM_ALG_SHA512:
	rc = Array_Unmarshal(target->sha512, SHA512_DIGEST_SIZE, buffer, size);
	break;
#endif
#if ALG_SM3_256
      case TPM_ALG_SM3_256:
	rc = Array_Unmarshal(target->sm3_256, SM3_256_DIGEST_SIZE, buffer, size);
	break;
#endif
      case TPM_ALG_NULL:
	break;
      default:
	rc = TPM_RC_SELECTOR;
    }
    return rc;
}

/* Table 71 - Definition of TPMT_HA Structure <IN/OUT> */

TPM_RC
TPMT_HA_Unmarshal(TPMT_HA *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_HASH_Unmarshal(&target->hashAlg, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_HA_Unmarshal(&target->digest, buffer, size, target->hashAlg);
    }
    return rc;
}

/* Table 72 - Definition of TPM2B_DIGEST Structure */

TPM_RC
TPM2B_DIGEST_Unmarshal(TPM2B_DIGEST *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, sizeof(TPMU_HA), buffer, size);
    }
    return rc;
}

/* Table 73 - Definition of TPM2B_DATA Structure */

TPM_RC
TPM2B_DATA_Unmarshal(TPM2B_DATA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, sizeof(TPMT_HA), buffer, size);
    }
    return rc;
}

/* Table 74 - Definition of Types for TPM2B_NONCE */

TPM_RC
TPM2B_NONCE_Unmarshal(TPM2B_NONCE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 75 - Definition of Types for TPM2B_AUTH */

TPM_RC
TPM2B_AUTH_Unmarshal(TPM2B_AUTH *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 77 - Definition of TPM2B_EVENT Structure */

TPM_RC
TPM2B_EVENT_Unmarshal(TPM2B_EVENT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, sizeof(TPM2B_EVENT) - sizeof(UINT16), buffer, size);
    }
    return rc;
}
 
/* Table 78 - Definition of TPM2B_MAX_BUFFER Structure */

TPM_RC
TPM2B_MAX_BUFFER_Unmarshal(TPM2B_MAX_BUFFER *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, MAX_DIGEST_BUFFER, buffer, size);
    }
    return rc;
}

/* Table 79 - Definition of TPM2B_MAX_NV_BUFFER Structure */

TPM_RC
TPM2B_MAX_NV_BUFFER_Unmarshal(TPM2B_MAX_NV_BUFFER *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, MAX_NV_BUFFER_SIZE, buffer, size);
    }
    return rc;
}

/* Table 80 - Definition of TPM2B_TIMEOUT Structure <IN/OUT> */

TPM_RC
TPM2B_TIMEOUT_Unmarshal(TPM2B_TIMEOUT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, sizeof(target->t.buffer), buffer, size);
    }
    return rc;
}

/* Table 81 - Definition of TPM2B_IV Structure <IN/OUT> */

TPM_RC
TPM2B_IV_Unmarshal(TPM2B_IV *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, MAX_SYM_BLOCK_SIZE, buffer, size);
    }
    return rc;
}

/* Table 83 - Definition of TPM2B_NAME Structure */

TPM_RC
TPM2B_NAME_Unmarshal(TPM2B_NAME *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, sizeof(TPMU_NAME), buffer, size);
    }
    return rc;
}

/* Table 85 - Definition of TPMS_PCR_SELECTION Structure */

TPM_RC
TPMS_PCR_SELECTION_Unmarshal(TPMS_PCR_SELECTION *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_HASH_Unmarshal(&target->hash, buffer, size, NO);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT8_Unmarshal(&target->sizeofSelect, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if ((target->sizeofSelect < PCR_SELECT_MIN) ||
	    (target->sizeofSelect > PCR_SELECT_MAX)) {
	    rc = TPM_RC_VALUE;
	    target->sizeofSelect = 0; // libtpms added
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = Array_Unmarshal(target->pcrSelect, target->sizeofSelect, buffer, size);
    }
    return rc;
}
 
/* Table 88 - Definition of TPMT_TK_CREATION Structure */

TPM_RC
TPMT_TK_CREATION_Unmarshal(TPMT_TK_CREATION *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPM_ST orig_tag = target->tag; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ST_Unmarshal(&target->tag, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->tag != TPM_ST_CREATION) {
	    rc = TPM_RC_TAG;
	    target->tag = orig_tag; // libtpms added
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_RH_HIERARCHY_Unmarshal(&target->hierarchy, buffer, size, YES);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->digest, buffer, size);
    }
    return rc;
}

/* Table 89 - Definition of TPMT_TK_VERIFIED Structure */

TPM_RC
TPMT_TK_VERIFIED_Unmarshal(TPMT_TK_VERIFIED *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPM_ST orig_tag = target->tag; // libtpms added
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ST_Unmarshal(&target->tag, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->tag != TPM_ST_VERIFIED) {
	    rc = TPM_RC_TAG;
	    target->tag = orig_tag; // libtpms added
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_RH_HIERARCHY_Unmarshal(&target->hierarchy, buffer, size, YES);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->digest, buffer, size);
    }
    return rc;
}

/* Table 90 - Definition of TPMT_TK_AUTH Structure */

TPM_RC
TPMT_TK_AUTH_Unmarshal(TPMT_TK_AUTH *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPM_ST orig_tag = target->tag; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ST_Unmarshal(&target->tag, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if ((target->tag != TPM_ST_AUTH_SIGNED) &&
	    (target->tag != TPM_ST_AUTH_SECRET)) {
	    rc = TPM_RC_TAG;
	    target->tag = orig_tag; // libtpms added
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_RH_HIERARCHY_Unmarshal(&target->hierarchy, buffer, size, YES);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->digest, buffer, size);
    }
    return rc;
}

/* Table 91 - Definition of TPMT_TK_HASHCHECK Structure */

TPM_RC
TPMT_TK_HASHCHECK_Unmarshal(TPMT_TK_HASHCHECK *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPM_ST orig_tag = target->tag; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ST_Unmarshal(&target->tag, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->tag != TPM_ST_HASHCHECK) {
	    rc = TPM_RC_TAG;
	    target->tag = orig_tag; // libtpms added
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_RH_HIERARCHY_Unmarshal(&target->hierarchy, buffer, size, YES);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->digest, buffer, size);
    }
    return rc;
}

/* Table 92 - Definition of TPMS_ALG_PROPERTY Structure <OUT> */

#if 0		// libtpms added
TPM_RC
TPMS_ALG_PROPERTY_Unmarshal(TPMS_ALG_PROPERTY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(&target->alg, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMA_ALGORITHM_Unmarshal(&target->algProperties, buffer, size);
    }
    return rc;
}
#endif		// libtpms added

/* Table 93 - Definition of TPMS_TAGGED_PROPERTY Structure <OUT> */

#if 0		// libtpms added
TPM_RC
TPMS_TAGGED_PROPERTY_Unmarshal(TPMS_TAGGED_PROPERTY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_PT_Unmarshal(&target->property, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->value, buffer, size);
    }
    return rc;
}
#endif		// libtpms added
 
/* Table 94 - Definition of TPMS_TAGGED_PCR_SELECT Structure <OUT> */

#if 0		// libtpms added
TPM_RC
TPMS_TAGGED_PCR_SELECT_Unmarshal(TPMS_TAGGED_PCR_SELECT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_PT_PCR_Unmarshal(&target->tag, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT8_Unmarshal(&target->sizeofSelect, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = Array_Unmarshal(target->pcrSelect, target->sizeofSelect, buffer, size);
    }
     return rc;
}
#endif		// libtpms added

/* Table 95 - Definition of TPML_CC Structure */

TPM_RC
TPML_CC_Unmarshal(TPML_CC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > MAX_CAP_CC) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPM_CC_Unmarshal(&target->commandCodes[i], buffer, size);
    }
    return rc;
}

/* Table 2:96 - Definition of TPMS_TAGGED_POLICY Structure (StructuresTable()) */

#if 0		// libtpms added
TPM_RC
TPMS_TAGGED_POLICY_Unmarshal(TPMS_TAGGED_POLICY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_HANDLE_Unmarshal(&target->handle, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {	
	rc = TPMT_HA_Unmarshal(&target->policyHash, buffer, size, NO);
    }	
    return rc;
}
#endif		// libtpms added

/* Table 96 - Definition of TPML_CCA Structure <OUT> */

#if 0		// libtpms added
TPM_RC
TPML_CCA_Unmarshal(TPML_CCA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > MAX_CAP_CC) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPMA_CC_Unmarshal(&target->commandAttributes[i], buffer, size);
    }
    return rc;
}
#endif		// libtpms added
 
/* Table 97 - Definition of TPML_ALG Structure */

TPM_RC
TPML_ALG_Unmarshal(TPML_ALG *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > MAX_ALG_LIST_SIZE) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPM_ALG_ID_Unmarshal(&target->algorithms[i], buffer, size);
    }
    return rc;
}

/* Table 98 - Definition of TPML_HANDLE Structure <OUT> */

#if 0		// libtpms added
TPM_RC
TPML_HANDLE_Unmarshal(TPML_HANDLE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > MAX_CAP_HANDLES) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPM_HANDLE_Unmarshal(&target->handle[i], buffer, size);
    }
    return rc;
}
#endif		// libtpms added

/* Table 99 - Definition of TPML_DIGEST Structure */

/* PolicyOr has a restriction of at least a count of two.  This function is also used to unmarshal
   PCR_Read, where a count of one is permitted.
*/

TPM_RC
TPML_DIGEST_Unmarshal(TPML_DIGEST *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	/* TPM side is hard coded to 2 minimum */
	if (target->count < 2) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > 8) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPM2B_DIGEST_Unmarshal(&target->digests[i], buffer, size);
    }
    return rc;
}

/* Table 100 - Definition of TPML_DIGEST_VALUES Structure */

TPM_RC
TPML_DIGEST_VALUES_Unmarshal(TPML_DIGEST_VALUES *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > HASH_COUNT) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPMT_HA_Unmarshal(&target->digests[i], buffer, size, NO);
    }
    return rc;
}

/* Table 102 - Definition of TPML_PCR_SELECTION Structure */

TPM_RC
TPML_PCR_SELECTION_Unmarshal(TPML_PCR_SELECTION *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > HASH_COUNT) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPMS_PCR_SELECTION_Unmarshal(&target->pcrSelections[i], buffer, size);
    }
    return rc;
}


#if 0

/* Table 103 - Definition of TPML_ALG_PROPERTY Structure <OUT> */

TPM_RC
TPML_ALG_PROPERTY_Unmarshal(TPML_ALG_PROPERTY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    UINT32 i;
   if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > MAX_CAP_ALGS) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPMS_ALG_PROPERTY_Unmarshal(&target->algProperties[i], buffer, size);
    }
    return rc;
}

/* Table 104 - Definition of TPML_TAGGED_TPM_PROPERTY Structure <OUT> */

TPM_RC
TPML_TAGGED_TPM_PROPERTY_Unmarshal(TPML_TAGGED_TPM_PROPERTY  *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > MAX_TPM_PROPERTIES) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPMS_TAGGED_PROPERTY_Unmarshal(&target->tpmProperty[i], buffer, size);
    }
    return rc;
}

/* Table 105 - Definition of TPML_TAGGED_PCR_PROPERTY Structure <OUT> */

TPM_RC
TPML_TAGGED_PCR_PROPERTY_Unmarshal(TPML_TAGGED_PCR_PROPERTY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > MAX_PCR_PROPERTIES) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPMS_TAGGED_PCR_SELECT_Unmarshal(&target->pcrProperty[i], buffer, size);
    }
    return rc;
}

/* Table 106 - Definition of {ECC} TPML_ECC_CURVE Structure <OUT> */

TPM_RC
TPML_ECC_CURVE_Unmarshal(TPML_ECC_CURVE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > MAX_ECC_CURVES) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPM_ECC_CURVE_Unmarshal(&target->eccCurves[i], buffer, size);
    }
    return rc;
}

/* Table 2:109 - Definition of TPML_TAGGED_POLICY Structure (StructuresTable()) */

TPM_RC
TPML_TAGGED_POLICY_Unmarshal(TPML_TAGGED_POLICY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    UINT32 i;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->count, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->count > MAX_TAGGED_POLICIES) {
	    rc = TPM_RC_SIZE;
	    target->count = 0; // libtpms added
	}
    }
    for (i = 0 ; (rc == TPM_RC_SUCCESS) && (i < target->count) ; i++) {
	rc = TPMS_TAGGED_POLICY_Unmarshal(&target->policies[i], buffer, size);
    }
    return rc;
}

/* Table 2:110 - Definition of TPMU_CAPABILITIES Union (StructuresTable()) */

TPM_RC
TPMU_CAPABILITIES_Unmarshal(TPMU_CAPABILITIES *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
      case TPM_CAP_ALGS:
	rc = TPML_ALG_PROPERTY_Unmarshal(&target->algorithms, buffer, size);
	break;
      case TPM_CAP_HANDLES:
	rc = TPML_HANDLE_Unmarshal(&target->handles, buffer, size);
	break;
      case TPM_CAP_COMMANDS:
	rc = TPML_CCA_Unmarshal(&target->command, buffer, size);
	break;
      case TPM_CAP_PP_COMMANDS:
	rc = TPML_CC_Unmarshal(&target->ppCommands, buffer, size);
	break;
      case TPM_CAP_AUDIT_COMMANDS:
	rc = TPML_CC_Unmarshal(&target->auditCommands, buffer, size);
	break;
      case TPM_CAP_PCRS:
	rc = TPML_PCR_SELECTION_Unmarshal(&target->assignedPCR, buffer, size);
	break;
      case TPM_CAP_TPM_PROPERTIES:
	rc = TPML_TAGGED_TPM_PROPERTY_Unmarshal(&target->tpmProperties, buffer, size);
	break;
      case TPM_CAP_PCR_PROPERTIES:
	rc = TPML_TAGGED_PCR_PROPERTY_Unmarshal(&target->pcrProperties, buffer, size);
	break;
      case TPM_CAP_ECC_CURVES:
	rc = TPML_ECC_CURVE_Unmarshal(&target->eccCurves, buffer, size);
	break;
      case TPM_CAP_AUTH_POLICIES:	
	rc = TPML_TAGGED_POLICY_Unmarshal(&target->authPolicies, buffer, size);
	break;
      default:
	rc = TPM_RC_SELECTOR;
    }
    return rc;
}

/* Table 2:111 - Definition of TPMS_CAPABILITY_DATA Structure (StructuresTable()) */

TPM_RC
TPMS_CAPABILITY_DATA_Unmarshal(TPMS_CAPABILITY_DATA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
  
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_CAP_Unmarshal(&target->capability, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_CAPABILITIES_Unmarshal(&target->data, buffer, size, target->capability);
    }
    return rc;
}

/* Table 109 - Definition of TPMS_CLOCK_INFO Structure */

TPM_RC
TPMS_CLOCK_INFO_Unmarshal(TPMS_CLOCK_INFO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT64_Unmarshal(&target->clock, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->resetCount, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->restartCount, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_YES_NO_Unmarshal(&target->safe, buffer, size);
    }
    return rc;
}

/* Table 110 - Definition of TPMS_TIME_INFO Structure */

TPM_RC
TPMS_TIME_INFO_Unmarshal(TPMS_TIME_INFO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT64_Unmarshal(&target->time, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_CLOCK_INFO_Unmarshal(&target->clockInfo, buffer, size);
    }
    return rc;
}

/* Table 111 - Definition of TPMS_TIME_ATTEST_INFO Structure <OUT> */

TPM_RC
TPMS_TIME_ATTEST_INFO_Unmarshal(TPMS_TIME_ATTEST_INFO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_TIME_INFO_Unmarshal(&target->time, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT64_Unmarshal(&target->firmwareVersion, buffer, size);
    }
    return rc;
}

/* Table 112 - Definition of TPMS_CERTIFY_INFO Structure <OUT> */

TPM_RC
TPMS_CERTIFY_INFO_Unmarshal(TPMS_CERTIFY_INFO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_NAME_Unmarshal(&target->name, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_NAME_Unmarshal(&target->qualifiedName, buffer, size);
    }
    return rc;
}

/* Table 113 - Definition of TPMS_QUOTE_INFO Structure <OUT> */

TPM_RC
TPMS_QUOTE_INFO_Unmarshal(TPMS_QUOTE_INFO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPML_PCR_SELECTION_Unmarshal(&target->pcrSelect, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->pcrDigest, buffer, size);
    }
    return rc;
}

/* Table 114 - Definition of TPMS_COMMAND_AUDIT_INFO Structure <OUT> */

TPM_RC
TPMS_COMMAND_AUDIT_INFO_Unmarshal(TPMS_COMMAND_AUDIT_INFO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT64_Unmarshal(&target->auditCounter, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(&target->digestAlg, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->auditDigest, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->commandDigest, buffer, size);
    }
    return rc;
}

/* Table 115 - Definition of TPMS_SESSION_AUDIT_INFO Structure <OUT> */

TPM_RC
TPMS_SESSION_AUDIT_INFO_Unmarshal(TPMS_SESSION_AUDIT_INFO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_YES_NO_Unmarshal(&target->exclusiveSession, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->sessionDigest, buffer, size);
    }
    return rc;
}

/* Table 116 - Definition of TPMS_CREATION_INFO Structure <OUT> */

TPM_RC
TPMS_CREATION_INFO_Unmarshal(TPMS_CREATION_INFO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_NAME_Unmarshal(&target->objectName, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->creationHash, buffer, size);
    }
    return rc;
}

/* Table 117 - Definition of TPMS_NV_CERTIFY_INFO Structure <OUT> */

TPM_RC
TPMS_NV_CERTIFY_INFO_Unmarshal(TPMS_NV_CERTIFY_INFO *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_NAME_Unmarshal(&target->indexName, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->offset, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_MAX_NV_BUFFER_Unmarshal(&target->nvContents, buffer, size);
    }
    return rc;
}

/* Table 118 - Definition of (TPM_ST) TPMI_ST_ATTEST Type <OUT> */

TPM_RC
TPMI_ST_ATTEST_Unmarshal(TPMI_ST_ATTEST *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ST_ATTEST orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ST_Unmarshal(target, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case TPM_ST_ATTEST_CERTIFY:
	  case TPM_ST_ATTEST_CREATION:
	  case TPM_ST_ATTEST_QUOTE:
	  case TPM_ST_ATTEST_COMMAND_AUDIT:
	  case TPM_ST_ATTEST_SESSION_AUDIT:
	  case TPM_ST_ATTEST_TIME:
	  case TPM_ST_ATTEST_NV:
	    break;
	  default:
	    rc = TPM_RC_SELECTOR;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/*  Table 119 - Definition of TPMU_ATTEST Union <OUT> */

TPM_RC
TPMU_ATTEST_Unmarshal(TPMU_ATTEST *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
      case TPM_ST_ATTEST_CERTIFY:
	rc = TPMS_CERTIFY_INFO_Unmarshal(&target->certify, buffer, size);
	break;
      case TPM_ST_ATTEST_CREATION:
	rc = TPMS_CREATION_INFO_Unmarshal(&target->creation, buffer, size);
	break;
      case TPM_ST_ATTEST_QUOTE:
	rc = TPMS_QUOTE_INFO_Unmarshal(&target->quote, buffer, size);
	break;
      case TPM_ST_ATTEST_COMMAND_AUDIT:
	rc = TPMS_COMMAND_AUDIT_INFO_Unmarshal(&target->commandAudit, buffer, size);
	break;
      case TPM_ST_ATTEST_SESSION_AUDIT:
	rc = TPMS_SESSION_AUDIT_INFO_Unmarshal(&target->sessionAudit, buffer, size);
	break;
      case TPM_ST_ATTEST_TIME:
	rc = TPMS_TIME_ATTEST_INFO_Unmarshal(&target->time, buffer, size);
	break;
      case TPM_ST_ATTEST_NV:
	rc = TPMS_NV_CERTIFY_INFO_Unmarshal(&target->nv, buffer, size);
	break;
      default:
	rc = TPM_RC_SELECTOR;
	
    }
    return rc;
}

/* Table 120 - Definition of TPMS_ATTEST Structure <OUT> */

TPM_RC
TPMS_ATTEST_Unmarshal(TPMS_ATTEST *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_GENERATED_Unmarshal(&target->magic, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ST_ATTEST_Unmarshal(&target->type, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_NAME_Unmarshal(&target->qualifiedSigner, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DATA_Unmarshal(&target->extraData, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_CLOCK_INFO_Unmarshal(&target->clockInfo, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT64_Unmarshal(&target->firmwareVersion, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_ATTEST_Unmarshal(&target->attested, buffer, size, target->type);
    }
    return rc;
}

/* Table 121 - Definition of TPM2B_ATTEST Structure <OUT> */

TPM_RC
TPM2B_ATTEST_Unmarshal(TPM2B_ATTEST *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, sizeof(TPMS_ATTEST), buffer, size);
    }
    return rc;
}

#endif

/* Table 124 - Definition of {!ALG.S} (TPM_KEY_BITS) TPMI_!ALG.S_KEY_BITS Type */

#if ALG_AES
TPM_RC
TPMI_AES_KEY_BITS_Unmarshal(TPMI_AES_KEY_BITS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_AES_KEY_BITS orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_KEY_BITS_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if AES_128	// libtpms added
	  case 128:
#endif		// libtpms added begin
#if AES_192
	  case 192:
#endif
#if AES_256	// libtpms added end
	  case 256:
#endif		// libtpms added
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}
#endif
 
#if ALG_CAMELLIA
TPM_RC
TPMI_CAMELLIA_KEY_BITS_Unmarshal(TPMI_CAMELLIA_KEY_BITS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_CAMELLIA_KEY_BITS orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_KEY_BITS_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if CAMELLIA_128	// libtpms added
	  case 128:
#endif			// libtpms added begin
#if CAMELLIA_192
	  case 192:
#endif
#if CAMELLIA_256
	  case 256:
#endif			// libtpms added end
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}
#endif

#if ALG_SM4
TPM_RC
TPMI_SM4_KEY_BITS_Unmarshal(TPMI_SM4_KEY_BITS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_SM4_KEY_BITS orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_KEY_BITS_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case 128:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}
#endif

#if ALG_TDES		// libtpms added begin
TPM_RC
TPMI_TDES_KEY_BITS_Unmarshal(TPMI_SM4_KEY_BITS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_SM4_KEY_BITS orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_KEY_BITS_Unmarshal(target, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case 128:
	  case 192:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}
#endif			// libtpms added end

/* Table 125 - Definition of TPMU_SYM_KEY_BITS Union */

TPM_RC
TPMU_SYM_KEY_BITS_Unmarshal(TPMU_SYM_KEY_BITS *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
#if ALG_AES
      case TPM_ALG_AES:
	rc = TPMI_AES_KEY_BITS_Unmarshal(&target->aes, buffer, size);
	break;
#endif
#if ALG_SM4
      case TPM_ALG_SM4:
	rc = TPMI_SM4_KEY_BITS_Unmarshal(&target->sm4, buffer, size);
	break;
#endif
#if ALG_CAMELLIA
      case TPM_ALG_CAMELLIA:
	rc = TPMI_CAMELLIA_KEY_BITS_Unmarshal(&target->camellia, buffer, size);
	break;
#endif
#if ALG_TDES		// libtpms added beging
      case TPM_ALG_TDES:
	rc = TPMI_TDES_KEY_BITS_Unmarshal(&target->tdes, buffer, size);
	break;
#endif			// libtpms added end
#if ALG_XOR
      case TPM_ALG_XOR:
	rc = TPMI_ALG_HASH_Unmarshal(&target->xorr, buffer, size, NO);
	break;
#endif
      case TPM_ALG_NULL:
	break;
      default:
	rc = TPM_RC_SELECTOR;
    }
    return rc;
}

/* Table 126 - Definition of TPMU_SYM_MODE Union */

TPM_RC
TPMU_SYM_MODE_Unmarshal(TPMU_SYM_MODE *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
#if ALG_AES
      case TPM_ALG_AES:
	rc = TPMI_ALG_SYM_MODE_Unmarshal(&target->aes, buffer, size, YES);
	break;
#endif
#if ALG_SM4
      case TPM_ALG_SM4:
	rc = TPMI_ALG_SYM_MODE_Unmarshal(&target->sm4, buffer, size, YES);
	break;
#endif
#if ALG_CAMELLIA
      case TPM_ALG_CAMELLIA:
	rc = TPMI_ALG_SYM_MODE_Unmarshal(&target->camellia, buffer, size, YES);
	break;
#endif
#if ALG_TDES		// libtpms added begin
      case TPM_ALG_TDES:
	rc = TPMI_ALG_SYM_MODE_Unmarshal(&target->tdes, buffer, size, YES);
	break;
#endif			// libtpms added end
      case TPM_ALG_XOR:
      case TPM_ALG_NULL:
	break;
      default:
	rc = TPM_RC_SELECTOR;
    }
    return rc;
}

/* Table 131 - Definition of TPMT_SYM_DEF Structure */

TPM_RC
TPMT_SYM_DEF_Unmarshal(TPMT_SYM_DEF *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_SYM_Unmarshal(&target->algorithm, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_SYM_KEY_BITS_Unmarshal(&target->keyBits, buffer, size, target->algorithm);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_SYM_MODE_Unmarshal(&target->mode, buffer, size, target->algorithm);
    }
    return rc;
}

/* Table 132 - Definition of TPMT_SYM_DEF_OBJECT Structure */

TPM_RC
TPMT_SYM_DEF_OBJECT_Unmarshal(TPMT_SYM_DEF_OBJECT *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_SYM_OBJECT_Unmarshal(&target->algorithm, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_SYM_KEY_BITS_Unmarshal(&target->keyBits, buffer, size, target->algorithm);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_SYM_MODE_Unmarshal(&target->mode, buffer, size, target->algorithm);
    }
    return rc;
}

/* Table 133 - Definition of TPM2B_SYM_KEY Structure */

TPM_RC
TPM2B_SYM_KEY_Unmarshal(TPM2B_SYM_KEY *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, MAX_SYM_KEY_BYTES, buffer, size);
    }
    return rc;
}

/* Table 134 - Definition of TPMS_SYMCIPHER_PARMS Structure */

TPM_RC
TPMS_SYMCIPHER_PARMS_Unmarshal(TPMS_SYMCIPHER_PARMS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMT_SYM_DEF_OBJECT_Unmarshal(&target->sym, buffer, size, NO);
    }
    return rc;
}

/* Table 2:135 - Definition of TPM2B_LABEL Structure (StructuresTable()) */

TPM_RC
TPM2B_LABEL_Unmarshal(TPM2B_LABEL *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, LABEL_MAX_BUFFER, buffer, size);
    }
    return rc;
}

/* Table 2:136 - Definition of TPMS_DERIVE Structure (StructuresTable()) */

TPM_RC
TPMS_DERIVE_Unmarshal(TPMS_DERIVE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_LABEL_Unmarshal(&target->label, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_LABEL_Unmarshal(&target->context, buffer, size);
    }
    return rc;
}

/* Table 139 - Definition of TPM2B_SENSITIVE_DATA Structure */

TPM_RC
TPM2B_SENSITIVE_DATA_Unmarshal(TPM2B_SENSITIVE_DATA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, MAX_SYM_DATA, buffer, size);
    }
    return rc;
}

/* Table 133 - Definition of TPMS_SENSITIVE_CREATE Structure <IN> */

TPM_RC
TPMS_SENSITIVE_CREATE_Unmarshal(TPMS_SENSITIVE_CREATE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_AUTH_Unmarshal(&target->userAuth, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_SENSITIVE_DATA_Unmarshal(&target->data, buffer, size);
    }
    return rc;
}

/* Table 134 - Definition of TPM2B_SENSITIVE_CREATE Structure <IN, S> */

TPM_RC
TPM2B_SENSITIVE_CREATE_Unmarshal(TPM2B_SENSITIVE_CREATE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    INT32 startSize;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size == 0) {
	    rc = TPM_RC_SIZE;
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	startSize = *size;
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SENSITIVE_CREATE_Unmarshal(&target->sensitive, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size != startSize - *size) {
	    rc = TPM_RC_SIZE;
	    target->size = 0; // libtpms added
	}
    }
    return rc;
}

/* Table 135 - Definition of TPMS_SCHEME_HASH Structure */

TPM_RC
TPMS_SCHEME_HASH_Unmarshal(TPMS_SCHEME_HASH *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_HASH_Unmarshal(&target->hashAlg, buffer, size, NO);
    }
    return rc;
}

/* Table 136 - Definition of {ECC} TPMS_SCHEME_ECDAA Structure */

TPM_RC
TPMS_SCHEME_ECDAA_Unmarshal(TPMS_SCHEME_ECDAA *target, BYTE **buffer, INT32 *size) 
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_HASH_Unmarshal(&target->hashAlg, buffer, size, NO);	
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->count, buffer, size);	
    }
    return rc;
}

/* Table 137 - Definition of (TPM_ALG_ID) TPMI_ALG_KEYEDHASH_SCHEME Type */

TPM_RC
TPMI_ALG_KEYEDHASH_SCHEME_Unmarshal(TPMI_ALG_KEYEDHASH_SCHEME *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_KEYEDHASH_SCHEME orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_HMAC
	  case TPM_ALG_HMAC:	
#endif
#if ALG_XOR
	  case TPM_ALG_XOR:	
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 138 - Definition of Types for HMAC_SIG_SCHEME */

TPM_RC
TPMS_SCHEME_HMAC_Unmarshal(TPMS_SCHEME_HMAC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 139 - Definition of TPMS_SCHEME_XOR Structure */

TPM_RC
TPMS_SCHEME_XOR_Unmarshal(TPMS_SCHEME_XOR *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_HASH_Unmarshal(&target->hashAlg, buffer, size, NO);	/* as of rev 147 */
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_KDF_Unmarshal(&target->kdf, buffer, size, YES);
    }
    return rc;
}
    
/* Table 140 - Definition of TPMU_SCHEME_KEYEDHASH Union <IN/OUT, S> */

TPM_RC
TPMU_SCHEME_KEYEDHASH_Unmarshal(TPMU_SCHEME_KEYEDHASH *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
#if ALG_HMAC
      case TPM_ALG_HMAC:
	rc = TPMS_SCHEME_HMAC_Unmarshal(&target->hmac, buffer, size);
	break;
#endif
#if ALG_XOR
      case TPM_ALG_XOR:
	rc = TPMS_SCHEME_XOR_Unmarshal(&target->xorr, buffer, size);
	break;
#endif
      case TPM_ALG_NULL:
	break;
      default:
	rc = TPM_RC_SELECTOR;
    }
    return rc;
}

/* Table 141 - Definition of TPMT_KEYEDHASH_SCHEME Structure */

TPM_RC
TPMT_KEYEDHASH_SCHEME_Unmarshal(TPMT_KEYEDHASH_SCHEME *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_KEYEDHASH_SCHEME_Unmarshal(&target->scheme, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_SCHEME_KEYEDHASH_Unmarshal(&target->details, buffer, size, target->scheme);
    }
    return rc;
}

/* Table 142 - Definition of {RSA} Types for RSA Signature Schemes */

TPM_RC
TPMS_SIG_SCHEME_RSAPSS_Unmarshal(TPMS_SIG_SCHEME_RSAPSS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 142 - Definition of {RSA} Types for RSA Signature Schemes */

TPM_RC
TPMS_SIG_SCHEME_RSASSA_Unmarshal(TPMS_SIG_SCHEME_RSASSA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 143 - Definition of {ECC} Types for ECC Signature Schemes */

TPM_RC
TPMS_SIG_SCHEME_ECDAA_Unmarshal(TPMS_SIG_SCHEME_ECDAA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_ECDAA_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 143 - Definition of {ECC} Types for ECC Signature Schemes */

TPM_RC
TPMS_SIG_SCHEME_ECDSA_Unmarshal(TPMS_SIG_SCHEME_ECDSA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 143 - Definition of {ECC} Types for ECC Signature Schemes */

TPM_RC
TPMS_SIG_SCHEME_ECSCHNORR_Unmarshal(TPMS_SIG_SCHEME_ECSCHNORR *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 143 - Definition of {ECC} Types for ECC Signature Schemes */

TPM_RC
TPMS_SIG_SCHEME_SM2_Unmarshal(TPMS_SIG_SCHEME_SM2 *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 144 - Definition of TPMU_SIG_SCHEME Union <IN/OUT, S> */

TPM_RC
TPMU_SIG_SCHEME_Unmarshal(TPMU_SIG_SCHEME *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
#if ALG_RSASSA
      case TPM_ALG_RSASSA:
	rc = TPMS_SIG_SCHEME_RSASSA_Unmarshal(&target->rsassa, buffer, size);
	break;
#endif
#if ALG_RSAPSS
      case TPM_ALG_RSAPSS:
	rc = TPMS_SIG_SCHEME_RSAPSS_Unmarshal(&target->rsapss, buffer, size);
	break;
#endif
#if ALG_ECDSA
      case TPM_ALG_ECDSA:
	rc = TPMS_SIG_SCHEME_ECDSA_Unmarshal(&target->ecdsa, buffer, size);
	break;
#endif
#if ALG_ECDAA
      case TPM_ALG_ECDAA:
	rc = TPMS_SIG_SCHEME_ECDAA_Unmarshal(&target->ecdaa, buffer, size);
	break;
#endif
#if ALG_SM2
      case TPM_ALG_SM2:
	rc = TPMS_SIG_SCHEME_SM2_Unmarshal(&target->sm2, buffer, size);
	break;
#endif
#if ALG_ECSCHNORR
      case TPM_ALG_ECSCHNORR:
	rc = TPMS_SIG_SCHEME_ECSCHNORR_Unmarshal(&target->ecschnorr, buffer, size);
	break;
#endif
#if ALG_HMAC
      case TPM_ALG_HMAC:
	rc = TPMS_SCHEME_HMAC_Unmarshal(&target->hmac, buffer, size);
	break;
#endif
      case TPM_ALG_NULL:
	break;
      default:
	rc = TPM_RC_SELECTOR;
    }
    return rc;
}

/* Table 145 - Definition of TPMT_SIG_SCHEME Structure */

TPM_RC
TPMT_SIG_SCHEME_Unmarshal(TPMT_SIG_SCHEME *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_SIG_SCHEME_Unmarshal(&target->scheme, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_SIG_SCHEME_Unmarshal(&target->details, buffer, size, target->scheme);
    }
    return rc;
}

/* Table 146 - Definition of Types for {RSA} Encryption Schemes */

TPM_RC
TPMS_ENC_SCHEME_OAEP_Unmarshal(TPMS_ENC_SCHEME_OAEP *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 146 - Definition of Types for {RSA} Encryption Schemes */

TPM_RC
TPMS_ENC_SCHEME_RSAES_Unmarshal(TPMS_ENC_SCHEME_RSAES *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_EMPTY_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 147 - Definition of Types for {ECC} ECC Key Exchange */

TPM_RC
TPMS_KEY_SCHEME_ECDH_Unmarshal(TPMS_KEY_SCHEME_ECDH *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size); 
    }
    return rc;
}

/* Table 147 - Definition of Types for {ECC} ECC Key Exchange */

TPM_RC
TPMS_KEY_SCHEME_ECMQV_Unmarshal(TPMS_KEY_SCHEME_ECMQV *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size); 
    }
    return rc;
}

/* Table 148 - Definition of Types for KDF Schemes, hash-based key- or mask-generation functions */

TPM_RC
TPMS_KDF_SCHEME_KDF1_SP800_108_Unmarshal(TPMS_KDF_SCHEME_KDF1_SP800_108 *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size); 
    }
    return rc;
}

/* Table 148 - Definition of Types for KDF Schemes, hash-based key- or mask-generation functions */

TPM_RC
TPMS_KDF_SCHEME_KDF1_SP800_56A_Unmarshal(TPMS_KDF_SCHEME_KDF1_SP800_56A *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size); 
    }
    return rc;
}

/* Table 148 - Definition of Types for KDF Schemes, hash-based key- or mask-generation functions */

TPM_RC
TPMS_KDF_SCHEME_KDF2_Unmarshal(TPMS_KDF_SCHEME_KDF2 *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 148 - Definition of Types for KDF Schemes, hash-based key- or mask-generation functions */

TPM_RC
TPMS_KDF_SCHEME_MGF1_Unmarshal(TPMS_KDF_SCHEME_MGF1 *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 149 - Definition of TPMU_KDF_SCHEME Union <IN/OUT, S> */

TPM_RC
TPMU_KDF_SCHEME_Unmarshal(TPMU_KDF_SCHEME *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
#if ALG_MGF1
      case TPM_ALG_MGF1:
	rc = TPMS_KDF_SCHEME_MGF1_Unmarshal(&target->mgf1, buffer, size);
	break;
#endif
#if ALG_KDF1_SP800_56A
      case TPM_ALG_KDF1_SP800_56A:
	rc = TPMS_KDF_SCHEME_KDF1_SP800_56A_Unmarshal(&target->kdf1_sp800_56a, buffer, size);
	break;
#endif
#if ALG_KDF2
      case TPM_ALG_KDF2:
	rc = TPMS_KDF_SCHEME_KDF2_Unmarshal(&target->kdf2, buffer, size);
	break;
#endif
#if ALG_KDF1_SP800_108
      case TPM_ALG_KDF1_SP800_108:
	rc = TPMS_KDF_SCHEME_KDF1_SP800_108_Unmarshal(&target->kdf1_sp800_108, buffer, size);
	break;
#endif
      case TPM_ALG_NULL:
	break;
      default:
	rc = TPM_RC_SELECTOR;
    }
    return rc;
}

/* Table 150 - Definition of TPMT_KDF_SCHEME Structure */

TPM_RC
TPMT_KDF_SCHEME_Unmarshal(TPMT_KDF_SCHEME *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_KDF_Unmarshal(&target->scheme, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_KDF_SCHEME_Unmarshal(&target->details, buffer, size, target->scheme);
    }
    return rc;
}

/* Table 151 - Definition of (TPM_ALG_ID) TPMI_ALG_ASYM_SCHEME Type <> */

#if 0
TPM_RC
TPMI_ALG_ASYM_SCHEME_Unmarshal(TPMI_ALG_ASYM_SCHEME *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_ASYM_SCHEME orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_ECDH
	  case TPM_ALG_ECDH:
#endif
#if ALG_ECMQV
	  case TPM_ALG_ECMQV:
#endif
#if ALG_RSASSA
	  case TPM_ALG_RSASSA:
#endif
#if ALG_RSAPSS
	  case TPM_ALG_RSAPSS:
#endif
#if ALG_ECDSA
	  case TPM_ALG_ECDSA:
#endif
#if ALG_ECDAA
	  case TPM_ALG_ECDAA:
#endif
#if ALG_SM2
	  case TPM_ALG_SM2:
#endif
#if ALG_ECSCHNORR
	  case TPM_ALG_ECSCHNORR:
#endif
#if ALG_RSAES
	  case TPM_ALG_RSAES:
#endif
#if ALG_OAEP
	  case TPM_ALG_OAEP:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}
#endif

/* Table 152 - Definition of TPMU_ASYM_SCHEME Union */

TPM_RC
TPMU_ASYM_SCHEME_Unmarshal(TPMU_ASYM_SCHEME *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
#if ALG_ECDH
      case TPM_ALG_ECDH:
	rc = TPMS_KEY_SCHEME_ECDH_Unmarshal(&target->ecdh, buffer, size);
	break;
#endif
#if ALG_ECMQV
      case TPM_ALG_ECMQV:
	rc = TPMS_KEY_SCHEME_ECMQV_Unmarshal(&target->ecmqv, buffer, size);
	break;
#endif
#if ALG_RSASSA
      case TPM_ALG_RSASSA:
	rc = TPMS_SIG_SCHEME_RSASSA_Unmarshal(&target->rsassa, buffer, size);
	break;
#endif
#if ALG_RSAPSS
      case TPM_ALG_RSAPSS:
	rc = TPMS_SIG_SCHEME_RSAPSS_Unmarshal(&target->rsapss, buffer, size);
	break;
#endif
#if ALG_ECDSA
      case TPM_ALG_ECDSA:
	rc = TPMS_SIG_SCHEME_ECDSA_Unmarshal(&target->ecdsa, buffer, size);
	break;
#endif
#if ALG_ECDAA
      case TPM_ALG_ECDAA:
	rc = TPMS_SIG_SCHEME_ECDAA_Unmarshal(&target->ecdaa, buffer, size);
	break;
#endif
#if ALG_SM2
      case TPM_ALG_SM2:
	rc = TPMS_SIG_SCHEME_SM2_Unmarshal(&target->sm2, buffer, size);
	break;
#endif
#if ALG_ECSCHNORR
      case TPM_ALG_ECSCHNORR:
	rc = TPMS_SIG_SCHEME_ECSCHNORR_Unmarshal(&target->ecschnorr, buffer, size);
	break;
#endif
#if ALG_RSAES
      case TPM_ALG_RSAES:
	rc = TPMS_ENC_SCHEME_RSAES_Unmarshal(&target->rsaes, buffer, size);
	break;
#endif
#if ALG_OAEP
      case TPM_ALG_OAEP:
	rc = TPMS_ENC_SCHEME_OAEP_Unmarshal(&target->oaep, buffer, size);
	break;
#endif
      case TPM_ALG_NULL:
	break;
      default:
	rc = TPM_RC_SELECTOR;
    }
    return rc;
}

/* Table 154 - Definition of (TPM_ALG_ID) {RSA} TPMI_ALG_RSA_SCHEME Type */

TPM_RC
TPMI_ALG_RSA_SCHEME_Unmarshal(TPMI_ALG_RSA_SCHEME *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_RSA_SCHEME orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_RSASSA
	  case TPM_ALG_RSASSA:
#endif
#if ALG_RSAPSS
	  case TPM_ALG_RSAPSS:
#endif
#if ALG_RSAES
	  case TPM_ALG_RSAES:
#endif
#if ALG_OAEP
	  case TPM_ALG_OAEP:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 155 - Definition of {RSA} TPMT_RSA_SCHEME Structure */

TPM_RC
TPMT_RSA_SCHEME_Unmarshal(TPMT_RSA_SCHEME *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_RSA_SCHEME_Unmarshal(&target->scheme, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_ASYM_SCHEME_Unmarshal(&target->details, buffer, size, target->scheme);
    }
    return rc;
}

/* Table 156 - Definition of (TPM_ALG_ID) {RSA} TPMI_ALG_RSA_DECRYPT Type */

TPM_RC
TPMI_ALG_RSA_DECRYPT_Unmarshal(TPMI_ALG_RSA_DECRYPT *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_RSA_DECRYPT orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_RSAES
	  case TPM_ALG_RSAES:
#endif
#if ALG_OAEP
	  case TPM_ALG_OAEP:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 157 - Definition of {RSA} TPMT_RSA_DECRYPT Structure */

TPM_RC
TPMT_RSA_DECRYPT_Unmarshal(TPMT_RSA_DECRYPT *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_RSA_DECRYPT_Unmarshal(&target->scheme, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_ASYM_SCHEME_Unmarshal(&target->details, buffer, size, target->scheme);
    }
    return rc;
}

/* Table 158 - Definition of {RSA} TPM2B_PUBLIC_KEY_RSA Structure */

TPM_RC
TPM2B_PUBLIC_KEY_RSA_Unmarshal(TPM2B_PUBLIC_KEY_RSA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, MAX_RSA_KEY_BYTES, buffer, size);
    }
    return rc;
}
    
/* Table 159 - Definition of {RSA} (TPM_KEY_BITS) TPMI_RSA_KEY_BITS Type */

TPM_RC
TPMI_RSA_KEY_BITS_Unmarshal(TPMI_RSA_KEY_BITS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_RSA_KEY_BITS orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_KEY_BITS_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
	  case 1024:
	  case 2048:
	  case 3072:
	    break;
	  default:
	    rc = TPM_RC_VALUE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 160 - Definition of {RSA} TPM2B_PRIVATE_KEY_RSA Structure */

TPM_RC
TPM2B_PRIVATE_KEY_RSA_Unmarshal(TPM2B_PRIVATE_KEY_RSA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, MAX_RSA_KEY_BYTES/2, buffer, size);
    }
    return rc;
}
 
/* Table 161 - Definition of {ECC} TPM2B_ECC_PARAMETER Structure */

TPM_RC
TPM2B_ECC_PARAMETER_Unmarshal(TPM2B_ECC_PARAMETER *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
     	rc = TPM2B_Unmarshal(&target->b, MAX_ECC_KEY_BYTES, buffer, size);
    }
    return rc;
}
    
/* Table 162 - Definition of {ECC} TPMS_ECC_POINT Structure */

TPM_RC
TPMS_ECC_POINT_Unmarshal(TPMS_ECC_POINT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_ECC_PARAMETER_Unmarshal(&target->x, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_ECC_PARAMETER_Unmarshal(&target->y, buffer, size);
    }
    return rc;
}

/* Table 163 - Definition of {ECC} TPM2B_ECC_POINT Structure */

TPM_RC
TPM2B_ECC_POINT_Unmarshal(TPM2B_ECC_POINT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    INT32 startSize;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size == 0) {
	    rc = TPM_RC_SIZE;
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	startSize = *size;
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_ECC_POINT_Unmarshal(&target->point, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size != startSize - *size) {
	    rc = TPM_RC_SIZE;
	    target->size = 0; // libtpms added
	}
    }
    return rc;
}

/* Table 164 - Definition of (TPM_ALG_ID) {ECC} TPMI_ALG_ECC_SCHEME Type */

TPM_RC
TPMI_ALG_ECC_SCHEME_Unmarshal(TPMI_ALG_ECC_SCHEME *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_ECC_SCHEME orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_ECDSA
	  case TPM_ALG_ECDSA:
#endif
#if ALG_SM2
	  case TPM_ALG_SM2:
#endif
#if ALG_ECDAA
	  case TPM_ALG_ECDAA:
#endif
#if ALG_ECSCHNORR
	  case TPM_ALG_ECSCHNORR:
#endif
#if ALG_ECDH
	  case TPM_ALG_ECDH:
#endif
#if ALG_ECMQV
	  case TPM_ALG_ECMQV:
#endif
	    break;
	  case TPM_ALG_NULL:
	    if (allowNull) {
		break;
	    }
	  default:
	    rc = TPM_RC_SCHEME;
	    *target = orig_target; //  libtpms added
	}
    }
    return rc;
}

/* Table 165 - Definition of {ECC} (TPM_ECC_CURVE) TPMI_ECC_CURVE Type */

TPM_RC
TPMI_ECC_CURVE_Unmarshal(TPMI_ECC_CURVE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ECC_CURVE orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ECC_CURVE_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ECC_BN_P256
	  case TPM_ECC_BN_P256:
#endif
#if ECC_BN_P638		// libtpms added begin
	  case TPM_ECC_BN_P638:
#endif
#if ECC_NIST_P192
	  case TPM_ECC_NIST_P192:
#endif
#if ECC_NIST_P224
	  case TPM_ECC_NIST_P224:
#endif			// libtpms added end
#if ECC_NIST_P256
	  case TPM_ECC_NIST_P256:
#endif
#if ECC_NIST_P384
	  case TPM_ECC_NIST_P384:
#endif
#if ECC_NIST_P521	// libtpms added begin
	  case TPM_ECC_NIST_P521:
#endif
#if ECC_SM2_P256
	  case TPM_ECC_SM2_P256:
#endif
          if (!CryptEccIsCurveRuntimeUsable(*target))
              rc = TPM_RC_CURVE;
                      // libtpms added end
	    break;
	  default:
	    rc = TPM_RC_CURVE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 166 - Definition of (TPMT_SIG_SCHEME) {ECC} TPMT_ECC_SCHEME Structure */

TPM_RC
TPMT_ECC_SCHEME_Unmarshal(TPMT_ECC_SCHEME *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_ECC_SCHEME_Unmarshal(&target->scheme, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_ASYM_SCHEME_Unmarshal(&target->details, buffer, size, target->scheme);
    }
    return rc;
}

/* Table 168 - Definition of {RSA} TPMS_SIGNATURE_RSA Structure */

TPM_RC
TPMS_SIGNATURE_RSA_Unmarshal(TPMS_SIGNATURE_RSA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_HASH_Unmarshal(&target->hash, buffer, size, NO);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_PUBLIC_KEY_RSA_Unmarshal(&target->sig, buffer, size);
    }
    return rc;
}

/* Table 169 - Definition of Types for {RSA} Signature */

TPM_RC
TPMS_SIGNATURE_RSASSA_Unmarshal(TPMS_SIGNATURE_RSASSA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SIGNATURE_RSA_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 169 - Definition of Types for {RSA} Signature */
    
TPM_RC
TPMS_SIGNATURE_RSAPSS_Unmarshal(TPMS_SIGNATURE_RSAPSS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SIGNATURE_RSA_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 170 - Definition of {ECC} TPMS_SIGNATURE_ECC Structure */

TPM_RC
TPMS_SIGNATURE_ECC_Unmarshal(TPMS_SIGNATURE_ECC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_HASH_Unmarshal(&target->hash, buffer, size, NO);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_ECC_PARAMETER_Unmarshal(&target->signatureR, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_ECC_PARAMETER_Unmarshal(&target->signatureS, buffer, size);
    }
    return rc;
}

/* Table 171 - Definition of Types for {ECC} TPMS_SIGNATURE_ECC */

TPM_RC
TPMS_SIGNATURE_ECDSA_Unmarshal(TPMS_SIGNATURE_ECDSA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
     
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SIGNATURE_ECC_Unmarshal(target, buffer, size);
    }
    return rc;
}
    
TPM_RC
TPMS_SIGNATURE_ECDAA_Unmarshal(TPMS_SIGNATURE_ECDAA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
     
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SIGNATURE_ECC_Unmarshal(target, buffer, size);
    }
    return rc;
}

TPM_RC
TPMS_SIGNATURE_SM2_Unmarshal(TPMS_SIGNATURE_SM2 *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
     
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SIGNATURE_ECC_Unmarshal(target, buffer, size);
    }
    return rc;
}

TPM_RC
TPMS_SIGNATURE_ECSCHNORR_Unmarshal(TPMS_SIGNATURE_ECSCHNORR *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
     
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_SIGNATURE_ECC_Unmarshal(target, buffer, size);
    }
    return rc;
}

/* Table 172 - Definition of TPMU_SIGNATURE Union <IN/OUT, S> */

TPM_RC
TPMU_SIGNATURE_Unmarshal(TPMU_SIGNATURE *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
#if ALG_RSASSA
      case TPM_ALG_RSASSA:
	rc = TPMS_SIGNATURE_RSASSA_Unmarshal(&target->rsassa, buffer, size);
	break;
#endif
#if ALG_RSAPSS
      case TPM_ALG_RSAPSS:
	rc = TPMS_SIGNATURE_RSAPSS_Unmarshal(&target->rsapss, buffer, size);
	break;
#endif
#if ALG_ECDSA
      case TPM_ALG_ECDSA:
	rc = TPMS_SIGNATURE_ECDSA_Unmarshal(&target->ecdsa, buffer, size);
	break;
#endif
#if ALG_ECDAA
      case TPM_ALG_ECDAA:
	rc = TPMS_SIGNATURE_ECDAA_Unmarshal(&target->ecdaa, buffer, size);
	break;
#endif
#if ALG_SM2
      case TPM_ALG_SM2:
	rc = TPMS_SIGNATURE_SM2_Unmarshal(&target->sm2, buffer, size);
	break;
#endif
#if ALG_ECSCHNORR
      case TPM_ALG_ECSCHNORR:
	rc = TPMS_SIGNATURE_ECSCHNORR_Unmarshal(&target->ecschnorr, buffer, size);
	break;
#endif
#if ALG_HMAC
      case TPM_ALG_HMAC:
	rc = TPMT_HA_Unmarshal(&target->hmac, buffer, size, NO);
	break;
#endif
      case TPM_ALG_NULL:
	break;
      default:
	rc = TPM_RC_SELECTOR;
    }
    return rc;
}

/* Table 173 - Definition of TPMT_SIGNATURE Structure */

TPM_RC
TPMT_SIGNATURE_Unmarshal(TPMT_SIGNATURE *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_SIG_SCHEME_Unmarshal(&target->sigAlg, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_SIGNATURE_Unmarshal(&target->signature, buffer, size, target->sigAlg);
    }
    return rc;
}

/* Table 175 - Definition of TPM2B_ENCRYPTED_SECRET Structure */

TPM_RC
TPM2B_ENCRYPTED_SECRET_Unmarshal(TPM2B_ENCRYPTED_SECRET *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, sizeof(TPMU_ENCRYPTED_SECRET), buffer, size);
    }
    return rc;
}

/* Table 176 - Definition of (TPM_ALG_ID) TPMI_ALG_PUBLIC Type */

TPM_RC
TPMI_ALG_PUBLIC_Unmarshal(TPMI_ALG_PUBLIC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMI_ALG_PUBLIC orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM_ALG_ID_Unmarshal(target, buffer, size);  
    }
    if (rc == TPM_RC_SUCCESS) {
	switch (*target) {
#if ALG_KEYEDHASH
	  case TPM_ALG_KEYEDHASH:
#endif
#if ALG_RSA
	  case TPM_ALG_RSA:
#endif
#if ALG_ECC
	  case TPM_ALG_ECC:
#endif
#if ALG_SYMCIPHER
	  case TPM_ALG_SYMCIPHER:
#endif
	    break;
	  default:
	    rc = TPM_RC_TYPE;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}
    
/* Table 177 - Definition of TPMU_PUBLIC_ID Union <IN/OUT, S> */

TPM_RC
TPMU_PUBLIC_ID_Unmarshal(TPMU_PUBLIC_ID *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
#if ALG_KEYEDHASH
      case TPM_ALG_KEYEDHASH:
	rc = TPM2B_DIGEST_Unmarshal(&target->keyedHash, buffer, size);
	break;
#endif
#if ALG_SYMCIPHER
      case TPM_ALG_SYMCIPHER:
	rc = TPM2B_DIGEST_Unmarshal(&target->sym, buffer, size);
	break;
#endif
#if ALG_RSA
      case TPM_ALG_RSA: 
	rc = TPM2B_PUBLIC_KEY_RSA_Unmarshal(&target->rsa, buffer, size);
	break;
#endif
#if ALG_ECC
      case TPM_ALG_ECC:
	rc = TPMS_ECC_POINT_Unmarshal(&target->ecc, buffer, size);
	break;
#endif
      default:
	rc = TPM_RC_SELECTOR;
    }
    return rc;
}

/* Table 178 - Definition of TPMS_KEYEDHASH_PARMS Structure */

TPM_RC
TPMS_KEYEDHASH_PARMS_Unmarshal(TPMS_KEYEDHASH_PARMS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMT_KEYEDHASH_SCHEME_Unmarshal(&target->scheme, buffer, size, YES);
    }
    return rc;
}

/* Table 180 - Definition of {RSA} TPMS_RSA_PARMS Structure */

TPM_RC
TPMS_RSA_PARMS_Unmarshal(TPMS_RSA_PARMS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMT_SYM_DEF_OBJECT_Unmarshal(&target->symmetric, buffer, size, YES);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMT_RSA_SCHEME_Unmarshal(&target->scheme, buffer, size, YES);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_RSA_KEY_BITS_Unmarshal(&target->keyBits, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(&target->exponent, buffer, size);
    }
    return rc;
}

/* Table 181 - Definition of {ECC} TPMS_ECC_PARMS Structure */

TPM_RC
TPMS_ECC_PARMS_Unmarshal(TPMS_ECC_PARMS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMT_SYM_DEF_OBJECT_Unmarshal(&target->symmetric, buffer, size, YES);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMT_ECC_SCHEME_Unmarshal(&target->scheme, buffer, size, YES);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ECC_CURVE_Unmarshal(&target->curveID, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMT_KDF_SCHEME_Unmarshal(&target->kdf, buffer, size, YES);
    }
    return rc;
}

/* Table 182 - Definition of TPMU_PUBLIC_PARMS Union <IN/OUT, S> */

TPM_RC
TPMU_PUBLIC_PARMS_Unmarshal(TPMU_PUBLIC_PARMS *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
#if ALG_KEYEDHASH
      case TPM_ALG_KEYEDHASH:
	rc = TPMS_KEYEDHASH_PARMS_Unmarshal(&target->keyedHashDetail, buffer, size);
	break;
#endif
#if ALG_SYMCIPHER
      case TPM_ALG_SYMCIPHER:
	rc = TPMS_SYMCIPHER_PARMS_Unmarshal(&target->symDetail, buffer, size);
	break;
#endif
#if ALG_RSA
      case TPM_ALG_RSA:
	rc = TPMS_RSA_PARMS_Unmarshal(&target->rsaDetail, buffer, size);
	break;
#endif
#if ALG_ECC
      case TPM_ALG_ECC:
	rc = TPMS_ECC_PARMS_Unmarshal(&target->eccDetail, buffer, size);
	break;
#endif
      default:
	rc = TPM_RC_SELECTOR;
    }
    return rc;
}

/* Table 183 - Definition of TPMT_PUBLIC_PARMS Structure */

TPM_RC
TPMT_PUBLIC_PARMS_Unmarshal(TPMT_PUBLIC_PARMS *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_PUBLIC_Unmarshal(&target->type, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_PUBLIC_PARMS_Unmarshal(&target->parameters, buffer, size, target->type);
    }
    return rc;
}

/* Table 191 - Definition of TPMT_PUBLIC Structure */

TPM_RC
TPMT_PUBLIC_Unmarshal(TPMT_PUBLIC *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_PUBLIC_Unmarshal(&target->type, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_HASH_Unmarshal(&target->nameAlg, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMA_OBJECT_Unmarshal(&target->objectAttributes, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->authPolicy, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_PUBLIC_PARMS_Unmarshal(&target->parameters, buffer, size, target->type);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_PUBLIC_ID_Unmarshal(&target->unique, buffer, size, target->type);
    }
    return rc;
}

/* Table 192 - Definition of TPM2B_PUBLIC Structure */

TPM_RC
TPM2B_PUBLIC_Unmarshal(TPM2B_PUBLIC *target, BYTE **buffer, INT32 *size, BOOL allowNull)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    INT32 startSize;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size == 0) {
	    rc = TPM_RC_SIZE;
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	startSize = *size;
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMT_PUBLIC_Unmarshal(&target->publicArea, buffer, size, allowNull);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size != startSize - *size) {
	    rc = TPM_RC_SIZE;
	    target->size = 0; // libtpms added
	}
    }
    return rc;
}

/* Table 2:193 - Definition of TPM2B_TEMPLATE Structure (StructuresTable()) */

TPM_RC
TPM2B_TEMPLATE_Unmarshal(TPM2B_TEMPLATE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, sizeof(TPMT_PUBLIC), buffer, size);
    }
    return rc;
}

/* Table 187 - Definition of TPMU_SENSITIVE_COMPOSITE Union <IN/OUT, S> */

TPM_RC
TPMU_SENSITIVE_COMPOSITE_Unmarshal(TPMU_SENSITIVE_COMPOSITE *target, BYTE **buffer, INT32 *size, UINT32 selector)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    switch (selector) {
#if ALG_RSA
      case TPM_ALG_RSA:
	rc = TPM2B_PRIVATE_KEY_RSA_Unmarshal(&target->rsa, buffer, size);
	break;
#endif
#if ALG_ECC
      case TPM_ALG_ECC:
	rc = TPM2B_ECC_PARAMETER_Unmarshal(&target->ecc, buffer, size);
	break;
#endif
#if ALG_KEYEDHASH
      case TPM_ALG_KEYEDHASH:
	rc = TPM2B_SENSITIVE_DATA_Unmarshal(&target->bits, buffer, size);
	break;
#endif
#if ALG_SYMCIPHER
      case TPM_ALG_SYMCIPHER:
	rc = TPM2B_SYM_KEY_Unmarshal(&target->sym, buffer, size);
	break;
#endif
      default:
	rc = TPM_RC_SELECTOR;
    }
    return rc;
}

/* Table 188 - Definition of TPMT_SENSITIVE Structure */

TPM_RC
TPMT_SENSITIVE_Unmarshal(TPMT_SENSITIVE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_PUBLIC_Unmarshal(&target->sensitiveType, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_AUTH_Unmarshal(&target->authValue, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->seedValue, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMU_SENSITIVE_COMPOSITE_Unmarshal(&target->sensitive, buffer, size, target->sensitiveType);
    }
    return rc;
}

/* Table 189 - Definition of TPM2B_SENSITIVE Structure <IN/OUT> */

TPM_RC
TPM2B_SENSITIVE_Unmarshal(TPM2B_SENSITIVE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    INT32 startSize;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->size, buffer, size);
    }
    if (target->size != 0) {
	if (rc == TPM_RC_SUCCESS) {
	    startSize = *size;
	}
	if (rc == TPM_RC_SUCCESS) {
	    rc = TPMT_SENSITIVE_Unmarshal(&target->sensitiveArea, buffer, size);
	}
	if (rc == TPM_RC_SUCCESS) {
	    if (target->size != startSize - *size) {
		rc = TPM_RC_SIZE;
		target->size = 0; // libtpms added
	    }
	}
    }
    return rc;
}

/* Table 191 - Definition of TPM2B_PRIVATE Structure <IN/OUT, S> */

TPM_RC
TPM2B_PRIVATE_Unmarshal(TPM2B_PRIVATE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, sizeof(_PRIVATE), buffer, size);
    }
    return rc;
}
    
/* Table 193 - Definition of TPM2B_ID_OBJECT Structure <IN/OUT> */

TPM_RC
TPM2B_ID_OBJECT_Unmarshal(TPM2B_ID_OBJECT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, sizeof(TPMS_ID_OBJECT), buffer, size);
    }
    return rc;
}

/* Table 196 - Definition of (UINT32) TPMA_NV Bits */

TPM_RC
TPMA_NV_Unmarshal(TPMA_NV *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    TPMA_NV orig_target = *target; // libtpms added

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal((UINT32 *)target, buffer, size);  /* libtpms changed */
    }
    if (rc == TPM_RC_SUCCESS) {
	if (*target & TPMA_NV_RESERVED) {
	    rc = TPM_RC_RESERVED_BITS;
	    *target = orig_target; // libtpms added
	}
    }
    return rc;
}

/* Table 197 - Definition of TPMS_NV_PUBLIC Structure */

TPM_RC
TPMS_NV_PUBLIC_Unmarshal(TPMS_NV_PUBLIC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_RH_NV_INDEX_Unmarshal(&target->nvIndex, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_ALG_HASH_Unmarshal(&target->nameAlg, buffer, size, NO);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMA_NV_Unmarshal(&target->attributes, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_DIGEST_Unmarshal(&target->authPolicy, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->dataSize, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->dataSize > MAX_NV_INDEX_SIZE) {
	    rc = TPM_RC_SIZE;
	    target->dataSize = 0; // libtpms added
	}
    }
    return rc;
}

/* Table 198 - Definition of TPM2B_NV_PUBLIC Structure */

TPM_RC
TPM2B_NV_PUBLIC_Unmarshal(TPM2B_NV_PUBLIC *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    INT32 startSize;
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT16_Unmarshal(&target->size, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size == 0) {
	    rc = TPM_RC_SIZE;
	}
    }
    if (rc == TPM_RC_SUCCESS) {
	startSize = *size;
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMS_NV_PUBLIC_Unmarshal(&target->nvPublic, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	if (target->size != startSize - *size) {
	    rc = TPM_RC_SIZE;
	    target->size = 0; // libtpms added
	}
    }
    return rc;
}

/* Table 199 - Definition of TPM2B_CONTEXT_SENSITIVE Structure <IN/OUT> */

#if 0		// libtpms added
TPM_RC
TPM2B_CONTEXT_SENSITIVE_Unmarshal(TPM2B_CONTEXT_SENSITIVE *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, MAX_CONTEXT_SIZE, buffer, size);
    }
    return rc;
}
#endif		// libtpms added

/* Table 201 - Definition of TPM2B_CONTEXT_DATA Structure <IN/OUT> */

TPM_RC
TPM2B_CONTEXT_DATA_Unmarshal(TPM2B_CONTEXT_DATA *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_Unmarshal(&target->b, sizeof(TPMS_CONTEXT_DATA), buffer, size);
    }
    return rc;
}

/* Table 202 - Definition of TPMS_CONTEXT Structure */

TPM_RC
TPMS_CONTEXT_Unmarshal(TPMS_CONTEXT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;
    INT32 orig_size = *size; // libtpms added
    
    if (rc == TPM_RC_SUCCESS) {
	rc = UINT64_Unmarshal(&target->sequence, buffer, size);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_DH_SAVED_Unmarshal(&target->savedHandle, buffer, size, NO);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPMI_RH_HIERARCHY_Unmarshal(&target->hierarchy, buffer, size, YES);
    }
    if (rc == TPM_RC_SUCCESS) {
	rc = TPM2B_CONTEXT_DATA_Unmarshal(&target->contextBlob, buffer, size);
    }
    // libtpms added begin
    if (rc == TPM_RC_SUCCESS) {
        if (*size > 0) {
            /* Windows 2019 server pads the command TPM_ContextLoad up to the value of
             * TPM_PT_MAX_OBJECT_CONTENT for the TPMS_CONTEXT part and we end up with
             * left-over padding bytes here that will make the TPM2_ContextLoad command
             * fail. This is because we don't just write an OBJECT as the context but use
             * ANY_OBJECT_Marshal to write it, which consumes less bytes. We had to do
             * this due to a Linux TPM resource manager bug that couldn't deal with the
             * larger context sizes once RSA 3072 was enabled and it ran out of memory
             * when receiving contexts.
             * Luckily only one command needs TPMS_CONTEXT unmarshalled, so we can adjust
             * for the left-over padding here but also ONLY do this if
             * 'orig_size' == value(TPM_PT_MAX_OBJECT_CONTENT).
             */
            static UINT32 tpm_pt_max_object_context;

            if (tpm_pt_max_object_context == 0) {
                TPML_TAGGED_TPM_PROPERTY tttp;

                TPMCapGetProperties(TPM_PT_MAX_OBJECT_CONTEXT, 1, &tttp);
                if (tttp.count == 1)
                    tpm_pt_max_object_context = tttp.tpmProperty[0].value;
            }
            if ((UINT32)orig_size == tpm_pt_max_object_context)
                *size = 0; /* consume the padding bytes */
        }
    }
    // libtpms added end
    return rc;
}

/* Table 225 - Definition of (UINT32) TPM_AT Constants */

TPM_RC
TPM_AT_Unmarshal(TPM_AT *target, BYTE **buffer, INT32 *size)
{
    TPM_RC rc = TPM_RC_SUCCESS;

    if (rc == TPM_RC_SUCCESS) {
	rc = UINT32_Unmarshal(target, buffer, size);  
    }
    return rc;
}
