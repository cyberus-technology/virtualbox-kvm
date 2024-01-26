/********************************************************************************/
/*										*/
/*				Platform Clock			.     		*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: PlatformClock.h $		*/
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
/*  (c) Copyright IBM Corp. and others, 2019 - 2020				*/
/*										*/
/********************************************************************************/

// C.16 PlatformClock.h This file contains the instance data for the Platform module. It is
// collected in this file so that the state of the module is easier to manage.
#ifndef _PLATFORM_CLOCK_H_
#define _PLATFORM_CLOCK_H_
#ifdef _MSC_VER
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#include <time.h>
#endif
// CLOCK_NOMINAL is the number of hardware ticks per mS. A value of 300000 means that the nominal
// clock rate used to drive the hardware clock is 30 MHz. The adjustment rates are used to determine
// the conversion of the hardware ticks to internal hardware clock value. In practice, we would
// expect that there would be a hardware register will accumulated mS. It would be incremented by
// the output of a pre-scaler. The pre-scaler would divide the ticks from the clock by some value
// that would compensate for the difference between clock time and real time. The code in Clock does
// the emulation of this function.
#define     CLOCK_NOMINAL           30000
// A 1% change in rate is 300 counts
#define     CLOCK_ADJUST_COARSE     300
// A 0.1% change in rate is 30 counts
#define     CLOCK_ADJUST_MEDIUM     30
// A minimum change in rate is 1 count
#define     CLOCK_ADJUST_FINE       1
// The clock tolerance is +/-15% (4500 counts) Allow some guard band (16.7%)
#define     CLOCK_ADJUST_LIMIT      5000
#endif // _PLATFORM_CLOCK_H_
