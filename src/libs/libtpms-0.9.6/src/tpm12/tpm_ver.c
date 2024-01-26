/********************************************************************************/
/*                                                                              */
/*                           Ver Structure Handler                              */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_ver.c $               */
/*                                                                              */
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

#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_structures.h"

#include "tpm_ver.h"

/*
  TPM_STRUCT_VER

  This indicates the version of the structure.
  
  Version 1.2 deprecates the use of this structure in all other structures. The structure is not
  deprecated as many of the structures that contain this structure are not deprecated.
  
  The rationale behind keeping this structure and adding the new version structure is that in
  version 1.1 this structure was in use for two purposes. The first was to indicate the structure
  version, and in that mode the revMajor and revMinor were supposed to be set to 0. The second use
  was in TPM_GetCapability and the structure would then return the correct revMajor and
  revMinor. This use model caused problems in keeping track of when the revs were or were not set
  and how software used the information. Version 1.2 went to structure tags. Some structures did not
  change and the TPM_STRUCT_VER is still in use. To avoid the problems from 1.1 this structure now
  is a fixed value and only remains for backwards compatibility. Structure versioning comes from the
  tag on the structure and the TPM_GetCapability response for TPM versioning uses TPM_VERSION.
*/

/* TPM_StructVer_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_StructVer_Init(TPM_STRUCT_VER *tpm_struct_ver)
{
    printf(" TPM_StructVer_Init:\n");
    tpm_struct_ver->major = 0x01;
    tpm_struct_ver->minor = 0x01;
    tpm_struct_ver->revMajor = 0x00;
    tpm_struct_ver->revMinor = 0x00;
    return;
}

/* TPM_StructVer_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
*/

TPM_RESULT TPM_StructVer_Load(TPM_STRUCT_VER *tpm_struct_ver,
                              unsigned char **stream,
                              uint32_t *stream_size)
{
    TPM_RESULT rc = 0;

    printf(" TPM_StructVer_Load:\n");
    if (rc == 0) {
        rc = TPM_Load8(&(tpm_struct_ver->major), stream, stream_size);
    }
    if (rc == 0) {
        rc = TPM_Load8(&(tpm_struct_ver->minor), stream, stream_size);
    }
    if (rc == 0) {
        rc = TPM_Load8(&(tpm_struct_ver->revMajor), stream, stream_size);
    }
    if (rc == 0) {
        rc = TPM_Load8(&(tpm_struct_ver->revMinor), stream, stream_size);
    }
    return rc;
}
    

/* TPM_StructVer_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_StructVer_Store(TPM_STORE_BUFFER *sbuffer,
                               const TPM_STRUCT_VER *tpm_struct_ver)
{
    TPM_RESULT rc = 0;

    printf(" TPM_StructVer_Store:\n");
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_struct_ver->major), sizeof(BYTE));               
    }
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_struct_ver->minor), sizeof(BYTE));               
    }
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_struct_ver->revMajor), sizeof(BYTE));            
    }
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_struct_ver->revMinor), sizeof(BYTE));            
    }
    return rc;
}

/* TPM_StructVer_Copy() copies the src to the destination. */

void TPM_StructVer_Copy(TPM_STRUCT_VER *tpm_struct_ver_dest,
                        TPM_STRUCT_VER *tpm_struct_ver_src)
{
    printf(" TPM_StructVer_Copy:\n");
    tpm_struct_ver_dest->major    = tpm_struct_ver_src->major;
    tpm_struct_ver_dest->minor    = tpm_struct_ver_src->minor;
    tpm_struct_ver_dest->revMajor = tpm_struct_ver_src->revMajor;
    tpm_struct_ver_dest->revMinor = tpm_struct_ver_src->revMinor;
    return;
}

/* TPM_StructVer_CheckVer() checks that the major and minor version are 0x01, 0x01 */
    
TPM_RESULT TPM_StructVer_CheckVer(TPM_STRUCT_VER *tpm_struct_ver)
{
    TPM_RESULT rc = 0;

    printf(" TPM_StructVer_CheckVer: version %u.%u.%u.%u\n",
           tpm_struct_ver->major,
           tpm_struct_ver->minor,
           tpm_struct_ver->revMajor,
           tpm_struct_ver->revMinor);
    if ((tpm_struct_ver->major != 0x01) ||
        (tpm_struct_ver->minor != 0x01)) {
        printf("TPM_StructVer_CheckVer: Error checking version\n");
        rc = TPM_BAD_VERSION;
    }
    return rc;
}

/*
  TPM_VERSION

  This structure provides information relative the version of the TPM. This structure should only be
  in use by TPM_GetCapability to provide the information relative to the TPM.
*/

void TPM_Version_Init(TPM_VERSION *tpm_version)
{
    printf(" TPM_Version_Init:\n");
    tpm_version->major = 0;
    tpm_version->minor = 0;
    tpm_version->revMajor = 0;
    tpm_version->revMinor = 0;
    return;
}

void TPM_Version_Set(TPM_VERSION *tpm_version,
                     TPM_PERMANENT_DATA *tpm_permanent_data)
{
    printf(" TPM_Version_Set:\n");
    /* This SHALL indicate the major version of the TPM, mostSigVer MUST be 0x01, leastSigVer MUST
       be 0x00 */
    tpm_version->major = TPM_MAJOR;
    /* This SHALL indicate the minor version of the TPM, mostSigVer MUST be 0x01 or 0x02,
       leastSigVer MUST be 0x00 */
    tpm_version->minor = TPM_MINOR;
    /* This SHALL be the value of the TPM_PERMANENT_DATA -> revMajor */
    tpm_version->revMajor = tpm_permanent_data->revMajor;
    /* This SHALL be the value of the TPM_PERMANENT_DATA -> revMinor */
    tpm_version->revMinor = tpm_permanent_data->revMinor;
    return;
}

#if 0
/* TPM_Version_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_Version_Init()
*/

TPM_RESULT TPM_Version_Load(TPM_VERSION *tpm_version,
                            unsigned char **stream,
                            uint32_t *stream_size)
{
    TPM_RESULT rc = 0;

    printf(" TPM_Version_Load:\n");
    /* load major */
    if (rc == 0) {
        rc = TPM_Load8(&(tpm_version->major), stream, stream_size);
    }
    /* load minor */
    if (rc == 0) {
        rc = TPM_Load8(&(tpm_version->minor), stream, stream_size);
    }                  
    /* load revMajor */
    if (rc == 0) {     
        rc = TPM_Load8(&(tpm_version->revMajor), stream, stream_size);
    }                  
    /* load revMinor */
    if (rc == 0) {     
        rc = TPM_Load8(&(tpm_version->revMinor), stream, stream_size);
    }
    return rc;
}
#endif
/* TPM_Version_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_Version_Store(TPM_STORE_BUFFER *sbuffer,
                             const TPM_VERSION *tpm_version)
     
{
    TPM_RESULT rc = 0;

    printf(" TPM_Version_Store:\n");
    /* store major */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_version->major), sizeof(BYTE));          
    }
    /* store minor */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_version->minor), sizeof(BYTE));          
    }
    /* store revMajor */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_version->revMajor), sizeof(BYTE));               
    }
    /* store revMinor */
    if (rc == 0) {
        rc = TPM_Sbuffer_Append(sbuffer, &(tpm_version->revMinor), sizeof(BYTE));               
    }
    return rc;
}
