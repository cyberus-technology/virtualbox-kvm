/********************************************************************************/
/*										*/
/*			     TPM Command Processor				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_process.c $		*/
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef TPM_POSIX
#include <sys/types.h>
#include <unistd.h>
#endif

#include "tpm_admin.h"
#include "tpm_audit.h"
#include "tpm_auth.h"
#include "tpm_constants.h"
#include "tpm_commands.h"
#include "tpm_counter.h"
#include "tpm_cryptoh.h"
#include "tpm_crypto.h"
#include "tpm_daa.h"
#include "tpm_debug.h"
#include "tpm_delegate.h"
#include "tpm_error.h"
#include "tpm_identity.h"
#include "tpm_init.h"
#include "tpm_io.h"
#include "tpm_key.h"
#include "tpm_maint.h"
#include "tpm_memory.h"
#include "tpm_migration.h"
#include "tpm_nonce.h"
#include "tpm_nvram.h"
#include "tpm_owner.h"
#include "tpm_pcr.h"
#include "tpm_permanent.h"
#include "tpm_platform.h"
#include "tpm_session.h"
#include "tpm_sizedbuffer.h"
#include "tpm_startup.h"
#include "tpm_storage.h"
#include "tpm_ticks.h"
#include "tpm_transport.h"
#include "tpm_ver.h"

#include "tpm_process.h"

/* local prototypes */

/* get capabilities */

static TPM_RESULT TPM_GetCapability_CapOrd(TPM_STORE_BUFFER *capabilityResponse,
					   uint32_t ordinal);
static TPM_RESULT TPM_GetCapability_CapAlg(TPM_STORE_BUFFER *capabilityResponse,
					   uint32_t algorithmID);
static TPM_RESULT TPM_GetCapability_CapPid(TPM_STORE_BUFFER *capabilityResponse,
					   uint16_t protocolID);
static TPM_RESULT TPM_GetCapability_CapFlag(TPM_STORE_BUFFER *capabilityResponse,
					    tpm_state_t *tpm_state,
					    uint32_t capFlag);
static TPM_RESULT TPM_GetCapability_CapProperty(TPM_STORE_BUFFER *capabilityResponse,
						tpm_state_t *tpm_state,
						uint32_t capProperty);
static TPM_RESULT TPM_GetCapability_CapVersion(TPM_STORE_BUFFER *capabilityResponse);
static TPM_RESULT TPM_GetCapability_CapCheckLoaded(TPM_STORE_BUFFER *capabilityResponse,
						   const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry,
						   TPM_SIZED_BUFFER *subCap);
static TPM_RESULT TPM_GetCapability_CapSymMode(TPM_STORE_BUFFER *capabilityResponse,
					       TPM_SYM_MODE symMode);
static TPM_RESULT TPM_GetCapability_CapKeyStatus(TPM_STORE_BUFFER *capabilityResponse,
						 TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
						 uint32_t tpm_key_handle);
static TPM_RESULT TPM_GetCapability_CapMfr(TPM_STORE_BUFFER *capabilityResponse,
					   tpm_state_t *tpm_state,
					   TPM_SIZED_BUFFER *subCap);
static TPM_RESULT TPM_GetCapability_CapNVIndex(TPM_STORE_BUFFER *capabilityResponse,
					       tpm_state_t *tpm_state,
					       uint32_t nvIndex);
static TPM_RESULT TPM_GetCapability_CapTransAlg(TPM_STORE_BUFFER *capabilityResponse,
						TPM_ALGORITHM_ID algorithmID);
static TPM_RESULT TPM_GetCapability_CapHandle(TPM_STORE_BUFFER *capabilityResponse,
					      tpm_state_t *tpm_state,
					      TPM_RESOURCE_TYPE resourceType);
static TPM_RESULT TPM_GetCapability_CapTransEs(TPM_STORE_BUFFER *capabilityResponse,
					       TPM_ENC_SCHEME encScheme);
static TPM_RESULT TPM_GetCapability_CapAuthEncrypt(TPM_STORE_BUFFER *capabilityResponse,
						   uint32_t algorithmID);
static TPM_RESULT TPM_GetCapability_CapSelectSize(TPM_STORE_BUFFER *capabilityResponse,
						  TPM_SIZED_BUFFER *subCap);
#if  (TPM_REVISION >= 103)	/* added for rev 103 */
static TPM_RESULT TPM_GetCapability_CapDaLogic(TPM_STORE_BUFFER *capabilityResponse,
					       TPM_SIZED_BUFFER *subCap,
					       tpm_state_t *tpm_state);
#endif
static TPM_RESULT TPM_GetCapability_CapVersionVal(TPM_STORE_BUFFER *capabilityResponse,
						  TPM_PERMANENT_DATA *tpm_permanent_data);

static TPM_RESULT TPM_GetCapability_CapPropTisTimeout(TPM_STORE_BUFFER *capabilityResponse);
static TPM_RESULT TPM_GetCapability_CapPropDuration(TPM_STORE_BUFFER *capabilityResponse);

/* set capabilities */

static TPM_RESULT TPM_SetCapability_CapPermFlags(tpm_state_t *tpm_state,
						 TPM_BOOL ownerAuthorized,
						 TPM_BOOL presenceAuthorized,
						 uint32_t subCap32,
						 TPM_BOOL valueBool);
static TPM_RESULT TPM_SetCapability_CapPermData(tpm_state_t *tpm_state,
						TPM_BOOL ownerAuthorized,
						TPM_BOOL presenceAuthorized,
						uint32_t subCap32,
						uint32_t valueUint32);
static TPM_RESULT TPM_SetCapability_CapStclearFlags(tpm_state_t *tpm_state,
						    TPM_BOOL ownerAuthorized,
						    TPM_BOOL presenceAuthorized,
						    uint32_t subCap32,
						    TPM_BOOL valueBool);
static TPM_RESULT TPM_SetCapability_CapStclearData(tpm_state_t *tpm_state,
						   TPM_BOOL ownerAuthorized,
						   TPM_BOOL presenceAuthorized,
						   uint32_t subCap32,
						   uint32_t valueUint32);
static TPM_RESULT TPM_SetCapability_CapStanyFlags(tpm_state_t *tpm_state,
						  TPM_BOOL ownerAuthorized,
						  TPM_BOOL presenceAuthorized,
						  uint32_t subCap32,
						  TPM_BOOL valueBool);
static TPM_RESULT TPM_SetCapability_CapStanyData(tpm_state_t *tpm_state,
						 TPM_BOOL ownerAuthorized,
						 TPM_BOOL presenceAuthorized,
						 uint32_t subCap32,
						 TPM_SIZED_BUFFER *setValue);
static TPM_RESULT TPM_SetCapability_CapVendor(tpm_state_t *tpm_state,
					      TPM_BOOL ownerAuthorized,
					      TPM_BOOL presenceAuthorized,
					      uint32_t subCap32,
					      TPM_SIZED_BUFFER *setValue);

/*
  TPM_CAP_VERSION_INFO
*/

/* TPM_CapVersionInfo_Init()

   sets members to default values
   sets all pointers to NULL and sizes to 0
   always succeeds - no return code
*/

void TPM_CapVersionInfo_Init(TPM_CAP_VERSION_INFO *tpm_cap_version_info)
{
    printf(" TPM_CapVersionInfo_Init:\n");
    TPM_Version_Init(&(tpm_cap_version_info->version));
    tpm_cap_version_info->specLevel = TPM_SPEC_LEVEL;
    tpm_cap_version_info->errataRev = TPM_ERRATA_REV;
    memcpy(&(tpm_cap_version_info->tpmVendorID), TPM_VENDOR_ID,
	   sizeof(tpm_cap_version_info->tpmVendorID));
    tpm_cap_version_info->vendorSpecificSize = 0;
    tpm_cap_version_info->vendorSpecific = NULL;
    return;
}

/* TPM_CapVersionInfo_Set() sets members to software specific data */

void TPM_CapVersionInfo_Set(TPM_CAP_VERSION_INFO *tpm_cap_version_info,
			    TPM_PERMANENT_DATA *tpm_permanent_data)
{
    printf(" TPM_CapVersionInfo_Set:\n");
    TPM_Version_Set(&(tpm_cap_version_info->version), tpm_permanent_data);
    tpm_cap_version_info->specLevel = TPM_SPEC_LEVEL;
    tpm_cap_version_info->errataRev = TPM_ERRATA_REV;
    memcpy(&(tpm_cap_version_info->tpmVendorID), TPM_VENDOR_ID,
	   sizeof(tpm_cap_version_info->tpmVendorID));
    tpm_cap_version_info->vendorSpecificSize = 0;
    tpm_cap_version_info->vendorSpecific = NULL;
    return;
}

#if 0	/* not required */
/* TPM_CapVersionInfo_Load()

   deserialize the structure from a 'stream'
   'stream_size' is checked for sufficient data
   returns 0 or error codes
   
   Before use, call TPM_CapVersionInfo_Init() or TPM_CapVersionInfo_Set()
   After use, call TPM_CapVersionInfo_Delete() to free memory
*/

TPM_RESULT TPM_CapVersionInfo_Load(TPM_CAP_VERSION_INFO *tpm_cap_version_info,
				   unsigned char **stream,
				   uint32_t *stream_size)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CapVersionInfo_Load:\n");
    /* check tag */
    if (rc == 0) {
	rc = TPM_CheckTag(TPM_TAG_CAP_VERSION_INFO, stream, stream_size);
    }
    /* load version */
    if (rc == 0) {
	rc = TPM_Version_Load(&(tpm_cap_version_info->version), stream, stream_size);
    }
    /* load specLevel */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_cap_version_info->specLevel), stream, stream_size);
    }
    /* load errataRev */
    if (rc == 0) {
	rc = TPM_Loadn(&(tpm_cap_version_info->errataRev), sizeof(tpm_cap_version_info->errataRev),
		       stream, stream_size);
    }
    /* load tpmVendorID */
    if (rc == 0) {
	rc = TPM_Loadn(tpm_cap_version_info->tpmVendorID, sizeof(tpm_cap_version_info->tpmVendorID),
		       stream, stream_size);
    }
    /* load vendorSpecificSize */
    if (rc == 0) {
	rc = TPM_Load16(&(tpm_cap_version_info->vendorSpecificSize), stream, stream_size);
    }
    /* allocate memory for vendorSpecific */
    if ((rc == 0) && (tpm_cap_version_info->vendorSpecificSize > 0)) {
	rc = TPM_Malloc(&(tpm_cap_version_info->vendorSpecific),
			tpm_cap_version_info->vendorSpecificSize);
    }
    /* load vendorSpecific */
    if ((rc == 0) && (tpm_cap_version_info->vendorSpecificSize > 0)) {
	rc = TPM_Loadn(tpm_cap_version_info->vendorSpecific,
		       tpm_cap_version_info->vendorSpecificSize,
		       stream, stream_size);
    }
    return rc;
}
#endif

/* TPM_CapVersionInfo_Store()
   
   serialize the structure to a stream contained in 'sbuffer'
   returns 0 or error codes
*/

TPM_RESULT TPM_CapVersionInfo_Store(TPM_STORE_BUFFER *sbuffer,
				    const TPM_CAP_VERSION_INFO *tpm_cap_version_info)
{
    TPM_RESULT		rc = 0;

    printf(" TPM_CapVersionInfo_Store:\n");
    /* store tag */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, TPM_TAG_CAP_VERSION_INFO); 
    }
    /* store version */
    if (rc == 0) {
	rc = TPM_Version_Store(sbuffer, &(tpm_cap_version_info->version));
    }
    /* store specLevel */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_cap_version_info->specLevel);
    }
    /* store errataRev */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, &(tpm_cap_version_info->errataRev),
				sizeof(tpm_cap_version_info->errataRev));
    }
    /* store tpmVendorID */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer, tpm_cap_version_info->tpmVendorID,
				sizeof(tpm_cap_version_info->tpmVendorID));
    }
    /* store vendorSpecificSize */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append16(sbuffer, tpm_cap_version_info->vendorSpecificSize);
    }
    /* store vendorSpecific */
    if (rc == 0) {
	rc = TPM_Sbuffer_Append(sbuffer,
				tpm_cap_version_info->vendorSpecific,
				tpm_cap_version_info->vendorSpecificSize);
    }
    return rc;
}

/* TPM_CapVersionInfo_Delete()

   No-OP if the parameter is NULL, else:
   frees memory allocated for the object
   sets pointers to NULL
   calls TPM_CapVersionInfo_Init to set members back to default values
   The object itself is not freed
*/   

void TPM_CapVersionInfo_Delete(TPM_CAP_VERSION_INFO *tpm_cap_version_info)
{
    printf(" TPM_CapVersionInfo_Delete:\n");
    if (tpm_cap_version_info != NULL) {
	free(tpm_cap_version_info->vendorSpecific);
	TPM_CapVersionInfo_Init(tpm_cap_version_info);
    }
    return;
}

/*
  Processing Commands
*/


/* 17. Ordinals rev 107

   This structure maps the specification Ordinals table to software functions and parameters.

   It provides direct mapping that easier to understand and maintain than scattering and hard coding
   these values.

   The functions currently supported are:

	- processing jump table for 1.1 and 1.2 (implied get capability - ordinals supported)
	- allow audit
	- audit default value
	- owner delegation permissions
	- key delegation permissions
	- wrappable

   Future possibilities include:

	- no owner, disabled, deactivated
	- 0,1,2 auth

   typedef struct tdTPM_ORDINAL_TABLE {

   TPM_COMMAND_CODE ordinal;
   tpm_process_function_t process_function_v11;
   tpm_process_function_t process_function_v12;
   TPM_BOOL auditable;				       
   TPM_BOOL auditDefault;			       
   uint16_t ownerPermissionBlock;			
   uint32_t ownerPermissionPosition;			
   uint16_t keyPermissionBlock;			
   uint32_t keyPermissionPosition;
   uint32_t inputHandleSize;
   uint32_t keyHandles;
   uint32_t outputHandleSize;
   TPM_BOOL transportWrappable;
   TPM_BOOL instanceWrappable;				
   TPM_BOOL hardwareWrappable;
   } TPM_ORDINAL_TABLE;
*/

static TPM_ORDINAL_TABLE tpm_ordinal_table[] =
{
    {TPM_ORD_ActivateIdentity,
     TPM_Process_ActivateIdentity, TPM_Process_ActivateIdentity,
     TRUE,
     TRUE,
     1, TPM_DELEGATE_ActivateIdentity,
     1, TPM_KEY_DELEGATE_ActivateIdentity,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_AuthorizeMigrationKey,
     TPM_Process_AuthorizeMigrationKey, TPM_Process_AuthorizeMigrationKey,
     TRUE,
     TRUE,
     1, TPM_DELEGATE_AuthorizeMigrationKey,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CertifyKey,
     TPM_Process_CertifyKey, TPM_Process_CertifyKey,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_CertifyKey,
     sizeof(TPM_KEY_HANDLE) + sizeof(TPM_KEY_HANDLE),
     2,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CertifyKey2,
     TPM_Process_Unused, TPM_Process_CertifyKey2,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_CertifyKey2,
     sizeof(TPM_KEY_HANDLE) + sizeof(TPM_KEY_HANDLE),
     2,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CertifySelfTest,
     TPM_Process_CertifySelfTest, TPM_Process_Unused,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ChangeAuth,
     TPM_Process_ChangeAuth, TPM_Process_ChangeAuth,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_ChangeAuth,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ChangeAuthAsymFinish,
     TPM_Process_ChangeAuthAsymFinish, TPM_Process_ChangeAuthAsymFinish,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_ChangeAuthAsymFinish,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ChangeAuthAsymStart,
     TPM_Process_ChangeAuthAsymStart, TPM_Process_ChangeAuthAsymStart,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_ChangeAuthAsymStart,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ChangeAuthOwner,
     TPM_Process_ChangeAuthOwner, TPM_Process_ChangeAuthOwner,
     TRUE,
     TRUE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CMK_ApproveMA,
     TPM_Process_Unused, TPM_Process_CMK_ApproveMA,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_CMK_ApproveMA,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CMK_ConvertMigration,
     TPM_Process_Unused, TPM_Process_CMK_ConvertMigration,
     TRUE,
     FALSE,
     1, TPM_KEY_DELEGATE_CMK_ConvertMigration,
     0, 0,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CMK_CreateBlob,
     TPM_Process_Unused, TPM_Process_CMK_CreateBlob,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_CMK_CreateBlob,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CMK_CreateKey,
     TPM_Process_Unused, TPM_Process_CMK_CreateKey,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_CMK_CreateKey,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CMK_CreateTicket,
     TPM_Process_Unused, TPM_Process_CMK_CreateTicket,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_CMK_CreateTicket,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CMK_SetRestrictions,
     TPM_Process_Unused, TPM_Process_CMK_SetRestrictions,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ContinueSelfTest,
     TPM_Process_ContinueSelfTest, TPM_Process_ContinueSelfTest,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ConvertMigrationBlob,
     TPM_Process_ConvertMigrationBlob, TPM_Process_ConvertMigrationBlob,
     TRUE,
     TRUE,
     0, 0,
     1, TPM_KEY_DELEGATE_ConvertMigrationBlob,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CreateCounter,
     TPM_Process_Unused, TPM_Process_CreateCounter,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_CreateCounter,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CreateEndorsementKeyPair,
     TPM_Process_CreateEndorsementKeyPair, TPM_Process_CreateEndorsementKeyPair,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     TRUE,
     FALSE},
    
    {TPM_ORD_CreateMaintenanceArchive,
#if defined(TPM_NOMAINTENANCE) || defined(TPM_NOMAINTENANCE_COMMANDS)
     TPM_Process_Unused, TPM_Process_Unused,
     FALSE,
     FALSE,
#else
     TPM_Process_CreateMaintenanceArchive, TPM_Process_CreateMaintenanceArchive,
     TRUE,
     TRUE,
#endif
     1, TPM_DELEGATE_CreateMaintenanceArchive,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CreateMigrationBlob,
     TPM_Process_CreateMigrationBlob, TPM_Process_CreateMigrationBlob,
     TRUE,
     TRUE,
     0, 0,
     1, TPM_KEY_DELEGATE_CreateMigrationBlob,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CreateRevocableEK,
     TPM_Process_Unused, TPM_Process_CreateRevocableEK,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_CreateWrapKey,
     TPM_Process_CreateWrapKey, TPM_Process_CreateWrapKey,
     TRUE,
     TRUE,
     0, 0,
     1, TPM_KEY_DELEGATE_CreateWrapKey,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_DAA_Join,
     TPM_Process_Unused, TPM_Process_DAAJoin,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_DAA_Join,
     0, 0,
     sizeof(TPM_HANDLE),
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_DAA_Sign,
     TPM_Process_Unused, TPM_Process_DAASign,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_DAA_Sign,
     0, 0,
     sizeof(TPM_HANDLE),
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Delegate_CreateKeyDelegation,
     TPM_Process_Unused, TPM_Process_DelegateCreateKeyDelegation,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_Delegate_CreateKeyDelegation,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Delegate_CreateOwnerDelegation,
     TPM_Process_Unused, TPM_Process_DelegateCreateOwnerDelegation,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_Delegate_CreateOwnerDelegation,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Delegate_LoadOwnerDelegation,
     TPM_Process_Unused, TPM_Process_DelegateLoadOwnerDelegation,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_Delegate_LoadOwnerDelegation,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Delegate_Manage,
     TPM_Process_Unused, TPM_Process_DelegateManage,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_Delegate_Manage,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Delegate_ReadTable,
     TPM_Process_Unused, TPM_Process_DelegateReadTable,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Delegate_UpdateVerification,
     TPM_Process_Unused, TPM_Process_DelegateUpdateVerification,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_Delegate_UpdateVerification,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Delegate_VerifyDelegation,
     TPM_Process_Unused, TPM_Process_DelegateVerifyDelegation,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_DirRead,
     TPM_Process_DirRead, TPM_Process_DirRead,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_DirWriteAuth,
     TPM_Process_DirWriteAuth, TPM_Process_DirWriteAuth,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_DirWriteAuth,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_DisableForceClear,
     TPM_Process_DisableForceClear, TPM_Process_DisableForceClear,
     TRUE,
     TRUE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_DisableOwnerClear,
     TPM_Process_DisableOwnerClear, TPM_Process_DisableOwnerClear,
     TRUE,
     TRUE,
     1, TPM_DELEGATE_DisableOwnerClear,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_DisablePubekRead,
     TPM_Process_DisablePubekRead, TPM_Process_DisablePubekRead,
     TRUE,
     TRUE,
     1, TPM_DELEGATE_DisablePubekRead,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_DSAP,
     TPM_Process_Unused, TPM_Process_DSAP,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     sizeof(TPM_ENTITY_TYPE) + sizeof(TPM_KEY_HANDLE) + TPM_NONCE_SIZE + sizeof(uint32_t),
     0xffffffff,
     sizeof(TPM_AUTHHANDLE) + TPM_NONCE_SIZE + TPM_NONCE_SIZE,
     TRUE,
     TRUE,
     TRUE},
    
    {TPM_ORD_EstablishTransport,
     TPM_Process_Unused, TPM_Process_EstablishTransport,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_EstablishTransport,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     FALSE,
     FALSE,
     FALSE},
    
    {TPM_ORD_EvictKey,
     TPM_Process_EvictKey, TPM_Process_EvictKey,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ExecuteTransport,
     TPM_Process_Unused, TPM_Process_ExecuteTransport,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     FALSE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Extend,
     TPM_Process_Extend, TPM_Process_Extend,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     TRUE,
     FALSE},
    
    {TPM_ORD_FieldUpgrade,
     TPM_Process_Unused, TPM_Process_Unused,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_FieldUpgrade,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_FlushSpecific,
     TPM_Process_Unused, TPM_Process_FlushSpecific,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     sizeof(TPM_HANDLE),
     0xffffffff,
     0,
     TRUE,
     TRUE,
     TRUE},
    
    {TPM_ORD_ForceClear,
     TPM_Process_ForceClear, TPM_Process_ForceClear,
     TRUE,
     TRUE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_GetAuditDigest,
     TPM_Process_Unused, TPM_Process_GetAuditDigest,
     FALSE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_GetAuditDigestSigned,
     TPM_Process_Unused, TPM_Process_GetAuditDigestSigned,
     FALSE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_GetAuditDigestSigned,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_GetAuditEvent,
     TPM_Process_Unused, TPM_Process_Unused,
     FALSE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_GetAuditEventSigned,
     TPM_Process_Unused, TPM_Process_Unused,
     FALSE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_GetCapability,
     TPM_Process_GetCapability, TPM_Process_GetCapability,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     TRUE,
     FALSE},
    
    {TPM_ORD_GetCapabilityOwner,
     TPM_Process_GetCapabilityOwner, TPM_Process_GetCapabilityOwner,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_GetCapabilitySigned,
     TPM_Process_GetCapabilitySigned, TPM_Process_Unused,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_GetOrdinalAuditStatus,
     TPM_Process_Unused, TPM_Process_Unused,
     FALSE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_GetPubKey,
     TPM_Process_GetPubKey, TPM_Process_GetPubKey,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_GetPubKey,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_GetRandom,
     TPM_Process_GetRandom, TPM_Process_GetRandom,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_GetTestResult,
     TPM_Process_GetTestResult, TPM_Process_GetTestResult,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_GetTicks,
     TPM_Process_Unused, TPM_Process_GetTicks,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_IncrementCounter,
     TPM_Process_Unused, TPM_Process_IncrementCounter,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Init,
     TPM_Process_Init, TPM_Process_Init,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_KeyControlOwner,
     TPM_Process_Unused, TPM_Process_KeyControlOwner,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_KeyControlOwner,
     0, 0,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_KillMaintenanceFeature,
#if defined(TPM_NOMAINTENANCE) || defined(TPM_NOMAINTENANCE_COMMANDS)
     TPM_Process_Unused, TPM_Process_Unused,
     FALSE,
     FALSE,
#else
     TPM_Process_KillMaintenanceFeature, TPM_Process_KillMaintenanceFeature,
     TRUE,
     TRUE,
#endif
     1, TPM_DELEGATE_KillMaintenanceFeature,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_LoadAuthContext,
     TPM_Process_LoadAuthContext, TPM_Process_LoadAuthContext,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     sizeof(TPM_HANDLE),
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_LoadContext,
     TPM_Process_Unused, TPM_Process_LoadContext,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     sizeof(TPM_HANDLE),
     0,
     sizeof(TPM_HANDLE),
     TRUE,
     TRUE,
     FALSE},
    
    {TPM_ORD_LoadKey,
     TPM_Process_LoadKey, TPM_Process_LoadKey,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_LoadKey,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_LoadKey2,
     TPM_Process_Unused, TPM_Process_LoadKey2,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_LoadKey2,
     sizeof(TPM_KEY_HANDLE),
     1,
     sizeof(TPM_KEY_HANDLE),
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_LoadKeyContext,
     TPM_Process_LoadKeyContext, TPM_Process_LoadKeyContext,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     sizeof(TPM_KEY_HANDLE),
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_LoadMaintenanceArchive,
#if defined(TPM_NOMAINTENANCE) || defined(TPM_NOMAINTENANCE_COMMANDS)
     TPM_Process_Unused, TPM_Process_Unused,
     FALSE,
     FALSE,
#else
     TPM_Process_LoadMaintenanceArchive, TPM_Process_LoadMaintenanceArchive,
     TRUE,
     TRUE,
#endif
     1, TPM_DELEGATE_LoadMaintenanceArchive,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_LoadManuMaintPub,
#if defined(TPM_NOMAINTENANCE) || defined(TPM_NOMAINTENANCE_COMMANDS)
     TPM_Process_Unused, TPM_Process_Unused,
     FALSE,
     FALSE,
#else
     TPM_Process_LoadManuMaintPub, TPM_Process_LoadManuMaintPub,
     TRUE,
     TRUE,
#endif
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_MakeIdentity,
     TPM_Process_MakeIdentity, TPM_Process_MakeIdentity,
     TRUE,
     TRUE,
     1, TPM_DELEGATE_MakeIdentity,
     1, TPM_KEY_DELEGATE_MakeIdentity,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_MigrateKey,
     TPM_Process_Unused, TPM_Process_MigrateKey,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_MigrateKey,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_NV_DefineSpace,
     TPM_Process_Unused, TPM_Process_NVDefineSpace,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_NV_DefineSpace,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_NV_ReadValue,
     TPM_Process_Unused, TPM_Process_NVReadValue,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_NV_ReadValue,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_NV_ReadValueAuth,
     TPM_Process_Unused, TPM_Process_NVReadValueAuth,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_NV_WriteValue,
     TPM_Process_Unused, TPM_Process_NVWriteValue,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_NV_WriteValue,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_NV_WriteValueAuth,
     TPM_Process_Unused, TPM_Process_NVWriteValueAuth,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_OIAP,
     TPM_Process_OIAP, TPM_Process_OIAP,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     sizeof(TPM_AUTHHANDLE) + TPM_NONCE_SIZE,
     TRUE,
     TRUE,
     TRUE},
    
    {TPM_ORD_OSAP,
     TPM_Process_OSAP, TPM_Process_OSAP,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     sizeof(TPM_ENTITY_TYPE) + sizeof(uint32_t) + TPM_NONCE_SIZE,
     0,		/* TPM_OSAP: no input or output parameters are encrypted or logged */
     sizeof(TPM_AUTHHANDLE) + TPM_NONCE_SIZE + TPM_NONCE_SIZE,
     TRUE,
     TRUE,
     TRUE},
    
    {TPM_ORD_OwnerClear,
     TPM_Process_OwnerClear, TPM_Process_OwnerClear,
     TRUE,
     TRUE,
     1, TPM_DELEGATE_OwnerClear,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_OwnerReadInternalPub,
     TPM_Process_Unused, TPM_Process_OwnerReadInternalPub,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_OwnerReadInternalPub,
     0, 0,
     0,
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_OwnerReadPubek,
     TPM_Process_OwnerReadPubek, TPM_Process_OwnerReadPubek,
     TRUE,
     TRUE,
     1, TPM_DELEGATE_OwnerReadPubek,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_OwnerSetDisable,
     TPM_Process_OwnerSetDisable, TPM_Process_OwnerSetDisable,
     TRUE,
     TRUE,
     1, TPM_DELEGATE_OwnerSetDisable,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_PCR_Reset,
     TPM_Process_Unused, TPM_Process_PcrReset,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_PcrRead,
     TPM_Process_PcrRead, TPM_Process_PcrRead,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_PhysicalDisable,
     TPM_Process_PhysicalDisable, TPM_Process_PhysicalDisable,
     TRUE,
     TRUE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_PhysicalEnable,
     TPM_Process_PhysicalEnable, TPM_Process_PhysicalEnable,
     TRUE,
     TRUE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     TRUE,
     FALSE},
    
    {TPM_ORD_PhysicalSetDeactivated,
     TPM_Process_PhysicalSetDeactivated, TPM_Process_PhysicalSetDeactivated,
     TRUE,
     TRUE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     TRUE,
     FALSE},
    
    {TPM_ORD_Quote,
     TPM_Process_Quote, TPM_Process_Quote,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_Quote,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     TRUE},
    
    {TPM_ORD_Quote2,
     TPM_Process_Unused, TPM_Process_Quote2,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_Quote2,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     TRUE},
    
    {TPM_ORD_ReadCounter,
     TPM_Process_Unused, TPM_Process_ReadCounter,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ReadManuMaintPub,
#if defined(TPM_NOMAINTENANCE) || defined(TPM_NOMAINTENANCE_COMMANDS)
     TPM_Process_Unused, TPM_Process_Unused,
     FALSE,
     FALSE,
#else
     TPM_Process_ReadManuMaintPub, TPM_Process_ReadManuMaintPub,
     TRUE,
     TRUE,
#endif
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ReadPubek,
     TPM_Process_ReadPubek, TPM_Process_ReadPubek,
     TRUE,
     TRUE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ReleaseCounter,
     TPM_Process_Unused, TPM_Process_ReleaseCounter,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ReleaseCounterOwner,
     TPM_Process_Unused, TPM_Process_ReleaseCounterOwner,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_ReleaseCounterOwner,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ReleaseTransportSigned,
     TPM_Process_Unused, TPM_Process_ReleaseTransportSigned,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_ReleaseTransportSigned,
     0,
     0,
     0,
     FALSE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Reset,
     TPM_Process_Reset, TPM_Process_Reset,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_ResetLockValue,
     TPM_Process_Unused, TPM_Process_ResetLockValue,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_ResetLockValue,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_RevokeTrust,
     TPM_Process_Unused, TPM_Process_RevokeTrust,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SaveAuthContext,
     TPM_Process_SaveAuthContext, TPM_Process_SaveAuthContext,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     sizeof(TPM_AUTHHANDLE),
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SaveContext,
     TPM_Process_Unused, TPM_Process_SaveContext,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     sizeof(TPM_HANDLE),
     0xffffffff,
     0,
     TRUE,
     TRUE,
     FALSE},
    
    {TPM_ORD_SaveKeyContext,
     TPM_Process_SaveKeyContext, TPM_Process_SaveKeyContext,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SaveState,
     TPM_Process_SaveState, TPM_Process_SaveState,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     TRUE,
     FALSE},
    
    {TPM_ORD_Seal,
     TPM_Process_Seal, TPM_Process_Seal,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_Seal,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Sealx,
     TPM_Process_Unused, TPM_Process_Sealx,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_Sealx,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SelfTestFull,
     TPM_Process_SelfTestFull, TPM_Process_SelfTestFull,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SetCapability,
     TPM_Process_Unused, TPM_Process_SetCapability,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_SetCapability,
     0, 0,
     0,
     0,
     0,
     TRUE,
     TRUE,
     FALSE},
    
    {TPM_ORD_SetOperatorAuth,
     TPM_Process_Unused, TPM_Process_SetOperatorAuth,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SetOrdinalAuditStatus,
     TPM_Process_SetOrdinalAuditStatus, TPM_Process_SetOrdinalAuditStatus,
     TRUE,
     TRUE,
     1, TPM_DELEGATE_SetOrdinalAuditStatus,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SetOwnerInstall,
     TPM_Process_SetOwnerInstall, TPM_Process_SetOwnerInstall,
     TRUE,
     TRUE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SetOwnerPointer,
     TPM_Process_Unused, TPM_Process_SetOwnerPointer,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SetRedirection,
     TPM_Process_Unused, TPM_Process_Unused,
     TRUE,
     FALSE,
     1, TPM_DELEGATE_SetRedirection,
     0, 0,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SetTempDeactivated,
     TPM_Process_SetTempDeactivated, TPM_Process_SetTempDeactivated,
     TRUE,
     TRUE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SHA1Complete,
     TPM_Process_SHA1Complete, TPM_Process_SHA1Complete,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SHA1CompleteExtend,
     TPM_Process_SHA1CompleteExtend, TPM_Process_SHA1CompleteExtend,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SHA1Start,
     TPM_Process_SHA1Start, TPM_Process_SHA1Start,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_SHA1Update,
     TPM_Process_SHA1Update, TPM_Process_SHA1Update,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Sign,
     TPM_Process_Sign, TPM_Process_Sign,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_Sign,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Startup,
     TPM_Process_Startup, TPM_Process_Startup,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     TRUE,
     FALSE},
    
    {TPM_ORD_StirRandom,
     TPM_Process_StirRandom, TPM_Process_StirRandom,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_TakeOwnership,
     TPM_Process_TakeOwnership, TPM_Process_TakeOwnership,
     TRUE,
     TRUE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Terminate_Handle,
     TPM_Process_TerminateHandle, TPM_Process_TerminateHandle,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     sizeof(TPM_AUTHHANDLE),
     0,
     0,
     TRUE,
     TRUE,
     TRUE},
    
    {TPM_ORD_TickStampBlob,
     TPM_Process_Unused, TPM_Process_TickStampBlob,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_TickStampBlob,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_UnBind,
     TPM_Process_UnBind, TPM_Process_UnBind,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_UnBind,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TPM_ORD_Unseal,
     TPM_Process_Unseal, TPM_Process_Unseal,
     TRUE,
     FALSE,
     0, 0,
     1, TPM_KEY_DELEGATE_Unseal,
     sizeof(TPM_KEY_HANDLE),
     1,
     0,
     TRUE,
     FALSE,
     FALSE},
    
    {TSC_ORD_PhysicalPresence,
     TPM_Process_PhysicalPresence, TPM_Process_PhysicalPresence,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     TRUE,
     FALSE},
    
    {TSC_ORD_ResetEstablishmentBit,
     TPM_Process_Unused, TPM_Process_ResetEstablishmentBit,
     TRUE,
     FALSE,
     0, 0,
     0, 0,
     0,
     0,
     0,
     TRUE,
     FALSE,
     FALSE}
    

    
};

/* 
   Ordinal Table Utilities
*/

/* TPM_OrdinalTable_GetEntry() gets the table entry for the ordinal.

   If the ordinal is not in the table, TPM_BAD_ORDINAL is returned
*/

TPM_RESULT TPM_OrdinalTable_GetEntry(TPM_ORDINAL_TABLE **entry,
				     TPM_ORDINAL_TABLE *ordinalTable,
				     TPM_COMMAND_CODE ordinal)
{
    TPM_RESULT	rc = TPM_BAD_ORDINAL;
    size_t	i;

    /* printf(" TPM_OrdinalTable_GetEntry: Ordinal %08x\n", ordinal); */
    *entry = NULL;
    for (i = 0 ; i < (sizeof(tpm_ordinal_table)/sizeof(TPM_ORDINAL_TABLE)) ; i++) {
	if (ordinalTable[i].ordinal == ordinal) {	/* if found */
	    *entry = &(ordinalTable[i]);		/* return the entry */
	    rc = 0;					/* return found */
	    break;
	}
    }
    return rc;
}

/* TPM_OrdinalTable_GetProcessFunction() returns the processing function for the ordinal.

   If the ordinal is not in the table, the function TPM_Process_Unused() is returned.
*/

void TPM_OrdinalTable_GetProcessFunction(tpm_process_function_t *tpm_process_function,
					 TPM_ORDINAL_TABLE *ordinalTable,
					 TPM_COMMAND_CODE ordinal)
{
    TPM_RESULT	rc = 0;
    TPM_ORDINAL_TABLE *entry;
    
    printf(" TPM_OrdinalTable_GetProcessFunction: Ordinal %08x\n", ordinal);

    if (rc == 0) {
	rc = TPM_OrdinalTable_GetEntry(&entry, ordinalTable, ordinal);
    }
    if (rc == 0) {	/* if found */
#ifdef TPM_V12
	*tpm_process_function = entry->process_function_v12;
#else
	*tpm_process_function = entry->process_function_v11;
#endif
    }
    else {	/* if not found, default processing function */
	*tpm_process_function = TPM_Process_Unused;
    }
    return;
}

/* TPM_OrdinalTable_GetAuditable() determines whether the ordinal can ever be audited.

   Used by TPM_Process_SetOrdinalAuditStatus()
*/

void TPM_OrdinalTable_GetAuditable(TPM_BOOL *auditable,
				   TPM_COMMAND_CODE ordinal)
{
    TPM_RESULT	rc = 0;
    TPM_ORDINAL_TABLE *entry;
    
    printf(" TPM_OrdinalTable_GetAuditable: Ordinal %08x\n", ordinal);
    if (rc == 0) {
	rc = TPM_OrdinalTable_GetEntry(&entry, tpm_ordinal_table, ordinal);
    }
    /* if not found, unimplemented, not auditable */
    if (rc != 0) {
	*auditable = FALSE;
    }
    /* if unimplemented, not auditable */
#ifdef TPM_V12
    else if (entry->process_function_v12 == TPM_Process_Unused) {
	*auditable = FALSE;
    }
#else
    else if (entry->process_function_v11 == TPM_Process_Unused) {
	*auditable = FALSE;
    }
#endif
    /* if found an entry, use it */
    else {
	*auditable = entry->auditable;
    }
    return;
}

/* TPM_OrdinalTable_GetAuditDefault() determines whether the ordinal is audited by default.

   Used to initialize TPM_PERMANENT_DATA -> ordinalAuditStatus

   Returns FALSE if the ordinal is not in the ordinals table.
*/

void TPM_OrdinalTable_GetAuditDefault(TPM_BOOL *auditDefault,
				      TPM_COMMAND_CODE ordinal)
{
    TPM_RESULT rc = 0;
    TPM_ORDINAL_TABLE *entry;

    if (rc == 0) {
	rc = TPM_OrdinalTable_GetEntry(&entry, tpm_ordinal_table, ordinal);
    }
    /* if not found, unimplemented, not auditable */
    if (rc != 0) {
	*auditDefault = FALSE;
    }
    /* found an entry, return it */
    else {
	*auditDefault = entry->auditDefault;
    }
    return;
}


/* TPM_OrdinalTable_GetOwnerPermission() gets the owner permission block and the position within the
   block for a permission bit based on the ordinal
*/

TPM_RESULT TPM_OrdinalTable_GetOwnerPermission(uint16_t *ownerPermissionBlock,
					       uint32_t *ownerPermissionPosition,
					       TPM_COMMAND_CODE ordinal)
{
    TPM_RESULT rc = 0;
    TPM_ORDINAL_TABLE *entry;

    if (rc == 0) {
	rc = TPM_OrdinalTable_GetEntry(&entry, tpm_ordinal_table, ordinal);
    }
    if (rc == 0) {
	*ownerPermissionBlock = entry->ownerPermissionBlock;
	*ownerPermissionPosition = entry->ownerPermissionPosition;
	/* sanity check ordinal table entry value */
	if (*ownerPermissionPosition >= (sizeof(uint32_t) * CHAR_BIT)) {
	    printf("TPM_OrdinalTable_GetOwnerPermission: Error (fatal): "
		   "ownerPermissionPosition out of range %u\n", *ownerPermissionPosition);
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    return rc;
}

/* TPM_OrdinalTable_GetKeyPermission() gets the key permission block and the position within the
   block for a permission bit based on the ordinal
*/

TPM_RESULT TPM_OrdinalTable_GetKeyPermission(uint16_t *keyPermissionBlock,
					     uint32_t *keyPermissionPosition,
					     TPM_COMMAND_CODE ordinal)
{	
    TPM_RESULT rc = 0;
    TPM_ORDINAL_TABLE *entry;

    if (rc == 0) {
	rc = TPM_OrdinalTable_GetEntry(&entry, tpm_ordinal_table, ordinal);
    }
    if (rc == 0) {
	*keyPermissionBlock = entry->keyPermissionBlock;
	*keyPermissionPosition = entry->keyPermissionPosition;
	if (*keyPermissionPosition >= (sizeof(uint32_t) * CHAR_BIT)) {
	    printf("TPM_OrdinalTable_GetKeyPermission: Error (fatal): "
		   "keyPermissionPosition out of range %u\n", *keyPermissionPosition);
	    rc = TPM_FAIL;	/* should never occur */
	}
    }
    return rc;
}

/* TPM_OrdinalTable_ParseWrappedCmd() parses a transport wrapped command, extracting

	- index into DATAw
	- length of DATAw
	- number of key handles and their indexes
	- ordinal
	- transportWrappable FALSE if the command cannot be wrapped in a transport session

   FIXME if audit has to occur before command parsing, this command becomes more generally useful,
   and might do the auditing and return the inParamDigest as well.

   This function cannot get the actual key handle(s) because the value may be encrypted, and the
   decryption has not occurred yet.
*/

TPM_RESULT TPM_OrdinalTable_ParseWrappedCmd(uint32_t *datawStart,
					    uint32_t *datawLen,
					    uint32_t *keyHandles,
					    uint32_t *keyHandle1Index,
					    uint32_t *keyHandle2Index,
					    TPM_COMMAND_CODE *ordinal,
					    TPM_BOOL *transportWrappable,
					    TPM_SIZED_BUFFER *wrappedCmd)
{
    TPM_RESULT		rc = 0;
    uint32_t		stream_size;
    unsigned char	*stream;
    TPM_TAG		tag = 0;
    uint32_t		paramSize = 0;
    TPM_ORDINAL_TABLE	*entry;		/* table entry for the ordinal */
    uint32_t		authLen;	/* length of below the line parameters */

    printf(" TPM_OrdinalTable_ParseWrappedCmd:\n");
    /* Extract the standard command parameters from the command stream.	 This also validates
       paramSize against wrappedCmdSize */
    if (rc == 0) {
	/* make temporary copies so the wrappedCmd is not touched */
	/* FIXME might want to return paramSize and tag and move the wrappedCmd pointers */
	stream = wrappedCmd->buffer;
	stream_size = wrappedCmd->size;
	/* parse the three standard input parameters, check paramSize against wrappedCmd->size */
	rc = TPM_Process_GetCommandParams(&tag, &paramSize, ordinal,
					  &stream, &stream_size);
    }
    /* get the entry from the ordinal table */
    if (rc == 0) {
	printf("  TPM_OrdinalTable_ParseWrappedCmd: ordinal %08x\n", *ordinal);
	rc = TPM_OrdinalTable_GetEntry(&entry, tpm_ordinal_table, *ordinal);
    }
    if (rc == 0) {
	/* datawStart indexes into the dataW area, skip the standard 3 inputs and the handles */
	*datawStart = sizeof(TPM_TAG) + sizeof(uint32_t) + sizeof(TPM_COMMAND_CODE) +
		      entry->inputHandleSize;
	/* authLen is the length of the below-the-line auth parameters that are excluded from the
	   dataW area */
	switch (tag) {
	  case TPM_TAG_RQU_AUTH1_COMMAND:
	    authLen = sizeof(TPM_AUTHHANDLE) + TPM_NONCE_SIZE +
		      sizeof(TPM_BOOL) + TPM_AUTHDATA_SIZE;
	    break;
	  case TPM_TAG_RQU_AUTH2_COMMAND:
	    authLen = 2 *
		      (sizeof(TPM_AUTHHANDLE) + TPM_NONCE_SIZE +
		       sizeof(TPM_BOOL) + TPM_AUTHDATA_SIZE);
	    break;
	  case TPM_TAG_RQU_COMMAND:
	    /* if the tag is illegal, assume the dataW area goes to the end of the command */
	  default:
	    authLen = 0;
	    break;
	}
	if (paramSize < *datawStart + authLen) {
	    printf("TPM_OrdinalTable_ParseWrappedCmd: Error, "
		   "paramSize %u less than datawStart %u + authLen %u\n",
		   paramSize, *datawStart, authLen);
	    rc = TPM_BAD_PARAM_SIZE;
	}
    }
    if (rc == 0) {
	/* subtract safe, cannot be negative after above check */
	*datawLen = paramSize - *datawStart - authLen;
	printf("  TPM_OrdinalTable_ParseWrappedCmd: datawStart %u datawLen %u\n",
	       *datawStart, *datawLen);
	/* determine whether the command can be wrapped in a transport session */
	*transportWrappable = entry->transportWrappable;
	/* return the number of key handles */
	*keyHandles = entry->keyHandles;
    }
    if (rc == 0) {
	printf("  TPM_OrdinalTable_ParseWrappedCmd: key handles %u\n", *keyHandles);
	switch (*keyHandles) {
	  case 0:
	    /* no key handles */
	    break;
	  case 1:
	    /* one key handle */
	    *keyHandle1Index = sizeof(TPM_TAG) + sizeof(uint32_t) + sizeof(TPM_COMMAND_CODE);
	    break;
	  case 2: 
	    /* first key handle */
	    *keyHandle1Index = sizeof(TPM_TAG) + sizeof(uint32_t) + sizeof(TPM_COMMAND_CODE);
	    /* second key handle */
	    *keyHandle2Index = sizeof(TPM_TAG) + sizeof(uint32_t) + sizeof(TPM_COMMAND_CODE) +
			       sizeof(TPM_KEY_HANDLE);
	    break;
	  case 0xffffffff:
	    printf("  TPM_OrdinalTable_ParseWrappedCmd: key handles special case\n");
	    /* potential key handle */
	    *keyHandle1Index = sizeof(TPM_TAG) + sizeof(uint32_t) + sizeof(TPM_COMMAND_CODE);
	    /* can't determine handle type here since resourceType is encrypted */
	    break;
	  default:
	    /* sanity check ordinal table */
	    printf("TPM_OrdinalTable_ParseWrappedCmd: Error (fatal), "
		   "invalid key handles for %08x for ordinal %08x\n", *keyHandles, *ordinal);
	    rc = TPM_FAIL;	/* should never occur */
	    break;
	}
    }
    return rc;
}

/* TPM_OrdinalTable_ParseWrappedRsp() parses a transport wrapped response, extracting

   - index into DATAw
   - length of DATAw
   - return code RCw

   FIXME this command might do the auditing and return the outParamDigest as well.
*/

TPM_RESULT TPM_OrdinalTable_ParseWrappedRsp(uint32_t *datawStart,
					    uint32_t *datawLen,
					    TPM_RESULT *rcw,
					    TPM_COMMAND_CODE ordinal,
					    const unsigned char *wrappedRspStream,
					    uint32_t wrappedRspStreamSize)
{
    TPM_RESULT		rc = 0;
    TPM_TAG		tag = 0;
    uint32_t		paramSize = 0;
    TPM_ORDINAL_TABLE	*entry;		/* table entry for the ordinal */
    uint32_t		authLen;	/* length of below the line parameters */

    printf(" TPM_OrdinalTable_ParseWrappedRsp: ordinal %08x\n", ordinal);
    /* Extract the standard response parameters from the response stream.  This also validates
       paramSize against wrappedRspSize */
    if (rc == 0) {
	rc = TPM_Process_GetResponseParams(&tag, &paramSize, rcw,
					   (unsigned char **)&wrappedRspStream,
					   &wrappedRspStreamSize);
    }
    /* get the entry from the ordinal table */
    if (rc == 0) {
	printf(" TPM_OrdinalTable_ParseWrappedRsp: returnCode %08x\n", *rcw);
	rc = TPM_OrdinalTable_GetEntry(&entry, tpm_ordinal_table, ordinal);
    }
    /* parse the success return code case */
    if ((rc == 0) && (*rcw == TPM_SUCCESS)) {
	if (rc == 0) {
	    /* datawStart indexes into the dataW area, skip the standard 3 inputs and the handles */
	    *datawStart = sizeof(TPM_TAG) + sizeof(uint32_t) + sizeof(TPM_RESULT) +
			  entry->outputHandleSize;
	    /* authLen is the length of the below-the-line auth parameters that are excluded from
	       the dataW area */
	    switch (tag) {
	      case TPM_TAG_RSP_AUTH1_COMMAND:
		authLen = TPM_NONCE_SIZE + sizeof(TPM_BOOL) + TPM_AUTHDATA_SIZE;
		break;
	      case TPM_TAG_RSP_AUTH2_COMMAND:
		authLen = 2 * (TPM_NONCE_SIZE + sizeof(TPM_BOOL) + TPM_AUTHDATA_SIZE);
		break;
	      case TPM_TAG_RSP_COMMAND:
		/* if the tag is illegal, assume the dataW area goes to the end of the response */
	      default:
		authLen = 0;
		break;
	    }
	    if (paramSize < *datawStart + authLen) {
		printf("TPM_OrdinalTable_ParseWrappedRsp: Error, "
		       "paramSize %u less than datawStart %u + authLen %u\n",
		       paramSize, *datawStart, authLen);
		rc = TPM_BAD_PARAM_SIZE;	/* FIXME not clear what to do here */
	    }
	}
	if (rc == 0) {
	    /* subtract safe, cannot be negative after about check */
	    *datawLen = paramSize - *datawStart - authLen;
	    printf("  TPM_OrdinalTable_ParseWrappedRsp: datawStart %u datawLen %u\n",
		   *datawStart, *datawLen);
	}
    }
    /* if the wrapped command failed, datawStart is not used, and datawLen is 0 */
    else if ((rc == 0) && (*rcw != TPM_SUCCESS)) {
	*datawStart = sizeof(TPM_TAG) + sizeof(uint32_t) + sizeof(TPM_RESULT);
	*datawLen = 0;
	printf("  TPM_OrdinalTable_ParseWrappedRsp: datawLen %u\n", *datawLen);
    }
    return rc;
}

void TPM_KeyHandleEntries_Trace(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries);

void TPM_KeyHandleEntries_Trace(TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries)
{
    size_t i;
    for (i = 0 ; (i < 4) && (i < TPM_KEY_HANDLES) ; i++) {
	printf("TPM_KeyHandleEntries_Trace: %lu handle %08x tpm_key %p\n",
	       (unsigned long)i, tpm_key_handle_entries[i].handle, tpm_key_handle_entries[i].key);
    }
    return;
}

void TPM_State_Trace(tpm_state_t *tpm_state);

void TPM_State_Trace(tpm_state_t *tpm_state)
{
    printf("TPM_State_Trace: disable %u p_deactive %u v_deactive %u owned %u state %u\n",
	   tpm_state->tpm_permanent_flags.disable,
	   tpm_state->tpm_permanent_flags.deactivated,
	   tpm_state->tpm_stclear_flags.deactivated,
	   tpm_state->tpm_permanent_data.ownerInstalled,
	   tpm_state->testState);
    return;
}

/* TPM_ProcessA() is an alternate to TPM_Process() that uses standard C types.  It provides an entry
   point to the TPM without requiring the TPM_STORE_BUFFER class.

   The design pattern for the response is:

   - set '*response' to NULL at the first call

   - on subsequent calls, pass 'response' and 'response_total' back in.  Set 'response_size' back
     to 0.

   On input:
   
   '*response' - pointer to a buffer that was allocated (can be NULL)

   'response_size' - the number of valid bytes in buffer (ignored if buffer is NULL, can be 0,
   cannot be greater than total.  Set to zero, unless one wants the TPM_Process() function to append
   a response to some existing data.

   '*response_total' - the total number of allocated bytes (ignored if buffer is NULL)

   On output:

   '*response' - pointer to a buffer that was allocated or reallocated

   'response_size' - the number of valid bytes in buffer
   
   '*response_total' - the total number of allocated or reallocated bytes
*/

TPM_RESULT TPM_ProcessA(unsigned char **response,
			uint32_t *response_size,
			uint32_t *response_total,
			unsigned char *command,		/* complete command array */
			uint32_t command_size)		/* actual bytes in command */

{
    TPM_RESULT rc = 0;
    TPM_STORE_BUFFER responseSbuffer;

    /* set the sbuffer from the response parameters */
    if (rc == 0) {
	rc = TPM_Sbuffer_Set(&responseSbuffer,
			     *response,
			     *response_size,
			     *response_total);
    }
    if (rc == 0) {
	rc = TPM_Process(&responseSbuffer,
			 command,		/* complete command array */
			 command_size);		/* actual bytes in command */

    }
    /* get the response parameters from the sbuffer */
    if (rc == 0) {
	TPM_Sbuffer_GetAll(&responseSbuffer,
			   response,
			   response_size,
			   response_total);
    }
    return rc;
}

/* Process the command from the host to the TPM.

   'command_size' is the actual size of the command stream.

   Returns:
       0 on success

       non-zero on a fatal error preventing the command from being processed.  The response is
       invalid in this case.
*/

TPM_RESULT TPM_Process(TPM_STORE_BUFFER *response,
		       unsigned char *command,		/* complete command array */
		       uint32_t command_size)		/* actual bytes in command */
{
    TPM_RESULT		rc = 0;				/* fatal error, no response */
    TPM_RESULT		returnCode = TPM_SUCCESS;	/* fatal error in ordinal processing,
							   can be returned */
    TPM_TAG		tag = 0;
    uint32_t		paramSize = 0;
    TPM_COMMAND_CODE	ordinal = 0;
    tpm_process_function_t tpm_process_function = NULL;	/* based on ordinal */
    tpm_state_t		*targetInstance = NULL;		/* TPM global state */
    TPM_STORE_BUFFER	localBuffer;		/* for response if instance was not found */
    TPM_STORE_BUFFER	*sbuffer;		/* either localBuffer or the instance response
						   buffer */

    TPM_Sbuffer_Init(&localBuffer);	/* freed @1 */
    /* get the global TPM state */
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	targetInstance = tpm_instances[0];
    }
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	/* clear the response form the previous ordinal, the response buffer is reused */
	TPM_Sbuffer_Clear(&(targetInstance->tpm_stclear_data.ordinalResponse));
	/* extract the standard command parameters from the command stream */
	returnCode = TPM_Process_GetCommandParams(&tag, &paramSize, &ordinal,
						  &command, &command_size);
    }	 
    /* preprocessing common to all ordinals */
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	returnCode = TPM_Process_Preprocess(targetInstance, ordinal, NULL);
    }
    /* NOTE Only for debugging */
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	TPM_KeyHandleEntries_Trace(targetInstance->tpm_key_handle_entries);
    }
    /* process the ordinal */
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	/* get the processing function from the ordinal table */
	TPM_OrdinalTable_GetProcessFunction(&tpm_process_function, tpm_ordinal_table, ordinal);
	/* call the processing function to execute the command */
	returnCode = tpm_process_function(targetInstance,
					  &(targetInstance->tpm_stclear_data.ordinalResponse),
					  tag, command_size, ordinal, command,
					  NULL);	/* not from encrypted transport */
    }
    /* NOTE Only for debugging */
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	TPM_KeyHandleEntries_Trace(targetInstance->tpm_key_handle_entries);
    }
    /* NOTE Only for debugging */
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	TPM_State_Trace(targetInstance);
    }
#ifdef TPM_VOLATILE_STORE
    /* save the volatile state after each command to handle fail-over restart */
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	returnCode = TPM_VolatileAll_NVStore(targetInstance);
    }
#endif	/* TPM_VOLATILE_STORE */
    /* If the ordinal processing function returned without a fatal error, append its ordinalResponse
       to the output response buffer */
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	returnCode = TPM_Sbuffer_AppendSBuffer(response,
					       &(targetInstance->tpm_stclear_data.ordinalResponse));
    }
    if ((rc == 0) && (returnCode != TPM_SUCCESS)) {
	/* gets here if:
	   
	   - there was an error before the ordinal was processed	
	   - the ordinal returned a fatal error
	   - an error occurred appending the ordinal response
	    
	   returnCode should be the response
	   errors here are fatal, can't create an error response
	*/
	/* if it failed after the target instance was found, use the instance's response buffer */
	if (targetInstance != NULL) {
	    sbuffer = &(targetInstance->tpm_stclear_data.ordinalResponse);
	}
	/* if it failed before even the target instance was found, use a local buffer */
	else {
	    sbuffer = &localBuffer;
	}
	if (rc == 0) {
	    /* it's not even known whether the initial response was stored, so just start
	       over */
	    TPM_Sbuffer_Clear(sbuffer);
	    /* store the tag, paramSize, and returnCode */
	    printf("TPM_Process: Ordinal returnCode %08x %u\n",
		   returnCode, returnCode);
	    rc = TPM_Sbuffer_StoreInitialResponse(sbuffer, TPM_TAG_RQU_COMMAND, returnCode);
	}
	/* call this to handle the TPM_FAIL causing the TPM going into failure mode */
	if (rc == 0) {
	    rc = TPM_Sbuffer_StoreFinalResponse(sbuffer, returnCode, targetInstance);
	}
	if (rc == 0) {
	    rc = TPM_Sbuffer_AppendSBuffer(response, sbuffer);
	}
    }
    /*
      cleanup
    */
    TPM_Sbuffer_Delete(&localBuffer);	/* @1 */
    return rc;
}

/* TPM_Process_Wrapped() is called recursively to process a wrapped command.

   'command_size' is the actual size of the command stream.

   'targetInstance' is an input indicating the TPM instance being called.

   'transportInternal' not NULL indicates that this function was called recursively from
   TPM_ExecuteTransport

   For wrapped commands, this function cannot trust that command_size and the incoming paramSize in
   the command stream are consistent.  Therefore, this function checks for consistency.

   The processor ensures that the response bytes are set according to the outgoing paramSize on
   return.

   Returns:
	0 on success

	non-zero on a fatal error preventing the command from being processed.	The response is
	invalid in this case.
*/

TPM_RESULT TPM_Process_Wrapped(TPM_STORE_BUFFER *response,
			       unsigned char *command,		/* complete command array */
			       uint32_t command_size,		/* actual bytes in command */
			       tpm_state_t *targetInstance,	/* global TPM state */
			       TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT		rc = 0;				/* fatal error, no response */
    TPM_RESULT		returnCode = TPM_SUCCESS;	/* non-fatal error, returned in response */
    TPM_TAG		tag = 0;
    uint32_t		paramSize = 0;
    TPM_COMMAND_CODE	ordinal = 0;
    tpm_process_function_t tpm_process_function = NULL; /* based on ordinal */
    TPM_STORE_BUFFER	ordinalResponse;		/* response for this ordinal */
    
    printf("TPM_Process_Wrapped:\n");
    TPM_Sbuffer_Init(&ordinalResponse);		/* freed @1 */
    /* Set the tag, paramSize, and ordinal from the wrapped command stream */
    /* If paramSize does not equal the command stream size, return TPM_BAD_PARAM_SIZE */
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	returnCode = TPM_Process_GetCommandParams(&tag, &paramSize, &ordinal,
						  &command, &command_size);
    }
    /* preprocessing common to all ordinals */
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	returnCode = TPM_Process_Preprocess(targetInstance, ordinal, transportInternal);
    }
    /* process the ordinal */
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	/* get the processing function from the ordinal table */
	TPM_OrdinalTable_GetProcessFunction(&tpm_process_function, tpm_ordinal_table, ordinal);
	/* call the processing function to execute the command */
	returnCode = tpm_process_function(targetInstance, &ordinalResponse,
					  tag, command_size, ordinal, command,
					  transportInternal);
    }
    /* If the ordinal processing function returned without a fatal error, append its ordinalResponse
       to the output response buffer */
    if ((rc == 0) && (returnCode == TPM_SUCCESS)) {
	returnCode = TPM_Sbuffer_AppendSBuffer(response, &ordinalResponse);
    }
    /* If:

       - an error in this function occurred before the ordinal was processed
       - the ordinal processing function returned a fatal error
       - an error occurred appending the ordinal response

       then use the return code of that failure as the final response.	Failure here is fatal, since
       no error code can be returned.
    */
    if ((rc == 0) && (returnCode != TPM_SUCCESS)) {
	rc = TPM_Sbuffer_StoreFinalResponse(response, returnCode, targetInstance);
    }
    /*
      cleanup
    */
    TPM_Sbuffer_Delete(&ordinalResponse);	/* @1 */
    return rc;
}

/* TPM_Process_GetCommandParams() gets the standard 3 parameters from the command input stream

   The stream is adjusted to point past the parameters.

   The resulting paramSize is checked against the stream size for consistency.	paramSize is
   returned for reference, but command_size reflects the remaining bytes in the stream.
*/

TPM_RESULT TPM_Process_GetCommandParams(TPM_TAG *tag,
					uint32_t *paramSize ,
					TPM_COMMAND_CODE *ordinal,
					unsigned char **command,
					uint32_t *command_size)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_Process_GetCommandParams:\n");
    /* get tag */
    if (rc == 0) {
	rc = TPM_Load16(tag, command, command_size);
    }
    /* get paramSize */
    if (rc == 0) {
	rc = TPM_Load32(paramSize, command, command_size);
    }
    /* get ordinal */
    if (rc == 0) {
	rc = TPM_Load32(ordinal, command, command_size);
    }
    /* check the paramSize against the command_size */
    if (rc == 0) {
	if (*paramSize !=
	    *command_size + sizeof(TPM_TAG) + sizeof(uint32_t) + sizeof(TPM_COMMAND_CODE)) {

	    printf("TPM_Process_GetCommandParams: Error, "
		   "command size %lu not equal to paramSize %u\n",
		   (unsigned long)
		   (*command_size + sizeof(TPM_TAG) + sizeof(uint32_t) + sizeof(TPM_COMMAND_CODE)),
		   *paramSize);
	    rc = TPM_BAD_PARAM_SIZE;
	}
	else {
	    printf("  TPM_Process_GetCommandParams: tag %04x paramSize %u ordinal %08x\n",
		   *tag, *paramSize, *ordinal);
	}
    }
    return rc;
}

/* TPM_Process_GetResponseParams() gets the standard 3 parameters from the response output stream

   The stream is adjusted to point past the parameters.

   The resulting paramSize is checked against the stream size for consistency.	paramSize is
   returned for reference, but response_size reflects the remaining bytes in the stream.
*/

TPM_RESULT TPM_Process_GetResponseParams(TPM_TAG *tag,
					 uint32_t *paramSize ,
					 TPM_RESULT *returnCode,
					 unsigned char **response,
					 uint32_t *response_size)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_Process_GetResponseParams:\n");
    /* get tag */
    if (rc == 0) {
	rc = TPM_Load16(tag, response, response_size);
    }
    /* get paramSize */
    if (rc == 0) {
	rc = TPM_Load32(paramSize, response, response_size);
    }
    /* get returnCode */
    if (rc == 0) {
	rc = TPM_Load32(returnCode, response, response_size);
    }
    /* check the paramSize against the response_size */
    if (rc == 0) {
	if (*paramSize != (*response_size + sizeof(TPM_TAG) +
			   sizeof(uint32_t) + sizeof(TPM_RESULT))) {
	    
	    printf("TPM_Process_GetResponseParams: Error, "
		   "response size %lu not equal to paramSize %u\n",
		   (unsigned long)
		   (*response_size + sizeof(TPM_TAG) + sizeof(uint32_t) + sizeof(TPM_RESULT)),
		   *paramSize);
	    rc = TPM_BAD_PARAM_SIZE;
	}
	else {
	    printf("  TPM_Process_GetResponseParams: tag %04x paramSize %u ordinal %08x\n",
		   *tag, *paramSize, *returnCode);
	}
    }
    return rc;
}

/* TPM_CheckRequestTagnnn() is common code to verify the command tag */

TPM_RESULT TPM_CheckRequestTag210(TPM_TAG tpm_tag)
{
    TPM_RESULT	rc = 0;

    if ((tpm_tag != TPM_TAG_RQU_AUTH2_COMMAND) &&
	(tpm_tag != TPM_TAG_RQU_AUTH1_COMMAND) &&
	(tpm_tag != TPM_TAG_RQU_COMMAND)) {
	printf("TPM_CheckRequestTag210: Error, tag %04hx\n", tpm_tag);
	rc = TPM_BADTAG;
    }
    return rc;
}

TPM_RESULT TPM_CheckRequestTag21(TPM_TAG tpm_tag)
{
    TPM_RESULT	rc = 0;
    
    if ((tpm_tag != TPM_TAG_RQU_AUTH2_COMMAND) &&
	(tpm_tag != TPM_TAG_RQU_AUTH1_COMMAND)) {
	printf("TPM_CheckRequestTag21: Error, tag %04hx\n", tpm_tag);
	rc = TPM_BADTAG;
    }
    return rc;
}

TPM_RESULT TPM_CheckRequestTag2(TPM_TAG tpm_tag)
{
    TPM_RESULT	rc = 0;
    
    if (tpm_tag != TPM_TAG_RQU_AUTH2_COMMAND) {
	printf("TPM_CheckRequestTag2: Error, tag %04hx\n", tpm_tag);
	rc = TPM_BADTAG;
    }
    return rc;
}

TPM_RESULT TPM_CheckRequestTag10(TPM_TAG tpm_tag)
{
    TPM_RESULT	rc = 0;
    
    if ((tpm_tag != TPM_TAG_RQU_AUTH1_COMMAND) &&
	(tpm_tag != TPM_TAG_RQU_COMMAND)) {
	printf("TPM_CheckRequestTag10: Error, tag %04hx\n", tpm_tag);
	rc = TPM_BADTAG;
    }
    return rc;
}

TPM_RESULT TPM_CheckRequestTag1(TPM_TAG tpm_tag)
{
    TPM_RESULT	rc = 0;
    
    if (tpm_tag != TPM_TAG_RQU_AUTH1_COMMAND) {
	printf("TPM_CheckRequestTag1: Error, tag %04hx\n", tpm_tag);
	rc = TPM_BADTAG;
    }
    return rc;
}

TPM_RESULT TPM_CheckRequestTag0(TPM_TAG tpm_tag)
{
    TPM_RESULT	rc = 0;
    
    if (tpm_tag != TPM_TAG_RQU_COMMAND) {
	printf("TPM_CheckRequestTag0: Error, tag %04hx\n", tpm_tag);
	rc = TPM_BADTAG;
    }
    return rc;
}

TPM_RESULT TPM_Process_Unused(tpm_state_t *tpm_state,
			      TPM_STORE_BUFFER *response,
			      TPM_TAG tag,
			      uint32_t paramSize,
			      TPM_COMMAND_CODE ordinal,
			      unsigned char *command,
			      TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;

    printf("TPM_Process_Unused:\n");
    tpm_state = tpm_state;			/* not used */
    paramSize = paramSize;			/* not used */
    ordinal = ordinal;				/* not used */
    command = command;				/* not used */
    transportInternal = transportInternal;	/* not used */
    printf("TPM_Process_Unused: Ordinal returnCode %08x %u\n",
	   TPM_BAD_ORDINAL, TPM_BAD_ORDINAL);
    rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, TPM_BAD_ORDINAL);
    return rcf;
}

/* TPM_CheckState() should be called by all commands.  It checks a set of flags specified by
   tpm_check_map to determine whether the command can execute in that state.

   Returns: 0 if the command can execute
	    non-zero error code that should be returned as a response
*/

TPM_RESULT TPM_CheckState(tpm_state_t *tpm_state,
			  TPM_TAG tag,
			  uint32_t tpm_check_map)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_CheckState: Check map %08x\n", tpm_check_map);
    /* check the dictionary attack lockout, only for authorized commands */
    if (rc == 0) {
	if ((tpm_check_map & TPM_CHECK_NO_LOCKOUT) && (tag != TPM_TAG_RQU_COMMAND)) {
	    rc = TPM_Authdata_CheckState(tpm_state);
	}
    }
    /* TPM_GetTestResult. This command can assist the TPM manufacturer in determining the cause of
       the self-test failure.  iii.  All other operations will return the error code
       TPM_FAILEDSELFTEST.  */
    if (rc == 0) {
	if (tpm_check_map & TPM_CHECK_NOT_SHUTDOWN) {
	    if (tpm_state->testState == TPM_TEST_STATE_FAILURE) {
		printf("TPM_CheckState: Error, shutdown is TRUE\n");
		rc = TPM_FAILEDSELFTEST;
	    }
	}
    }
    /* TPM_Startup SHALL execute as normal, and is the only function that does not call
       TPM_CheckState().  All other commands SHALL return TPM_INVALID_POSTINIT */
    if (rc == 0) {
	if (tpm_state->tpm_stany_flags.postInitialise) {
	    printf("TPM_CheckState: Error, postInitialise is TRUE\n");
	    rc = TPM_INVALID_POSTINIT;
	}
    }
    /*
      For checking disabled and deactivated, the check is NOT done if it's one of the special NV
      commands (indicated by TPM_CHECK_NV_NOAUTH) and nvLocked is FALSE, indicating that the NV
      store does not require authorization
    */
    /* For commands available only when enabled. */
    if (rc == 0) {
	if ((tpm_check_map & TPM_CHECK_ENABLED) &&
	    !((tpm_check_map & TPM_CHECK_NV_NOAUTH) && !tpm_state->tpm_permanent_flags.nvLocked)) {
	    if (tpm_state->tpm_permanent_flags.disable) {
		printf("TPM_CheckState: Error, disable is TRUE\n");
		rc = TPM_DISABLED;
	    }
	}
    }
    /* For commands only available when activated.  */
    if (rc == 0) {
	if ((tpm_check_map & TPM_CHECK_ACTIVATED) &&
	    !((tpm_check_map & TPM_CHECK_NV_NOAUTH) && !tpm_state->tpm_permanent_flags.nvLocked)) {
	    if (tpm_state->tpm_stclear_flags.deactivated) {
		printf("TPM_CheckState: Error, deactivated is TRUE\n");
		rc = TPM_DEACTIVATED;
	    }
	}
    }
    /* For commands available only after an owner is installed.	 see Ordinals chart */
    if (rc == 0) {
	if (tpm_check_map & TPM_CHECK_OWNER) {
	    if (!tpm_state->tpm_permanent_data.ownerInstalled) {
		printf("TPM_CheckState: Error, ownerInstalled is FALSE\n");
		rc = TPM_NOSRK;
	    }
	}
    }
    return rc;
}

/* TPM_Process_Preprocess() handles check functions common to all ordinals

   'transportPublic' not NULL indicates that this function was called recursively from
   TPM_ExecuteTransport
*/

TPM_RESULT TPM_Process_Preprocess(tpm_state_t *tpm_state,
				  TPM_COMMAND_CODE ordinal,
				  TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT		rc = 0;				/* fatal error, no response */

    printf(" TPM_Process_Preprocess: Ordinal %08x\n", ordinal);
    /* Preprocess to check if command can be run in limited operation mode */
    if (rc == 0) {
	if (tpm_state->testState == TPM_TEST_STATE_LIMITED) {
	    /* 1. At startup, a TPM MUST self-test all internal functions that are necessary to do
	       TPM_SHA1Start, TPM_SHA1Update, TPM_SHA1Complete, TPM_SHA1CompleteExtend, TPM_Extend,
	       TPM_Startup, TPM_ContinueSelfTest, a subset of TPM_GetCapability, and
	       TPM_GetTestResult..
	    */
	    if (!((ordinal == TPM_ORD_Startup) ||
		  (ordinal == TPM_ORD_SHA1Start) ||
		  (ordinal == TPM_ORD_SHA1Update) ||
		  (ordinal == TPM_ORD_SHA1Complete) ||
		  (ordinal == TPM_ORD_SHA1CompleteExtend) ||
		  (ordinal == TPM_ORD_Extend) ||
		  (ordinal == TPM_ORD_Startup) ||
		  (ordinal == TPM_ORD_ContinueSelfTest) ||
		  /* a subset of TPM_GetCapability does not require self-test.	The ordinal itself
		     decides whether to run TPM_ContinueSelfTest() */
		  (ordinal == TPM_ORD_GetCapability) ||
		  /* 3. The TPM MAY allow TPM_SelfTestFull to be used before completion of the
		     actions of TPM_ContinueSelfTest. */
		  (ordinal == TPM_ORD_SelfTestFull) ||
		  (ordinal == TPM_ORD_GetTestResult) ||
		  /* 2. The TSC_PhysicalPresence and TSC_ResetEstablishmentBit commands do not
		     operate on shielded-locations and have no requirement to be self-tested before
		     any use. TPM's SHOULD test these functions before operation. */
		  (ordinal == TSC_ORD_PhysicalPresence) ||
		  (ordinal == TSC_ORD_ResetEstablishmentBit)
		  )) {
		/* One of the optional actions. */
		/* rc = TPM_NEEDS_SELFTEST; */
		/* Alternatively, could run the actions of continue self-test */
		rc = TPM_ContinueSelfTestCmd(tpm_state);
	    }
	}
    }
    /* special pre-processing for SHA1 context */
    if (rc == 0) {
	rc = TPM_Check_SHA1Context(tpm_state, ordinal, transportInternal);
    }
    /* Special pre-processing to invalidate the saved state if it exists.  Omit this processing for
       TPM_Startup, since that function might restore the state first */
    if (rc == 0) {
	if (tpm_state->tpm_stany_flags.stateSaved &&
	    !((ordinal == TPM_ORD_Startup) ||
	      (ordinal == TPM_ORD_Init))) {
	    /* For any other ordinal, invalidate the saved state if it exists.	*/
	    rc = TPM_SaveState_NVDelete(tpm_state, TRUE);
	}
    }
    /* When an exclusive session is running, execution of any command other then
       TPM_ExecuteTransport or TPM_ReleaseTransportSigned targeting the exclusive session causes the
       abnormal invalidation of the exclusive transport session. */
    if ((rc == 0) && (transportInternal == NULL)) {	/* do test only for the outer ordinal */
	if ((tpm_state->tpm_stany_flags.transportExclusive != 0) &&	/* active exclusive */
	    /* These two ordinals terminate the exclusive transport session if the transport handle
	       is not the specified handle.  So the check is deferred until the command is parsed
	       for the transport handle. */
	    !((ordinal == TPM_ORD_ExecuteTransport) ||
	      (ordinal == TPM_ORD_ReleaseTransportSigned))) {
	    rc = TPM_TransportSessions_TerminateHandle
		 (tpm_state->tpm_stclear_data.transSessions,
		  tpm_state->tpm_stany_flags.transportExclusive,
		  &(tpm_state->tpm_stany_flags.transportExclusive));
	}
    }
    /* call platform specific code to set the localityModifier */
    if ((rc == 0) && (transportInternal == NULL)) {	/* do only for the outer ordinal */
	rc = TPM_IO_GetLocality(&(tpm_state->tpm_stany_flags.localityModifier),
				tpm_state->tpm_number);
    }
    return rc;
}


/* TPM_Check_SHA1Context() checks the current SHA1 context

   The TPM may not allow any other types of processing during the execution of a SHA-1
   session. There is only one SHA-1 session active on a TPM.  After the execution of SHA1Start, and
   prior to SHA1End, the receipt of any command other than SHA1Update will cause the invalidation of
   the SHA-1 session.

   2. After receipt of TPM_SHA1Start, and prior to the receipt of TPM_SHA1Complete or
   TPM_SHA1CompleteExtend, receipt of any command other than TPM_SHA1Update invalidates the SHA-1
   session.
   
   a. If the command received is TPM_ExecuteTransport, the SHA-1 session invalidation is based on
   the wrapped command, not the TPM_ExecuteTransport ordinal.
   
   b. A SHA-1 thread (start, update, complete) MUST take place either completely outside a transport
   session or completely within a single transport session.
*/

TPM_RESULT TPM_Check_SHA1Context(tpm_state_t *tpm_state,
				 TPM_COMMAND_CODE ordinal,
				 TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT rc = 0;

    if ((tpm_state->sha1_context != NULL) &&	/* if there was a SHA-1 context set up */
	(ordinal != TPM_ORD_ExecuteTransport))	/* depends on the wrapped command */
	{
	/* the non-SHA1 ordinals invalidate the SHA-1 session */
	if (
	    ((ordinal != TPM_ORD_SHA1Update) &&
	     (ordinal != TPM_ORD_SHA1Complete) &&
	     (ordinal != TPM_ORD_SHA1CompleteExtend)) ||
	    
	    /* invalidate if the SHA1 ordinal is within a transport session and the session was not
	       set up within the same transport session. */
	    ((transportInternal != NULL) &&
	     (tpm_state->transportHandle != transportInternal->transHandle)) ||

	    /* invalidate if the SHA1 ordinal is not within a transport session and the session was
	       set up with a transport session */
	    ((transportInternal == NULL) &&
	     (tpm_state->transportHandle != 0))
	    
	    ) {

	    printf("TPM_Check_SHA1Context: Invalidating SHA1 context\n");
	    TPM_SHA1Delete(&(tpm_state->sha1_context));
	}
    }
    return rc;
}

/* TPM_GetInParamDigest() does common processing of input parameters.
   
   Common processing includes:

   - determining if the ordinal is being run within an encrypted transport session, since the
     inParamDigest does not have to be calculated for audit in that case.

   - retrieving the audit status.  It is determinant of whether the input parameter digest should be
     calculated.

   - calculating the input parameter digest for HMAC authorization and/or auditing

   This function is called before authorization for several reasons.

   1 - It makes ordinal processing code more uniform, since authorization sometimes occurs far into
   the actions.

   2 - It is a minor optimization, since the resulting inParamDigest can be used twice in an auth-2
   command, as well as extending the audit digest.
*/
   
TPM_RESULT TPM_GetInParamDigest(TPM_DIGEST inParamDigest,		/* output */
				TPM_BOOL *auditStatus,			/* output */
				TPM_BOOL *transportEncrypt,		/* output */
				tpm_state_t *tpm_state,
				TPM_TAG tag,
				TPM_COMMAND_CODE ordinal,
				unsigned char *inParamStart,
				unsigned char *inParamEnd,
				TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT		rc = 0;			/* this function return code */ 
    TPM_COMMAND_CODE	nOrdinal;		/* ordinal in network byte order */
    
    printf(" TPM_GetInParamDigest:\n");
    if (rc == 0) {
	/* TRUE if called from encrypted transport session.  This is currently only needed when
	   auditing, but it's safer to always initialize it */
	*transportEncrypt = 
	    (transportInternal != NULL) &&	
	    (transportInternal->transPublic.transAttributes & TPM_TRANSPORT_ENCRYPT);
	printf("  TPM_GetInParamDigest: transportEncrypt %02x\n", *transportEncrypt);
	/* Determine if the ordinal should be audited. */
	rc = TPM_OrdinalAuditStatus_GetAuditStatus(auditStatus,
						   ordinal,
						   &(tpm_state->tpm_permanent_data));
    }
    /* If inParamDigest is needed for:

       1 - for auditing (auditStatus == TRUE) and not called from an encrypted transport.  Different
       parameters are audited if the ordinal is called through an encrypted transport session.

       2 - for authorization (tag != auth-0)
    */
    if (rc == 0) {
	if ((*auditStatus && !(*transportEncrypt))	||	/* digest for auditing */
	    (tag != TPM_TAG_RQU_COMMAND)) {		/* digest for authorization */

	    /* convert ordinal to network byte order */
	    nOrdinal = htonl(ordinal);

	    /* a. Create inParamDigest - digest of inputs above the double line.  NOTE: If there
	       are no inputs other than the ordinal, inParamEnd - inParamStart will be 0,
	       terminating the SHA1 vararg hash.  It is important that the termination condition
	       be the length and not the NULL pointer. */
	    rc = TPM_SHA1(inParamDigest,
			  sizeof(TPM_COMMAND_CODE), &nOrdinal,	   /* 1S */
			  inParamEnd - inParamStart, inParamStart, /* 2S - ... */
			  0, NULL);
	    if (rc == 0) {
		TPM_PrintFour("  TPM_GetInParamDigest: inParamDigest", inParamDigest);
	    }
	}
    }
    return rc;
}

/* TPM_GetOutParamDigest() does common processing of output parameters.
   
   It calculates the output parameter digest for HMAC generation and/or auditing if required.	
*/	

TPM_RESULT TPM_GetOutParamDigest(TPM_DIGEST outParamDigest,	/* output */
				 TPM_BOOL auditStatus,		/* input audit status */
				 TPM_BOOL transportEncrypt,	/* wrapped in encrypt transport */
				 TPM_TAG tag,			
				 TPM_RESULT returnCode,
				 TPM_COMMAND_CODE ordinal,	/* command ordinal (hbo) */
				 unsigned char *outParamStart,	/* starting point of param's */
				 uint32_t outParamLength)	/* length of param's */
{
    TPM_RESULT		rc = 0; 
    TPM_RESULT		nreturnCode;	/* returnCode in nbo */
    TPM_COMMAND_CODE	nOrdinal;	/* ordinal in network byte order */
    
    printf(" TPM_GetOutParamDigest:\n");
    if (rc == 0)	{
	if ((auditStatus && !transportEncrypt) || (tag != TPM_TAG_RQU_COMMAND)) {
	    nreturnCode = htonl(returnCode);
	    nOrdinal = htonl(ordinal);	
	    /* a. Create outParamDigest - digest of outputs above the double line.  NOTE: If there
	       are no outputs other than the returnCode and ordinal, outParamLength
	       will be 0, terminating the SHA1 vararg hash.  It is important that the termination
	       condition be the length and not the NULL pointer. */
	    rc = TPM_SHA1(outParamDigest,
			  sizeof(TPM_RESULT), &nreturnCode,		/* 1S */
			  sizeof(TPM_COMMAND_CODE), &nOrdinal,		/* 2S */
			  outParamLength, outParamStart,		/* 3S - ...*/
			  0, NULL);
	    if (rc == 0) {
		TPM_PrintFour("  TPM_GetOutParamDigest: outParamDigest", outParamDigest);
	    }
	}	
    }	
    return rc;	
}	

/* TPM_ProcessAudit() rev 109

   This function is called when command auditing is required.
   
   This function must be called after the output authorization, since it requires the (almost) final
   return code.
*/
   
TPM_RESULT TPM_ProcessAudit(tpm_state_t *tpm_state,
			    TPM_BOOL transportEncrypt,	/* wrapped in encrypt transport */
			    TPM_DIGEST inParamDigest,
			    TPM_DIGEST outParamDigest,
			    TPM_COMMAND_CODE ordinal)
{
    TPM_RESULT		rc = 0;			/* audit return code */
    TPM_BOOL		isZero;
    TPM_RESULT		nreturnCode;		/* returnCode in nbo */
    TPM_COMMAND_CODE	nOrdinal;		/* ordinal in network byte order */
    TPM_DIGEST		transportDigest;	/* special case digest in encrypted transport */
    
    printf(" TPM_ProcessAudit:\n");

    /* The TPM will execute the ordinal and perform auditing in the following manner: */
    /* 1. Execute command */
    /* a. Execution implies the performance of the listed actions for the ordinal. */
    /* 2. If the command will return TPM_SUCCESS */
    /* a. If TPM_STANY_DATA -> auditDigest is all zeros */
    if (rc == 0) {
	TPM_Digest_IsZero(&isZero, tpm_state->tpm_stclear_data.auditDigest);
	if (isZero) {
	    /* i. Increment TPM_PERMANENT_DATA -> auditMonotonicCounter by 1 */
	    tpm_state->tpm_permanent_data.auditMonotonicCounter.counter++;
	    printf("  TPM_ProcessAudit: Incrementing auditMonotonicCounter to %u\n",
		   tpm_state->tpm_permanent_data.auditMonotonicCounter.counter);
	    rc = TPM_PermanentAll_NVStore(tpm_state,
					  TRUE,		/* write NV */
					  0);		/* no roll back */
	}
    }
    /* b. Create A1 a TPM_AUDIT_EVENT_IN structure */
    /* i. Set A1 -> inputParms to the digest of the input parameters from the command */
    /* (1) Digest value according to the HMAC digest rules of the "above the line" parameters
       (i.e. the first HMAC digest calculation). */
    /* ii. Set A1 -> auditCount to TPM_PERMANENT_DATA -> auditMonotonicCounter */
    /* c. Set TPM_STANY_DATA -> auditDigest to SHA-1 (TPM_STANY_DATA -> auditDigest || A1) */
    if (rc == 0) {
	/* normal case, audit uses inParamDigest */
	if (!transportEncrypt) {
	    rc = TPM_AuditDigest_ExtendIn(tpm_state, inParamDigest);
	}
	/* 1. When the wrapped command requires auditing and the transport session specifies
	   encryption, the TPM MUST perform the audit. However, when computing the audit digest:
	*/
	else {
	    /* a. For input, only the ordinal is audited. */
	    if (rc == 0) {
		nOrdinal = htonl(ordinal);
		rc = TPM_SHA1(transportDigest,
			      sizeof(TPM_COMMAND_CODE), &nOrdinal,
			      0, NULL);
	    }
	    if (rc == 0) {
		rc = TPM_AuditDigest_ExtendIn(tpm_state, transportDigest);
	    }
	}
    }
    /* d. Create A2 a TPM_AUDIT_EVENT_OUT structure */
    /* i. Set A2 -> outputParms to the digest of the output parameters from the command */
    /* (1). Digest value according to the HMAC digest rules of the "above the line" parameters
       (i.e. the first HMAC digest calculation). */
    /* ii. Set A2 -> auditCount to TPM_PERMANENT_DATA -> auditMonotonicCounter */
    /* e. Set TPM_STANY_DATA -> auditDigest to SHA-1 (TPM_STANY_DATA -> auditDigest || A2) */

    /* Audit Generation Corner cases 3.a. TPM_SaveState: Only the input parameters are audited, and
       the audit occurs before the state is saved.  If an error occurs while or after the state is
       saved, the audit still occurs.
    */
    if ((rc == 0) && (ordinal != TPM_ORD_SaveState)) {
	/* normal case, audit uses outParamDigest */
	if (!transportEncrypt) {
	    rc = TPM_AuditDigest_ExtendOut(tpm_state, outParamDigest);
	}
	/* 1. When the wrapped command requires auditing and the transport session specifies
	   encryption, the TPM MUST perform the audit. However, when computing the audit digest:
	*/
	else {
	    /* b. For output, only the ordinal and return code are audited. */
	    if (rc == 0) {
		nreturnCode = htonl(TPM_SUCCESS);	/* only called when TPM_SUCCESS */
		nOrdinal = htonl(ordinal);
		rc = TPM_SHA1(transportDigest,
			      sizeof(TPM_RESULT), &nreturnCode,
			      sizeof(TPM_COMMAND_CODE), &nOrdinal,
			      0, NULL);
	    }
	    if (rc == 0) {
		rc = TPM_AuditDigest_ExtendOut(tpm_state, transportDigest);
	    }
	}
    }
    /* 1. When, in performing the audit process, the TPM has an internal failure (unable to write,
       SHA-1 failure etc.) the TPM MUST set the internal TPM state such that the TPM returns the
       TPM_FAILEDSELFTEST error on subsequent attempts to execute a command. */
    /* 2. The return code for the command uses the following rules */
    /* a. Command result success, audit success -> return TPM_SUCCESS */
    /* b. Command result failure, no audit -> return command result failure */
    /* c. Command result success, audit failure -> return TPM_AUDITFAIL_SUCCESSFUL */
    /* 3. If the TPM is permanently nonrecoverable after an audit failure, then the TPM MUST always
       return TPM_FAILEDSELFTEST for every command other than TPM_GetTestResult.  This state must
       persist regardless of power cycling, the execution of TPM_Init or any other actions. */
    if (rc != 0) {
	rc = TPM_AUDITFAIL_SUCCESSFUL;
	tpm_state->testState = TPM_TEST_STATE_FAILURE;
    }
    return rc;
}

/*
  Processing Functions
*/

/* 7.1 TPM_GetCapability rev 99

   This command returns current information regarding the TPM.

   The limitation on what can be returned in failure mode restricts the information a manufacturer
   may return when capArea indicates TPM_CAP_MFR.
*/

TPM_RESULT TPM_Process_GetCapability(tpm_state_t *tpm_state,
				     TPM_STORE_BUFFER *response,
				     TPM_TAG tag,
				     uint32_t paramSize,
				     TPM_COMMAND_CODE ordinal,
				     unsigned char *command,
				     TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;		/* fatal error precluding response */
    TPM_TAG	returnCode = 0;		/* command return code */

    /* input parameters */
    TPM_CAPABILITY_AREA capArea;	/* Partition of capabilities to be interrogated */
    TPM_SIZED_BUFFER	subCap;		/* Further definition of information */

    /* processing parameters */
    unsigned char *	inParamStart;		/* starting point of inParam's */
    unsigned char *	inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt = FALSE;/* wrapped in encrypted transport session */
    uint16_t		subCap16 = 0;		/* the subCap as a uint16_t */
    uint32_t		subCap32 = 0;		/* the subCap as a uint32_t */
    TPM_STORE_BUFFER	capabilityResponse;	/* response */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    
    printf("TPM_Process_GetCapability: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&subCap);		/* freed @1 */
    TPM_Sbuffer_Init(&capabilityResponse);	/* freed @2 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get capArea parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&capArea, &command, &paramSize);
    }
    /* get subCap parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_GetCapability: capArea %08x\n", capArea);
	returnCode = TPM_SizedBuffer_Load(&subCap, &command, &paramSize);
    }
    /* subCap is often a uint16_t or uint32_t, create them now */
    if (returnCode == TPM_SUCCESS) {
	TPM_GetSubCapInt(&subCap16, &subCap32, &subCap);
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
    /* The shutdown test is delayed until after the subcap is calculated */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_NO_LOCKOUT);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_GetCapability: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      check state
    */
    /* 1. The TPM validates the capArea and subCap indicators. If the information is available, the
	  TPM creates the response field and fills in the actual information. */
    /* 2. The structure document contains the list of caparea and subCap values */
    if (returnCode == TPM_SUCCESS) {
	/* 3. If the TPM is in failure mode or limited operation mode, the TPM MUST return */
	if ((tpm_state->testState == TPM_TEST_STATE_FAILURE) ||
	    (tpm_state->testState == TPM_TEST_STATE_LIMITED)) {
	    /* a. TPM_CAP_VERSION */
	    /* b. TPM_CAP_VERSION_VAL */
	    /* c. TPM_CAP_MFR */
	    /* d. TPM_CAP_PROPERTY -> TPM_CAP_PROP_MANUFACTURER */
	    /* e. TPM_CAP_PROPERTY -> TPM_CAP_PROP_DURATION */
	    /* f. TPM_CAP_PROPERTY -> TPM_CAP_PROP_TIS_TIMEOUT */
	    /* g. The TPM MAY return any other capability. */
	    if (
		!(capArea == TPM_CAP_VERSION) &&
		!(capArea == TPM_CAP_VERSION_VAL) &&
		!(capArea == TPM_CAP_MFR) &&
		!((capArea == TPM_CAP_PROPERTY) && (subCap32 == TPM_CAP_PROP_MANUFACTURER)) &&
		!((capArea == TPM_CAP_PROPERTY) && (subCap32 == TPM_CAP_PROP_DURATION)) &&
		!((capArea == TPM_CAP_PROPERTY) && (subCap32 == TPM_CAP_PROP_TIS_TIMEOUT))
		) {
		if (tpm_state->testState == TPM_TEST_STATE_FAILURE)  {
		    printf("TPM_Process_GetCapability: Error, shutdown capArea %08x subCap %08x\n",
			   capArea, subCap32);
		    returnCode = TPM_FAILEDSELFTEST;
		}
		else {
		    printf("TPM_Process_GetCapability: Limited operation, run self-test\n");
		    returnCode = TPM_ContinueSelfTestCmd(tpm_state);
		}
	    }
	}
    }
    /*
      Processing
    */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_GetCapability: capArea %08x subCap32 subCap16 %08x %04x\n",
	       capArea, subCap32, subCap16);
	returnCode = TPM_GetCapabilityCommon(&capabilityResponse, tpm_state,
					     capArea, subCap16, subCap32, &subCap);
    }
    /*
      response
    */
    if (rcf == 0) {
	printf("TPM_Process_GetCapability: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* store the capabilityResponse */
	    returnCode = TPM_Sbuffer_AppendAsSizedBuffer(response, &capabilityResponse);
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
    TPM_SizedBuffer_Delete(&subCap);		/* @1 */
    TPM_Sbuffer_Delete(&capabilityResponse);	/* @2 */
    return rcf;
}

/* TPM_GetSubCapInt() converts from a TPM_SIZED_BUFFER to either a uint16_t or uint32_t as
   applicable

   No return code is needed.  If the size it not applicable, a 0 value is returned, which is
   (fortunately) always illegal for subCap integral values.
*/

void TPM_GetSubCapInt(uint16_t *subCap16,
		      uint32_t *subCap32,
		      TPM_SIZED_BUFFER *subCap)
{
    *subCap16 = 0;	/* default, means was not a uint16_t */
    *subCap32 = 0;	/* default, means was not a uint32_t */
    if (subCap->size == sizeof(uint32_t)) {
	*subCap32 = htonl(*(uint32_t *)subCap->buffer);
	printf(" TPM_GetSubCapInt: subCap %08x\n", *subCap32);
    }
    else if (subCap->size == sizeof(uint16_t)) {
	*subCap16 = htons(*(uint16_t *)subCap->buffer);
	printf(" TPM_GetSubCapInt: subCap %04x\n", *subCap16);
    }
}


/* TPM_GetCapabilityCommon() is common code for getting a capability.
   
   It loads the result to 'capabilityResponse'

   A previously called TPM_GetSubCapInt() converts the subCap buffer into a subCap16 if the size is
   2 or subCap32 if the size is 4.  If the values are used, this function checks the size to ensure
   that the incoming subCap parameter was correct for the capArea.
*/

TPM_RESULT TPM_GetCapabilityCommon(TPM_STORE_BUFFER *capabilityResponse,
				   tpm_state_t *tpm_state, 
				   TPM_CAPABILITY_AREA capArea, 
				   uint16_t subCap16, 
				   uint32_t subCap32,
				   TPM_SIZED_BUFFER *subCap)
			     
{
    TPM_RESULT rc = 0;

    printf(" TPM_GetCapabilityCommon: capArea %08x\n", capArea);
    switch (capArea) {
      case TPM_CAP_ORD:
	if (subCap->size == sizeof(uint32_t)) {
	    rc = TPM_GetCapability_CapOrd(capabilityResponse, subCap32);
	}
	else {
	    printf("TPM_GetCapabilityCommon: Error, Bad subCap size %u\n", subCap->size);
	    rc = TPM_BAD_MODE;
	}
	break;
      case TPM_CAP_ALG:
	if (subCap->size == sizeof(uint32_t)) {
	    rc = TPM_GetCapability_CapAlg(capabilityResponse, subCap32);
	}
	else {
	    printf("TPM_GetCapabilityCommon: Error, Bad subCap size %u\n", subCap->size);
	    rc = TPM_BAD_MODE;
	}
	break;
      case TPM_CAP_PID:
	if (subCap->size == sizeof(uint16_t)) {
	    rc = TPM_GetCapability_CapPid(capabilityResponse, subCap16);
	}
	else {
	    printf("TPM_GetCapabilityCommon: Error, Bad subCap size %u\n", subCap->size);
	    rc = TPM_BAD_MODE;
	}
	break;
      case TPM_CAP_FLAG: 
	if (subCap->size == sizeof(uint32_t)) {
	    rc = TPM_GetCapability_CapFlag(capabilityResponse, tpm_state, subCap32);
	}
	else {
	    printf("TPM_GetCapabilityCommon: Error, Bad subCap size %u\n", subCap->size);
	    rc = TPM_BAD_MODE;
	}
	break;
      case TPM_CAP_PROPERTY: 
	if (subCap->size == sizeof(uint32_t)) {
	    rc = TPM_GetCapability_CapProperty(capabilityResponse, tpm_state, subCap32);
	}
	else {
	    printf("TPM_GetCapabilityCommon: Error, Bad subCap size %u\n", subCap->size);
	    rc = TPM_BAD_MODE;
	}
	break;
      case TPM_CAP_VERSION: 
	rc = TPM_GetCapability_CapVersion(capabilityResponse);
	break;
      case TPM_CAP_KEY_HANDLE:
	/* This is command is available for backwards compatibility. It is the same as
	   TPM_CAP_HANDLE with a resource type of keys. */
	rc = TPM_KeyHandleEntries_StoreHandles(capabilityResponse,
					       tpm_state->tpm_key_handle_entries);
	break;
      case TPM_CAP_CHECK_LOADED: 
	rc = TPM_GetCapability_CapCheckLoaded(capabilityResponse,
					      tpm_state->tpm_key_handle_entries,
					      subCap);
	break;
      case TPM_CAP_SYM_MODE:
	if (subCap->size == sizeof(uint32_t)) {
	    rc = TPM_GetCapability_CapSymMode(capabilityResponse, subCap32);
	}
	else {
	    printf("TPM_GetCapabilityCommon: Error, Bad subCap size %u\n", subCap->size);
	    rc = TPM_BAD_MODE;
	}
	break;
      case TPM_CAP_KEY_STATUS: 
	if (subCap->size == sizeof(uint32_t)) {
	    rc = TPM_GetCapability_CapKeyStatus(capabilityResponse,
						tpm_state->tpm_key_handle_entries,
						subCap32);
	}
	else {
	    printf("TPM_GetCapabilityCommon: Error, Bad subCap size %u\n", subCap->size);
	    rc = TPM_BAD_MODE;
	}
	break;
      case TPM_CAP_NV_LIST:
	rc = TPM_NVIndexEntries_GetNVList(capabilityResponse, &(tpm_state->tpm_nv_index_entries));
	break;
      case TPM_CAP_MFR:
	rc = TPM_GetCapability_CapMfr(capabilityResponse, tpm_state, subCap);
	break;
      case TPM_CAP_NV_INDEX: 
	if (subCap->size == sizeof(uint32_t)) {
	    rc = TPM_GetCapability_CapNVIndex(capabilityResponse, tpm_state, subCap32);
	}
	else {
	    printf("TPM_GetCapabilityCommon: Error, Bad subCap size %u\n", subCap->size);
	    rc = TPM_BAD_MODE;
	}
	break;
      case TPM_CAP_TRANS_ALG:
	if (subCap->size == sizeof(uint32_t)) {
	    rc = TPM_GetCapability_CapTransAlg(capabilityResponse, subCap32);
	}
	else {
	    printf("TPM_GetCapabilityCommon: Error, Bad subCap size %u\n", subCap->size);
	    rc = TPM_BAD_MODE;
	}
	break;
      case TPM_CAP_HANDLE:
	if (subCap->size == sizeof(uint32_t)) {
	    rc = TPM_GetCapability_CapHandle(capabilityResponse, tpm_state, subCap32);
	}
	else {
	    printf("TPM_GetCapabilityCommon: Error, Bad subCap size %u\n", subCap->size);
	    rc = TPM_BAD_MODE;
	}
	break;
      case TPM_CAP_TRANS_ES:
	if (subCap->size == sizeof(uint16_t)) {
	    rc = TPM_GetCapability_CapTransEs(capabilityResponse, subCap16);
	}
	else {
	    printf("TPM_GetCapabilityCommon: Error, Bad subCap size %u\n", subCap->size);
	    rc = TPM_BAD_MODE;
	}
	break;
      case TPM_CAP_AUTH_ENCRYPT:
	if (subCap->size == sizeof(uint32_t)) {
	    rc = TPM_GetCapability_CapAuthEncrypt(capabilityResponse, subCap32);
	}
	else {
	    printf("TPM_GetCapabilityCommon: Error, Bad subCap size %u\n", subCap->size);
	    rc = TPM_BAD_MODE;
	}
	break;
      case TPM_CAP_SELECT_SIZE:
	rc = TPM_GetCapability_CapSelectSize(capabilityResponse, subCap);
	break;
#if  (TPM_REVISION >= 103)	/* added for rev 103 */
      case TPM_CAP_DA_LOGIC:
	rc = TPM_GetCapability_CapDaLogic(capabilityResponse, subCap, tpm_state);
	break;
#endif
      case TPM_CAP_VERSION_VAL:
	rc = TPM_GetCapability_CapVersionVal(capabilityResponse,
					     &(tpm_state->tpm_permanent_data));
	break;
      default:
	printf("TPM_GetCapabilityCommon: Error, unsupported capArea %08x", capArea);
	rc = TPM_BAD_MODE;
	break;
    }
    return rc;
}

/* Boolean value.

   TRUE indicates that the TPM supports the ordinal.

   FALSE indicates that the TPM does not support the ordinal.
*/

static TPM_RESULT TPM_GetCapability_CapOrd(TPM_STORE_BUFFER *capabilityResponse,
					   uint32_t ordinal)
{
    TPM_RESULT			rc = 0;
    tpm_process_function_t	tpm_process_function;
    TPM_BOOL			supported;

    TPM_OrdinalTable_GetProcessFunction(&tpm_process_function, tpm_ordinal_table, ordinal);
    /* determine of the ordinal is supported */
    if (tpm_process_function != TPM_Process_Unused) {
	supported = TRUE;
    }
    /* if the processing function is 'Unused', it's not supported */
    else {
	supported = FALSE;
    }	
    printf("  TPM_GetCapability_CapOrd: Ordinal %08x, result %02x\n",
	   ordinal, supported);
    rc = TPM_Sbuffer_Append(capabilityResponse, &supported, sizeof(TPM_BOOL));
    return rc;
}

/* algorithmID is TPM_ALG_XX: A value from TPM_ALGORITHM_ID

   Boolean value. TRUE means that the TPM supports the asymmetric algorithm for TPM_Sign, TPM_Seal,
   TPM_UnSeal and TPM_UnBind and related commands. FALSE indicates that the asymmetric algorithm is
   not supported for these types of commands. The TPM MAY return TRUE or FALSE for other than
   asymmetric algorithms that it supports. Unassigned and unsupported algorithm IDs return FALSE.
*/

static TPM_RESULT TPM_GetCapability_CapAlg(TPM_STORE_BUFFER *capabilityResponse,
					   uint32_t algorithmID)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	supported;

    printf(" TPM_GetCapability_CapAlg: algorithmID %08x\n", algorithmID);
    if (algorithmID == TPM_ALG_RSA) {
	supported = TRUE;
    }
    else {
	supported = FALSE;
    }
    printf("  TPM_GetCapability_CapAlg: Result %08x\n", supported);
    rc = TPM_Sbuffer_Append(capabilityResponse, &supported, sizeof(TPM_BOOL));
    return rc;
}    

/* Boolean value.

   TRUE indicates that the TPM supports the protocol,

   FALSE indicates that the TPM does not support the protocol.
*/

static TPM_RESULT TPM_GetCapability_CapPid(TPM_STORE_BUFFER *capabilityResponse,
					   uint16_t protocolID)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	supported;

    printf(" TPM_GetCapability_CapPid: protocolID %04hx\n", protocolID);
    switch (protocolID) {
	/* supported protocols */
      case TPM_PID_OIAP:
      case TPM_PID_OSAP:
      case TPM_PID_ADIP:
      case TPM_PID_ADCP:
      case TPM_PID_DSAP:
      case TPM_PID_TRANSPORT:
      case TPM_PID_OWNER:
	supported = TRUE;
	break;
	/* unsupported protocols */
      default:
	supported = FALSE;
	break;
    }	
    printf("  TPM_GetCapability_CapPid: Result %08x\n", supported);
    rc = TPM_Sbuffer_Append(capabilityResponse, &supported, sizeof(TPM_BOOL));
    return rc;
}    

/*
  Either of the next two subcaps

  TPM_CAP_FLAG_PERMANENT  Return the TPM_PERMANENT_FLAGS structure

  TPM_CAP_FLAG_VOLATILE	 Return the TPM_STCLEAR_FLAGS structure
*/

static TPM_RESULT TPM_GetCapability_CapFlag(TPM_STORE_BUFFER *capabilityResponse,
					    tpm_state_t *tpm_state,
					    uint32_t capFlag)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_GetCapability_CapFlag: capFlag %08x\n", capFlag);
    switch (capFlag) {
      case TPM_CAP_FLAG_PERMANENT:
	printf("  TPM_GetCapability_CapFlag: TPM_CAP_FLAG_PERMANENT\n");;
	rc = TPM_PermanentFlags_StoreBytes(capabilityResponse, &(tpm_state->tpm_permanent_flags));
	break;
      case TPM_CAP_FLAG_VOLATILE:
	printf("  TPM_GetCapability_CapFlag: TPM_CAP_FLAG_VOLATILE\n");
	rc = TPM_StclearFlags_Store(capabilityResponse, &(tpm_state->tpm_stclear_flags));
	break;
      default:
	printf("TPM_GetCapability_CapFlag: Error, illegal capFlag %08x\n", capFlag);
	rc = TPM_BAD_MODE;
	break;
    }
    return rc;
}    

/* TPM_GetCapability_CapProperty() handles Subcap values for CAP_PROPERTY rev 100
 */

static TPM_RESULT TPM_GetCapability_CapProperty(TPM_STORE_BUFFER *capabilityResponse,
						tpm_state_t *tpm_state,
						uint32_t capProperty)
{
    TPM_RESULT	rc = 0;
    uint32_t 	uint32;
    uint32_t 	uint32a;
    uint32_t 	dummy;	/* to hold unused response parameter */

    printf(" TPM_GetCapability_CapProperty: capProperty %08x\n", capProperty);
    switch (capProperty) {
      case TPM_CAP_PROP_PCR:	/* Returns the number of PCR registers supported by the TPM */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_PCR %u\n", TPM_NUM_PCR);
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_NUM_PCR);
	break;
      case TPM_CAP_PROP_DIR:	/* Returns the number of DIR registers under control of the TPM
				   owner supported by the TPM. */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_DIR %u\n", TPM_AUTHDIR_SIZE);
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_AUTHDIR_SIZE);
	break;
      case TPM_CAP_PROP_MANUFACTURER:	/* Returns the Identifier of the TPM manufacturer. */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_MANUFACTURER %.4s\n",
	       TPM_MANUFACTURER);
	rc = TPM_Sbuffer_Append(capabilityResponse, (const unsigned char *)TPM_MANUFACTURER, 4);
	break;
      case TPM_CAP_PROP_KEYS:	/* Returns the number of 2048-bit RSA keys that can be loaded. This
				   MAY vary with time and circumstances. */
	TPM_KeyHandleEntries_GetSpace(&uint32, tpm_state->tpm_key_handle_entries);
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_KEYS %u\n", uint32);
	rc = TPM_Sbuffer_Append32(capabilityResponse, uint32);
	break;
      case TPM_CAP_PROP_MIN_COUNTER: /* uint32_t. The minimum amount of time in 10ths of a second
					that must pass between invocations of incrementing the
					monotonic counter. */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_MIN_COUNTER\n");
	rc = TPM_Sbuffer_Append32(capabilityResponse, 0);
	break;
      case TPM_CAP_PROP_AUTHSESS:	/* The number of available authorization sessions. This MAY
					   vary with time and circumstances. */
	TPM_AuthSessions_GetSpace(&uint32, tpm_state->tpm_stclear_data.authSessions);
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_AUTHSESS space %u\n", uint32);
	rc = TPM_Sbuffer_Append32(capabilityResponse, uint32);
	break;
      case TPM_CAP_PROP_TRANSESS:	/* The number of available transport sessions. This MAY vary
					   with time and circumstances.	 */
	TPM_TransportSessions_GetSpace(&uint32, tpm_state->tpm_stclear_data.transSessions);
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_TRANSESS space %u\n", uint32);
	rc = TPM_Sbuffer_Append32(capabilityResponse, uint32);
	break;
      case TPM_CAP_PROP_COUNTERS:	/* The number of available monotonic counters. This MAY vary
					   with time and circumstances. */
	TPM_Counters_GetSpace(&uint32, tpm_state->tpm_permanent_data.monotonicCounter);
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_COUNTERS %u\n", uint32);
	rc = TPM_Sbuffer_Append32(capabilityResponse, uint32);
	break;
      case TPM_CAP_PROP_MAX_AUTHSESS:	/* The maximum number of loaded authorization sessions the
					   TPM supports. */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_MAX_AUTHSESS %u\n",
	       TPM_MIN_AUTH_SESSIONS);
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_MIN_AUTH_SESSIONS);
	break;
      case TPM_CAP_PROP_MAX_TRANSESS:	/* The maximum number of loaded transport sessions the TPM
					   supports. */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_MAX_TRANSESS %u\n",
	       TPM_MIN_TRANS_SESSIONS);
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_MIN_TRANS_SESSIONS);
	break;
      case TPM_CAP_PROP_MAX_COUNTERS:	/* The maximum number of monotonic counters under control of
					   TPM_CreateCounter */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_MAX_COUNTERS %u\n",
	       TPM_MIN_COUNTERS);
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_MIN_COUNTERS);
	break;
      case TPM_CAP_PROP_MAX_KEYS:	/* The maximum number of 2048 RSA keys that the TPM can
					   support. The number does not include the EK or SRK. */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_MAX_KEYS %u\n", TPM_KEY_HANDLES);
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_KEY_HANDLES);
	break;
      case TPM_CAP_PROP_OWNER:	/* A value of TRUE indicates that the TPM has successfully installed
				   an owner. */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_OWNER %02x\n",
	       tpm_state->tpm_permanent_data.ownerInstalled);
	rc = TPM_Sbuffer_Append(capabilityResponse,
				&(tpm_state->tpm_permanent_data.ownerInstalled), sizeof(TPM_BOOL));
	break;
      case TPM_CAP_PROP_CONTEXT:	/* The number of available saved session slots. This MAY
					   vary with time and circumstances. */
	TPM_ContextList_GetSpace(&uint32, &dummy, tpm_state->tpm_stclear_data.contextList);
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_CONTEXT %u\n", uint32);
	rc = TPM_Sbuffer_Append32(capabilityResponse, uint32);
	break;
      case TPM_CAP_PROP_MAX_CONTEXT:	/* The maximum number of saved session slots. */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_MAX_CONTEXT %u\n",
	       TPM_MIN_SESSION_LIST);
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_MIN_SESSION_LIST);
	break;
      case TPM_CAP_PROP_FAMILYROWS:	/* The number of rows in the family table */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_FAMILYROWS %u\n",
	       TPM_NUM_FAMILY_TABLE_ENTRY_MIN);
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_NUM_FAMILY_TABLE_ENTRY_MIN);
	break;
      case TPM_CAP_PROP_TIS_TIMEOUT:
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_TIS_TIMEOUT\n");
	rc = TPM_GetCapability_CapPropTisTimeout(capabilityResponse);
	break;
      case TPM_CAP_PROP_STARTUP_EFFECT: /* The TPM_STARTUP_EFFECTS structure */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_STARTUP_EFFECT %08x\n",
	       TPM_STARTUP_EFFECTS_VALUE);
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_STARTUP_EFFECTS_VALUE);
	break;
      case TPM_CAP_PROP_DELEGATE_ROW:	/* The size of the delegate table in rows. */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_DELEGATE_ENTRIES %u\n",
	       TPM_NUM_DELEGATE_TABLE_ENTRY_MIN);
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_NUM_DELEGATE_TABLE_ENTRY_MIN);
	break;
      case TPM_CAP_PROP_MAX_DAASESS:	/* The maximum number of loaded DAA sessions (join or sign)
					   that the TPM supports */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_DAA_MAX\n");
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_MIN_DAA_SESSIONS);
	break;
      case TPM_CAP_PROP_DAASESS:	/* The number of available DAA sessions. This may vary with
					   time and circumstances */
	TPM_DaaSessions_GetSpace(&uint32, tpm_state->tpm_stclear_data.daaSessions);
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_SESSION_DAA space %u\n", uint32);
	rc = TPM_Sbuffer_Append32(capabilityResponse, uint32);
	break;
      case TPM_CAP_PROP_CONTEXT_DIST:	/* The maximum distance between context count values. This
					   MUST be at least 2^16-1. */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_CONTEXT_DIST\n");
	rc = TPM_Sbuffer_Append32(capabilityResponse, 0xffffffff);
	break;
      case TPM_CAP_PROP_DAA_INTERRUPT:	/* BOOL. A value of TRUE indicates that the TPM will accept
					   ANY command while executing a DAA Join or Sign.

					   A value of FALSE indicates that the TPM will invalidate
					   the DAA Join or Sign upon the receipt of any command
					   other than the next join/sign in the session or a
					   TPM_SaveContext */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_DAA_INTERRUPT\n");
	rc = TPM_Sbuffer_Append8(capabilityResponse, TRUE);   
	break;
      case TPM_CAP_PROP_SESSIONS: /* UNIT32. The number of available authorization and transport
				     sessions from the pool. This may vary with time and
				     circumstances. */
	TPM_AuthSessions_GetSpace(&uint32, tpm_state->tpm_stclear_data.authSessions);
	TPM_TransportSessions_GetSpace(&uint32a, tpm_state->tpm_stclear_data.transSessions);
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_SESSIONS %u + %u\n", uint32, uint32a);
	rc = TPM_Sbuffer_Append32(capabilityResponse, uint32 + uint32a);
	break;
      case TPM_CAP_PROP_MAX_SESSIONS: /* uint32_t. The maximum number of sessions the
					 TPM supports. */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_MAX_SESSIONS\n");
	rc = TPM_Sbuffer_Append32(capabilityResponse,
				  TPM_MIN_AUTH_SESSIONS + TPM_MIN_TRANS_SESSIONS);
	break;
      case TPM_CAP_PROP_CMK_RESTRICTION: /* uint32_t TPM_Permanent_Data -> restrictDelegate */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_CMK_RESTRICTION %08x\n",
	       tpm_state->tpm_permanent_data.restrictDelegate);
	rc = TPM_Sbuffer_Append32(capabilityResponse,
				  tpm_state->tpm_permanent_data.restrictDelegate);
	break;
      case TPM_CAP_PROP_DURATION: 
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_DURATION\n");
	rc = TPM_GetCapability_CapPropDuration(capabilityResponse);
	break;
      case TPM_CAP_PROP_ACTIVE_COUNTER: /* TPM_COUNT_ID. The id of the current counter. 0xff..ff if
					   no counter is active */
	TPM_Counters_GetActiveCounter(&uint32, tpm_state->tpm_stclear_data.countID);
	/* The illegal value after releasing an active counter must be mapped back to the null
	   value */
	if (uint32 == TPM_COUNT_ID_ILLEGAL) {
	    uint32 = TPM_COUNT_ID_NULL;
	}
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_ACTIVE_COUNTER %u\n", uint32);
	rc = TPM_Sbuffer_Append32(capabilityResponse, uint32);
	break;
      case TPM_CAP_PROP_MAX_NV_AVAILABLE: /* uint32_t. Deprecated.  The maximum number of NV space
					     that can be allocated, MAY vary with time and
					     circumstances.  This capability was not implemented
					     consistently, and is replaced by
					     TPM_NV_INDEX_TRIAL.  */
	rc = TPM_NVIndexEntries_GetFreeSpace(&uint32, &(tpm_state->tpm_nv_index_entries));
	if (rc == 0) {
	    printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_MAX_NV_AVAILABLE %u\n", uint32);
	    rc = TPM_Sbuffer_Append32(capabilityResponse, uint32);
	}
	/* There should always be free space >= 0.  If the call fails here, there is an internal
	   error. */
	else {
	    printf(" TPM_GetCapability_CapProperty: Error (fatal) "
		   "in TPM_CAP_PROP_MAX_NV_AVAILABLE\n");
	    rc = TPM_FAIL;
	}
	break;
      case TPM_CAP_PROP_INPUT_BUFFER: /* uint32_t. The size of the TPM input and output buffers in
					 bytes. */
	printf(" TPM_GetCapability_CapProperty: TPM_CAP_PROP_INPUT_BUFFER %u\n",
	       TPM_BUFFER_MAX);
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_BUFFER_MAX);
	break;
     default:
	printf("TPM_GetCapability_CapProperty: Error, illegal capProperty %08x\n", capProperty);
	rc = TPM_BAD_MODE;
	break;
    }
    return rc;
}    

/* TPM_VERSION structure. The Major and Minor must indicate 1.1.

   The manufacturer information MUST indicate the firmware version of the TPM.

   Any software using this structure MUST be aware that when included in a structure the value MUST
   be 1.1.0.0, when reported by this command the manufacturer information MAY include firmware
   versions.  The use of this value is deprecated, new software SHOULD use TPM_CAP_VERSION_VAL to
   obtain version information regarding the TPM.
   
   Return 0.0 for revision for 1.1 backward compatibility, since TPM_PERMANENT_DATA now holds the
   new type TPM_VERSION_BYTE.
*/

static TPM_RESULT TPM_GetCapability_CapVersion(TPM_STORE_BUFFER *capabilityResponse)
{
    TPM_RESULT	rc = 0;
    TPM_STRUCT_VER	tpm_struct_ver;

    TPM_StructVer_Init(&tpm_struct_ver);
    printf(" TPM_GetCapability_CapVersion: %u.%u.%u.%u\n",
	   tpm_struct_ver.major, tpm_struct_ver.minor,
	   tpm_struct_ver.revMajor, tpm_struct_ver.revMinor);
    rc = TPM_StructVer_Store(capabilityResponse, &tpm_struct_ver);
    return rc;
}

/* A Boolean value.

   TRUE indicates that the TPM has enough memory available to load a key of the type specified by
   ALGORITHM.

   FALSE indicates that the TPM does not have enough memory.
*/

static TPM_RESULT TPM_GetCapability_CapCheckLoaded(TPM_STORE_BUFFER *capabilityResponse,
						   const TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entry,
						   TPM_SIZED_BUFFER *subCap)
{
    TPM_RESULT		rc = 0;
    uint32_t		stream_size;
    unsigned char	*stream;
    TPM_KEY_PARMS	keyParms;
    TPM_BOOL		isSpace;
    uint32_t		index;

    TPM_KeyParms_Init(&keyParms);		/* freed @1 */
    if (rc == 0) {
	/* make temporary copies so the subCap is not touched */
	stream = subCap->buffer;
	stream_size = subCap->size;
	rc = TPM_KeyParms_Load(&keyParms, &stream, &stream_size);
    }
    if (rc == 0) {
	if (keyParms.algorithmID == TPM_ALG_RSA) {
	    TPM_KeyHandleEntries_IsSpace(&isSpace, &index, tpm_key_handle_entry);
	}
	else {
	    printf(" TPM_GetCapability_CapCheckLoaded: algorithmID %08x is not TPM_ALG_RSA %08x\n",
		   keyParms.algorithmID, TPM_ALG_RSA);
	    isSpace = FALSE;
	}
    }
    if (rc == 0) {
	printf(" TPM_GetCapability_CapCheckLoaded: Return %02x\n", isSpace);
	rc = TPM_Sbuffer_Append(capabilityResponse, &isSpace, sizeof(TPM_BOOL));
    }
    TPM_KeyParms_Delete(&keyParms);		/* @1 */
    return rc;
}

/* (Deprecated) This indicates the mode of a symmetric encryption. Mode is Electronic CookBook (ECB)
   or some other such mechanism.
*/

static TPM_RESULT TPM_GetCapability_CapSymMode(TPM_STORE_BUFFER *capabilityResponse,
					       TPM_SYM_MODE symMode)
{
    TPM_RESULT	rc = 0;
    
    symMode = symMode;	/* not currently used */
    printf(" TPM_GetCapability_CapSymMode: Return %02x\n", FALSE);
    rc = TPM_Sbuffer_Append8(capabilityResponse, FALSE);
    return rc;
}

/* Boolean value of ownerEvict. The handle MUST point to a valid key handle.
 */

static TPM_RESULT TPM_GetCapability_CapKeyStatus(TPM_STORE_BUFFER *capabilityResponse,
						 TPM_KEY_HANDLE_ENTRY *tpm_key_handle_entries,
						 uint32_t tpm_key_handle)
{
    TPM_RESULT			rc = 0;
    TPM_KEY_HANDLE_ENTRY	*tpm_key_handle_entry;	/* corresponding to handle */
    TPM_BOOL			ownerEvict;
    
    printf(" TPM_GetCapability_CapKeyStatus: key handle %08x\n", tpm_key_handle);
    /* map from the handle to the TPM_KEY structure */
    if (rc == 0) {
	rc = TPM_KeyHandleEntries_GetEntry(&tpm_key_handle_entry,
					   tpm_key_handle_entries,
					   tpm_key_handle);
	if (rc != 0) {
	    printf("TPM_GetCapability_CapKeyStatus: Error, key handle %08x not found\n",
		   tpm_key_handle);
	}
    }
    /* test the ownerEvict bit */
    if (rc == 0) {
	ownerEvict = (tpm_key_handle_entry->keyControl & TPM_KEY_CONTROL_OWNER_EVICT) ?
		     TRUE : FALSE;;
	printf(" TPM_GetCapability_CapKeyStatus: return %02x\n", ownerEvict);
	rc = TPM_Sbuffer_Append(capabilityResponse, &ownerEvict, sizeof(TPM_BOOL));
    }
    return rc;
}

/* Manufacturer specific. The manufacturer may provide any additional information regarding the TPM
   and the TPM state but MUST not expose any sensitive information.
*/

static TPM_RESULT TPM_GetCapability_CapMfr(TPM_STORE_BUFFER *capabilityResponse,
					   tpm_state_t *tpm_state,
					   TPM_SIZED_BUFFER *subCap)
{
    TPM_RESULT	rc = 0;
    uint32_t	subCap32;
    
    /* all of the subCaps are at least a uint32_t.  Some have more data */
    if (rc == 0) {
	if (subCap->size >= sizeof(uint32_t)) {
	    subCap32 = htonl(*(uint32_t *)subCap->buffer);
	    printf(" TPM_GetCapability_CapMfr: subCap %08x\n", subCap32);
	}
	else {
	    printf("TPM_GetCapability_CapMfr: Error, subCap size %u < %lu\n",
		   subCap->size, (unsigned long)sizeof(uint32_t));
	    rc = TPM_BAD_MODE;
	}
    }
    /* switch on the subCap and append the get capability response to the capabilityResponse
       buffer */
    if (rc == 0) {
	switch(subCap32) {
#ifdef TPM_POSIX
	  case TPM_CAP_PROCESS_ID:
	    if (subCap->size == sizeof(uint32_t)) {
		pid_t pid = getpid();
		printf(" TPM_GetCapability_CapMfr: TPM_CAP_PROCESS_ID %u\n", (uint32_t)pid);
		rc = TPM_Sbuffer_Append32(capabilityResponse, (uint32_t)pid);
	    }
	    else {
		printf("TPM_GetCapability_CapMfr: Error, Bad subCap size %u\n", subCap->size);
		rc = TPM_BAD_MODE;
	    }
	    break;
#endif
	  default:
	    capabilityResponse = capabilityResponse;	/* not used */
	    tpm_state = tpm_state;			/* not used */
	    printf("TPM_GetCapability_CapMfr: Error, unsupported subCap %08x\n", subCap32);
	    rc = TPM_BAD_MODE;
	    break;
	}
    }
    return rc;
}

/* Returns a TPM_NV_DATA_PUBLIC structure that indicates the values for the TPM_NV_INDEX
*/

static TPM_RESULT TPM_GetCapability_CapNVIndex(TPM_STORE_BUFFER *capabilityResponse,
					       tpm_state_t *tpm_state,
					       uint32_t nvIndex)
{
    TPM_RESULT		rc = 0;
    TPM_NV_DATA_PUBLIC 	*tpm_nv_data_public;
    
    printf(" TPM_GetCapability_CapNVIndex: nvIndex %08x\n", nvIndex);
    /* map from the nvIndex to the TPM_NV_DATA_PUBLIC structure */
    if (rc == 0) {
	rc = TPM_NVIndexEntries_GetDataPublic(&tpm_nv_data_public,
					      &(tpm_state->tpm_nv_index_entries),
					      nvIndex);
    }
    /* serialize the structure */
    if (rc == 0) {
	rc = TPM_NVDataPublic_Store(capabilityResponse, tpm_nv_data_public,
				    FALSE);	/* do not optimize digestAtRelease */
    }
    return rc;
}

/* Returns a Boolean value.

   TRUE means that the TPM supports the algorithm for TPM_EstablishTransport, TPM_ExecuteTransport
   and TPM_ReleaseTransportSigned.

   FALSE indicates that for these three commands the algorithm is not supported."
*/

static TPM_RESULT TPM_GetCapability_CapTransAlg(TPM_STORE_BUFFER *capabilityResponse,
						TPM_ALGORITHM_ID algorithmID)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	supported;

    printf(" TPM_GetCapability_CapTransAlg: algorithmID %08x\n", algorithmID);
    TPM_TransportPublic_CheckAlgId(&supported, algorithmID);
    printf("  TPM_GetCapability_CapTransAlg: Result %08x\n", supported);
    rc = TPM_Sbuffer_Append(capabilityResponse, &supported, sizeof(TPM_BOOL));
    return rc;
}

/* Returns a TPM_KEY_HANDLE_LIST structure that enumerates all handles currently loaded in the TPM
   for the given resource type.

   TPM_KEY_HANDLE_LIST is the number of handles followed by a list of the handles.

   When describing keys the handle list only contains the number of handles that an external manager
   can operate with and does not include the EK or SRK.

   Legal resources are TPM_RT_KEY, TPM_RT_AUTH, TPM_RT_TRANS, TPM_RT_COUNTER

   TPM_RT_CONTEXT is valid and returns not a list of handles but a list of the context count values.
*/

static TPM_RESULT TPM_GetCapability_CapHandle(TPM_STORE_BUFFER *capabilityResponse,
					      tpm_state_t *tpm_state,
					      TPM_RESOURCE_TYPE resourceType)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_GetCapability_CapHandle: resourceType %08x\n", resourceType);
    switch (resourceType) {
      case TPM_RT_KEY:
	printf("  TPM_GetCapability_CapHandle: TPM_RT_KEY\n");
	rc = TPM_KeyHandleEntries_StoreHandles(capabilityResponse,
					       tpm_state->tpm_key_handle_entries);
	break;
      case TPM_RT_AUTH:
	printf("  TPM_GetCapability_CapHandle: TPM_RT_AUTH\n");
	rc = TPM_AuthSessions_StoreHandles(capabilityResponse,
					   tpm_state->tpm_stclear_data.authSessions);
	break;
      case TPM_RT_TRANS:
	printf("  TPM_GetCapability_CapHandle: TPM_RT_TRANS\n");
	rc = TPM_TransportSessions_StoreHandles(capabilityResponse,
						tpm_state->tpm_stclear_data.transSessions);
	break;
      case TPM_RT_CONTEXT:
	printf("  TPM_GetCapability_CapHandle: TPM_RT_CONTEXT\n");
	rc = TPM_ContextList_StoreHandles(capabilityResponse,
					  tpm_state->tpm_stclear_data.contextList);
	break;
      case TPM_RT_COUNTER:
	printf("  TPM_GetCapability_CapHandle: TPM_RT_COUNTER\n");
	rc = TPM_Counters_StoreHandles(capabilityResponse,
				       tpm_state->tpm_permanent_data.monotonicCounter);
	break;
      case TPM_RT_DAA_TPM:
	printf("  TPM_GetCapability_CapHandle: TPM_RT_DAA_TPM\n");
	rc = TPM_DaaSessions_StoreHandles(capabilityResponse,
					  tpm_state->tpm_stclear_data.daaSessions);
	break;
      default:
	printf("TPM_GetCapability_CapHandle: Error, illegal resource type %08x\n",
	       resourceType);
	rc = TPM_BAD_PARAMETER;
    }
    return rc;
}

/* Returns Boolean value.

   TRUE means the TPM supports the encryption scheme in a transport session.
*/

static TPM_RESULT TPM_GetCapability_CapTransEs(TPM_STORE_BUFFER *capabilityResponse,
					       TPM_ENC_SCHEME encScheme)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	supported;

    printf(" TPM_GetCapability_CapTransEs: encScheme %04hx\n", encScheme);
    switch (encScheme) {
	/* supported protocols */
      case TPM_ES_SYM_CTR:
      case TPM_ES_SYM_OFB:
	supported = TRUE;
	break;
	/* unsupported protocols */
      case TPM_ES_RSAESPKCSv15:
      case TPM_ES_RSAESOAEP_SHA1_MGF1:
      default:
	supported = FALSE;
	break;
    }	
    printf("  TPM_GetCapability_CapTransEs: Result %08x\n", supported);
    rc = TPM_Sbuffer_Append(capabilityResponse, &supported, sizeof(TPM_BOOL));
    return rc;
}

/* Boolean value.

   TRUE indicates that the TPM supports the encryption algorithm in OSAP encryption of AuthData
   values
*/

static TPM_RESULT TPM_GetCapability_CapAuthEncrypt(TPM_STORE_BUFFER *capabilityResponse,
						   TPM_ALGORITHM_ID algorithmID)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	supported;

    printf(" TPM_GetCapability_CapAuthEncrypt: algorithmID %08x\n", algorithmID);
    switch (algorithmID) {
      case TPM_ALG_XOR:
      case TPM_ALG_AES128:
	/* supported protocols */
	supported = TRUE;
	break;
      case TPM_ALG_RSA:
      case TPM_ALG_SHA:
      case TPM_ALG_HMAC:
      case TPM_ALG_MGF1:
      case TPM_ALG_AES192:
      case TPM_ALG_AES256:
      default:
	/* unsupported protocols */
	supported = FALSE;
	break;
    }	
    printf("  TPM_GetCapability_CapAuthEncrypt: Result %08x\n", supported);
    rc = TPM_Sbuffer_Append(capabilityResponse, &supported, sizeof(TPM_BOOL));
    return rc;
}

/* Boolean value.

   TRUE indicates that the TPM supports the size for the given version.

   For instance a request could ask for version 1.1 size 2 and the TPM would indicate TRUE. For 1.1
   size 3 the TPM would indicate FALSE. For 1.2 size 3 the TPM would indicate TRUE.
*/

static TPM_RESULT TPM_GetCapability_CapSelectSize(TPM_STORE_BUFFER *capabilityResponse,
						  TPM_SIZED_BUFFER *subCap)
{
    TPM_RESULT		rc = 0;
    TPM_SELECT_SIZE	tpm_select_size;
    unsigned char	*stream;
    uint32_t		stream_size;
    TPM_BOOL		supported;

    printf(" TPM_GetCapability_CapSelectSize:\n");
    TPM_SelectSize_Init(&tpm_select_size);		/* no free required */
    /* deserialize the subCap to the structure */
    if (rc == 0) {
	stream = subCap->buffer;
	stream_size = subCap->size;
	rc = TPM_SelectSize_Load(&tpm_select_size, &stream , &stream_size);
    }
    if (rc == 0) {
	/* The TPM MUST return an error if sizeOfSelect is 0 */
	printf("  TPM_GetCapability_CapSelectSize: subCap reqSize %u\n",
	       tpm_select_size.reqSize);
	if ((tpm_select_size.reqSize > (TPM_NUM_PCR/CHAR_BIT)) ||
	    (tpm_select_size.reqSize == 0)) {
	    supported = FALSE;
	}
	else {
	    supported = TRUE;
	}
    }
    if (rc == 0) {
	printf("  TPM_GetCapability_CapSelectSize: Result %08x\n", supported);
	rc = TPM_Sbuffer_Append(capabilityResponse, &supported, sizeof(TPM_BOOL));
    }
    return rc;
}

#if  (TPM_REVISION >= 103)	/* added for rev 103 */
/* TPM_GetCapability_CapDaLogic() rev 100

   A TPM_DA_INFO or TPM_DA_INFO_LIMITED structure that returns data according to the selected entity
   type (e.g., TPM_ET_KEYHANDLE, TPM_ET_OWNER, TPM_ET_SRK, TPM_ET_COUNTER, TPM_ET_OPERATOR,
   etc.). If the implemented dictionary attack logic does not support different secret types, the
   entity type can be ignored.
*/

static TPM_RESULT TPM_GetCapability_CapDaLogic(TPM_STORE_BUFFER *capabilityResponse,
					       TPM_SIZED_BUFFER *subCap,
					       tpm_state_t *tpm_state)
{
    TPM_RESULT		rc = 0;
    TPM_DA_INFO_LIMITED tpm_da_info_limited;
    TPM_DA_INFO		tpm_da_info;

    printf(" TPM_GetCapability_CapDaLogic:\n");
    TPM_DaInfoLimited_Init(&tpm_da_info_limited);	/* freed @1 */
    TPM_DaInfo_Init(&tpm_da_info);			/* freed @2 */
    subCap = subCap;			/* dictionary attack mitigation not per entity type in this
					   implementation. */
    /* if disableFullDALogicInfo is TRUE, the full dictionary attack TPM_GetCapability info is
       deactivated.  The returned structure is TPM_DA_INFO_LIMITED. */
    if (tpm_state->tpm_permanent_flags.disableFullDALogicInfo) {
	TPM_DaInfoLimited_Set(&tpm_da_info_limited, tpm_state);
	rc = TPM_DaInfoLimited_Store(capabilityResponse, &tpm_da_info_limited);
	
    }
    /* if disableFullDALogicInfo is FALSE, the full dictionary attack TPM_GetCapability 
       info is activated.  The returned structure is 
       TPM_DA_INFO. */
    else {
	TPM_DaInfo_Set(&tpm_da_info, tpm_state);
	rc = TPM_DaInfo_Store(capabilityResponse, &tpm_da_info);
    }
    TPM_DaInfoLimited_Delete(&tpm_da_info_limited);	/* @1 */
    TPM_DaInfo_Delete(&tpm_da_info);			/* @2 */
    return rc;
}
#endif

/* Returns TPM_CAP_VERSION_INFO structure.

   The TPM fills in the structure and returns the information indicating what the TPM currently
   supports.
*/

static TPM_RESULT TPM_GetCapability_CapVersionVal(TPM_STORE_BUFFER *capabilityResponse,
						  TPM_PERMANENT_DATA *tpm_permanent_data)
{
    TPM_RESULT			rc = 0;
    TPM_CAP_VERSION_INFO	tpm_cap_version_info;

    printf(" TPM_GetCapability_CapVersionVal:\n");
    TPM_CapVersionInfo_Set(&tpm_cap_version_info, tpm_permanent_data);	/* freed @1 */
    printf("  TPM_GetCapability_CapVersionVal: specLevel %04hx\n", tpm_cap_version_info.specLevel);
    printf("  TPM_GetCapability_CapVersionVal: errataRev %02x\n", tpm_cap_version_info.errataRev);
    printf("  TPM_GetCapability_CapVersionVal: revMajor %02x revMinor %02x\n",
	   tpm_cap_version_info.version.revMajor, tpm_cap_version_info.version.revMinor);
    printf("  TPM_GetCapability_CapVersionVal: tpmVendorID %02x %02x %02x %02x\n",
	   tpm_cap_version_info.tpmVendorID[0],
	   tpm_cap_version_info.tpmVendorID[1],
	   tpm_cap_version_info.tpmVendorID[2],
	   tpm_cap_version_info.tpmVendorID[3]);
    rc = TPM_CapVersionInfo_Store(capabilityResponse, &tpm_cap_version_info);
    TPM_CapVersionInfo_Delete(&tpm_cap_version_info);			/* @1 */
    return rc;
}

/* Returns a 4 element array of uint32_t values each denoting the timeout value in microseconds for
   the following in this order:
							 
   TIMEOUT_A, TIMEOUT_B, TIMEOUT_C, TIMEOUT_D

   Where these timeouts are to be used is determined by the platform specific TPM Interface
   Specification.
*/

static TPM_RESULT TPM_GetCapability_CapPropTisTimeout(TPM_STORE_BUFFER *capabilityResponse)
{
    TPM_RESULT			rc = 0;

    printf(" TPM_GetCapability_CapPropTisTimeout:\n");
    if (rc == 0) { 
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_TIMEOUT_A);
    }								   
    if (rc == 0) {						   
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_TIMEOUT_B);
    }								   
    if (rc == 0) {						   
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_TIMEOUT_C);
    }								   
    if (rc == 0) {						   
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_TIMEOUT_D);
    }
    return rc;
}

/* Returns a 3 element array of uint32_t values each denoting the duration value in microseconds of
   the duration of the three classes of commands: Small, Medium and Long in the following in this
   order:

   SMALL_DURATION, MEDIUM_DURATION, LONG_DURATION
*/

static TPM_RESULT TPM_GetCapability_CapPropDuration(TPM_STORE_BUFFER *capabilityResponse)
{
    TPM_RESULT			rc = 0;

    printf(" TPM_GetCapability_CapPropDuration:\n");
    if (rc == 0) { 
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_SMALL_DURATION);
    }								   
    if (rc == 0) {						   
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_MEDIUM_DURATION);
    }								   
    if (rc == 0) {						   
	rc = TPM_Sbuffer_Append32(capabilityResponse, TPM_LONG_DURATION);
    }								   
    return rc;
}

/* 7.3 TPM_GetCapabilityOwner rev 98

   TPM_GetCapabilityOwner enables the TPM Owner to retrieve all the non-volatile flags and the
   volatile flags in a single operation.  This command is deprecated, mandatory.

   The flags summarize many operational aspects of the TPM. The information represented by some
   flags is private to the TPM Owner. So, for simplicity, proof of ownership of the TPM must be
   presented to retrieve the set of flags. When necessary, the flags that are not private to the
   Owner can be deduced by Users via other (more specific) means.
   
   The normal TPM authentication mechanisms are sufficient to prove the integrity of the
   response. No additional integrity check is required.

   For 31>=N>=0

   1. Bit-N of the TPM_PERMANENT_FLAGS structure is the Nth bit after the opening bracket in the
   definition of TPM_PERMANENT_FLAGS in the version of the specification indicated by the parameter
   "version". The bit immediately after the opening bracket is the 0th bit.

   2. Bit-N of the TPM_STCLEAR_FLAGS structure is the Nth bit after the opening bracket in the
   definition of TPM_STCLEAR_FLAGS in the version of the specification indicated by the parameter
   "version". The bit immediately after the opening bracket is the 0th bit.

   3. Bit-N of non_volatile_flags corresponds to the Nth bit in TPM_PERMANENT_FLAGS, and the lsb of
   non_volatile_flags corresponds to bit0 of TPM_PERMANENT_FLAGS

   4. Bit-N of volatile_flags corresponds to the Nth bit in TPM_STCLEAR_FLAGS, and the lsb of
   volatile_flags corresponds to bit0 of TPM_STCLEAR_FLAGS
*/

TPM_RESULT TPM_Process_GetCapabilityOwner(tpm_state_t *tpm_state,
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
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for Owner
					   authentication. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session */
    TPM_AUTHDATA	ownerAuth;	/* The authorization session digest for inputs and owner
					   authentication. HMAC key: ownerAuth. */

    /* processing parameters */
    unsigned char *	inParamStart;	/* starting point of inParam's */
    unsigned char *	inParamEnd;	/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL		authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA *auth_session_data;	/* session data for authHandle */
    TPM_SECRET		*hmacKey;

    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_VERSION		version;		/* A properly filled out version structure. */
    uint32_t		non_volatile_flags;	/* The current state of the non-volatile flags. */
    uint32_t		volatile_flags;		/* The current state of the volatile flags. */

    printf("TPM_Process_GetCapabilityOwner: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
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
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_GetCapabilityOwner: Error, command has %u extra bytes\n",
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
    /* 1. The TPM validates that the TPM Owner authorizes the command. */
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
					*hmacKey,		/* owner HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					ownerAuth);		/* Authorization digest for input */
    }
    /* 2. The TPM creates the parameter non_volatile_flags by setting each bit to the same state as
       the corresponding bit in TPM_PERMANENT_FLAGS. Bits in non_volatile_flags for which there is
       no corresponding bit in TPM_PERMANENT_FLAGS are set to zero. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_PermanentFlags_StoreBitmap(&non_volatile_flags,
						    &(tpm_state->tpm_permanent_flags));
    }	 
    /* 3. The TPM creates the parameter volatile_flags by setting each bit to the same state as the
       corresponding bit in TPM_STCLEAR_FLAGS. Bits in volatile_flags for which there is no
       corresponding bit in TPM_STCLEAR_FLAGS are set to zero. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_StclearFlags_StoreBitmap(&volatile_flags,
						  &(tpm_state->tpm_stclear_flags));
    }	 
    /* 4. The TPM generates the parameter "version". */
    if (returnCode == TPM_SUCCESS) {
	TPM_Version_Set(&version, &(tpm_state->tpm_permanent_data));
    }	 
    /* 5. The TPM returns non_volatile_flags, volatile_flags and version to the caller. */
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_GetCapabilityOwner: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return the version */
	    returnCode = TPM_Version_Store(response, &version);
	}
	/* return the non_volatile_flags */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Sbuffer_Append32(response, non_volatile_flags);
	}
	/* return the volatile_flags */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Sbuffer_Append32(response, volatile_flags);
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
    return rcf;
}

/* 29.1 TPM_GetCapabilitySigned rev 94

   TPM_GetCapabilitySigned is almost the same as TPM_GetCapability. The differences are that the
   input includes a challenge (a nonce) and the response includes a digital signature to vouch for
   the source of the answer.

   If a caller itself requires proof, it is sufficient to use any signing key for which only the TPM
   and the caller have AuthData.

   If a caller requires proof for a third party, the signing key must be one whose signature is
   trusted by the third party. A TPM-identity key may be suitable.
*/

TPM_RESULT TPM_Process_GetCapabilitySigned(tpm_state_t *tpm_state,
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
    TPM_KEY_HANDLE	keyHandle;	/* The handle of a loaded key that can perform digital
					   signatures. */
    TPM_NONCE		antiReplay;	/* Nonce provided to allow caller to defend against replay
					   of messages */
    TPM_CAPABILITY_AREA capArea = 0;	/* Partition of capabilities to be interrogated */
    TPM_SIZED_BUFFER	subCap;		/* Further definition of information */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for keyHandle
					   authorization */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA	privAuth;	/* The authorization session digest that authorizes the use
					   of keyHandle. HMAC key: key.usageAuth */
   
    /* processing parameters */
    unsigned char *	inParamStart;		/* starting point of inParam's */
    unsigned char *	inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt = FALSE;/* wrapped in encrypted transport session */
    TPM_BOOL		authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA *auth_session_data;	/* session data for authHandle */
    TPM_SECRET		*hmacKey;
    TPM_KEY		*sigKey = NULL;		/* the key specified by keyHandle */
    TPM_SECRET		*keyUsageAuth;
    TPM_BOOL		parentPCRStatus;
    uint16_t		subCap16;		/* the subCap as a uint16_t */
    uint32_t		subCap32;		/* the subCap as a uint32_t */
    TPM_STORE_BUFFER	r1Response;		/* capability response */
    const unsigned char *r1_buffer;		/* r1 serialization */
    uint32_t		r1_length;
    TPM_DIGEST		s1;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_VERSION		version;	/* A properly filled out version structure. */
    TPM_SIZED_BUFFER	resp;		/* The capability response */
    TPM_SIZED_BUFFER	sig;		/* The resulting digital signature. */

    printf("TPM_Process_GetCapabilitySigned: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&subCap);	/* freed @1 */
    TPM_SizedBuffer_Init(&resp);	/* freed @2 */
    TPM_SizedBuffer_Init(&sig);		/* freed @3 */
    TPM_Sbuffer_Init(&r1Response);	/* freed @4 */
    /*
      get inputs
    */
    /* get keyHandle parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&keyHandle, &command, &paramSize);
    }
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get antiReplay parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_GetCapabilitySigned: keyHandle %08x\n", keyHandle);
	returnCode = TPM_Nonce_Load(antiReplay, &command, &paramSize);
    }
    /* get capArea parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&capArea, &command, &paramSize);
    }
    /* get get subCap parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&subCap, &command, &paramSize);
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
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_GetCapabilitySigned: Error, command has %u extra bytes\n",
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
    /* 1. The TPM validates the authority to use keyHandle */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)) {
	if (sigKey->authDataUsage != TPM_AUTH_NEVER) {
	    printf("TPM_Process_GetCapabilitySigned: Error, authorization required\n");
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
    

    /* subCap is often a uint16_t or uint32_t, create them now */
    if (returnCode == TPM_SUCCESS) {
	TPM_GetSubCapInt(&subCap16, &subCap32, &subCap);
    }
    /* 2. The TPM calls TPM_GetCapability passing the capArea and subCap fields and saving the resp
       field as R1 */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_GetCapabilityCommon(&r1Response, tpm_state,
					     capArea, subCap16, subCap32, &subCap);
    }	 
    if (returnCode == TPM_SUCCESS) {
	/* get the capability r1 serialization */
	TPM_Sbuffer_Get(&r1Response, &r1_buffer, &r1_length);
	printf("TPM_Process_GetCapabilitySigned: resp length %08x\n", r1_length);
	TPM_PrintFour("TPM_Process_GetCapabilitySigned: Hashing resp", r1_buffer);
	TPM_PrintFour("TPM_Process_GetCapabilitySigned: antiReplay", antiReplay);
	/* 3. The TPM creates S1 by taking a SHA1 hash of the concatenation (r1 || antiReplay).	 */
	returnCode = TPM_SHA1(s1,
			      r1_length, r1_buffer,
			      TPM_NONCE_SIZE, antiReplay,
			      0, NULL);
    }	 
    /* 4. The TPM validates the authority to use keyHandle */
    /* The key in keyHandle MUST have a KEYUSAGE value of type TPM_KEY_SIGNING or TPM_KEY_LEGACY or
       TPM_KEY_IDENTITY. */
    if (returnCode == TPM_SUCCESS) {
	if ((sigKey->keyUsage != TPM_KEY_SIGNING) &&
	    ((sigKey->keyUsage) != TPM_KEY_IDENTITY) &&
	    ((sigKey->keyUsage) != TPM_KEY_LEGACY)) {
	    printf("TPM_Process_GetCapabilitySigned: Error, keyUsage %04hx is invalid\n",
		   sigKey->keyUsage);
	    returnCode = TPM_INVALID_KEYUSAGE;
	}
    }
    /* 5. The TPM creates a digital signature of S1 using the key in keyHandle and returns the
       result in sig. */
    if (returnCode == TPM_SUCCESS) {
	if (sigKey->algorithmParms.sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) {
	    printf("TPM_Process_GetCapabilitySigned: Error, inappropriate signature scheme %04x\n",
		   sigKey->algorithmParms.sigScheme);
	    returnCode = TPM_INAPPROPRIATE_SIG;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_GetCapabilitySigned: Signing s1", s1);
	returnCode = TPM_RSASignToSizedBuffer(&sig,		/* signature */
					      s1,		/* message */
					      TPM_DIGEST_SIZE,	/* message size */
					      sigKey);		/* signing key and parameters */
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_GetCapabilitySigned: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return the version */
	    TPM_Version_Set(&version, &(tpm_state->tpm_permanent_data));
	    returnCode = TPM_Version_Store(response, &version);
	}
	/* return the capability response size */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Sbuffer_Append32(response, r1_length);
	}
	/* return the capability response */
	if (returnCode == TPM_SUCCESS) {
	    returnCode = TPM_Sbuffer_Append(response, r1_buffer, r1_length);
	}
	/* return the signature */
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
    TPM_SizedBuffer_Delete(&subCap);	/* @1 */
    TPM_SizedBuffer_Delete(&resp);	/* @2 */
    TPM_SizedBuffer_Delete(&sig);	/* @3 */
    TPM_Sbuffer_Delete(&r1Response);	/* @4 */
    return rcf;
}

/* 7.2 TPM_SetCapability rev 96

   This command sets values in the TPM
*/

TPM_RESULT TPM_Process_SetCapability(tpm_state_t *tpm_state,
				     TPM_STORE_BUFFER *response,
				     TPM_TAG tag,
				     uint32_t paramSize,
				     TPM_COMMAND_CODE ordinal,
				     unsigned char *command,
				     TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;		/* fatal error precluding response */
    TPM_TAG	returnCode = 0;		/* command return code */

    /* input parameters */
    TPM_CAPABILITY_AREA capArea;	/* Partition of capabilities to be set */
    TPM_SIZED_BUFFER	subCap;		/* Further definition of information */
    TPM_SIZED_BUFFER	setValue;	/* The value to set */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for owner
					   authentication. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL		continueAuthSession = TRUE;	/* The continue use flag for the
							   authorization session handle */
    TPM_AUTHDATA	ownerAuth;	/* Authorization. HMAC key: owner.usageAuth */

    /* processing parameters */
    unsigned char *	inParamStart;		/* starting point of inParam's */
    unsigned char *	inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt = FALSE;/* wrapped in encrypted transport session */
    TPM_BOOL		authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA *auth_session_data;	/* session data for authHandle */
    TPM_SECRET		*hmacKey;
    uint16_t		subCap16;		/* the subCap as a uint16_t */
    uint32_t		subCap32;		/* the subCap as a uint32_t */
    TPM_BOOL		ownerAuthorized = FALSE;	/* TRUE if owner authorization validated */
    TPM_BOOL		presenceAuthorized = FALSE;	/* TRUE if physicalPresence validated */

    /* output parameters */
    uint32_t		outParamStart;		/* starting point of outParam's */
    uint32_t		outParamEnd;		/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_SetCapability: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&subCap);	/* freed @1 */
    TPM_SizedBuffer_Init(&setValue);	/* freed @2 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get capArea parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Load32(&capArea, &command, &paramSize);
    }
    /* get subCap parameter */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SetCapability: capArea %08x \n", capArea);
	returnCode = TPM_SizedBuffer_Load(&subCap, &command, &paramSize);
    }
    /* get setValue parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Load(&setValue , &command, &paramSize);
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
	returnCode = TPM_CheckRequestTag10(tag);
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	ownerAuthorized = TRUE;
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_SetCapability: Error, command has %u extra bytes\n",
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
    /* 1. If tag = TPM_TAG_RQU_AUTH1_COMMAND, validate the command and parameters using ownerAuth,
       return TPM_AUTHFAIL on error */
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
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* owner HMAC key */
					inParamDigest,	
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					ownerAuth);		/* Authorization digest for input */
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Global_GetPhysicalPresence(&presenceAuthorized, tpm_state);
    }
    /* 2. The TPM validates the capArea and subCap indicators, including the ability to set value
       based on any set restrictions */
    /* 3. If the capArea and subCap indicators conform with one of the entries in the structure
       TPM_CAPABILITY_AREA (Values for TPM_SetCapability) */
    /* a. The TPM sets the relevant flag/data to the value of setValue parameter.  */
    /* 4. Else */
    /* a. Return the error code TPM_BAD_PARAMETER. */
    if (returnCode == TPM_SUCCESS) {
	/* subCap is often a uint16_t or uint32_t, create them now */
	TPM_GetSubCapInt(&subCap16, &subCap32, &subCap);
	returnCode = TPM_SetCapabilityCommon(tpm_state, ownerAuthorized, presenceAuthorized,
					     capArea, subCap16, subCap32, &subCap,
					     &setValue);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SetCapability: Ordinal returnCode %08x %u\n",
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
    /* if there was an error, terminate the session. */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    /*
      cleanup
    */
    TPM_SizedBuffer_Delete(&subCap);	/* @1 */
    TPM_SizedBuffer_Delete(&setValue);	/* @2 */
    return rcf;
}

/* TPM_SetCapabilityCommon() is common code for setting a capability from setValue
   
   NOTE: This function assumes that the caller has validated either owner authorization or physical
   presence!
*/

TPM_RESULT TPM_SetCapabilityCommon(tpm_state_t *tpm_state,
				   TPM_BOOL ownerAuthorized,
				   TPM_BOOL presenceAuthorized,
				   TPM_CAPABILITY_AREA capArea, 
				   uint16_t subCap16, 
				   uint32_t subCap32,
				   TPM_SIZED_BUFFER *subCap,
				   TPM_SIZED_BUFFER *setValue)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	valueBool;
    uint32_t	valueUint32 = 0;	/* start with illegal value */
    
    printf(" TPM_SetCapabilityCommon:\n");
    subCap16 = subCap16;			/* not used */
    subCap = subCap;				/* not used */
    if (rc == 0) {
	if ((capArea == TPM_SET_PERM_FLAGS) ||
	    (capArea == TPM_SET_STCLEAR_FLAGS) ||
	    (capArea == TPM_SET_STANY_FLAGS)) {
	    rc = TPM_SizedBuffer_GetBool(&valueBool, setValue);
	}
	else if (((capArea == TPM_SET_PERM_DATA) && (subCap32 != TPM_PD_DAAPROOF)) ||
		 (capArea == TPM_SET_STCLEAR_DATA)) {	/* deferredPhysicalPresence */
	    rc = TPM_SizedBuffer_GetUint32(&valueUint32, setValue);
	}
    }
    if (rc == 0) {
	switch (capArea) {
	  case TPM_SET_PERM_FLAGS:
	    rc = TPM_SetCapability_CapPermFlags(tpm_state, ownerAuthorized, presenceAuthorized,
						subCap32, valueBool);
	    break;
	  case TPM_SET_PERM_DATA:
	    rc = TPM_SetCapability_CapPermData(tpm_state, ownerAuthorized, presenceAuthorized,
					       subCap32, valueUint32);
	    break;
	  case TPM_SET_STCLEAR_FLAGS:
	    rc = TPM_SetCapability_CapStclearFlags(tpm_state, ownerAuthorized, presenceAuthorized,
						   subCap32, valueBool);
	    break;
	  case TPM_SET_STCLEAR_DATA:
	    rc = TPM_SetCapability_CapStclearData(tpm_state, ownerAuthorized, presenceAuthorized,
						  subCap32, valueUint32);
	    break;
	  case TPM_SET_STANY_FLAGS:
	    rc = TPM_SetCapability_CapStanyFlags(tpm_state, ownerAuthorized, presenceAuthorized,
						 subCap32, valueBool);
	    break;
	  case TPM_SET_STANY_DATA:
	    rc = TPM_SetCapability_CapStanyData(tpm_state, ownerAuthorized, presenceAuthorized,
						subCap32, setValue);
	    break;
	  case TPM_SET_VENDOR:
	    rc = TPM_SetCapability_CapVendor(tpm_state, ownerAuthorized, presenceAuthorized,
					     subCap32, setValue);
	    break;
	  default:
	    printf("TPM_SetCapabilityCommon: Error, unsupported capArea %08x", capArea);
	    rc = TPM_BAD_MODE;
	    break;
	}
    }
    return rc;
}

/* TPM_SetCapability_Flag() tests if the values are not already equal.	If they are not, 'flag' is
   set to 'value' and 'altered' is set TRUE.  Otherwise 'altered' is returned unchanged.

   The 'altered' flag is used by the caller to determine if an NVRAM write is required.
*/

void TPM_SetCapability_Flag(TPM_BOOL *altered,
			    TPM_BOOL *flag,
			    TPM_BOOL value)
{
    /* If the values are not already equal.  Can't use != since there are many values for TRUE. */
    if ((value && !*flag) ||
	(!value && *flag)) {
	*altered = TRUE;
	*flag = value;
    }
    return;
}

/* TPM_SetCapability_CapPermFlags() rev 100

   Sets TPM_PERMANENT_FLAGS values
*/
     
static TPM_RESULT TPM_SetCapability_CapPermFlags(tpm_state_t *tpm_state,
						 TPM_BOOL ownerAuthorized,
						 TPM_BOOL presenceAuthorized,
						 uint32_t subCap32,
						 TPM_BOOL valueBool)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	altered = FALSE;	/* TRUE if the structure has been changed */
    
    printf(" TPM_SetCapability_CapPermFlags: valueBool %02x\n", valueBool);
    if (rc == 0) {						   
	switch (subCap32) {
	  case TPM_PF_DISABLE:
	    printf("  TPM_SetCapability_CapPermFlags: TPM_PF_DISABLE\n");
	    /* Owner authorization or physical presence
	       TPM_OwnerSetDisable
	       TPM_PhysicalEnable
	       TPM_PhysicalDisable
	    */
	    if (rc == 0) {
		if (!ownerAuthorized && !presenceAuthorized) {
		    printf("TPM_SetCapability_CapPermFlags: Error, no authorization\n");
		    rc = TPM_AUTHFAIL;
		}
	    }
	    if (rc == 0) {
		TPM_SetCapability_Flag(&altered,
				       &(tpm_state->tpm_permanent_flags.disable),
				       valueBool);
	    }
	    break;
	  case TPM_PF_OWNERSHIP:
	    printf("  TPM_SetCapability_CapPermFlags: TPM_PF_OWNERSHIP\n");
	    /* No authorization. No ownerInstalled. Physical presence asserted
	       Not available when TPM deactivated or disabled
	       TPM_SetOwnerInstall
	    */
	    if (rc == 0) {
		if (tpm_state->tpm_permanent_data.ownerInstalled) {
		    printf("TPM_SetCapability_CapPermFlags: Error, owner installed\n");
		    rc = TPM_OWNER_SET;
		}
	    }
	    if (rc == 0) {
		if (!presenceAuthorized) {
		    printf("TPM_SetCapability_CapPermFlags: Error, no physicalPresence\n");
		    rc = TPM_AUTHFAIL;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_permanent_flags.disable) {
		    printf("TPM_SetCapability_CapPermFlags: Error, disabled\n");
		    rc = TPM_DISABLED;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_stclear_flags.deactivated) {
		    printf("TPM_SetCapability_CapPermFlags: Error, deactivated\n");
		    rc = TPM_DEACTIVATED;
		}
	    }
	    if (rc == 0) {
		TPM_SetCapability_Flag(&altered,
				       &(tpm_state->tpm_permanent_flags.ownership),
				       valueBool);
	    }
	    break;
	  case TPM_PF_DEACTIVATED:
	    printf("  TPM_SetCapability_CapPermFlags: TPM_PF_DEACTIVATED\n");
	    /* No authorization, physical presence assertion
	       Not available when TPM disabled
	       TPM_PhysicalSetDeactivated
	    */
	    if (rc == 0) {
		if (!presenceAuthorized) {
		    printf("TPM_SetCapability_CapPermFlags: Error, no physicalPresence\n");
		    rc = TPM_AUTHFAIL;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_permanent_flags.disable) {
		    printf("TPM_SetCapability_CapPermFlags: Error, disabled\n");
		    rc = TPM_DISABLED;
		}
	    }
	    if (rc == 0) {
		TPM_SetCapability_Flag(&altered,
				       &(tpm_state->tpm_permanent_flags.deactivated),
				       valueBool);
	    }
	    break;
	  case TPM_PF_READPUBEK:
	    printf("  TPM_SetCapability_CapPermFlags: TPM_PF_READPUBEK\n");
	    /* Owner authorization
	       Not available when TPM deactivated or disabled
	    */
	    if (rc == 0) {
		if (!ownerAuthorized) {
		    printf("TPM_SetCapability_CapPermFlags: Error, not owner authorized\n");
		    rc = TPM_AUTHFAIL;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_permanent_flags.disable) {
		    printf("TPM_SetCapability_CapPermFlags: Error, disabled\n");
		    rc = TPM_DISABLED;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_stclear_flags.deactivated) {
		    printf("TPM_SetCapability_CapPermFlags: Error, deactivated\n");
		    rc = TPM_DEACTIVATED;
		}
	    }
	    if (rc == 0) {
		TPM_SetCapability_Flag(&altered,
				       &(tpm_state->tpm_permanent_flags.readPubek),
				       valueBool);
	    }
	    if (rc == 0) {
		printf("  TPM_SetCapability_CapPermFlags : readPubek %02x\n",
		       tpm_state->tpm_permanent_flags.readPubek);
	    }
	    break;
	  case TPM_PF_DISABLEOWNERCLEAR:
	    printf("  TPM_SetCapability_CapPermFlags: TPM_PF_DISABLEOWNERCLEAR\n");
	    /* Owner authorization. Can only set to TRUE, FALSE invalid value. 
	       After being set only ForceClear resets back to FALSE.
	       Not available when TPM deactivated or disabled
	       TPM_DisableOwnerClear */
	    if (rc == 0) {
		if (!ownerAuthorized) {
		    printf("TPM_SetCapability_CapPermFlags: Error, not owner authorized\n");
		    rc = TPM_AUTHFAIL;
		}
	    }
	    if (rc == 0) {
		if (!valueBool) {
		    printf("TPM_SetCapability_CapPermFlags: Error, cannot set FALSE\n");
		    rc = TPM_BAD_PARAMETER;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_permanent_flags.disable) {
		    printf("TPM_SetCapability_CapPermFlags: Error, disabled\n");
		    rc = TPM_DISABLED;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_stclear_flags.deactivated) {
		    printf("TPM_SetCapability_CapPermFlags: Error, deactivated\n");
		    rc = TPM_DEACTIVATED;
		}
	    }
	    if (rc == 0) {
		TPM_SetCapability_Flag(&altered,
				       &(tpm_state->tpm_permanent_flags.disableOwnerClear),
				       valueBool);
	    }
	    break;
	  case TPM_PF_ALLOWMAINTENANCE:
	    printf("  TPM_SetCapability_CapPermFlags: TPM_PF_ALLOWMAINTENANCE\n");
	    /* Owner authorization. Can only set to FALSE, TRUE invalid value. 
	       After being set only changing TPM owner resets back to TRUE
	       Not available when TPM deactivated or disabled
	       TPM_KillMaintenanceFeature
	    */
	    if (rc == 0) {
		if (!ownerAuthorized) {
		    printf("TPM_SetCapability_CapPermFlags: Error, not owner authorized\n");
		    rc = TPM_AUTHFAIL;
		}
	    }
	    if (rc == 0) {
		if (valueBool) {
		    printf("TPM_SetCapability_CapPermFlags: Error, cannot set TRUE\n");
		    rc = TPM_BAD_PARAMETER;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_permanent_flags.disable) {
		    printf("TPM_SetCapability_CapPermFlags: Error, disabled\n");
		    rc = TPM_DISABLED;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_stclear_flags.deactivated) {
		    printf("TPM_SetCapability_CapPermFlags: Error, deactivated\n");
		    rc = TPM_DEACTIVATED;
		}
	    }
	    if (rc == 0) {
		TPM_SetCapability_Flag(&altered,
				       &(tpm_state->tpm_permanent_flags.allowMaintenance),
				       valueBool);
	    }
	    break;
	  case TPM_PF_READSRKPUB:
	    printf("  TPM_SetCapability_CapPermFlags: TPM_PF_READSRKPUB\n");
	    /* Owner Authorization
	       Not available when TPM deactivated or disabled
	       TPM_SetCapability
	    */
	    if (rc == 0) {
		if (!ownerAuthorized) {
		    printf("TPM_SetCapability_CapPermFlags: Error, not owner authorized\n");
		    rc = TPM_AUTHFAIL;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_permanent_flags.disable) {
		    printf("TPM_SetCapability_CapPermFlags: Error, disable is TRUE\n");
		    rc = TPM_DISABLED;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_stclear_flags.deactivated) {
		    printf("TPM_SetCapability_CapPermFlags: Error, deactivated is TRUE\n");
		    rc = TPM_DEACTIVATED;
		}
	    }
	    if (rc == 0) {
		TPM_SetCapability_Flag(&altered,
				       &(tpm_state->tpm_permanent_flags.readSRKPub),
				       valueBool);
	    }
	    break;
	  case TPM_PF_TPMESTABLISHED:
	    printf("  TPM_SetCapability_CapPermFlags: TPM_PF_TPMESTABLISHED\n");
	    /* Locality 3 or locality 4
	       Can only set to FALSE
	       TPM_ResetEstablishmentBit
	    */
	    if (rc == 0) {
		rc = TPM_Locality_Check(TPM_LOC_THREE | TPM_LOC_FOUR,  /* BYTE bitmap */
					tpm_state->tpm_stany_flags.localityModifier);
	    }
	    if (rc == 0) {
		if (valueBool) {
		    printf("TPM_SetCapability_CapPermFlags: Error, can only set to FALSE\n");
		    rc = TPM_BAD_PARAMETER;
		}
	    }
	    if (rc == 0) {
		TPM_SetCapability_Flag(&altered,
				       &(tpm_state->tpm_permanent_flags.tpmEstablished),
				       valueBool);
	    }
	    break;
#if  (TPM_REVISION >= 103)	/* added for rev 103 */
	  case TPM_PF_DISABLEFULLDALOGICINFO:
	    /* Owner Authorization
	       TPM_SetCapability
	    */
	    printf("  TPM_SetCapability_CapPermFlags: TPM_PF_DISABLEFULLDALOGICINFO\n");
	    if (rc == 0) {
		if (!ownerAuthorized) {
		    printf("TPM_SetCapability_CapPermFlags: Error, not owner authorized\n");
		    rc = TPM_AUTHFAIL;
		}
	    }
	    if (rc == 0) {
		TPM_SetCapability_Flag(&altered,
				       &(tpm_state->tpm_permanent_flags.disableFullDALogicInfo),
				       valueBool);
	    }
	    break;
#endif
	  case TPM_PF_PHYSICALPRESENCELIFETIMELOCK:
	  case TPM_PF_PHYSICALPRESENCEHWENABLE:
	  case TPM_PF_PHYSICALPRESENCECMDENABLE:
	  case TPM_PF_CEKPUSED:
	  case TPM_PF_TPMPOST:
	  case TPM_PF_TPMPOSTLOCK:
	  case TPM_PF_FIPS:
	  case TPM_PF_OPERATOR:
	  case TPM_PF_ENABLEREVOKEEK:
	  case TPM_PF_NV_LOCKED:
	  case TPM_PF_MAINTENANCEDONE:
	  default:
	    printf("TPM_SetCapability_CapPermFlags: Error, bad subCap32 %u\n",
		   subCap32);
	    rc = TPM_BAD_PARAMETER;
	}
    }
    rc = TPM_PermanentAll_NVStore(tpm_state,
				  altered,
				  rc);
    return rc;
}

/* TPM_SetCapability_CapPermData() rev 105

   Sets TPM_PERMANENT_DATA values
*/
     
static TPM_RESULT TPM_SetCapability_CapPermData(tpm_state_t *tpm_state,
						TPM_BOOL ownerAuthorized,
						TPM_BOOL presenceAuthorized,
						uint32_t subCap32,
						uint32_t valueUint32)
{
    TPM_RESULT	rc = 0;
    TPM_BOOL	writeAllNV = FALSE;	/* TRUE if the structure has been changed */
    
    printf(" TPM_SetCapability_CapPermData:\n");
    presenceAuthorized = presenceAuthorized;			/* not used */
    if (rc == 0) {						   
	switch (subCap32) {
	  case TPM_PD_RESTRICTDELEGATE:
	    printf("  TPM_SetCapability_CapPermData: TPM_PD_RESTRICTDELEGATE\n");
	    /* Owner authorization.  Not available when TPM deactivated or disabled */
	    /* TPM_CMK_SetRestrictions */
	    if (rc == 0) {
		if (!ownerAuthorized) {
		    printf("TPM_SetCapability_CapPermData: Error, not owner authorized\n");
		    rc = TPM_AUTHFAIL;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_permanent_flags.disable) {
		    printf("TPM_SetCapability_CapPermData: Error, disabled\n");
		    rc = TPM_DISABLED;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_stclear_flags.deactivated) {
		    printf("TPM_SetCapability_CapPermData: Error, deactivated\n");
		    rc = TPM_DEACTIVATED;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_permanent_data.restrictDelegate != valueUint32) {
		    tpm_state->tpm_permanent_data.restrictDelegate = valueUint32;
		    writeAllNV = TRUE;
		}
	    }
	    break;
	  case TPM_PD_DAAPROOF:
	    /* TPM_PD_DAAPROOF This capability has no value.  When specified by TPM_SetCapability, a
	       new daaProof, tpmDAASeed, and daaBlobKey are generated. */
		rc = TPM_PermanentData_InitDaa(&(tpm_state->tpm_permanent_data));
		writeAllNV = TRUE;
	    break;
	  case TPM_PD_REVMAJOR:
	  case TPM_PD_REVMINOR:
	  case TPM_PD_TPMPROOF:
	  case TPM_PD_OWNERAUTH:
	  case TPM_PD_OPERATORAUTH:
	  case TPM_PD_MANUMAINTPUB:
	  case TPM_PD_ENDORSEMENTKEY:
	  case TPM_PD_SRK:
	  case TPM_PD_DELEGATEKEY:
	  case TPM_PD_CONTEXTKEY:
	  case TPM_PD_AUDITMONOTONICCOUNTER:
	  case TPM_PD_MONOTONICCOUNTER:
	  case TPM_PD_PCRATTRIB:
	  case TPM_PD_ORDINALAUDITSTATUS:
	  case TPM_PD_AUTHDIR:
	  case TPM_PD_RNGSTATE:
	  case TPM_PD_FAMILYTABLE:
	  case TPM_DELEGATETABLE:
	  case TPM_PD_EKRESET:
	  case TPM_PD_LASTFAMILYID:
	  case TPM_PD_NOOWNERNVWRITE:
	  case TPM_PD_TPMDAASEED:
	  default:
	    printf("TPM_SetCapability_CapPermData: Error, bad subCap32 %u\n",
		   subCap32);
	    rc = TPM_BAD_PARAMETER;
	}
    }
    rc = TPM_PermanentAll_NVStore(tpm_state,
				  writeAllNV,
				  rc);
    return rc;
}

/* TPM_SetCapability_CapStclearFlags() rev 85

   Sets TPM_STCLEAR_FLAGS values
*/
     
static TPM_RESULT TPM_SetCapability_CapStclearFlags(tpm_state_t *tpm_state,
						    TPM_BOOL ownerAuthorized,
						    TPM_BOOL presenceAuthorized,
						    uint32_t subCap32,
						    TPM_BOOL valueBool)
{
    TPM_RESULT			rc = 0;

    printf(" TPM_SetCapability_CapStclearFlags: valueBool %02x\n", valueBool);
    ownerAuthorized = ownerAuthorized;		/* not used */
    presenceAuthorized = presenceAuthorized;	/* not used */
    if (rc == 0) {						   
	switch (subCap32) {
	  case TPM_SF_DISABLEFORCECLEAR:
	    printf("  TPM_SetCapability_CapStclearFlags: TPM_SF_DISABLEFORCECLEAR\n");
	    /* Not available when TPM deactivated or disabled */
	    /* TPM_DisableForceClear */
	    if (rc == 0) {
		if (tpm_state->tpm_permanent_flags.disable) {
		    printf("TPM_SetCapability_CapStclearFlags: Error, disabled\n");
		    rc = TPM_DISABLED;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_stclear_flags.deactivated) {
		    printf("TPM_SetCapability_CapStclearFlags: Error, deactivated\n");
		    rc = TPM_DEACTIVATED;
		}
	    }
	    /* Can only set to TRUE */
	    if (rc == 0) {
		if (!valueBool) {
		    printf("TPM_SetCapability_CapStclearFlags: Error, cannot set FALSE\n");
		    rc = TPM_BAD_PARAMETER;
		}
	    }
	    if (rc == 0) {
		tpm_state->tpm_stclear_flags.disableForceClear = TRUE;
	    }
	    break;
	  case TPM_SF_DEACTIVATED:
	  case TPM_SF_PHYSICALPRESENCE:
	  case TPM_SF_PHYSICALPRESENCELOCK:
	  case TPM_SF_BGLOBALLOCK:
	  default:
	    printf("TPM_SetCapability_CapStclearFlags: Error, bad subCap32 %u\n",
		   subCap32);
	    rc = TPM_BAD_PARAMETER;
	}
    }
    return rc;
}

/* TPM_SetCapability_CapStclearData() rev 100

   Sets TPM_STCLEAR_DATA values
*/
     
static TPM_RESULT TPM_SetCapability_CapStclearData(tpm_state_t *tpm_state,
						   TPM_BOOL ownerAuthorized,
						   TPM_BOOL presenceAuthorized,
						   uint32_t subCap32,
						   uint32_t valueUint32)
{
    TPM_RESULT			rc = 0;
#if  (TPM_REVISION < 103)	/* added for rev 103 */
    tpm_state = tpm_state;	/* to quiet the compiler */
    presenceAuthorized = presenceAuthorized;
    valueUint32 = valueUint32;
#endif

    printf(" TPM_SetCapability_CapStclearData:\n");
    ownerAuthorized = ownerAuthorized;		/* not used */
    if (rc == 0) {						   
	switch (subCap32) {
#if  (TPM_REVISION >= 103)	/* added for rev 103 */
	  case TPM_SD_DEFERREDPHYSICALPRESENCE:
	    printf("  TPM_SetCapability_CapStclearData: TPM_SD_DEFERREDPHYSICALPRESENCE\n");
	    /* Can only set to TRUE if PhysicalPresence is asserted.  Can set to FALSE at any
	       time. */
	    /* 1. If physical presence is not asserted */
	    /* a. If TPM_SetCapability -> setValue has a bit set that is not already set in
	       TPM_STCLEAR_DATA -> deferredPhysicalPresence, return TPM_BAD_PRESENCE. */
	    if (rc == 0) {
		if (!presenceAuthorized) {
		    if (~(tpm_state->tpm_stclear_data.deferredPhysicalPresence) & valueUint32) {
			printf("TPM_SetCapability_CapStclearData: "
			       "Error, no physicalPresence and deferredPhysicalPresence %08x\n",
			       tpm_state->tpm_stclear_data.deferredPhysicalPresence);
			rc = TPM_BAD_PRESENCE;
		    }
		}
	    }
	    /* 2.Set TPM_STCLEAR_DATA -> deferredPhysicalPresence to TPM_SetCapability -> setValue.
	    */
	    if (rc == 0) {
		printf("   TPM_SetCapability_CapStclearData: deferredPhysicalPresence now %08x\n",
		       valueUint32);
		tpm_state->tpm_stclear_data.deferredPhysicalPresence = valueUint32;
	    }
	    break;
#endif
	  case TPM_SD_CONTEXTNONCEKEY:
	  case TPM_SD_COUNTID:
	  case TPM_SD_OWNERREFERENCE:
	  case TPM_SD_DISABLERESETLOCK:
	  case TPM_SD_PCR:
	  default:
	    printf("TPM_SetCapability_CapStclearData: Error, bad subCap32 %u\n",
		   subCap32);
	    rc = TPM_BAD_PARAMETER;
	}
    }
    return rc;
}

/* TPM_SetCapability_CapStanyFlags() rev 85

   Sets TPM_STANY_FLAGS values
*/
     
static TPM_RESULT TPM_SetCapability_CapStanyFlags(tpm_state_t *tpm_state,
						  TPM_BOOL ownerAuthorized,
						  TPM_BOOL presenceAuthorized,
						  uint32_t subCap32,
						  TPM_BOOL valueBool)
{
    TPM_RESULT			rc = 0;

    printf(" TPM_SetCapability_CapStanyFlags:\n");
    ownerAuthorized = ownerAuthorized;			/* not used */
    presenceAuthorized = presenceAuthorized;		/* not used */
    if (rc == 0) {						   
	switch (subCap32) {
	  case TPM_AF_TOSPRESENT:
	    printf("  TPM_SetCapability_CapStanyFlags: TPM_AF_TOSPRESENT\n");
	    /* locality 3 or 4 */
	    /* Not available when TPM deactivated or disabled */
	    if (rc == 0) {
		rc = TPM_Locality_Check(TPM_LOC_THREE | TPM_LOC_FOUR,
					tpm_state->tpm_stany_flags.localityModifier);
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_permanent_flags.disable) {
		    printf("TPM_SetCapability_CapStanyFlags: Error, disabled\n");
		    rc = TPM_DISABLED;
		}
	    }
	    if (rc == 0) {
		if (tpm_state->tpm_stclear_flags.deactivated) {
		    printf("TPM_SetCapability_CapStanyFlags: Error, deactivated\n");
		    rc = TPM_DEACTIVATED;
		}
	    }
	    /* can only be set to FALSE */
	    if (rc == 0) {
		if (valueBool) {
		    printf("TPM_SetCapability_CapStanyFlags: Error, cannot set TRUE\n");
		    rc = TPM_BAD_PARAMETER;
		}
	    }
	    if (rc == 0) {
		tpm_state->tpm_stany_flags.TOSPresent = FALSE;
	    }
	    break;
	  case TPM_AF_POSTINITIALISE:
	  case TPM_AF_LOCALITYMODIFIER:
	  case TPM_AF_TRANSPORTEXCLUSIVE:
	  default:
	    printf("TPM_SetCapability_CapStanyFlags: Error, bad subCap32 %u\n",
		   subCap32);
	    rc = TPM_BAD_PARAMETER;
	}
    }
    return rc;
}

/* TPM_SetCapability_CapStanyData() rev 85

   Sets TPM_STANY_DATA values
*/

static TPM_RESULT TPM_SetCapability_CapStanyData(tpm_state_t *tpm_state,
						 TPM_BOOL ownerAuthorized,
						 TPM_BOOL presenceAuthorized,
						 uint32_t subCap32,
						 TPM_SIZED_BUFFER *setValue)
{
    TPM_RESULT			rc = 0;

    printf(" TPM_SetCapability_CapStanyData:\n");
    tpm_state = tpm_state;			/* not used */
    ownerAuthorized = ownerAuthorized;		/* not used */
    presenceAuthorized = presenceAuthorized;	/* not used */
    setValue = setValue;			/* not used */
    if (rc == 0) {						   
	switch (subCap32) {
	  case TPM_AD_CONTEXTNONCESESSION:
	  case TPM_AD_AUDITDIGEST:
	  case TPM_AD_CURRENTTICKS:
	  case TPM_AD_CONTEXTCOUNT:
	  case TPM_AD_CONTEXTLIST:
	  case TPM_AD_SESSIONS:
	  default:
	    printf("TPM_SetCapability_CapStanyData: Error, bad subCap32 %u\n",
		   subCap32);
	    rc = TPM_BAD_PARAMETER;
	}				
    }
    return rc;
}

/* These are subCaps to TPM_SetCapability -> TPM_SET_VENDOR capArea, the vendor specific area.
*/

static TPM_RESULT TPM_SetCapability_CapVendor(tpm_state_t *tpm_state,
					      TPM_BOOL ownerAuthorized,
					      TPM_BOOL presenceAuthorized,
					      uint32_t subCap32,
					      TPM_SIZED_BUFFER *setValue)
{
    TPM_RESULT			rc = 0;
    
    printf(" TPM_SetCapability_CapVendor:\n");
    ownerAuthorized = ownerAuthorized;		/* not used */
    presenceAuthorized = presenceAuthorized;	/* not used */
    setValue = setValue;
    /* make temporary copies so the setValue is not touched */
    if (rc == 0) {
	switch(subCap32) {
	  default:
	    printf("TPM_SetCapability_CapVendor: Error, unsupported subCap %08x\n", subCap32);
	    tpm_state = tpm_state;			/* not used */
	    rc = TPM_BAD_PARAMETER;
	    break;

	}
    }
    return rc;
}
