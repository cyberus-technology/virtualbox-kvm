/********************************************************************************/
/*										*/
/*				Maintenance Handler				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_maint.c $		*/
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

#if !defined(TPM_NOMAINTENANCE) && !defined(TPM_NOMAINTENANCE_COMMANDS)

#include <stdio.h>
#include <stdlib.h>

#include "tpm_auth.h"
#include "tpm_crypto.h"
#include "tpm_cryptoh.h"
#include "tpm_debug.h"
#include "tpm_digest.h"
#include "tpm_error.h"
#include "tpm_io.h"
#include "tpm_key.h"
#include "tpm_memory.h"
#include "tpm_nonce.h"
#include "tpm_owner.h"
#include "tpm_permanent.h"
#include "tpm_process.h"

#include "tpm_maint.h"

/*
  Processing Functions
*/

/* 12. Maintenance Functions (optional)

   The maintenance mechanisms in the TPM MUST not require the TPM to hold a global secret. The
   definition of global secret is a secret value shared by more than one TPM.

   The TPME is not allowed to pre-store or use unique identifiers in the TPM for the purpose of
   maintenance.	 The TPM MUST NOT use the endorsement key for identification or encryption in the
   maintenance process. The maintenance process MAY use a TPM Identity to deliver maintenance
   information to specific TPM's.

   The maintenance process can only change the SRK, tpmProof and TPM Owner AuthData fields.

   The maintenance process can only access data in shielded locations where this data is necessary
   to validate the TPM Owner, validate the TPME and manipulate the blob

   The TPM MUST be conformant to the TPM specification, protection profiles and security targets
   after maintenance. The maintenance MAY NOT decrease the security values from the original
   security target.

   The security target used to evaluate this TPM MUST include this command in the TOE.
*/

/* When a maintenance archive is created with generateRandom FALSE, the maintenance blob is XOR
   encrypted with the owner authorization before encryption with the maintenance public key. This
   prevents the manufacturer from obtaining plaintext data. The receiving TPM must have the same
   owner authorization as the sending TPM in order to XOR decrypt the archive.
   
   When generateRandom is TRUE, the maintenance blob is XOR encrypted with random data, which is
   also returned. This permits someone trusted by the Owner to load the maintenance archive into the
   replacement platform in the absence of the Owner and manufacturer, without the Owner having to
   reveal information about his auth value. The receiving and sending TPM's may have different owner
   authorizations. The random data is transferred from the sending TPM owner to the receiving TPM
   owner out of band, so the maintenance blob remains hidden from the manufacturer.
   
  This is a typical maintenance sequence:
  1.	Manufacturer:
  -	generates maintenance key pair
  -	gives public key to TPM1 owner
  2.	TPM1: TPM_LoadManuMaintPub
  -	load maintenance public key
  3.	TPM1: TPM_CreateMaintenanceArchive
  -	XOR encrypt with owner auth or random
  -	encrypt with maintenance public key
  4.	Manufacturer:
  -	decrypt with maintenance private key
  -	(still XOR encrypted with owner auth or random)
  -	encrypt with TPM2 SRK public key
  5.	TPM2: TPM_LoadMaintenanceArchive
  -	decrypt with SRK private key
  -	XOR decrypt with owner auth or random
*/

/* 12.1 TPM_CreateMaintenanceArchive rev 101

   This command creates the MaintenanceArchive. It can only be executed by the owner, and may be
   shut off with the TPM_KillMaintenanceFeature command.
*/

TPM_RESULT TPM_Process_CreateMaintenanceArchive(tpm_state_t *tpm_state,
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
    TPM_BOOL		generateRandom; /* Use RNG or Owner auth to generate 'random'. */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for owner
					   authentication. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA	ownerAuth;	/* The authorization session digest for inputs and owner
					   authentication.  HMAC key: ownerAuth.  */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus = FALSE;	/* audit the ordinal */
    TPM_BOOL			transportEncrypt = FALSE;	/* wrapped in encrypted transport
								   session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey = NULL;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    uint32_t			o1Oaep_size;
    BYTE			*o1Oaep;
    BYTE			*r1InnerWrapKey;
    BYTE			*x1InnerWrap;
    TPM_KEY			a1;			/* SRK archive result */
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back flags */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	random;		/* Random data to XOR with result. */
    TPM_STORE_BUFFER	archive;	/* Encrypted key archive. */

    printf("TPM_Process_CreateMaintenanceArchive: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&random);	/* freed @1 */
    TPM_Key_Init(&a1);			/* freed @2 */
    TPM_Sbuffer_Init(&archive);		/* freed @3 */
    o1Oaep = NULL;			/* freed @4 */
    r1InnerWrapKey = NULL;		/* freed @5 */
    x1InnerWrap = NULL;			/* freed @6 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get generateRandom parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_LoadBool(&generateRandom, &command, &paramSize);
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
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_CreateMaintenanceArchive: Error, command has %u extra bytes\n",
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
    /* Upon authorization being confirmed this command does the following: */
    /* 1. Validates that the TPM_PERMANENT_FLAGS -> AllowMaintenance is TRUE. If it is FALSE, the
       TPM SHALL return TPM_DISABLED_CMD and exit this capability. */
    if (returnCode == TPM_SUCCESS) {
	if (!tpm_state->tpm_permanent_flags.allowMaintenance) {
	    printf("TPM_Process_CreateMaintenanceArchive: Error allowMaintenance FALSE\n");
	    returnCode = TPM_DISABLED_CMD;
	}
    }
    /* 2. Validates the TPM Owner AuthData. */
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
    /* 3. If the value of TPM_PERMANENT_DATA -> ManuMaintPub is zero, the TPM MUST return the error
       code TPM_KEYNOTFOUND */
    if (returnCode == TPM_SUCCESS) {
	/* since there is no keyUsage, algorithmID seems like a way to check for an empty key */
	if (tpm_state->tpm_permanent_data.manuMaintPub.algorithmParms.algorithmID != TPM_ALG_RSA) {
	    printf("TPM_Process_CreateMaintenanceArchive: manuMaintPub key not found\n");
	    returnCode = TPM_KEYNOTFOUND;
	}
    }
    /* 4. Build a1 a TPM_KEY structure using the SRK. The encData field is not a normal
       TPM_STORE_ASYMKEY structure but rather a TPM_MIGRATE_ASYMKEY structure built using the
       following actions. */
    if (returnCode == TPM_SUCCESS) {
	TPM_Key_Copy(&a1,
		     &(tpm_state->tpm_permanent_data.srk),
		     FALSE);		/* don't copy encData */
    }
    /* 5. Build a TPM_STORE_PRIVKEY structure from the SRK. This privKey element should be 132 bytes
       long for a 2K RSA key. */
    /* 6. Create k1 and k2 by splitting the privKey element created in step 4 into 2 parts. k1 is
       the first 20 bytes of privKey, k2 contains the remainder of privKey. */
    /* 7. Build m1 by creating and filling in a TPM_MIGRATE_ASYMKEY structure */
    /* a. m1 -> usageAuth is set to TPM_PERMANENT_DATA -> tpmProof */
    /* b. m1 -> pubDataDigest is set to the digest value of the SRK fields from step 4 */
    /* c. m1 -> payload is set to TPM_PT_MAINT */
    /* d. m1 -> partPrivKey is set to k2 */
    /* 8. Create o1 (which SHALL be 198 bytes for a 2048 bit RSA key) by performing the OAEP
       encoding of m using OAEP parameters of */
    /* a. m = TPM_MIGRATE_ASYMKEY structure (step 7) */
    /* b. pHash = TPM_PERMANENT_DATA -> ownerAuth */
    /* c. seed = s1 = k1 (step 6) */
    if (returnCode == TPM_SUCCESS) {
	TPM_StoreAsymkey_GetO1Size(&o1Oaep_size,
				   tpm_state->tpm_permanent_data.srk.tpm_store_asymkey);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Malloc(&o1Oaep, o1Oaep_size);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Malloc(&r1InnerWrapKey, o1Oaep_size);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Malloc(&x1InnerWrap, o1Oaep_size);
    }
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_StoreAsymkey_StoreO1
		     (o1Oaep,
		      o1Oaep_size,
		      tpm_state->tpm_permanent_data.srk.tpm_store_asymkey,
		      tpm_state->tpm_permanent_data.ownerAuth,	/* pHash */
		      TPM_PT_MAINT,				/* TPM_PAYLOAD_TYPE */
		      tpm_state->tpm_permanent_data.tpmProof);	/* usageAuth */
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_CreateMaintenanceArchive: o1 -", o1Oaep);
	/* 9. If generateRandom = TRUE */
	if (generateRandom) {
	    /* a. Create r1 by obtaining values from the TPM RNG. The size of r1 MUST be the same
	       size as o1. */ 
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_Random(r1InnerWrapKey, o1Oaep_size);
	    }
	    /* Set random parameter to r1 */
	    if (returnCode == TPM_SUCCESS) {
		returnCode = TPM_SizedBuffer_Set(&random, o1Oaep_size, r1InnerWrapKey);
	    }
	}
	/* 10. If generateRandom = FALSE */
	else {
	    /* a. Create r1 by applying MGF1 to the TPM Owner AuthData. The size of r1 MUST be the
	       same size as o1. */ 
	    returnCode = TPM_MGF1(r1InnerWrapKey,			/* unsigned char *mask */
				  o1Oaep_size,				/* long len */
				  tpm_state->tpm_permanent_data.ownerAuth,	/* const unsigned
										   char *seed */
				  TPM_SECRET_SIZE);			/* long seedlen */
	    /* Set randomSize to 0. */
	    /* NOTE Done by TPM_SizedBuffer_Init() */
	}
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_CreateMaintenanceArchive: r1 -", r1InnerWrapKey);
	/* 11. Create x1 by XOR of o1 with r1 */
	TPM_XOR(x1InnerWrap, o1Oaep, r1InnerWrapKey, o1Oaep_size);
	TPM_PrintFour("TPM_Process_CreateMaintenanceArchive: x1", x1InnerWrap);
	/* 12. Encrypt x1 with the manuMaintPub key using the TPM_ES_RSAESOAEP_SHA1_MGF1
	   encryption scheme. NOTE The check for OAEP is done by TPM_LoadManuMaintPub */
	/* 13. Set a1 -> encData to the encryption of x1 */
	returnCode = TPM_RSAPublicEncrypt_Pubkey(&(a1.encData),
						 x1InnerWrap,
						 o1Oaep_size,
						 &(tpm_state->tpm_permanent_data.manuMaintPub));
	TPM_PrintFour("TPM_Process_CreateMaintenanceArchive: encData", a1.encData.buffer);
    }
    /* 14. Set TPM_PERMANENT_FLAGS -> maintenanceDone to TRUE */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_CreateMaintenanceArchive: Set maintenanceDone\n");
	TPM_SetCapability_Flag(&writeAllNV,					/* altered */
			       &(tpm_state->tpm_permanent_flags.maintenanceDone),	/* flag */
			       TRUE);						/* value */
    }
    /* Store the permanent flags back to NVRAM */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV,
					  returnCode);
    /* 15. Return a1 in the archive parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Key_Store(&archive, &a1);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_CreateMaintenanceArchive: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return randomSize and random */
	    returnCode = TPM_SizedBuffer_Store(response, &random);
	}
	if (returnCode == TPM_SUCCESS) {
	    /* return archiveSize and archive */
	    returnCode = TPM_Sbuffer_AppendAsSizedBuffer(response, &archive);
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
    TPM_SizedBuffer_Delete(&random);		/* @1 */
    TPM_Key_Delete(&a1);			/* @2 */
    TPM_Sbuffer_Delete(&archive);		/* @3 */
    free(o1Oaep);				/* @4 */
    free(r1InnerWrapKey);			/* @5 */
    free(x1InnerWrap);				/* @6 */
    return rcf;
}

/* 12.2 TPM_LoadMaintenanceArchive rev 98

   This command loads in a Maintenance archive that has been massaged by the manufacturer to load
   into another TPM

   If the maintenance archive was created using the owner authorization for XOR encryption, the
   current owner authorization must be used for decryption. The owner authorization does not change.

   If the maintenance archive was created using random data for the XOR encryption, the vendor
   specific arguments must include the random data. The owner authorization may change.
*/

TPM_RESULT TPM_Process_LoadMaintenanceArchive(tpm_state_t *tpm_state,
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
    
    TPM_SIZED_BUFFER	archive;	/* Vendor specific arguments, from
					   TPM_CreateMaintenanceArchive */
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for owner
					   authentication. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA	ownerAuth;	/* The authorization session digest for inputs and owner
					   authentication.  HMAC key: ownerAuth.*/
    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus = FALSE;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt = FALSE;	/* wrapped in encrypted transport
								   session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_SECRET			saveKey;		/* copy of HMAC key, since key changes */
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    unsigned char		*stream;			/* the input archive stream */
    uint32_t			stream_size;
    BYTE			*x1InnerWrap;
    uint32_t			x1InnerWrap_size;	
    BYTE			*r1InnerWrapKey;	/* for XOR decryption */
    BYTE			*o1Oaep;
    TPM_KEY			newSrk;
    TPM_STORE_ASYMKEY		srk_store_asymkey;
    TPM_STORE_BUFFER		asym_sbuffer;
    TPM_BOOL			writeAllNV1 = FALSE;	/* flags to write back data */
    TPM_BOOL			writeAllNV2 = FALSE;	/* flags to write back NV */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    /* Vendor specific arguments */

    printf("TPM_Process_LoadMaintenanceArchive: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&archive);		/* freed @1 */
    TPM_Key_Init(&newSrk);			/* freed @2 */
    x1InnerWrap = NULL;				/* freed @3 */
    r1InnerWrapKey = NULL;			/* freed @4 */
    o1Oaep = NULL;				/* freed @5 */
    TPM_StoreAsymkey_Init(&srk_store_asymkey);	/* freed @6 */
    TPM_Sbuffer_Init(&asym_sbuffer);		/* freed @7 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get Vendor specific arguments */
    if (returnCode == TPM_SUCCESS) {
	/* NOTE TPM_CreateMaintenanceArchive sends a TPM_SIZED_BUFFER archive. */
	returnCode = TPM_SizedBuffer_Load(&archive, &command, &paramSize);
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
					ownerAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_LoadMaintenanceArchive: Error, command has %u extra bytes\n",
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
    /* 1. Validate the TPM Owner's AuthData */
    /* Upon authorization being confirmed this command does the following: */
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
    /* 2. Validate that the maintenance information was sent by the TPME. The validation mechanism
       MUST use a strength of function that is at least the same strength of function as a digital
       signature performed using a 2048 bit RSA key. */
    /* NOTE SRK is 2048 bits minimum */
    /* 3. The packet MUST contain m2 as defined in Section 12.1 */
    /* The TPM_SIZED_BUFFER archive contains a TPM_KEY with a TPM_MIGRATE_ASYMKEY that will become
       the new SRK */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadMaintenanceArchive: Deserializing TPM_KEY parameter\n");
	stream = archive.buffer;
	stream_size = archive.size;
	returnCode = TPM_Key_Load(&newSrk, &stream, &stream_size);
    }
    /* decrypt the TPM_KEY -> encData to x1 using the current SRK */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadMaintenanceArchive: Decrypting TPM_KEY -> encData with SRK\n");
	returnCode = TPM_RSAPrivateDecryptMalloc(&x1InnerWrap,
						 &x1InnerWrap_size,
						 newSrk.encData.buffer,
						 newSrk.encData.size,
						 &(tpm_state->tpm_permanent_data.srk));
    }
    /* allocate memory for r1 based on x1 XOR encrypted data */
    if (returnCode == TPM_SUCCESS) {
	TPM_PrintFour("TPM_Process_LoadMaintenanceArchive: x1", x1InnerWrap);
	printf("TPM_Process_LoadMaintenanceArchive: x1 size %u\n", x1InnerWrap_size);
	returnCode = TPM_Malloc(&r1InnerWrapKey, x1InnerWrap_size);
    }
    /* allocate memory for o1 based on x1 XOR encrypted data */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Malloc(&o1Oaep, x1InnerWrap_size);
    }
    /* generate the XOR encryption secret from the ownerAuth */
    /* NOTE:  This does not yet support a supplied random number as the inner wrapper key */
    if (returnCode == TPM_SUCCESS) {
	TPM_MGF1(r1InnerWrapKey,				/* unsigned char *mask */
		 x1InnerWrap_size,				/* long len */
		 tpm_state->tpm_permanent_data.ownerAuth,	/* const unsigned char *seed */
		 TPM_SECRET_SIZE);				/* long seedlen */
	TPM_PrintFour("TPM_Process_LoadMaintenanceArchive: r1 -", r1InnerWrapKey);
	/* decrypt x1 to o1 using XOR encryption secret */
	printf("TPM_Process_LoadMaintenanceArchive: XOR Decrypting TPM_KEY SRK parameter\n");
	TPM_XOR(o1Oaep, x1InnerWrap, r1InnerWrapKey, x1InnerWrap_size);
	TPM_PrintFour("TPM_Process_LoadMaintenanceArchive: o1 -", o1Oaep);
    }
    /* convert o1 to TPM_STORE_ASYMKEY */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_StoreAsymkey_LoadO1(&srk_store_asymkey, o1Oaep, x1InnerWrap_size);
    }
    /* TPM1 tpmProof comes in as TPM_STORE_ASYMKEY -> usageAuth */
    /* TPM1 ownerAuth comes in as TPM_STORE_ASYMKEY -> migrationAuth (from pHash) */
    /* 4. Ensure that only the target TPM can interpret the maintenance packet. The protection
       mechanism MUST use a strength of function that is at least the same strength of function as a
       digital signature performed using a 2048 bit RSA key. */
    /* 5. Execute the actions of TPM_OwnerClear. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_OwnerClearCommon(tpm_state,
					  FALSE);	/* don't erase NVRAM with D bit set */
	writeAllNV1 = TRUE;
    }
    if (returnCode == TPM_SUCCESS) {
	/* 6. Process the maintenance information */
	/* a. Update the SRK */
	/* i. Set the SRK usageAuth to be the same as the TPM source owner's AuthData */
	/* NOTE The source srk.usageAuth was lost, as usageAuth is used to transfer the tpmProof */
	TPM_Secret_Copy(srk_store_asymkey.usageAuth, srk_store_asymkey.migrationAuth);
	/* b. Update TPM_PERMANENT_DATA -> tpmProof */
	TPM_Secret_Copy(tpm_state->tpm_permanent_data.tpmProof, srk_store_asymkey.usageAuth);
	/* save a copy of the HMAC key for the response before invalidating */
	TPM_Secret_Copy(saveKey, *hmacKey);
	/* c. Update TPM_PERMANENT_DATA -> ownerAuth */
	TPM_Secret_Copy(tpm_state->tpm_permanent_data.ownerAuth, srk_store_asymkey.migrationAuth);
	/* serialize the TPM_STORE_ASYMKEY object */
	returnCode = TPM_StoreAsymkey_Store(&asym_sbuffer, FALSE, &srk_store_asymkey);
    }
    /* copy back to the new srk encData (clear text for SRK) */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_SetFromStore(&(newSrk.encData), &asym_sbuffer);
    }
    if (returnCode == TPM_SUCCESS) {
	/* free old SRK resources */
	TPM_Key_Delete(&(tpm_state->tpm_permanent_data.srk));
	/* Copy new SRK to TPM_PERMANENT_DATA -> srk */
	/* This copies the basic TPM_KEY, but not the TPM_STORE_ASYMKEY cache */
	returnCode = TPM_Key_Copy(&(tpm_state->tpm_permanent_data.srk), &newSrk,
				  TRUE);	/* copy encData */
    }
    /* Recreate the TPM_STORE_ASYMKEY cache */
    if (returnCode == TPM_SUCCESS) {
	stream = newSrk.encData.buffer;
	stream_size = newSrk.encData.size;
	returnCode = TPM_Key_LoadStoreAsymKey(&(tpm_state->tpm_permanent_data.srk), FALSE,
					      &stream, &stream_size);
    }
    /* 7. Set TPM_PERMANENT_FLAGS -> maintenanceDone to TRUE */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_LoadMaintenanceArchive: Set maintenanceDone\n");
	TPM_SetCapability_Flag(&writeAllNV2,				  	/* altered */
			       &(tpm_state->tpm_permanent_flags.maintenanceDone),	/* flag */
			       TRUE);						/* value */
    }
    /* Store the permanent data and flags back to NVRAM */
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  (TPM_BOOL)(writeAllNV1 || writeAllNV2),
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_LoadMaintenanceArchive: Ordinal returnCode %08x %u\n",
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
					    saveKey,		/* the original owner HMAC key */
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
    TPM_SizedBuffer_Delete(&archive);			/* @1 */
    TPM_Key_Delete(&newSrk);				/* @2 */
    free(x1InnerWrap);					/* @3 */
    free(r1InnerWrapKey);				/* @4 */
    free(o1Oaep);					/* @5 */
    TPM_StoreAsymkey_Delete(&srk_store_asymkey);	/* @6 */
    TPM_Sbuffer_Delete(&asym_sbuffer);			/* @7 */
    return rcf;
}

/* 12.3 TPM_KillMaintenanceFeature rev 87

   The KillMaintencanceFeature is a permanent action that prevents ANYONE from creating a
   maintenance archive. This action, once taken, is permanent until a new TPM Owner is set.

   This action is to allow those customers who do not want the maintenance feature to not allow the
   use of the maintenance feature.

   At the discretion of the Owner, it should be possible to kill the maintenance feature in such a
   way that the only way to recover maintainability of the platform would be to wipe out the root
   keys. This feature is mandatory in any TPM that implements the maintenance feature.
*/

TPM_RESULT TPM_Process_KillMaintenanceFeature(tpm_state_t *tpm_state,
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
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for owner
					   authentication. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA	ownerAuth;	/* The authorization session digest for inputs and owner
					   authentication.  HMAC key: ownerAuth.*/
    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_SECRET			*hmacKey;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back flags */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_KillMaintenanceFeature: Ordinal Entry\n");
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
	    printf("TPM_Process_KillMaintenanceFeature: Error, command has %u extra bytes\n",
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
    /* 1. Validate the TPM Owner AuthData */
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
    /* 2. Set the TPM_PERMANENT_FLAGS.allowMaintenance flag to FALSE.  */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_KillMaintenanceFeature: Clear allowMaintenance\n");
	TPM_SetCapability_Flag(&writeAllNV,					/* altered */
			       &(tpm_state->tpm_permanent_flags.allowMaintenance),	/* flag */
			       FALSE);						/* value */
	/* Store the permanent flags back to NVRAM */
	returnCode = TPM_PermanentAll_NVStore(tpm_state,
					      writeAllNV,
					      returnCode);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_KillMaintenanceFeature: Ordinal returnCode %08x %u\n",
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

/* 12.4 TPM_LoadManuMaintPub rev 96

   The LoadManuMaintPub command loads the manufacturer's public key for use in the maintenance
   process.  The command installs ManuMaintPub in PERMANENT data storage inside a TPM. Maintenance
   enables duplication of non-migratory data in protected storage. There is therefore a security
   hole if a platform is shipped before the maintenance public key has been installed in a TPM.

   The command is expected to be used before installation of a TPM Owner or any key in TPM protected
   storage.  It therefore does not use authorization.

   The pubKey MUST specify an algorithm whose strength is not less than the RSA algorithm with 2048
   bit keys.

   pubKey SHOULD unambiguously identify the entity that will perform the maintenance process with
   the TPM Owner.

   TPM_PERMANENT_DATA -> manuMaintPub SHALL exist in a TPM-shielded location, only.

   If an entity (Platform Entity) does not support the maintenance process but issues a platform
   credential for a platform containing a TPM that supports the maintenance process, the value of
   TPM_PERMANENT_DATA -> manuMaintPub MUST be set to zero before the platform leaves the entity's
   control.
*/

TPM_RESULT TPM_Process_LoadManuMaintPub(tpm_state_t *tpm_state,
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
    TPM_NONCE	antiReplay;	/* AntiReplay and validation nonce */
    TPM_PUBKEY	pubKey;		/* The public key of the manufacturer to be in use for maintenance
				   */

    /* processing parameters */
    unsigned char *	inParamStart;		/* starting point of inParam's */
    unsigned char *	inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus = FALSE;	/* audit the ordinal */
    TPM_BOOL		transportEncrypt = FALSE;	/* wrapped in encrypted transport session */
    TPM_STORE_BUFFER	pubKeySerial;		/* serialization for checksum calculation */
    const unsigned char *pubKeyBuffer;
    uint32_t		pubKeyLength;
    TPM_BOOL		writeAllNV = FALSE;	/* flag to write back NV */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_DIGEST		checksum;	/* Digest of pubKey and antiReplay  */

    printf("TPM_Process_LoadManuMaintPub: Ordinal Entry\n");
    TPM_Pubkey_Init(&pubKey);		/* freed @1 */
    TPM_Sbuffer_Init(&pubKeySerial);	/* freed @2 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get antiReplay parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Nonce_Load(antiReplay, &command, &paramSize);
    }
    /* get pubKey parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Pubkey_Load(&pubKey, &command, &paramSize);
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
	    printf("TPM_Process_LoadManuMaintPub: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* The first valid TPM_LoadManuMaintPub command received by a TPM SHALL */
    if (returnCode == TPM_SUCCESS) {
	if (!tpm_state->tpm_permanent_data.allowLoadMaintPub) {
	    printf("TPM_Process_LoadManuMaintPub: Error, command already run\n");
	    returnCode = TPM_DISABLED_CMD;
	}
    }
    /* The pubKey MUST specify an algorithm whose strength is not less than the RSA algorithm with
       2048 bit keys. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_KeyParms_CheckProperties(&(pubKey.algorithmParms),	/* TPM_KEY_PARMS */
						  TPM_KEY_STORAGE,		/* TPM_KEY_USAGE */
						  2048,			/* required, in bits */
						  TRUE);			/* FIPS */
    }
    /* 1. Store the parameter pubKey as TPM_PERMANENT_DATA -> manuMaintPub. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Pubkey_Copy(&(tpm_state->tpm_permanent_data.manuMaintPub),
				     &pubKey);
	writeAllNV = TRUE;
    }
    /* 2. Set checksum to SHA-1 of (pubkey || antiReplay) */
    if (returnCode == TPM_SUCCESS) {
	/* serialize pubkey */
	returnCode = TPM_Pubkey_Store(&pubKeySerial, &pubKey);
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_Sbuffer_Get(&pubKeySerial, &pubKeyBuffer, &pubKeyLength);
	/* create the checksum */
	returnCode = TPM_SHA1(checksum,
			      pubKeyLength, pubKeyBuffer,
			      sizeof(TPM_NONCE), antiReplay,
			      0, NULL);
    }
    /* 4. Subsequent calls to TPM_LoadManuMaintPub SHALL return code TPM_DISABLED_CMD. */
    if (returnCode == TPM_SUCCESS) {
	tpm_state->tpm_permanent_data.allowLoadMaintPub = FALSE;
    }
    returnCode = TPM_PermanentAll_NVStore(tpm_state,
					  writeAllNV,
					  returnCode);
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_LoadManuMaintPub: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 3. Export the checksum */
	    returnCode = TPM_Digest_Store(response, checksum);
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
    TPM_Pubkey_Delete(&pubKey);		/* @1 */
    TPM_Sbuffer_Delete(&pubKeySerial);	/* @2 */
    return rcf;
}

/* 12.5 TPM_ReadManuMaintPub rev 99

   The ReadManuMaintPub command is used to check whether the manufacturer's public maintenance key
   in a TPM has the expected value. This may be useful during the manufacture process. The command
   returns a digest of the installed key, rather than the key itself. This hinders discovery of the
   maintenance key, which may (or may not) be useful for manufacturer privacy.

   The command is expected to be used before installation of a TPM Owner or any key in TPM protected
   storage.  It therefore does not use authorization.

   This command returns the hash of the antiReplay nonce and the previously loaded manufacturer's 
   maintenance public key.
*/

TPM_RESULT TPM_Process_ReadManuMaintPub(tpm_state_t *tpm_state,
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
    TPM_NONCE	antiReplay;		/* AntiReplay and validation nonce */

    /* processing parameters */
    unsigned char *	inParamStart;			/* starting point of inParam's */
    unsigned char *	inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_STORE_BUFFER	pubKeySerial;	/* serialization for checksum calculation */
    const unsigned char *pubKeyBuffer;
    uint32_t		pubKeyLength;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_DIGEST		checksum;	/* Digest of pubKey and antiReplay */

    printf("TPM_Process_ReadManuMaintPub: Ordinal Entry\n");
    TPM_Sbuffer_Init(&pubKeySerial);	/* freed @1 */
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get antiReplay parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Nonce_Load(antiReplay, &command, &paramSize);
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
	    printf("TPM_Process_ReadManuMaintPub: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Create "checksum" by concatenating data to form (TPM_PERMANENT_DATA -> manuMaintPub
       || antiReplay) and passing the concatenated data through SHA-1. */
    if (returnCode == TPM_SUCCESS) {
	/* serialize pubkey */
	returnCode = TPM_Pubkey_Store(&pubKeySerial, &(tpm_state->tpm_permanent_data.manuMaintPub));
    }
    if (returnCode == TPM_SUCCESS) {
	TPM_Sbuffer_Get(&pubKeySerial, &pubKeyBuffer, &pubKeyLength);
	/* create the checksum */
	returnCode = TPM_SHA1(checksum,
			      pubKeyLength, pubKeyBuffer,
			      sizeof(TPM_NONCE), antiReplay,
			      0, NULL);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_ReadManuMaintPub: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* 2. Export the checksum */
	    returnCode = TPM_Digest_Store(response, checksum);
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
    TPM_Sbuffer_Delete(&pubKeySerial);	/* @1 */
    return rcf;
}

#endif
