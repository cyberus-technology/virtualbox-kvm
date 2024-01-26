/********************************************************************************/
/*										*/
/*			Self-Test of Cryptographic Functions 			*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: CryptSelfTest.c $		*/
/*										*/
/*  Licenses and Notices							*/
/*										*/
/*  1. Copyright Licenses:							*/
/*										*/
/*  - Trusted Computing Group (TCG) grants to the user of the source code in	*/
/*    this specification (the "Source Code") a worldwide, irrevocable, 		*/
/*    nonexclusive, royalty free, copyright license to reproduce, create 	*/
/*    derivative works, distribute, display and perform the Source Code and	*/
/*    derivative works thereof, and to grant others the rights granted herein.	*/
/*										*/
/*  - The TCG grants to the user of the other parts of the specification 	*/
/*    (other than the Source Code) the rights to reproduce, distribute, 	*/
/*    display, and perform the specification solely for the purpose of 		*/
/*    developing products based on such documents.				*/
/*										*/
/*  2. Source Code Distribution Conditions:					*/
/*										*/
/*  - Redistributions of Source Code must retain the above copyright licenses, 	*/
/*    this list of conditions and the following disclaimers.			*/
/*										*/
/*  - Redistributions in binary form must reproduce the above copyright 	*/
/*    licenses, this list of conditions	and the following disclaimers in the 	*/
/*    documentation and/or other materials provided with the distribution.	*/
/*										*/
/*  3. Disclaimers:								*/
/*										*/
/*  - THE COPYRIGHT LICENSES SET FORTH ABOVE DO NOT REPRESENT ANY FORM OF	*/
/*  LICENSE OR WAIVER, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, WITH	*/
/*  RESPECT TO PATENT RIGHTS HELD BY TCG MEMBERS (OR OTHER THIRD PARTIES)	*/
/*  THAT MAY BE NECESSARY TO IMPLEMENT THIS SPECIFICATION OR OTHERWISE.		*/
/*  Contact TCG Administration (admin@trustedcomputinggroup.org) for 		*/
/*  information on specification licensing rights available through TCG 	*/
/*  membership agreements.							*/
/*										*/
/*  - THIS SPECIFICATION IS PROVIDED "AS IS" WITH NO EXPRESS OR IMPLIED 	*/
/*    WARRANTIES WHATSOEVER, INCLUDING ANY WARRANTY OF MERCHANTABILITY OR 	*/
/*    FITNESS FOR A PARTICULAR PURPOSE, ACCURACY, COMPLETENESS, OR 		*/
/*    NONINFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS, OR ANY WARRANTY 		*/
/*    OTHERWISE ARISING OUT OF ANY PROPOSAL, SPECIFICATION OR SAMPLE.		*/
/*										*/
/*  - Without limitation, TCG and its members and licensors disclaim all 	*/
/*    liability, including liability for infringement of any proprietary 	*/
/*    rights, relating to use of information in this specification and to the	*/
/*    implementation of this specification, and TCG disclaims all liability for	*/
/*    cost of procurement of substitute goods or services, lost profits, loss 	*/
/*    of use, loss of data or any incidental, consequential, direct, indirect, 	*/
/*    or special damages, whether under contract, tort, warranty or otherwise, 	*/
/*    arising in any way out of use or reliance upon this specification or any 	*/
/*    information herein.							*/
/*										*/
/*  (c) Copyright IBM Corp. and others, 2016 - 2020				*/
/*										*/
/********************************************************************************/

/* 10.2.7 CryptSelfTest.c */
/* 10.2.7.1 Introduction */
/* The functions in this file are designed to support self-test of cryptographic functions in the
   TPM. The TPM allows the user to decide whether to run self-test on a demand basis or to run all
   the self-tests before proceeding. */
/* The self-tests are controlled by a set of bit vectors. The g_untestedDecryptionAlgorithms vector
   has a bit for each decryption algorithm that needs to be tested and
   g_untestedEncryptionAlgorithms has a bit for each encryption algorithm that needs to be
   tested. Before an algorithm is used, the appropriate vector is checked (indexed using the
   algorithm ID). If the bit is 1, then the test function should be called. */
/* For more information, see TpmSelfTests().txt */
#include "Tpm.h"
/*     10.2.7.2 Functions */
/* 10.2.7.2.1 RunSelfTest() */
/* Local function to run self-test */
static TPM_RC
CryptRunSelfTests(
		  ALGORITHM_VECTOR    *toTest         // IN: the vector of the algorithms to test
		  )
{
    TPM_ALG_ID           alg;
    // For each of the algorithms that are in the toTestVecor, need to run a
    // test
    for(alg = TPM_ALG_FIRST; alg <= TPM_ALG_LAST; alg++)
	{
	    if(TEST_BIT(alg, *toTest))
		{
		    TPM_RC          result = CryptTestAlgorithm(alg, toTest);
		    if(result != TPM_RC_SUCCESS)
			return result;
		}
	}
    return TPM_RC_SUCCESS;
}
/* 10.2.7.2.2 CryptSelfTest() */
/* This function is called to start/complete a full self-test. If fullTest is NO, then only the
   untested algorithms will be run. If fullTest is YES, then g_untestedDecryptionAlgorithms is
   reinitialized and then all tests are run. This implementation of the reference design does not
   support processing outside the framework of a TPM command. As a consequence, this command does
   not complete until all tests are done. Since this can take a long time, the TPM will check after
   each test to see if the command is canceled. If so, then the TPM will returned
   TPM_RC_CANCELLED. To continue with the self-tests, call TPM2_SelfTest(fullTest == No) and the TPM
   will complete the testing. */
/* Error Returns Meaning */
/* TPM_RC_CANCELED if the command is canceled */
LIB_EXPORT
TPM_RC
CryptSelfTest(
	      TPMI_YES_NO      fullTest       // IN: if full test is required
	      )
{
#if SIMULATION
    if(g_forceFailureMode)
	FAIL(FATAL_ERROR_FORCED);
#endif
    // If the caller requested a full test, then reset the to test vector so that
    // all the tests will be run
    if(fullTest == YES)
	{
	    MemoryCopy(g_toTest,
		       g_implementedAlgorithms,
		       sizeof(g_toTest));
	}
    return CryptRunSelfTests(&g_toTest);
}
/* 10.2.7.2.3 CryptIncrementalSelfTest() */
/* This function is used to perform an incremental self-test. This implementation will perform the
   toTest values before returning. That is, it assumes that the TPM cannot perform background tasks
   between commands. */
/* This command may be canceled. If it is, then there is no return result. However, this command can
   be run again and the incremental progress will not be lost. */
/* Error Returns Meaning */
/* TPM_RC_CANCELED processing of this command was canceled */
/* TPM_RC_TESTING if toTest list is not empty */
/* TPM_RC_VALUE an algorithm in the toTest list is not implemented */
TPM_RC
CryptIncrementalSelfTest(
			 TPML_ALG            *toTest,        // IN: list of algorithms to be tested
			 TPML_ALG            *toDoList       // OUT: list of algorithms needing test
			 )
{
    ALGORITHM_VECTOR     toTestVector = {0};
    TPM_ALG_ID           alg;
    UINT32               i;
    pAssert(toTest != NULL && toDoList != NULL);
    if(toTest->count > 0)
	{
	    // Transcribe the toTest list into the toTestVector
	    for(i = 0; i < toTest->count; i++)
		{
		    alg = toTest->algorithms[i];
		    // make sure that the algorithm value is not out of range
		    if((alg > TPM_ALG_LAST) || !TEST_BIT(alg, g_implementedAlgorithms))
			return TPM_RC_VALUE;
		    SET_BIT(alg, toTestVector);
		}
	    // Run the test
	    if(CryptRunSelfTests(&toTestVector) == TPM_RC_CANCELED)
		return TPM_RC_CANCELED;
	}
    // Fill in the toDoList with the algorithms that are still untested
    toDoList->count = 0;
    for(alg = TPM_ALG_FIRST;
	toDoList->count < MAX_ALG_LIST_SIZE && alg <= TPM_ALG_LAST;
	alg++)
	{
	    if(TEST_BIT(alg, g_toTest))
		toDoList->algorithms[toDoList->count++] = alg;
	}
    return TPM_RC_SUCCESS;
}
/* 10.2.7.2.4 CryptInitializeToTest() */
/* This function will initialize the data structures for testing all the algorithms. This should not
   be called unless CryptAlgsSetImplemented() has been called */
void
CryptInitializeToTest(
		      void
		      )
{
    // Indicate that nothing has been tested
    memset(&g_cryptoSelfTestState, 0, sizeof(g_cryptoSelfTestState));
    // Copy the implemented algorithm vector
    MemoryCopy(g_toTest, g_implementedAlgorithms, sizeof(g_toTest));
    // Setting the algorithm to null causes the test function to just clear
    // out any algorithms for which there is no test.
    CryptTestAlgorithm(TPM_ALG_ERROR, &g_toTest);
    return;
}
/* 10.2.7.2.5 CryptTestAlgorithm() */
/* Only point of contact with the actual self tests. If a self-test fails, there is no return and
   the TPM goes into failure mode. The call to TestAlgorithm() uses an algorithm selector and a bit
   vector. When the test is run, the corresponding bit in toTest and in g_toTest is CLEAR. If toTest
   is NULL, then only the bit in g_toTest is CLEAR. There is a special case for the call to
   TestAlgorithm(). When alg is TPM_ALG_ERROR, TestAlgorithm() will CLEAR any bit in toTest for
   which it has no test. This allows the knowledge about which algorithms have test to be accessed
   through the interface that provides the test. */
/* Error Returns Meaning */
/* TPM_RC_CANCELED test was canceled */
LIB_EXPORT
TPM_RC
CryptTestAlgorithm(
		   TPM_ALG_ID           alg,
		   ALGORITHM_VECTOR    *toTest
		   )
{
    TPM_RC                   result;
#if SELF_TEST
    result = TestAlgorithm(alg, toTest);
#else
    // If this is an attempt to determine the algorithms for which there is a
    // self test, pretend that all of them do. We do that by not clearing any
    // of the algorithm bits. When/if this function is called to run tests, it
    // will over report. This can be changed so that any call to check on which
    // algorithms have tests, 'toTest' can be cleared.
    if(alg != TPM_ALG_ERROR)
	{
	    CLEAR_BIT(alg, g_toTest);
	    if(toTest != NULL)
		CLEAR_BIT(alg, *toTest);
	}
    result = TPM_RC_SUCCESS;
#endif
    return result;
}
