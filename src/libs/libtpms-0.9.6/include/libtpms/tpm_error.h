/********************************************************************************/
/*                                                                              */
/*                              Error Response                                  */
/*                           Written by Ken Goldman                             */
/*                     IBM Thomas J. Watson Research Center                     */
/*            $Id: tpm_error.h $             */
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

#ifndef TPM_ERROR_H
#define TPM_ERROR_H

/* 16. Return codes rev 99

   The TPM has five types of return code. One indicates successful operation and four indicate 
   failure. TPM_SUCCESS (00000000) indicates successful execution. The failure reports are: 
   TPM defined fatal errors (00000001 to 000003FF), vendor defined fatal errors (00000400 to 
   000007FF), TPM defined non-fatal errors (00000800 to 00000BFF), and vendor defined 
   non-fatal errors (00000C00 to 00000FFF).
   
   The range of vendor defined non-fatal errors was determined by the TSS-WG, which defined 
   XXXX YCCC with XXXX as OS specific and Y defining the TSS SW stack layer (0: TPM layer)
   
   All failure cases return only a non-authenticated fixed set of information. This is because 
   the failure may have been due to authentication or other factors, and there is no possibility 
   of producing an authenticated response.
   
   Fatal errors also terminate any authorization sessions. This is a result of returning only the 
   error code, as there is no way to return the nonces necessary to maintain an authorization 
   session. Non-fatal errors do not terminate authorization sessions.

   The return code MUST use the following base. The return code MAY be TCG defined or vendor
   defined. */

#define TPM_BASE                0x0             /*  The start of TPM return codes */
#define TPM_SUCCESS             TPM_BASE        /* Successful completion of the operation */
#define TPM_VENDOR_ERROR        TPM_Vendor_Specific32   /* Mask to indicate that the error code is
                                                           vendor specific for vendor specific
                                                           commands. */
#define TPM_NON_FATAL           0x00000800 /* Mask to indicate that the error code is a non-fatal
                                              failure. */

/* TPM-defined fatal error codes */

#define TPM_AUTHFAIL            TPM_BASE + 1  /* Authentication failed */
#define TPM_BADINDEX            TPM_BASE + 2  /* The index to a PCR, DIR or other register is
                                                 incorrect */
#define TPM_BAD_PARAMETER       TPM_BASE + 3  /* One or more parameter is bad */
#define TPM_AUDITFAILURE        TPM_BASE + 4  /* An operation completed successfully but the auditing
                                                 of that operation failed.  */
#define TPM_CLEAR_DISABLED      TPM_BASE + 5  /* The clear disable flag is set and all clear
                                                 operations now require physical access */
#define TPM_DEACTIVATED         TPM_BASE + 6  /* The TPM is deactivated */
#define TPM_DISABLED            TPM_BASE + 7  /* The TPM is disabled */
#define TPM_DISABLED_CMD        TPM_BASE + 8  /* The target command has been disabled */
#define TPM_FAIL                TPM_BASE + 9  /* The operation failed */
#define TPM_BAD_ORDINAL         TPM_BASE + 10 /* The ordinal was unknown or inconsistent */
#define TPM_INSTALL_DISABLED    TPM_BASE + 11 /* The ability to install an owner is disabled */
#define TPM_INVALID_KEYHANDLE   TPM_BASE + 12 /* The key handle presented was invalid */
#define TPM_KEYNOTFOUND         TPM_BASE + 13 /* The target key was not found */
#define TPM_INAPPROPRIATE_ENC   TPM_BASE + 14 /* Unacceptable encryption scheme */
#define TPM_MIGRATEFAIL         TPM_BASE + 15 /* Migration authorization failed */
#define TPM_INVALID_PCR_INFO    TPM_BASE + 16 /* PCR information could not be interpreted */
#define TPM_NOSPACE             TPM_BASE + 17 /* No room to load key.  */
#define TPM_NOSRK               TPM_BASE + 18 /* There is no SRK set */
#define TPM_NOTSEALED_BLOB      TPM_BASE + 19 /* An encrypted blob is invalid or was not created by
                                                 this TPM */
#define TPM_OWNER_SET           TPM_BASE + 20 /* There is already an Owner */
#define TPM_RESOURCES           TPM_BASE + 21 /* The TPM has insufficient internal resources to
                                                 perform the requested action.  */
#define TPM_SHORTRANDOM         TPM_BASE + 22 /* A random string was too short */
#define TPM_SIZE                TPM_BASE + 23 /* The TPM does not have the space to perform the
                                                 operation. */
#define TPM_WRONGPCRVAL         TPM_BASE + 24 /* The named PCR value does not match the current PCR
                                                 value. */
#define TPM_BAD_PARAM_SIZE      TPM_BASE + 25 /* The paramSize argument to the command has the
                                                 incorrect value */
#define TPM_SHA_THREAD          TPM_BASE + 26 /* There is no existing SHA-1 thread.  */
#define TPM_SHA_ERROR           TPM_BASE + 27 /* The calculation is unable to proceed because the
                                                 existing SHA-1 thread has already encountered an
                                                 error.  */
#define TPM_FAILEDSELFTEST      TPM_BASE + 28 /* Self-test has failed and the TPM has shutdown.  */
#define TPM_AUTH2FAIL           TPM_BASE + 29 /* The authorization for the second key in a 2 key
                                                 function failed authorization */
#define TPM_BADTAG              TPM_BASE + 30 /* The tag value sent to for a command is invalid */
#define TPM_IOERROR             TPM_BASE + 31 /* An IO error occurred transmitting information to
                                                 the TPM */
#define TPM_ENCRYPT_ERROR       TPM_BASE + 32 /* The encryption process had a problem.  */
#define TPM_DECRYPT_ERROR       TPM_BASE + 33 /* The decryption process did not complete.  */
#define TPM_INVALID_AUTHHANDLE  TPM_BASE + 34 /* An invalid handle was used.  */
#define TPM_NO_ENDORSEMENT      TPM_BASE + 35 /* The TPM does not a EK installed */
#define TPM_INVALID_KEYUSAGE    TPM_BASE + 36 /* The usage of a key is not allowed */
#define TPM_WRONG_ENTITYTYPE    TPM_BASE + 37 /* The submitted entity type is not allowed */
#define TPM_INVALID_POSTINIT    TPM_BASE + 38 /* The command was received in the wrong sequence
                                                 relative to TPM_Init and a subsequent TPM_Startup
                                                 */
#define TPM_INAPPROPRIATE_SIG   TPM_BASE + 39 /* Signed data cannot include additional DER
                                                 information */
#define TPM_BAD_KEY_PROPERTY    TPM_BASE + 40 /* The key properties in TPM_KEY_PARMs are not
                                                 supported by this TPM */
#define TPM_BAD_MIGRATION       TPM_BASE + 41 /* The migration properties of this key are incorrect.
                                               */
#define TPM_BAD_SCHEME          TPM_BASE + 42 /* The signature or encryption scheme for this key is
                                                 incorrect or not permitted in this situation.  */
#define TPM_BAD_DATASIZE        TPM_BASE + 43 /* The size of the data (or blob) parameter is bad or
                                                 inconsistent with the referenced key */
#define TPM_BAD_MODE            TPM_BASE + 44 /* A mode parameter is bad, such as capArea or
                                                 subCapArea for TPM_GetCapability, physicalPresence
                                                 parameter for TPM_PhysicalPresence, or
                                                 migrationType for TPM_CreateMigrationBlob.  */
#define TPM_BAD_PRESENCE        TPM_BASE + 45 /* Either the physicalPresence or physicalPresenceLock
                                                 bits have the wrong value */
#define TPM_BAD_VERSION         TPM_BASE + 46 /* The TPM cannot perform this version of the
                                                 capability */
#define TPM_NO_WRAP_TRANSPORT   TPM_BASE + 47 /* The TPM does not allow for wrapped transport
                                                 sessions */
#define TPM_AUDITFAIL_UNSUCCESSFUL TPM_BASE + 48 /* TPM audit construction failed and the
                                                    underlying command was returning a failure
                                                    code also */
#define TPM_AUDITFAIL_SUCCESSFUL   TPM_BASE + 49 /* TPM audit construction failed and the underlying
                                                    command was returning success */
#define TPM_NOTRESETABLE        TPM_BASE + 50 /* Attempt to reset a PCR register that does not have
                                                 the resettable attribute */
#define TPM_NOTLOCAL            TPM_BASE + 51 /* Attempt to reset a PCR register that requires
                                                 locality and locality modifier not part of command
                                                 transport */
#define TPM_BAD_TYPE            TPM_BASE + 52 /* Make identity blob not properly typed */
#define TPM_INVALID_RESOURCE    TPM_BASE + 53 /* When saving context identified resource type does
                                                 not match actual resource */
#define TPM_NOTFIPS             TPM_BASE + 54 /* The TPM is attempting to execute a command only
                                                 available when in FIPS mode */
#define TPM_INVALID_FAMILY      TPM_BASE + 55 /* The command is attempting to use an invalid family
                                                 ID */
#define TPM_NO_NV_PERMISSION    TPM_BASE + 56 /* The permission to manipulate the NV storage is not
                                                 available */
#define TPM_REQUIRES_SIGN       TPM_BASE + 57 /* The operation requires a signed command */
#define TPM_KEY_NOTSUPPORTED    TPM_BASE + 58 /* Wrong operation to load an NV key */
#define TPM_AUTH_CONFLICT       TPM_BASE + 59 /* NV_LoadKey blob requires both owner and blob
                                                 authorization */
#define TPM_AREA_LOCKED         TPM_BASE + 60 /* The NV area is locked and not writable */
#define TPM_BAD_LOCALITY        TPM_BASE + 61 /* The locality is incorrect for the attempted
                                                 operation */
#define TPM_READ_ONLY           TPM_BASE + 62 /* The NV area is read only and can't be written to
                                               */
#define TPM_PER_NOWRITE         TPM_BASE + 63 /* There is no protection on the write to the NV area
                                               */
#define TPM_FAMILYCOUNT         TPM_BASE + 64 /* The family count value does not match */
#define TPM_WRITE_LOCKED        TPM_BASE + 65 /* The NV area has already been written to */
#define TPM_BAD_ATTRIBUTES      TPM_BASE + 66 /* The NV area attributes conflict */
#define TPM_INVALID_STRUCTURE   TPM_BASE + 67 /* The structure tag and version are invalid or
                                                 inconsistent */
#define TPM_KEY_OWNER_CONTROL   TPM_BASE + 68 /* The key is under control of the TPM Owner and can
                                                 only be evicted by the TPM Owner.  */
#define TPM_BAD_COUNTER         TPM_BASE + 69 /* The counter handle is incorrect */
#define TPM_NOT_FULLWRITE       TPM_BASE + 70 /* The write is not a complete write of the area */
#define TPM_CONTEXT_GAP         TPM_BASE + 71 /* The gap between saved context counts is too large
                                               */
#define TPM_MAXNVWRITES         TPM_BASE + 72 /* The maximum number of NV writes without an owner
                                                 has been exceeded */
#define TPM_NOOPERATOR          TPM_BASE + 73 /* No operator authorization value is set */
#define TPM_RESOURCEMISSING     TPM_BASE + 74 /* The resource pointed to by context is not loaded
                                               */
#define TPM_DELEGATE_LOCK       TPM_BASE + 75 /* The delegate administration is locked */
#define TPM_DELEGATE_FAMILY     TPM_BASE + 76 /* Attempt to manage a family other then the delegated
                                                 family */
#define TPM_DELEGATE_ADMIN      TPM_BASE + 77 /* Delegation table management not enabled */
#define TPM_TRANSPORT_NOTEXCLUSIVE TPM_BASE + 78 /* There was a command executed outside of an
                                                 exclusive transport session */
#define TPM_OWNER_CONTROL       TPM_BASE + 79 /* Attempt to context save a owner evict controlled
                                                 key */
#define TPM_DAA_RESOURCES       TPM_BASE + 80 /* The DAA command has no resources available to
                                                 execute the command */
#define TPM_DAA_INPUT_DATA0     TPM_BASE + 81 /* The consistency check on DAA parameter inputData0
                                                 has failed. */
#define TPM_DAA_INPUT_DATA1     TPM_BASE + 82 /* The consistency check on DAA parameter inputData1
                                                 has failed. */
#define TPM_DAA_ISSUER_SETTINGS TPM_BASE + 83 /* The consistency check on DAA_issuerSettings has
                                                 failed. */
#define TPM_DAA_TPM_SETTINGS    TPM_BASE + 84 /* The consistency check on DAA_tpmSpecific has
                                                 failed. */
#define TPM_DAA_STAGE           TPM_BASE + 85 /* The atomic process indicated by the submitted DAA
                                                 command is not the expected process. */
#define TPM_DAA_ISSUER_VALIDITY TPM_BASE + 86 /* The issuer's validity check has detected an
                                                 inconsistency */
#define TPM_DAA_WRONG_W         TPM_BASE + 87 /* The consistency check on w has failed. */
#define TPM_BAD_HANDLE          TPM_BASE + 88 /* The handle is incorrect */
#define TPM_BAD_DELEGATE        TPM_BASE + 89 /* Delegation is not correct */
#define TPM_BADCONTEXT          TPM_BASE + 90 /* The context blob is invalid */
#define TPM_TOOMANYCONTEXTS     TPM_BASE + 91 /* Too many contexts held by the TPM */
#define TPM_MA_TICKET_SIGNATURE TPM_BASE + 92 /* Migration authority signature validation failure
                                               */
#define TPM_MA_DESTINATION      TPM_BASE + 93 /* Migration destination not authenticated */
#define TPM_MA_SOURCE           TPM_BASE + 94 /* Migration source incorrect */
#define TPM_MA_AUTHORITY        TPM_BASE + 95 /* Incorrect migration authority */
#define TPM_PERMANENTEK         TPM_BASE + 97 /* Attempt to revoke the EK and the EK is not revocable */
#define TPM_BAD_SIGNATURE       TPM_BASE + 98 /* Bad signature of CMK ticket */ 
#define TPM_NOCONTEXTSPACE      TPM_BASE + 99 /* There is no room in the context list for additional
                                                 contexts */

/* As error codes are added here, they should also be added to lib/miscfunc.c */

/* TPM-defined non-fatal errors */

#define TPM_RETRY               TPM_BASE + TPM_NON_FATAL /* The TPM is too busy to respond to the
                                                            command immediately, but the command
                                                            could be submitted at a later time */
#define TPM_NEEDS_SELFTEST      TPM_BASE + TPM_NON_FATAL + 1 /* TPM_ContinueSelfTest has has not
                                                                been run*/
#define TPM_DOING_SELFTEST      TPM_BASE + TPM_NON_FATAL + 2 /* The TPM is currently executing the
                                                                actions of TPM_ContinueSelfTest
                                                                because the ordinal required
                                                                resources that have not been
                                                                tested. */
#define TPM_DEFEND_LOCK_RUNNING TPM_BASE + TPM_NON_FATAL + 3
                                                        /* The TPM is defending against dictionary
                                                           attacks and is in some time-out
                                                           period. */

#endif
