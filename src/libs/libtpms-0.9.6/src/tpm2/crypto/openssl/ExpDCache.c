/********************************************************************************/
/*										*/
/*			Private Exponent D cache functions			*/
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
/*  (c) Copyright IBM Corp., 2021						*/
/*										*/
/********************************************************************************/

#include "Tpm.h"
#include "ExpDCache_fp.h"

/* Implement a cache for the private exponent D so it doesn't need to be
 * recalculated every time from P, Q, E and N (modulus). The cache has a
 * number of entries that cache D and use P, Q, and E for lookup.
 * A least-recently-used cache eviction strategy is implemented that evicts
 * the oldest cache entry in case space is needed. An entry is young once
 * it is added or made young when it was found via lookup. All other entries
 * age by '1' when an entry is added or accessed.
 */

struct ExpDCacheEntry {
    /* The age of the entry; the higher the number the more likely it
     * will be evicted soon
     */
    unsigned int age;
    BIGNUM *P; /* input */
    BIGNUM *N; /* input */
    BIGNUM *E; /* input */
    BIGNUM *Q; /* cached */
    BIGNUM *D; /* cached */
};

#define DCACHE_NUM_ENTRIES 64

static struct ExpDCacheEntry ExpDCache[DCACHE_NUM_ENTRIES];

/* Increment the age of all cache entries that have a current age <= maxage */
static void ExpDCacheIncrementAge(unsigned maxage)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(ExpDCache); i++) {
        if (ExpDCache[i].age <= maxage && ExpDCache[i].D != NULL)
            ExpDCache[i].age++;
    }
}

/* Free the data associated with a ExpDCacheEntry and initialize it */
static void ExpDCacheEntryFree(struct ExpDCacheEntry *dce)
{
    BN_clear_free(dce->P);
    BN_free(dce->N);
    BN_free(dce->E);
    BN_clear_free(dce->Q);
    BN_clear_free(dce->D);
    memset(dce, 0, sizeof(*dce));
}

void ExpDCacheFree(void)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(ExpDCache); i++)
        ExpDCacheEntryFree(&ExpDCache[i]);
}

/* Get a ExpDCacheEntry by finding either an unused entry or evicting the oldest
 * entry. The returned entry will have all NULL pointers and age 0.
 */
static struct ExpDCacheEntry *ExpDCacheEntryGet(void)
{
    size_t i, use_i = 0;
    unsigned oldest_age = 0;
    struct ExpDCacheEntry *dce;

    for (i = 0; i < ARRAY_SIZE(ExpDCache); i++) {
        if (ExpDCache[i].D == NULL) {
            /* use this free entry */
            use_i = i;
            break;
        }
        if (ExpDCache[i].age > oldest_age) {
            /* this one is currently the oldest */
            use_i = i;
            oldest_age = ExpDCache[i].age;
        }
    }
    dce = &ExpDCache[use_i];

    ExpDCacheEntryFree(dce);

    return dce;
}

/* Add 'D' to the ExpDCache. This function does not check for duplicates */
void ExpDCacheAdd(const BIGNUM *P, const BIGNUM *N, const BIGNUM *E,
                  const BIGNUM *Q, const BIGNUM *D)
{
    struct ExpDCacheEntry *dce = ExpDCacheEntryGet();

    /* age of 'dce' is '0' */
    dce->P = BN_dup(P);
    dce->N = BN_dup(N);
    dce->E = BN_dup(E);
    dce->Q = BN_dup(Q);
    dce->D = BN_dup(D);

    if (!dce->P || !dce->N || !dce->E || !dce->Q || !dce->D)
        ExpDCacheEntryFree(dce);
    else
        ExpDCacheIncrementAge(~0);
}

BIGNUM *ExpDCacheFind(const BIGNUM *P, const BIGNUM *N, const BIGNUM *E, BIGNUM **Q)
{
    size_t i;
    unsigned myage;
    BIGNUM *D;

    for (i = 0; i < ARRAY_SIZE(ExpDCache); i++) {
        if (BN_cmp(ExpDCache[i].P, P) == 0 && BN_cmp(ExpDCache[i].N, N) == 0 &&
            BN_cmp(ExpDCache[i].E, E) == 0) {
            /* entry found */
            myage = ExpDCache[i].age;
            /* mark this entry as most recently used */
            ExpDCache[i].age = 0;
            /* Increment everyone who is <= 'myage'.
             * The age of this entry will be '1' after that.
             */
            ExpDCacheIncrementAge(myage);

            *Q = BN_dup(ExpDCache[i].Q);
            if (*Q == NULL)
                return NULL;
            D = BN_dup(ExpDCache[i].D);
            if (D == NULL) {
                BN_clear_free(*Q);
                *Q = NULL;
                return NULL;
            }
            BN_set_flags(*Q, BN_FLG_CONSTTIME);
            BN_set_flags(D, BN_FLG_CONSTTIME);
            return D;
        }
    }

    return NULL;
}
