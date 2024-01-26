/********************************************************************************/
/*										*/
/*			     Entropy						*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Entropy.c $		*/
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

/* C.4 Entropy.c */
/* C.4.1. Includes and Local values*/
#define _CRT_RAND_S
#include <stdlib.h>
#include <memory.h>

#include <openssl/rand.h>   /* libtpms added */

#include <time.h>
#include "Platform.h"
#ifdef _MSC_VER
#include <process.h>
#else
#include <unistd.h>
#endif

/* This is the last 32-bits of hardware entropy produced. We have to check to see that two
   consecutive 32-bit values are not the same because (according to FIPS 140-2, annex C */
/* "If each call to a RNG produces blocks of n bits (where n > 15), the first n-bit block generated
   after power-up, initialization, or reset shall not be used, but shall be saved for comparison
   with the next n-bit block to be generated. Each subsequent generation of an n-bit block shall be
   compared with the previously generated block. The test shall fail if any two compared n-bit
   blocks are equal." */
extern uint32_t        lastEntropy;

/* C.4.2.	Functions */
/* C.4.2.1.	rand32() */
/* Local function to get a 32-bit random number */

static uint32_t
rand32(
       void
       )
{
    uint32_t    rndNum = rand();
#if RAND_MAX < UINT16_MAX
    // If the maximum value of the random number is a 15-bit number, then shift it up
    // 15 bits, get 15 more bits, shift that up 2 and then XOR in another value to get
    // a full 32 bits.
    rndNum = (rndNum << 15) ^ rand();
    rndNum = (rndNum << 2) ^ rand();
#elif RAND_MAX == UINT16_MAX
    // If the maximum size is 16-bits, shift it and add another 16 bits
    rndNum = (rndNum << 16) ^ rand();
#elif RAND_MAX < UINT32_MAX
    // If 31 bits, then shift 1 and include another random value to get the extra bit
    rndNum = (rndNum << 1) ^ rand();
#endif
    return rndNum;
}

/* C.4.2.2 _plat__GetEntropy() */
/* This function is used to get available hardware entropy. In a hardware implementation of this
   function, there would be no call to the system to get entropy. */
/* Return Values Meaning */
/* < 0 hardware failure of the entropy generator, this is sticky */
/* >= 0 the returned amount of entropy (bytes) */
LIB_EXPORT int32_t
_plat__GetEntropy(
		  unsigned char       *entropy,           // output buffer
		  uint32_t             amount             // amount requested
		  )
{
    uint32_t            rndNum;
    int32_t             ret;
    //
    // libtpms added begin
    if (amount > 0 && RAND_bytes(entropy, amount) == 1)
        return amount;
    // fall back to 'original' method
    // libtpms added end

    if(amount == 0)
	{
	    // Seed the platform entropy source if the entropy source is software. There is
	    // no reason to put a guard macro (#if or #ifdef) around this code because this
	    // code would not be here if someone was changing it for a system with actual
	    // hardware.
	    //
	    // NOTE 1: The following command does not provide proper cryptographic entropy.
	    // Its primary purpose to make sure that different instances of the simulator,
	    // possibly started by a script on the same machine, are seeded differently.
	    // Vendors of the actual TPMs need to ensure availability of proper entropy
	    // using their platform specific means.
	    //
	    // NOTE 2: In debug builds by default the reference implementation will seed
	    // its RNG deterministically (without using any platform provided randomness).
	    // See the USE_DEBUG_RNG macro and DRBG_GetEntropy() function.
#ifdef _MSC_VER
	    srand((unsigned)_plat__RealTime() ^ _getpid());
#else
	    srand((unsigned)_plat__RealTime() ^ getpid());
#endif
	    lastEntropy = rand32();
	    ret = 0;
	}
    else
	{
	    rndNum = rand32();
	    if(rndNum == lastEntropy)
		{
		    ret = -1;
		}
	    else
		{
		    lastEntropy = rndNum;
		    // Each process will have its random number generator initialized according
		    // to the process id and the initialization time. This is not a lot of
		    // entropy so, to add a bit more, XOR the current time value into the
		    // returned entropy value.
		    // NOTE: the reason for including the time here rather than have it in
		    // in the value assigned to lastEntropy is that rand() could be broken and
		    // using the time would in the lastEntropy value would hide this.
		    rndNum ^= (uint32_t)_plat__RealTime();
		    // Only provide entropy 32 bits at a time to test the ability
		    // of the caller to deal with partial results.
		    ret = MIN(amount, sizeof(rndNum));
		    memcpy(entropy, &rndNum, ret);
		}
	}
    return ret;
}

