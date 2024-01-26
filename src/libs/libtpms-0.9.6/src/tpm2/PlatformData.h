/********************************************************************************/
/*										*/
/*		Instance data for the Platform module. 				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: PlatformData.h $		*/
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
/*  (c) Copyright IBM Corp. and others, 2016 - 2019.				*/
/*										*/
/********************************************************************************/

/* c.8 PlatformData.h */
/* This file contains the instance data for the Platform module. It is collected in this file so
   that the state of the module is easier to manage. */

#ifndef _PLATFORM_DATA_H_
#define _PLATFORM_DATA_H_
#ifdef  _PLATFORM_DATA_C_
#define EXTERN
#else
#define EXTERN  extern
#endif

/* From Cancel.c Cancel flag.  It is initialized as FALSE, which indicate the command is not being
   canceled */
EXTERN int     s_isCanceled;

#ifndef HARDWARE_CLOCK
typedef uint64_t     clock64_t;
// This is the value returned the last time that the system clock was read. This is only relevant
// for a simulator or virtual TPM.
EXTERN clock64_t        s_realTimePrevious;
// These values are used to try to synthesize a long lived version of clock().
EXTERN clock64_t        s_lastSystemTime;
EXTERN clock64_t        s_lastReportedTime;
// This is the rate adjusted value that is the equivalent of what would be read from a hardware
// register that produced rate adjusted time.
EXTERN clock64_t        s_tpmTime;
/* libtpms added begin */
EXTERN int64_t          s_hostMonotonicAdjustTime;
EXTERN uint64_t         s_suspendedElapsedTime;
/* libtpms added end */
#endif // HARDWARE_CLOCK

/* This value indicates that the timer was reset */
EXTERN BOOL              s_timerReset;
/* This value indicates that the timer was stopped. It causes a clock discontinuity. */
EXTERN BOOL              s_timerStopped;
/* This variable records the time when _plat__TimerReset() is called.  This mechanism allow us to
   subtract the time when TPM is power off from the total time reported by clock() function */
EXTERN uint64_t         s_initClock;
/* This variable records the timer adjustment factor. */
EXTERN unsigned int     s_adjustRate;
/* From LocalityPlat.c Locality of current command */
EXTERN unsigned char s_locality;
/* From NVMem.c Choose if the NV memory should be backed by RAM or by file. If this macro is
   defined, then a file is used as NV.  If it is not defined, then RAM is used to back NV
   memory. Comment out to use RAM. */
#if (!defined VTPM) || ((VTPM != NO) && (VTPM != YES))
#   undef VTPM
#   define      VTPM            NO                 // Default: Either YES or NO   libtpms: NO
#endif

// For a simulation, use a file to back up the NV

#if (!defined FILE_BACKED_NV) || ((FILE_BACKED_NV != NO) && (FILE_BACKED_NV != YES))
#   undef   FILE_BACKED_NV
#   define  FILE_BACKED_NV          (VTPM && YES)     // Default: Either YES or NO
#endif
#if !SIMULATION
#   undef       FILE_BACKED_NV
#   define      FILE_BACKED_NV          YES          // libtpms: write NvChip file if no callbacks are set
#else
#error Do not define SIMULATION for libtpms!
#endif // SIMULATION

EXTERN unsigned char    s_NV[NV_MEMORY_SIZE];
EXTERN BOOL              s_NvIsAvailable;
EXTERN BOOL              s_NV_unrecoverable;
EXTERN BOOL              s_NV_recoverable;
/* From PPPlat.c Physical presence.  It is initialized to FALSE */
EXTERN BOOL     s_physicalPresence;
/* From Power */
EXTERN BOOL        s_powerLost;
/* From Entropy.c */
EXTERN uint32_t        lastEntropy;

#define DEFINE_ACT(N)   EXTERN ACT_DATA ACT_##N;
FOR_EACH_ACT(DEFINE_ACT)
EXTERN int             actTicksAllowed;

#endif // _PLATFORM_DATA_H_
