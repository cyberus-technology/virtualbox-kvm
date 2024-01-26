/********************************************************************************/
/*										*/
/*				NVRAM Utilities					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_nvram.c $		*/
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
#include <errno.h>

#include "tpm_auth.h"
#include "tpm_crypto.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_io.h"
#include "tpm_memory.h"
#include "tpm_nvfile.h"
#include "tpm_pcr.h"
#include "tpm_permanent.h"
#include "tpm_platform.h"
#include "tpm_process.h"
#include "tpm_secret.h"
#include "tpm_storage.h"
#include "tpm_structures.h"

#include "tpm_nvram.h"

/*
  NV Defined Space Utilities
*/

/*
  TPM_NV_ATTRIBUTES
*/

/* TPM_NVAttributes_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_NVAttributes_Init(TPM_NV_ATTRIBUTES *tpm_nv_attributes)
{
    printf(" TPM_NVAttributes_Init:\n");
    tpm_nv_attributes->attributes = 0;
    return;
}

/* TPM_NVAttributes_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_NVAttributes_Init()
   After use, call TPM_NVAttributes_Delete() to free memory
*/

TPM_RESULT TPM_NVAttributes_Load(TPM_NV_ATTRIBUTES *tpm_nv_attributes,
				 unsigned char **stream,
				 uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_NVAttributes_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_NV_ATTRIBUTES, stream, stream_size);
    }
    /* load attributes */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_nv_attributes->attributes), stream, stream_size);
    }
    return rc;
}

/* TPM_NVAttributes_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_NVAttributes_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_NV_ATTRIBUTES *tpm_nv_attributes)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_NVAttributes_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_NV_ATTRIBUTES);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_nv_attributes->attributes);
    }
    return rc;
}

/* TPM_NVAttributes_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the nv_attributes
   sets pointers to NULL
   calls TPM_NVAttributes_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_NVAttributes_Delete(TPM_NV_ATTRIBUTES *tpm_nv_attributes)
{
    printf(" TPM_NVAttributes_Delete:\n");
    if (tpm_nv_attributes != NULL) {
	TPM_NVAttributes_Init(tpm_nv_attributes);
    }
    return;
}

void TPM_NVAttributes_Copy(TPM_NV_ATTRIBUTES *tpm_nv_attributes_dest,
			   TPM_NV_ATTRIBUTES *tpm_nv_attributes_src)
{
    tpm_nv_attributes_dest->attributes = tpm_nv_attributes_src->attributes;
    return;
}

/*
  TPM_NV_DATA_PUBLIC
*/

/* TPM_NVDataPublic_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_NVDataPublic_Init(TPM_NV_DATA_PUBLIC *tpm_nv_data_public)
{
    printf(" TPM_NVDataPublic_Init:\n");
    tpm_nv_data_public->nvIndex = TPM_NV_INDEX_LOCK;	/* mark unused */
    TPM_PCRInfoShort_Init(&(tpm_nv_data_public->pcrInfoRead));
    TPM_PCRInfoShort_Init(&(tpm_nv_data_public->pcrInfoWrite));
    TPM_NVAttributes_Init(&(tpm_nv_data_public->permission));
    tpm_nv_data_public->bReadSTClear = FALSE;
    tpm_nv_data_public->bWriteSTClear = FALSE;
    tpm_nv_data_public->bWriteDefine = FALSE; 
    tpm_nv_data_public->dataSize = 0;
    return;
}

/* TPM_NVDataPublic_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_NVDataPublic_Init()
   After use, call TPM_NVDataPublic_Delete() to free memory
*/

TPM_RESULT TPM_NVDataPublic_Load(TPM_NV_DATA_PUBLIC *tpm_nv_data_public,
				 unsigned char **stream,
				 uint32_t *stream_size,
				 TPM_BOOL optimize)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_NVDataPublic_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_NV_DATA_PUBLIC, stream, stream_size);
    }
    /* load nvIndex */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_nv_data_public->nvIndex), stream, stream_size);
    }
    /* load pcrInfoRead */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_Load(&(tpm_nv_data_public->pcrInfoRead), stream, stream_size, optimize);
    }
    /* load pcrInfoWrite */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_Load(&(tpm_nv_data_public->pcrInfoWrite), stream, stream_size, optimize);
    }
    /* load permission */
    if (rc == 0) {
	rc = TPM_NVAttributes_Load(&(tpm_nv_data_public->permission), stream, stream_size);
    }
    /* load bReadSTClear */
    if (rc == 0) {
	rc = TPM_LoadBool(&(tpm_nv_data_public->bReadSTClear), stream, stream_size);
    }
    /* load bWriteSTClear */
    if (rc == 0) {
	rc = TPM_LoadBool(&(tpm_nv_data_public->bWriteSTClear), stream, stream_size);
    }
    /* load bWriteDefine */
    if (rc == 0) {
	rc = TPM_LoadBool(&(tpm_nv_data_public->bWriteDefine), stream, stream_size);
    }
    /* load dataSize */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_nv_data_public->dataSize), stream, stream_size);
    }
    return rc;
}

/* TPM_NVDataPublic_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_NVDataPublic_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_NV_DATA_PUBLIC *tpm_nv_data_public,
				  TPM_BOOL optimize)
{	
    TPM_RESULT		rc = 0;

    printf(" TPM_NVDataPublic_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_NV_DATA_PUBLIC);
    }
    /* store nvIndex */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_nv_data_public->nvIndex);
    }
    /* store pcrInfoRead */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_Store(sbuffer, &(tpm_nv_data_public->pcrInfoRead), optimize);
    }
    /* store pcrInfoWrite */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_Store(sbuffer, &(tpm_nv_data_public->pcrInfoWrite), optimize);
    }
    /* store permission */
    if (rc == 0) {
	rc = TPM_NVAttributes_Store(sbuffer, &(tpm_nv_data_public->permission));
    }
    /* store bReadSTClear */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_nv_data_public->bReadSTClear), sizeof(TPM_BOOL));
    }
    /* store bWriteSTClear */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_nv_data_public->bWriteSTClear), sizeof(TPM_BOOL));
    }
    /* store bWriteDefine */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_nv_data_public->bWriteDefine), sizeof(TPM_BOOL));
    }
    /* store dataSize */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, tpm_nv_data_public->dataSize);
    }
    return rc;
}

/* TPM_NVDataPublic_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_NVDataPublic_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_NVDataPublic_Delete(TPM_NV_DATA_PUBLIC *tpm_nv_data_public)
{
    printf(" TPM_NVDataPublic_Delete:\n");
    if (tpm_nv_data_public != NULL) {
	TPM_PCRInfoShort_Delete(&(tpm_nv_data_public->pcrInfoRead));
	TPM_PCRInfoShort_Delete(&(tpm_nv_data_public->pcrInfoWrite));
	TPM_NVAttributes_Delete(&(tpm_nv_data_public->permission));
	TPM_NVDataPublic_Init(tpm_nv_data_public);
    }
    return;
}

/*
  TPM_NV_DATA_SENSITIVE
*/

/* TPM_NVDataSensitive_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_NVDataSensitive_Init(TPM_NV_DATA_SENSITIVE *tpm_nv_data_sensitive)
{
    printf(" TPM_NVDataSensitive_Init:\n");
    TPM_NVDataPublic_Init(&(tpm_nv_data_sensitive->pubInfo));
    TPM_Secret_Init(tpm_nv_data_sensitive->authValue);
    tpm_nv_data_sensitive->data = NULL;
    TPM_Digest_Init(tpm_nv_data_sensitive->digest);
    return;
}

/* TPM_NVDataSensitive_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_NVDataSensitive_Init()
   After use, call TPM_NVDataSensitive_Delete() to free memory
*/

TPM_RESULT TPM_NVDataSensitive_Load(TPM_NV_DATA_SENSITIVE *tpm_nv_data_sensitive,
				    TPM_TAG nvEntriesVersion,
				    unsigned char **stream,
				    uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    TPM_BOOL 		optimize;
    TPM_BOOL		isGPIO;

    printf(" TPM_NVDataSensitive_Load: nvEntriesVersion %04hx\n", nvEntriesVersion);
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_NV_DATA_SENSITIVE, stream, stream_size);
    }
    /* load pubInfo */
    if (rc == 0) {
	/* versions after V1 optimise the serialization */
	optimize = (nvEntriesVersion != TPM_TAG_NVSTATE_NV_V1);
	rc = TPM_NVDataPublic_Load(&(tpm_nv_data_sensitive->pubInfo),
				   stream, stream_size,
				   optimize);	/* optimize digestAtRelease */
    }
    /* load authValue */
    if (rc == 0) {
	rc = TPM_Secret_Load(tpm_nv_data_sensitive->authValue, stream, stream_size);
    }
    /* is the nvIndex GPIO space */
    if (rc == 0) {
	rc = TPM_NVDataSensitive_IsGPIO(&isGPIO, tpm_nv_data_sensitive->pubInfo.nvIndex);
    }
    /* allocate memory for data */
    if ((rc == 0) && !isGPIO) {
	rc = TPM_Malloc(&(tpm_nv_data_sensitive->data),
			tpm_nv_data_sensitive->pubInfo.dataSize);
    }
    /* load data */
    if ((rc == 0) && !isGPIO) {
	rc = TPM_Loadn(tpm_nv_data_sensitive->data, tpm_nv_data_sensitive->pubInfo.dataSize,
		       stream, stream_size);
    }
    /* create digest.  The digest is not stored to save NVRAM space */
    if (rc == 0) {
	rc = TPM_SHA1(tpm_nv_data_sensitive->digest,
		      sizeof(TPM_NV_INDEX),
		      (unsigned char *)&tpm_nv_data_sensitive->pubInfo.nvIndex, 
		      TPM_AUTHDATA_SIZE, tpm_nv_data_sensitive->authValue,
		      0, NULL);
    }
    return rc;
}

/* TPM_NVDataSensitive_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes

   nvWrite TRUE indicates a write command, not a command to define the space.
*/

TPM_RESULT TPM_NVDataSensitive_Store(TPM_STORE_BUFFER *sbuffer,
				     const TPM_NV_DATA_SENSITIVE *tpm_nv_data_sensitive)
{
    TPM_RESULT		rc = 0;
    TPM_BOOL		isGPIO;

    printf(" TPM_NVDataSensitive_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_NV_DATA_SENSITIVE);
    }
    /* store pubInfo */
    if (rc == 0) {
	rc = TPM_NVDataPublic_Store(sbuffer, &(tpm_nv_data_sensitive->pubInfo),
				    TRUE);	/* optimize digestAtRelease */
    }
    /* store authValue */
    if (rc == 0) {
	rc = TPM_Secret_Store(sbuffer, tpm_nv_data_sensitive->authValue);
    }
    /* is the nvIndex GPIO space */
    if (rc == 0) {
	rc = TPM_NVDataSensitive_IsGPIO(&isGPIO, tpm_nv_data_sensitive->pubInfo.nvIndex);
    }
    /* store data */
    if ((rc == 0) && !isGPIO) {
	rc = TPM_Sbuffer_Append(sbuffer, tpm_nv_data_sensitive->data,
				tpm_nv_data_sensitive->pubInfo.dataSize);
    }
    return rc;
}

/* TPM_NVDataSensitive_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_NVDataSensitive_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_NVDataSensitive_Delete(TPM_NV_DATA_SENSITIVE *tpm_nv_data_sensitive)
{
    printf(" TPM_NVDataSensitive_Delete:\n");
    if (tpm_nv_data_sensitive != NULL) {
	/* zero any secrets in NV index data */
	if (tpm_nv_data_sensitive->data != NULL) {
	    memset(tpm_nv_data_sensitive->data, 0xff, tpm_nv_data_sensitive->pubInfo.dataSize);
	}
	TPM_NVDataPublic_Delete(&(tpm_nv_data_sensitive->pubInfo));
	TPM_Secret_Delete(tpm_nv_data_sensitive->authValue);
	free(tpm_nv_data_sensitive->data);
	TPM_NVDataSensitive_Init(tpm_nv_data_sensitive);
    }
    return;
}

/* TPM_NVDataSensitive_IsValidIndex() determines if 'nvIndex' is permissible for an NV defined space
   TPM_NV_DATA_SENSITIVE structure.

   Some values have special meaning, so they are allowed for the TPM_NV_DefineSpace command but will
   not actually define a space.
*/

TPM_RESULT TPM_NVDataSensitive_IsValidIndex(TPM_NV_INDEX nvIndex)
{
    TPM_RESULT		rc = 0;
    TPM_BOOL		isGPIO;

    printf(" TPM_NVDataSensitive_IsValidIndex: nvIndex %08x\n", nvIndex);
    if (rc == 0) {
	if ((nvIndex == TPM_NV_INDEX_LOCK) ||
	    (nvIndex == TPM_NV_INDEX0) ||
	    (nvIndex == TPM_NV_INDEX_DIR)) {
	    printf("TPM_NVDataSensitive_IsValidIndex: Error, illegal special index\n");
	    rc = TPM_BADINDEX;
	}
    }
    if (rc == 0) {
	if ((nvIndex & TPM_NV_INDEX_RESVD) != 0) {
	    printf("TPM_NVDataSensitive_IsValidIndex: Error, illegal reserved index\n");
	    rc = TPM_BADINDEX;
	}
    }
    if (rc == 0) {
	rc = TPM_NVDataSensitive_IsValidPlatformIndex(nvIndex);
    }
    /* The GPIO range validity is platform dependent */
    if (rc == 0) {
	rc = TPM_NVDataSensitive_IsGPIO(&isGPIO, nvIndex);
    }
    return rc;
}

/* TPM_NVDataSensitive_IsGPIO() determines if 'nvIndex' is in the GPIO range and is valid.

   Returns:

   TPM_SUCCESS , FALSE if 'nvIndex' is not in the GPIO range
   TPM_SUCCESS , TRUE  if 'nvIndex' is in the GPIO range and the platform allows GPIO defined space
   TPM_BADINDEX, FALSE if 'nvIndex' is in the GPIO range and the platform does not allow GPIO
	defined space
*/

TPM_RESULT TPM_NVDataSensitive_IsGPIO(TPM_BOOL *isGPIO, TPM_NV_INDEX nvIndex)
{
    TPM_RESULT		rc = 0;

    printf("  TPM_NVDataSensitive_IsGPIO: nvIndex %08x\n", nvIndex);
    *isGPIO = FALSE;
#if defined TPM_PCCLIENT
    if (rc == 0) {
	/* GPIO space allowed for PC Client */
	if ((nvIndex >= TPM_NV_INDEX_GPIO_START) &&
	    (nvIndex <= TPM_NV_INDEX_GPIO_END)) {
	    printf("   TPM_NVDataSensitive_IsGPIO: nvIndex is GPIO space\n");
	    *isGPIO = TRUE;
	}	
    }
    /* #elif */
#else
    if (rc == 0) {
	/* GPIO space cannot be defined in platforms with no GPIO */
	if ((nvIndex >= TPM_NV_INDEX_GPIO_START) &&
	    (nvIndex <= TPM_NV_INDEX_GPIO_END)) {
	    printf("TPM_NVDataSensitive_IsGPIO: Error, illegal index\n");
	    rc = TPM_BADINDEX;
	}	
    }
#endif
    return rc;
} 

TPM_RESULT TPM_NVDataSensitive_IsValidPlatformIndex(TPM_NV_INDEX nvIndex)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_NVDataSensitive_IsValidPlatformIndex: nvIndex %08x\n", nvIndex);
#ifndef TPM_PCCLIENT
    if (rc == 0) {
	if (((nvIndex & TPM_NV_INDEX_PURVIEW_MASK) >> TPM_NV_INDEX_PURVIEW_BIT) == TPM_PC) {
	    printf("  TPM_NVDataSensitive_IsValidPlatformIndex: Error, PC Client index\n");
	    rc = TPM_BADINDEX;
	}
    }
#endif 
    return rc;
}

/*
  NV Index Entries

  This handles the in-memory copy of NV defined space
*/

/*
  TPM_NVIndexEntries_Init() initializes the TPM_NV_INDEX_ENTRIES array
*/

void TPM_NVIndexEntries_Init(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    printf(" TPM_NVIndexEntries_Init:\n");
    tpm_nv_index_entries->nvIndexCount = 0;
    tpm_nv_index_entries->tpm_nvindex_entry = NULL;
    return;
}

/*
  TPM_NVIndexEntries_Delete() iterates through the entire TPM_NV_INDEX_ENTRIES array, deleting any
  used entries.

  It then frees and reinitializes the array.
*/


void TPM_NVIndexEntries_Delete(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    size_t i;

    printf(" TPM_NVIndexEntries_Delete: Deleting from %u slots\n",
	   tpm_nv_index_entries->nvIndexCount);
    /* free the entries */
    for (i = 0 ; i < tpm_nv_index_entries->nvIndexCount ; i++) {
	TPM_NVDataSensitive_Delete(&(tpm_nv_index_entries->tpm_nvindex_entry[i]));
    }
    /* free the array */
    free(tpm_nv_index_entries->tpm_nvindex_entry);
    TPM_NVIndexEntries_Init(tpm_nv_index_entries);
    return;
}

/* TPM_NVIndexEntries_Trace() traces the TPM_NV_INDEX_ENTRIES array.

   Edit and call as required for debugging.
*/

void TPM_NVIndexEntries_Trace(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    uint32_t	i;
    TPM_NV_DATA_SENSITIVE *tpm_nv_data_sensitive;
    
    printf("\tTPM_NVIndexEntries_Trace: %u slots\n", tpm_nv_index_entries->nvIndexCount);
    for (i = 0 ; i < tpm_nv_index_entries->nvIndexCount ; i++) {
	tpm_nv_data_sensitive = &(tpm_nv_index_entries->tpm_nvindex_entry[i]);
	printf("\tTPM_NVIndexEntries_Trace: TPM_NV_DATA_SENSITIVE.data %p\n",
	       tpm_nv_data_sensitive->data);
    }
    return;
}

/*
  TPM_NVIndexEntries_Load() loads the TPM_NV_INDEX_ENTRIES array from a stream.

  The first data in the stream must be a uint32_t count of the number of entries to follow.
*/

TPM_RESULT TPM_NVIndexEntries_Load(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries,
				   unsigned char **stream,
				   uint32_t *stream_size)
{
    TPM_RESULT 	rc = 0;
    uint32_t	i;
    TPM_TAG	nvEntriesVersion;

    printf(" TPM_NVIndexEntries_Load:\n");
    /* get the NV entries version number */
    if (rc == 0) {
	rc = TPM_Load16(&nvEntriesVersion, stream, stream_size); 
    }
    /* check tag */
    if (rc == 0) {
	switch (nvEntriesVersion) {
	  case TPM_TAG_NVSTATE_NV_V1:
	  case TPM_TAG_NVSTATE_NV_V2:
	    break;
	  default:
            printf("TPM_NVIndexEntries_Load: Error (fatal), version %04x unsupported\n",
		   nvEntriesVersion);
            rc = TPM_FAIL;
	    break;
	}
    }
    /* nvIndexCount */
    if (rc == 0) {
	rc = TPM_Load32(&(tpm_nv_index_entries->nvIndexCount), stream, stream_size); 
    }
    /* allocate memory for the array, nvIndexCount TPM_NV_DATA_SENSITIVE structures */
    if ((rc == 0) && (tpm_nv_index_entries->nvIndexCount > 0)) {
	printf("  TPM_NVIndexEntries_Load: Loading %u slots\n", tpm_nv_index_entries->nvIndexCount);
	rc = TPM_Malloc((unsigned char **)&(tpm_nv_index_entries->tpm_nvindex_entry),
			sizeof(TPM_NV_DATA_SENSITIVE) * tpm_nv_index_entries->nvIndexCount);
    }
    /* immediately after allocating, initialize so that _Delete is safe even on a _Load error */
    for (i = 0 ; (rc == 0) && (i < tpm_nv_index_entries->nvIndexCount) ; i++) {
	TPM_NVDataSensitive_Init(&(tpm_nv_index_entries->tpm_nvindex_entry[i]));
    }
    /* tpm_nvindex_entry array */
    for (i = 0 ; (rc == 0) && (i < tpm_nv_index_entries->nvIndexCount) ; i++) {
	printf("  TPM_NVIndexEntries_Load: Loading slot %u\n", i);
	if (rc == 0) {
	    rc = TPM_NVDataSensitive_Load(&(tpm_nv_index_entries->tpm_nvindex_entry[i]),
					  nvEntriesVersion, stream, stream_size);
	}
	/* should never load an unused entry */
	if (rc == 0) {
	    printf("  TPM_NVIndexEntries_Load: Loaded NV index %08x\n",
		   tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.nvIndex);
	    if (tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.nvIndex == TPM_NV_INDEX_LOCK) {
		printf("TPM_NVIndexEntries_Load: Error (fatal) Entry %u bad NV index %08x\n",
		       i, tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.nvIndex);
		rc = TPM_FAIL;
	    }
	}
    }
    return rc;
}

/*
  TPM_NVIndexEntries_Store() serializes the TPM_NV_INDEX_ENTRIES array into a stream.  Only used
  entries are serialized.

  The first data in the stream is the used count, obtained by iterating through the array.
*/

TPM_RESULT TPM_NVIndexEntries_Store(TPM_STORE_BUFFER *sbuffer,
				    TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    TPM_RESULT 	rc = 0;
    uint32_t 	count;		/* number of used entries to store */
    size_t i;
   
    printf(" TPM_NVIndexEntries_Store: Storing from %u slots\n",
	   tpm_nv_index_entries->nvIndexCount);
    /* append the NV entries version number to the stream */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_NVSTATE_NV_V2); 
    }
    /* count the number of used entries */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_GetUsedCount(&count, tpm_nv_index_entries);
    }
    /* store the actual used count, not the number of array entries */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append32(sbuffer, count); 
    }
    /* tpm_nvindex_entry array */
    for (i = 0 ; (rc == 0) && (i < tpm_nv_index_entries->nvIndexCount) ; i++) {
	/* if the entry is used */
	if (tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.nvIndex != TPM_NV_INDEX_LOCK) {
	    printf("  TPM_NVIndexEntries_Store: Storing slot %lu NV index %08x\n",
		   (unsigned long)i, tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.nvIndex);
	    rc = TPM_NVDataSensitive_Store(sbuffer, &(tpm_nv_index_entries->tpm_nvindex_entry[i]));
	}
	else {
	    printf("  TPM_NVIndexEntries_Store: Skipping unused slot %lu\n", (unsigned long)i);
	}
    }
    return rc;
}

/* TPM_NVIndexEntries_StClear() steps through each entry in the NV TPM_NV_INDEX_ENTRIES array,
   setting the volatile flags to FALSE.
*/

void TPM_NVIndexEntries_StClear(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    size_t i;

    printf(" TPM_NVIndexEntries_StClear: Clearing %u slots\n", tpm_nv_index_entries->nvIndexCount);
    /* bReadSTClear and bWriteSTClear are volatile, in that they are set FALSE at
       TPM_Startup(ST_Clear) */
    for (i = 0 ; i < tpm_nv_index_entries->nvIndexCount ; i++) {
	tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.bReadSTClear = FALSE;	
	tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.bWriteSTClear = FALSE;
    }
    return;
}

/* TPM_NVIndexEntries_LoadVolatile() deserializes the stream into the volatile members of the
   TPM_NV_INDEX_ENTRIES array.
*/

TPM_RESULT TPM_NVIndexEntries_LoadVolatile(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries,
					   unsigned char **stream,
					   uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    uint32_t 		usedCount;
    uint32_t		entryIndex;
    TPM_NV_DATA_PUBLIC	*tpm_nv_data_public;

    printf(" TPM_NVIndexEntries_LoadVolatile:\n");
    /* check tag */
    if (rc == 0) {
        rc = TPM_CheckTag(TPM_TAG_NV_INDEX_ENTRIES_VOLATILE_V1, stream, stream_size);
    }
    /* Get the number of used slots.  This should be equal to the total number of slots. */
    if (rc == 0) {
	rc = TPM_Load32(&usedCount, stream, stream_size);
    }
    if (rc == 0) {
	printf("  TPM_NVIndexEntries_LoadVolatile: usedCount %u\n", usedCount);
	if (usedCount != tpm_nv_index_entries->nvIndexCount) {
	    printf("TPM_NVIndexEntries_LoadVolatile: Error (fatal), "
		   "usedCount %u does not equal slot count %u\n",
		   usedCount, tpm_nv_index_entries->nvIndexCount);
	    rc = TPM_FAIL;
	}
    }
    /* deserialize the stream into the TPM_NV_INDEX_ENTRIES array */
    for (entryIndex = 0 ;
	 (rc == 0) && (entryIndex  < tpm_nv_index_entries->nvIndexCount) ;
	 entryIndex++) {

	tpm_nv_data_public = &(tpm_nv_index_entries->tpm_nvindex_entry[entryIndex].pubInfo);
	printf("  TPM_NVIndexEntries_LoadVolatile: Loading index %08x\n",
	       tpm_nv_data_public->nvIndex);
	/* load bReadSTClear */
	if (rc == 0) {
	    rc = TPM_LoadBool(&(tpm_nv_data_public->bReadSTClear), stream, stream_size);
	}
	/* load bWriteSTClear */
	if (rc == 0) {
	    rc = TPM_LoadBool(&(tpm_nv_data_public->bWriteSTClear), stream, stream_size);
	}
    }
    return rc;
}

/* TPM_NVIndexEntries_StoreVolatile() serializes the volatile members of the
   TPM_NV_INDEX_ENTRIES array into the TPM_STORE_BUFFER.
*/

TPM_RESULT TPM_NVIndexEntries_StoreVolatile(TPM_STORE_BUFFER *sbuffer,
					    TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    TPM_RESULT		rc = 0;
    uint32_t 		usedCount;
    uint32_t		entryIndex;
    TPM_NV_DATA_PUBLIC	*tpm_nv_data_public;
    
    printf(" TPM_NVIndexEntries_StoreVolatile: %u slots\n", tpm_nv_index_entries->nvIndexCount);
    /* store tag */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_NV_INDEX_ENTRIES_VOLATILE_V1);
    }
    /* Get the number of used slots.  If indexes were deleted since the last TPM_Init, there can be
       some unused slots. */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_GetUsedCount(&usedCount, tpm_nv_index_entries);
    }
    /* store usedCount */
    if (rc == 0) {
	printf("  TPM_NVIndexEntries_StoreVolatile: usedCount %u\n", usedCount);
	rc = TPM_Sbuffer_Append32(sbuffer, usedCount);
    }
    /* save entries into the array */
    for (entryIndex = 0 ;
	 (rc == 0) && (entryIndex  < tpm_nv_index_entries->nvIndexCount) ;
	 entryIndex++) {
	/* Only save used slots.  During a rollback, slots are deleted and recreated.  At that time,
	   unused slots will be reclaimed.  */
	if (tpm_nv_index_entries->tpm_nvindex_entry[entryIndex].pubInfo.nvIndex !=
	    TPM_NV_INDEX_LOCK) {

	    tpm_nv_data_public = &(tpm_nv_index_entries->tpm_nvindex_entry[entryIndex].pubInfo);
	    printf("  TPM_NVIndexEntries_StoreVolatile: Storing index %08x\n",
		   tpm_nv_data_public->nvIndex);
	    /* store bReadSTClear */
	    if (rc == 0) {
		rc = TPM_Sbuffer_Append(sbuffer,
					&(tpm_nv_data_public->bReadSTClear), sizeof(TPM_BOOL));
	    }
	    /* store bWriteSTClear */
	    if (rc == 0) {
		rc = TPM_Sbuffer_Append(sbuffer,
					&(tpm_nv_data_public->bWriteSTClear), sizeof(TPM_BOOL));
	    }
	}
    }
    return rc;
}

/* TPM_NVIndexEntries_GetVolatile() saves an array of the NV defined space volatile flags.

   The array is used during a rollback, since the volatile flags are not stored in NVRAM
*/

TPM_RESULT TPM_NVIndexEntries_GetVolatile(TPM_NV_DATA_ST **tpm_nv_data_st, /* freed by caller */
					  TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    TPM_RESULT 	rc = 0;
    uint32_t 	usedCount;
    uint32_t	entryIndex;
    uint32_t	usedIndex;

    printf(" TPM_NVIndexEntries_GetVolatile: %u slots\n", tpm_nv_index_entries->nvIndexCount);
    /* Get the number of used slots.  If indexes were deleted since the last TPM_Init, there can be
       some unused slots. */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_GetUsedCount(&usedCount, tpm_nv_index_entries);
    }
    /* allocate memory for the array, nvIndexCount TPM_NV_DATA_SENSITIVE structures */
    if ((rc == 0) && (usedCount > 0)) {
	printf("  TPM_NVIndexEntries_GetVolatile: Aloocating for %u used slots\n", usedCount);
	rc = TPM_Malloc((unsigned char **)tpm_nv_data_st,
			sizeof(TPM_NV_DATA_ST) * usedCount);
    }
    /* save entries into the array */
    for (entryIndex = 0 , usedIndex = 0 ;
	 (rc == 0) && (entryIndex  < tpm_nv_index_entries->nvIndexCount) && (usedCount > 0) ;
	 entryIndex++) {
	/* Only save used slots.  During a rollback, slots are deleted and recreated.  At that time,
	   unused slots will be reclaimed.  */
	if (tpm_nv_index_entries->tpm_nvindex_entry[entryIndex].pubInfo.nvIndex !=
	    TPM_NV_INDEX_LOCK) {

	    printf("  TPM_NVIndexEntries_GetVolatile: Saving slot %u at used %u NV index %08x\n",
		   entryIndex, usedIndex,
		   tpm_nv_index_entries->tpm_nvindex_entry[entryIndex].pubInfo.nvIndex);
	    
	    printf("  TPM_NVIndexEntries_GetVolatile: bReadSTClear %u bWriteSTClear %u\n",
		   tpm_nv_index_entries->tpm_nvindex_entry[entryIndex].pubInfo.bReadSTClear,
		   tpm_nv_index_entries->tpm_nvindex_entry[entryIndex].pubInfo.bWriteSTClear);
	    (*tpm_nv_data_st)[usedIndex].nvIndex =
		tpm_nv_index_entries->tpm_nvindex_entry[entryIndex].pubInfo.nvIndex;
	    (*tpm_nv_data_st)[usedIndex].bReadSTClear =
		tpm_nv_index_entries->tpm_nvindex_entry[entryIndex].pubInfo.bReadSTClear;
	    (*tpm_nv_data_st)[usedIndex].bWriteSTClear =
		tpm_nv_index_entries->tpm_nvindex_entry[entryIndex].pubInfo.bWriteSTClear;
	    usedIndex++;
	}
    }
    return rc;
}

/* TPM_NVIndexEntries_SetVolatile() restores an array of the NV defined space volatile flags.

   The array is used during a rollback, since the volatile flags are not stored in NVRAM
*/

TPM_RESULT TPM_NVIndexEntries_SetVolatile(TPM_NV_DATA_ST *tpm_nv_data_st,
					  TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    TPM_RESULT 	rc = 0;
    uint32_t 	usedCount;
    uint32_t	i;

    printf(" TPM_NVIndexEntries_SetVolatile: %u slots\n", tpm_nv_index_entries->nvIndexCount);
    /* Get the number of used slots.  This should be equal to the total number of slots. */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_GetUsedCount(&usedCount, tpm_nv_index_entries);
    }
    if (rc == 0) {
	if (usedCount != tpm_nv_index_entries->nvIndexCount) {
	    printf("TPM_NVIndexEntries_SetVolatile: Error (fatal), "
		   "usedCount %u does not equal slot count %u\n",
		   usedCount, tpm_nv_index_entries->nvIndexCount);
	    rc = TPM_FAIL;
	}
    }    
    /* if the used count is non-zero, the volatile array should not be NULL */
    if (rc == 0) {
	if ((usedCount > 0) && (tpm_nv_data_st == NULL)) {
	    printf("TPM_NVIndexEntries_SetVolatile: Error (fatal), "
		   "usedCount %u unconsistant with volatile array NULL\n", usedCount);
	    rc = TPM_FAIL;
	}
    }
    /* copy entries into the array */
    for (i = 0 ; (rc == 0) && (i < tpm_nv_index_entries->nvIndexCount) ; i++) {
	printf("  TPM_NVIndexEntries_SetVolatile: slot %u index %08x\n",
	       i, tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.nvIndex);
	/* sanity check on a mismatch of entries between the save and restore */
	if (tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.nvIndex !=
	    tpm_nv_data_st[i].nvIndex) {

	    printf("TPM_NVIndexEntries_SetVolatile: Error (fatal), "
		   "mismatch NV entry %08x, saved %08x\n",
		   tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.nvIndex,
		   tpm_nv_data_st[i].nvIndex);
	    rc = TPM_FAIL;
	}
	/* restore entries from the array */
	else {
	    printf("  TPM_NVIndexEntries_SetVolatile: bReadSTClear %u bWriteSTClear %u\n",
		   tpm_nv_data_st[i].bReadSTClear, tpm_nv_data_st[i].bWriteSTClear);
	    tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.bReadSTClear =
		tpm_nv_data_st[i].bReadSTClear;
	    tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.bWriteSTClear =
		tpm_nv_data_st[i].bWriteSTClear;
	}
    }
    return rc;
}

/* TPM_NVIndexEntries_GetFreeEntry() gets a free entry in the TPM_NV_INDEX_ENTRIES array.

   If a free entry exists, it it returned.  It should already be initialized.

   If a free entry does not exist, it it created and initialized.

   If a slot cannot be created, tpm_nv_data_sensitive returns NULL, so a subsequent free is safe.
*/

TPM_RESULT TPM_NVIndexEntries_GetFreeEntry(TPM_NV_DATA_SENSITIVE **tpm_nv_data_sensitive,
					   TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    TPM_RESULT		rc = 0;
    TPM_BOOL		done = FALSE;
    size_t 		i;

    printf(" TPM_NVIndexEntries_GetFreeEntry: Searching %u slots\n",
	   tpm_nv_index_entries->nvIndexCount);
    /* for debug - trace the entire TPM_NV_INDEX_ENTRIES array */
    for (i = 0 ; i < tpm_nv_index_entries->nvIndexCount ; i++) {
	*tpm_nv_data_sensitive = &(tpm_nv_index_entries->tpm_nvindex_entry[i]);
	printf("   TPM_NVIndexEntries_GetFreeEntry: slot %lu entry %08x\n",
	       (unsigned long)i, (*tpm_nv_data_sensitive)->pubInfo.nvIndex);
    }    
    /* search the existing array for a free entry */
    for (i = 0 ; (rc == 0) && (i < tpm_nv_index_entries->nvIndexCount) && !done ; i++) {
	*tpm_nv_data_sensitive = &(tpm_nv_index_entries->tpm_nvindex_entry[i]);
	/* if the entry is not used */
	if ((*tpm_nv_data_sensitive)->pubInfo.nvIndex == TPM_NV_INDEX_LOCK) {
	    printf("  TPM_NVIndexEntries_GetFreeEntry: Found free slot %lu\n", (unsigned long)i);
	    done = TRUE;
	}
    }
    /* need to expand the array */
    if ((rc == 0) && !done) {
	*tpm_nv_data_sensitive = NULL;
	rc = TPM_Realloc((unsigned char **)&(tpm_nv_index_entries->tpm_nvindex_entry),
			 sizeof(TPM_NV_DATA_SENSITIVE) * (i + 1));
    }
    /* initialize the new entry in the array */
    if ((rc == 0) && !done) {
	printf("  TPM_NVIndexEntries_GetFreeEntry: Created new slot at index %lu\n",
	       (unsigned long)i);
	*tpm_nv_data_sensitive = &(tpm_nv_index_entries->tpm_nvindex_entry[i]);
	TPM_NVDataSensitive_Init(*tpm_nv_data_sensitive);
	tpm_nv_index_entries->nvIndexCount++;
    }
    return rc;
}

/* TPM_NVIndexEntries_GetEntry() gets the TPM_NV_DATA_SENSITIVE entry corresponding to nvIndex.

   Returns TPM_BADINDEX on non-existent nvIndex
*/

TPM_RESULT TPM_NVIndexEntries_GetEntry(TPM_NV_DATA_SENSITIVE **tpm_nv_data_sensitive,
				       TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries,
				       TPM_NV_INDEX nvIndex)
{
    TPM_RESULT			rc = 0;
    size_t 			i;
    TPM_BOOL			found;
    
    printf(" TPM_NVIndexEntries_GetEntry: Getting NV index %08x in %u slots\n",
	   nvIndex, tpm_nv_index_entries->nvIndexCount);
    /* for debug tracing */
    for (i = 0 ; i < tpm_nv_index_entries->nvIndexCount ; i++) {
	*tpm_nv_data_sensitive = &(tpm_nv_index_entries->tpm_nvindex_entry[i]);
	printf("   TPM_NVIndexEntries_GetEntry: slot %lu entry %08x\n",
	       (unsigned long)i, (*tpm_nv_data_sensitive)->pubInfo.nvIndex);
    }    
    /* check for the special index that indicates an empty entry */
    if (rc == 0) {
	if (nvIndex == TPM_NV_INDEX_LOCK) {
	    rc = TPM_BADINDEX;
	}
    }
    for (i = 0 , found = FALSE ;
	 (rc == 0) && (i < tpm_nv_index_entries->nvIndexCount) && !found ;
	 i++) {

	*tpm_nv_data_sensitive = &(tpm_nv_index_entries->tpm_nvindex_entry[i]);
	if ((*tpm_nv_data_sensitive)->pubInfo.nvIndex == nvIndex) {
	    printf("  TPM_NVIndexEntries_GetEntry: Found NV index at slot %lu\n", (unsigned long)i);
	    printf("   TPM_NVIndexEntries_GetEntry: permission %08x dataSize %u\n",
		   (*tpm_nv_data_sensitive)->pubInfo.permission.attributes,
		   (*tpm_nv_data_sensitive)->pubInfo.dataSize);
	    printf("   TPM_NVIndexEntries_GetEntry: "
		   "bReadSTClear %02x bWriteSTClear %02x bWriteDefine %02x\n",
		   (*tpm_nv_data_sensitive)->pubInfo.bReadSTClear,
		   (*tpm_nv_data_sensitive)->pubInfo.bWriteSTClear,
		   (*tpm_nv_data_sensitive)->pubInfo.bWriteDefine);
	    found = TRUE;
	}
    }
    if (rc == 0) {
	if (!found) {
	    printf("  TPM_NVIndexEntries_GetEntry: NV index not found\n");
	    rc = TPM_BADINDEX;
	}
    }
    return rc;
}

/* TPM_NVIndexEntries_GetUsedCount() returns the number of used entries in the TPM_NV_INDEX_ENTRIES
   array.

   At startup, all entries will be used.  If an NV index is deleted, the entryis marked unused, but
   the TPM_NV_INDEX_ENTRIES space is not reclaimed until the next startup.
*/

TPM_RESULT TPM_NVIndexEntries_GetUsedCount(uint32_t *count,
					   TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    TPM_RESULT	rc = 0;
    size_t 	i;
    
    *count = 0;
    for (i = 0 ; (rc == 0) && (i < tpm_nv_index_entries->nvIndexCount) ; i++) {
	/* if the entry is used */
	if (tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.nvIndex != TPM_NV_INDEX_LOCK) {
	    (*count)++;
	}
    }
    printf(" TPM_NVIndexEntries_GetUsedCount: Used count %d in %u slots\n",
	   *count, tpm_nv_index_entries->nvIndexCount);
    return rc;
}

/* TPM_NVIndexEntries_GetNVList() serializes a list of the used NV indexes into the
   TPM_STORE_BUFFER
*/

TPM_RESULT TPM_NVIndexEntries_GetNVList(TPM_STORE_BUFFER *sbuffer,
					TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    TPM_RESULT	rc = 0;
    size_t 	i;

    printf(" TPM_NVIndexEntries_GetNVList: Creating list from %u slots\n",
	   tpm_nv_index_entries->nvIndexCount);
    
    for (i = 0 ; (rc == 0) && (i < tpm_nv_index_entries->nvIndexCount) ; i++) {
	/* if the entry is used */
	if (tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.nvIndex != TPM_NV_INDEX_LOCK) {
	    rc = TPM_Sbuffer_Append32(sbuffer,
				      tpm_nv_index_entries->tpm_nvindex_entry[i].pubInfo.nvIndex);
	}
    }
    return rc;
}

/* TPM_NVIndexEntries_GetUsedSpace() gets the NV space consumed by NV defined space indexes.

   It does it inefficiently but reliably by serializing the structure with the same function used
   when writing to NV storage.
*/

TPM_RESULT TPM_NVIndexEntries_GetUsedSpace(uint32_t *usedSpace,
					   TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    TPM_RESULT	rc = 0;
    TPM_STORE_BUFFER sbuffer;
    const unsigned char *buffer;
    
    printf("  TPM_NVIndexEntries_GetUsedSpace:\n");
    TPM_Sbuffer_Init(&sbuffer);			/* freed @1 */
    /* serialize NV defined space */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_Store(&sbuffer, tpm_nv_index_entries);
    }
    /* get the serialized buffer and its length */
    if (rc == 0) {
	TPM_Sbuffer_Get(&sbuffer, &buffer, usedSpace);
	printf("  TPM_NVIndexEntries_GetUsedSpace: Used space %u\n", *usedSpace);
    }
    TPM_Sbuffer_Delete(&sbuffer);	/* @1 */
    return rc;
}

/* TPM_NVIndexEntries_GetFreeSpace() gets the total free NV defined space.

   When defining an index, not all can be used for data, as some is consumed by metadata such as
   authorization and the index number.
*/

TPM_RESULT TPM_NVIndexEntries_GetFreeSpace(uint32_t *freeSpace,
					   TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries)
{
    TPM_RESULT	rc = 0;
    uint32_t usedSpace;

    printf("  TPM_NVIndexEntries_GetFreeSpace:\n");
    /* get the used space */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_GetUsedSpace(&usedSpace, tpm_nv_index_entries);
    }
    /* sanity check */
    if (rc == 0) {
	if (usedSpace > TPM_MAX_NV_DEFINED_SIZE) {
	    printf("TPM_NVIndexEntries_GetFreeSpace: used %u greater than max %u\n",
		   usedSpace, TPM_MAX_NV_DEFINED_SIZE);
	    rc = TPM_NOSPACE;
	}
    }
    /* calculate the free space */
    if (rc == 0) {
	*freeSpace = TPM_MAX_NV_DEFINED_SIZE - usedSpace;
	printf("  TPM_NVIndexEntries_GetFreeSpace: Free space %u\n", *freeSpace);
    }
    return rc;
}
					  
/* TPM_OwnerClear: rev 99
   12. The TPM MUST deallocate all defined NV storage areas where
   a. TPM_NV_PER_OWNERWRITE is TRUE if nvIndex does not have the "D" bit set
   b. TPM_NV_PER_OWNERREAD is TRUE if nvIndex does not have the "D" bit set
   c. The TPM MUST NOT deallocate any other currently defined NV storage areas.

   TPM_RevokeTrust: a. NV items with the pubInfo -> nvIndex D value set MUST be deleted. This
   changes the TPM_OwnerClear handling of the same NV areas

   If deleteAllNvram is TRUE, all NVRAM is deleted.  If it is FALSE, indexes with the D bit set are
   not cleared.

   The write to NV space is done bu the caller.
*/

TPM_RESULT TPM_NVIndexEntries_DeleteOwnerAuthorized(TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries,
						    TPM_BOOL deleteAllNvram)
{
    TPM_RESULT			rc = 0;
    size_t 			i;
    TPM_NV_DATA_SENSITIVE	*tpm_nv_data_sensitive;	/* an entry in the array */
    
    printf(" TPM_NVIndexEntries_DeleteOwnerAuthorized: Deleting from %u slots\n",
	   tpm_nv_index_entries->nvIndexCount);
    for (i = 0 ; i < tpm_nv_index_entries->nvIndexCount ; i++) {
	/* get an entry in the array */
	tpm_nv_data_sensitive = &(tpm_nv_index_entries->tpm_nvindex_entry[i]);

	/* if the index is in use */
	if (tpm_nv_data_sensitive->pubInfo.nvIndex != TPM_NV_INDEX_LOCK) {
	    /* if TPM_NV_PER_OWNERWRITE or TPM_NV_PER_OWNERREAD and nvIndex does not have the "D"
	       bit set */
	    if ((tpm_nv_data_sensitive->pubInfo.permission.attributes & TPM_NV_PER_OWNERWRITE) ||
		(tpm_nv_data_sensitive->pubInfo.permission.attributes & TPM_NV_PER_OWNERREAD)) {
		if (!(tpm_nv_data_sensitive->pubInfo.nvIndex & TPM_NV_INDEX_D_BIT) ||
		    deleteAllNvram) {
		    /* delete the index */
		    printf(" TPM_NVIndexEntries_DeleteOwnerAuthorized: Deleting NV index %08x\n",
			   tpm_nv_data_sensitive->pubInfo.nvIndex);
		    TPM_NVDataSensitive_Delete(tpm_nv_data_sensitive);
		}
	    }
	}
    }
    return rc;
}

/* TPM_NVIndexEntries_GetDataPublic() returns the TPM_NV_DATA_PUBLIC corresponding to the nvIndex
 */

TPM_RESULT TPM_NVIndexEntries_GetDataPublic(TPM_NV_DATA_PUBLIC **tpm_nv_data_public,
					    TPM_NV_INDEX_ENTRIES *tpm_nv_index_entries,
					    TPM_NV_INDEX nvIndex)
{
    TPM_RESULT			rc = 0;
    TPM_NV_DATA_SENSITIVE 	*tpm_nv_data_sensitive;
    
    printf(" TPM_NVIndexEntries_GetDataPublic: Getting data at NV index %08x\n", nvIndex);
    if (rc == 0) {
	rc = TPM_NVIndexEntries_GetEntry(&tpm_nv_data_sensitive,
					 tpm_nv_index_entries,
					 nvIndex);
    }
    if (rc == 0) {
	*tpm_nv_data_public = &(tpm_nv_data_sensitive->pubInfo);
    }
    return rc;
}

/*
  Command Processing Functions
*/

/* 20.4 TPM_NV_ReadValue rev 114

   Read a value from the NV store. This command uses optional owner authorization.

   Action 1 indicates that if the NV area is not locked then reading of the NV area continues
   without ANY authorization. This is intentional, and allows a platform manufacturer to set the NV
   areas, read them back, and then lock them all without having to install a TPM owner.
*/

TPM_RESULT TPM_Process_NVReadValue(tpm_state_t *tpm_state,
				   TPM_STORE_BUFFER *response,
				   TPM_TAG tag,
				   uint32_t paramSize,
				   TPM_COMMAND_CODE ordinal,
				   unsigned char *command,
				   TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_NV_INDEX	nvIndex;	/* The index of the area to set */
    uint32_t		offset = 0;	/* The offset into the area */
    uint32_t		dataSize = 0;	/* The size of the data area */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for TPM Owner
					   authorization */
    TPM_NONCE		nonceOdd;	/* Nonce generated by caller */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA	ownerAuth;	/* HMAC key: TPM Owner authorization */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_BOOL			ignore_auth = FALSE;
    TPM_BOOL			dir = FALSE;
    TPM_BOOL			physicalPresence;
    TPM_BOOL			isGPIO = FALSE;
    BYTE 			*gpioData = NULL;
    TPM_NV_DATA_SENSITIVE	*d1NvdataSensitive = NULL;
    uint32_t			s1Last;
    
    /* output parameters  */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	data;		/* The data to set the area to */

    printf("TPM_Process_NVReadValue: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&data);			/* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get nvIndex parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&nvIndex, &command, &paramSize);
    }
    /* get offset parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&offset, &command, &paramSize);
    }
    /* get dataSize parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&dataSize, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_NO_LOCKOUT |
						     TPM_CHECK_NV_NOAUTH));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag10(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_NVReadValue: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. If TPM_PERMANENT_FLAGS -> nvLocked is FALSE then all authorization checks are
       ignored */
    /* a. Ignored checks include physical presence, owner authorization, PCR, bReadSTClear,
       locality, TPM_NV_PER_OWNERREAD, disabled and deactivated */
    /* b. TPM_NV_PER_AUTHREAD is not ignored. */
    /* c. If ownerAuth is present, the TPM MAY check the authorization HMAC. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_NVReadValue: index %08x offset %u dataSize %u\n",
	       nvIndex, offset, dataSize);
	if (!(tpm_state->tpm_permanent_flags.nvLocked)) {
	    printf("TPM_Process_NVReadValue: nvLocked FALSE, ignoring authorization\n");
	    ignore_auth = TRUE;
	}
	/* determine whether the nvIndex is legal GPIO space */
	if (returnCode == 0) {
	    returnCode = TPM_NVDataSensitive_IsGPIO(&isGPIO, nvIndex);
	}
    }
    /* 2. Set D1 a TPM_NV_DATA_AREA structure to the area pointed to by nvIndex, if not found
       return TPM_BADINDEX */
    if (returnCode == TPM_SUCCESS) {
	/* a. If nvIndex = TPM_NV_INDEX_DIR, set D1 to TPM_PERMANENT_DATA -> authDir[0] */
	if (nvIndex == TPM_NV_INDEX_DIR) {
	    printf("TPM_Process_NVReadValue: Reading DIR\n");
	    dir = TRUE;
	}
	else {
	    printf("TPM_Process_NVReadValue: Loading data from NVRAM\n");
	    returnCode = TPM_NVIndexEntries_GetEntry(&d1NvdataSensitive,
						     &(tpm_state->tpm_nv_index_entries),
						     nvIndex);
	    if (returnCode != 0) {
		printf("TPM_Process_NVReadValue: Error, NV index %08x not found\n", nvIndex);
	    }
	}
    }
    /* Do not check permission for DIR, DIR is no-auth */
    if ((returnCode == TPM_SUCCESS) && !dir) {
	/* 3. If TPM_PERMANENT_FLAGS -> nvLocked is TRUE */
	if (tpm_state->tpm_permanent_flags.nvLocked) {
	    /* a. If D1 -> permission -> TPM_NV_PER_OWNERREAD is TRUE */
	    if (d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_OWNERREAD) {
		/* i. If TPM_PERMANENT_FLAGS -> disable is TRUE, return TPM_DISABLED */
		if (tpm_state->tpm_permanent_flags.disable) {
		    printf("TPM_Process_NVReadValue: Error, disabled\n");
		    return TPM_DISABLED;
		}
		/* ii. If TPM_STCLEAR_FLAGS -> deactivated is TRUE, return TPM_DEACTIVATED */
		else if (tpm_state->tpm_stclear_flags.deactivated) {
		    printf("TPM_Process_NVReadValue: Error, deactivated\n");
		    return TPM_DEACTIVATED;;
		}
	    }
	    /* NOTE: Intel software requires NV access disabled and deactivated */
	    /* b. If D1 -> permission -> TPM_NV_PER_OWNERREAD is FALSE */
	    /* i. If TPM_PERMANENT_FLAGS -> disable is TRUE, the TPM MAY return TPM_DISABLED */
	    /* ii. If TPM_STCLEAR_FLAGS -> deactivated is TRUE, the TPM MAY return
	       TPM_DEACTIVATED */
	}
    }
    /* 4. If tag = TPM_TAG_RQU_AUTH1_COMMAND then */
    /* NOTE: This is optional if ignore_auth is TRUE */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND) && !dir) {
	/* a. If D1 -> TPM_NV_PER_OWNERREAD is FALSE return TPM_AUTH_CONFLICT */
	if (!(d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_OWNERREAD)) {
	    printf("TPM_Process_NVReadValue: Error, "
		   "owner authorization conflict, attributes %08x\n",
		   d1NvdataSensitive->pubInfo.permission.attributes);
	    returnCode = TPM_AUTH_CONFLICT;
	}
    }
    /* b. Validate command and parameters using TPM Owners authorization on error return
       TPM_AUTHFAIL */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_OWNER,
					      ordinal,
					      NULL,
					      &(tpm_state->tpm_permanent_data.ownerAuth), /* OIAP */
					      tpm_state->tpm_permanent_data.ownerAuth);	  /* OSAP */
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND) && !ignore_auth) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					ownerAuth);		/* Authorization digest for input */
    }
    /* 5. Else */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND) && !dir) {
	/* a. If D1 -> TPM_NV_PER_AUTHREAD is TRUE return TPM_AUTH_CONFLICT */
	if (d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_AUTHREAD) {
	    printf("TPM_Process_NVReadValue: Error, authorization conflict TPM_NV_PER_AUTHREAD\n");
	    returnCode = TPM_AUTH_CONFLICT;
	}
    }
    /* b. If D1 -> TPM_NV_PER_OWNERREAD is TRUE return TPM_AUTH_CONFLICT */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND) && !ignore_auth && !dir) {
	if (d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_OWNERREAD) {
	    printf("TPM_Process_NVReadValue: Error, authorization conflict TPM_NV_PER_OWNERREAD\n");
	    returnCode = TPM_AUTH_CONFLICT;
	}
    }
    /* 6. Check that D1 -> pcrInfoRead -> localityAtRelease for TPM_STANY_DATA -> localityModifier
       is TRUE */
    /* a. For example if TPM_STANY_DATA -> localityModifier was 2 then D1 -> pcrInfo ->
       localityAtRelease -> TPM_LOC_TWO would have to be TRUE */
    /* b. On error return TPM_BAD_LOCALITY */
    /* NOTE Done by TPM_PCRInfoShort_CheckDigest() */
    /* 7. If D1 -> attributes specifies TPM_NV_PER_PPREAD then validate physical presence is
       asserted if not return TPM_BAD_PRESENCE */
    if ((returnCode == TPM_SUCCESS) && !ignore_auth && !dir) {
	if (d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_PPREAD) {
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_Global_GetPhysicalPresence(&physicalPresence, tpm_state);
	    }
	    if (returnCode == TPM_SUCCESS) {
		if (!physicalPresence) {
		    printf("TPM_Process_NVReadValue: Error, physicalPresence is FALSE\n");
		    returnCode = TPM_BAD_PRESENCE;
		}
	    }
	}
    }
    if ((returnCode == TPM_SUCCESS) && !ignore_auth && !dir) {
	/* 8. If D1 -> TPM_NV_PER_READ_STCLEAR then */
	if ((d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_READ_STCLEAR) &&
	    /* a. If D1 -> bReadSTClear is TRUE return TPM_DISABLED_CMD */
	    (d1NvdataSensitive->pubInfo.bReadSTClear)) {
	    printf("TPM_Process_NVReadValue: Error, area locked by bReadSTClear\n");
	    returnCode = TPM_DISABLED_CMD;
	}
    }
    /* 9. If D1 -> pcrInfoRead -> pcrSelection specifies a selection of PCR */
    /* a. Create P1 a composite hash of the PCR specified by D1 -> pcrInfoRead */
    /* b. Compare P1 to D1 -> pcrInfoRead -> digestAtRelease return TPM_WRONGPCRVAL on
       mismatch */
    if ((returnCode == TPM_SUCCESS) && !ignore_auth && !dir) {
	returnCode = TPM_PCRInfoShort_CheckDigest(&(d1NvdataSensitive->pubInfo.pcrInfoRead),
						  tpm_state->tpm_stclear_data.PCRS,
						  tpm_state->tpm_stany_flags.localityModifier);
    }
    if (returnCode == TPM_SUCCESS && !dir) {
	/* 10. If dataSize is 0 then */
	if (dataSize == 0) {
	    printf("TPM_Process_NVReadValue: dataSize 0, setting bReadSTClear\n");
	    /* a. Set D1 -> bReadSTClear to TRUE */
	    d1NvdataSensitive->pubInfo.bReadSTClear = TRUE;
	    /* b. Set data to NULL (output parameter dataSize to 0) */
	    /* NOTE Done by TPM_SizedBuffer_Init */
	}
	/* 11. Else (if dataSize is not 0) */
	else {
	    if (returnCode == TPM_SUCCESS) {
		/* a. Set S1 to offset + dataSize */
		s1Last = offset + dataSize; /* set to last data point */
		/* b. If S1 > D1 -> dataSize return TPM_NOSPACE */
		if (s1Last > d1NvdataSensitive->pubInfo.dataSize) {
		    printf("TPM_Process_NVReadValue: Error, NVRAM dataSize %u\n",
			   d1NvdataSensitive->pubInfo.dataSize);
		    returnCode = TPM_NOSPACE;
		}
	    }
	    /* c. Set data to area pointed to by offset */
	    if ((returnCode == TPM_SUCCESS) && !isGPIO) {
		TPM_PrintFourLimit("TPM_Process_NVReadValue: read data",
			      d1NvdataSensitive->data + offset, dataSize);
		returnCode = TPM_SizedBuffer_Set(&data,
						 dataSize, d1NvdataSensitive->data + offset);
	    }
	    /* GPIO */
	    if ((returnCode == TPM_SUCCESS) && isGPIO) {
		returnCode = TPM_Malloc(&gpioData, dataSize);	/* freed @2 */
	    }	    
	    if ((returnCode == TPM_SUCCESS) && isGPIO) {
		printf("TPM_Process_NVReadValue: Reading GPIO\n");
		returnCode = TPM_IO_GPIO_Read(nvIndex,
					      dataSize,
					      gpioData,
					      tpm_state->tpm_number);
	    }	    
	    if ((returnCode == TPM_SUCCESS) && isGPIO) {
		returnCode = TPM_SizedBuffer_Set(&data,
						 dataSize, gpioData);
	    }	    
	}
    }
    /* DIR read */
    if (returnCode == TPM_SUCCESS && dir) {
	/* DIR is hard coded as a TPM_DIRVALUE array */
	if (returnCode == TPM_SUCCESS) {
	    s1Last = offset + dataSize;	    /* set to last data point */
	    if (s1Last > TPM_DIGEST_SIZE) {
		printf("TPM_Process_NVReadValue: Error, NVRAM dataSize %u too small\n",
		       TPM_DIGEST_SIZE);
		returnCode = TPM_NOSPACE;
	    }
	}
	/* i.This includes partial reads of TPM_NV_INDEX_DIR. */
	if (returnCode == TPM_SUCCESS) {
	    printf("TPM_Process_NVReadValue: Copying data\n");
	    returnCode = TPM_SizedBuffer_Set(&data, dataSize,
					     tpm_state->tpm_permanent_data.authDIR + offset);
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_NVReadValue: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return data */
	    returnCode = TPM_SizedBuffer_Store(response, &data);
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,	/* owner HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&data);		/* @1 */
    free(gpioData);				/* @2 */
    return rcf;
}

/* 20.5 TPM_NV_ReadValueAuth rev 87

   This command requires that the read be authorized by a value set with the blob.
*/

TPM_RESULT TPM_Process_NVReadValueAuth(tpm_state_t *tpm_state,
				       TPM_STORE_BUFFER *response,
				       TPM_TAG tag,
				       uint32_t paramSize,
				       TPM_COMMAND_CODE ordinal,
				       unsigned char *command,
				       TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_NV_INDEX	nvIndex;	/* The index of the area to set */
    uint32_t		offset = 0;	/* The offset from the data area */
    uint32_t		dataSize = 0;	/* The size of the data area */
    TPM_AUTHHANDLE	authHandle;	/* The auth handle for the NV element authorization */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA	authHmac;	/* HMAC key: nv element authorization */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey;
    TPM_NV_DATA_SENSITIVE	*d1NvdataSensitive;
    uint32_t			s1Last;
    TPM_BOOL			physicalPresence;
    TPM_BOOL			isGPIO;
    BYTE 			*gpioData = NULL;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	data;		/* The data */

    printf("TPM_Process_NVReadValueAuth: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&data);			/* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get nvIndex parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&nvIndex, &command, &paramSize);
    }
    /* get offset parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&offset, &command, &paramSize);
    }
    /* get dataSize parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&dataSize, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALL);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag1(tag);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					authHmac,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_NVReadValueAuth: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* determine whether the nvIndex is legal GPIO space */
    if (returnCode == 0) {
	returnCode = TPM_NVDataSensitive_IsGPIO(&isGPIO, nvIndex);
    }
    /* 1. Locate and set D1 to the TPM_NV_DATA_AREA that corresponds to nvIndex, on error return
       TPM_BAD_INDEX */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_NVReadValueAuth: index %08x offset %u dataSize %u\n",
	       nvIndex, offset, dataSize);
	printf("TPM_Process_NVReadValueAuth: Loading data from NVRAM\n");
	returnCode = TPM_NVIndexEntries_GetEntry(&d1NvdataSensitive,
						 &(tpm_state->tpm_nv_index_entries),
						 nvIndex);
	if (returnCode != 0) {
	    printf("TPM_Process_NVReadValueAuth: Error, NV index %08x not found\n", nvIndex);
	}
    }
    /* 2. If D1 -> TPM_NV_PER_AUTHREAD is FALSE return TPM_AUTH_CONFLICT */
    if (returnCode == TPM_SUCCESS) {
	if (!(d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_AUTHREAD)) {
	    printf("TPM_Process_NVReadValueAuth: Error, authorization conflict\n");
	    returnCode = TPM_AUTH_CONFLICT;
	}
    }
    /* 3. Validate authHmac using D1 -> authValue on error return TPM_AUTHFAIL */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_NV,
					      ordinal,
					      NULL,
					      &(d1NvdataSensitive->authValue),	/* OIAP */
					      d1NvdataSensitive->digest);	/* OSAP */
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					authHmac);		/* Authorization digest for input */
    }
    /* 4. If D1 -> attributes specifies TPM_NV_PER_PPREAD then validate physical presence is
       asserted if not return TPM_BAD_PRESENCE */
    if (returnCode == TPM_SUCCESS) {
	if (d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_PPREAD) {
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_Global_GetPhysicalPresence(&physicalPresence, tpm_state);
	    }
	    if (returnCode == TPM_SUCCESS) {
		if (!physicalPresence) {
		    printf("TPM_Process_NVReadValueAuth: Error, physicalPresence is FALSE\n");
		    returnCode = TPM_BAD_PRESENCE;
		}
	    }
	}
    }
    /* 5. Check that D1 -> pcrInfoRead -> localityAtRelease for TPM_STANY_DATA -> localityModifier
       is TRUE */
    /* a. For example if TPM_STANY_DATA -> localityModifier was 2 then D1 -> pcrInfo ->
       localityAtRelease -> TPM_LOC_TWO would have to be TRUE */
    /* b. On error return TPM_BAD_LOCALITY */
    /* 6. If D1 -> pcrInfoRead -> pcrSelection specifies a selection of PCR */
    /* a. Create P1 a composite hash of the PCR specified by D1 -> pcrInfoRead */
    /* b. Compare P1 to D1 -> pcrInfoRead -> digestAtRelease return TPM_WRONGPCRVAL on
       mismatch */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_PCRInfoShort_CheckDigest(&(d1NvdataSensitive->pubInfo.pcrInfoRead),
						  tpm_state->tpm_stclear_data.PCRS,
						  tpm_state->tpm_stany_flags.localityModifier);
    }
    if (returnCode == TPM_SUCCESS) {
	/* 7. If D1 specifies TPM_NV_PER_READ_STCLEAR then */
	if ((d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_READ_STCLEAR) &&
	    /* a. If D1 -> bReadSTClear is TRUE return TPM_DISABLED_CMD */
	    (d1NvdataSensitive->pubInfo.bReadSTClear)) {
	    printf("TPM_Process_NVReadValueAuth: Error, area locked by bReadSTClear\n");
	    returnCode = TPM_DISABLED_CMD;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* 8. If dataSize is 0 then */
	if (dataSize == 0) {
	    printf("TPM_Process_NVReadValueAuth: dataSize 0, setting bReadSTClear\n");
	    /* a. Set D1 -> bReadSTClear to TRUE */
	    d1NvdataSensitive->pubInfo.bReadSTClear = TRUE;
	    /* b. Set data to NULL */
	    /* NOTE Done by TPM_SizedBuffer_Init */
	}
	/* 9. Else (if dataSize is not 0) */
	else {
	    if (returnCode == TPM_SUCCESS) {
		/* a. Set S1 to offset + dataSize */
		s1Last = offset + dataSize; /* set to last data point */
		/* b. If S1 > D1 -> dataSize return TPM_NOSPACE */
		if (s1Last > d1NvdataSensitive->pubInfo.dataSize) {
		    printf("TPM_Process_NVReadValueAuth: Error, NVRAM dataSize %u too small\n",
			   d1NvdataSensitive->pubInfo.dataSize);
		    returnCode = TPM_NOSPACE;
		}
	    }
	    /* c. Set data to area pointed to by offset */
	    if ((returnCode == TPM_SUCCESS) && !isGPIO) {
		TPM_PrintFourLimit("TPM_Process_NVReadValueAuth: read data",
			      d1NvdataSensitive->data + offset, dataSize);
		returnCode = TPM_SizedBuffer_Set(&data, dataSize, d1NvdataSensitive->data + offset);
	    }
	    /* GPIO */
	    if ((returnCode == TPM_SUCCESS) && isGPIO) {
		returnCode = TPM_Malloc(&gpioData, dataSize);	/* freed @2 */
	    }	    
	    if ((returnCode == TPM_SUCCESS) && isGPIO) {
		printf("TPM_Process_NVReadValueAuth: Reading GPIO\n");
		returnCode = TPM_IO_GPIO_Read(nvIndex,
					      dataSize,
					      gpioData,
					      tpm_state->tpm_number);
	    }	    
	    if ((returnCode == TPM_SUCCESS) && isGPIO) {
		returnCode = TPM_SizedBuffer_Set(&data,
						 dataSize, gpioData);
	    }	    
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_NVReadValueAuth: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return data */
	    returnCode = TPM_SizedBuffer_Store(response, &data);
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,	/* HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&data);			/* @1 */
    return rcf;
}

/* 20.2 TPM_NV_WriteValue rev 117

   This command writes the value to a defined area. The write can be TPM Owner authorized or
   unauthorized and protected by other attributes and will work when no TPM Owner is present.

   The action setting bGlobalLock to TRUE is intentionally before the action checking the 
   owner authorization.	 This allows code (e.g., a BIOS) to lock NVRAM without knowing the 
   owner authorization.

   The DIR (TPM_NV_INDEX_DIR) has the attributes TPM_NV_PER_OWNERWRITE and TPM_NV_WRITEALL.
  
   FIXME: A simpler way to do DIR might be to create the DIR as NV defined space at first
   initialization and remove the special casing here.
*/

TPM_RESULT TPM_Process_NVWriteValue(tpm_state_t *tpm_state,
				    TPM_STORE_BUFFER *response,
				    TPM_TAG tag,
				    uint32_t paramSize,
				    TPM_COMMAND_CODE ordinal,
				    unsigned char *command,
				    TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */
    int		irc;
    
    /* input parameters */
    TPM_NV_INDEX	nvIndex;	/* The index of the area to set */
    uint32_t		offset = 0;	/* The offset into the NV Area */
    TPM_SIZED_BUFFER	data;		/* The data to set the area to */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for TPM Owner */
    TPM_NONCE		nonceOdd;	/* Nonce generated by caller */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA	ownerAuth;	/* The authorization digest HMAC key: TPM Owner auth */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey = NULL;
    TPM_BOOL			ignore_auth = FALSE;
    TPM_BOOL			index0 = FALSE;
    TPM_BOOL			done = FALSE;
    TPM_BOOL			dir = FALSE;
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back NV */
    TPM_NV_DATA_SENSITIVE	*d1NvdataSensitive = NULL;
    uint32_t			s1Last;
    TPM_BOOL			physicalPresence;
    TPM_BOOL			isGPIO = FALSE;
    uint32_t			nv1 = tpm_state->tpm_permanent_data.noOwnerNVWrite;
							/* temp for noOwnerNVWrite, initialize to
							   silence compiler */
    TPM_BOOL			nv1Incremented = FALSE;	/* flag that nv1 was incremented */
    
    /* output parameters  */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_NVWriteValue: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&data);			/* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get nvIndex parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&nvIndex, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&offset, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&data, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_NO_LOCKOUT |
						     TPM_CHECK_NV_NOAUTH));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag10(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_NVWriteValue: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_NVWriteValue: index %08x offset %u dataSize %u\n",
	       nvIndex, offset, data.size);
	TPM_PrintFourLimit("TPM_Process_NVWriteValue: data", data.buffer, data.size);
	/* 1. If TPM_PERMANENT_FLAGS -> nvLocked is FALSE then all authorization checks except for
	   the max NV writes are ignored */
	/* a. Ignored checks include physical presence, owner authorization, TPM_NV_PER_OWNERWRITE,
	   PCR, bWriteDefine, bGlobalLock, bWriteSTClear, locality, disabled and deactivated */
	/* b. TPM_NV_PER_AUTHWRITE is not ignored. */
	/* a.If ownerAuth is present, the TPM MAY check the authorization HMAC. */
	if (!(tpm_state->tpm_permanent_flags.nvLocked)) {
	    printf("TPM_Process_NVWriteValue: nvLocked FALSE, ignoring authorization\n");
	    ignore_auth = TRUE;
	}
	if (nvIndex == TPM_NV_INDEX0) {
	    index0 = TRUE;
	}
	/* determine whether the nvIndex is legal GPIO space */
	if (returnCode == 0) {
	    returnCode = TPM_NVDataSensitive_IsGPIO(&isGPIO, nvIndex);
	}
    }
    /* 2. Locate and set D1 to the TPM_NV_DATA_AREA that corresponds to nvIndex, return TPM_BADINDEX
       on error */
    if ((returnCode == TPM_SUCCESS) && !index0) {
	/* a. If nvIndex = TPM_NV_INDEX_DIR, set D1 to TPM_PERMANENT_DATA -> authDir[0] */
	if (nvIndex == TPM_NV_INDEX_DIR) {
	    printf("TPM_Process_NVWriteValue: Writing DIR\n");
	    dir = TRUE;
	}
	else {
	    printf("TPM_Process_NVWriteValue: Loading data space from NVRAM\n");
	    returnCode = TPM_NVIndexEntries_GetEntry(&d1NvdataSensitive,
						     &(tpm_state->tpm_nv_index_entries),
						     nvIndex);
	    if (returnCode != 0) {
		printf("TPM_Process_NVWriteValue: Error, NV index %08x not found\n", nvIndex);
	    }
	}
    }
    if ((returnCode == TPM_SUCCESS) && !index0) {
	/* 3. If TPM_PERMANENT_FLAGS -> nvLocked is TRUE */
	if (tpm_state->tpm_permanent_flags.nvLocked) {
	    /* a. If D1 -> permission -> TPM_NV_PER_OWNERWRITE is TRUE */
	    if (dir ||		    /* DIR always has TPM_NV_PER_OWNERWRITE */
		(d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_OWNERWRITE)) {
		/* i. If TPM_PERMANENT_FLAGS -> disable is TRUE, return TPM_DISABLED */
		if (tpm_state->tpm_permanent_flags.disable) {
		    printf("TPM_Process_NVWriteValue: Error, disabled\n");
		    return TPM_DISABLED;
		}
		/* ii.If TPM_STCLEAR_FLAGS -> deactivated is TRUE, return TPM_DEACTIVATED */
		else if (tpm_state->tpm_stclear_flags.deactivated) {
		    printf("TPM_Process_NVWriteValue: Error, deactivated\n");
		    return TPM_DEACTIVATED;;
		}
	    }
	    /* NOTE: Intel software requires NV access disabled and deactivated */
	    /* b. If D1 -> permission -> TPM_NV_PER_OWNERWRITE is FALSE */
	    /* i. If TPM_PERMANENT_FLAGS -> disable is TRUE, the TPM MAY return TPM_DISABLED */
	    /* ii. If TPM_STCLEAR_FLAGS -> deactivated is TRUE, the TPM MAY return
	       TPM_DEACTIVATED */
	}
    }
    /* 4. If tag = TPM_TAG_RQU_AUTH1_COMMAND then */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND) && !dir && !index0) {
	/* a. If D1 -> permission -> TPM_NV_PER_OWNERWRITE is FALSE return TPM_AUTH_CONFLICT */
	/* i. This check is ignored if nvIndex is TPM_NV_INDEX0. */
	if (!(d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_OWNERWRITE)) {
	    printf("TPM_Process_NVWriteValue: Error, owner authorization conflict\n");
	    returnCode = TPM_AUTH_CONFLICT;
	}
    }
    /* b. Validate command and parameters using ownerAuth HMAC with TPM Owner authentication as the
       secret, return TPM_AUTHFAIL on error */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_OWNER,
					      ordinal,
					      NULL,
					      &(tpm_state->tpm_permanent_data.ownerAuth), /* OIAP */
					      tpm_state->tpm_permanent_data.ownerAuth);	  /* OSAP */
    }
    /* NOTE: This is optional if ignore_auth is TRUE */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					ownerAuth);		/* Authorization digest for input */
    }
    /* 5. Else */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND) && !ignore_auth && !index0) {
	/* a. If D1 -> permission -> TPM_NV_PER_OWNERWRITE is TRUE return TPM_AUTH_CONFLICT */
	if (dir ||		/* DIR always has TPM_NV_PER_OWNERWRITE */
	    (d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_OWNERWRITE)) {
	    printf("TPM_Process_NVWriteValue: Error, no owner authorization conflict\n");
	    returnCode = TPM_AUTH_CONFLICT;
	}
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND) && !index0) {
	/* b. If no TPM Owner validate max NV writes without an owner */
	/* i. Set NV1 to TPM_PERMANENT_DATA -> noOwnerNVWrite */
	nv1 = tpm_state->tpm_permanent_data.noOwnerNVWrite;
	/* ii. Increment NV1 by 1 */
	nv1++;
	/* iii. If NV1 > TPM_MAX_NV_WRITE_NOOWNER return TPM_MAXNVWRITES */
	if (nv1 > TPM_MAX_NV_WRITE_NOOWNER) {
	    printf("TPM_Process_NVWriteValue: Error, max NV writes %d w/o owner reached\n",
		   tpm_state->tpm_permanent_data.noOwnerNVWrite);
	    returnCode = TPM_MAXNVWRITES;
	}
	/* iv. Set NV1_INCREMENTED to TRUE */
	else {
	    nv1Incremented = TRUE;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* 6. If nvIndex = 0 then */
	if (nvIndex == 0) {
	    /* a. If dataSize is not 0, the TPM MAY return TPM_BADINDEX. */
	    if (data.size != 0) {
		printf("TPM_Process_NVWriteValue: Error, index 0 size %u\n", data.size);
		returnCode = TPM_BADINDEX;
	    }
	    else {
		/* b. Set TPM_STCLEAR_FLAGS -> bGlobalLock to TRUE */
		printf("TPM_Process_NVWriteValue: nvIndex 0, setting bGlobalLock\n");
		tpm_state->tpm_stclear_flags.bGlobalLock = TRUE;
		/* c. Return TPM_SUCCESS */
		done = TRUE;
	    }
	}
    }
    /* 7. If D1 -> permission -> TPM_NV_PER_AUTHWRITE is TRUE return TPM_AUTH_CONFLICT */
    if ((returnCode == TPM_SUCCESS) && !done && !dir) {
	if (d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_AUTHWRITE) {
	    printf("TPM_Process_NVWriteValue: Error, authorization conflict, attributes %08x \n",
		   d1NvdataSensitive->pubInfo.permission.attributes);
	    returnCode = TPM_AUTH_CONFLICT;
	}
    }
    /* 8. Check that D1 -> pcrInfoWrite -> localityAtRelease for TPM_STANY_DATA -> localityModifier
       is TRUE */
    /* a. For example if TPM_STANY_DATA -> localityModifier was 2 then D1 -> pcrInfo ->
       localityAtRelease -> TPM_LOC_TWO would have to be TRUE */
    /* b. On error return TPM_BAD_LOCALITY */
    /* NOTE Done by TPM_PCRInfoShort_CheckDigest() */
    /* 9. If D1 -> attributes specifies TPM_NV_PER_PPWRITE then validate physical presence is
       asserted if not return TPM_BAD_PRESENCE */
    if ((returnCode == TPM_SUCCESS) && !done && !ignore_auth && !dir) {
	if (d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_PPWRITE) {
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_Global_GetPhysicalPresence(&physicalPresence, tpm_state);
	    }
	    if (returnCode == TPM_SUCCESS) {
		if (!physicalPresence) {
		    printf("TPM_Process_NVWriteValue: Error, physicalPresence is FALSE\n");
		    returnCode = TPM_BAD_PRESENCE;
		}
	    }
	}
    }
    if ((returnCode == TPM_SUCCESS) && !done && !ignore_auth && !dir) {
	/* 10. If D1 -> attributes specifies TPM_NV_PER_WRITEDEFINE */
	if ((d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_WRITEDEFINE) &&
	    /* a. If D1 -> bWriteDefine is TRUE return TPM_AREA_LOCKED */
	    (d1NvdataSensitive->pubInfo.bWriteDefine)) {
	    printf("TPM_Process_NVWriteValue: Error, area locked by bWriteDefine\n");
	    returnCode = TPM_AREA_LOCKED;
	}
    }
    if ((returnCode == TPM_SUCCESS) && !done && !ignore_auth && !dir) {
	/* 11. If D1 -> attributes specifies TPM_NV_PER_GLOBALLOCK */
	if ((d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_GLOBALLOCK) &&
	    /* a. If TPM_STCLEAR_FLAGS -> bGlobalLock is TRUE return TPM_AREA_LOCKED */
	    (tpm_state->tpm_stclear_flags.bGlobalLock)) {
	    printf("TPM_Process_NVWriteValue: Error, area locked by bGlobalLock\n");
	    returnCode = TPM_AREA_LOCKED;
	}
    }
    if ((returnCode == TPM_SUCCESS) && !done && !ignore_auth && !dir) {
	/* 12. If D1 -> attributes specifies TPM_NV_PER_WRITE_STCLEAR */
	if ((d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_WRITE_STCLEAR) &&
	    /* a. If D1 ->bWriteSTClear is TRUE return TPM_AREA_LOCKED */
	    (d1NvdataSensitive->pubInfo.bWriteSTClear)) {
	    printf("TPM_Process_NVWriteValue: Error, area locked by bWriteSTClear\n");
	    returnCode = TPM_AREA_LOCKED;
	}
    }
    /* 13. If D1 -> pcrInfoWrite -> pcrSelection specifies a selection of PCR */
    /* a. Create P1 a composite hash of the PCR specified by D1 -> pcrInfoWrite */
    /* b. Compare P1 to D1 -> pcrInfoWrite -> digestAtRelease return TPM_WRONGPCRVAL on mismatch
     */
    if ((returnCode == TPM_SUCCESS) && !done && !ignore_auth && !dir) {
	returnCode = TPM_PCRInfoShort_CheckDigest(&(d1NvdataSensitive->pubInfo.pcrInfoWrite),
						  tpm_state->tpm_stclear_data.PCRS,
						  tpm_state->tpm_stany_flags.localityModifier);
    }
    if ((returnCode == TPM_SUCCESS) && !done && !dir) {
	/* 14. If dataSize = 0 then */
	if (data.size == 0) {
	    printf("TPM_Process_NVWriteValue: dataSize 0, setting bWriteSTClear, bWriteDefine\n");
	    /* a. Set D1 -> bWriteSTClear to TRUE */
	    d1NvdataSensitive->pubInfo.bWriteSTClear = TRUE;
	    /* b. Set D1 -> bWriteDefine */
	    if (!d1NvdataSensitive->pubInfo.bWriteDefine) {	/* save wearout, only write if
								   FALSE */
		d1NvdataSensitive->pubInfo.bWriteDefine = TRUE;
		/* must write TPM_PERMANENT_DATA back to NVRAM, set this flag after structure is
		   written */
		writeAllNV = TRUE;
	    }
	}
	/* 15. Else (if dataSize is not 0) */
	else {
	    if (returnCode == TPM_SUCCESS) {
		/* a. Set S1 to offset + dataSize */
		s1Last = offset + data.size;	    /* set to last data point */
		/* b. If S1 > D1 -> dataSize return TPM_NOSPACE */
		if (s1Last > d1NvdataSensitive->pubInfo.dataSize) {
		    printf("TPM_Process_NVWriteValue: Error, NVRAM dataSize %u too small\n",
			   d1NvdataSensitive->pubInfo.dataSize);
		    returnCode = TPM_NOSPACE;
		}
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* c. If D1 -> attributes specifies TPM_NV_PER_WRITEALL */
		if ((d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_WRITEALL) &&
		    /* i. If dataSize != D1 -> dataSize return TPM_NOT_FULLWRITE */
		    (data.size != d1NvdataSensitive->pubInfo.dataSize)) {
		    printf("TPM_Process_NVWriteValue: Error, Must write full %u\n",
			   d1NvdataSensitive->pubInfo.dataSize);
		    returnCode = TPM_NOT_FULLWRITE;
		}
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* not GPIO */
		if (!isGPIO) {
		    /* wearout optimization, don't write if the data is the same */
		    irc = memcmp((d1NvdataSensitive->data) + offset, data.buffer, data.size);
		    if (irc != 0) {
			printf("TPM_Process_NVWriteValue: Copying data\n");
			/* d. Write the new value into the NV storage area */
			memcpy((d1NvdataSensitive->data) + offset, data.buffer, data.size);
			/* must write TPM_PERMANENT_DATA back to NVRAM, set this flag after
			   structure is written */
			writeAllNV = TRUE;
		    }
		    else {
			printf("TPM_Process_NVWriteValue: Same data, no copy\n");
		    }
		}
		/* GPIO */
		else {
		    printf("TPM_Process_NVWriteValue: Writing GPIO\n");
		    returnCode = TPM_IO_GPIO_Write(nvIndex,
						   data.size,
						   data.buffer,
						   tpm_state->tpm_number);
		}
	    }
	}
    }
    /* DIR write */
    if ((returnCode == TPM_SUCCESS) && !done && dir) {
	/* For TPM_NV_INDEX_DIR, the ordinal MUST NOT set an error code for the "if dataSize = 0"
	   action.  However, the flags set in this case are not applicable to the DIR. */
	if (data.size != 0) {
	    /* DIR is hard coded as a TPM_DIRVALUE array, TPM_NV_WRITEALL is implied */
	    if (returnCode == TPM_SUCCESS) {
		if ((offset != 0) || (data.size != TPM_DIGEST_SIZE)) {
		    printf("TPM_Process_NVWriteValue: Error, Must write full DIR %u\n",
			   TPM_DIGEST_SIZE);
		    returnCode = TPM_NOT_FULLWRITE;
		}
	    }
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_NVWriteValue: Copying data\n");
		memcpy(tpm_state->tpm_permanent_data.authDIR, data.buffer, TPM_DIGEST_SIZE);
		writeAllNV = TRUE;
	    }
	}
    }
    if ((returnCode == TPM_SUCCESS) && !done && !dir) {
	/* 16. Set D1 -> bReadSTClear to FALSE (unlocked by a successful write) */
	d1NvdataSensitive->pubInfo.bReadSTClear = FALSE;
    }
    /* 15.d Write the new value into the NV storage area */
    if (writeAllNV) {
	printf("TPM_Process_NVWriteValue: Writing data to NVRAM\n");
	/* NOTE Don't do this step until just before the serialization */
	/* e. If NV1_INCREMENTED is TRUE */
	if (nv1Incremented) {
	    /* i. Set TPM_PERMANENT_DATA -> noOwnerNVWrite to NV1 */
	    tpm_state->tpm_permanent_data.noOwnerNVWrite = nv1;
	}
    }
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV,
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_NVWriteValue: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,	/* owner HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&data);			/* @1 */
    return rcf;
}

/* 20.3 TPM_NV_WriteValueAuth rev 87
	
   This command writes to a previously defined area. The area must require authorization to
   write. This command is for using when authorization other than the owner authorization is to be
   used. Otherwise, you should use TPM_NV_WriteValue
*/

TPM_RESULT TPM_Process_NVWriteValueAuth(tpm_state_t *tpm_state,
					TPM_STORE_BUFFER *response,
					TPM_TAG tag,
					uint32_t paramSize,
					TPM_COMMAND_CODE ordinal,
					unsigned char *command,
					TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */
    int		irc;

    /* input parameters */
    TPM_NV_INDEX	nvIndex;	/* The index of the area to set */
    uint32_t		offset = 0;	/* The offset into the chunk */
    TPM_SIZED_BUFFER	data;		/* The data to set the area to */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for NV element
					   authorization */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA	authValue;	/* HMAC key: NV element auth value */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus = FALSE;	/* audit the ordinal */
    TPM_BOOL			transportEncrypt = FALSE;	/* wrapped in encrypted transport
								   session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey = NULL;
    TPM_NV_DATA_SENSITIVE	*d1NvdataSensitive;
    uint32_t			s1Last;
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back NV */
    TPM_BOOL			physicalPresence;
    TPM_BOOL			isGPIO;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_NVWriteValueAuth: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&data);			/* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get nvIndex parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&nvIndex, &command, &paramSize);
    }
    /* get offset parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&offset, &command, &paramSize);
    }
    /* get data parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&data, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALL);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag1(tag);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					authValue,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_NVWriteValueAuth: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* determine whether the nvIndex is legal GPIO space */
    if (returnCode == 0) {
	returnCode = TPM_NVDataSensitive_IsGPIO(&isGPIO, nvIndex);
    }
    /* 1. Locate and set D1 to the TPM_NV_DATA_AREA that corresponds to nvIndex, return TPM_BADINDEX
       on error */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_NVWriteValueAuth: index %08x offset %u dataSize %u\n",
	       nvIndex, offset, data.size);
	TPM_PrintFourLimit("TPM_Process_NVWriteValueAuth: data", data.buffer, data.size);
	printf("TPM_Process_NVWriteValueAuth: Loading data from NVRAM\n");
	returnCode = TPM_NVIndexEntries_GetEntry(&d1NvdataSensitive,
						 &(tpm_state->tpm_nv_index_entries),
						 nvIndex);
	if (returnCode != 0) {
	    printf("TPM_Process_NVWriteValueAuth: Error, NV index %08x not found\n", nvIndex);
	}
    }
    /* 2. If D1 -> attributes does not specify TPM_NV_PER_AUTHWRITE then return TPM_AUTH_CONFLICT */
    if (returnCode == TPM_SUCCESS) {
	if (!(d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_AUTHWRITE)) {
	    printf("TPM_Process_NVWriteValueAuth: Error, authorization conflict\n");
	    returnCode = TPM_AUTH_CONFLICT;
	}
    }
    /* 3. Validate authValue using D1 -> authValue, return TPM_AUTHFAIL on error */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_NV,
					      ordinal,
					      NULL,
					      &(d1NvdataSensitive->authValue),	/* OIAP */
					      d1NvdataSensitive->digest);	/* OSAP */
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					authValue);		/* Authorization digest for input */
    }
    /* 4. Check that D1 -> pcrInfoWrite -> localityAtRelease for TPM_STANY_DATA -> localityModifier
       is TRUE */
    /* a. For example if TPM_STANY_DATA -> localityModifier was 2 then D1 -> pcrInfo ->
       localityAtRelease -> TPM_LOC_TWO would have to be TRUE */
    /* b. On error return TPM_BAD_LOCALITY */
    /* NOTE Done by TPM_PCRInfoShort_CheckDigest() */
    /* 5. If D1 -> attributes specifies TPM_NV_PER_PPWRITE then validate physical presence is
       asserted if not return TPM_BAD_PRESENCE */
    if (returnCode == TPM_SUCCESS) {
	if (d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_PPWRITE) {
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_Global_GetPhysicalPresence(&physicalPresence, tpm_state);
	    }
	    if (returnCode == TPM_SUCCESS) {
		if (!physicalPresence) {
		    printf("TPM_Process_NVWriteValueAuth: Error, physicalPresence is FALSE\n");
		    returnCode = TPM_BAD_PRESENCE;
		}
	    }
	}
    }
    /* 6. If D1 -> pcrInfoWrite -> pcrSelection specifies a selection of PCR */
    /* a. Create P1 a composite hash of the PCR specified by D1 -> pcrInfoWrite */
    /* b. Compare P1 to digestAtRelease return TPM_WRONGPCRVAL on mismatch */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_PCRInfoShort_CheckDigest(&(d1NvdataSensitive->pubInfo.pcrInfoWrite),
						  tpm_state->tpm_stclear_data.PCRS,
						  tpm_state->tpm_stany_flags.localityModifier);
    }
    if (returnCode == TPM_SUCCESS) {
	/* 7. If D1 -> attributes specifies TPM_NV_PER_WRITEDEFINE */
	if ((d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_WRITEDEFINE) &&
	    /* a. If D1 -> bWriteDefine is TRUE return TPM_AREA_LOCKED */
	    (d1NvdataSensitive->pubInfo.bWriteDefine)) {
	    printf("TPM_Process_NVWriteValueAuth: Error, area locked by bWriteDefine\n");
	    returnCode = TPM_AREA_LOCKED;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* 8. If D1 -> attributes specifies TPM_NV_PER_GLOBALLOCK */
	if ((d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_GLOBALLOCK) &&
	    /* a. If TPM_STCLEAR_FLAGS -> bGlobalLock is TRUE return TPM_AREA_LOCKED */
	    (tpm_state->tpm_stclear_flags.bGlobalLock)) {
	    printf("TPM_Process_NVWriteValueAuth: Error, area locked by bGlobalLock\n");
	    returnCode = TPM_AREA_LOCKED;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* 9. If D1 -> attributes specifies TPM_NV_PER_WRITE_STCLEAR */
	if ((d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_WRITE_STCLEAR) &&
	    /* a. If D1 -> bWriteSTClear is TRUE return TPM_AREA_LOCKED */
	    (d1NvdataSensitive->pubInfo.bWriteSTClear)) {
	    printf("TPM_Process_NVWriteValueAuth: Error, area locked by bWriteSTClear\n");
	    returnCode = TPM_AREA_LOCKED;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* 10. If dataSize = 0 then */
	if (data.size == 0) {
	    printf("TPM_Process_NVWriteValueAuth: "
		   "dataSize 0, setting bWriteSTClear, bWriteDefine\n");
	    /* a. Set D1 -> bWriteSTClear to TRUE */
	    d1NvdataSensitive->pubInfo.bWriteSTClear = TRUE;
	    /* b. Set D1 -> bWriteDefine to TRUE */
	    if (!d1NvdataSensitive->pubInfo.bWriteDefine) {	/* save wearout, only write if
								   FALSE */
		d1NvdataSensitive->pubInfo.bWriteDefine = TRUE;
		/* must write TPM_PERMANENT_DATA back to NVRAM, set this flag after structure is
		   written */
		writeAllNV = TRUE;
	    }
	}
	/* 11. Else (if dataSize is not 0) */
	else {
	    if (returnCode == TPM_SUCCESS) {
		/* a. Set S1 to offset + dataSize */
		s1Last = offset + data.size;	    /* set to last data point */
		/* b. If S1 > D1 -> dataSize return TPM_NOSPACE */
		if (s1Last > d1NvdataSensitive->pubInfo.dataSize) {
		    printf("TPM_Process_NVWriteValueAuth: Error, NVRAM dataSize %u\n",
			   d1NvdataSensitive->pubInfo.dataSize);
		    returnCode = TPM_NOSPACE;
		}
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* c. If D1 -> attributes specifies TPM_PER_WRITEALL */
		if ((d1NvdataSensitive->pubInfo.permission.attributes & TPM_NV_PER_WRITEALL) &&
		    /* i. If dataSize != D1 -> dataSize return TPM_NOT_FULLWRITE */
		    (data.size != d1NvdataSensitive->pubInfo.dataSize)) {
		    printf("TPM_Process_NVWriteValueAuth: Error, Must write all %u\n",
			   d1NvdataSensitive->pubInfo.dataSize);
		    returnCode = TPM_NOT_FULLWRITE;
		}
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* not GPIO */
		if (!isGPIO) {
		    /* wearout optimization, don't write if the data is the same */
		    irc = memcmp((d1NvdataSensitive->data) + offset, data.buffer, data.size);
		    if (irc != 0) {
			/* d. Write the new value into the NV storage area */
			printf("TPM_Process_NVWriteValueAuth: Copying data\n");
			memcpy((d1NvdataSensitive->data) + offset, data.buffer, data.size);
			/* must write TPM_PERMANENT_DATA back to NVRAM, set this flag after
			   structure is written */
			writeAllNV = TRUE;
		    }
		    else {
			printf("TPM_Process_NVWriteValueAuth: Same data, no copy\n");
		    }
		}
		/* GPIO */
		else {
		    printf("TPM_Process_NVWriteValueAuth: Writing GPIO\n");
		    returnCode = TPM_IO_GPIO_Write(nvIndex,
						   data.size,
						   data.buffer,
						   tpm_state->tpm_number);
		}
	    }
	}
    }
    /* 12. Set D1 -> bReadSTClear to FALSE */
    if (returnCode == TPM_SUCCESS) {
	d1NvdataSensitive->pubInfo.bReadSTClear = FALSE;
	printf("TPM_Process_NVWriteValueAuth: Writing data to NVRAM\n");
    }
    /* write back TPM_PERMANENT_DATA if required */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV,
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_NVWriteValueAuth: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&data);			/* @1 */
    return rcf;
}

/* 20.1 TPM_NV_DefineSpace rev 109

   This establishes the space necessary for the indicated index.  The definition will include the
   access requirements for writing and reading the area.

	Previously defined space at the index and new size is non-zero (and space is available,
	etc.) -> redefine the index

	No previous space at the index and new size is non-zero (and space is available, etc.)->
	define the index

	Previously defined space at the index and new size is 0 -> delete the index

	No previous space at the index and new size is 0 -> error
   
   The space definition size does not include the area needed to manage the space.

   Setting TPM_PERMANENT_FLAGS -> nvLocked TRUE when it is already TRUE is not an error.

   For the case where pubInfo -> dataSize is 0, pubInfo -> pcrInfoRead and pubInfo -> pcrInfoWrite
   are not used.  However, since the general principle is to validate parameters before changing
   state, the TPM SHOULD parse pubInfo completely before invalidating the data area.
*/

TPM_RESULT TPM_Process_NVDefineSpace(tpm_state_t *tpm_state,
				     TPM_STORE_BUFFER *response,
				     TPM_TAG tag,
				     uint32_t paramSize,
				     TPM_COMMAND_CODE ordinal,
				     unsigned char *command,
				     TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_NV_INDEX	newNVIndex = TPM_NV_INDEX_LOCK;	/* from input TPM_NV_DATA_PUBLIC, initialize
							   to silence compiler */
    TPM_ENCAUTH		encAuth;	/* The encrypted AuthData, only valid if the attributes
					   require subsequent authorization */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for ownerAuth */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA	ownerAuth;	/* The authorization session digest HMAC key: ownerAuth */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey = NULL;
    TPM_BOOL			ignore_auth = FALSE;
    TPM_BOOL			writeAllNV = FALSE;		/* flag to write back NV */
    TPM_BOOL			done = FALSE;			/* processing is done */
    TPM_DIGEST			a1Auth;
    TPM_NV_DATA_SENSITIVE	*d1_old;			/* possibly old data */
    TPM_NV_DATA_SENSITIVE	*d1_new = NULL;			/* new data */
    TPM_NV_DATA_PUBLIC 		*pubInfo = NULL;		/* new, initialize to silence
								   compiler */
    uint32_t 			freeSpace;			/* free space after allocating new
								   index */
    TPM_BOOL			writeLocalities = FALSE;
    TPM_BOOL			physicalPresence;
    TPM_BOOL			foundOld = TRUE;		/* index already exists, initialize
								   to silence compiler */
    uint32_t			nv1 = tpm_state->tpm_permanent_data.noOwnerNVWrite;
						/* temp for noOwnerNVWrite, initialize to silence 
						   compiler */
    TPM_BOOL			nv1Incremented = FALSE;		/* flag that nv1 was incremented */
    
    /* output parameters  */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_NVDefineSpace: Ordinal Entry\n");
    /* This design gets a slot in the TPM_NV_INDEX_ENTRIES array, either an existing empty one or a
       newly re'allocated one.  The incoming parameters are deserialized directly into the slot.

       On success, the slot remains.  On failure, the slot is deleted.  There is no need to remove
       the slot from the array.  It can remain for the next call.
    */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get or create a free index in the TPM_NV_INDEX_ENTRIES array */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_NVIndexEntries_GetFreeEntry(&d1_new, &(tpm_state->tpm_nv_index_entries));
    }    
    /* get pubInfo parameter */
    if (returnCode == TPM_SUCCESS) {
	pubInfo = &(d1_new->pubInfo);	/* pubInfo is an input parameter */
	returnCode = TPM_NVDataPublic_Load(pubInfo,
					   &command, &paramSize,
					   FALSE);	/* not optimized for digestAtRelease */
	/* The NV index cannot be immediately deserialized in the slot, or the function will think
	   that the index already exists.  Therefore, the nvIndex parameter is saved and temporarily
	   set to empty until the old slot is deleted. */
	newNVIndex = pubInfo->nvIndex;		/* save the possibly new index */
	pubInfo->nvIndex = TPM_NV_INDEX_LOCK;	/* temporarily mark unused */
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_NVDefineSpace: index %08x permission %08x dataSize %08x\n",
	       newNVIndex, pubInfo->permission.attributes, pubInfo->dataSize);
	TPM_PCRInfo_Trace("TPM_Process_NVDefineSpace: pcrInfoRead",
			  pubInfo->pcrInfoRead.pcrSelection,
			  pubInfo->pcrInfoRead.digestAtRelease);
	TPM_PCRInfo_Trace("TPM_Process_NVDefineSpace: pcrInfoWrite",
			  pubInfo->pcrInfoWrite.pcrSelection,
			  pubInfo->pcrInfoWrite.digestAtRelease);
	/* get encAuth parameter */
	returnCode = TPM_Secret_Load(encAuth, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALLOW_NO_OWNER | TPM_CHECK_NV_NOAUTH);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag10(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_NVDefineSpace: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. If pubInfo -> nvIndex == TPM_NV_INDEX_LOCK and tag = TPM_TAG_RQU_COMMAND */
    if ((returnCode == TPM_SUCCESS) &&
	(newNVIndex == TPM_NV_INDEX_LOCK) &&
	(tag == TPM_TAG_RQU_COMMAND)) {
	/* a. If pubInfo -> dataSize is not 0, the command MAY return TPM_BADINDEX. */
	if (pubInfo->dataSize != 0) {
	    printf("TPM_Process_NVDefineSpace: Error, TPM_NV_INDEX_LOCK dataSize %u\n",
		   pubInfo->dataSize);
	    returnCode = TPM_BADINDEX;
	}
	else {
	    /* b. Set TPM_PERMANENT_FLAGS -> nvLocked to TRUE */
	    /* writeAllNV set to TRUE if nvLocked is being set, not if already set */
	    printf("TPM_Process_NVDefineSpace: Setting nvLocked\n");
	    TPM_SetCapability_Flag(&writeAllNV,					/* altered */
				   &(tpm_state->tpm_permanent_flags.nvLocked ), /* flag */
				   TRUE);					/* value */
	}
	/* c. Return TPM_SUCCESS */
	done = TRUE;
    }
    /* 2. If TPM_PERMANENT_FLAGS -> nvLocked is FALSE then all authorization checks except for the
       Max NV writes are ignored */
    /* a. Ignored checks include physical presence, owner authorization, 'D' bit check, bGlobalLock,
       no authorization with a TPM owner present, bWriteSTClear, the check that pubInfo -> dataSize
       is 0 in Action 5.c. (the no-authorization case), disabled and deactivated. */
    /* NOTE: The disabled and deactivated flags are conditionally checked by TPM_CheckState() using
       the TPM_CHECK_NV_NOAUTH flag */
    /* ii. The check that pubInfo -> dataSize is 0 is still enforced in Action 6.f. (returning after
       deleting a previously defined storage area) and Action 9.f. (not allowing a space of size 0
       to be defined). */
    /* i.If ownerAuth is present, the TPM MAY check the authorization HMAC. */
    if (returnCode == TPM_SUCCESS) {
	if (!(tpm_state->tpm_permanent_flags.nvLocked)) {
	    printf("TPM_Process_NVDefineSpace: nvLocked FALSE, ignoring authorization\n");
	    ignore_auth = TRUE;
	}
    }
    /*	b.The check for pubInfo -> nvIndex == 0 in Action 3. is not ignored. */
    if ((returnCode == TPM_SUCCESS) && !done) {
	if (newNVIndex == TPM_NV_INDEX0) {
	    printf("TPM_Process_NVDefineSpace: Error, bad index %08x\n", newNVIndex);
	    returnCode = TPM_BADINDEX;
	}
    }
    /* 3. If pubInfo -> nvIndex has the D bit (bit 28) set to a 1 or pubInfo -> nvIndex == 0 then */
    if ((returnCode == TPM_SUCCESS) && !done && !ignore_auth) {
	/* b. The D bit specifies an index value that is set in manufacturing and can never be
	   deleted or added to the TPM */
	if (newNVIndex & TPM_NV_INDEX_D_BIT) {
	    /* c. Index value of 0 is reserved and cannot be defined */
	    /* a. Return TPM_BADINDEX */
	    printf("TPM_Process_NVDefineSpace: Error, bad index %08x\n", newNVIndex);
	    returnCode = TPM_BADINDEX;
	}
    }
    /* 4. If tag = TPM_TAG_RQU_AUTH1_COMMAND then */
    /* b. authHandle session type MUST be OSAP */
    /* must get the HMAC key for the response even if ignore_auth is TRUE */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_OSAP,
					      TPM_ET_OWNER,
					      ordinal,
					      NULL,
					      NULL,
					      tpm_state->tpm_permanent_data.ownerAuth);
    }
    /* a. The TPM MUST validate the command and parameters using the TPM Owner authentication and
       ownerAuth, on error return TPM_AUTHFAIL */
    /* NOTE: This is optional if ignore_auth is TRUE */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND) && !done) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					ownerAuth);		/* Authorization digest for input */
    }
    /* c. Create A1 by decrypting encAuth according to the ADIP indicated by authHandle. */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND) && !done) {
	returnCode = TPM_AuthSessionData_Decrypt(a1Auth,
						 NULL,
						 encAuth,
						 auth_session_data,
						 NULL,
						 NULL,
						 FALSE);	/* even and odd */
    }
    /* 5. else (not auth1) */
    /* a. Validate the assertion of physical presence. Return TPM_BAD_PRESENCE on error. */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND) && !done && !ignore_auth) {
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Global_GetPhysicalPresence(&physicalPresence, tpm_state);
	}
	if (returnCode == TPM_SUCCESS) {
	    if (!physicalPresence) {
		printf("TPM_Process_NVDefineSpace: Error, physicalPresence is FALSE\n");
		returnCode = TPM_BAD_PRESENCE;
	    }
	}
    }
    /* b. If TPM Owner is present then return TPM_OWNER_SET. */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND) && !done && !ignore_auth) {
	if (tpm_state->tpm_permanent_data.ownerInstalled) {
	    printf("TPM_Process_NVDefineSpace: Error, no authorization, but owner installed\n");
	    returnCode = TPM_OWNER_SET;
	}
    }
    /* c. If pubInfo -> dataSize is 0 then return TPM_BAD_DATASIZE. Setting the size to 0 represents
       an attempt to delete the value without TPM Owner authentication. */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND) && !done && !ignore_auth) {
	if (pubInfo->dataSize == 0) {
	    printf("TPM_Process_NVDefineSpace: Error, no owner authorization and dataSize 0\n");
	    returnCode = TPM_BAD_DATASIZE;
	}
    }
    /* d. Validate max NV writes without an owner */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND) && !done) {
	/* i. Set NV1 to TPM_PERMANENT_DATA -> noOwnerNVWrite */
	nv1 = tpm_state->tpm_permanent_data.noOwnerNVWrite;
	/* ii. Increment NV1 by 1 */
	nv1++;
	/* iii. If NV1 > TPM_MAX_NV_WRITE_NOOWNER return TPM_MAXNVWRITES */
	if (nv1 > TPM_MAX_NV_WRITE_NOOWNER) {
	    printf("TPM_Process_NVDefineSpace: Error, max NV writes %d w/o owner reached\n",
		   tpm_state->tpm_permanent_data.noOwnerNVWrite);
	    returnCode = TPM_MAXNVWRITES;
	}
	else {
	    /* iv. Set NV1_INCREMENTED to TRUE */
	    nv1Incremented = TRUE;
	}
    }
    /* e. Set A1 to encAuth. There is no nonce or authorization to create the encryption string,
       hence the AuthData value is passed in the clear */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND) && !done) {
	TPM_Digest_Copy(a1Auth, encAuth);
    }
    /* 6. If pubInfo -> nvIndex points to a valid previously defined storage area then */
    /* 6.a. Map D1 a TPM_NV_DATA_SENSITIVE to the storage area */
    if ((returnCode == TPM_SUCCESS) && !done) {
	printf("TPM_Process_NVDefineSpace: Loading existing NV index %08x\n", newNVIndex);
	returnCode = TPM_NVIndexEntries_GetEntry(&d1_old,
						 &(tpm_state->tpm_nv_index_entries),
						 newNVIndex);
	if (returnCode == TPM_SUCCESS) {
	    printf("TPM_Process_NVDefineSpace: NV index %08x exists\n", newNVIndex);
	    foundOld = TRUE;
	}
	else if (returnCode == TPM_BADINDEX) {
	    returnCode = TPM_SUCCESS;	/* non-existent index is not an error */
	    foundOld = FALSE;
	    printf("TPM_Process_NVDefineSpace: Index %08x is new\n", newNVIndex);
	}
    }
    if ((returnCode == TPM_SUCCESS) && !done && !ignore_auth && foundOld) {
	/* 6.b. If D1 -> attributes specifies TPM_NV_PER_GLOBALLOCK then */
	if (d1_old->pubInfo.permission.attributes & TPM_NV_PER_GLOBALLOCK) {
	    /* i. If TPM_STCLEAR_FLAGS -> bGlobalLock is TRUE then return TPM_AREA_LOCKED */
	    if (tpm_state->tpm_stclear_flags.bGlobalLock) {
		printf("TPM_Process_NVDefineSpace: Error, index %08x (bGlobalLock) locked\n",
		       newNVIndex);
		returnCode = TPM_AREA_LOCKED;
	    }
	}
    }
    if ((returnCode == TPM_SUCCESS) && !done && !ignore_auth && foundOld) {
	/* 6.c. If D1 -> attributes specifies TPM_NV_PER_WRITE_STCLEAR */
	if (d1_old->pubInfo.permission.attributes & TPM_NV_PER_WRITE_STCLEAR) {
	    /* i. If D1 -> pubInfo -> bWriteSTClear is TRUE then return TPM_AREA_LOCKED */
	    if (d1_old->pubInfo.bWriteSTClear) {
		printf("TPM_Process_NVDefineSpace: Error, area locked by bWriteSTClear\n");
		returnCode = TPM_AREA_LOCKED;
	    }
	}
    }
    /* NOTE Changed the Action order.  Must terminate auth sessions while the old index digest
       still exists.
    */
    /* 6.f. The TPM invalidates authorization sessions */
    /* i. MUST invalidate all authorization sessions associated with D1 */
    /* ii. MAY invalidate any other authorization session */
    if ((returnCode == TPM_SUCCESS) && !done && foundOld) {
	TPM_AuthSessions_TerminateEntity(&continueAuthSession,
					 authHandle,
					 tpm_state->tpm_stclear_data.authSessions,
					 TPM_ET_NV,
					 &(d1_old->digest));
    }
    if ((returnCode == TPM_SUCCESS) && !done && foundOld) {
	/* 6.d. Invalidate the data area currently pointed to by D1 and ensure that if the area is
	   reallocated no residual information is left */
	printf("TPM_Process_NVDefineSpace: Deleting index %08x\n", newNVIndex);
	TPM_NVDataSensitive_Delete(d1_old);
	/* must write deleted space back to NVRAM */
	writeAllNV = TRUE;
	/* 6.e. If NV1_INCREMENTED is TRUE */
	/* i. Set TPM_PERMANENT_DATA -> noOwnerNVWrite to NV1 */
	/* NOTE Don't do this step until just before the serialization */
    }
    /* g. If pubInfo -> dataSize is 0 then return TPM_SUCCESS */
    if ((returnCode == TPM_SUCCESS) && !done && foundOld) {
	if (pubInfo->dataSize == 0) {
	    printf("TPM_Process_NVDefineSpace: Size 0, done\n");
	    done = TRUE;
	}
    }
    /* 7. Parse pubInfo -> pcrInfoRead */
    /* a. Validate pcrInfoRead structure on error return TPM_INVALID_STRUCTURE */
    /* i. Validation includes proper PCR selections and locality selections */
    /* NOTE: Done by TPM_NVDataPublic_Load() */
    /* 8. Parse pubInfo -> pcrInfoWrite */
    /* a. Validate pcrInfoWrite structure on error return TPM_INVALID_STRUCTURE */
    /* i. Validation includes proper PCR selections and locality selections */
    /* NOTE: Done by TPM_NVDataPublic_Load() */
    if ((returnCode == TPM_SUCCESS) && !done) {
	/* b. If pcrInfoWrite -> localityAtRelease disallows some localities */
	if (pubInfo->pcrInfoRead.localityAtRelease != TPM_LOC_ALL) {
	    /* i. Set writeLocalities to TRUE */
	    writeLocalities = TRUE;
	}
	/* c. Else */
	else {
	    /* i. Set writeLocalities to FALSE */
	    writeLocalities = FALSE;
	}
    }
    /* 9. Validate that the attributes are consistent */
    /* a. The TPM SHALL ignore the bReadSTClear, bWriteSTClear and bWriteDefine attributes during
       the execution of this command */
    /* b. If TPM_NV_PER_OWNERWRITE is TRUE and TPM_NV_PER_AUTHWRITE is TRUE return TPM_AUTH_CONFLICT
       */
    if ((returnCode == TPM_SUCCESS) && !done) {
	if ((pubInfo->permission.attributes & TPM_NV_PER_OWNERWRITE) &&
	    (pubInfo->permission.attributes & TPM_NV_PER_AUTHWRITE)) {
	    printf("TPM_Process_NVDefineSpace: Error, write authorization conflict\n");
	    returnCode = TPM_AUTH_CONFLICT;
	}
    }
    /* c. If TPM_NV_PER_OWNERREAD is TRUE and TPM_NV_PER_AUTHREAD is TRUE return TPM_AUTH_CONFLICT
       */
    if ((returnCode == TPM_SUCCESS) && !done) {
	if ((pubInfo->permission.attributes & TPM_NV_PER_OWNERREAD) &&
	    (pubInfo->permission.attributes & TPM_NV_PER_AUTHREAD)) {
	    printf("TPM_Process_NVDefineSpace: Error, read authorization conflict\n");
	    returnCode = TPM_AUTH_CONFLICT;
	}
    }
    /* d. If TPM_NV_PER_OWNERWRITE and TPM_NV_PER_AUTHWRITE and TPM_NV_PER_WRITEDEFINE and
       TPM_NV_PER_PPWRITE and writeLocalities are all FALSE */
    if ((returnCode == TPM_SUCCESS) && !done) {
	if (!(pubInfo->permission.attributes & TPM_NV_PER_OWNERWRITE) &&
	    !(pubInfo->permission.attributes & TPM_NV_PER_AUTHWRITE) &&
	    !(pubInfo->permission.attributes & TPM_NV_PER_WRITEDEFINE) &&
	    !(pubInfo->permission.attributes & TPM_NV_PER_PPWRITE) &&
	    !writeLocalities) {
	    /* i. Return TPM_PER_NOWRITE */
	    printf("TPM_Process_NVDefineSpace: Error, no write\n");
	    returnCode = TPM_PER_NOWRITE;
	}
    }
    /* e. Validate pubInfo -> nvIndex */
    /* i. Make sure that the index is applicable for this TPM return TPM_BADINDEX on error */
    if ((returnCode == TPM_SUCCESS) && !done) {
	returnCode = TPM_NVDataSensitive_IsValidIndex(newNVIndex);
    }
    /* f. If dataSize is 0 return TPM_BAD_PARAM_SIZE */
    if ((returnCode == TPM_SUCCESS) && !done) {
	if (pubInfo->dataSize == 0) {
	    printf("TPM_Process_NVDefineSpace: Error, New index data size is zero\n");
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* 10. Create D1 a TPM_NV_DATA_SENSITIVE structure */
    /* NOTE Created and initialized d1_new directly in the TPM_NV_INDEX_ENTRIES array */
    /* a. Set D1 -> pubInfo to pubInfo */
    /* NOTE deserialized in place */
    if ((returnCode == TPM_SUCCESS) && !done) {
	/* b. Set D1 -> authValue to A1 */
	TPM_Digest_Copy(d1_new->authValue, a1Auth);
	/* c. Set D1 -> pubInfo -> bReadSTClear to FALSE */
	/* d. Set D1 -> pubInfo -> bWriteSTClear to FALSE */
	/* e. Set D1 -> pubInfo -> bWriteDefine to FALSE */
	pubInfo->bReadSTClear = FALSE;
	pubInfo->bWriteSTClear  = FALSE;
	pubInfo->bWriteDefine  = FALSE;
    }
    if ((returnCode == TPM_SUCCESS) && !done) {
	/* assign the empty slot to the index now so it will be counted as used space during the
	   serialization. */
	pubInfo->nvIndex = newNVIndex;
	/* 12.a. Reserve NV space for pubInfo -> dataSize

	   NOTE: Action is out or order.  Must allocate data space now so that the serialization
	   inherent in TPM_NVIndexEntries_GetFreeSpace() is valid
	*/
	returnCode = TPM_Malloc(&(d1_new->data), pubInfo->dataSize);
    }
    /* 11. Validate that sufficient NV is available to store D1 and pubInfo -> dataSize bytes of
       data*/
    /* a. return TPM_NOSPACE if pubInfo -> dataSize is not available in the TPM */
    if ((returnCode == TPM_SUCCESS) && !done) {
	printf("TPM_Process_NVDefineSpace: Allocated %u data bytes at %p\n",
	       pubInfo->dataSize, d1_new->data);
	printf("TPM_Process_NVDefineSpace: Checking for %u bytes free space\n", pubInfo->dataSize);
	returnCode = TPM_NVIndexEntries_GetFreeSpace(&freeSpace,
						     &(tpm_state->tpm_nv_index_entries));
	if (returnCode != TPM_SUCCESS) {
	    printf("TPM_Process_NVDefineSpace: Error: No space\n");
	}
    }
     /* if there is no free space, free the NV index in-memory structure.  This implicitly removes
       the entry from tpm_nv_index_entries.  If pubInfo -> nvIndex is TPM_NV_INDEX_TRIAL, the entry
       should also be removed. */
    if ((returnCode != TPM_SUCCESS) ||
	(newNVIndex == TPM_NV_INDEX_TRIAL)) {
	if (newNVIndex == TPM_NV_INDEX_TRIAL) {
	    printf("TPM_Process_NVDefineSpace: nvIndex is TPM_NV_INDEX_TRIAL, done\n");
	    /* don't actually write, just return success or failure */
	    done = TRUE;
	}
	TPM_NVDataSensitive_Delete(d1_new);
    }
    /* 12. If pubInfo -> nvIndex is not TPM_NV_INDEX_TRIAL  */
    if ((returnCode == TPM_SUCCESS) && !done) {
	printf("TPM_Process_NVDefineSpace: Creating index %08x\n", newNVIndex);
	/* b. Set all bytes in the newly defined area to 0xFF */
	memset(d1_new->data, 0xff, pubInfo->dataSize);
	/* must write newly defined space back to NVRAM */
	writeAllNV = TRUE;
    }
    if (returnCode == TPM_SUCCESS) {
	/* c. If NV1_INCREMENTED is TRUE */
	if (nv1Incremented) {
	    /* i. Set TPM_PERMANENT_DATA -> noOwnerNVWrite to NV1 */
	    tpm_state->tpm_permanent_data.noOwnerNVWrite = nv1;
	}	    
	/* 13. Ignore continueAuthSession on input and set to FALSE on output */
	continueAuthSession = FALSE;
    }
    /* write the file to NVRAM */
    /* write back TPM_PERMANENT_DATA and TPM_PERMANENT_FLAGS if required */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV,
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_NVDefineSpace: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* owner HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    return rcf;
}

/* 27.3 DIR commands rev 87

   The DIR commands are replaced by the NV storage commands.
   
   The DIR [0] in 1.1 is now TPM_PERMANENT_DATA -> authDIR[0] and is always available for the TPM to
   use. It is accessed by DIR commands using dirIndex 0 and by NV commands using nvIndex
   TPM_NV_INDEX_DIR.
   
   If the TPM vendor supports additional DIR registers, the TPM vendor may return errors or provide
   vendor specific mappings for those DIR registers to NV storage locations.

   1. A dirIndex value of 0 MUST corresponds to an NV storage nvIndex value TPM_NV_INDEX_DIR.

   2. The TPM vendor MAY return errors or MAY provide vendor specific mappings for DIR dirIndex
   values greater than 0 to NV storage locations.
*/

/* 27.3.1 TPM_DirWriteAuth rev 87

   The TPM_DirWriteAuth operation provides write access to the Data Integrity Registers. DIRs are
   non-volatile memory registers held in a TPM-shielded location. Owner authentication is required
   to authorize this action.

   Access is also provided through the NV commands with nvIndex TPM_NV_INDEX_DIR.  Owner
   authorization is not required when nvLocked is FALSE.

   Version 1.2 requires only one DIR. If the DIR named does not exist, the TPM_DirWriteAuth
   operation returns TPM_BADINDEX.
*/

TPM_RESULT TPM_Process_DirWriteAuth(tpm_state_t *tpm_state,
				    TPM_STORE_BUFFER *response,
				    TPM_TAG tag,
				    uint32_t paramSize,
				    TPM_COMMAND_CODE ordinal,
				    unsigned char *command,
				    TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_DIRINDEX	dirIndex;	/* Index of the DIR */
    TPM_DIRVALUE	newContents;	/* New value to be stored in named DIR */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for command. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA	ownerAuth;	/* The authorization session digest for inputs. HMAC key:
					   ownerAuth. */

    /* processing parameters  */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */

    /* output parameters  */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_DirWriteAuth: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get dirIndex parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&dirIndex, &command, &paramSize);
    }
    /* get newContents parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DirWriteAuth: dirIndex %08x\n", dirIndex);
	returnCode = TPM_Digest_Load(newContents, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_DirWriteAuth: newContents", newContents);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALLOW_NO_OWNER);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag1(tag);
    }
    /* get the 'below the line' authorization parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_DirWriteAuth: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /* do not terminate sessions if the command did not parse correctly */
    if (returnCode != TPM_SUCCESS) {
	authHandleValid = FALSE;
    }
    /*
      Processing
    */
    /* 1. Validate that authHandle contains a TPM Owner AuthData to execute the TPM_DirWriteAuth
       command */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_OWNER,
					      ordinal,
					      NULL,
					      &(tpm_state->tpm_permanent_data.ownerAuth), /* OIAP */
					      tpm_state->tpm_permanent_data.ownerAuth);	  /* OSAP */
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					ownerAuth);		/* Authorization digest for input */
    }
    /* 2. Validate that dirIndex points to a valid DIR on this TPM */
    if (returnCode == TPM_SUCCESS) {
	if (dirIndex != 0) {	/* only one TPM_PERMANENT_DATA -> authDIR */
	    printf("TPM_Process_DirWriteAuth: Error, Invalid index %08x\n", dirIndex);
	    returnCode = TPM_BADINDEX;
	}
    }
    /* 3. Write newContents into the DIR pointed to by dirIndex */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DirWriteAuth: Writing data\n");
	TPM_Digest_Copy(tpm_state->tpm_permanent_data.authDIR, newContents);
	/* write back TPM_PERMANENT_DATA */
	returnCode = TPM_PermanentAll_NVStore(tpm_state,
					      TRUE,
					      returnCode);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DirWriteAuth: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* calculate and set the below the line parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_AuthParams_Set(response,
					    *hmacKey,		/* owner HMAC key */
					    auth_session_data,
					    outParamDigest,
					    nonceOdd,
					    continueAuthSession);
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /* if there was an error, or continueAuthSession is FALSE, terminate the session */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    return rcf;
}

/* 27.3.2 TPM_DirRead rev 87

   The TPM_DirRead operation provides read access to the DIRs. No authentication is required to
   perform this action because typically no cryptographically useful AuthData is available early in
   boot. TSS implementors may choose to provide other means of authorizing this action. Version 1.2
   requires only one DIR. If the DIR named does not exist, the TPM_DirRead operation returns
   TPM_BADINDEX.
*/

TPM_RESULT TPM_Process_DirRead(tpm_state_t *tpm_state,
			       TPM_STORE_BUFFER *response,
			       TPM_TAG tag,
			       uint32_t paramSize,
			       TPM_COMMAND_CODE ordinal,
			       unsigned char *command,
			       TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_DIRINDEX	dirIndex;	/* Index of the DIR to be read */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_DirRead: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get dirIndex parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&dirIndex, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DirRead: dirIndex %08x\n", dirIndex);
    }
    /* save the ending point of inParam's for authorization and auditing */
    inParamEnd = command;
    /* digest the input parameters */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetInParamDigest(inParamDigest,	/* output */
					  &auditStatus,		/* output */
					  &transportEncrypt,	/* output */
					  tpm_state,
					  tag,
					  ordinal,
					  inParamStart,
					  inParamEnd,
					  transportInternal);
    }
    /* check state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_ALLOW_NO_OWNER);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_DirRead: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Validate that dirIndex points to a valid DIR on this TPM */
    if (returnCode == TPM_SUCCESS) {
	if (dirIndex != 0) {		/* only one TPM_PERMANENT_DATA -> authDIR */
	    printf("TPM_Process_DirRead: Error, Invalid index %08x\n", dirIndex);
	    returnCode = TPM_BADINDEX;
	}
    }
    /* 2. Return the contents of the DIR in dirContents */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_DirRead: Reading data\n");
	TPM_PrintFour("TPM_Process_DirRead:", tpm_state->tpm_permanent_data.authDIR);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_DirRead: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* append dirContents */
	    returnCode = TPM_Digest_Store(response, tpm_state->tpm_permanent_data.authDIR);
	    /* checkpoint the end of the outParam's */
	    outParamEnd = response->buffer_current - response->buffer;
	}
	/* digest the above the line output parameters */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_GetOutParamDigest(outParamDigest,	/* output */
					       auditStatus,	/* input audit status */
					       transportEncrypt,
					       tag,			
					       returnCode,
					       ordinal,		/* command ordinal */
					       response->buffer + outParamStart,	/* start */
					       outParamEnd - outParamStart);	/* length */
	}
	/* audit if required */
	if ((returnCode == TPM_SUCCESS) && auditStatus) {
	    returnCode = TPM_ProcessAudit(tpm_state,
					  transportEncrypt,
					  inParamDigest,
					  outParamDigest,
					  ordinal);
	}
	/* adjust the initial response */
	rcf = TPM_Sbuffer_StoreFinalResponse(response, returnCode, tpm_state);
    }
    /*
      cleanup
    */
    return rcf;
}
