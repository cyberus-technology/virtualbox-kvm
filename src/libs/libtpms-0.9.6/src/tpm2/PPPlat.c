/********************************************************************************/
/*										*/
/*		Simulates the Physical Present Interface	     		*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: PPPlat.c $		*/
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

/* C.10 PPPlat.c */
/* C.10.1. Description */
/* This module simulates the physical presence interface pins on the TPM. */
/* C.10.2. Includes */
#include "Platform.h"
#include "LibtpmsCallbacks.h" /* libtpms added */
/* C.10.3. Functions */
/* C.10.3.1. _plat__PhysicalPresenceAsserted() */
/* Check if physical presence is signaled */
/* Return Values Meaning */
/* TRUE(1) if physical presence is signaled */
/* FALSE(0) if physical presence is not signaled */
LIB_EXPORT int
_plat__PhysicalPresenceAsserted(
				void
				)
{
#ifdef TPM_LIBTPMS_CALLBACKS
    BOOL pp;
    int ret = libtpms_plat__PhysicalPresenceAsserted(&pp);

    if (ret != LIBTPMS_CALLBACK_FALLTHROUGH)
        return pp;
#endif /* TPM_LIBTPMS_CALLBACKS */

    // Do not know how to check physical presence without real hardware.
    // so always return TRUE;
    return s_physicalPresence;
}
#if 0 /* libtpms added */
/* C.10.3.2. _plat__Signal_PhysicalPresenceOn() */
/* Signal physical presence on */
LIB_EXPORT void
_plat__Signal_PhysicalPresenceOn(
				 void
				 )
{
    s_physicalPresence = TRUE;
    return;
}
/* C.10.3.3. _plat__Signal_PhysicalPresenceOff() */
/* Signal physical presence off */
LIB_EXPORT void
_plat__Signal_PhysicalPresenceOff(
				  void
				  )
{
    s_physicalPresence = FALSE;
    return;
}
#endif /* libtpms added */
