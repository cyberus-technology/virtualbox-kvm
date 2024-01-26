/********************************************************************************/
/*										*/
/*		Backwards compatibility stuff related to OBJECT		*/
/*			     Written by Stefan Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*										*/
/* (c) Copyright IBM Corporation 2017,2018.					*/
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

#include <assert.h>

#include "BackwardsCompatibilityObject.h"

#ifndef static_assert
#define static_assert(test, msg)
#endif

/* The following are data structure from libtpms 0.7.x with RSA 2048 support
 * that help to resume key and hash contexts (TPM2_ContextSave/Load) from this
 * earlier version. All structures that have different sizes in 0.8 are found
 * here.
 */
typedef union {
    struct {
	UINT16                  size;
	BYTE                    buffer[2048/8];
    }            t;
    TPM2B        b;
} OLD_TPM2B_PUBLIC_KEY_RSA;

typedef union {
    TPM2B_DIGEST             keyedHash;
    TPM2B_DIGEST             sym;
    OLD_TPM2B_PUBLIC_KEY_RSA rsa;
    TPMS_ECC_POINT           ecc;
//    TPMS_DERIVE             derive;
} OLD_TPMU_PUBLIC_ID;

typedef struct {
    TPMI_ALG_PUBLIC         type;
    TPMI_ALG_HASH           nameAlg;
    TPMA_OBJECT             objectAttributes;
    TPM2B_DIGEST            authPolicy;
    TPMU_PUBLIC_PARMS       parameters;
    OLD_TPMU_PUBLIC_ID      unique;
} OLD_TPMT_PUBLIC;

static_assert(sizeof(OLD_TPMT_PUBLIC) == 356,
	      "OLD_TPMT_PUBLIC has wrong size");

typedef union {
    struct {
	UINT16                  size;
	BYTE                    buffer[((2048/8)/2)*5];
    }            t;
    TPM2B        b;
} OLD_TPM2B_PRIVATE_KEY_RSA;

static_assert(sizeof(OLD_TPM2B_PRIVATE_KEY_RSA) == 642,
	      "OLD_TPM2B_PRIVATE_KEY_RSA has wrong size");

typedef union {
    struct {
	UINT16                  size;
	BYTE                    buffer[((2048/8)/2)*5];
    }            t;
    TPM2B        b;
} OLD_TPM2B_PRIVATE_VENDOR_SPECIFIC;

typedef union {
    OLD_TPM2B_PRIVATE_KEY_RSA         rsa;
    TPM2B_ECC_PARAMETER               ecc;
    TPM2B_SENSITIVE_DATA              bits;
    TPM2B_SYM_KEY                     sym;
    OLD_TPM2B_PRIVATE_VENDOR_SPECIFIC any;
} OLD_TPMU_SENSITIVE_COMPOSITE;

typedef struct {
    TPMI_ALG_PUBLIC              sensitiveType;
    TPM2B_AUTH                   authValue;
    TPM2B_DIGEST                 seedValue;
    OLD_TPMU_SENSITIVE_COMPOSITE sensitive;
} OLD_TPMT_SENSITIVE;

static_assert(sizeof(OLD_TPMT_SENSITIVE) == 776,
	      "OLD_TPMT_SENSITIVE has wrong size");

BN_TYPE(old_prime, (2048 / 2));

typedef struct OLD_privateExponent
{
    bn_old_prime_t          Q;
    bn_old_prime_t          dP;
    bn_old_prime_t          dQ;
    bn_old_prime_t          qInv;
} OLD_privateExponent_t;

static inline void CopyFromOldPrimeT(bn_prime_t *dst,
				     const bn_old_prime_t *src)
{
    dst->allocated = src->allocated;
    dst->size = src->size;
    memcpy(dst->d, src->d, sizeof(src->d));
}

static_assert(sizeof(OLD_privateExponent_t) == 608,
	      "OLD_privateExponent_t has wrong size");

typedef struct OLD_OBJECT
{
    // The attributes field is required to be first followed by the publicArea.
    // This allows the overlay of the object structure and a sequence structure
    OBJECT_ATTRIBUTES   attributes;         // object attributes
    OLD_TPMT_PUBLIC     publicArea;         // public area of an object
    OLD_TPMT_SENSITIVE  sensitive;          // sensitive area of an object
    OLD_privateExponent_t privateExponent;  // Additional field for the private
    TPM2B_NAME          qualifiedName;      // object qualified name
    TPMI_DH_OBJECT      evictHandle;        // if the object is an evict object,
    // the original handle is kept here.
    // The 'working' handle will be the
    // handle of an object slot.
    TPM2B_NAME          name;               // Name of the object name. Kept here
    // to avoid repeatedly computing it.

    // libtpms added: OBJECT lies in NVRAM; to avoid that it needs different number
    // of bytes on 32 bit and 64 bit architectures, we need to make sure it's the
    // same size; simple padding at the end works here
    UINT32             _pad;
} OLD_OBJECT;

static_assert(sizeof(OLD_OBJECT) == 1896,
	      "OLD_OBJECT has wrong size");

// Convert an OLD_OBJECT that was copied into buffer using MemoryCopy
TPM_RC
OLD_OBJECTToOBJECT(OBJECT *newObject, BYTE *buffer, INT32 size)
{
    OLD_OBJECT    oldObject;
    TPM_RC        rc = 0;

    // get the attributes
    MemoryCopy(newObject, buffer, sizeof(newObject->attributes));
    if (ObjectIsSequence(newObject))
	{
	    /* resuming old hash contexts is not supported */
	    rc = TPM_RC_DISABLED;
	}
    else
        {
	    if (size != sizeof(OLD_OBJECT))
		return TPM_RC_SIZE;
	    MemoryCopy(&oldObject, buffer, sizeof(OLD_OBJECT));

	    /* fill the newObject with the contents of the oldObject */
	    newObject->attributes = oldObject.attributes;

	    newObject->publicArea.type = oldObject.publicArea.type;
	    newObject->publicArea.nameAlg = oldObject.publicArea.nameAlg;
	    newObject->publicArea.objectAttributes = oldObject.publicArea.objectAttributes;
	    newObject->publicArea.authPolicy = oldObject.publicArea.authPolicy;
	    newObject->publicArea.parameters = oldObject.publicArea.parameters;
	    /* the unique part can be one or two TPM2B's */
	    switch (newObject->publicArea.type) {
	    case TPM_ALG_KEYEDHASH:
		MemoryCopy2B(&newObject->publicArea.unique.keyedHash.b,
			     &oldObject.publicArea.unique.keyedHash.b,
			     sizeof(oldObject.publicArea.unique.keyedHash.t.buffer));
		break;
	    case TPM_ALG_SYMCIPHER:
		MemoryCopy2B(&newObject->publicArea.unique.sym.b,
			     &oldObject.publicArea.unique.sym.b,
			     sizeof(oldObject.publicArea.unique.sym.t.buffer));
		break;
	    case TPM_ALG_RSA:
		MemoryCopy2B(&newObject->publicArea.unique.rsa.b,
			     &oldObject.publicArea.unique.rsa.b,
			     sizeof(oldObject.publicArea.unique.rsa.t.buffer));
		break;
	    case TPM_ALG_ECC:
		MemoryCopy2B(&newObject->publicArea.unique.ecc.x.b,
			     &oldObject.publicArea.unique.ecc.x.b,
			     sizeof(oldObject.publicArea.unique.ecc.x.t.buffer));
		MemoryCopy2B(&newObject->publicArea.unique.ecc.y.b,
			     &oldObject.publicArea.unique.ecc.y.b,
			     sizeof(oldObject.publicArea.unique.ecc.y.t.buffer));
		break;
	    }

	    newObject->sensitive.sensitiveType = oldObject.sensitive.sensitiveType;
	    newObject->sensitive.authValue = oldObject.sensitive.authValue;
	    newObject->sensitive.seedValue = oldObject.sensitive.seedValue;
	    /* The OLD_TPMU_SENSITIVE_COMPOSITE is always a TPM2B */
	    MemoryCopy2B(&newObject->sensitive.sensitive.any.b,
			 &oldObject.sensitive.sensitive.any.b,
			 sizeof(oldObject.sensitive.sensitive.any.t.buffer));

	    CopyFromOldPrimeT(&newObject->privateExponent.Q, &oldObject.privateExponent.Q);
	    CopyFromOldPrimeT(&newObject->privateExponent.dP, &oldObject.privateExponent.dP);
	    CopyFromOldPrimeT(&newObject->privateExponent.dQ, &oldObject.privateExponent.dQ);
	    CopyFromOldPrimeT(&newObject->privateExponent.qInv, &oldObject.privateExponent.qInv);

	    newObject->qualifiedName = oldObject.qualifiedName;
	    newObject->evictHandle = oldObject.evictHandle;
	    newObject->name = oldObject.name;
    }

    return rc;
}

