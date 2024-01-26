/********************************************************************************/
/*										*/
/*			 Platform Power Support    				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: PowerPlat.c $		*/
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

/* C.7 PowerPlat.c */
/* C.7.1. Includes and Function Prototypes */
#include    "Platform.h"
#include    "PlatformACT_fp.h"		/* added kgold */
#include    "_TPM_Init_fp.h"
/* C.7.2. Functions */
/* C.7.2.1. _plat__Signal_PowerOn() */
/* Signal platform power on */
LIB_EXPORT int
_plat__Signal_PowerOn(
		      void
		      )
{
    // Reset the timer
    _plat__TimerReset();
    // Need to indicate that we lost power
    s_powerLost = TRUE;
    return 0;
}
/* C.7.2.2. _plat__WasPowerLost() */
/* Test whether power was lost before a _TPM_Init(). */
/* This function will clear the hardware indication of power loss before return. This means that
   there can only be one spot in the TPM code where this value gets read. This method is used here
   as it is the most difficult to manage in the TPM code and, if the hardware actually works this
   way, it is hard to make it look like anything else. So, the burden is placed on the TPM code
   rather than the platform code */
/* Return Values Meaning */
/* TRUE(1) power was lost */
/* FALSE(0)	power was not lost */
LIB_EXPORT int
_plat__WasPowerLost(
		    void
		    )
{
    int retVal = s_powerLost;
    s_powerLost = FALSE;
    return retVal;
}
/* C.7.2.3. _plat_Signal_Reset() */
/* This a TPM reset without a power loss. */
LIB_EXPORT int
_plat__Signal_Reset(
		    void
		    )
{
    // Initialize locality
    s_locality = 0;
    // Command cancel
    s_isCanceled = FALSE;
    _TPM_Init();
    // if we are doing reset but did not have a power failure, then we should
    // not need to reload NV ...
    return 0;
}
/* C.7.2.4. _plat__Signal_PowerOff() */
/* Signal platform power off */
LIB_EXPORT void
_plat__Signal_PowerOff(
		       void
		       )
{
    // Prepare NV memory for power off
    _plat__NVDisable(0);
    // Disable tick ACT tick processing
    _plat__ACT_EnableTicks(FALSE);
    return;
}
