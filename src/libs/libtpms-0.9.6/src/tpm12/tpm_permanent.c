/********************************************************************************/
/*										*/
/*			Permanent Flag and Data Handler				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_permanent.c $		*/
/*										*/
/* (c) Copyright IBM Corporation 2006, 2010.					*/
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tpm_audit.h"
#include "tpm_counter.h"
#include "tpm_crypto.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_delegate.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_global.h"
#include "tpm_key.h"
#include "tpm_nonce.h"
#include "tpm_nvfile.h"
#include "tpm_nvfilename.h"
#include "tpm_nvram.h"
#include "tpm_pcr.h"
#include "tpm_secret.h"
#include "tpm_storage.h"
#include "tpm_structures.h"
#include "tpm_types.h"
#include "tpm_svnrevision.h"


#include "tpm_permanent.h"

/*
  TPM_PERMANENT_FLAGS
*/

void TPM_PermanentFlags_Init(TPM_PERMANENT_FLAGS *tpm_permanent_flags)
{
    printf(" TPM_PermanentFlags_Init:\n");
#ifndef TPM_ENABLE_ACTIVATE
    tpm_permanent_flags->disable = TRUE;
#else	/* for servers, not TCG standard */
    tpm_permanent_flags->disable = FALSE;
#endif
    tpm_permanent_flags->ownership = TRUE;
#ifndef TPM_ENABLE_ACTIVATE
    tpm_permanent_flags->deactivated = TRUE;
#else	/* for servers, not TCG standard */
    tpm_permanent_flags->deactivated = FALSE;
#endif
    tpm_permanent_flags->readPubek = TRUE; 
    tpm_permanent_flags->disableOwnerClear = FALSE;
    tpm_permanent_flags->allowMaintenance = TRUE;
    tpm_permanent_flags->physicalPresenceLifetimeLock = FALSE;
    tpm_permanent_flags->physicalPresenceHWEnable = FALSE;
#ifndef TPM_PP_CMD_ENABLE	/* TCG standard */
    tpm_permanent_flags->physicalPresenceCMDEnable = FALSE;
#else				/* 'ship' TRUE */
    tpm_permanent_flags->physicalPresenceCMDEnable = TRUE;
#endif
    /* tpm_permanent_flags->CEKPUsed = ; This flag has no default value */
    tpm_permanent_flags->TPMpost = FALSE;
    tpm_permanent_flags->TPMpostLock = FALSE;
    tpm_permanent_flags->FIPS = FALSE;	/* if TRUE, could not test no-auth commands */
    tpm_permanent_flags->tpmOperator = FALSE;
    tpm_permanent_flags->enableRevokeEK = TRUE;
    tpm_permanent_flags->nvLocked = FALSE;
    tpm_permanent_flags->readSRKPub = FALSE;
    tpm_permanent_flags->tpmEstablished = FALSE;
    tpm_permanent_flags->maintenanceDone = FALSE;
#if  (TPM_REVISION >= 103)	/* added for rev 103 */
    tpm_permanent_flags->disableFullDALogicInfo = FALSE;
#endif
}

/* TPM_PermanentFlags_Load()

   deserializes the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes

   It is used when deserializing the structure from storage in NVRAM.
*/

TPM_RESULT TPM_PermanentFlags_Load(TPM_PERMANENT_FLAGS *tpm_permanent_flags,
				   unsigned char **stream,
				   uint32_t *stream_size)
{
    TPM_RESULT	rc = 0;
    uint32_t 	tpm_bitmap;
    TPM_TAG	permanentFlagsVersion;
    
    printf(" TPM_PermanentFlags_Load:\n");
    /* load the TPM_PERMANENT_FLAGS version tag from the stream */
    if (rc == 0) {
	rc = TPM_Load16(&permanentFlagsVersion, stream, stream_size);
    }
    /* load the TPM_PERMANENT_FLAGS from the stream */
    if (rc == 0) {
	rc = TPM_Load32(&tpm_bitmap, stream, stream_size);
    }
    /* load the TPM_PERMANENT_FLAGS from the bitmap */
    if (rc == 0) {
	rc = TPM_PermanentFlags_LoadBitmap(tpm_permanent_flags, permanentFlagsVersion, tpm_bitmap);
    }
    return rc;
}

/* TPM_PermanentFlags_Store() serializes the TPM_PERMANENT_FLAGS structure as a bitmap.

   It is used when serializing the structure for storage in NVRAM.
*/

TPM_RESULT TPM_PermanentFlags_Store(TPM_STORE_BUFFER *sbuffer,
				    const TPM_PERMANENT_FLAGS *tpm_permanent_flags)

{
    TPM_RESULT rc = 0;
    uint32_t tpm_bitmap;

    printf(" TPM_PermanentFlags_Store:\n");
    /* store the TPM_PERMANENT_FLAGS structure in a bit map */
    if (rc == 0) {
	rc = TPM_PermanentFlags_StoreBitmap(&tpm_bitmap, tpm_permanent_flags);
    }
    /* append a TPM_PERMANENT_FLAGS version tag */
#if  (TPM_REVISION >= 103)
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_NVSTATE_PF103);
    }
#else
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_NVSTATE_PF94);
    }
#endif
    /* append the bitmap to the stream */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_bitmap);
    }
    return rc;
}

/* TPM_PermanentFlags_StoreBytes() serializes the TPM_PERMANENT_FLAGS structure as bytes

 */

TPM_RESULT TPM_PermanentFlags_StoreBytes(TPM_STORE_BUFFER *sbuffer,
					 const TPM_PERMANENT_FLAGS *tpm_permanent_flags)
{
    TPM_RESULT rc = 0;
     
    printf(" TPM_PermanentFlags_StoreBytes:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_PERMANENT_FLAGS);
    }
    /* store disable */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->disable), sizeof(BYTE));
    }
    /* store ownership */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->ownership), sizeof(BYTE));
    }
    /* store deactivated */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->deactivated), sizeof(BYTE));
    }
    /* store readPubek */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->readPubek), sizeof(BYTE));
    }
    /* store disableOwnerClear */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->disableOwnerClear), sizeof(BYTE));
    }
    /* store allowMaintenance */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->allowMaintenance), sizeof(BYTE));
    }
    /* store physicalPresenceLifetimeLock */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->physicalPresenceLifetimeLock),
				sizeof(BYTE));
    }
    /* store physicalPresenceHWEnable */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->physicalPresenceHWEnable),
				sizeof(BYTE));
    }
    /* store physicalPresenceCMDEnable */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->physicalPresenceCMDEnable),
				sizeof(BYTE));
    }
    /* store CEKPUsed */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->CEKPUsed), sizeof(BYTE));
    }
    /* store TPMpost */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->TPMpost), sizeof(BYTE));
    }
    /* store TPMpostLock */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->TPMpostLock), sizeof(BYTE));
    }
    /* store FIPS */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->FIPS), sizeof(BYTE));		
    }
    /* store operator */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->tpmOperator), sizeof(BYTE));
    }
    /* store enableRevokeEK */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->enableRevokeEK), sizeof(BYTE));
    }
    /* store nvLocked */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->nvLocked), sizeof(BYTE));
    }
    /* store readSRKPub */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->readSRKPub), sizeof(BYTE));
    }
    /* store tpmEstablished */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->tpmEstablished), sizeof(BYTE));
    }
    /* store maintenanceDone */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_flags->maintenanceDone), sizeof(BYTE));
    }
#if  (TPM_REVISION >= 103)	/* added for rev 103 */
    /* store disableFullDALogicInfo */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer,
				&(tpm_permanent_flags->disableFullDALogicInfo), sizeof(BYTE));
    }
#endif
    return rc;
}

/* TPM_PermanentFlags_LoadBitmap() loads the TPM_PERMANENT_FLAGS structure from the bit map

   permanentFlagsVersion indicates the version being loaded from NVRAM
*/

TPM_RESULT TPM_PermanentFlags_LoadBitmap(TPM_PERMANENT_FLAGS *tpm_permanent_flags,
					 TPM_TAG permanentFlagsVersion,
					 uint32_t tpm_bitmap)
{
    TPM_RESULT	rc = 0;
    uint32_t	pos = 0;	/* position in bitmap */
    
    if (rc == 0) {
	switch (permanentFlagsVersion) {
	  case TPM_TAG_NVSTATE_PF94:
	    break;
	  case TPM_TAG_NVSTATE_PF103:
	    /* if the TPM_REVISION supports the permanentFlagsVersion, break with no error.  If it
	       doesn't, omit the break and fall through to the unsupported case. */
#if  (TPM_REVISION >= 103)
	    break;
#endif
	  default:
	    /* no forward compatibility */
	    printf("TPM_PermanentFlags_LoadBitmap: Error (fatal) unsupported version tag %04x\n",
		   permanentFlagsVersion);
	    rc = TPM_FAIL;
	    break;
	}
    }
    printf(" TPM_PermanentFlags_LoadBitmap:\n");
    /* load disable */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->disable), tpm_bitmap, &pos);
    }
    /* load ownership */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->ownership), tpm_bitmap, &pos);		 
    }
    /* load deactivated */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->deactivated), tpm_bitmap, &pos);		 
    }
    /* load readPubek */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->readPubek), tpm_bitmap, &pos);		 
    }
    /* load disableOwnerClear */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->disableOwnerClear), tpm_bitmap, &pos);
    }
    /* load allowMaintenance */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->allowMaintenance), tpm_bitmap, &pos);
    }
    /* load physicalPresenceLifetimeLock */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->physicalPresenceLifetimeLock),
			     tpm_bitmap, &pos);
    }
    /* load physicalPresenceHWEnable */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->physicalPresenceHWEnable), tpm_bitmap, &pos);
    }
    /* load physicalPresenceCMDEnable */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->physicalPresenceCMDEnable), tpm_bitmap, &pos);
    }
    /* load CEKPUsed */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->CEKPUsed), tpm_bitmap, &pos);
    }
    /* load TPMpost */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->TPMpost), tpm_bitmap, &pos);	 
    }
    /* load TPMpostLock */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->TPMpostLock), tpm_bitmap, &pos);		 
    }
    /* load FIPS */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->FIPS), tpm_bitmap, &pos);		 
    }
    /* load operator */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->tpmOperator), tpm_bitmap, &pos);		    
    }
    /* load enableRevokeEK */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->enableRevokeEK), tpm_bitmap, &pos);
    }
    /* load nvLocked */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->nvLocked), tpm_bitmap, &pos);
    }
    /* load readSRKPub */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->readSRKPub), tpm_bitmap, &pos);
    }
    /* load tpmEstablished */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->tpmEstablished), tpm_bitmap, &pos);
    }
    /* load maintenanceDone */
    if (rc == 0) {
	rc = TPM_Bitmap_Load(&(tpm_permanent_flags->maintenanceDone), tpm_bitmap, &pos);
    }
#if  (TPM_REVISION >= 103)	/* added for rev 103 */
    if (rc == 0) {
	switch (permanentFlagsVersion) {
	  case TPM_TAG_NVSTATE_PF94:
	    /* 94 to 103, set extra flags to default value */
	    tpm_permanent_flags->disableFullDALogicInfo = FALSE;
	    break;
	  case TPM_TAG_NVSTATE_PF103:
	    /* 103 to 103, process normally */
	    /* load disableFullDALogicInfo */
	    rc = TPM_Bitmap_Load(&(tpm_permanent_flags->disableFullDALogicInfo), tpm_bitmap, &pos);
	    break;
	}
    }
#endif
    return rc;
}

/* TPM_PermanentFlags_StoreBitmap() stores the TPM_PERMANENT_FLAGS structure in a bit map

   It is used when serializing the structure for storage in NVRAM and as the return to
   TPM_GetCapability.
*/

TPM_RESULT TPM_PermanentFlags_StoreBitmap(uint32_t *tpm_bitmap,
					  const TPM_PERMANENT_FLAGS *tpm_permanent_flags)
{
    TPM_RESULT	rc = 0;
    uint32_t	pos = 0;	/* position in bitmap */
    
    printf(" TPM_PermanentFlags_StoreBitmap:\n");
    *tpm_bitmap = 0;		/* set unused bits to 0 */
    /* store disable */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->disable, &pos);
    }
    /* store ownership */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->ownership, &pos);	       
    }
    /* store deactivated */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->deactivated, &pos);	       
    }
    /* store readPubek */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->readPubek, &pos);	       
    }
    /* store disableOwnerClear */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->disableOwnerClear, &pos);
    }
    /* store allowMaintenance */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->allowMaintenance, &pos);
    }
    /* store physicalPresenceLifetimeLock */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->physicalPresenceLifetimeLock, &pos);
    }
    /* store physicalPresenceHWEnable */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->physicalPresenceHWEnable, &pos);
    }
    /* store physicalPresenceCMDEnable */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->physicalPresenceCMDEnable, &pos);
    }
    /* store CEKPUsed */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->CEKPUsed, &pos);
    }
    /* store TPMpost */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->TPMpost, &pos);	       
    }
    /* store TPMpostLock */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->TPMpostLock, &pos);	       
    }
    /* store FIPS */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->FIPS, &pos);	       
    }
    /* store operator */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->tpmOperator, &pos);		  
    }
    /* store enableRevokeEK */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->enableRevokeEK, &pos);
    }
    /* store nvLocked */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->nvLocked, &pos);
    }
    /* store readSRKPub */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->readSRKPub, &pos);
    }
    /* store tpmEstablished */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->tpmEstablished, &pos);
    }
    /* store maintenanceDone */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->maintenanceDone, &pos);
    }
#if  (TPM_REVISION >= 103)	/* added for rev 103 */
    /* store disableFullDALogicInfo */
    if (rc == 0) {
	rc = TPM_Bitmap_Store(tpm_bitmap, tpm_permanent_flags->disableFullDALogicInfo, &pos);
    }
#endif
    return rc;
}

/*
  TPM_PERMANENT_DATA
*/

/* TPM_PermanentData_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0

   This function generates a new contextKey, delegateKey, daaBlobKey.
*/


TPM_RESULT TPM_PermanentData_Init(TPM_PERMANENT_DATA *tpm_permanent_data,
				  TPM_BOOL instanceData)
{
    TPM_RESULT		rc = 0;
    
    printf(" TPM_PermanentData_Init:\n");
    if (rc == 0) {
	tpm_permanent_data->revMajor = ((tpm_svn_revision >> 8) & 0xff);
	tpm_permanent_data->revMinor = ((tpm_svn_revision     ) & 0xff);
	printf("  TPM_PermanentData_Init: revMajor %02x revMinor %02x\n",
	       tpm_permanent_data->revMajor, tpm_permanent_data->revMinor);
	/* zero all secrets */
	TPM_PermanentData_Zero(tpm_permanent_data, instanceData);
	
#ifndef TPM_NOMAINTENANCE
	TPM_Pubkey_Init(&(tpm_permanent_data->manuMaintPub));
#endif
	TPM_Key_Init(&(tpm_permanent_data->endorsementKey));
	TPM_Key_Init(&(tpm_permanent_data->srk));
	tpm_permanent_data->contextKey = NULL;
	rc = TPM_SymmetricKeyData_New(&(tpm_permanent_data->contextKey));
    }
    if (rc == 0) {
	tpm_permanent_data->delegateKey = NULL;
	rc = TPM_SymmetricKeyData_New(&(tpm_permanent_data->delegateKey));
    }
    if (rc == 0) {
	TPM_CounterValue_Init(&(tpm_permanent_data->auditMonotonicCounter));
	TPM_Counters_Init(tpm_permanent_data->monotonicCounter);
	TPM_PCRAttributes_Init(tpm_permanent_data->pcrAttrib);
	rc = TPM_OrdinalAuditStatus_Init(tpm_permanent_data);
    }
    if (rc == 0) {
	TPM_FamilyTable_Init(&(tpm_permanent_data->familyTable));
	TPM_DelegateTable_Init(&(tpm_permanent_data->delegateTable));
	tpm_permanent_data->lastFamilyID = 0;
	tpm_permanent_data->noOwnerNVWrite = 0;
	tpm_permanent_data->restrictDelegate = 0;
	/* tpmDAASeed done by TPM_PermanentData_Zero() */
	/* daaProof done by TPM_PermanentData_Zero() */
	rc = TPM_SymmetricKeyData_New(&(tpm_permanent_data->daaBlobKey));
    }
    if (rc == 0) {
	tpm_permanent_data->ownerInstalled = FALSE;
	/* tscOrdinalAuditStatus initialized by TPM_OrdinalAuditStatus_Init() */
	/* instanceOrdinalAuditStatus initialized by TPM_OrdinalAuditStatus_Init() */
	tpm_permanent_data->allowLoadMaintPub = TRUE;
	if (instanceData) {
	}
    }
    return rc;
}

/* TPM_PermanentData_Load()

   deserializes the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
*/

TPM_RESULT TPM_PermanentData_Load(TPM_PERMANENT_DATA *tpm_permanent_data,
				  unsigned char **stream,
				  uint32_t *stream_size,
				  TPM_BOOL instanceData)
{
    TPM_RESULT 		rc = 0;
    size_t 		i;
    TPM_BOOL		tpm_bool;
    
     
    printf(" TPM_PermanentData_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_PERMANENT_DATA, stream, stream_size);
    }
    /* load revMajor */
    /* load revMinor */
    /* not stored, loaded from hard coded value */
    if (rc == 0) {
	tpm_permanent_data->revMajor = (tpm_svn_revision >> 8) & 0xff;
	tpm_permanent_data->revMinor = tpm_svn_revision & 0xff;
    }
    /* load tpmProof */
    if (rc == 0) {
	printf("  TPM_PermanentData_Load: Loading tpmProof\n");
	rc = TPM_Secret_Load(tpm_permanent_data->tpmProof, stream, stream_size);
    }
    /* load EKReset */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_permanent_data->EKReset, stream, stream_size);
    }
    /* load ownerAuth */
    if (rc == 0) {
	printf("  TPM_PermanentData_Load: Loading ownerAuth \n");
	rc = TPM_Secret_Load(tpm_permanent_data->ownerAuth, stream, stream_size);
    }
    /* load operatorAuth */
    if (rc == 0) {
	rc = TPM_Secret_Load(tpm_permanent_data->operatorAuth, stream, stream_size);
    }
    /* load authDIR */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_permanent_data->authDIR, stream, stream_size);
    }
    /* load manuMaintPub present marker */
    if (rc == 0) {
	rc = TPM_Load8(&tpm_bool, stream, stream_size);
    }
#ifndef TPM_NOMAINTENANCE
    /* check that manuMaintPub is present */
    if (rc == 0) {
	if (!tpm_bool) {
	    printf("  TPM_PermanentData_Load: Error (fatal) missing manuMaintPub\n");
	    rc = TPM_FAIL;
	}
    }
    /* load manuMaintPub */ 
    if (rc == 0) {
	printf("  TPM_PermanentData_Load: Load manuMaintPub\n");
	rc = TPM_Pubkey_Load(&(tpm_permanent_data->manuMaintPub), stream, stream_size);
    }
#else
    /* check that manuMaintPub is absent */
    if (tpm_bool) {
	printf("  TPM_PermanentData_Load: Error (fatal) contains manuMaintPub\n");
	rc = TPM_FAIL;
    }
#endif
    /* load endorsementKey */
    if (rc == 0) {
	printf("  TPM_PermanentData_Load: Load endorsement key\n");
	rc = TPM_Key_LoadClear(&(tpm_permanent_data->endorsementKey), TRUE, stream, stream_size);
    }
    /* load srk */
    if (rc == 0) {
	printf("  TPM_PermanentData_Load: Load SRK\n");
	rc = TPM_Key_LoadClear(&(tpm_permanent_data->srk), FALSE, stream, stream_size);
    }
    /* load contextKey */
    if (rc == 0) {
	printf("  TPM_PermanentData_Load: Load contextKey\n");
	rc = TPM_SymmetricKeyData_Load(tpm_permanent_data->contextKey, stream, stream_size);
    }
    /* load delegateKey */
    if (rc == 0) {
	printf("  TPM_PermanentData_Load: Load delegateKey\n");
	rc = TPM_SymmetricKeyData_Load(tpm_permanent_data->delegateKey, stream, stream_size);
    }
    /* load auditMonotonicCounter */
    if (rc == 0) {
	rc = TPM_CounterValue_Load(&(tpm_permanent_data->auditMonotonicCounter),
				   stream, stream_size);
    }
    /* load monotonicCounter's */
    if (rc == 0) {
	rc = TPM_Counters_Load(tpm_permanent_data->monotonicCounter, stream, stream_size);
    }
    /* load pcrAttrib's, since they are constants, no need to load from NV space */
    if (rc == 0) {
	TPM_PCRAttributes_Init(tpm_permanent_data->pcrAttrib);
    }
    if (rc == 0) {
	printf("  TPM_PermanentData_Load: Load ordinalAuditStatus\n");
    }
    /* load ordinalAuditStatus's */
    for (i = 0 ; (rc == 0) && (i < (TPM_ORDINALS_MAX/CHAR_BIT)) ; i++) {
	rc = TPM_Load8(&(tpm_permanent_data->ordinalAuditStatus[i]), stream, stream_size);
    }
    /* load familyTable */
    if (rc == 0) {
	rc = TPM_FamilyTable_Load(&(tpm_permanent_data->familyTable), stream, stream_size);
    }
    /* load delegateTable */
    if (rc == 0) {
	rc = TPM_DelegateTable_Load(&(tpm_permanent_data->delegateTable), stream, stream_size);
    }
    /* load lastFamilyID */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_permanent_data->lastFamilyID), stream, stream_size);
    }
    /* load noOwnerNVWrite */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_permanent_data->noOwnerNVWrite), stream, stream_size);
    }
    /* load restrictDelegate */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_permanent_data->restrictDelegate), stream, stream_size);
    }
    /* load tpmDAASeed */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_permanent_data->tpmDAASeed, stream, stream_size);
    }
    /* load ownerInstalled */
    if (rc == 0) {
	rc = TPM_LoadBool(&(tpm_permanent_data->ownerInstalled), stream, stream_size);
    }
    /* load tscOrdinalAuditStatus */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_permanent_data->tscOrdinalAuditStatus), stream, stream_size);
    }
    /* load allowLoadMaintPub */
    if (rc == 0) {
	rc = TPM_LoadBool(&(tpm_permanent_data->allowLoadMaintPub), stream, stream_size);
    }
    /* load daaProof */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_permanent_data->daaProof, stream, stream_size);
    }
    /* load daaBlobKey */
    if (rc == 0) {
	printf("  TPM_PermanentData_Load: Loading DAA Blob key\n");
	rc = TPM_SymmetricKeyData_Load(tpm_permanent_data->daaBlobKey, stream, stream_size);
    }
    instanceData = instanceData;	/* to quiet the compiler */
    return rc;
}

/* TPM_PermanentData_Store() serializes the TPM_PERMANENT_DATA structure

 */

TPM_RESULT TPM_PermanentData_Store(TPM_STORE_BUFFER *sbuffer,
				   TPM_PERMANENT_DATA *tpm_permanent_data,
				   TPM_BOOL instanceData)
{
    TPM_RESULT 	rc = 0;
    size_t 	i;
    
    printf(" TPM_PermanentData_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_PERMANENT_DATA);
    }
    /* store revMajor */
    /* store revMinor */
    /* not stored, loaded from hard coded value */
    /* store tpmProof */
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_permanent_data->tpmProof);
    }
    /* store EKReset */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_permanent_data->EKReset);		
    }
    /* store ownerAuth */
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_permanent_data->ownerAuth);	
    }
    /* store operatorAuth */
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_permanent_data->operatorAuth);	
    }
    /* store authDIR */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_permanent_data->authDIR);
    }
#ifndef TPM_NOMAINTENANCE
    /* mark that manuMaintPub is present */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append8(sbuffer, TRUE);
    }
    /* store manuMaintPub */
    if (rc == 0) {
	rc = TPM_Pubkey_Store(sbuffer, &(tpm_permanent_data->manuMaintPub));
    }
#else
    /* mark that manuMaintPub is absent */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, FALSE);
    }
#endif
    /* store endorsementKey */
    if (rc == 0) {
	rc = TPM_Key_StoreClear(sbuffer, TRUE, &(tpm_permanent_data->endorsementKey));	      
    }
    /* store srk */
    if (rc == 0) {
	rc = TPM_Key_StoreClear(sbuffer, FALSE, &(tpm_permanent_data->srk));	       
    }
    /* store contextKey */
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_Store(sbuffer, tpm_permanent_data->contextKey);
    }
    /* store delegateKey */
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_Store(sbuffer, tpm_permanent_data->delegateKey);	
    }
    /* store auditMonotonicCounter */
    if (rc == 0)  {
	rc = TPM_CounterValue_Store(sbuffer, &(tpm_permanent_data->auditMonotonicCounter));
    }
    /* store monotonicCounter */
    if (rc == 0)  {
	rc = TPM_Counters_Store(sbuffer, tpm_permanent_data->monotonicCounter);
    }
    /* store pcrAttrib, since they are constants, no need to store to NV space */
    /* store ordinalAuditStatus */
    for (i = 0 ; (rc == 0) && (i < (TPM_ORDINALS_MAX/CHAR_BIT)) ; i++) {
	rc = TPM_Sbuffer_Append(sbuffer,
				&(tpm_permanent_data->ordinalAuditStatus[i]), sizeof(BYTE));
    }
    /* store familyTable */
    if (rc == 0) {
	rc = TPM_FamilyTable_Store(sbuffer,
				   &(tpm_permanent_data->familyTable),
				   FALSE);	/* don't store the tag, to save NV space */
    }
    /* store delegateTable */
    if (rc == 0) {
	rc = TPM_DelegateTable_Store(sbuffer, &(tpm_permanent_data->delegateTable));	
    }
    /* store lastFamilyID */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_permanent_data->lastFamilyID);
    }
    /* store noOwnerNVWrite */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_permanent_data->noOwnerNVWrite);
    }
    /* store restrictDelegate */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_permanent_data->restrictDelegate);	
    }
    /* store tpmDAASeed */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_permanent_data->tpmDAASeed);
    }
    /* store ownerInstalled */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_data->ownerInstalled), sizeof(BYTE));
    }
    /* store tscOrdinalAuditStatus */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_data->tscOrdinalAuditStatus),
				sizeof(BYTE));
    }
    /* store allowLoadMaintPub */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_permanent_data->allowLoadMaintPub), sizeof(BYTE));
    }
    /* store daaProof */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_permanent_data->daaProof);
    }
    /* store daaBlobKey */
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_Store(sbuffer, tpm_permanent_data->daaBlobKey);
    }
    instanceData = instanceData;	/* to quiet the compiler */
    return rc;
}

/* TPM_PermanentData_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_PermanentData_Zero to zero secrets that are not Delete'd
   The object itself is not freed
*/   

void TPM_PermanentData_Delete(TPM_PERMANENT_DATA *tpm_permanent_data,
			      TPM_BOOL instanceData)
{
    printf(" TPM_PermanentData_Delete:\n");
    if (tpm_permanent_data != NULL) {
#ifndef TPM_NOMAINTENANCE
	TPM_Pubkey_Delete(&(tpm_permanent_data->manuMaintPub)); 
#endif
	TPM_Key_Delete(&(tpm_permanent_data->endorsementKey));
	TPM_Key_Delete(&(tpm_permanent_data->srk));
	TPM_SymmetricKeyData_Free(&(tpm_permanent_data->contextKey));
	TPM_SymmetricKeyData_Free(&(tpm_permanent_data->delegateKey));
	TPM_FamilyTable_Delete(&(tpm_permanent_data->familyTable));
	TPM_DelegateTable_Delete(&(tpm_permanent_data->delegateTable)); 
	TPM_SymmetricKeyData_Free(&(tpm_permanent_data->daaBlobKey));
	/* zero all secrets */
	TPM_PermanentData_Zero(tpm_permanent_data, instanceData);
    }
    return;
}

/* TPM_PermanentData_Zero() zeros all secrets not already zeroed and freed by
   TPM_PermanentData_Delete()

   It is called by TPM_PermanentData_Delete() and TPM_PermanentData_Init().  It does a subset of
   TPM_PermanentData_Init() that will never fail.
*/

void TPM_PermanentData_Zero(TPM_PERMANENT_DATA *tpm_permanent_data,
			    TPM_BOOL instanceData)
{
    printf("  TPM_PermanentData_Zero:\n");
    instanceData = instanceData;
    if (tpm_permanent_data != NULL) {
	TPM_Secret_Init(tpm_permanent_data->tpmProof);
	TPM_Nonce_Init(tpm_permanent_data->EKReset);
	TPM_Secret_Init(tpm_permanent_data->ownerAuth);
	TPM_Secret_Init(tpm_permanent_data->operatorAuth);
	TPM_Digest_Init(tpm_permanent_data->authDIR);
	/* endorsementKey handled by TPM_Key_Delete() */
	/* srk handled by TPM_Key_Delete() */
	/* contextKey handled by TPM_SymmetricKeyData_Free() */
	/* delegateKey handled by TPM_SymmetricKeyData_Free() */
	TPM_Nonce_Init(tpm_permanent_data->tpmDAASeed);
	TPM_Nonce_Init(tpm_permanent_data->daaProof);
	/* daaBlobKey handled by TPM_SymmetricKeyData_Free() */
    }
    return;
}

/* TPM_PermanentData_InitDaa() generates new values for the 3 DAA elements: tpmDAASeed, daaProof,
   and daaBlobKey.

   This is common code, use when creating the EK, revoke trust, and the set capability
   used by the owner to invalidate DAA blobs.
*/

TPM_RESULT TPM_PermanentData_InitDaa(TPM_PERMANENT_DATA *tpm_permanent_data)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_PermanentData_InitDaa:\n");
    /* generate tpmDAASeed */
    if (rc == 0) {
	rc = TPM_Nonce_Generate(tpm_permanent_data->tpmDAASeed);
    }
    /* generate daaProof*/
    if (rc == 0) {
	rc = TPM_Nonce_Generate(tpm_permanent_data->daaProof);
    }
    /* generate daaBlobKey */
    if (rc == 0) {
	rc = TPM_SymmetricKeyData_GenerateKey(tpm_permanent_data->daaBlobKey);
    }
    return rc;
}

/*
  PermanentAll is TPM_PERMANENT_DATA, TPM_PERMANENT_FLAGS, owner evict keys, and NV defined space.
*/

/* TPM_PermanentAll_Load() deserializes all TPM NV data from a stream created by
   TPM_PermanentAll_Store().

   The two functions must be kept in sync.

   Data includes TPM_PERMANENT_DATA, TPM_PERMANENT_FLAGS, Owner Evict keys, and NV defined space.
*/

TPM_RESULT TPM_PermanentAll_Load(tpm_state_t *tpm_state,
				 unsigned char **stream,
				 uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    unsigned char	*stream_start = *stream;	/* copy for integrity check */		
    uint32_t		stream_size_start = *stream_size;
    
    printf(" TPM_PermanentAll_Load:\n");
    /* check format tag */
    /* In the future, if multiple formats are supported, this check will be replaced by a 'switch'
       on the tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_NVSTATE_V1, stream, stream_size);
    }
    /* TPM_PERMANENT_DATA deserialize from stream */
    if (rc == 0) {
	rc = TPM_PermanentData_Load(&(tpm_state->tpm_permanent_data),
				    stream, stream_size, TRUE);
    }
    /* TPM_PERMANENT_FLAGS deserialize from stream */
    if (rc == 0) {
	rc = TPM_PermanentFlags_Load(&(tpm_state->tpm_permanent_flags),
				     stream, stream_size);
    }
    /* owner evict keys deserialize from stream */
    if (rc == 0) {
	rc = TPM_KeyHandleEntries_OwnerEvictLoad(tpm_state->tpm_key_handle_entries,
						 stream, stream_size);
    }
    /* NV defined space deserialize from stream */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_Load(&(tpm_state->tpm_nv_index_entries),
				     stream, stream_size);
    }
    /* sanity check the stream size */
    if (rc == 0) {
	if (*stream_size != TPM_DIGEST_SIZE) {
	    printf("TPM_PermanentAll_Load: Error (fatal) stream size %u not %u\n",
		   *stream_size, TPM_DIGEST_SIZE);
	    rc = TPM_FAIL;
	}
    }
    /* check the integrity digest */
    if (rc == 0) {
	printf("  TPM_PermanentAll_Load: Checking integrity digest\n");
	rc = TPM_SHA1_Check(*stream, 	/* currently points to integrity digest */
			    stream_size_start - TPM_DIGEST_SIZE, stream_start,
			    0, NULL);
    }
    /* remove the integrity digest from the stream */
    if (rc == 0) {
	*stream_size -= TPM_DIGEST_SIZE;
    }
    return rc;
}

/* TPM_PermanentAll_Store() serializes all TPM NV data into a stream that can be restored through
   TPM_PermanentAll_Load().

   The two functions must be kept in sync.

   Data includes TPM_PERMANENT_DATA, TPM_PERMANENT_FLAGS, Owner Evict keys, and NV defined space.

   The TPM_STORE_BUFFER, buffer and length are returned for convenience.

   This has two uses:

   - It is called before the actual NV store to serialize the data
   - It is called by TPM_NV_DefineSpace to determine if there is enough NV space for the new index
*/

TPM_RESULT TPM_PermanentAll_Store(TPM_STORE_BUFFER *sbuffer,	/* freed by caller */
				  const unsigned char **buffer,
				  uint32_t *length,
				  tpm_state_t *tpm_state)
{
    TPM_RESULT		rc = 0;
    TPM_DIGEST		tpm_digest;

    printf(" TPM_PermanentAll_Store:\n");
    /* overall format tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_NVSTATE_V1);
    }
    /* serialize TPM_PERMANENT_DATA  */
    if (rc == 0) {
	rc = TPM_PermanentData_Store(sbuffer,
				     &(tpm_state->tpm_permanent_data), TRUE);
    }
    /* serialize TPM_PERMANENT_FLAGS */
    if (rc == 0) {
	rc = TPM_PermanentFlags_Store(sbuffer,
				      &(tpm_state->tpm_permanent_flags));
    }
    /* serialize owner evict keys */
    if (rc == 0) {
	rc = TPM_KeyHandleEntries_OwnerEvictStore(sbuffer,
						  tpm_state->tpm_key_handle_entries);
    }
    /* serialize NV defined space */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_Store(sbuffer,
				      &(tpm_state->tpm_nv_index_entries));
    }
    if (rc == 0) {
	/* get the current serialized buffer and its length */
	TPM_Sbuffer_Get(sbuffer, buffer, length);
	/* generate the integrity digest */
	rc = TPM_SHA1(tpm_digest,
		      *length, *buffer,
		      0, NULL);
    }
    /* append the integrity digest to the stream */
    if (rc == 0) {
	printf(" TPM_PermanentAll_Store: Appending integrity digest\n");
	rc = TPM_Sbuffer_Append(sbuffer, tpm_digest, TPM_DIGEST_SIZE);
    }
    /* get the final serialized buffer and its length */
    if (rc == 0) {
	TPM_Sbuffer_Get(sbuffer, buffer, length);
    }
    return rc;
}

/* TPM_PermanentAll_NVLoad()

   Deserialize the TPM_PERMANENT_DATA, TPM_PERMANENT_FLAGS, owner evict keys, and NV defined
   space from a stream read from the NV file TPM_PERMANENT_ALL_NAME.

   Returns:

   0 success
   TPM_RETRY if file does not exist (first time)
   TPM_FAIL on failure to load (fatal), since they should never occur
*/

TPM_RESULT TPM_PermanentAll_NVLoad(tpm_state_t *tpm_state)
{
    TPM_RESULT		rc = 0;
    unsigned char	*stream = NULL;
    unsigned char	*stream_start = NULL;
    uint32_t		stream_size;

    printf(" TPM_PermanentAll_NVLoad:\n");
    if (rc == 0) {
	/* try loading from NVRAM */
	/* Returns TPM_RETRY on non-existent file */
	rc = TPM_NVRAM_LoadData(&stream,		/* freed @1 */
				&stream_size,
				tpm_state->tpm_number,
				TPM_PERMANENT_ALL_NAME);
    }
    /* deserialize from stream */
    if (rc == 0) {
	stream_start = stream;			/* save starting point for free() */
	rc = TPM_PermanentAll_Load(tpm_state, &stream, &stream_size);
	if (rc != 0) {
	    printf("TPM_PermanentAll_NVLoad: Error (fatal) loading deserializing NV state\n");
	    rc = TPM_FAIL;
	}
    }
    free(stream_start); /* @1 */
    return rc;
}

/* TPM_PermanentAll_NVStore() serializes all NV data and stores it in the NV file
   TPM_PERMANENT_ALL_NAME

   If the writeAllNV flag is FALSE, the function is a no-op, and returns the input 'rcIn'.

   If writeAllNV is TRUE and rcIn is not TPM_SUCCESS, this indicates that the ordinal
   modified the in-memory TPM_PERMANENT_DATA and/or TPM_PERMANENT_FLAGS structures (perhaps only
   partially) and then detected an error.  Since the command is failing, roll back the structure by
   reading the NV file.	 If the read then fails, this is a fatal error.

   Similarly, if writeAllNV is TRUE and the actual NV write fails, this is a fatal error.
*/

TPM_RESULT TPM_PermanentAll_NVStore(tpm_state_t *tpm_state,
				    TPM_BOOL writeAllNV,
				    TPM_RESULT rcIn)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;	/* safe buffer for storing binary data */
    const unsigned char *buffer;
    uint32_t		length;
    TPM_NV_DATA_ST 	*tpm_nv_data_st = NULL;	/* array of saved NV index volatile flags */ 

    printf(" TPM_PermanentAll_NVStore: write flag %u\n", writeAllNV);
    TPM_Sbuffer_Init(&sbuffer);			/* freed @1 */
    if (writeAllNV) {
	if (rcIn == TPM_SUCCESS) {
	    /* serialize state to be written to NV */
	    if (rc == 0) {
		rc = TPM_PermanentAll_Store(&sbuffer,
					    &buffer, &length,
					    tpm_state);
	    }
	    /* validate the length of the stream against the maximum provided NV space */
	    if (rc == 0) {
		printf("   TPM_PermanentAll_NVStore: Require %u bytes\n", length);
		if (length > TPM_MAX_NV_SPACE) {
		    printf("TPM_PermanentAll_NVStore: Error, No space, need %u max %u\n",
			   length, TPM_MAX_NV_SPACE);
		    rc = TPM_NOSPACE;
		}
	    }
	    /* store the buffer in NVRAM */
	    if (rc == 0) {
		rc = TPM_NVRAM_StoreData(buffer,
					 length,
					 tpm_state->tpm_number,
					 TPM_PERMANENT_ALL_NAME); 
	    }
	    if (rc != 0) {
		printf("TPM_PermanentAll_NVStore: Error (fatal), "
		       "NV structure in-memory caches are in invalid state\n");
		rc = TPM_FAIL;
	    }
	}
	else {	
	    /* An in-memory structure was altered, but the ordinal had a subsequent error.  Since
	       the structure is in an invalid state, roll back to the previous value by reading the
	       NV file. */
	    printf("  TPM_PermanentAll_NVStore: Ordinal error, "
		   "rolling back NV structure cache\n");
	    /* Save a copy of the NV defined space volatile state.  It is not stored in NV, so it
	       will be destroyed during the rollback. */
	    /* get a copy of the NV volatile flags, to be used during a rollback */
	    if (rc == 0) {
		rc = TPM_NVIndexEntries_GetVolatile(&tpm_nv_data_st,	/* freed @2 */
						    &(tpm_state->tpm_nv_index_entries));
	    }
	    /* Returns TPM_RETRY on non-existent file */
	    if (rc == 0) {
		printf(" TPM_PermanentAllNVStore: Deleting TPM_PERMANENT_DATA structure\n");
		TPM_PermanentData_Delete(&(tpm_state->tpm_permanent_data), TRUE);
		printf(" TPM_PermanentAllNVStore: Deleting owner evict keys\n");
		TPM_KeyHandleEntries_OwnerEvictDelete(tpm_state->tpm_key_handle_entries);
		printf(" TPM_PermanentAllNVStore: Deleting NV defined space \n");
		TPM_NVIndexEntries_Delete(&(tpm_state->tpm_nv_index_entries));
		printf(" TPM_PermanentAllNVStore: "
		       "Rereading TPM_PERMANENT_DATA, TPM_PERMANENT_FLAGS, owner evict keys\n");
		/* re-allocate TPM_PERMANENT_DATA data structures */
		rc = TPM_PermanentData_Init(&(tpm_state->tpm_permanent_data), TRUE);
	    }
	    if (rc == 0) {
		rc = TPM_PermanentAll_NVLoad(tpm_state);
	    }
	    if (rc == 0) {
		rc = TPM_NVIndexEntries_SetVolatile(tpm_nv_data_st,
						    &(tpm_state->tpm_nv_index_entries));
	    }
	    /* after a successful rollback, return the ordinal's original error code */
	    if (rc == 0) {
		rc = rcIn;
	    }
	    /* a failure during rollback is fatal */
	    else {
		printf("TPM_PermanentAll_NVStore: Error (fatal), "
		       "Permanent Data, Flags, or owner evict keys structure is invalid\n");
		rc = TPM_FAIL;
	    }
		
	}
    }
    /* no write required, no-op */
    else {
	rc = rcIn;
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    free(tpm_nv_data_st);		/* @2 */
    return rc;
}

/* TPM_PermanentAll_NVDelete() deletes ann NV data in the NV file TPM_PERMANENT_ALL_NAME.

   If mustExist is TRUE, returns an error if the file does not exist.
   
   It does not delete the in-memory copy.
*/

TPM_RESULT TPM_PermanentAll_NVDelete(uint32_t tpm_number,
				     TPM_BOOL mustExist)
{
    TPM_RESULT		rc = 0;
    
    printf(" TPM_PermanentAll_NVDelete:\n");
    /* remove the NVRAM file */
    if (rc == 0) {
	rc = TPM_NVRAM_DeleteName(tpm_number,
				  TPM_PERMANENT_ALL_NAME,
				  mustExist);
    }
    return rc;
}

/* TPM_PermanentAll_IsSpace() determines if there is enough NV space for the serialized NV state.

   It does this by serializing the entire state and comparing the length to the configured maximum.
*/

TPM_RESULT TPM_PermanentAll_IsSpace(tpm_state_t *tpm_state)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;	/* safe buffer for storing binary data */
    const unsigned char *buffer;
    uint32_t		length;
    
    printf("TPM_PermanentAll_IsSpace :\n");
    TPM_Sbuffer_Init(&sbuffer);		/* freed @1 */
    if (rc == 0) {
	rc = TPM_PermanentAll_Store(&sbuffer,
				    &buffer, &length,
				    tpm_state);
    }
    if (rc == 0) {
	printf("  TPM_PermanentAll_IsSpace: Require %u bytes\n", length);
	if (length > TPM_MAX_NV_SPACE) {
	    printf("TPM_PermanentAll_IsSpace: No space, need %u max %u\n",
		   length, TPM_MAX_NV_SPACE);
	    rc = TPM_NOSPACE;
	}
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/* TPM_PermanentAll_GetSpace() returns the NV free space.

   It does this by serializing the entire state and comparing the length to the configured maximum.
*/

TPM_RESULT TPM_PermanentAll_GetSpace(uint32_t *bytes_free,
				     tpm_state_t *tpm_state)
{
    TPM_RESULT		rc = 0;
    TPM_STORE_BUFFER	sbuffer;	/* safe buffer for storing binary data */
    const unsigned char *buffer;
    uint32_t		length;
    
    printf(" TPM_NVRAM_IsSpace:\n");
    TPM_Sbuffer_Init(&sbuffer);		/* freed @1 */
    if (rc == 0) {
	rc = TPM_PermanentAll_Store(&sbuffer,
				    &buffer, &length,
				    tpm_state);
    }
    if (rc == 0) {
	printf("  TPM_PermanentAll_GetSpace: Used %u max %u bytes\n", length, TPM_MAX_NV_SPACE);
	if (length > TPM_MAX_NV_SPACE) {
	    /* This should never occur */
	    printf("TPM_PermanentAll_GetSpace: Error (fatal) Used more than maximum\n");
	    rc = TPM_FAIL;
	}
    }	    
    if (rc == 0) {
	*bytes_free = TPM_MAX_NV_SPACE - length;
	printf("  TPM_PermanentAll_GetSpace: Free space %u\n", *bytes_free);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

