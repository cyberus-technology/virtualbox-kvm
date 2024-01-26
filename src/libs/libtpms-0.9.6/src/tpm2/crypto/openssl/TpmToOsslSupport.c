/********************************************************************************/
/*										*/
/*		Initialization of the Interface to the OpenSSL Library.	   	*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: TpmToOsslSupport.c $	*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2019				*/
/*										*/
/********************************************************************************/

/* B.2.3.3. TpmToOsslSupport.c */
/* B.2.3.3.1. Introduction */
/* The functions in this file are used for initialization of the interface to the OpenSSL
   library. */
/* B.2.3.3.2. Defines and Includes */
#include "Tpm.h"

#if defined(HASH_LIB_OSSL) || defined(MATH_LIB_OSSL) || defined(SYM_LIB_OSSL)

/*     Used to pass the pointers to the correct sub-keys */
typedef const BYTE *desKeyPointers[3];
/* B.2.3.3.2.1. SupportLibInit() */
/* This does any initialization required by the support library. */
LIB_EXPORT int
SupportLibInit(
	       void
	       )
{
    return TRUE;
}
/* B.2.3.3.2.2. OsslContextEnter() */
/* This function is used to initialize an OpenSSL context at the start of a function that will
   call to an OpenSSL math function. */
BN_CTX *
OsslContextEnter(
		 void
		 )
{
    BN_CTX              *CTX = BN_CTX_new();
    return OsslPushContext(CTX);
}
/* B.2.3.3.2.3. OsslContextLeave() */
/* This is the companion function to OsslContextEnter(). */
void
OsslContextLeave(
		 BN_CTX          *CTX
		 )
{
    OsslPopContext(CTX);
    BN_CTX_free(CTX);
}

/* B.2.3.3.2.4.	OsslPushContext() */
/* This function is used to create a frame in a context. All values allocated within this context after the frame is started will be automatically freed when the context (OsslPopContext() */
BN_CTX *
OsslPushContext(
		BN_CTX          *CTX
		)
{
    if(CTX == NULL)
	FAIL(FATAL_ERROR_ALLOCATION);
    BN_CTX_start(CTX);
    return CTX;
}

/* B.2.3.3.2.5.	OsslPopContext() */
/* This is the companion function to OsslPushContext(). */
void
OsslPopContext(
	       BN_CTX          *CTX
	       )
{
    // BN_CTX_end can't be called with NULL. It will blow up.
    if(CTX != NULL)
	BN_CTX_end(CTX);
}

#endif // HASH_LIB_OSSL || MATH_LIB_OSSL || SYM_LIB_OSSL
