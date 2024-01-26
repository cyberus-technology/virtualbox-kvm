/********************************************************************************/
/*										*/
/*			 Constant time debugging helper functions		*/
/*			     Written by Stefan Berger				*/
/*		       IBM Thomas J. Watson Research Center			*/
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
/*  (c) Copyright IBM Corp. and others, 2020					*/
/*										*/
/********************************************************************************/

#ifndef CONSTTIME_UTILS_H
#define CONSTTIME_UTILS_H

#include <assert.h>
#include <stdio.h>

#include "BnValues.h"

#include <openssl/bn.h>

static __inline__ unsigned long long rdtsc() {
    unsigned long h, l;

    __asm__ __volatile__ ("rdtsc" : "=a"(l), "=d"(h));

    return  (unsigned long long)l |
           ((unsigned long long)h << 32 );
}

// Make sure that the given BIGNUM has the given number of expected bytes.
// Skip over any leading zeros the BIGNUM may have.
static inline void assert_ossl_num_bytes(const BIGNUM *a,
                                         unsigned int num_bytes,
                                         int verbose,
                                         const char *caller) {
    unsigned char buffer[LARGEST_NUMBER] = { 0, };
    int len, i;

    len = BN_bn2bin(a, buffer);
    for (i = 0; i < len; i++) {
        if (buffer[i])
            break;
    }
    len -= i;
    if (num_bytes != (unsigned int)len) {
        printf("%s: Expected %u bytes but found %d (caller: %s)\n", __func__, num_bytes, len, caller);
    } else {
        if (verbose)
            printf("%s: check passed; num_bytes = %d (caller: %s)\n",__func__, num_bytes, caller);
    }
    assert(num_bytes == (unsigned int)len);
}

// Make sure that the bigNum has the expected number of bytes after it was
// converted to an OpenSSL BIGNUM.
static inline void assert_bn_ossl_num_bytes(bigNum tpmb,
                                            unsigned int num_bytes,
                                            int verbose,
                                            const char *caller) {
    BIG_INITIALIZED(osslb, tpmb);

    assert_ossl_num_bytes(osslb, num_bytes, verbose, caller);

    BN_free(osslb);
}

#endif /* CONSTTIME_UTILS_H */
