/********************************************************************************/
/*										*/
/*			  Code to perform the various self-test functions.	*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: AlgorithmTests.c $	*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2021				*/
/*										*/
/********************************************************************************/

/* 10.2.1 AlgorithmTests.c */
/* 10.2.1.1 Introduction */
/* This file contains the code to perform the various self-test functions. */
/* 10.2.1.2 Includes and Defines */
#include    "Tpm.h"
#define     SELF_TEST_DATA
#if SELF_TEST
/* These includes pull in the data structures. They contain data definitions for the various
   tests. */
#include    "SelfTest.h"
#include    "SymmetricTest.h"
#include    "RsaTestData.h"
#include    "EccTestData.h"
#include    "HashTestData.h"
#include    "KdfTestData.h"
#define TEST_DEFAULT_TEST_HASH(vector)					\
    if(TEST_BIT(DEFAULT_TEST_HASH, g_toTest))				\
	TestHash(DEFAULT_TEST_HASH, vector);
/* Make sure that the algorithm has been tested */
#define CLEAR_BOTH(alg)     {   CLEAR_BIT(alg, *toTest);		\
	if(toTest != &g_toTest)						\
	    CLEAR_BIT(alg, g_toTest); }
#define SET_BOTH(alg)     {   SET_BIT(alg, *toTest);			\
	if(toTest != &g_toTest)						\
	    SET_BIT(alg, g_toTest); }
#define TEST_BOTH(alg)       ((toTest != &g_toTest)			\
			      ? TEST_BIT(alg, *toTest) || TEST_BIT(alg, g_toTest) \
			      : TEST_BIT(alg, *toTest))
/* Can only cancel if doing a list. */
#define CHECK_CANCELED							\
    if(_plat__IsCanceled() && toTest != &g_toTest)			\
	return TPM_RC_CANCELED;
/* 10.2.1.3 Hash Tests */
/* 10.2.1.3.1 Description */
/* The hash test does a known-value HMAC using the specified hash algorithm. */
/* 10.2.1.3.2 TestHash() */
/* The hash test function. */
static TPM_RC
TestHash(
	 TPM_ALG_ID          hashAlg,
	 ALGORITHM_VECTOR    *toTest
	 )
{
    TPM2B_DIGEST      computed;  // value computed
    HMAC_STATE        state;
    UINT16            digestSize;
    const TPM2B       *testDigest = NULL;
    //    TPM2B_TYPE(HMAC_BLOCK, DEFAULT_TEST_HASH_BLOCK_SIZE);
    pAssert(hashAlg != TPM_ALG_NULL);
#define HASH_CASE_FOR_TEST(HASH, hash)        case ALG_##HASH##_VALUE:	\
    testDigest = &c_##HASH##_digest.b;					\
    break;
    switch(hashAlg)
	{
	    FOR_EACH_HASH(HASH_CASE_FOR_TEST)

	  default:
	    FAIL(FATAL_ERROR_INTERNAL);
	}
    // Clear the to-test bits
    CLEAR_BOTH(hashAlg);

    // If there is an algorithm without test vectors, then assume that things are OK.
    if(testDigest == NULL || testDigest->size == 0)
	return TPM_RC_SUCCESS;

    // Set the HMAC key to twice the digest size
    digestSize = CryptHashGetDigestSize(hashAlg);
    CryptHmacStart(&state, hashAlg, digestSize * 2,
		   (BYTE *)c_hashTestKey.t.buffer);
    CryptDigestUpdate(&state.hashState, 2 * CryptHashGetBlockSize(hashAlg),
		      (BYTE *)c_hashTestData.t.buffer);
    computed.t.size = digestSize;
    CryptHmacEnd(&state, digestSize, computed.t.buffer);
    if((testDigest->size != computed.t.size)
       || (memcmp(testDigest->buffer, computed.t.buffer, computed.b.size) != 0)) {
	SELF_TEST_FAILURE;
    }
    return TPM_RC_SUCCESS;
}
// libtpms added begin
#if SMAC_IMPLEMENTED && ALG_CMAC
static TPM_RC
TestSMAC(
	 ALGORITHM_VECTOR    *toTest
	 )
{
    HMAC_STATE          state;
    UINT16              copied;
    BYTE                out[MAX_SYM_BLOCK_SIZE];
    UINT32              outSize = sizeof(out);
    UINT16              blocksize;
    int                 i;
    TPMU_PUBLIC_PARMS   cmac_keyParms;

    // initializing this statically seems impossible with gcc...
    cmac_keyParms.symDetail.sym.algorithm = TPM_ALG_AES;
    cmac_keyParms.symDetail.sym.keyBits.sym = 128;

    for (i = 0; CMACTests[i].key; i++ )
	{
	    blocksize = CryptMacStart(&state, &cmac_keyParms,
				      TPM_ALG_CMAC, CMACTests[i].key);
	    pAssert(blocksize <= outSize);
	    CryptDigestUpdate(&state.hashState, CMACTests[i].datalen,
			      CMACTests[i].data);
	    copied = CryptMacEnd(&state, outSize, out);
	    if((CMACTests[i].outlen != copied)
	      || (memcmp(out, CMACTests[i].out, CMACTests[i].outlen) != 0)) {
		SELF_TEST_FAILURE;
	    }
	}
    return TPM_RC_SUCCESS;
}
#endif
// libtpms added end
/* 10.2.1.4 Symmetric Test Functions */
/* 10.2.1.4.1 MakeIv() */
/* Internal function to make the appropriate IV depending on the mode. */
static UINT32
MakeIv(
       TPM_ALG_ID    mode,     // IN: symmetric mode
       UINT32        size,     // IN: block size of the algorithm
       BYTE         *iv        // OUT: IV to fill in
       )
{
    BYTE          i;
    if(mode == TPM_ALG_ECB)
	return 0;
    if(mode == TPM_ALG_CTR)
	{
	    // The test uses an IV that has 0xff in the last byte
	    for(i = 1; i <= size; i++)
		*iv++ = 0xff - (BYTE)(size - i);
	}
    else
	{
	    for(i = 0; i < size; i++)
		*iv++ = i;
	}
    return size;
}
/* 10.2.1.4.2 TestSymmetricAlgorithm() */
/* Function to test a specific algorithm, key size, and mode. */
static void
TestSymmetricAlgorithm(
		       const SYMMETRIC_TEST_VECTOR     *test,          //
		       TPM_ALG_ID                       mode           //
		       )
{
    BYTE                            encrypted[MAX_SYM_BLOCK_SIZE * 2];
    BYTE                            decrypted[MAX_SYM_BLOCK_SIZE * 2];
    TPM2B_IV                        iv;

    // libtpms added beging
    if (test->dataOut[mode - TPM_ALG_CTR] == NULL)
        return;
    // libtpms added end

    //
    // Get the appropriate IV
    iv.t.size = (UINT16)MakeIv(mode, test->ivSize, iv.t.buffer);
    // Encrypt known data
    CryptSymmetricEncrypt(encrypted, test->alg, test->keyBits, test->key, &iv,
			  mode, test->dataInOutSize, test->dataIn);
    // Check that it matches the expected value
    if(!MemoryEqual(encrypted, test->dataOut[mode - TPM_ALG_CTR],
		    test->dataInOutSize)) {
	SELF_TEST_FAILURE;
    }
    // Reinitialize the iv for decryption
    MakeIv(mode, test->ivSize, iv.t.buffer);
    CryptSymmetricDecrypt(decrypted, test->alg, test->keyBits, test->key, &iv,
			  mode, test->dataInOutSize,
			  test->dataOut[mode - TPM_ALG_CTR]);
    // Make sure that it matches what we started with
    if(!MemoryEqual(decrypted, test->dataIn, test->dataInOutSize)) {
	SELF_TEST_FAILURE;
    }
}
/* 10.2.1.4.3 AllSymsAreDone() */
/* Checks if both symmetric algorithms have been tested. This is put here so that addition of a
   symmetric algorithm will be relatively easy to handle */
/* Return Value	Meaning */
/* TRUE(1)	all symmetric algorithms tested */
/* FALSE(0)	not all symmetric algorithms tested */
static BOOL
AllSymsAreDone(
	       ALGORITHM_VECTOR        *toTest
	       )
{
    return (!TEST_BOTH(TPM_ALG_AES) && !TEST_BOTH(TPM_ALG_SM4));
}
/* 10.2.1.4.4 AllModesAreDone() */
/* Checks if all the modes have been tested */
/* Return Value	Meaning */
/* TRUE(1)	all modes tested */
/* FALSE(0)	all modes not tested */
static BOOL
AllModesAreDone(
		ALGORITHM_VECTOR            *toTest
		)
{
    TPM_ALG_ID                  alg;
    for(alg = SYM_MODE_FIRST; alg <= SYM_MODE_LAST; alg++)
	if(TEST_BOTH(alg))
	    return FALSE;
    return TRUE;
}
/* 10.2.1.4.5 TestSymmetric() */
/* If alg is a symmetric block cipher, then all of the modes that are selected are tested. If alg is
   a mode, then all algorithms of that mode are tested. */
static TPM_RC
TestSymmetric(
	      TPM_ALG_ID                   alg,
	      ALGORITHM_VECTOR            *toTest
	      )
{
    SYM_INDEX                    index;
    TPM_ALG_ID                   mode;
    //
    if(!TEST_BIT(alg, *toTest))
	return TPM_RC_SUCCESS;
    if(alg == TPM_ALG_AES || alg == TPM_ALG_SM4 || alg == TPM_ALG_CAMELLIA || alg == TPM_ALG_TDES)
	{
	    // Will test the algorithm for all modes and key sizes
	    CLEAR_BOTH(alg);
	    // A test this algorithm for all modes
	    for(index = 0; index < NUM_SYMS; index++)
		{
		    if(c_symTestValues[index].alg == alg)
			{
			    for(mode = SYM_MODE_FIRST;
				mode <= SYM_MODE_LAST;
				mode++)
				{
				    if(TEST_BIT(mode, g_implementedAlgorithms)) // libtpms always test implemented modes
					TestSymmetricAlgorithm(&c_symTestValues[index], mode);
				}
			}
		}
	    // if all the symmetric tests are done
	    if(AllSymsAreDone(toTest))
		{
		    // all symmetric algorithms tested so no modes should be set
		    for(alg = SYM_MODE_FIRST; alg <= SYM_MODE_LAST; alg++)
			CLEAR_BOTH(alg);
		}
	}
    else if(SYM_MODE_FIRST <= alg && alg <= SYM_MODE_LAST)
	{
	    // Test this mode for all key sizes and algorithms
	    for(index = 0; index < NUM_SYMS; index++)
		{
		    // The mode testing only comes into play when doing self tests
		    // by command. When doing self tests by command, the block ciphers are
		    // tested first. That means that all of their modes would have been
		    // tested for all key sizes. If there is no block cipher left to
		    // test, then clear this mode bit.
		    if(!TEST_BIT(TPM_ALG_AES, *toTest)
		       && !TEST_BIT(TPM_ALG_SM4, *toTest))
			{
			    CLEAR_BOTH(alg);
			}
		    else
			{
			    for(index = 0; index < NUM_SYMS; index++)
				{
				    if(TEST_BIT(c_symTestValues[index].alg, *toTest))
					TestSymmetricAlgorithm(&c_symTestValues[index], alg);
				}
			    // have tested this mode for all algorithms
			    CLEAR_BOTH(alg);
			}
		}
	    if(AllModesAreDone(toTest))
		{
		    CLEAR_BOTH(TPM_ALG_AES);
		    CLEAR_BOTH(TPM_ALG_SM4);
		}
	}
    else
	pAssert(alg == 0 && alg != 0);
    return TPM_RC_SUCCESS;
}
/* 10.2.1.5 RSA Tests */
#if ALG_RSA
/* 10.2.1.5.1 Introduction */
/* The tests are for public key only operations and for private key operations. Signature
   verification and encryption are public key operations. They are tested by using a KVT. For
   signature verification, this means that a known good signature is checked by
   CryptRsaValidateSignature(). If it fails, then the TPM enters failure mode. For encryption, the
   TPM encrypts known values using the selected scheme and checks that the returned value matches
   the expected value. */
/* For private key operations, a full scheme check is used. For a signing key, a known key is used
   to sign a known message. Then that signature is verified. since the signature may involve use of
   random values, the signature will be different each time and we can't always check that the
   signature matches a known value. The same technique is used for decryption (RSADP/RSAEP). */
/* When an operation uses the public key and the verification has not been tested, the TPM will do a
   KVT. */
/* The test for the signing algorithm is built into the call for the algorithm */
/* 10.2.1.5.2 RsaKeyInitialize() */
/* The test key is defined by a public modulus and a private prime. The TPM's RSA code computes the
   second prime and the private exponent. */
static void
RsaKeyInitialize(
		 OBJECT          *testObject
		 )
{
    MemoryCopy2B(&testObject->publicArea.unique.rsa.b, (P2B)&c_rsaPublicModulus,
		 sizeof(c_rsaPublicModulus));
    MemoryCopy2B(&testObject->sensitive.sensitive.rsa.b, (P2B)&c_rsaPrivatePrime,
		 sizeof(testObject->sensitive.sensitive.rsa.t.buffer));
    testObject->publicArea.parameters.rsaDetail.keyBits = RSA_TEST_KEY_SIZE * 8;
    // Use the default exponent
    testObject->publicArea.parameters.rsaDetail.exponent = 0;
    testObject->attributes.privateExp = 0;
}
/* 10.2.1.5.3 TestRsaEncryptDecrypt() */
/* These tests are for a public key encryption that uses a random value. */
static TPM_RC
TestRsaEncryptDecrypt(
		      TPM_ALG_ID           scheme,            // IN: the scheme
		      ALGORITHM_VECTOR    *toTest             //
		      )
{
    TPM2B_PUBLIC_KEY_RSA             testInput;
    TPM2B_PUBLIC_KEY_RSA             testOutput;
    OBJECT                           testObject;
    const TPM2B_RSA_TEST_KEY        *kvtValue = NULL;
    TPM_RC                           result = TPM_RC_SUCCESS;
    const TPM2B                     *testLabel = NULL;
    TPMT_RSA_DECRYPT                 rsaScheme;
    //
    // Don't need to initialize much of the test object but do need to initialize
    // the flag indicating that the private exponent has been computed.
    testObject.attributes.privateExp = CLEAR;
    RsaKeyInitialize(&testObject);
    rsaScheme.scheme = scheme;
    rsaScheme.details.anySig.hashAlg = DEFAULT_TEST_HASH;
    CLEAR_BOTH(scheme);
    CLEAR_BOTH(TPM_ALG_NULL);
    if(scheme == TPM_ALG_NULL)
	{
	    // This is an encryption scheme using the private key without any encoding.
	    memcpy(testInput.t.buffer, c_RsaTestValue, sizeof(c_RsaTestValue));
	    testInput.t.size = sizeof(c_RsaTestValue);
	    if(TPM_RC_SUCCESS != CryptRsaEncrypt(&testOutput, &testInput.b,
						 &testObject, &rsaScheme, NULL, NULL)) {
		SELF_TEST_FAILURE;
	    }
	    if(!MemoryEqual(testOutput.t.buffer, c_RsaepKvt.buffer, c_RsaepKvt.size)) {
		SELF_TEST_FAILURE;
	    }
	    MemoryCopy2B(&testInput.b, &testOutput.b, sizeof(testInput.t.buffer));
	    if(TPM_RC_SUCCESS != CryptRsaDecrypt(&testOutput.b, &testInput.b,
						 &testObject, &rsaScheme, NULL)) {
		SELF_TEST_FAILURE;
	    }
	    if(!MemoryEqual(testOutput.t.buffer, c_RsaTestValue,
			    sizeof(c_RsaTestValue))) {
		SELF_TEST_FAILURE;
	    }
	}
    else
	{
	    // TPM_ALG_RSAES:
	    // This is an decryption scheme using padding according to
	    // PKCS#1v2.1, 7.2. This padding uses random bits. To test a public
	    // key encryption that uses random data, encrypt a value and then
	    // decrypt the value and see that we get the encrypted data back.
	    // The hash is not used by this encryption so it can be TMP_ALG_NULL
	    // TPM_ALG_OAEP_:
	    // This is also an decryption scheme and it also uses a
	    // pseudo-random
	    // value. However, this also uses a hash algorithm. So, we may need
	    // to test that algorithm before use.
	    if(scheme == TPM_ALG_OAEP)
		{
		    TEST_DEFAULT_TEST_HASH(toTest);
		    kvtValue = &c_OaepKvt;
		    testLabel = OAEP_TEST_STRING;
		}
	    else if(scheme == TPM_ALG_RSAES)
		{
		    kvtValue = &c_RsaesKvt;
		    testLabel = NULL;
		}
	    else {
		SELF_TEST_FAILURE;
	    }
	    // Only use a digest-size portion of the test value
	    memcpy(testInput.t.buffer, c_RsaTestValue, DEFAULT_TEST_DIGEST_SIZE);
	    testInput.t.size = DEFAULT_TEST_DIGEST_SIZE;
	    // See if the encryption works
	    if(TPM_RC_SUCCESS != CryptRsaEncrypt(&testOutput, &testInput.b,
						 &testObject, &rsaScheme, testLabel,
						 NULL)) {
		SELF_TEST_FAILURE;
	    }
	    MemoryCopy2B(&testInput.b, &testOutput.b, sizeof(testInput.t.buffer));
	    // see if we can decrypt this value and get the original data back
	    if(TPM_RC_SUCCESS != CryptRsaDecrypt(&testOutput.b, &testInput.b,
						 &testObject, &rsaScheme, testLabel)) {
		SELF_TEST_FAILURE;
	    }
	    // See if the results compare
	    if(testOutput.t.size != DEFAULT_TEST_DIGEST_SIZE
	       || !MemoryEqual(testOutput.t.buffer, c_RsaTestValue,
			       DEFAULT_TEST_DIGEST_SIZE)) {
		SELF_TEST_FAILURE;
	    }
	    // Now check that the decryption works on a known value
	    MemoryCopy2B(&testInput.b, (P2B)kvtValue,
			 sizeof(testInput.t.buffer));
	    if(TPM_RC_SUCCESS != CryptRsaDecrypt(&testOutput.b, &testInput.b,
						 &testObject, &rsaScheme, testLabel)) {
		SELF_TEST_FAILURE;
	    }
	    if(testOutput.t.size != DEFAULT_TEST_DIGEST_SIZE
	       || !MemoryEqual(testOutput.t.buffer, c_RsaTestValue,
			       DEFAULT_TEST_DIGEST_SIZE)) {
		SELF_TEST_FAILURE;
	    }
	}
    return result;
}
/* 10.2.1.5.4 TestRsaSignAndVerify() */
/* This function does the testing of the RSA sign and verification functions. This test does a
   KVT. */
static TPM_RC
TestRsaSignAndVerify(
		     TPM_ALG_ID               scheme,
		     ALGORITHM_VECTOR        *toTest
		     )
{
    TPM_RC                      result = TPM_RC_SUCCESS;
    OBJECT                      testObject;
    TPM2B_DIGEST                testDigest;
    TPMT_SIGNATURE              testSig;
    // Do a sign and signature verification.
    // RSASSA:
    // This is a signing scheme according to PKCS#1-v2.1 8.2. It does not
    // use random data so there is a KVT for the signing operation. On
    // first use of the scheme for signing, use the TPM's RSA key to
    // sign a portion of c_RsaTestData and compare the results to c_RsassaKvt. Then
    // decrypt the data to see that it matches the starting value. This verifies
    // the signature with a KVT
    // Clear the bits indicating that the function has not been checked. This is to
    // prevent looping
    CLEAR_BOTH(scheme);
    CLEAR_BOTH(TPM_ALG_NULL);
    CLEAR_BOTH(TPM_ALG_RSA);
    RsaKeyInitialize(&testObject);
    memcpy(testDigest.t.buffer, (BYTE *)c_RsaTestValue, DEFAULT_TEST_DIGEST_SIZE);
    testDigest.t.size = DEFAULT_TEST_DIGEST_SIZE;
    testSig.sigAlg = scheme;
    testSig.signature.rsapss.hash = DEFAULT_TEST_HASH;
    // RSAPSS:
    // This is a signing scheme a according to PKCS#1-v2.2 8.1 it uses
    // random data in the signature so there is no KVT for the signing
    // operation. To test signing, the TPM will use the TPM's RSA key
    // to sign a portion of c_RsaTestValue and then it will verify the
    // signature. For verification, c_RsapssKvt is verified before the
    // user signature blob is verified. The worst case for testing of this
    // algorithm is two private and one public key operation.
    // The process is to sign known data. If RSASSA is being done, verify that the
    // signature matches the precomputed value. For both, use the signed value and
    // see that the verification says that it is a good signature. Then
    // if testing RSAPSS, do a verify of a known good signature. This ensures that
    // the validation function works.
    if(TPM_RC_SUCCESS != CryptRsaSign(&testSig, &testObject, &testDigest, NULL)) {
	SELF_TEST_FAILURE;
    }
    // For RSASSA, make sure the results is what we are looking for
    if(testSig.sigAlg == TPM_ALG_RSASSA)
	{
	    if(testSig.signature.rsassa.sig.t.size != RSA_TEST_KEY_SIZE
	       || !MemoryEqual(c_RsassaKvt.buffer,
			       testSig.signature.rsassa.sig.t.buffer,
			       RSA_TEST_KEY_SIZE)) {
		SELF_TEST_FAILURE;
	    }
	}
    // See if the TPM will validate its own signatures
    if(TPM_RC_SUCCESS != CryptRsaValidateSignature(&testSig, &testObject,
						   &testDigest)) {
	SELF_TEST_FAILURE;
    }
    // If this is RSAPSS, check the verification with known signature
    // Have to copy because  CrytpRsaValidateSignature() eats the signature
    if(TPM_ALG_RSAPSS == scheme)
	{
	    MemoryCopy2B(&testSig.signature.rsapss.sig.b, (P2B)&c_RsapssKvt,
			 sizeof(testSig.signature.rsapss.sig.t.buffer));
	    if(TPM_RC_SUCCESS != CryptRsaValidateSignature(&testSig, &testObject,
							   &testDigest)) {
		SELF_TEST_FAILURE;
	    }
	}
    return result;
}
/* 10.2.1.5.5 TestRSA() */
/* Function uses the provided vector to indicate which tests to run. It will clear the vector after
   each test is run and also clear g_toTest */
static TPM_RC
TestRsa(
	TPM_ALG_ID               alg,
	ALGORITHM_VECTOR        *toTest
	)
{
    TPM_RC                  result = TPM_RC_SUCCESS;
    //
    switch(alg)
	{
	  case TPM_ALG_NULL:
	    // This is the RSAEP/RSADP function. If we are processing a list, don't
	    // need to test these now because any other test will validate
	    // RSAEP/RSADP. Can tell this is list of test by checking to see if
	    // 'toTest' is pointing at g_toTest. If so, this is an isolated test
	    // an need to go ahead and do the test;
	    if((toTest == &g_toTest)
	       || (!TEST_BIT(TPM_ALG_RSASSA, *toTest)
		   && !TEST_BIT(TPM_ALG_RSAES, *toTest)
		   && !TEST_BIT(TPM_ALG_RSAPSS, *toTest)
		   && !TEST_BIT(TPM_ALG_OAEP, *toTest)))
		// Not running a list of tests or no other tests on the list
		// so run the test now
		result = TestRsaEncryptDecrypt(alg, toTest);
	    // if not running the test now, leave the bit on, just in case things
	    // get interrupted
	    break;
	  case TPM_ALG_OAEP:
	  case TPM_ALG_RSAES:
	    result = TestRsaEncryptDecrypt(alg, toTest);
	    break;
	  case TPM_ALG_RSAPSS:
	  case TPM_ALG_RSASSA:
	    result = TestRsaSignAndVerify(alg, toTest);
	    break;
	  default:
	    SELF_TEST_FAILURE;
	}
    return result;
}
#endif // TPM_ALG_RSA
/* 10.2.1.6 ECC Tests */
#if ALG_ECC
/* 10.2.1.6.1 LoadEccParameter() */
/* This function is mostly for readability and type checking */
static void
LoadEccParameter(
		 TPM2B_ECC_PARAMETER          *to,       // target
		 const TPM2B_EC_TEST          *from      // source
		 )
{
    MemoryCopy2B(&to->b, &from->b, sizeof(to->t.buffer));
}
/* 10.2.1.6.2 LoadEccPoint() */
static void
LoadEccPoint(
	     TPMS_ECC_POINT               *point,       // target
	     const TPM2B_EC_TEST          *x,             // source
	     const TPM2B_EC_TEST           *y
	     )
{
    MemoryCopy2B(&point->x.b, (TPM2B *)x, sizeof(point->x.t.buffer));
    MemoryCopy2B(&point->y.b, (TPM2B *)y, sizeof(point->y.t.buffer));
}
/* 10.2.1.6.3 TestECDH() */
/* This test does a KVT on a point multiply. */
static TPM_RC
TestECDH(
	 TPM_ALG_ID          scheme,         // IN: for consistency
	 ALGORITHM_VECTOR    *toTest         // IN/OUT: modified after test is run
	 )
{
    TPMS_ECC_POINT          Z;
    TPMS_ECC_POINT          Qe;
    TPM2B_ECC_PARAMETER     ds;
    TPM_RC                  result = TPM_RC_SUCCESS;
    //
    NOT_REFERENCED(scheme);
    CLEAR_BOTH(TPM_ALG_ECDH);
    LoadEccParameter(&ds, &c_ecTestKey_ds);
    LoadEccPoint(&Qe, &c_ecTestKey_QeX, &c_ecTestKey_QeY);
    if(TPM_RC_SUCCESS != CryptEccPointMultiply(&Z, c_testCurve, &Qe, &ds,
					       NULL, NULL)) {
	SELF_TEST_FAILURE;
    }
    if(!MemoryEqual2B(&c_ecTestEcdh_X.b, &Z.x.b)
       || !MemoryEqual2B(&c_ecTestEcdh_Y.b, &Z.y.b)) {
	SELF_TEST_FAILURE;
    }
    return result;
}
/* 10.2.1.6.4	TestEccSignAndVerify() */
static TPM_RC
TestEccSignAndVerify(
		     TPM_ALG_ID                   scheme,
		     ALGORITHM_VECTOR            *toTest
		     )
{
    OBJECT                       testObject;
    TPMT_SIGNATURE               testSig;
    TPMT_ECC_SCHEME              eccScheme;
    testSig.sigAlg = scheme;
    testSig.signature.ecdsa.hash = DEFAULT_TEST_HASH;
    eccScheme.scheme = scheme;
    eccScheme.details.anySig.hashAlg = DEFAULT_TEST_HASH;
    CLEAR_BOTH(scheme);
    CLEAR_BOTH(TPM_ALG_ECDH);
    // ECC signature verification testing uses a KVT.
    switch(scheme)
	{
	  case TPM_ALG_ECDSA:
	    LoadEccParameter(&testSig.signature.ecdsa.signatureR, &c_TestEcDsa_r);
	    LoadEccParameter(&testSig.signature.ecdsa.signatureS, &c_TestEcDsa_s);
	    break;
	  case TPM_ALG_ECSCHNORR:
	    LoadEccParameter(&testSig.signature.ecschnorr.signatureR,
			     &c_TestEcSchnorr_r);
	    LoadEccParameter(&testSig.signature.ecschnorr.signatureS,
			     &c_TestEcSchnorr_s);
	    break;
	  case TPM_ALG_SM2:
	    // don't have a test for SM2
	    return TPM_RC_SUCCESS;
	  default:
	    SELF_TEST_FAILURE;
	    break;
	}
    TEST_DEFAULT_TEST_HASH(toTest);
    // Have to copy the key. This is because the size used in the test vectors
    // is the size of the ECC parameter for the test key while the size of a point
    // is TPM dependent
    MemoryCopy2B(&testObject.sensitive.sensitive.ecc.b, &c_ecTestKey_ds.b,
		 sizeof(testObject.sensitive.sensitive.ecc.t.buffer));
    LoadEccPoint(&testObject.publicArea.unique.ecc, &c_ecTestKey_QsX,
		 &c_ecTestKey_QsY);
    testObject.publicArea.parameters.eccDetail.curveID = c_testCurve;
    if(TPM_RC_SUCCESS != CryptEccValidateSignature(&testSig, &testObject,
						   (TPM2B_DIGEST *)&c_ecTestValue.b))
	{
	    SELF_TEST_FAILURE;
	}
    CHECK_CANCELED;
    // Now sign and verify some data
    if(TPM_RC_SUCCESS != CryptEccSign(&testSig, &testObject,
				      (TPM2B_DIGEST *)&c_ecTestValue,
				      &eccScheme, NULL)) {
	SELF_TEST_FAILURE;
    }
    CHECK_CANCELED;
    if(TPM_RC_SUCCESS != CryptEccValidateSignature(&testSig, &testObject,
						   (TPM2B_DIGEST *)&c_ecTestValue)) {
	SELF_TEST_FAILURE;
    }
    CHECK_CANCELED;
    return TPM_RC_SUCCESS;
}
/* 10.2.1.6.5	TestKDFa() */

static TPM_RC
TestKDFa(
	 ALGORITHM_VECTOR        *toTest
	 )
{
    static TPM2B_KDF_TEST_KEY   keyOut;
    UINT32                      counter = 0;
    //
    CLEAR_BOTH(TPM_ALG_KDF1_SP800_108);
    keyOut.t.size = CryptKDFa(KDF_TEST_ALG, &c_kdfTestKeyIn.b, &c_kdfTestLabel.b,
			      &c_kdfTestContextU.b, &c_kdfTestContextV.b,
			      TEST_KDF_KEY_SIZE * 8, keyOut.t.buffer,
			      &counter, FALSE);
    if (   keyOut.t.size != TEST_KDF_KEY_SIZE
	   || !MemoryEqual(keyOut.t.buffer, c_kdfTestKeyOut.t.buffer,
			   TEST_KDF_KEY_SIZE))
	SELF_TEST_FAILURE;
    return TPM_RC_SUCCESS;
}
/* 10.2.1.6.6	TestEcc() */
static TPM_RC
TestEcc(
	TPM_ALG_ID              alg,
	ALGORITHM_VECTOR        *toTest
	)
{
    TPM_RC                  result = TPM_RC_SUCCESS;
    NOT_REFERENCED(toTest);
    switch(alg)
	{
	  case TPM_ALG_ECC:
	  case TPM_ALG_ECDH:
	    // If this is in a loop then see if another test is going to deal with
	    // this.
	    // If toTest is not a self-test list
	    if((toTest == &g_toTest)
	       // or this is the only ECC test in the list
	       || !(TEST_BIT(TPM_ALG_ECDSA, *toTest)
		    || TEST_BIT(TPM_ALG_ECSCHNORR, *toTest)
		    || TEST_BIT(TPM_ALG_SM2, *toTest)))
		{
		    result = TestECDH(alg, toTest);
		}
	    break;
	  case TPM_ALG_ECDSA:
	  case TPM_ALG_ECSCHNORR:
	  case TPM_ALG_SM2:
	    result = TestEccSignAndVerify(alg, toTest);
	    break;
	  default:
	    SELF_TEST_FAILURE;
	    break;
	}
    return result;
}
#endif // TPM_ALG_ECC
/* 10.2.1.6.4 TestAlgorithm() */
/* Dispatches to the correct test function for the algorithm or gets a list of testable
   algorithms. */
/* If toTest is not NULL, then the test decisions are based on the algorithm selections in
   toTest. Otherwise, g_toTest is used. When bits are clear in g_toTest they will also be cleared
   toTest. */
/* If there doesn't happen to be a test for the algorithm, its associated bit is quietly cleared. */
/* If alg is zero (TPM_ALG_ERROR), then the toTest vector is cleared of any bits for which there is
   no test (i.e. no tests are actually run but the vector is cleared). */
/* NOTE: toTest will only ever have bits set for implemented algorithms but alg can be anything. */
/* Error Returns Meaning */
/* TPM_RC_CANCELED test was canceled */
LIB_EXPORT
TPM_RC
TestAlgorithm(
	      TPM_ALG_ID               alg,
	      ALGORITHM_VECTOR        *toTest
	      )
{
    TPM_ALG_ID              first = (alg == TPM_ALG_ERROR) ? TPM_ALG_FIRST : alg;
    TPM_ALG_ID              last = (alg == TPM_ALG_ERROR) ? TPM_ALG_LAST : alg;
    BOOL                    doTest = (alg != TPM_ALG_ERROR);
    TPM_RC                  result = TPM_RC_SUCCESS;
    if(toTest == NULL)
	toTest = &g_toTest;
    // This is kind of strange. This function will either run a test of the selected
    // algorithm or just clear a bit if there is no test for the algorithm. So,
    // either this loop will be executed once for the selected algorithm or once for
    // each of the possible algorithms. If it is executed more than once ('alg' ==
    // TPM_ALG_ERROR), then no test will be run but bits will be cleared for
    // unimplemented algorithms. This was done this way so that there is only one
    // case statement with all of the algorithms. It was easier to have one case
    // statement than to have multiple ones to manage whenever an algorithm ID is
    // added.
    for(alg = first; (alg <= last); alg++)
	{
	    // if 'alg' was TPM_ALG_ERROR, then we will be cycling through
	    // values, some of which may not be implemented. If the bit in toTest
	    // happens to be set, then we could either generated an assert, or just
	    // silently CLEAR it. Decided to just clear.
	    if(!TEST_BIT(alg, g_implementedAlgorithms))
		{
		    CLEAR_BIT(alg, *toTest);
		    continue;
		}
	    // Process whatever is left.
	    // NOTE: since this switch will only be called if the algorithm is
	    // implemented, it is not necessary to modify this list except to comment
	    // out the algorithms for which there is no test
	    switch(alg)
		{
		    // Symmetric block ciphers
#if ALG_AES
		  case TPM_ALG_AES:
// libtpms added begin
#if SMAC_IMPLEMENTED && ALG_CMAC
                    if (doTest) {
                         result = TestSMAC(toTest);
                         if (result != TPM_RC_SUCCESS)
                             break;
                    }
#endif
// libtpms added end
#endif
#if ALG_SM4
		    // if SM4 is implemented, its test is like other block ciphers but there
		    // aren't any test vectors for it yet
		    //            case TPM_ALG_SM4:
#endif
#if ALG_CAMELLIA
		  case TPM_ALG_CAMELLIA:  // libtpms activated
#endif
#if ALG_TDES
                  case TPM_ALG_TDES:      // libtpms added
#endif
		    // Symmetric modes
#if !ALG_CFB
#   error   CFB is required in all TPM implementations
#endif // !TPM_ALG_CFB
		  case TPM_ALG_CFB:
		    if(doTest)
			result = TestSymmetric(alg, toTest);
		    break;
#if ALG_CTR
		  case TPM_ALG_CTR:
#endif // TPM_ALG_CRT
#if ALG_OFB
		  case TPM_ALG_OFB:
#endif // TPM_ALG_OFB
#if ALG_CBC
		  case TPM_ALG_CBC:
#endif // TPM_ALG_CBC
#if ALG_ECB
		  case TPM_ALG_ECB:
#endif
		    if(doTest)
			result = TestSymmetric(alg, toTest);
		    else
			// If doing the initialization of g_toTest vector, only need
			// to test one of the modes for the symmetric algorithms. If
			// initializing for a SelfTest(FULL_TEST), allow all the modes.
			if(toTest == &g_toTest)
			    CLEAR_BIT(alg, *toTest);
		    break;
#if !ALG_HMAC
#   error   HMAC is required in all TPM implementations
#endif
		  case TPM_ALG_HMAC:
		    // Clear the bit that indicates that HMAC is required because
		    // HMAC is used as the basic test for all hash algorithms.
		    CLEAR_BOTH(alg);
		    // Testing HMAC means test the default hash
		    if(doTest)
			TestHash(DEFAULT_TEST_HASH, toTest);
		    else
			// If not testing, then indicate that the hash needs to be
			// tested because this uses HMAC
			SET_BOTH(DEFAULT_TEST_HASH);
		    break;
		    // Have to use two arguments for the macro even though only the first is used in the
		    // expansion.
#define HASH_CASE_TEST(HASH, hash)					\
	            case ALG_##HASH##_VALUE:
		    FOR_EACH_HASH(HASH_CASE_TEST)
#undef HASH_CASE_TEST
	                if(doTest)
	                    result = TestHash(alg, toTest);
		    break;
		    // RSA-dependent
#if ALG_RSA
		  case TPM_ALG_RSA:
		    CLEAR_BOTH(alg);
		    if(doTest)
			result = TestRsa(TPM_ALG_NULL, toTest);
		    else
			SET_BOTH(TPM_ALG_NULL);
		    break;
		  case TPM_ALG_RSASSA:
		  case TPM_ALG_RSAES:
		  case TPM_ALG_RSAPSS:
		  case TPM_ALG_OAEP:
		  case TPM_ALG_NULL:    // used or RSADP
		    if(doTest)
			result = TestRsa(alg, toTest);
		    break;
#endif // ALG_RSA
#if ALG_KDF1_SP800_108
		  case TPM_ALG_KDF1_SP800_108:
		    if(doTest)
			result = TestKDFa(toTest);
		    break;
#endif // ALG_KDF1_SP800_108
#if ALG_ECC
		    // ECC dependent but no tests
		    //        case TPM_ALG_ECDAA:
		    //        case TPM_ALG_ECMQV:
		    //        case TPM_ALG_KDF1_SP800_56a:
		    //        case TPM_ALG_KDF2:
		    //        case TPM_ALG_MGF1:
		  case TPM_ALG_ECC:
		    CLEAR_BOTH(alg);
		    if(doTest)
			result = TestEcc(TPM_ALG_ECDH, toTest);
		    else
			SET_BOTH(TPM_ALG_ECDH);
		    break;
		  case TPM_ALG_ECDSA:
		  case TPM_ALG_ECDH:
		  case TPM_ALG_ECSCHNORR:
		    //            case TPM_ALG_SM2:
		    if(doTest)
			result = TestEcc(alg, toTest);
		    break;
#endif // ALG_ECC
		  default:
		    CLEAR_BIT(alg, *toTest);
		    break;
		}
	    if(result != TPM_RC_SUCCESS)
		break;
	}
    return result;
}
#endif // SELF_TESTS
