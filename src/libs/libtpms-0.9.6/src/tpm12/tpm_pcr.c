/********************************************************************************/
/*										*/
/*				PCR Handler					*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_pcr.c $		*/
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

#include "tpm_auth.h"
#include "tpm_constants.h"
#include "tpm_cryptoh.h"
#include "tpm_digest.h"
#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_global.h"
#include "tpm_io.h"
#include "tpm_key.h"
#include "tpm_memory.h"
#include "tpm_nonce.h"
#include "tpm_process.h"
#include "tpm_sizedbuffer.h"
#include "tpm_types.h"
#include "tpm_ver.h"

#include "tpm_pcr.h"


/*
  Locality Utilities
*/

/* TPM_Locality_Set() sets a bit in the TPM_LOCALITY_SELECTION (BYTE) bitmap based on the
   TPM_STANY_FLAGS -> TPM_MODIFIER_INDICATOR (uint32_t) -> localityModifier
*/

TPM_RESULT TPM_Locality_Set(TPM_LOCALITY_SELECTION *tpm_locality_selection,	/* BYTE bitmap */
			    TPM_MODIFIER_INDICATOR tpm_modifier_indicator)	/* uint32_t from
										   TPM_STANY_FLAGS
										*/
{
    TPM_RESULT	rc = 0;
    printf(" TPM_Locality_Set:\n");
    switch (tpm_modifier_indicator) {
      case 0:
	*tpm_locality_selection = TPM_LOC_ZERO;
	break;
      case 1:
	*tpm_locality_selection = TPM_LOC_ONE;
	break;
      case 2:
	*tpm_locality_selection = TPM_LOC_TWO;
	break;
      case 3:
	*tpm_locality_selection = TPM_LOC_THREE;
	break;
      case 4:
	*tpm_locality_selection = TPM_LOC_FOUR;
	break;
      default:
	/* This should never occur.  The code that sets TPM_STANY_FLAGS should screen out bad values
	   */
	printf("TPM_Locality_Set: Error (fatal), tpm_modifier_indicator %u out of range\n",
	       tpm_modifier_indicator);
	rc = TPM_FAIL;
    }
    return rc;
}

/* TPM_Locality_Check() checks that a bit in the TPM_LOCALITY_SELECTION (BYTE) bitmap is set for bit
   TPM_STANY_FLAGS -> TPM_MODIFIER_INDICATOR (uint32_t) -> localityModifier

   'tpm_locality_selection' is typically localityAtRelease, pcrResetLocal, pcrExtendLocal
   'localityModifier' is TPM_STANY_FLAGS.localityModifier
*/

TPM_RESULT TPM_Locality_Check(TPM_LOCALITY_SELECTION tpm_locality_selection,	/* BYTE bitmap */
			      TPM_MODIFIER_INDICATOR localityModifier)	/* uint32_t from
									   TPM_STANY_FLAGS */
{

    TPM_RESULT	rc = 0;
    printf(" TPM_Locality_Check:\n");
    switch (localityModifier) {
      case 0:
	if ((tpm_locality_selection & TPM_LOC_ZERO) == 0) {
	    rc = TPM_BAD_LOCALITY;
	}
	break;
      case 1:
	if ((tpm_locality_selection & TPM_LOC_ONE) == 0) {
	    rc = TPM_BAD_LOCALITY;
	}
	break;
      case 2:
	if ((tpm_locality_selection & TPM_LOC_TWO) == 0) {
	    rc = TPM_BAD_LOCALITY;
	}
	break;
      case 3:
	if ((tpm_locality_selection & TPM_LOC_THREE) == 0) {
	    rc = TPM_BAD_LOCALITY;
	}
	break;
      case 4:
	if ((tpm_locality_selection & TPM_LOC_FOUR) == 0) {
	    rc = TPM_BAD_LOCALITY;
	}
	break;
      default:
	/* This should never occur.  The code that sets TPM_STANY_FLAGS should screen out bad values
	 */
	printf("TPM_Locality_Check: Error (fatal), localityModifier %u out of range\n",
	       localityModifier);
	rc = TPM_FAIL;
    }
    if (rc != 0) {
	printf("TPM_Locality_Check: Error, "
	       "localityModifier %u tpm_locality_selection %02x\n",
	       localityModifier, tpm_locality_selection);
    }
    return rc;
}

TPM_RESULT TPM_LocalitySelection_CheckLegal(TPM_LOCALITY_SELECTION tpm_locality_selection) /* BYTE
											   bitmap */
{
    TPM_RESULT	rc = 0;

    printf(" TPM_LocalitySelection_CheckLegal: TPM_LOCALITY_SELECTION %02x\n",
	   tpm_locality_selection);
    /* if any extra bits are set, illegal value */
    if ((tpm_locality_selection & ~TPM_LOC_ALL) ||
	/* This value MUST not be zero (0). (can never be satisfied) */
	(tpm_locality_selection == 0)) {
	printf("TPM_LocalitySelection_CheckLegal: Error, bad locality selection %02x\n",
	       tpm_locality_selection);
	rc = TPM_INVALID_STRUCTURE;
    }
    return rc;
}

TPM_RESULT TPM_LocalityModifier_CheckLegal(TPM_MODIFIER_INDICATOR localityModifier)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_LocalityModifier_CheckLegal: TPM_MODIFIER_INDICATOR %08x\n", localityModifier);
    /* if past the maximum, illegal value */
    if (localityModifier > TPM_LOC_MAX) {
	printf("TPM_LocalityModifier_CheckLegal: Error, bad locality modifier %u\n",
	       localityModifier);
	rc = TPM_BAD_LOCALITY;
    }
    return rc;
}

void TPM_PCRLocality_Compare(TPM_BOOL *match,
			     TPM_LOCALITY_SELECTION tpm_locality_selection1,
			     TPM_LOCALITY_SELECTION tpm_locality_selection2)
{
    if (tpm_locality_selection1 == tpm_locality_selection2) {
	*match = TRUE;
    }
    else {
	*match = FALSE;
    }
    return;
}

/*
  state PCR's
*/

TPM_RESULT TPM_PCR_CheckRange(TPM_PCRINDEX index)
{
    TPM_RESULT	rc = 0;

    if (index >= TPM_NUM_PCR) {
	printf("TPM_PCR_CheckRange: Error, PCR index was %u should be <= %u\n",
	       index, TPM_NUM_PCR);
	rc = TPM_BADINDEX;
    }
    return rc;
}

/* TPM_PCR_Init() initializes the PCR based on the platform specification.  This should be called by
   TPM_Init.

   The caller must check that the PCR index is in range!
*/

void TPM_PCR_Init(TPM_PCRVALUE *tpm_pcrs,		/* points to the TPM PCR array */
		  const TPM_PCR_ATTRIBUTES *tpm_pcr_attributes,
		  size_t pcrIndex)
{
    printf("  TPM_PCR_Init: pcrIndex %lu\n", (unsigned long)pcrIndex);
    
#if defined TPM_PCCLIENT		/* These values are from the PC Client specification */ 
    tpm_pcr_attributes = tpm_pcr_attributes;
    if ((pcrIndex >= 17) && (pcrIndex <= 22)) {
	TPM_Digest_Set(tpm_pcrs[pcrIndex]);		/* 17-22 init to ff */
    }
    else {
	TPM_Digest_Init(tpm_pcrs[pcrIndex]);		/* 0-16,23 init to 0 */
    }
    /* #elif Add other platform specific values here */
#else					/* This is the default case for the main specification */
    if (!(tpm_pcr_attributes[pcrIndex].pcrReset)) {
	/* FALSE- Default value of the PCR MUST be 0x00..00 */
	TPM_Digest_Init(tpm_pcrs[pcrIndex]);
    }
    else {
	/* TRUE - Default value of the PCR MUST be 0xFF..FF. */
	TPM_Digest_Set(tpm_pcrs[pcrIndex]);
    }
#endif
    return;
}

/* TPM_PCR_Reset() resets the PCR based on the platform specification.	This should be called by the
   TPM_PCR_Reset ordinal.

   The caller must check that the PCR index is in range and that pcrReset is TRUE!
*/

void TPM_PCR_Reset(TPM_PCRVALUE *tpm_pcrs,		/* points to the TPM PCR array */
		   TPM_BOOL TOSPresent,
		   TPM_PCRINDEX pcrIndex)
{
    TPM_PCRVALUE		zeroPCR;
    TPM_PCRVALUE		onesPCR;

    TPM_Digest_Init(zeroPCR);
    TPM_Digest_Set(onesPCR);
#if defined TPM_PCCLIENT		/* These values are from the PC Client specification */ 
    if (TOSPresent ||			/* TOSPresent -> 00 */
	(pcrIndex == 16) ||		/* PCR 16 -> 00 */
	(pcrIndex == 23)) {		/* PCR 23 -> 00 */
	TPM_PCR_Store(tpm_pcrs, pcrIndex, zeroPCR);
    }
    else {
	TPM_PCR_Store(tpm_pcrs, pcrIndex, onesPCR);	/* PCR 17-22 -> ff */
    }
    /* #elif Add other platform specific values here */
#else					/* This is the default case for the main specification */
    if (TOSPresent) {
	TPM_PCR_Store(tpm_pcrs, pcrIndex, zeroPCR);
    }
    else {
	TPM_PCR_Store(tpm_pcrs, pcrIndex, onesPCR);
    }
#endif
    return;
}

/* TPM_PCR_Load() copies the PCR at 'index' to 'dest_pcr'

*/

TPM_RESULT TPM_PCR_Load(TPM_PCRVALUE dest_pcr,
			TPM_PCRVALUE *tpm_pcrs,
			TPM_PCRINDEX index)
{
    TPM_RESULT		rc = 0;
    
    /* range check pcrNum */
    if (rc == 0) {
	rc = TPM_PCR_CheckRange(index);
    }
    if (rc == 0) {
	TPM_Digest_Copy(dest_pcr, tpm_pcrs[index]);
    }
    return rc;
}

/* TPM_PCR_Store() copies 'src_pcr' to the PCR at 'index'

*/

TPM_RESULT TPM_PCR_Store(TPM_PCRVALUE *tpm_pcrs,
			 TPM_PCRINDEX index,
			 TPM_PCRVALUE src_pcr)
{
    TPM_RESULT		rc = 0;

    /* range check pcrNum */
    if (rc == 0) {
	rc = TPM_PCR_CheckRange(index);
    }
    if (rc == 0) {
	TPM_Digest_Copy(tpm_pcrs[index], src_pcr);
    }
    return rc;
}

/*
  TPM_SELECT_SIZE
*/

/* TPM_SelectSize_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_SelectSize_Init(TPM_SELECT_SIZE *tpm_select_size)
{
    printf(" TPM_SelectSize_Init:\n");
    tpm_select_size->major = TPM_MAJOR;
    tpm_select_size->minor = TPM_MINOR;
    tpm_select_size->reqSize = TPM_NUM_PCR/CHAR_BIT;
    return;
}

/* TPM_SelectSize_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_SelectSize_Init()
*/

TPM_RESULT TPM_SelectSize_Load(TPM_SELECT_SIZE *tpm_select_size,
			       unsigned char **stream,
			       uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;
    printf(" TPM_SelectSize_Load:\n");
    /* load major */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_select_size->major), stream, stream_size);
    }
    /* This SHALL indicate the major version of the TPM. This MUST be 0x01 */
    if (rc == 0) {
	if (tpm_select_size->major != 0x01) {
	    printf("TPM_SelectSize_Load: Error, major %02x should be 01\n", tpm_select_size->major);
	    rc = TPM_BAD_PARAMETER;
	}
    }
    /* load minor */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_select_size->minor), stream, stream_size);
    }
    /* This SHALL indicate the minor version of the TPM. This MAY be 0x01 or 0x02 */
    if (rc == 0) {
	if ((tpm_select_size->minor != 0x01) &&
	    (tpm_select_size->minor != 0x02)) {
	    printf("TPM_SelectSize_Load: Error, minor %02x should be 01\n", tpm_select_size->minor);
	    rc = TPM_BAD_PARAMETER;
	}
    }
    /* load reqSize */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_select_size->reqSize), stream, stream_size);
    }
    return rc;
}

/*
  TPM_PCR_ATTRIBUTES
*/

/* 8.9	Debug PCR register

   There is a need to define a PCR that allows for debugging. The attributes of the debug register
   are such that it is easy to reset but the register provides no measurement value that can not be
   spoofed. Production applications should not use the debug PCR for any SEAL or other
   operations. The anticipation is that the debug PCR is set and used by application developers
   during the application development cycle. Developers are responsible for ensuring that a conflict
   between two programs does not invalidate the settings they are interested in.
   
   The specific register that is the debug PCR MUST be set by the platform specific specification. 
   
   The attributes for the debug PCR SHALL be the following:
   pcrReset = TRUE;
   pcrResetLocal = 0x1f;
   pcrExtendLocal = 0x1f;
   pcrUseLocal = 0x1f
   
   These settings are to create a PCR register that developers can use to reset at any time during
   their development cycle.
   
   The debug PCR does NOT need to be saved during TPM_SaveState.

   8.7	PCR Attributes
   
   1. The PCR attributes MUST be set during manufacturing.
   
   2. For a specific PCR register, the PCR attributes MUST match the requirements of the TCG
   platform specific specification that describes the platform.
*/

void TPM_PCRAttributes_Init(TPM_PCR_ATTRIBUTES *tpm_pcr_attributes)
{
    size_t	i;
    
    printf(" TPM_PCRAttributes_Init:\n");
    for (i = 0 ; i < TPM_NUM_PCR ; i++) {
#if defined TPM_PCCLIENT		/* These values are from the PC Client specification */
#if TPM_NUM_PCR != 24
#error "Number of PCRs must be 24 for PC Client"
#endif
	if (i <=15) {
	    tpm_pcr_attributes[i].pcrReset = FALSE;		/* 0-15 are not resettable */
	    tpm_pcr_attributes[i].pcrResetLocal = 0;
	    tpm_pcr_attributes[i].pcrExtendLocal = TPM_LOC_ALL;
	}
	else {
	    tpm_pcr_attributes[i].pcrReset = TRUE;
	    switch (i) {
	      case 16:
	      case 23:
		tpm_pcr_attributes[i].pcrResetLocal = TPM_LOC_ALL;
		tpm_pcr_attributes[i].pcrExtendLocal = TPM_LOC_ALL;
		break;
	      case 17:
	      case 18:
		tpm_pcr_attributes[i].pcrResetLocal = TPM_LOC_FOUR;
		tpm_pcr_attributes[i].pcrExtendLocal = TPM_LOC_FOUR | TPM_LOC_THREE | TPM_LOC_TWO;
		break;
	      case 19:
		tpm_pcr_attributes[i].pcrResetLocal = TPM_LOC_FOUR;
		tpm_pcr_attributes[i].pcrExtendLocal = TPM_LOC_THREE | TPM_LOC_TWO;
		break;
	      case 20:
		tpm_pcr_attributes[i].pcrResetLocal = TPM_LOC_FOUR | TPM_LOC_TWO;
		tpm_pcr_attributes[i].pcrExtendLocal = TPM_LOC_THREE | TPM_LOC_TWO | TPM_LOC_ONE;
		break;
	      case 21:
	      case 22:
		tpm_pcr_attributes[i].pcrResetLocal = TPM_LOC_TWO;
		tpm_pcr_attributes[i].pcrExtendLocal = TPM_LOC_TWO;
		break;
	    }
	}
	/* #elif Add other platform specific values here */
#else					/* This is the default case for the main specification */
	if (i != TPM_DEBUG_PCR) {
	    tpm_pcr_attributes[i].pcrReset = FALSE;
	    tpm_pcr_attributes[i].pcrResetLocal = 0;	/* not relevant when pcrReset is FALSE */
	    tpm_pcr_attributes[i].pcrExtendLocal = TPM_LOC_ALL;
	}
	else {	/* debug PCR */
	    tpm_pcr_attributes[i].pcrReset = TRUE;
	    tpm_pcr_attributes[i].pcrResetLocal = TPM_LOC_ALL;
	    tpm_pcr_attributes[i].pcrExtendLocal = TPM_LOC_ALL;
	}
#endif
    }
    return;
}

/* TPM_PCRInfo_Trace() traces some PCR Info components */

void TPM_PCRInfo_Trace(const char *message,
		       TPM_PCR_SELECTION pcrSelection,
		       TPM_COMPOSITE_HASH digestAtRelease)
{
    printf("%s\n", message);
    printf("\tsizeOfSelect %hu\n", pcrSelection.sizeOfSelect);
    printf("\tpcrSelect %02x %02x %02x\n", 
	   pcrSelection.pcrSelect[0],
	   pcrSelection.pcrSelect[1],
	   pcrSelection.pcrSelect[2]);
    TPM_PrintFour("\tdigestAtRelease",
		  digestAtRelease);
    return;
}

/*
  PCRs - Functions that act on the entire set of PCRs
*/

/* TPM_PCRs_Init() initializes the entire PCR array.

   Typically called from TPM_Init.
*/

void TPM_PCRs_Init(TPM_PCRVALUE *tpm_pcrs,		/* points to the TPM PCR array */
		   const TPM_PCR_ATTRIBUTES *tpm_pcr_attributes)
{
    size_t	i;

    printf(" TPM_PCRs_Init:\n");
    for (i = 0 ; i < TPM_NUM_PCR ; i++) {
	TPM_PCR_Init(tpm_pcrs, tpm_pcr_attributes, i);	/* initialize a single PCR */
    }
    return;
}

TPM_RESULT TPM_PCRs_Load(TPM_PCRVALUE *tpm_pcrs,		/* points to the TPM PCR array */
			 const TPM_PCR_ATTRIBUTES *tpm_pcr_attributes,
			 unsigned char **stream,
			 uint32_t *stream_size)
{
    TPM_RESULT	rc = 0;
    size_t	i;

    printf(" TPM_PCRs_Load:\n");
    for (i = 0 ; (rc == 0) && (i < TPM_NUM_PCR) ; i++) {
	/* FALSE: Saved by TPM_SaveState
	   TRUE: MUST not be part of any state stored by TPM_SaveState */
	if (!(tpm_pcr_attributes[i].pcrReset)) {
	    rc = TPM_Digest_Load(tpm_pcrs[i], stream, stream_size); 
	}
    }
    return rc;
}

TPM_RESULT TPM_PCRs_Store(TPM_STORE_BUFFER *sbuffer,
			  TPM_PCRVALUE *tpm_pcrs,		/* points to the TPM PCR array */
			  const TPM_PCR_ATTRIBUTES *tpm_pcr_attributes)
{
    TPM_RESULT	rc = 0;
    size_t	i;

    printf(" TPM_PCRs_Store:\n");
    for (i = 0 ; (rc == 0) && (i < TPM_NUM_PCR) ; i++) {
	/* FALSE: Saved by TPM_SaveState
	   TRUE: MUST not be part of any state stored by TPM_SaveState */
	if (!(tpm_pcr_attributes[i].pcrReset)) {
	    rc = TPM_Digest_Store(sbuffer, tpm_pcrs[i]);
	}
    }
    return rc;
}

/*
  TPM_PCR_COMPOSITE
*/

/* TPM_PCRComposite_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_PCRComposite_Init(TPM_PCR_COMPOSITE *tpm_pcr_composite)
{
    TPM_PCRSelection_Init(&(tpm_pcr_composite->select));
    TPM_SizedBuffer_Init(&(tpm_pcr_composite->pcrValue));
    return;
}

/* TPM_PCRComposite_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
   
   After use, call TPM_PCRComposite_Delete() to free memory
*/

TPM_RESULT TPM_PCRComposite_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_PCR_COMPOSITE *tpm_pcr_composite)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRComposite_Store:\n");
    
    /* store TPM_PCR_SELECTION select */
    if (rc == 0) {
	rc = TPM_PCRSelection_Store(sbuffer, &(tpm_pcr_composite->select)); 
    }
    /* store pcrValue */
    if (rc == 0) {
	rc = TPM_SizedBuffer_Store(sbuffer, &(tpm_pcr_composite->pcrValue));
    }
    return rc;
}

/* TPM_PCRComposite_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_PCRComposite_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_PCRComposite_Delete(TPM_PCR_COMPOSITE *tpm_pcr_composite)
{
    printf(" TPM_PCRComposite_Delete:\n");
    if (tpm_pcr_composite != NULL) {
	TPM_PCRSelection_Delete(&(tpm_pcr_composite->select));
	TPM_SizedBuffer_Delete(&(tpm_pcr_composite->pcrValue));
	TPM_PCRComposite_Init(tpm_pcr_composite);
    }
    return;    
}

/* TPM_PCRComposite_Set()

   sets members to input parameter values
   allocates memory as required to fill in pointers
   returns 0 or error codes
   
   After use, call TPM_PCRComposite_Delete() to free memory
*/

TPM_RESULT TPM_PCRComposite_Set(TPM_PCR_COMPOSITE *tpm_pcr_composite,
				TPM_PCR_SELECTION *tpm_pcr_selection, /* input selection map */
				TPM_PCRVALUE *tpm_pcrs)		/* points to the TPM PCR array */
{
    TPM_RESULT		rc = 0;
    size_t		i;		/* byte in map */
    size_t		j;		/* bit map in byte */
    size_t		pcrs = 0;	/* number of selected PCR's */
    TPM_PCRINDEX	pcr_num;	/* selected PCR being copied */
    size_t		comp_num;	/* index into composite */

    printf(" TPM_PCRComposite_Set:\n");
    /* test sizeOfSelect value */
    if (rc == 0) {
	rc = TPM_PCRSelection_CheckRange(tpm_pcr_selection);
    }
    /* construct the TPM_PCR_COMPOSITE structure */
    if (rc == 0) {
	/* copy the TPM_PCR_SELECTION member */
	rc = TPM_PCRSelection_Copy(&(tpm_pcr_composite->select), tpm_pcr_selection);
    }
    /* iterate through all bytes in tpm_pcr_selection to count the number of selected PCR's */
    if (rc == 0) {
	for (i = 0, pcrs = 0 ; i < tpm_pcr_selection->sizeOfSelect ; i++) {
	    /* iterate through all bits in each byte */
	    for (j = 0x0001 ; j != (0x0001 << CHAR_BIT) ; j <<= 1) {
		if (tpm_pcr_selection->pcrSelect[i] & j) {	/* if the bit is set in the map */
		    pcrs++;
		}
	    }
	}
    }
    /* allocate memory for the pcrValue member (a TPM_PCRVALUE for each selected PCR) */
    if ((rc == 0) && (pcrs > 0)) {
	printf("  TPM_PCRComposite_Set: Digesting %lu pcrs\n", (unsigned long)pcrs);
	rc = TPM_SizedBuffer_Allocate(&(tpm_pcr_composite->pcrValue), pcrs * sizeof(TPM_PCRVALUE));
    }
    /* Next iterate through all bytes in tpm_pcr_selection and copy to TPM_PCR_COMPOSITE */
    if ((rc == 0) && (pcrs > 0)) {
	for (i = 0, pcr_num = 0, comp_num = 0 ; i < tpm_pcr_selection->sizeOfSelect ; i++) {
	    /* iterate through all bits in each byte */
	    for (j = 0x0001 ; j != (0x0001 << CHAR_BIT) ; j <<= 1, pcr_num++) {
		if (tpm_pcr_selection->pcrSelect[i] & j) {	/* if the bit is set in the map */
		    printf("  TPM_PCRComposite_Set: Adding PCR %u\n", pcr_num);
		    /* append the the PCR value to TPM_PCR_COMPOSITE.pcrValue */
		    /* NOTE: Ignore return code since range checked by
		       TPM_PCRSelection_CheckRange() */
		    TPM_PCR_Load(&(tpm_pcr_composite->pcrValue.buffer[comp_num]),
				 tpm_pcrs, pcr_num);
		    comp_num += sizeof(TPM_PCRVALUE);
		}
	    }
	}
    }
    return rc;
}

/*
  TPM_PCR_INFO_SHORT
*/

void TPM_PCRInfoShort_Init(TPM_PCR_INFO_SHORT *tpm_pcr_info_short)
{
    TPM_PCRSelection_Init(&(tpm_pcr_info_short->pcrSelection));
    tpm_pcr_info_short->localityAtRelease = TPM_LOC_ALL;
    TPM_Digest_Init(tpm_pcr_info_short->digestAtRelease);
    return;
}

/* TPM_PCRInfoShort_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes

   optimize invokes a special version used to an load TPM_NV_DATA_PUBLIC that may not include
   digestAtRelease
   
   After use, call TPM_PCRInfoShort_Delete() to free memory
*/

TPM_RESULT TPM_PCRInfoShort_Load(TPM_PCR_INFO_SHORT *tpm_pcr_info_short,
				 unsigned char **stream,
				 uint32_t *stream_size,
				 TPM_BOOL optimize)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL 	pcrUsage = TRUE;
    
    printf(" TPM_PCRInfoShort_Load:\n");
    /* load pcrSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Load(&(tpm_pcr_info_short->pcrSelection), stream, stream_size);
    }
    /* load the localityAtRelease */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_pcr_info_short->localityAtRelease), stream, stream_size);
    }
    /* check locality value */
    if (rc == 0) {
	rc = TPM_LocalitySelection_CheckLegal(tpm_pcr_info_short->localityAtRelease);
    }
    /* if the store was optimized, check whether the pcrSelection specifies PCRs */
    if ((rc == 0) && optimize) {
	rc = TPM_PCRSelection_GetPCRUsage(&pcrUsage,
					  &(tpm_pcr_info_short->pcrSelection),
					  0);	/* start_index */
    }
    /* load the digestAtRelease */
    if (rc == 0) {
	if (pcrUsage) {
	    rc = TPM_Digest_Load(tpm_pcr_info_short->digestAtRelease, stream, stream_size);
	}
	/* A pcrSelect of 0 indicates that the digestAsRelease is not checked. In this case, the TPM is
	   not required to consume NVRAM space to store the digest, although it may do so. When
	   TPM_GetCapability (TPM_CAP_NV_INDEX) returns the structure, a TPM that does not store the
	   digest can return zero. A TPM that does store the digest may return either the digest or
	   zero. Software should not be written to depend on either implementation.
	*/
	else {
	    TPM_Digest_Init(tpm_pcr_info_short->digestAtRelease);
	}
    }
    return rc;
}

/* TPM_PCRInfoShort_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
   
   optimize invokes a special version used to an store TPM_NV_DATA_PUBLIC that may not include
   digestAtRelease
   
   After use, call TPM_Sbuffer_Delete() to free memory
*/

TPM_RESULT TPM_PCRInfoShort_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_PCR_INFO_SHORT *tpm_pcr_info_short,
				  TPM_BOOL optimize)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL 	pcrUsage = TRUE;

    printf(" TPM_PCRInfoShort_Store:\n");
    /* store pcrSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Store(sbuffer, &(tpm_pcr_info_short->pcrSelection));
    }
    /* store the localityAtRelease */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_pcr_info_short->localityAtRelease),
				sizeof(TPM_LOCALITY_SELECTION));
    }
    /* check whether the pcrSelection specifies PCRs */
    if ((rc == 0) && optimize) {
	rc = TPM_PCRSelection_GetPCRUsage(&pcrUsage,
					  &(tpm_pcr_info_short->pcrSelection),
					  0);	/* start_index */
    }
    /* store the digestAtRelease */
    /* A pcrSelect of 0 indicates that the digestAsRelease is not checked. In this case, the TPM is
       not required to consume NVRAM space to store the digest, although it may do so. When
       TPM_GetCapability (TPM_CAP_NV_INDEX) returns the structure, a TPM that does not store the
       digest can return zero. A TPM that does store the digest may return either the digest or
       zero. Software should not be written to depend on either implementation.
    */    if ((rc == 0) && pcrUsage) {
	rc = TPM_Digest_Store(sbuffer, tpm_pcr_info_short->digestAtRelease);
    }
    return rc;
}

/* TPM_PCRInfoShort_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the PCRInfoShort
   sets pointers to NULL
   calls TPM_PCRInfoShort_Init to set members back to default values
   The PCRInfoShort itself is not freed
   returns 0 or error codes
*/   

void TPM_PCRInfoShort_Delete(TPM_PCR_INFO_SHORT *tpm_pcr_info_short)
{
    printf(" TPM_PCRInfoShort_Delete:\n");
    if (tpm_pcr_info_short != NULL) {
	TPM_PCRSelection_Delete(&(tpm_pcr_info_short->pcrSelection));
	TPM_PCRInfoShort_Init(tpm_pcr_info_short);
    }
    return;
}

/* TPM_PCRInfoShort_Create() allocates memory for a TPM_PCR_INFO_SHORT

*/

TPM_RESULT TPM_PCRInfoShort_Create(TPM_PCR_INFO_SHORT **tpm_pcr_info_short)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRInfoShort_Create:\n");
    /* This function should never be called when the TPM_PCR_INFO_SHORT structure has already been
       loaded.	This indicates an internal error. */
    if (rc == 0) {
	if (*tpm_pcr_info_short != NULL) {
	    printf("TPM_PCRInfoShort_Create: Error (fatal), TPM_PCR_INFO_SHORT already loaded\n");
	    rc = TPM_FAIL;
	}
    }
    if (rc == 0) {
	rc = TPM_Malloc((unsigned char **)tpm_pcr_info_short, sizeof(TPM_PCR_INFO_SHORT));
    }
    return rc;
}

/* TPM_PCRInfoShort_SetFromBuffer() sets a TPM_PCR_INFO_SHORT from a stream specified by a
   TPM_SIZED_BUFFER.  The TPM_SIZED_BUFFER is not modified.
*/

TPM_RESULT TPM_PCRInfoShort_LoadFromBuffer(TPM_PCR_INFO_SHORT *tpm_pcr_info_short,
					   const TPM_SIZED_BUFFER *tpm_sized_buffer)
{
    TPM_RESULT	rc = 0;
    unsigned char *stream;
    uint32_t stream_size;

    printf(" TPM_PCRInfoShort_LoadFromBuffer:\n");
    if (rc == 0) {
	TPM_PCRInfoShort_Init(tpm_pcr_info_short);
	stream = tpm_sized_buffer->buffer;
	stream_size = tpm_sized_buffer->size;
	/* deserialize the TPM_SIZED_BUFFER into a TPM_PCR_INFO_SHORT structure */
	rc = TPM_PCRInfoShort_Load(tpm_pcr_info_short, &stream, &stream_size, FALSE);
    }
    return rc;
}

/* TPM_PCRInfoShort_CreateFromBuffer() allocates the TPM_PCR_INFO_SHORT structure, typically a cache
   within another structure.  It then deserializes the TPM_SIZED_BUFFER into the structure.
   
   The TPM_SIZED_BUFFER is not modified.
*/

TPM_RESULT TPM_PCRInfoShort_CreateFromBuffer(TPM_PCR_INFO_SHORT **tpm_pcr_info_short,
					     const TPM_SIZED_BUFFER *tpm_sized_buffer)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	done = FALSE;

    printf(" TPM_PCRInfoShort_CreateFromBuffer:\n");
    /* if there is no TPM_PCR_INFO_SHORT - done */
    if (rc == 0) {
	if (tpm_sized_buffer->size == 0) {
	    done = TRUE;
	}
    }
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfoShort_Create(tpm_pcr_info_short);
    }
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfoShort_LoadFromBuffer(*tpm_pcr_info_short, tpm_sized_buffer);
    }
    return rc;
}

/* TPM_PCRInfoShort_Copy() copies the source pcrSelection, digestAtRelease, and digestAtCreation.

*/

TPM_RESULT TPM_PCRInfoShort_Copy(TPM_PCR_INFO_SHORT *dest_tpm_pcr_info_short,
				 TPM_PCR_INFO_SHORT *src_tpm_pcr_info_short)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRInfoShort_Copy:\n");
    /* copy TPM_PCR_SELECTION pcrSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Copy(&(dest_tpm_pcr_info_short->pcrSelection),
				   &(src_tpm_pcr_info_short->pcrSelection));
    }
    if (rc == 0) {
	/* copy TPM_LOCALITY_SELECTION localityAtRelease */
	dest_tpm_pcr_info_short->localityAtRelease = src_tpm_pcr_info_short->localityAtRelease;
	/* copy TPM_COMPOSITE_HASH digestAtRelease */
	TPM_Digest_Copy(dest_tpm_pcr_info_short->digestAtRelease,
			src_tpm_pcr_info_short->digestAtRelease);
    }
    return rc;
}

/* TPM_PCRInfoShort_CopyInfo() copies the source TPM_PCR_INFO to the destination TPM_PCR_INFO_SHORT.

   It copies pcrSelection and digestAtRelease.
   
   It handles localityAtRelease as per the specification.
*/

TPM_RESULT TPM_PCRInfoShort_CopyInfo(TPM_PCR_INFO_SHORT *dest_tpm_pcr_info_short,
				     TPM_PCR_INFO *src_tpm_pcr_info)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRInfoShort_CopyInfo:\n");
    /* 4. To set IS from IN */
    /* a. Set IS -> pcrSelection to IN -> pcrSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Copy(&(dest_tpm_pcr_info_short->pcrSelection),
				   &(src_tpm_pcr_info->pcrSelection));
    }
    /* b. Set IS -> digestAtRelease to IN -> digestAtRelease */
    if (rc == 0) {
	TPM_Digest_Copy(dest_tpm_pcr_info_short->digestAtRelease,
			src_tpm_pcr_info->digestAtRelease);
	/* c. Set IS -> localityAtRelease to 0x1F to indicate all localities are valid */
	dest_tpm_pcr_info_short->localityAtRelease = TPM_LOC_ALL;
	/* d. Ignore IN -> digestAtCreation */
    }
    return rc;
}

/* TPM_PCRInfoShort_CopyInfoLong() copies the source TPM_PCR_INFO_LONG to the destination
   TPM_PCR_INFO_SHORT.

   It copies creationPCRSelection, localityAtRelease, digestAtRelease.
*/

TPM_RESULT TPM_PCRInfoShort_CopyInfoLong(TPM_PCR_INFO_SHORT *dest_tpm_pcr_info_short,
					 TPM_PCR_INFO_LONG *src_tpm_pcr_info_long)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRInfoShort_CopyInfoLong:\n");
    /* 5. To set IS from IL */
    /* a. Set IS -> pcrSelection to IL -> releasePCRSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Copy(&(dest_tpm_pcr_info_short->pcrSelection),
				   &(src_tpm_pcr_info_long->releasePCRSelection));
    }
    /* b. Set IS -> localityAtRelease to IL -> localityAtRelease */
    if (rc == 0) {
	dest_tpm_pcr_info_short->localityAtRelease = src_tpm_pcr_info_long->localityAtRelease;
	/* c. Set IS -> digestAtRelease to IL -> digestAtRelease */
	TPM_Digest_Copy(dest_tpm_pcr_info_short->digestAtRelease,
			src_tpm_pcr_info_long->digestAtRelease);
	/* d. Ignore all other IL values */
    }
    return rc;
}

/* TPM_PCRInfoShort_CreateFromInfo() allocates memory for the TPM_PCR_INFO_SHORT structure.  It
   copies the source to the destination.
   
   If the source is NULL, the destination is NULL.
*/

TPM_RESULT TPM_PCRInfoShort_CreateFromInfo(TPM_PCR_INFO_SHORT **dest_tpm_pcr_info_short,
					   TPM_PCR_INFO *src_tpm_pcr_info)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	done = FALSE;

    printf(" TPM_PCRInfoShort_CreateFromInfo:\n");
    if (rc == 0) {
	/* if there is no source, leave the destination NULL */
	if (src_tpm_pcr_info == NULL) {
	    done = TRUE;
	}
    }
    /* create the structure */
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfoShort_Create(dest_tpm_pcr_info_short);
    }
    /* copy source to destination */
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfoShort_CopyInfo(*dest_tpm_pcr_info_short, src_tpm_pcr_info);
    }
    return rc;
}

/* TPM_PCRInfo_CreateFromInfoLong() allocates memory for the TPM_PCR_INFO structure.  It copies the
   source to the destination.

   If the source is NULL, the destination is NULL.
*/

TPM_RESULT TPM_PCRInfoShort_CreateFromInfoLong(TPM_PCR_INFO_SHORT **dest_tpm_pcr_info_short,
					       TPM_PCR_INFO_LONG *src_tpm_pcr_info_long)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	done = FALSE;

    printf(" TPM_PCRInfoShort_CreateFromInfoLong:\n");
    /* if there is no source, leave the destination NULL */
    if (rc == 0) {
	if (src_tpm_pcr_info_long == NULL) {
	    done = TRUE;
	}
    }
    /* create the structure */
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfoShort_Create(dest_tpm_pcr_info_short);
    }
    /* copy source to destination */
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfoShort_CopyInfoLong(*dest_tpm_pcr_info_short, src_tpm_pcr_info_long);
    }
    return rc;
}

/* TPM_PCRInfoShort_CreateFromKey() allocates memory for the TPM_PCR_INFO_SHORT structure.

   If the input is a TPM_KEY, it copies the TPM_PCR_INFO cache.
   
   If the input is a TPM_KEY12, it copies the TPM_PCR_INFO_LONG cache.
      
   If the source is NULL, the destination is NULL.
*/

TPM_RESULT TPM_PCRInfoShort_CreateFromKey(TPM_PCR_INFO_SHORT **dest_tpm_pcr_info_short,
					  TPM_KEY *tpm_key)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRInfoShort_CreateFromKey:\n");
    if (rc == 0) {
	if (((TPM_KEY12 *)tpm_key)->tag != TPM_TAG_KEY12) {	/* TPM_KEY */
	    rc = TPM_PCRInfoShort_CreateFromInfo(dest_tpm_pcr_info_short,
						 tpm_key->tpm_pcr_info);
	}
	else {							/* TPM_KEY12 */
	    rc = TPM_PCRInfoShort_CreateFromInfoLong(dest_tpm_pcr_info_short,
						     tpm_key->tpm_pcr_info_long);
	}
    }
    return rc;
}

/* TPM_PCRInfoShort_GenerateDigest() generates a Part 2 5.3.1 PCR composite hash

*/

TPM_RESULT TPM_PCRInfoShort_GenerateDigest(TPM_DIGEST tpm_digest,		/* output digest */
					   TPM_PCR_INFO_SHORT *tpm_pcr_info_short,	/* input */
					   TPM_PCRVALUE *tpm_pcrs) /* points to the TPM PCR array */
{
    TPM_RESULT		rc = 0;
    TPM_PCR_SELECTION	*tpm_pcr_selection;

    printf(" TPM_PCRInfoShort_GenerateDigest:\n");
    if (rc == 0) {
	if (tpm_pcr_info_short == NULL) {
	    printf("TPM_PCRInfoShort_GenerateDigest: Error (fatal), TPM_PCR_INFO_SHORT is NULL\n");
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    if (rc == 0) {
	tpm_pcr_selection = &(tpm_pcr_info_short->pcrSelection); /* get the TPM_PCR_SELECTION */
	rc = TPM_PCRSelection_GenerateDigest(tpm_digest,	/* output digest */
					     tpm_pcr_selection, /* input selection map */
					     tpm_pcrs);		/* points to the TPM PCR array */
    }
    return rc;
}

/* TPM_PCRInfoShort_CheckDigest() calculates a digestAtRelease based on the TPM_PCR_SELECTION and
   compares it to digestAtRelease in the structure.
*/

TPM_RESULT TPM_PCRInfoShort_CheckDigest(TPM_PCR_INFO_SHORT *tpm_pcr_info_short,
					TPM_PCRVALUE *tpm_pcrs, /* points to the TPM PCR array */
					TPM_MODIFIER_INDICATOR localityModifier)
{
    TPM_RESULT		rc = 0;
    TPM_COMPOSITE_HASH	tpm_composite_hash;
    TPM_BOOL		pcrUsage;	/* TRUE if PCR's are specified */
	
    printf(" TPM_PCRInfoShort_CheckDigest:\n");
    /* returns FALSE if tpm_pcr_info_short is NULL or selection bitmap is zero */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_GetPCRUsage(&pcrUsage, tpm_pcr_info_short);
    }
    /* Calculate a TPM_COMPOSITE_HASH of the PCR selected by tpm_pcr_info_short ->
       pcrSelection */
    if ((rc == 0) && pcrUsage) {
	rc = TPM_PCRSelection_GenerateDigest(tpm_composite_hash,
					     &(tpm_pcr_info_short->pcrSelection),
					     tpm_pcrs);		/* array of PCR's */
    }
    /* Compare to tpm_pcr_info_short -> digestAtRelease on mismatch return TPM_WRONGPCRVAL */
    if ((rc == 0) && pcrUsage) {
	rc = TPM_Digest_Compare(tpm_composite_hash,
				tpm_pcr_info_short->digestAtRelease);
	if (rc != 0) {
	    printf("TPM_PCRInfoShort_CheckDigest: Error, wrong digestAtRelease value\n");
	    rc = TPM_WRONGPCRVAL;
	}
    }
    /* If localityAtRelease is NOT 0x1f */
    if ((rc == 0) && (tpm_pcr_info_short != NULL)) {
	if (tpm_pcr_info_short->localityAtRelease != TPM_LOC_ALL) {
	    /* Validate that TPM_STANY_FLAGS -> localityModifier is matched by tpm_pcr_info_short ->
	       localityAtRelease on mismatch return TPM_BAD_LOCALITY */
	    rc = TPM_Locality_Check(tpm_pcr_info_short->localityAtRelease,
				    localityModifier);
	}
    }
    return rc;
}

/* TPM_PCRInfoShort_GetPCRUsage() returns 'pcrUsage' TRUE if any bit is set in the pcrSelect bit
   mask.  Returns FALSE if the TPM_PCR_INFO_SHORT is NULL.
*/

TPM_RESULT TPM_PCRInfoShort_GetPCRUsage(TPM_BOOL *pcrUsage,
					TPM_PCR_INFO_SHORT *tpm_pcr_info_short)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	done = FALSE;

    printf(" TPM_PCRInfoShort_GetPCRUsage\n");
    if (rc == 0) {
	/* if a loaded key had no pcrInfoShort, the structure remains NULL */
	if (tpm_pcr_info_short == NULL) {
	    *pcrUsage = FALSE;
	    done = TRUE;
	}
    }
    if ((rc == 0) && !done) {
	rc = TPM_PCRSelection_GetPCRUsage(pcrUsage, &(tpm_pcr_info_short->pcrSelection), 0);
    }
    if (rc == 0) {
	printf("  TPM_PCRInfoShort_GetPCRUsage: Result %d\n", *pcrUsage);
    }
    return rc;
}

/*
  TPM_PCR_INFO
*/

void TPM_PCRInfo_Init(TPM_PCR_INFO *tpm_pcr_info)
{
    TPM_PCRSelection_Init(&(tpm_pcr_info->pcrSelection));
    TPM_Digest_Init(tpm_pcr_info->digestAtRelease);
    TPM_Digest_Init(tpm_pcr_info->digestAtCreation);
    return;
}

/* TPM_PCRInfo_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes

   After use, call TPM_PCRInfo_Delete() to free memory
*/

TPM_RESULT TPM_PCRInfo_Load(TPM_PCR_INFO *tpm_pcr_info,
			    unsigned char **stream,
			    uint32_t *stream_size)
{
    TPM_RESULT	rc = 0;
    
    printf(" TPM_PCRInfo_Load:\n");
    /* load pcrSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Load(&(tpm_pcr_info->pcrSelection), stream, stream_size);
    }
    /* load the digestAtRelease */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_pcr_info->digestAtRelease, stream, stream_size);
    }
    /* load the digestAtCreation */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_pcr_info->digestAtCreation, stream, stream_size);
    }
    return rc;
}

/* TPM_PCRInfo_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes

   After use, call TPM_Sbuffer_Delete() to free memory
*/

TPM_RESULT TPM_PCRInfo_Store(TPM_STORE_BUFFER *sbuffer,
			     const TPM_PCR_INFO *tpm_pcr_info)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRInfo_Store:\n");
    /* store pcrSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Store(sbuffer, &(tpm_pcr_info->pcrSelection));
    }
    /* store digestAtRelease */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_pcr_info->digestAtRelease);
    }
    /* store digestAtCreation */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_pcr_info->digestAtCreation);
    }
    return rc;
}

/* TPM_PCRInfo_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the PCRInfo
   sets pointers to NULL2
   calls TPM_PCRInfo_Init to set members back to default values
   The PCRInfo itself is not freed
   returns 0 or error codes
*/   

void TPM_PCRInfo_Delete(TPM_PCR_INFO *tpm_pcr_info)
{
    printf(" TPM_PCRInfo_Delete:\n");
    if (tpm_pcr_info != NULL) {
	TPM_PCRSelection_Delete(&(tpm_pcr_info->pcrSelection));
	TPM_PCRInfo_Init(tpm_pcr_info);
    }
    return;
}

/* TPM_PCRInfo_Create() allocates memory for a TPM_PCR_INFO

*/

TPM_RESULT TPM_PCRInfo_Create(TPM_PCR_INFO **tpm_pcr_info)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRInfo_Create:\n");
    /* This function should never be called when the TPM_PCR_INFO structure has already been loaded.
       This indicates an internal error. */
    if (rc == 0) {
	if (*tpm_pcr_info != NULL) {
	    printf("TPM_PCRInfo_Create: Error (fatal), TPM_PCR_INFO already loaded\n");
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    if (rc == 0) {
	rc = TPM_Malloc((unsigned char **)tpm_pcr_info, sizeof(TPM_PCR_INFO));
    }
    return rc;
}

/* TPM_PCRInfo_LoadFromBuffer() sets a TPM_PCR_INFO from a stream specified by a TPM_SIZED_BUFFER.
   The TPM_SIZED_BUFFER is not modified.
*/

TPM_RESULT TPM_PCRInfo_LoadFromBuffer(TPM_PCR_INFO *tpm_pcr_info,
				      const TPM_SIZED_BUFFER *tpm_sized_buffer)
{
    TPM_RESULT	rc = 0;
    unsigned char *stream;
    uint32_t stream_size;

    printf(" TPM_PCRInfo_LoadFromBuffer:\n");
    if (rc == 0) {
	TPM_PCRInfo_Init(tpm_pcr_info);
	stream = tpm_sized_buffer->buffer;
	stream_size = tpm_sized_buffer->size;
	/* deserialize the TPM_SIZED_BUFFER into a TPM_PCR_INFO structure */
	rc = TPM_PCRInfo_Load(tpm_pcr_info, &stream, &stream_size);
    }
    return rc;
}

/* TPM_PCRInfo_CreateFromBuffer() allocates the TPM_PCR_INFO structure, typically a cache within
   another structure.  It then deserializes the TPM_SIZED_BUFFER into the structure.

   If the stream is empty, a NULL is returned.
   
   The TPM_SIZED_BUFFER is not modified.
*/

TPM_RESULT TPM_PCRInfo_CreateFromBuffer(TPM_PCR_INFO **tpm_pcr_info,
					const TPM_SIZED_BUFFER *tpm_sized_buffer)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	done = FALSE;

    printf(" TPM_PCRInfo_CreateFromBuffer:\n");
    /* if there is no TPM_PCR_INFO - done */
    if (rc == 0) {
	if (tpm_sized_buffer->size == 0) {
	    done = TRUE;
	}
    }
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfo_Create(tpm_pcr_info);
    }
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfo_LoadFromBuffer(*tpm_pcr_info, tpm_sized_buffer);
    }
    return rc;
}

/* TPM_PCRInfo_Copy() copies the source to the destination.

   It copies pcrSelection, digestAtRelease, and digestAtCreation.
*/

TPM_RESULT TPM_PCRInfo_Copy(TPM_PCR_INFO *dest_tpm_pcr_info,
			    TPM_PCR_INFO *src_tpm_pcr_info)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRInfo_Copy:\n");
    /* copy TPM_PCR_SELECTION pcrSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Copy(&(dest_tpm_pcr_info->pcrSelection),
				   &(src_tpm_pcr_info->pcrSelection));
    }
    /* copy TPM_COMPOSITE_HASH's */
    if (rc == 0) {
	TPM_Digest_Copy(dest_tpm_pcr_info->digestAtRelease,
			src_tpm_pcr_info->digestAtRelease);
	TPM_Digest_Copy(dest_tpm_pcr_info->digestAtCreation,
			src_tpm_pcr_info->digestAtCreation);
    }
    return rc;
}

/* TPM_PCRInfo_CopyInfoLong() copies the source TPM_PCR_INFO_LONG to the destination TPM_PCR_INFO.

   It copies pcrSelection and digestAtRelease.

   It handles digestAtCreation as per the specification.
*/

TPM_RESULT TPM_PCRInfo_CopyInfoLong(TPM_PCR_INFO *dest_tpm_pcr_info,
				    TPM_PCR_INFO_LONG *src_tpm_pcr_info_long)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL selectMatch;
    TPM_BOOL localityMatch;

    printf(" TPM_PCRInfo_Copy:\n");
    /* 9. To set IN from IL */
    /* a. Set IN -> pcrSelection to IL -> releasePCRSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Copy(&(dest_tpm_pcr_info->pcrSelection),
				   &(src_tpm_pcr_info_long->releasePCRSelection));
    }
    /* copy TPM_COMPOSITE_HASH's */
    if (rc == 0) {
	/* b. Set IN -> digestAtRelease to IL -> digestAtRelease */
	TPM_Digest_Copy(dest_tpm_pcr_info->digestAtRelease,
			src_tpm_pcr_info_long->digestAtRelease);
	TPM_PCRSelection_Compare(&selectMatch,
				 &(src_tpm_pcr_info_long->creationPCRSelection),
				 &(src_tpm_pcr_info_long->releasePCRSelection));
	TPM_PCRLocality_Compare(&localityMatch,
				src_tpm_pcr_info_long->localityAtCreation,
				src_tpm_pcr_info_long->localityAtRelease);
	/* c. If IL -> creationPCRSelection and IL -> localityAtCreation both match IL ->
	   releasePCRSelection and IL -> localityAtRelease */
	if (selectMatch && localityMatch) {
	    /* i. Set IN -> digestAtCreation to IL -> digestAtCreation */
	    TPM_Digest_Copy(dest_tpm_pcr_info->digestAtCreation,
			    src_tpm_pcr_info_long->digestAtCreation);
	}
	/* d. Else */
	else {
	    /* i. Set IN -> digestAtCreation to NULL */
	    TPM_Digest_Init(dest_tpm_pcr_info->digestAtCreation);
	}
    }
    return rc;
}

/* TPM_PCRInfo_CreateFromInfo() allocates memory for the TPM_PCR_INFO structure.  It copies the
   source to the destination.

   If the source is NULL, the destination is NULL.
*/

TPM_RESULT TPM_PCRInfo_CreateFromInfo(TPM_PCR_INFO **dest_tpm_pcr_info,
				      TPM_PCR_INFO *src_tpm_pcr_info)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	done = FALSE;

    printf(" TPM_PCRInfo_CreateFromInfo:\n");
    /* if there is no source, leave the destination NULL */
    if (rc == 0) {
	if (src_tpm_pcr_info == NULL) {
	    done = TRUE;
	}
    }
    /* create the structure */
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfo_Create(dest_tpm_pcr_info);
    }
    /* copy source to destination */
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfo_Copy(*dest_tpm_pcr_info, src_tpm_pcr_info);
    }
    return rc;
}

/* TPM_PCRInfo_CreateFromInfoLong() allocates memory for the TPM_PCR_INFO structure.  It copies the
   source to the destination.

   If the source is NULL, the destination is NULL.
*/

TPM_RESULT TPM_PCRInfo_CreateFromInfoLong(TPM_PCR_INFO **dest_tpm_pcr_info,
					  TPM_PCR_INFO_LONG *src_tpm_pcr_info_long)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	done = FALSE;

    printf(" TPM_PCRInfo_CreateFromInfoLong:\n");
    /* if there is no source, leave the destination NULL */
    if (rc == 0) {
	if (src_tpm_pcr_info_long == NULL) {
	    done = TRUE;
	}
    }
    /* create the structure */
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfo_Create(dest_tpm_pcr_info);
    }
    /* copy source to destination */
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfo_CopyInfoLong(*dest_tpm_pcr_info, src_tpm_pcr_info_long);
    }
    return rc;
}

/* TPM_PCRInfo_CreateFromKey() allocates memory for the TPM_PCR_INFO structure.

   If the input is a TPM_KEY, it copies the TPM_PCR_INFO cache.

   If the input is a TPM_KEY12, it copies the TPM_PCR_INFO_LONG cache.
   
   If the source is NULL, the destination is NULL.
*/


TPM_RESULT TPM_PCRInfo_CreateFromKey(TPM_PCR_INFO **dest_tpm_pcr_info,
				     TPM_KEY *tpm_key)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRInfo_CreateFromKey:\n");
    if (rc == 0) {
	if (((TPM_KEY12 *)tpm_key)->tag != TPM_TAG_KEY12) {	/* TPM_KEY */
	    rc = TPM_PCRInfo_CreateFromInfo(dest_tpm_pcr_info, tpm_key->tpm_pcr_info);
	}
	else {							/* TPM_KEY12 */
	    rc = TPM_PCRInfo_CreateFromInfoLong(dest_tpm_pcr_info, tpm_key->tpm_pcr_info_long);
	}
    }
    return rc;
}

/* TPM_PCRInfo_GenerateDigest() generates a Part 2 5.3.1 PCR composite hash

*/

TPM_RESULT TPM_PCRInfo_GenerateDigest(TPM_DIGEST tpm_digest,		/* output digest */
				      TPM_PCR_INFO *tpm_pcr_info,	/* input */
				      TPM_PCRVALUE *tpm_pcrs)	/* points to the TPM PCR array */
{
    TPM_RESULT		rc = 0;
    TPM_PCR_SELECTION	*tpm_pcr_selection;

    printf(" TPM_PCRInfo_GenerateDigest:\n");
    if (rc == 0) {
	if (tpm_pcr_info == NULL) {
	    printf("TPM_PCRInfo_GenerateDigest: Error (fatal), TPM_PCR_INFO is NULL\n");
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    if (rc == 0) {
	tpm_pcr_selection = &(tpm_pcr_info->pcrSelection);	/* get the TPM_PCR_SELECTION */
	rc = TPM_PCRSelection_GenerateDigest(tpm_digest,	/* output digest */
					     tpm_pcr_selection, /* input selection map */
					     tpm_pcrs);		/* points to the TPM PCR array */
    }
    return rc;
}

/* TPM_PCRInfo_CheckDigest() calculates a digestAtRelease based on the TPM_PCR_SELECTION
   and compares it to digestAtRelease in the structure.
*/

TPM_RESULT TPM_PCRInfo_CheckDigest(TPM_PCR_INFO *tpm_pcr_info,
				   TPM_PCRVALUE *tpm_pcrs)	/* points to the TPM PCR
								   array */
{
    TPM_RESULT		rc = 0;
    TPM_COMPOSITE_HASH	tpm_composite_hash;
    TPM_BOOL		pcrUsage;	/* TRUE if PCR's are specified */
	
    printf(" TPM_PCRInfo_CheckDigest:\n");
    /* Calculate a TPM_COMPOSITE_HASH of the PCR selected by tpm_pcr_info -> pcrSelection */
    if (rc == 0) {
	rc = TPM_PCRInfo_GetPCRUsage(&pcrUsage, tpm_pcr_info, 0);
    }
    if ((rc == 0) && pcrUsage) {
	rc = TPM_PCRSelection_GenerateDigest(tpm_composite_hash,
					     &(tpm_pcr_info->pcrSelection),
					     tpm_pcrs);		/* array of PCR's */
    }
    /* Compare to pcrInfo -> digestAtRelease on mismatch return TPM_WRONGPCRVAL */
    if ((rc == 0) && pcrUsage) {
	rc = TPM_Digest_Compare(tpm_composite_hash,
				tpm_pcr_info->digestAtRelease);
	if (rc != 0) {
	    printf("TPM_PCRInfo_CheckDigest: Error, wrong digestAtRelease value\n");
	    rc = TPM_WRONGPCRVAL;
	}
    }
    return rc;
}

/* TPM_PCRInfo_SetDigestAtCreation() calculates a digestAtCreation based on the TPM_PCR_SELECTION
   already set in the TPM_PCR_INFO structure.
*/

TPM_RESULT TPM_PCRInfo_SetDigestAtCreation(TPM_PCR_INFO *tpm_pcr_info,
					   TPM_PCRVALUE *tpm_pcrs)	/* points to the TPM PCR
									   array */
{
    TPM_RESULT		rc = 0;
	
    printf(" TPM_PCRInfo_SetDigestAtCreation:\n");
    if (rc == 0) {
	rc = TPM_PCRInfo_GenerateDigest(tpm_pcr_info->digestAtCreation, tpm_pcr_info, tpm_pcrs);
    }
    return rc;
}

/* TPM_PCRInfo_GetPCRUsage() returns 'pcrUsage' TRUE if any bit is set in the pcrSelect bit mask.

   'start_pcr' indicates the starting byte index into pcrSelect[]
*/

TPM_RESULT TPM_PCRInfo_GetPCRUsage(TPM_BOOL *pcrUsage,
				   TPM_PCR_INFO *tpm_pcr_info,
				   size_t start_index)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	done = FALSE;

    printf(" TPM_PCRInfo_GetPCRUsage: Start %lu\n", (unsigned long)start_index);
    if (rc == 0) {
	/* if a loaded key had no pcrInfo, the structure remains NULL */
	if (tpm_pcr_info == NULL) {
	    *pcrUsage = FALSE;
	    done = TRUE;
	}
    }
    if ((rc == 0) && !done) {
	rc = TPM_PCRSelection_GetPCRUsage(pcrUsage, &(tpm_pcr_info->pcrSelection), start_index);
    }
    if (rc == 0) {
	printf("  TPM_PCRInfo_GetPCRUsage: Result %d\n", *pcrUsage);
    }
    return rc;
}

/*
  TPM_PCR_INFO_LONG
*/

/* TPM_PCRInfoLong_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_PCRInfoLong_Init(TPM_PCR_INFO_LONG *tpm_pcr_info_long)
{
    printf(" TPM_PCRInfoLong_Init:\n");
/*     tpm_pcr_info_long->tag = TPM_TAG_PCR_INFO_LONG; */
    tpm_pcr_info_long->localityAtCreation = TPM_LOC_ZERO;
    tpm_pcr_info_long->localityAtRelease = TPM_LOC_ALL;
    TPM_PCRSelection_Init(&(tpm_pcr_info_long->creationPCRSelection));
    TPM_PCRSelection_Init(&(tpm_pcr_info_long->releasePCRSelection));
    TPM_Digest_Init(tpm_pcr_info_long->digestAtCreation);
    TPM_Digest_Init(tpm_pcr_info_long->digestAtRelease);
    return;
}

/* TPM_PCRInfoLong_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_PCRInfoLong_Init()
   After use, call TPM_PCRInfoLong_Delete() to free memory
*/

TPM_RESULT TPM_PCRInfoLong_Load(TPM_PCR_INFO_LONG *tpm_pcr_info_long,
				unsigned char **stream,
				uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_PCRInfoLong_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_PCR_INFO_LONG, stream, stream_size);
    }
    /* load localityAtCreation */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_pcr_info_long->localityAtCreation), stream, stream_size);
    }
    /* check locality value.  The TPM MAY treat a localityAtCreation value of 0 as an error. */
    if (rc == 0) {
	rc = TPM_LocalitySelection_CheckLegal(tpm_pcr_info_long->localityAtCreation);
    }
    /* load localityAtRelease */
    if (rc == 0) {
	rc = TPM_Load8(&(tpm_pcr_info_long->localityAtRelease), stream, stream_size);
    }
    /* check locality value */
    if (rc == 0) {
	rc = TPM_LocalitySelection_CheckLegal(tpm_pcr_info_long->localityAtRelease);
    }
    /* load creationPCRSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Load(&(tpm_pcr_info_long->creationPCRSelection), stream, stream_size);
    }
    /* load releasePCRSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Load(&(tpm_pcr_info_long->releasePCRSelection), stream, stream_size);
    }
    /* load digestAtCreation */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_pcr_info_long->digestAtCreation, stream, stream_size);
    }
    /* load digestAtRelease */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_pcr_info_long->digestAtRelease, stream, stream_size);
    }
    return rc;
}

/* TPM_PCRInfoLong_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_PCRInfoLong_Store(TPM_STORE_BUFFER *sbuffer,
				 const TPM_PCR_INFO_LONG *tpm_pcr_info_long)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_PCRInfoLong_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_PCR_INFO_LONG);
    }
    /* store localityAtCreation */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_pcr_info_long->localityAtCreation),
				sizeof(TPM_LOCALITY_SELECTION));
    }
    /* store localityAtRelease */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_pcr_info_long->localityAtRelease),
				sizeof(TPM_LOCALITY_SELECTION));
    }
    /* store creationPCRSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Store(sbuffer, &(tpm_pcr_info_long->creationPCRSelection));
    }
    /* store releasePCRSelection */
    if (rc == 0) {
	rc = TPM_PCRSelection_Store(sbuffer, &(tpm_pcr_info_long->releasePCRSelection));
    }
    /* store digestAtCreation */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_pcr_info_long->digestAtCreation);
    }
    /* store digestAtRelease */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_pcr_info_long->digestAtRelease);
    }
    return rc;
}

/* TPM_PCRInfoLong_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_PCRInfoLong_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_PCRInfoLong_Delete(TPM_PCR_INFO_LONG *tpm_pcr_info_long)
{
    printf(" TPM_PCRInfoLong_Delete:\n");
    if (tpm_pcr_info_long != NULL) {
	TPM_PCRSelection_Delete(&(tpm_pcr_info_long->creationPCRSelection));
	TPM_PCRSelection_Delete(&(tpm_pcr_info_long->releasePCRSelection));
	TPM_PCRInfoLong_Init(tpm_pcr_info_long);
    }
    return;
}

/* TPM_PCRInfoLong_Create() allocates memory for a TPM_PCR_INFO_LONG

*/

TPM_RESULT TPM_PCRInfoLong_Create(TPM_PCR_INFO_LONG **tpm_pcr_info_long)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRInfoLong_Create:\n");
    /* This function should never be called when the TPM_PCR_INFO_LONG structure has already been
       loaded.	This indicates an internal error. */
    if (rc == 0) {
	if (*tpm_pcr_info_long != NULL) {
	    printf("TPM_PCRInfoLong_Create: Error (fatal), TPM_PCR_INFO_LONG already loaded\n");
	    rc = TPM_FAIL;
	}
    }
    if (rc == 0) {
	rc = TPM_Malloc((unsigned char **)tpm_pcr_info_long, sizeof(TPM_PCR_INFO_LONG));
    }
    return rc;
}

/* TPM_PCRInfoLong_LoadFromBuffer() sets a TPM_PCR_INFO_LONG from a stream specified by a
   TPM_SIZED_BUFFER.  The TPM_SIZED_BUFFER is not modified.
*/

TPM_RESULT TPM_PCRInfoLong_LoadFromBuffer(TPM_PCR_INFO_LONG *tpm_pcr_info_long,
					  const TPM_SIZED_BUFFER *tpm_sized_buffer)
{
    TPM_RESULT	rc = 0;
    unsigned char *stream;
    uint32_t stream_size;

    printf(" TPM_PCRInfoLong_LoadFromBuffer:\n");
    if (rc == 0) {
	TPM_PCRInfoLong_Init(tpm_pcr_info_long);
	stream = tpm_sized_buffer->buffer;
	stream_size = tpm_sized_buffer->size;
	/* deserialize the TPM_SIZED_BUFFER into a TPM_PCR_INFO_LONG structure */
	rc = TPM_PCRInfoLong_Load(tpm_pcr_info_long, &stream, &stream_size);
    }
    return rc;
}

/* TPM_PCRInfoLong_CreateFromBuffer() allocates the TPM_PCR_INFO_LONG structure, typically a cache
   within another structure.  It then deserializes the TPM_SIZED_BUFFER into the structure.
   
   If the stream is empty, a NULL is returned.

   The TPM_SIZED_BUFFER is not modified.
*/

TPM_RESULT TPM_PCRInfoLong_CreateFromBuffer(TPM_PCR_INFO_LONG **tpm_pcr_info_long,
					    const TPM_SIZED_BUFFER *tpm_sized_buffer)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	done = FALSE;

    printf(" TPM_PCRInfoLong_CreateFromBuffer:\n");
    /* if there is no TPM_PCR_INFO_LONG - done */
    if (rc == 0) {
	if (tpm_sized_buffer->size == 0) {
	    done = TRUE;
	}
    }
    /* allocate memory for the buffer */
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfoLong_Create(tpm_pcr_info_long);
    }
    /* deserialize the input stream */
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfoLong_LoadFromBuffer(*tpm_pcr_info_long, tpm_sized_buffer);
    }
    return rc;
}

/* TPM_PCRInfoLong_Copy() copies the source to the destination */

TPM_RESULT TPM_PCRInfoLong_Copy(TPM_PCR_INFO_LONG *dest_tpm_pcr_info_long,
				TPM_PCR_INFO_LONG *src_tpm_pcr_info_long)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRInfoLong_Copy:\n");
    if (rc == 0) {
	/* copy the localityAtCreation, localityAtRelease */
	dest_tpm_pcr_info_long->localityAtCreation = src_tpm_pcr_info_long->localityAtCreation;
	dest_tpm_pcr_info_long->localityAtRelease = src_tpm_pcr_info_long->localityAtRelease;
	/* copy TPM_PCR_SELECTION creationPCRSelection */
	rc = TPM_PCRSelection_Copy(&(dest_tpm_pcr_info_long->creationPCRSelection),
				   &(src_tpm_pcr_info_long->creationPCRSelection));
    }
    if (rc == 0) {
	/* copy TPM_PCR_SELECTION releasePCRSelection*/
	rc = TPM_PCRSelection_Copy(&(dest_tpm_pcr_info_long->releasePCRSelection),
				   &(src_tpm_pcr_info_long->releasePCRSelection));
    }
    /* copy TPM_COMPOSITE_HASH's */
    if (rc == 0) {
	TPM_Digest_Copy(dest_tpm_pcr_info_long->digestAtRelease,
			src_tpm_pcr_info_long->digestAtRelease);
	TPM_Digest_Copy(dest_tpm_pcr_info_long->digestAtCreation,
			src_tpm_pcr_info_long->digestAtCreation);
    }
    return rc;
}

/* TPM_PCRInfoLong_CreateFromInfoLong() allocates memory for the TPM_PCR_INFO_LONG structure.  It
   copies the source tag, localityAtCreation, localityAtRelease, creationPCRSelection,
   releasePCRSelection digestAtCreation, and digestAtRelease.

   If the source is NULL, the destination is NULL.
*/

TPM_RESULT TPM_PCRInfoLong_CreateFromInfoLong(TPM_PCR_INFO_LONG **dest_tpm_pcr_info_long,
					      TPM_PCR_INFO_LONG *src_tpm_pcr_info_long)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	done = FALSE;

    printf(" TPM_PCRInfoLong_CreateFromInfoLong:\n");
    if (rc == 0) {
	/* if there is no source, leave the destination NULL */
	if (src_tpm_pcr_info_long == NULL) {
	    done = TRUE;
	}
    }
    /* create the structure */
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfoLong_Create(dest_tpm_pcr_info_long);
    }
    /* copy source to destination */
    if ((rc == 0) && !done) {
	rc = TPM_PCRInfoLong_Copy(*dest_tpm_pcr_info_long, src_tpm_pcr_info_long);
    }
    return rc;
}

/* TPM_PCRInfoLong_GenerateDigest() generates a Part 2 5.3.1 PCR composite hash

*/

TPM_RESULT TPM_PCRInfoLong_GenerateDigest(TPM_DIGEST tpm_digest,		/* output digest */
					  TPM_PCR_INFO_LONG *tpm_pcr_info_long, /* input */
					  TPM_PCRVALUE *tpm_pcrs) /* points to the TPM PCR array */
{
    TPM_RESULT		rc = 0;
    TPM_PCR_SELECTION	*tpm_pcr_selection;

    printf(" TPM_PCRInfoLong_GenerateDigest:\n");
    if (rc == 0) {
	if (tpm_pcr_info_long == NULL) {
	    printf("TPM_PCRInfoLong_GenerateDigest: Error (fatal), TPM_PCR_INFO_LONG is NULL\n");
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    if (rc == 0) {
	tpm_pcr_selection = &(tpm_pcr_info_long->creationPCRSelection); /* get TPM_PCR_SELECTION */
	rc = TPM_PCRSelection_GenerateDigest(tpm_digest,	/* output digest */
					     tpm_pcr_selection, /* input selection map */
					     tpm_pcrs);		/* points to the TPM PCR array */
    }
    return rc;
}

/* TPM_PCRInfoLong_CheckDigest() calculates a digestAtRelease based on the TPM_PCR_SELECTION
   and compares it to digestAtRelease in the structure.
*/

TPM_RESULT TPM_PCRInfoLong_CheckDigest(TPM_PCR_INFO_LONG *tpm_pcr_info_long,
				       TPM_PCRVALUE *tpm_pcrs,	/* points to the TPM PCR array */
				       TPM_MODIFIER_INDICATOR localityModifier)
{
    TPM_RESULT		rc = 0;
    TPM_COMPOSITE_HASH	tpm_composite_hash;
    TPM_BOOL		pcrUsage;	/* TRUE if PCR's are specified */
	
    printf(" TPM_PCRInfoLong_CheckDigest:\n");
    /* returns FALSE if tpm_pcr_info_long is NULL or selection bitmap is zero */
    if (rc == 0) {
	rc = TPM_PCRInfoLong_GetPCRUsage(&pcrUsage, tpm_pcr_info_long, 0);
    }
    /* Calculate a TPM_COMPOSITE_HASH of the PCR selected by tpm_pcr_info_long ->
       releasePCRSelection */
    if ((rc == 0) && pcrUsage) {
	rc = TPM_PCRSelection_GenerateDigest(tpm_composite_hash,
					     &(tpm_pcr_info_long->releasePCRSelection),
					     tpm_pcrs);		/* array of PCR's */
    }
    /* Compare to tpm_pcr_info_long -> digestAtRelease on mismatch return TPM_WRONGPCRVAL */
    if ((rc == 0) && pcrUsage) {
	rc = TPM_Digest_Compare(tpm_composite_hash,
				tpm_pcr_info_long->digestAtRelease);
	if (rc != 0) {
	    printf("TPM_PCRInfoLong_CheckDigest: Error, wrong digestAtRelease value\n");
	    rc = TPM_WRONGPCRVAL;
	}
    }
    /* If localityAtRelease is NOT 0x1f */
    if ((rc == 0) && (tpm_pcr_info_long != NULL)) {
	if (tpm_pcr_info_long->localityAtRelease != TPM_LOC_ALL) {
	    /* Validate that TPM_STANY_FLAGS -> localityModifier is matched by tpm_pcr_info_short ->
	       localityAtRelease on mismatch return TPM_BAD_LOCALITY */
	    rc = TPM_Locality_Check(tpm_pcr_info_long->localityAtRelease, localityModifier);
	}
    }
    return rc;
}

/* TPM_PCRInfoLong_SetDigestAtCreation() calculates a digestAtCreation based on the
   TPM_PCR_SELECTION creationPCRSelection already set in the TPM_PCR_INFO_LONG structure.
*/

TPM_RESULT TPM_PCRInfoLong_SetDigestAtCreation(TPM_PCR_INFO_LONG *tpm_pcr_info_long,
					       TPM_PCRVALUE *tpm_pcrs)	/* points to the TPM PCR
									   array */
{
    TPM_RESULT		rc = 0;
	
    printf(" TPM_PCRInfoLong_SetDigestAtCreation:\n");
    if (rc == 0) {
	rc = TPM_PCRInfoLong_GenerateDigest(tpm_pcr_info_long->digestAtCreation,
					    tpm_pcr_info_long,
					    tpm_pcrs);
    }
    return rc;
}

/* TPM_PCRInfoLong_GetPCRUsage() returns 'pcrUsage' TRUE if any bit is set in the pcrSelect bit
   mask.  Returns FALSE if the TPM_PCR_INFO_LONG is NULL.

   'start_pcr' indicates the starting byte index into pcrSelect[]
*/

TPM_RESULT TPM_PCRInfoLong_GetPCRUsage(TPM_BOOL *pcrUsage,
				       TPM_PCR_INFO_LONG *tpm_pcr_info_long,
				       size_t start_index)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	done = FALSE;

    printf(" TPM_PCRInfoLong_GetPCRUsage: Start %lu\n", (unsigned long)start_index);;
    if (rc == 0) {
	/* if a loaded key had no pcrInfo, the structure remains NULL */
	if (tpm_pcr_info_long == NULL) {
	    *pcrUsage = FALSE;
	    done = TRUE;
	}
    }
    if ((rc == 0) && !done) {
	rc = TPM_PCRSelection_GetPCRUsage(pcrUsage,
					  &(tpm_pcr_info_long->releasePCRSelection), start_index);
    }
    if (rc == 0) {
	printf("  TPM_PCRInfoLong_GetPCRUsage: Result %d\n", *pcrUsage);
    }
    return rc;
}


/*
  TPM_PCR_SELECTION
*/

void TPM_PCRSelection_Init(TPM_PCR_SELECTION *tpm_pcr_selection)
{
    size_t i;

    printf(" TPM_PCRSelection_Init:\n");
    tpm_pcr_selection->sizeOfSelect = TPM_NUM_PCR/CHAR_BIT;
    for (i = 0 ; i < (TPM_NUM_PCR/CHAR_BIT) ; i++) {
	tpm_pcr_selection->pcrSelect[i] = 0;
    }
    return;
}

/* TPM_PCRSelection_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes

   After use, call TPM_PCRSelection_Delete() to free memory
*/

TPM_RESULT TPM_PCRSelection_Load(TPM_PCR_SELECTION *tpm_pcr_selection,
				 unsigned char **stream,
				 uint32_t *stream_size)
{
    TPM_RESULT	rc = 0;
    size_t	i;
    
    printf(" TPM_PCRSelection_Load:\n");
    /* load sizeOfSelect */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_pcr_selection->sizeOfSelect), stream, stream_size);
    }
    /* test sizeOfSelect value */
    if (rc == 0) {
	rc = TPM_PCRSelection_CheckRange(tpm_pcr_selection);
    }	
    /* load pcrSelect map */
    for (i = 0 ; (rc == 0) && (i < tpm_pcr_selection->sizeOfSelect) ; i++) {
	rc = TPM_Load8(&(tpm_pcr_selection->pcrSelect[i]), stream, stream_size);
    }
    /* if there was insufficient input, zero the rest of the map */
    for ( ; (rc == 0) && (i < (TPM_NUM_PCR/CHAR_BIT)) ; i++) {
	rc = tpm_pcr_selection->pcrSelect[i] = 0;
    }
    return rc;
}

/* TPM_PCRSelection_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes

   After use, call TPM_Sbuffer_Delete() to free memory
*/

TPM_RESULT TPM_PCRSelection_Store(TPM_STORE_BUFFER *sbuffer,
				  const TPM_PCR_SELECTION *tpm_pcr_selection)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_PCRSelection_Store:\n");
    /* NOTE: Cannot use TPM_SizedBuffer_Store since the first parameter is a uint16_t */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_pcr_selection->sizeOfSelect);
    }
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer,
				tpm_pcr_selection->pcrSelect, tpm_pcr_selection->sizeOfSelect);
    }
    return rc;
}


/* TPM_PCRSelection_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the PCRSelection
   sets pointers to NULL
   calls TPM_PCRSelection_Init to set members back to default values
   The PCRSelection itself is not freed
   returns 0 or error codes
*/   

void TPM_PCRSelection_Delete(TPM_PCR_SELECTION *tpm_pcr_selection)
{
    printf(" TPM_PCRSelection_Delete:\n");
    if (tpm_pcr_selection != NULL) {
	TPM_PCRSelection_Init(tpm_pcr_selection);
    }
    return;
}

/* TPM_PCRSelection_Copy() copies the source to the destination

   It returns an error if the source -> sizeOfSelect is too large.  If the source is smaller than
   the internally defined, fixed size of the destination, the remainder of the destination is filled
   with 0's.
*/

TPM_RESULT TPM_PCRSelection_Copy(TPM_PCR_SELECTION *destination, TPM_PCR_SELECTION *source)
{
    TPM_RESULT	rc = 0;
    size_t	i;
    
    printf(" TPM_PCRSelection_Copy:\n");
    if (rc == 0) {
	rc = TPM_PCRSelection_CheckRange(source);
    }
    if (rc == 0) {
	/* copy sizeOfSelect member */
	destination->sizeOfSelect = source->sizeOfSelect;
	/* copy pcrSelect map up to the size of the source */
	for (i = 0 ; i < source->sizeOfSelect ; i++) {
	    destination->pcrSelect[i] = source->pcrSelect[i];
	}
	/* if the input wasn't sufficient, zero the rest of the map */
	for ( ; i < (TPM_NUM_PCR/CHAR_BIT) ; i++) {
	    destination->pcrSelect[i] = 0;
	}
    }
    return rc;
}

/* TPM_PCRSelection_GenerateDigest() generates a digest based on the TPM_PCR_SELECTION and the
   current TPM PCR values.

   It internally generates a TPM_PCR_COMPOSITE according to Part 2 5.4.1.  To return this structure
   as well, use TPM_PCRSelection_GenerateDigest2().
*/

TPM_RESULT TPM_PCRSelection_GenerateDigest(TPM_DIGEST tpm_digest, /* output digest */
					   TPM_PCR_SELECTION *tpm_pcr_selection, /* input selection
										    map */
					   TPM_PCRVALUE *tpm_pcrs) /* points to the TPM PCR array */
{
    TPM_RESULT		rc = 0;
    TPM_PCR_COMPOSITE	tpm_pcr_composite;	/* structure to be hashed */

    printf(" TPM_PCRSelection_GenerateDigest:\n");
    TPM_PCRComposite_Init(&tpm_pcr_composite);		/* freed @1 */
    rc = TPM_PCRSelection_GenerateDigest2(tpm_digest,
					  &tpm_pcr_composite,
					  tpm_pcr_selection,
					  tpm_pcrs);
    TPM_PCRComposite_Delete(&tpm_pcr_composite);	/* @1 */
    return rc;
}

/* TPM_PCRSelection_GenerateDigest2() generates a digest based on the TPM_PCR_SELECTION and the
   current TPM PCR values.

   It first generates a TPM_PCR_COMPOSITE according to Part 2 5.4.1.  That structure is also
   returned.

   TPM_PCR_COMPOSITE should be initialized and deleted by the caller.  To generate and delete the
   structure internally, use TPM_PCRSelection_GenerateDigest().
*/

TPM_RESULT TPM_PCRSelection_GenerateDigest2(TPM_DIGEST tpm_digest, /* output digest */
					    TPM_PCR_COMPOSITE *tpm_pcr_composite, /* output
										     structure
										   */
					    TPM_PCR_SELECTION *tpm_pcr_selection, /* input selection
										     map */
					    TPM_PCRVALUE *tpm_pcrs) /* points to the TPM PCR
								       array */
{
    TPM_RESULT		rc = 0;
    TPM_BOOL		pcrUsage;

    printf(" TPM_PCRSelection_GenerateDigest2:\n");
    /* assemble the TPM_PCR_COMPOSITE structure */
    if (rc == 0) {
	rc = TPM_PCRComposite_Set(tpm_pcr_composite, tpm_pcr_selection, tpm_pcrs);
    }
    if (rc == 0) {
	rc = TPM_PCRSelection_GetPCRUsage(&pcrUsage, tpm_pcr_selection, 0);
    }
    if (rc == 0) {
	printf("  TPM_PCRSelection_GenerateDigest2: pcrUsage %02x\n", pcrUsage);
	if (pcrUsage) {
	    /* serialize and hash TPM_PCR_COMPOSITE */
	    if (rc == 0) {
		rc = TPM_SHA1_GenerateStructure(tpm_digest, tpm_pcr_composite,
						(TPM_STORE_FUNCTION_T)TPM_PCRComposite_Store);
	    }
	}
	/* 4. If TPM_PCR_SELECTION.pcrSelect is all 0's */
	/* a. a.For digestAtCreation, the TPM MUST set TPM_COMPOSITE_HASH to be all 0's. */
	else {
	    TPM_Digest_Init(tpm_digest);
	}
    }
    return rc;
}

/* TPM_PCRSelection_GetPCRUsage() returns 'pcrUsage' TRUE if any bit is set in the pcrSelect bit
   mask.

   'start_pcr' indicates the starting byte index into pcrSelect[].
*/

TPM_RESULT TPM_PCRSelection_GetPCRUsage(TPM_BOOL *pcrUsage,
					const TPM_PCR_SELECTION *tpm_pcr_selection,
					size_t start_index)
{
    TPM_RESULT	rc = 0;
    size_t	i;
    
    printf(" TPM_PCRSelection_GetPCRUsage: Start %lu\n", (unsigned long)start_index);
    if (rc == 0) {
	rc = TPM_PCRSelection_CheckRange(tpm_pcr_selection);
    }
    if (rc == 0) {
	*pcrUsage = FALSE;
	/* If sizeOfSelect is 0 or start_index is past the end, this loop won't be entered and FALSE
	   will be returned */
	for (i = start_index ; i < tpm_pcr_selection->sizeOfSelect ; i++) {
	    if (tpm_pcr_selection->pcrSelect[i] != 0) { /* is any bit set in the mask */
		*pcrUsage = TRUE;
		break;
	    }
	}
    }	 
    return rc;
}

/* TPM_PCRSelection_CheckRange() checks the sizeOfSelect index

*/

TPM_RESULT TPM_PCRSelection_CheckRange(const TPM_PCR_SELECTION *tpm_pcr_selection)
{
    TPM_RESULT	rc = 0;

    if (tpm_pcr_selection->sizeOfSelect > (TPM_NUM_PCR/CHAR_BIT)) {
	printf("TPM_PCRSelection_CheckRange: Error, sizeOfSelect %u must be 0 - %u\n",
	       tpm_pcr_selection->sizeOfSelect, TPM_NUM_PCR/CHAR_BIT);
	rc = TPM_INVALID_PCR_INFO;
    }
    return rc;
}

/* TPM_PCRSelection_Compare() compares the TPM_PCR_SELECTION's for equality

*/

void TPM_PCRSelection_Compare(TPM_BOOL *match,
			      TPM_PCR_SELECTION *tpm_pcr_selection1,
			      TPM_PCR_SELECTION *tpm_pcr_selection2)
{
    size_t i;
    *match = TRUE;

    if (tpm_pcr_selection1->sizeOfSelect != tpm_pcr_selection2->sizeOfSelect) {
	*match = FALSE;
    }
    for (i = 0 ; *match && (i < tpm_pcr_selection1->sizeOfSelect) ; i++) {
	if (tpm_pcr_selection1->pcrSelect[i] != tpm_pcr_selection2->pcrSelect[i]) {
	    *match = FALSE;
	}
    }	
    return;
}

#if 0
/* TPM_PCRSelection_LessThan() compares the new selection to the old selection.	 It returns lessThan
   TRUE is the new selection does not select a PCR that was selected by the old selection.
*/

void TPM_PCRSelection_LessThan(TPM_BOOL *lessThan,
			       TPM_PCR_SELECTION *tpm_pcr_selection_new,
			       TPM_PCR_SELECTION *tpm_pcr_selection_old)
{
    size_t i;
    *lessThan = TRUE;

    if (tpm_pcr_selection_new->sizeOfSelect != tpm_pcr_selection_old->sizeOfSelect) {
	*lessThan = FALSE;
    }
    for (i = 0 ; *lessThan && (i < tpm_pcr_selection_new->sizeOfSelect) ; i++) {
	/* if there's a 0 in the new selection and a 1 on the old selection */
	if (~(tpm_pcr_selection_new->pcrSelect[i]) & tpm_pcr_selection_old->pcrSelect[i]) {
	    *lessThan = FALSE;
	}
    }	
    return;
}
#endif


/*
  TPM_QUOTE_INFO
*/

/* TPM_QuoteInfo_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_QuoteInfo_Init(TPM_QUOTE_INFO *tpm_quote_info)
{
    printf(" TPM_QuoteInfo_Init:\n");
    TPM_StructVer_Init(&(tpm_quote_info->version));
    memcpy(&(tpm_quote_info->fixed), "QUOT", 4);
    TPM_Digest_Init(tpm_quote_info->digestValue);
    TPM_Nonce_Init(tpm_quote_info->externalData);
    return;
}

#if 0
/* TPM_QuoteInfo_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_QuoteInfo_Init()
   After use, call TPM_QuoteInfo_Delete() to free memory

   NOTE: Never called.
*/

TPM_RESULT TPM_QuoteInfo_Load(TPM_QUOTE_INFO *tpm_quote_info,
			      unsigned char **stream,
			      uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_QuoteInfo_Load:\n");
    /* load version */
    if (rc == 0) {
	rc = TPM_StructVer_Load(&(tpm_quote_info->version), stream, stream_size);
    }
    /* check ver immediately to ease debugging */
    if (rc == 0) {
	rc = TPM_StructVer_CheckVer(&(tpm_quote_info->version));
    }
    /* load fixed */
    if (rc == 0) {
	rc = TPM_Loadn(tpm_quote_info->fixed, 4, stream, stream_size);
    }
    /* load digestValue */
    if (rc == 0) {
	rc = TPM_Digest_Load(tpm_quote_info->digestValue, stream, stream_size);
    }
    /* load externalData */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_quote_info->externalData, stream, stream_size);
    }
    return rc;
}
#endif

/* TPM_QuoteInfo_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_QuoteInfo_Store(TPM_STORE_BUFFER *sbuffer,
			       const TPM_QUOTE_INFO *tpm_quote_info)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_QuoteInfo_Store:\n");
    /* store version */
    if (rc == 0) {
	rc = TPM_StructVer_Store(sbuffer, &(tpm_quote_info->version));
    }
    /* store fixed */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, tpm_quote_info->fixed, 4);
    }
    /* store digestValue */
    if (rc == 0) {
	rc = TPM_Digest_Store(sbuffer, tpm_quote_info->digestValue);
    }
    /* store externalData */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_quote_info->externalData);
    }
    return rc;
}

/* TPM_QuoteInfo_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_QuoteInfo_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_QuoteInfo_Delete(TPM_QUOTE_INFO *tpm_quote_info)
{
    printf(" TPM_QuoteInfo_Delete:\n");
    if (tpm_quote_info != NULL) {
	TPM_QuoteInfo_Init(tpm_quote_info);
    }
    return;
}

/*
  TPM_QUOTE_INFO2
*/

/* TPM_QuoteInfo2_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_QuoteInfo2_Init(TPM_QUOTE_INFO2 *tpm_quote_info2)
{
    printf(" TPM_QuoteInfo2_Init:\n");
    memcpy(tpm_quote_info2->fixed, "QUT2", 4);
    TPM_Nonce_Init(tpm_quote_info2->externalData);
    TPM_PCRInfoShort_Init(&(tpm_quote_info2->infoShort));
    return;
}

#if 0
/* TPM_QuoteInfo2_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_QuoteInfo2_Init()
   After use, call TPM_QuoteInfo2_Delete() to free memory
*/

TPM_RESULT TPM_QuoteInfo2_Load(TPM_QUOTE_INFO2 *tpm_quote_info2,
			       unsigned char **stream,
			       uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_QuoteInfo2_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_QUOTE_INFO2, stream, stream_size);
    }
    /* load fixed */
    if (rc == 0) {
	rc = TPM_Loadn(tpm_quote_info2->fixed, 4, stream, stream_size);
    }
    /* load externalData */
    if (rc == 0) {
	rc = TPM_Nonce_Load(tpm_quote_info2->externalData, stream, stream_size);
    }
    /* load infoShort */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_Load(&(tpm_quote_info2->infoShort), stream, stream_size, FALSE);
    }
    return rc;
}
#endif

/* TPM_QuoteInfo2_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_QuoteInfo2_Store(TPM_STORE_BUFFER *sbuffer,
				const TPM_QUOTE_INFO2 *tpm_quote_info2)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_QuoteInfo2_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_QUOTE_INFO2);
    }
    /* store fixed */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, tpm_quote_info2->fixed, 4);
    }
    /* store externalData */
    if (rc == 0) {
	rc = TPM_Nonce_Store(sbuffer, tpm_quote_info2->externalData);
    }
    /* store infoShort */
    if (rc == 0) {
	rc = TPM_PCRInfoShort_Store(sbuffer, &(tpm_quote_info2->infoShort), FALSE);
    }
    return rc;
}

/* TPM_QuoteInfo2_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_QuoteInfo2_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_QuoteInfo2_Delete(TPM_QUOTE_INFO2 *tpm_quote_info2)
{
    printf(" TPM_QuoteInfo2_Delete:\n");
    if (tpm_quote_info2 != NULL) {
	TPM_PCRInfoShort_Delete(&(tpm_quote_info2->infoShort));
	TPM_QuoteInfo2_Init(tpm_quote_info2);
    }
    return;
}

/*
  Command Processing Functions
*/


/* 16.2 TPM_PCRRead rev 109

   The TPM_PCRRead operation provides non-cryptographic reporting of the contents of a named PCR.
*/

TPM_RESULT TPM_Process_PcrRead(tpm_state_t *tpm_state,
			       TPM_STORE_BUFFER *response,
			       TPM_TAG tag,
			       uint32_t paramSize,
			       TPM_COMMAND_CODE ordinal,
			       unsigned char *command,
			       TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT		rcf = 0;			/* fatal error precluding response */
    TPM_RESULT		returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_PCRINDEX pcrIndex;	/* Index of the PCR to be read */

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
    TPM_PCRVALUE	outDigest;
    
    printf("TPM_Process_PcrRead: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get pcrIndex parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&pcrIndex, &command, &paramSize);
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
	    printf("TPM_Process_PcrRead: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1.Validate that pcrIndex represents a legal PCR number. On error, return TPM_BADINDEX. */
    /* 2. Set outDigest to TPM_STCLEAR_DATA -> PCR[pcrIndex] */
    /* NOTE Done by TPM_PCR_Load() */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_PcrRead: pcrIndex %u\n", pcrIndex);
	returnCode = TPM_PCR_Load(outDigest,
				  tpm_state->tpm_stclear_data.PCRS,
				  pcrIndex);
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_PcrRead: PCR value", outDigest);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_PcrRead: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* append outDigest */
	    returnCode = TPM_Digest_Store(response, outDigest);
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
    return rcf;
}

/* 16.3 TPM_Quote rev 101

   The TPM_Quote operation provides cryptographic reporting of PCR values. A loaded key is required
   for operation. TPM_Quote uses a key to sign a statement that names the current value of a chosen
   PCR and externally supplied data (which may be a nonce supplied by a Challenger).

   The term "ExternalData" is used because an important use of TPM_Quote is to provide a digital
   signature on arbitrary data, where the signature includes the PCR values of the platform at time
   of signing. Hence the "ExternalData" is not just for anti-replay purposes, although it is (of
   course) used for that purpose in an integrity challenge.
*/

TPM_RESULT TPM_Process_Quote(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	keyHandle;	/* The keyHandle identifier of a loaded key that can sign
					   the PCR values. */
    TPM_NONCE		externalData;	/* 160 bits of externally supplied data (typically a nonce
					   provided by a server to prevent replay-attacks) */
    TPM_PCR_SELECTION	targetPCR;	/* The indices of the PCRs that are to be reported. */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for keyHandle
					   authorization. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   handle */
    TPM_AUTHDATA	privAuth;	/* The authorization digest for inputs and keyHandle. HMAC
					   key: key -> usageAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey;
    TPM_KEY			*sigKey = NULL;			/* the key specified by keyHandle */
    TPM_SECRET			*keyUsageAuth;
    TPM_BOOL			parentPCRStatus;
    TPM_QUOTE_INFO		q1QuoteInfo;
    TPM_DIGEST			q1_digest;
    
    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_PCR_COMPOSITE	pcrData;	/* A structure containing the same indices as
					   targetPCR, plus the corresponding current PCR
					   values. */
    TPM_SIZED_BUFFER	sig;		/* The signed data blob. */

    printf("TPM_Process_Quote: Ordinal Entry\n");
    TPM_PCRSelection_Init(&targetPCR);		/* freed @1 */
    TPM_PCRComposite_Init(&pcrData);		/* freed @2 */
    TPM_QuoteInfo_Init(&q1QuoteInfo);		/* freed @3 */
    TPM_SizedBuffer_Init(&sig);			/* freed @4 */
    /*
      get inputs
    */
    /* get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get externalData parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Quote: keyHandle %08x\n", keyHandle);
	returnCode = TPM_Nonce_Load(externalData, &command, &paramSize);
    }
    /* get targetPCR parameter */
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_Quote: externalData", externalData);
	returnCode = TPM_PCRSelection_Load(&targetPCR, &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag10(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					privAuth,
					&command, &paramSize);
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	printf("TPM_Process_Quote: authHandle %08x\n", authHandle);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_Quote: Error, command has %u extra bytes\n",
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
    /* get the key corresponding to the keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&sigKey, &parentPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not r/o, used to sign */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)) {
	if (sigKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_Quote: Error, authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* get keyHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&keyUsageAuth, sigKey);
    }	 
    /* get the session data */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      sigKey,
					      keyUsageAuth,		/* OIAP */
					      sigKey->tpm_store_asymkey->pubDataDigest); /* OSAP */
    }
    /* 1. The TPM MUST validate the authorization to use the key pointed to by keyHandle. */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					privAuth);		/* Authorization digest for input */
    }
    /* 2. Validate that keyHandle -> sigScheme is TPM_SS_RSASSAPKCS1v15_SHA1 or
       TPM_SS_RSASSAPKCS1v15_INFO,, if not return TPM_INAPPROPRIATE_SIG. */
    if (returnCode == TPM_SUCCESS) {
	if ((sigKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) &&
	    (sigKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_INFO)) {
	    printf("TPM_Process_Quote: Error, invalid sigKey sigScheme %04hx\n",
		   sigKey->algorithmParms.sigScheme);
	    returnCode = TPM_INAPPROPRIATE_SIG;
	}
    }
    /* 3. Validate that keyHandle -> keyUsage is TPM_KEY_SIGNING, TPM_KEY_IDENTITY or
	  TPM_KEY_LEGACY, if not return TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if ((sigKey->keyUsage != TPM_KEY_SIGNING) &&
	    ((sigKey->keyUsage) != TPM_KEY_IDENTITY) &&
	    ((sigKey->keyUsage) != TPM_KEY_LEGACY)) {
	    printf("TPM_Process_Quote: Error, keyUsage %04hx is invalid\n", sigKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 4. Validate targetPCR */
    /* a. targetPCR is a valid TPM_PCR_SELECTION structure */
    /* b. On errors return TPM_INVALID_PCR_INFO */
    /* NOTE: done during TPM_PCRSelection_Load() */
    /* 5. Create H1 a SHA-1 hash of a TPM_PCR_COMPOSITE using the PCRs indicated by targetPCR ->
       pcrSelect */
    /* NOTE TPM_PCRSelection_GenerateDigest2() generates the TPM_PCR_COMPOSITE as well. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_PCRSelection_GenerateDigest2(q1QuoteInfo.digestValue,
						      &pcrData,		/* TPM_PCR_COMPOSITE */
						      &targetPCR,
						      tpm_state->tpm_stclear_data.PCRS);
    }
    /* 6. Create Q1 a TPM_QUOTE_INFO structure */
    /* a. Set Q1 -> version to 1.1.0.0 */
    /* b. Set Q1 -> fixed to "QUOT" */
    /* NOTE: done at TPM_QuoteInfo_Init() */
    /* c. Set Q1 -> digestValue to H1 */
    /* NOTE: Generated directly in Q1 */
    /* d. Set Q1 -> externalData to externalData */
    if (returnCode == TPM_SUCCESS) {
	TPM_Nonce_Copy(q1QuoteInfo.externalData, externalData);
    }
    /* 7. Sign SHA-1 hash of Q1 using keyHandle as the signature key */
    /* digest Q1 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1_GenerateStructure(q1_digest, &q1QuoteInfo,
						(TPM_STORE_FUNCTION_T)TPM_QuoteInfo_Store);
    }
    /* sign the Q1 digest */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_RSASignToSizedBuffer(&sig,		/* signature */
					      q1_digest,	/* message */
					      TPM_DIGEST_SIZE,	/* message size */
					      sigKey);		/* signing key and parameters */
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_Quote: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return the pcrData */
	    returnCode = TPM_PCRComposite_Store(response, &pcrData);
	}
	/* 8. Return the signature in sig */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_SizedBuffer_Store(response, &sig);
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
    TPM_PCRSelection_Delete(&targetPCR);	/* @1 */
    TPM_PCRComposite_Delete(&pcrData);		/* @2 */
    TPM_QuoteInfo_Delete(&q1QuoteInfo);		/* @3 */
    TPM_SizedBuffer_Delete(&sig);		/* @4 */
    return rcf;
}


/* 16.5 TPM_Quote2 rev 96

   The TPM_Quote operation provides cryptographic reporting of PCR values. A loaded key is required
   for operation. TPM_Quote uses a key to sign a statement that names the current value of a chosen
   PCR and externally supplied data (which may be a nonce supplied by a Challenger).
  
   The term "ExternalData" is used because an important use of TPM_Quote is to provide a digital
   signature on arbitrary data, where the signature includes the PCR values of the platform at time
   of signing. Hence the "ExternalData" is not just for anti-replay purposes, although it is (of
   course) used for that purpose in an integrity challenge.

   Quote2 differs from quote in that Quote2 uses TPM_PCR_INFO_SHORT to hold information relative to
   the PCR registers. INFO_SHORT includes locality information to provide the requester a more
   complete view of the current platform configuration.
*/

TPM_RESULT TPM_Process_Quote2(tpm_state_t *tpm_state,
			      TPM_STORE_BUFFER *response,
			      TPM_TAG tag,
			      uint32_t paramSize,
			      TPM_COMMAND_CODE ordinal,
			      unsigned char *command,
			      TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT		rcf = 0;			/* fatal error precluding response */
    TPM_RESULT		returnCode = TPM_SUCCESS;	/* command return code */

    /* input parameters */
    TPM_KEY_HANDLE	keyHandle;	/* The keyHandle identifier of a loaded key that can sign
					   the PCR values. */
    TPM_NONCE		externalData;	/* 160 bits of externally supplied data (typically a nonce
					   provided by a server to prevent replay-attacks) */
    TPM_PCR_SELECTION	targetPCR;	/* The indices of the PCRs that are to be reported. */
    TPM_BOOL		addVersion;	/* When TRUE add TPM_CAP_VERSION_INFO to the output */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for keyHandle
					   authorization. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization session
						   handle */
    TPM_AUTHDATA	privAuth;	/* The authorization session digest for inputs and
					   keyHandle. HMAC key: key -> usageAuth. */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey;
    TPM_KEY			*sigKey = NULL;			/* the key specified by keyHandle */
    TPM_SECRET			*keyUsageAuth;
    TPM_BOOL			parentPCRStatus;
    TPM_COMPOSITE_HASH		h1CompositeHash;
    TPM_QUOTE_INFO2		q1;
    TPM_PCR_INFO_SHORT		*s1 = NULL;
    TPM_STORE_BUFFER		q1_sbuffer;
    TPM_STORE_BUFFER		versionInfo_sbuffer;
    const unsigned char		*versionInfo_buffer;
    TPM_DIGEST			q1_digest;
     
    /* output parameters */
    uint32_t			outParamStart;	/* starting point of outParam's */
    uint32_t			outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST			outParamDigest;
    uint32_t			versionInfoSize;	/* Size of the version info */
    TPM_CAP_VERSION_INFO	versionInfo;	/* The version info */
    TPM_SIZED_BUFFER		sig;		/* The signed data blob. */

    printf("TPM_Process_Quote2: Ordinal Entry\n");
    TPM_PCRSelection_Init(&targetPCR);						/* freed @1 */
    TPM_CapVersionInfo_Set(&versionInfo, &(tpm_state->tpm_permanent_data));	/* freed @2 */
    TPM_SizedBuffer_Init(&sig);							/* freed @3 */
    TPM_QuoteInfo2_Init(&q1);							/* freed @4 */
    TPM_Sbuffer_Init(&q1_sbuffer);						/* freed @5 */
    TPM_Sbuffer_Init(&versionInfo_sbuffer);					/* freed @6 */
    /*
      get inputs
    */
    /* get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get externalData */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Quote2: keyHandle %08x\n", keyHandle);
	returnCode = TPM_Nonce_Load(externalData, &command, &paramSize);
    }
    /* get targetPCR parameter */
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_Quote2: externalData", externalData);
	returnCode = TPM_PCRSelection_Load(&targetPCR, &command, &paramSize);
    }
    /* get addVersion parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_LoadBool(&addVersion, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_Quote2: addVersion %02x\n", addVersion);
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
	returnCode = TPM_CheckRequestTag10(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					privAuth,
					&command, &paramSize);
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	printf("TPM_Process_Quote2: authHandle %08x\n", authHandle);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_Quote2: Error, command has %u extra bytes\n",
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
    /* get the key corresponding to the keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyHandleEntries_GetKey(&sigKey, &parentPCRStatus, tpm_state, keyHandle,
						 FALSE,		/* not r/o, used to sign */
						 FALSE,		/* do not ignore PCRs */
						 FALSE);	/* cannot use EK */
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)) {
	if (sigKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_Quote2: Error, authorization required\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }
    /* get keyHandle -> usageAuth */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Key_GetUsageAuth(&keyUsageAuth, sigKey);
    }	 
    /* get the session data */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthSessions_GetData(&auth_session_data,
					      &hmacKey,
					      tpm_state,
					      authHandle,
					      TPM_PID_NONE,
					      TPM_ET_KEYHANDLE,
					      ordinal,
					      sigKey,
					      keyUsageAuth,		/* OIAP */
					      sigKey->tpm_store_asymkey->pubDataDigest); /* OSAP */
    }
    /* 1. The TPM MUST validate the AuthData to use the key pointed to by keyHandle. */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					privAuth);		/* Authorization digest for input */
    }
    /* 2. Validate that keyHandle -> sigScheme is TPM_SS_RSASSAPKCS1v15_SHA1, if not return
       TPM_INAPPROPRIATE_SIG. */
    if (returnCode == TPM_SUCCESS) {
	if ((sigKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) &&
	    (sigKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_INFO)) {
	    printf("TPM_Process_Quote2: Error, inappropriate signature scheme %04x\n",
		   sigKey->algorithmParms.sigScheme);
	    returnCode = TPM_INAPPROPRIATE_SIG;
	}
    }
    /* 3. Validate that keyHandle -> keyUsage is TPM_KEY_SIGNING, TPM_KEY_IDENTITY or
       TPM_KEY_LEGACY, if not return TPM_INVALID_KEYUSAGE */
    if (returnCode == TPM_SUCCESS) {
	if ((sigKey->keyUsage != TPM_KEY_SIGNING) &&
	    ((sigKey->keyUsage) != TPM_KEY_IDENTITY) &&
	    ((sigKey->keyUsage) != TPM_KEY_LEGACY)) {
	    printf("TPM_Process_Quote2: Error, keyUsage %04hx is invalid\n", sigKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 4. Validate targetPCR is a valid TPM_PCR_SELECTION structure, on errors return
       TPM_INVALID_PCR_INFO */
    /* NOTE: done during TPM_PCRSelection_Load() */
    /* 5. Create H1 a SHA-1 hash of a TPM_PCR_COMPOSITE using the PCRs indicated by targetPCR ->
       pcrSelect */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_PCRSelection_GenerateDigest(h1CompositeHash,
						     &targetPCR,
						     tpm_state->tpm_stclear_data.PCRS);
    }
    if (returnCode == TPM_SUCCESS) {
	/* 6. Create S1 a TPM_PCR_INFO_SHORT */
	s1 = &(q1.infoShort);
	/* a. Set S1->pcrSelection to pcrSelect */
	returnCode = TPM_PCRSelection_Copy(&(s1->pcrSelection), &targetPCR);
    }
    /* b. Set S1->localityAtRelease to TPM_STANY_DATA -> localityModifier */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Locality_Set(&(s1->localityAtRelease),
				      tpm_state->tpm_stany_flags.localityModifier);
    }
    /* c. Set S1->digestAtRelease to H1 */
    if (returnCode == TPM_SUCCESS) {
	TPM_Digest_Copy(s1->digestAtRelease, h1CompositeHash);
	/* 7. Create Q1 a TPM_QUOTE_INFO2 structure */
	/* a. Set Q1 -> fixed to "QUT2" */
	/* NOTE: done at TPM_QuoteInfo2_Init() */
	/* b. Set Q1 -> infoShort to S1 */
	/* NOTE: created S1 in place */
	/* c. Set Q1 -> externalData to externalData */
	TPM_Nonce_Copy(q1.externalData, externalData);
	/* serialize q1 */
	returnCode = TPM_QuoteInfo2_Store(&q1_sbuffer, &q1);
    }
    if (returnCode == TPM_SUCCESS) {
	/* 8. If addVersion is TRUE */
	if (addVersion) {
	    if (returnCode == TPM_SUCCESS) {
		/* a. Concatenate to Q1 a TPM_CAP_VERSION_INFO structure */
		/* b. Set the output parameters for versionInfo */
		/* serialize versionInfo.  The result cannot be added directly to q1_sbuffer because
		   it is needed as an outgoing parameter */
		/* NOTE: Created at TPM_CapVersionInfo_Set() */
		returnCode = TPM_CapVersionInfo_Store(&versionInfo_sbuffer, &versionInfo);
	    }
	    if (returnCode == TPM_SUCCESS) {
		/* get the serialized results */
		TPM_Sbuffer_Get(&versionInfo_sbuffer, &versionInfo_buffer, &versionInfoSize);
		/* concatenate TPM_CAP_VERSION_INFO versionInfo to TPM_QUOTE_INFO2 q1 buffer */
		returnCode = TPM_Sbuffer_Append(&q1_sbuffer, versionInfo_buffer, versionInfoSize);
	    }
	}
	/* 9. Else */
	else {
	    /* a. Set versionInfoSize to 0 */
	    versionInfoSize = 0;
	    /* b. Return no bytes in versionInfo */
	    /* NOTE Done at response, (&& addVersion) */
	}
    }
    /* 10. Sign a SHA-1 hash of Q1 using keyHandle as the signature key */
    /* hash q1 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SHA1Sbuffer(q1_digest, &q1_sbuffer);
    }
    /* sign the Q1 digest */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_RSASignToSizedBuffer(&sig,		/* signature */
					      q1_digest,	/* message */
					      TPM_DIGEST_SIZE,	/* message size */
					      sigKey);		/* signing key and parameters */
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_Quote2: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return the TPM_PCR_INFO_SHORT pcrData */
	    returnCode = TPM_PCRInfoShort_Store(response, s1, FALSE);
	}
	/* An email clarification said that, if addVersion is FALSE, a versionInfoSize is 0 is
	   returned.  This indicates the missing versionInfo. */
	/* return the versionInfoSize */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Sbuffer_Append32(response, versionInfoSize);
	}
	/* return the versionInfo */
	if ((returnCode == TPM_SUCCESS) && addVersion) {
	    returnCode = TPM_Sbuffer_Append(response, versionInfo_buffer, versionInfoSize);
	}
	/* 11. Return the signature in sig */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_SizedBuffer_Store(response, &sig);
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
    TPM_PCRSelection_Delete(&targetPCR);					/* @1 */
    TPM_CapVersionInfo_Delete(&versionInfo);					/* @2 */
    TPM_SizedBuffer_Delete(&sig);						/* @3 */
    TPM_QuoteInfo2_Delete(&q1);							/* @4 */
    TPM_Sbuffer_Delete(&q1_sbuffer);						/* @5 */
    TPM_Sbuffer_Delete(&versionInfo_sbuffer);					/* @6 */
    return rcf;
}

/* TPM_ExtendCommon() rev 109

   Contains code common to TPM_Process_Extend() and TPM_Process_SHA1CompleteExtend().
   
   Add a measurement value to a PCR
*/

TPM_RESULT TPM_ExtendCommon(TPM_PCRVALUE outDigest,	/* The PCR value after execution of
							   the command */
			    tpm_state_t *tpm_state,
			    TPM_COMMAND_CODE ordinal,	/* command ordinal */
			    TPM_PCRINDEX pcrNum,	/* Index of the PCR to be modified */
			    TPM_DIGEST inDigest)	/* the event to be recorded */
{
    TPM_RESULT			rc = 0;
    TPM_PCRVALUE		currentPcrValue;
    TPM_DIGEST			h1;
    
    printf("TPM_ExtendCommon: pcrNum %u\n", pcrNum);
    /* 1. Validate that pcrNum represents a legal PCR number. On error, return TPM_BADINDEX. */
    if (rc == 0) {
	rc = TPM_PCR_CheckRange(pcrNum);
    }
    if (rc == 0) {
	/* 2. Map V1 to TPM_STANY_FLAGS */
	/* 3. Map L1 to V1 -> localityModifier */
	/* 4. If the current locality, held in L1, is not selected in TPM_PERMANENT_DATA ->
	   pcrAttrib[PCRIndex].pcrExtendLocal, return TPM_BAD_LOCALITY */
	rc = TPM_Locality_Check(tpm_state->tpm_permanent_data.pcrAttrib[pcrNum].pcrExtendLocal,
				tpm_state->tpm_stany_flags.localityModifier);
    }
    /* get the current PCR digest value */
    if (rc == 0) {
	rc = TPM_PCR_Load(currentPcrValue,
			  tpm_state->tpm_stclear_data.PCRS,
			  pcrNum);
    }
#if defined TPM_PCCLIENT
    /* From the PC Client TIS spec

       1. When the locality 4 PCR is at its reset value of 0, the entry for the locality 4 PCR in
          section 7.2 SHALL be interpreted as if the column labeled pcrExtendLocal for locality
          4,3,2,1,0 contains the bit field definitions: 1,0,0,0,0.

       2. Once the locality 4 PCR is no longer at its reset value of 0, table 4 in section 7.2
          applies as written.
    */
    if (rc == 0) {
	TPM_BOOL 			isZero;
	if ((pcrNum == 17) &&	/* PCR 17 is the Locality 4 PCR */
	    (tpm_state->tpm_stany_flags.localityModifier != 4)) {
	    /* if not locality 4, must not be at the reset value */
	    TPM_Digest_IsZero(&isZero, currentPcrValue);
	    if (isZero) {
		printf("TPM_ExtendCommon: Error, "
		       "pcrNum %u and locality %u and PCR at reset value\n",
		       pcrNum, tpm_state->tpm_stany_flags.localityModifier);
		rc = TPM_BAD_LOCALITY;
	    }
	}
    }
#endif
    /* 5. Create c1 by concatenating (PCRindex TPM_PCRVALUE || inDigest). This takes the current PCR
       value and concatenates the inDigest parameter. */
    /* NOTE: Not required, SHA1 uses varargs */
    /* 6. Create h1 by performing a SHA-1 digest of c1. */
    if (rc == 0) {
	TPM_PrintFour("TPM_ExtendCommon: Current PCR ", currentPcrValue);
	/* TPM_PrintFour("TPM_ExtendCommon: Current PCR ",
	   tpm_state->tpm_stclear_data.PCR[pcrNum]); */
	TPM_PrintFour("TPM_ExtendCommon: Input Digest", inDigest);
	rc = TPM_SHA1(h1,
		      TPM_DIGEST_SIZE, currentPcrValue,
		      TPM_DIGEST_SIZE, inDigest,
		      0, NULL);
    }
    if (rc == 0) {
	TPM_PrintFour("TPM_ExtendCommon: New PCR", h1);
	/* 7. Store h1 as the new TPM_PCRVALUE of PCRindex */
	rc = TPM_PCR_Store(tpm_state->tpm_stclear_data.PCRS,
			   pcrNum,
			   h1);
    }
    if (rc == 0) {
	/* 8. If TPM_PERMANENT_FLAGS -> disable is TRUE or TPM_STCLEAR_FLAGS -> deactivated is
	   TRUE */
	if ((tpm_state->tpm_permanent_flags.disable) ||
	    (tpm_state->tpm_stclear_flags.deactivated)) {
	    /* a. Set outDigest to 20 bytes of 0x00 */
	    TPM_Digest_Init(outDigest);
	}
	/* 9. Else */
	else {
	    /* a. Set outDigest to h1 */
	    TPM_Digest_Copy(outDigest, h1);
	}
    }
    if (rc == 0) {
	ordinal = ordinal;
    }
    return rc;
}

/* 16.1 TPM_Extend rev 109

   This adds a new measurement to a PCR.
*/

TPM_RESULT TPM_Process_Extend(tpm_state_t *tpm_state,
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
    TPM_PCRINDEX	pcrNum;		/* The PCR to be updated. */
    TPM_DIGEST		inDigest;	/* The 160 bit value representing the event to be
					   recorded. */

    /* processing parameters */
    unsigned char *	inParamStart;		/* starting point of inParam's */
    unsigned char *	inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_PCRVALUE	outDigest;	/* The PCR value after execution of the command. */

    printf("TPM_Process_Extend: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get pcrNum parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&pcrNum, &command, &paramSize);
    }
    /* get inDigest parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Digest_Load(inDigest, &command, &paramSize);
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
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_Extend: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* extend the resultant digest into a PCR */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_ExtendCommon(outDigest, tpm_state, ordinal, pcrNum, inDigest);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_Extend: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* append outDigest */
	    returnCode = TPM_Digest_Store(response, outDigest);
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
    return rcf;
}

/* 16.4 TPM_PCR_Reset rev 87
   
   For PCR with the pcrReset attribute set to TRUE, this command resets the PCR back to the default
   value, this mimics the actions of TPM_Init.	The PCR may have restrictions as to which locality
   can perform the reset operation.

   Sending a null pcrSelection results in an error is due to the requirement that the command
   actually do something. If pcrSelection is null there are no PCR to reset and the command would
   then do nothing.

   For PCR that are resettable, the presence of a Trusted Operating System (TOS) can change the
   behavior of TPM_PCR_Reset. The following pseudo code shows how the behavior changes

   At TPM_Startup 
     If TPM_PCR_ATTRIBUTES->pcrReset is FALSE
       Set PCR to 0x00...00
     Else
       Set PCR to 0xFF...FF

   At TPM_PCR_Reset
     If TPM_PCR_ATTRIBUTES->pcrReset is TRUE
       If TOSPresent
	 Set PCR to 0x00...00
       Else
	 Set PCR to 0xFF...FF
     Else
       Return error
       
   The above pseudocode is for example only, for the details of a specific platform, the reader must
   review the platform specific specification. The purpose of the above pseudocode is to show that
   both pcrReset and the TOSPresent bit control the value in use to when the PCR resets.
*/

TPM_RESULT TPM_Process_PcrReset(tpm_state_t *tpm_state,
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
    TPM_PCR_SELECTION	pcrSelection;		/* The PCR's to reset*/

    /* processing parameters */
    unsigned char *		inParamStart;	/* starting point of inParam's */
    unsigned char *		inParamEnd;	/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			pcrUsage;	/* TRUE if pcrSelection specifies one or more
						   PCR's */
    TPM_PERMANENT_DATA		*tpm_permanent_data = NULL;
    size_t			i;		/* PCR selection iterator */
    size_t			j;		/* PCR selection bit map in byte */
    TPM_PCRINDEX		pcr_num;	/* PCR iterator */
    TPM_MODIFIER_INDICATOR	localityModifier = 0; 
    uint16_t			sizeOfSelect = 0;	/* from pcrSelection input parameter */

    /* output parameters */
    uint16_t		outParamStart;	/* starting point of outParam's */
    uint16_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_PcrReset: Ordinal Entry\n");
    TPM_PCRSelection_Init(&pcrSelection);	/* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get pcrSelection */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_PCRSelection_Load(&pcrSelection, &command, &paramSize);
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
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_PcrReset: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Validate that pcrSelection is valid */
    /* a. is a valid TPM_PCR_SELECTION structure */
    /* NOTE: Done during TPM_PCRSelection_Load() */
    /* b. pcrSelection -> pcrSelect is non-zero */
    /* NOTE: TPM_PCRSelection_GetPCRUsage() range checks pcrSelection */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_PcrReset: Getting input PCR usage\n");
	returnCode = TPM_PCRSelection_GetPCRUsage(&pcrUsage, &pcrSelection, 0);
    }
    /* c. On errors return TPM_INVALID_PCR_INFO */
    if (returnCode == TPM_SUCCESS) {
	if (!pcrUsage) {
	    printf("TPM_Process_PcrReset: Error, pcrSelect is zero\n");
	    returnCode = TPM_INVALID_PCR_INFO;
	}
    }
    /* 2. Map L1 to TPM_STANY_FLAGS -> localityModifier (NOTE and other optimizations of the inner
	  loop) */
    if (returnCode == TPM_SUCCESS) {
	localityModifier = tpm_state->tpm_stany_flags.localityModifier;
	tpm_permanent_data = &(tpm_state->tpm_permanent_data);
	sizeOfSelect = pcrSelection.sizeOfSelect;	/* bytes of input PCR selection */
    }
    /* 3. For each PCR selected perform the following */
    for (i = 0, pcr_num = 0 ; (returnCode == TPM_SUCCESS) && (i < sizeOfSelect) ; i++) {
	/* iterate through all bits in each selection byte */
	for (j = 0x0001 ;
	     (returnCode == TPM_SUCCESS) && (j != (0x0001 << CHAR_BIT)) ;
	     j <<= 1, pcr_num++) {
	    
	    if (pcrSelection.pcrSelect[i] & j) {	/* if the bit is set in the selection map */
		/* a. If pcrAttrib[pcrIndex].pcrReset is FALSE */
		if (!(tpm_permanent_data->pcrAttrib[pcr_num].pcrReset)) {
		    printf("TPM_Process_PcrReset: Error, PCR %u not resettable\n", pcr_num);
		    /* a. Return TPM_NOTRESETABLE */
		    returnCode = TPM_NOTRESETABLE;
		}
		/* b.	If, for the value L1, the corresponding bit is clear in the bit map 
		   TPM_PERMANENT_DATA -> pcrAttrib[pcrIndex].pcrResetLocal, return 
		   TPM_NOTLOCAL */
		else {
		    returnCode =
			TPM_Locality_Check(tpm_permanent_data->pcrAttrib[pcr_num].pcrResetLocal,
					   localityModifier);
		    if (returnCode != TPM_SUCCESS) {
			printf("TPM_Process_PcrReset: Error, PCR %u bad pcrResetLocal %02x\n",
			       pcr_num, tpm_permanent_data->pcrAttrib[pcr_num].pcrResetLocal);
			returnCode = TPM_NOTLOCAL;
		    }
		}
		/* NOTE: No 'else reset' here.	The command MUST validate that all PCR registers
		   that are selected are available to be reset before resetting any PCR. */
	    }
	}
    }
    /* 3. For each PCR selected perform the following */
    if (returnCode == TPM_SUCCESS) {
	for (i = 0, pcr_num = 0 ; i < sizeOfSelect ; i++) {
	    /* iterate through all bits in each selection byte */
	    for (j = 0x0001 ; j != (0x0001 << CHAR_BIT) ; j <<= 1, pcr_num++) {
		if (pcrSelection.pcrSelect[i] & j) {	/* if the bit is set in the selection map */
		    printf("TPM_Process_PcrReset: Resetting PCR %u\n", pcr_num);
		    /* a. The PCR MAY only reset to 0x00...00 or 0xFF...FF */
		    /* b. The logic to determine which value to use MUST be described by a platform
		       specific specification
		    */
		    /* Ignore errors here since PCR selection has already been validated.  pcr_num
		       is guaranteed to be in range from from 'for' iterator, and pcrReset is
		       guaranteed to be TRUE from the previous loop. */
		    TPM_PCR_Reset(tpm_state->tpm_stclear_data.PCRS,
				  tpm_state->tpm_stany_flags.TOSPresent,
				  pcr_num);
		}
	    }
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_PcrReset: Ordinal returnCode %08x %u\n",
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
    TPM_PCRSelection_Delete(&pcrSelection);	/* @1 */
    return rcf;
}

