/********************************************************************************/
/*										*/
/*			    TPM Admin Test and Opt-in				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*	      $Id: tpm_admin.c $		*/
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

#include "tpm_auth.h"
#include "tpm_cryptoh.h"
#include "tpm_digest.h"
#include "tpm_debug.h"
#include "tpm_error.h"
#include "tpm_key.h"
#include "tpm_nonce.h"
#include "tpm_permanent.h"
#include "tpm_process.h"
#include "tpm_secret.h"
#include "tpm_ticks.h"
#include "tpm_time.h"

#include "tpm_admin.h"

/* The Software TPM Self Test works as follows:

   TPM_LimitedSelfTestCommon(void) - self tests which affect all TPM's
   TPM_LimitedSelfTestTPM(tpm_state) - self test per virtual TPM

   TPM_ContinueSelfTestCmd(tpm_state) - currently does nothing
       on failure, sets tpm_state->testState to failure for the virtual TPM

   TPM_SelfTestFullCmd(tpm_state) calls
       TPM_LimitedSelfTestTPM()
       TPM_ContinueSelfTestCmd(tpm_state)
       on failure, sets tpm_state->testState to failure for the virtual TPM

   TPM_MainInit(void) calls
       TPM_LimitedSelfTestCommon(void)
       TPM_LimitedSelfTestTPM(tpm_state)

   TPM_Process_ContinueSelfTest(tpm_state) calls either (depending on FIPS mode)
       TPM_SelfTestFullCmd(tpm_state)
       TPM_ContinueSelfTestCmd(tpm_state)

   TPM_Process_SelfTestFull(tpm_state) calls	
       TPM_SelfTestFullCmd(tpm_state)

   The Software TPM assumes that the coprocessor has run self tests before the application code even
   begins.  So this code doesn't do any real testing of the underlying hardware.  This simplifies
   the state machine, since TPM_Process_ContinueSelfTest doesn't require a separate thread.
*/

/* TPM_LimitedSelfTestCommon() provides the assurance that a selected subset of TPM commands will
   perform properly. The limited nature of the self-test allows the TPM to be functional in as short
   of time as possible. all the TPM tests.

   The caller is responsible for setting the shutdown state on error.
*/

TPM_RESULT TPM_LimitedSelfTestCommon(void)
{
    TPM_RESULT	rc = 0;
    uint32_t	tv_sec;
    uint32_t	tv_usec;

    printf(" TPM_LimitedSelfTestCommon:\n");
#if 0
    if (rc == 0) {
	rc = TPM_Sbuffer_Test();
    }
#endif
    if (rc == 0) {
	rc = TPM_Uint64_Test();
    }
    if (rc == 0) {
	rc = TPM_CryptoTest();
    }
    /* test time of day clock */
    if (rc == 0) {
	rc = TPM_GetTimeOfDay(&tv_sec, &tv_usec);
    }
    if (rc != 0) {
	rc = TPM_FAILEDSELFTEST;
    }
    return rc;
}

TPM_RESULT TPM_LimitedSelfTestTPM(tpm_state_t *tpm_state)
{
    TPM_RESULT		rc = 0;
    TPM_NONCE		clrData;
    TPM_SIZED_BUFFER	encData;
    TPM_NONCE		decData;
    uint32_t		decLength;
    
    
    printf(" TPM_LimitedSelfTestTPM:\n");
    TPM_SizedBuffer_Init(&encData);		/* freed @1 */

    /* 8. The TPM MUST check the following: */	
    /* a. RNG functionality */
    /* NOTE Tested by coprocessor boot */
    /* b. Reading and extending the integrity registers. The self-test for the integrity registers
       will leave the integrity registers in a known state. */
    /* NOTE Since there is nothing special about the PCR's, the common TPM_CryptoTest() is
       sufficient */
    /* c. Testing the EK integrity, if it exists */
    /* i. This requirement specifies that the TPM will verify that the endorsement key pair can
       encrypt and decrypt a known value.  This tests the RSA engine. If the EK has not yet been
       generated the TPM action is manufacturer specific. */
    if ((rc == 0) &&
	(tpm_state->tpm_permanent_data.endorsementKey.keyUsage != TPM_KEY_UNINITIALIZED)) {
	/* check the key integrity */
	if (rc == 0) {
	    rc = TPM_Key_CheckPubDataDigest(&(tpm_state->tpm_permanent_data.endorsementKey));
	}
	/* encrypt */
	if (rc == 0) {
	    TPM_Nonce_Generate(clrData);
	    rc = TPM_RSAPublicEncrypt_Key(&encData,		/* output */
					  clrData,		/* input */
					  TPM_NONCE_SIZE,	/* input */
					  &(tpm_state->tpm_permanent_data.endorsementKey));
	}	
	/* decrypt */
	if (rc == 0) {
	    rc = TPM_RSAPrivateDecryptH(decData,	/* decrypted data */
					&decLength,	/* length of data put into decrypt_data */
					TPM_NONCE_SIZE, /* size of decrypt_data buffer */
					encData.buffer,
					encData.size,
					&(tpm_state->tpm_permanent_data.endorsementKey));
	}
	/* verify */
	if (rc == 0) {
	    if (decLength != TPM_NONCE_SIZE) {
		printf("TPM_LimitedSelfTestTPM: Error, decrypt length %u should be %u\n",
		       decLength, TPM_NONCE_SIZE);
		rc = TPM_FAILEDSELFTEST;
	    }
	}
	if (rc == 0) {
	    rc = TPM_Nonce_Compare(clrData, decData);
	}
    }
    /* d. The integrity of the protected capabilities of the TPM */
    /* i. This means that the TPM must ensure that its "microcode" has not changed, and not that a
       test must be run on each function. */
    /* e. Any tamper-resistance markers */
    /* i. The tests on the tamper-resistance or tamper-evident markers are under programmable
       control.	 */
    /* There is no requirement to check tamper-evident tape or the status of epoxy surrounding the
       case. */
    /* NOTE: Done by coprocessor POST */
    /* 9. The TPM SHOULD check the following: */
    /* a. The hash functionality */
    /* i. This check will hash a known value and compare it to an expected result. There is no
       requirement to accept external data to perform the check.  */
    /* ii. The TPM MAY support a test using external data. */
    /* NOTE: Done by TPM_CryptoTest() */
    /* b. Any symmetric algorithms */
    /* i. This check will use known data with a random key to encrypt and decrypt the data */
    /* NOTE: Done by TPM_CryptoTest() */
    /* c. Any additional asymmetric algorithms */
    /* i. This check will use known data to encrypt and decrypt. */
    /* NOTE: So far only RSA is supported */
    /* d. The key-wrapping mechanism */
    /* i. The TPM should wrap and unwrap a key. The TPM MUST NOT use the endorsement key pair for
       this test. */
    /* NOTE: There is nothing special about serializing a TPM_STORE_ASYMKEY */
    /* e. Any other internal mechanisms */
    TPM_SizedBuffer_Delete(&encData);		/* @1 */
    if (rc != 0) {
	rc = TPM_FAILEDSELFTEST;
    }
    /* set the TPM test state */
    if ((rc == 0) && (tpm_state->testState != TPM_TEST_STATE_FAILURE)) {
	printf("  TPM_LimitedSelfTestTPM: Set testState to %u \n", TPM_TEST_STATE_LIMITED);
	tpm_state->testState = TPM_TEST_STATE_LIMITED;
    }
    else {
	printf("  TPM_LimitedSelfTestTPM: Set testState to %u \n", TPM_TEST_STATE_FAILURE);
	tpm_state->testState = TPM_TEST_STATE_FAILURE;
    }
    return rc;
}

/* TPM_ContinueSelfTestCmd() runs the continue self test actions

*/

TPM_RESULT TPM_ContinueSelfTestCmd(tpm_state_t *tpm_state)
{
    TPM_RESULT	rc = 0;

    /* NOTE all done by limited self test */
    printf(" TPM_ContinueSelfTestCmd:\n");
    if (rc != 0) {
	rc = TPM_FAILEDSELFTEST;
    }
    /* set the TPM test state */
    if (rc == 0) {
	printf("  TPM_ContinueSelfTestCmd: Set testState to %u \n", TPM_TEST_STATE_FULL);
	tpm_state->testState = TPM_TEST_STATE_FULL;
    }
    else {
	printf("  TPM_ContinueSelfTestCmd: Set testState to %u \n", TPM_TEST_STATE_FAILURE);
	tpm_state->testState = TPM_TEST_STATE_FAILURE;
    }
    return rc;
}

/* TPM_SelfTestFullCmd is a request to have the TPM perform another complete self-test.  This test
   will take some time but provides an accurate assessment of the TPM's ability to perform all
   operations.

   Runs the actions of self test full.
*/

TPM_RESULT TPM_SelfTestFullCmd(tpm_state_t *tpm_state)
{
    TPM_RESULT	rc = 0;

    printf(" TPM_SelfTestFullCmd\n");
    if (rc == 0) {
	rc = TPM_LimitedSelfTestTPM(tpm_state);
    }
    if (rc == 0) {
	rc = TPM_ContinueSelfTestCmd(tpm_state);
    }
    return rc;
}

/* 4.1 TPM_SelfTestFull rev 88

   SelfTestFull tests all of the TPM capabilities.

   Unlike TPM_ContinueSelfTest, which may optionally return immediately and then perform the tests,
   TPM_SelfTestFull always performs the tests and then returns success or failure.
*/

TPM_RESULT TPM_Process_SelfTestFull(tpm_state_t *tpm_state,
				    TPM_STORE_BUFFER *response,
				    TPM_TAG tag,
				    uint32_t paramSize,
				    TPM_COMMAND_CODE ordinal,
				    unsigned char *command,
				    TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

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

    printf("TPM_Process_SelfTestFull: Ordinal Entry\n");
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
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_SelfTestFull: Error, command has %u extra bytes\n", paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. TPM_SelfTestFull SHALL cause a TPM to perform self-test of each TPM internal function. */
    /* a. If the self-test succeeds, return TPM_SUCCESS. */
    /* b. If the self-test fails, return TPM_FAILEDSELFTEST. */
    /* 2. Failure of any test results in overall failure, and the TPM goes into failure mode. */
    /* 3. If the TPM has not executed the action of TPM_ContinueSelfTest, the TPM */
    /* a. MAY perform the full self-test. */
    /* b. MAY return TPM_NEEDS_SELFTEST. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SelfTestFullCmd(tpm_state);
    }
    /*
      response
    */
    if (rcf == 0) {
	printf("TPM_Process_SelfTestFull: Ordinal returnCode %08x %u\n",
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
					       response->buffer + outParamStart, /* start */
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

/* 4.2 TPM_ContinueSelfTest rev 88

   TPM_Process_ContinueSelfTest informs the TPM that it may complete the self test of all TPM
   functions.

   The TPM may return success immediately and then perform the self-test, or it may perform the
   self-test and then return success or failure.

   1. Prior to executing the actions of TPM_ContinueSelfTest, if the TPM receives a command C1 that
   uses an untested TPM function, the TPM MUST take one of these actions:

   a. The TPM MAY return TPM_NEEDS_SELFTEST

   i. This indicates that the TPM has not tested the internal resources required to execute C1.

   ii. The TPM does not execute C1.

   iii. The caller MUST issue TPM_ContinueSelfTest before re-issuing the command C1.

   (1) If the TPM permits TPM_SelfTestFull prior to completing the actions of TPM_ContinueSelfTest,
   the caller MAY issue TPM_SelfTestFull rather than TPM_ContinueSelfTest.

   b. The TPM MAY return TPM_DOING_SELFTEST

   i. This indicates that the TPM is doing the actions of TPM_ContinueSelfTest implicitly, as if the
   TPM_ContinueSelfTest command had been issued.

   ii. The TPM does not execute C1.

   iii. The caller MUST wait for the actions of TPM_ContinueSelfTest to complete before reissuing
   the command C1.

   c. The TPM MAY return TPM_SUCCESS or an error code associated with C1.

   i. This indicates that the TPM has completed the actions of TPM_ContinueSelfTest and has
   completed the command C1.

   ii. The error code MAY be TPM_FAILEDSELFTEST.
*/

TPM_RESULT TPM_Process_ContinueSelfTest(tpm_state_t *tpm_state,
					TPM_STORE_BUFFER *response,
					TPM_TAG tag,
					uint32_t paramSize,
					TPM_COMMAND_CODE ordinal,
					unsigned char *command,
					TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_ContinueSelfTest: Ordinal Entry\n");
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
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_ContinueSelfTest: Error, command has %u extra bytes\n", paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    if (returnCode == TPM_SUCCESS) {
	/* 1. If TPM_PERMANENT_FLAGS -> FIPS is TRUE or TPM_PERMANENT_FLAGS -> TPMpost is TRUE */
	if ((tpm_state->tpm_permanent_flags.FIPS) ||
	    (tpm_state->tpm_permanent_flags.TPMpost)) {
	    /* a. The TPM MUST run ALL self-tests */
	    returnCode = TPM_SelfTestFullCmd(tpm_state);
	}
	/* 2. Else */
	else {
	    /* a. The TPM MUST complete all self-tests that are outstanding */
	    /* i. Instead of completing all outstanding self-tests the TPM MAY run all self-tests */
	    returnCode = TPM_ContinueSelfTestCmd(tpm_state);
	}
    }
    /* 3. The TPM either
       a. MAY immediately return TPM_SUCCESS
       i. When TPM_ContinueSelfTest finishes execution, it MUST NOT respond to the caller with a
       return code.
       b. MAY complete the self-test and then return TPM_SUCCESS or TPM_FAILEDSELFTEST.
       NOTE Option 3.b. implemented
    */
    /*
      response
    */
    if (rcf == 0) {
	printf("TPM_Process_ContinueSelfTest: Ordinal returnCode %08x %u\n",
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
    return rcf;
}

/* 4.3 TPM_GetTestResult rev 96

   TPM_GetTestResult provides manufacturer specific information regarding the results of the self
   test. This command will work when the TPM is in self test failure mode. The reason for allowing
   this command to operate in the failure mode is to allow TPM manufacturers to obtain diagnostic
   information.
*/

TPM_RESULT TPM_Process_GetTestResult(tpm_state_t *tpm_state,
				     TPM_STORE_BUFFER *response,
				     TPM_TAG tag,
				     uint32_t paramSize,
				     TPM_COMMAND_CODE ordinal,
				     unsigned char *command,
				     TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
    TPM_SIZED_BUFFER	outData;	/* The outData this is manufacturer specific */
    
    printf("TPM_Process_GetTestResult: Ordinal Entry\n");
    TPM_SizedBuffer_Init(&outData);	/* freed @1 */
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
    /* This command will work when the TPM is in self test failure or limited operation mode. */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckState(tpm_state, tag, TPM_CHECK_NO_LOCKOUT);
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_GetTestResult: Error, command has %u extra bytes\n", paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. The TPM SHALL respond to this command with a manufacturer specific block of information
       that describes the result of the latest self test. */
    /* 2. The information MUST NOT contain any data that uniquely identifies an individual TPM. */
    /* allocate some reasonable area */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_SizedBuffer_Allocate(&outData, 128);
    }
    /* for now, just return the state of shutdown as a printable string */
    if (returnCode == TPM_SUCCESS) {
	size_t len = outData.size;
	/* cast because TPM_SIZED_BUFFER is typically unsigned (binary) but snprintf expects char */
	outData.size = snprintf((char *)(outData.buffer), len,
	                        "Shutdown %08x\n", tpm_state->testState);
	if (outData.size >= len) {
	    printf("TPM_Process_GetTestResult: Error (fatal), buffer too small\n");
	    returnCode = TPM_FAIL;
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_GetTestResult: Ordinal returnCode %08x %u\n",
	       returnCode, returnCode);
	rcf = TPM_Sbuffer_StoreInitialResponse(response, tag, returnCode);
    }
    /* success response, append the rest of the parameters.  */
    if (rcf == 0) {
	if (returnCode == TPM_SUCCESS) {
	    /* checkpoint the beginning of the outParam's */
	    outParamStart = response->buffer_current - response->buffer;
	    /* return outData */
	    returnCode = TPM_SizedBuffer_Store(response, &outData);
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
    TPM_SizedBuffer_Delete(&outData);		/* @1 */
    return rcf;
}

/* 5.1 TPM_SetOwnerInstall rev 100

    When enabled but without an owner this command sets the PERMANENT flag that allows or disallows
    the ability to insert an owner.
 */

TPM_RESULT TPM_Process_SetOwnerInstall(tpm_state_t *tpm_state,
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
    TPM_BOOL	      state;	/* State to which ownership flag is to be set. */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back NV */
    TPM_BOOL			physicalPresence;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_SetOwnerInstall: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get state */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_LoadBool(&state, &command, &paramSize);
    }
    /* save the ending point of inParam's for authorization */
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
	    printf("TPM_Process_SetOwnerInstall: Error, command has %u extra bytes\n", paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    if (returnCode == TPM_SUCCESS) {
	/* 1. If the TPM has a current owner, this command immediately returns with TPM_SUCCESS. */
	if (tpm_state->tpm_permanent_data.ownerInstalled) {
	    printf("TPM_Process_SetOwnerInstall: Already current owner\n");
	}
	/* If the TPM does not have a current owner */
	else {
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_SetOwnerInstall: No current owner\n");
		/* 2.  The TPM validates the assertion of physical presence. The TPM then sets the
		   value of TPM_PERMANENT_FLAGS -> ownership to the value in state.
		*/
		returnCode = TPM_Global_GetPhysicalPresence(&physicalPresence, tpm_state);
	    }
	    if (returnCode == TPM_SUCCESS) {
		if (!physicalPresence) {
		    printf("TPM_Process_SetOwnerInstall: Error, physicalPresence is FALSE\n");
		    returnCode = TPM_BAD_PRESENCE;
		}
	    }
	    if (returnCode == TPM_SUCCESS) {
		printf("TPM_Process_SetOwnerInstall: Setting ownership to %02x\n", state);
		TPM_SetCapability_Flag(&writeAllNV,				/* altered */
				       &(tpm_state->tpm_permanent_flags.ownership),	/* flag */
				       state);						/* value */
		/* Store the permanent flags back to NVRAM */
		returnCode = TPM_PermanentAll_NVStore(tpm_state,
						      writeAllNV,
						      returnCode);
	    }
	}
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SetOwnerInstall: Ordinal returnCode %08x %u\n",
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
    return rcf;
}

/* 5.2 TPM_OwnerSetDisable rev 107

   The TPM owner sets the PERMANENT disable flag to TRUE or FALSE.
*/

TPM_RESULT TPM_Process_OwnerSetDisable(tpm_state_t *tpm_state,
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
    TPM_BOOL		disableState;	/* Value for disable state */
    TPM_AUTHHANDLE	authHandle;	/* The authorization handle used for owner authorization. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL		continueAuthSession = TRUE;	/* The continue use flag for the
							   authorization handle */
    TPM_AUTHDATA	ownerAuth;	/* The authorization digest for inputs and owner
					   authorization. HMAC key: ownerAuth. */
    
    /* processing parameters */
    unsigned char *	inParamStart;	/* starting point of inParam's */
    unsigned char *	inParamEnd;	/* ending point of inParam's */
    TPM_DIGEST		inParamDigest;
    TPM_BOOL		auditStatus;		/* audit the ordinal */
    TPM_BOOL		transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL		authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA *auth_session_data;	/* session data for authHandle */
    TPM_SECRET		*hmacKey;
    TPM_BOOL		writeAllNV = FALSE;	/* flag to write back NV */

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_OwnerSetDisable: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get disableState parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_LoadBool(&disableState, &command, &paramSize);
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
						     TPM_CHECK_OWNER |
						     TPM_CHECK_NO_LOCKOUT));
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
	    printf("TPM_Process_OwnerSetDisable: Error, command has %u extra bytes\n", paramSize);
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
    /* 1. The TPM SHALL authenticate the command as coming from the TPM Owner. If unsuccessful, the
       TPM SHALL return TPM_AUTHFAIL. */
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
    /* 2. The TPM SHALL set the TPM_PERMANENT_FLAGS -> disable flag to the value in the
       disableState parameter. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_OwnerSetDisable: Setting disable to %u\n", disableState);
	TPM_SetCapability_Flag(&writeAllNV,					/* altered */
			       &(tpm_state->tpm_permanent_flags.disable),	/* flag */
			       disableState);					/* value */
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
	printf("TPM_Process_OwnerSetDisable: Ordinal returnCode %08x %u\n",
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
    /* if there was an error, terminate the session. */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    return rcf;
}

/* 5.3 TPM_PhysicalEnable rev 87

   Sets the PERMANENT disable flag to FALSE using physical presence as authorization.
*/

TPM_RESULT TPM_Process_PhysicalEnable(tpm_state_t *tpm_state,
				      TPM_STORE_BUFFER *response,
				      TPM_TAG tag,
				      uint32_t paramSize,
				      TPM_COMMAND_CODE ordinal,
				      unsigned char *command,
				      TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back NV */
    TPM_BOOL			physicalPresence;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_PhysicalEnable: Ordinal Entry\n");
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
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_PhysicalEnable: Error, command has %u extra bytes\n", paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Validate that physical presence is being asserted, if not return TPM_BAD_PRESENCE */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Global_GetPhysicalPresence(&physicalPresence, tpm_state);
    }
    if (returnCode == TPM_SUCCESS) {
	if (!physicalPresence) {
	    printf("TPM_Process_PhysicalEnable: Error, physicalPresence is FALSE\n");
	    returnCode = TPM_BAD_PRESENCE;
	}
    }
    /* 2. The TPM SHALL set the TPM_PERMANENT_FLAGS.disable value to FALSE. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_PhysicalEnable: Setting disable to FALSE\n");
	TPM_SetCapability_Flag(&writeAllNV,					/* altered */
			       &(tpm_state->tpm_permanent_flags.disable),	/* flag */
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
	printf("TPM_Process_PhysicalEnable: Ordinal returnCode %08x %u\n",
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
    return rcf;
}

/* 5.4 TPM_PhysicalDisable rev 87

   Sets the PERMANENT disable flag to TRUE using physical presence as authorization
*/
	    
TPM_RESULT TPM_Process_PhysicalDisable(tpm_state_t *tpm_state,
				       TPM_STORE_BUFFER *response,
				       TPM_TAG tag,
				       uint32_t paramSize,
				       TPM_COMMAND_CODE ordinal,
				       unsigned char *command,
				       TPM_TRANSPORT_INTERNAL *transportInternal)
{
    TPM_RESULT	rcf = 0;			/* fatal error precluding response */
    TPM_RESULT	returnCode = TPM_SUCCESS;	/* command return code */

    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back NV */
    TPM_BOOL			physicalPresence;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_PhysicalDisable: Ordinal Entry\n");
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
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_ENABLED |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_PhysicalDisable: Error, command has %u extra bytes\n", paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Validate that physical presence is being asserted, if not return TPM_BAD_PRESENCE */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Global_GetPhysicalPresence(&physicalPresence, tpm_state);
    }
    if (returnCode == TPM_SUCCESS) {
	if (!physicalPresence) {
	    printf("TPM_Process_PhysicalDisable: Error, physicalPresence is FALSE\n");
	    returnCode = TPM_BAD_PRESENCE;
	}
    }
    /* 2. The TPM SHALL set the TPM_PERMANENT_FLAGS.disable value to TRUE. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_PhysicalDisable: Setting disable to TRUE\n");
	TPM_SetCapability_Flag(&writeAllNV,					/* altered */
			       &(tpm_state->tpm_permanent_flags.disable ),	/* flag */
			       TRUE);						/* value */
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
	printf("TPM_Process_PhysicalDisable: Ordinal returnCode %08x %u\n",
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
    return rcf;
}

/* 5.5 TPM_PhysicalSetDeactivated rev 105

   Changes the TPM persistent deactivated flag using physical presence as authorization.
*/

TPM_RESULT TPM_Process_PhysicalSetDeactivated(tpm_state_t *tpm_state,
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
    TPM_BOOL	state;		/* State to which deactivated flag is to be set. */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back NV */
    TPM_BOOL			physicalPresence;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_PhysicalSetDeactivated: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get state parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_LoadBool(&state, &command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_PhysicalSetDeactivated: state %02x\n", state);
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
						     TPM_CHECK_ENABLED |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_CheckRequestTag0(tag);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_PhysicalSetDeactivated: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. Validate that physical presence is being asserted, if not return TPM_BAD_PRESENCE */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Global_GetPhysicalPresence(&physicalPresence, tpm_state);
    }
    if (returnCode == TPM_SUCCESS) {
	if (!physicalPresence) {
	    printf("TPM_Process_PhysicalSetDeactivated: Error, physicalPresence is FALSE\n");
	    returnCode = TPM_BAD_PRESENCE;
	}
    }
    /* 2. The TPM SHALL set the TPM_PERMANENT_FLAGS.deactivated flag to the value in the state
       parameter. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_PhysicalSetDeactivated: Setting deactivated to %u\n", state);
	TPM_SetCapability_Flag(&writeAllNV,					/* altered */
			       &(tpm_state->tpm_permanent_flags.deactivated),	/* flag */
			       state);						/* value */
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
	printf("TPM_Process_PhysicalSetDeactivated: Ordinal returnCode %08x %u\n",
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
    return rcf;
}

/* 5.6 TPM_SetTempDeactivated rev 87

   This command allows the operator of the platform to deactivate the TPM until the next boot of the
   platform.

   This command requires operator authorization. The operator can provide the authorization by
   either the assertion of physical presence or presenting the operation authorization value.
*/

TPM_RESULT TPM_Process_SetTempDeactivated(tpm_state_t *tpm_state,
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
    TPM_AUTHHANDLE	authHandle;	/* auth handle for operation validation.  Session type must
					   be OIAP. */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL		continueAuthSession = TRUE;	/* The continue use flag for the
							   authorization handle */
    TPM_AUTHDATA	operatorAuth;	/* HMAC key: operatorAuth */
    
    /* processing parameters */
    unsigned char *		inParamStart;			/* starting point of inParam's */
    unsigned char *		inParamEnd;			/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			authHandleValid = FALSE;
    TPM_AUTH_SESSION_DATA	*auth_session_data = NULL;	/* session data for authHandle */
    TPM_SECRET			*hmacKey;
#if TPM_V12
    TPM_BOOL			physicalPresence;
#endif
    
    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;
   
    printf("TPM_Process_SetTempDeactivated: Ordinal Entry\n");
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
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_ACTIVATED |
						     TPM_CHECK_NO_LOCKOUT));
    }
    /* check tag */
    if (returnCode == TPM_SUCCESS) {
#if TPM_V12
	returnCode = TPM_CheckRequestTag10(tag);
#else	/* v1.1 is always auth0.  This check implicitly bypasses the operatorAuth Actions below. */
	returnCode = TPM_CheckRequestTag0(tag);
#endif
    }
    /* get the optional 'below the line' authorization parameters */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_AuthParams_Get(&authHandle,
					&authHandleValid,
					nonceOdd,
					&continueAuthSession,
					operatorAuth,
					&command, &paramSize);
    }
    if (returnCode == TPM_SUCCESS) {
	if (paramSize != 0) {
	    printf("TPM_Process_SetTempDeactivated: Error, command has %u extra bytes\n",
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
    /* 1. If tag = TPM_TAG_REQ_AUTH1_COMMAND */
    /* a. If TPM_PERMANENT_FLAGS -> operator is FALSE return TPM_NOOPERATOR */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	if (!tpm_state->tpm_permanent_flags.tpmOperator) {
	    printf("TPM_Process_SetTempDeactivated: Error, no operator\n");
	    returnCode = TPM_NOOPERATOR;
	}
    }
    /* b. Validate command and parameters using operatorAuth, on error return TPM_AUTHFAIL */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	printf(" TPM_Process_SetTempDeactivated: authHandle %08x\n", authHandle);
	/* get the session data */
	returnCode =
	    TPM_AuthSessions_GetData(&auth_session_data,
				     &hmacKey,
				     tpm_state,
				     authHandle,
				     TPM_PID_OIAP,
				     0, /* OSAP entity type */
				     ordinal,
				     NULL,
				     &(tpm_state->tpm_permanent_data.operatorAuth),	/* OIAP */
				     NULL);						/* OSAP */
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_AUTH1_COMMAND)) {
	returnCode = TPM_Authdata_Check(tpm_state,
					*hmacKey,		/* operator HMAC key */
					inParamDigest,
					auth_session_data,	/* authorization session */
					nonceOdd,		/* Nonce generated by system
								   associated with authHandle */
					continueAuthSession,
					operatorAuth);		/* Authorization digest for input */
	
    }
#if TPM_V12	/* v1.1 does not require physical presence */
    /* 2. Else */
    /* a. If physical presence is not asserted the TPM MUST return TPM_BAD_PRESENCE */
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)) {
	returnCode = TPM_Global_GetPhysicalPresence(&physicalPresence, tpm_state);
    }
    if ((returnCode == TPM_SUCCESS) && (tag == TPM_TAG_RQU_COMMAND)) {
	if (!physicalPresence) {
	    printf("TPM_Process_SetTempDeactivated: Error, physicalPresence is FALSE\n");
	    returnCode = TPM_BAD_PRESENCE;
	}
    }
#endif
    /* 3. The TPM SHALL set the TPM_STCLEAR_FLAGS.deactivated flag to the value TRUE. */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_SetTempDeactivated: Setting deactivated to TRUE\n");
	tpm_state->tpm_stclear_flags.deactivated = TRUE;
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SetTempDeactivated: Ordinal returnCode %08x %u\n",
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
					    *hmacKey,	/* operator HMAC key */
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

/* 5.7 TPM_SetOperatorAuth rev 87

   This command allows the setting of the operator authorization value. 

   There is no confidentiality applied to the operator authorization as the value is sent under the
   assumption of being local to the platform. If there is a concern regarding the path between the
   TPM and the keyboard then unless the keyboard is using encryption and a secure channel an
   attacker can read the values.
*/

TPM_RESULT TPM_Process_SetOperatorAuth(tpm_state_t *tpm_state,
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
    TPM_SECRET operatorAuth;	/* The operator authorization */

    /* processing parameters */
    unsigned char *		inParamStart;		/* starting point of inParam's */
    unsigned char *		inParamEnd;		/* ending point of inParam's */
    TPM_DIGEST			inParamDigest;
    TPM_BOOL			auditStatus;		/* audit the ordinal */
    TPM_BOOL			transportEncrypt;	/* wrapped in encrypted transport session */
    TPM_BOOL			writeAllNV = FALSE;	/* flag to write back NV */
    TPM_BOOL			physicalPresence;

    /* output parameters */
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_SetOperatorAuth: Ordinal Entry\n");
    /*
      get inputs
    */
    /* save the starting point of inParam's for authorization and auditing */
    inParamStart = command;
    /* get operatorAuth parameter */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Secret_Load(operatorAuth, &command, &paramSize);
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
	    printf("TPM_Process_SetOperatorAuth: Error, command has %u extra bytes\n",
		   paramSize);
	    returnCode = TPM_BAD_PARAM_SIZE;
	}
    }
    /*
      Processing
    */
    /* 1. If physical presence is not asserted the TPM MUST return TPM_BAD_PRESENCE */
    if (returnCode == TPM_SUCCESS) {
	returnCode = TPM_Global_GetPhysicalPresence(&physicalPresence, tpm_state);
    }
    if (returnCode == TPM_SUCCESS) {
	if (!physicalPresence) {
	    printf("TPM_Process_SetOperatorAuth: Error, physicalPresence is FALSE\n");
	    returnCode = TPM_BAD_PRESENCE;
	}
    }
    if (returnCode == TPM_SUCCESS) {
	/* 2. The TPM SHALL set the TPM_PERSISTENT_DATA -> operatorAuth */
	TPM_Digest_Copy(tpm_state->tpm_permanent_data.operatorAuth, operatorAuth);
	/* 3. The TPM SHALL set TPM_PERMANENT_FLAGS -> operator to TRUE */
	printf("TPM_Process_SetOperatorAuth: Setting operator to TRUE\n");
	TPM_SetCapability_Flag(&writeAllNV,					/* altered */
			       &(tpm_state->tpm_permanent_flags.tpmOperator),	/* flag */
			       TRUE);						/* value */
	/* Store the permanent data and flags back to NVRAM */
	returnCode = TPM_PermanentAll_NVStore(tpm_state,
					      TRUE,
					      returnCode);
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_SetOperatorAuth: Ordinal returnCode %08x %u\n",
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
    return rcf;
}

/* 9.3 TPM_ResetLockValue rev 96

   Command that resets the TPM dictionary attack mitigation values

   This allows the TPM owner to cancel the effect of a number of successive authorization failures.

   If this command itself has an authorization failure, it is blocked for the remainder of the lock
   out period.	This prevents a dictionary attack on the owner authorization using this command.

   It is understood that this command allows the TPM owner to perform a dictionary attack on other
   authorization values by alternating a trial and this command.  Similarly, delegating this command
   allows the owner's delegate to perform a dictionary attack.
*/

TPM_RESULT TPM_Process_ResetLockValue(tpm_state_t *tpm_state,
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
    TPM_AUTHHANDLE	authHandle;	/* The authorization session handle used for TPM Owner
					   authorization */
    TPM_NONCE		nonceOdd;	/* Nonce generated by system associated with authHandle */
    TPM_BOOL	continueAuthSession = TRUE;	/* The continue use flag for the authorization
						   session handle */
    TPM_AUTHDATA	ownerAuth;	/* HMAC key TPM Owner auth */

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
    uint32_t		outParamStart;	/* starting point of outParam's */
    uint32_t		outParamEnd;	/* ending point of outParam's */
    TPM_DIGEST		outParamDigest;

    printf("TPM_Process_ResetLockValue: Ordinal Entry\n");
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
	/* Update disableResetLock.  Ignore the return code since this command is not locked out */
	TPM_Authdata_CheckState(tpm_state);
	/* NOTE No TPM_CHECK_NO_LOCKOUT, since this command proceeds anyway */
	returnCode = TPM_CheckState(tpm_state, tag, (TPM_CHECK_NOT_SHUTDOWN |
						     TPM_CHECK_ENABLED |
						     TPM_CHECK_ACTIVATED |
						     TPM_CHECK_OWNER));
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
	    printf("TPM_Process_ResetLockValue: Error, command has %u extra bytes\n", paramSize);
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
    /* 1. If TPM_STCLEAR_DATA -> disableResetLock is TRUE return TPM_AUTHFAIL */
    if (returnCode == TPM_SUCCESS) {
	if (tpm_state->tpm_stclear_data.disableResetLock) {
	    printf("TPM_Process_ResetLockValue: Error, command locked out\n");
	    returnCode = TPM_AUTHFAIL;
	}
    }	 
    /* a. The internal dictionary attack mechanism will set TPM_STCLEAR_DATA -> disableResetLock to
       FALSE when the timeout period expires */
    /* NOTE Done by TPM_Authdata_CheckState() */
    /* Validate the parameters and owner authorization for this command */
    if (returnCode == TPM_SUCCESS) {
	if (returnCode == TPM_SUCCESS) {
	    returnCode =
		TPM_AuthSessions_GetData(&auth_session_data,
					 &hmacKey,
					 tpm_state,
					 authHandle,
					 TPM_PID_NONE,
					 TPM_ET_OWNER,
					 ordinal,
					 NULL,
					 &(tpm_state->tpm_permanent_data.ownerAuth),	/* OIAP */
					 tpm_state->tpm_permanent_data.ownerAuth);	/* OSAP */
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
	/* 2. If the the command and parameters validation using ownerAuth fails */
	if (returnCode != TPM_SUCCESS) {
	    printf("TPM_Process_ResetLockValue: Error, disabling ordinal\n");
	    /* a. Set TPM_STCLEAR_DATA -> disableResetLock to TRUE */
	    tpm_state->tpm_stclear_data.disableResetLock = TRUE;
	    /* b. Restart the TPM dictionary attack lock out period */
	    /* A failure restarts it anyway with double the period.*/
	    /* c. Return TPM_AUTHFAIL */
	}
    }
    /* 3. Reset the internal TPM dictionary attack mitigation mechanism */
    /* a. The mechanism is vendor specific and can include time outs, reboots, and other mitigation
       strategies */
    if (returnCode == TPM_SUCCESS) {
	printf("TPM_Process_ResetLockValue: Resetting the failure counter\n");
	/* clear the authorization failure counter */
	tpm_state->tpm_stclear_data.authFailCount = 0;
    }
    /*
      response
    */
    /* standard response: tag, (dummy) paramSize, returnCode.  Failure is fatal. */
    if (rcf == 0) {
	printf("TPM_Process_ResetLockValue: Ordinal returnCode %08x %u\n",
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
    /* if there was an error, terminate the session. */
    if (((rcf != 0) ||
	 ((returnCode != TPM_SUCCESS) && (returnCode != TPM_DEFEND_LOCK_RUNNING)) ||
	 !continueAuthSession) &&
	authHandleValid) {
	TPM_AuthSessions_TerminateHandle(tpm_state->tpm_stclear_data.authSessions, authHandle);
    }
    return rcf;
}

