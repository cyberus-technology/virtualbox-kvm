/********************************************************************************/
/*										*/
/*		NV read and write access methods     				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: Platform_fp.h $		*/
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

/* C.8 Platform_fp.h */
#ifndef    _PLATFORM_FP_H_
#define    _PLATFORM_FP_H_

#include "BaseTypes.h"

/* C.8.1. From Cancel.c */
/* C.8.1.1. _plat__IsCanceled() */
/* Check if the cancel flag is set */
/* Return Values Meaning */
/* TRUE(1) if cancel flag is set */
/* FALSE(0) if cancel flag is not set */
LIB_EXPORT int
_plat__IsCanceled(
		  void
		  );
/* Set cancel flag. */
LIB_EXPORT void
_plat__SetCancel(
		 void
		 );
/* C.8.1.2. _plat__ClearCancel() */
/* Clear cancel flag */
LIB_EXPORT void
_plat__ClearCancel(
		   void
		   );
/* C.8.2. From Clock.c */
/* C.8.2.1. _plat__TimerReset() */
/* This function sets current system clock time as t0 for counting TPM time. This function is called
   at a power on event to reset the clock. When the clock is reset, the indication that the clock
   was stopped is also set. */
LIB_EXPORT void
_plat__TimerReset(
		  void
		  );
/* C.8.2.2. _plat__TimerRestart() */
/* This function should be called in order to simulate the restart of the timer should it be stopped
   while power is still applied. */
LIB_EXPORT void
_plat__TimerRestart(
		    void
		    );
// C.8.2.3. _plat__Time() This is another, probably futile, attempt to define a portable function
// that will return a 64-bit clock value that has mSec resolution.
LIB_EXPORT uint64_t
_plat__RealTime(
		void
		);
/* C.8.2.4. _plat__TimerRead() */
/* This function provides access to the tick timer of the platform. The TPM code uses this value to
   drive the TPM Clock. */
/* The tick timer is supposed to run when power is applied to the device. This timer should not be
   reset by time events including _TPM_Init(). It should only be reset when TPM power is
   re-applied. */
/* If the TPM is run in a protected environment, that environment may provide the tick time to the
   TPM as long as the time provided by the environment is not allowed to go backwards. If the time
   provided by the system can go backwards during a power discontinuity, then the
   _plat__Signal_PowerOn() should call _plat__TimerReset(). */
LIB_EXPORT uint64_t
_plat__TimerRead(
		 void
		 );
/* C.8.2.5. _plat__TimerWasReset() */
/* This function is used to interrogate the flag indicating if the tick timer has been reset. */
/* If the resetFlag parameter is SET, then the flag will be CLEAR before the function returns. */
LIB_EXPORT int
_plat__TimerWasReset(
		     void
		     );
/* C.8.2.6. _plat__TimerWasStopped() */
/* This function is used to interrogate the flag indicating if the tick timer has been stopped. If
   so, this is typically a reason to roll the nonce. */
/* This function will CLEAR the s_timerStopped flag before returning. This provides functionality
   that is similar to status register that is cleared when read. This is the model used here because
   it is the one that has the most impact on the TPM code as the flag can only be accessed by one
   entity in the TPM. Any other implementation of the hardware can be made to look like a read-once
   register. */
LIB_EXPORT int
_plat__TimerWasStopped(
		       void
		       );
/* C.8.2.7. _plat__ClockAdjustRate() */
/* Adjust the clock rate */
LIB_EXPORT void
_plat__ClockAdjustRate(
		       int              adjust         // IN: the adjust number.  It could be positive
		       //     or negative
		       );
/* C.8.3. From Entropy.c */
// C.8.4. _plat__GetEntropy()
// This function is used to get available hardware entropy. In a hardware implementation of this
// function, there would be no call to the system to get entropy.
/* Return Values Meaning */
/* < 0 hardware failure of the entropy generator, this is sticky */
/* >= 0 the returned amount of entropy (bytes) */
LIB_EXPORT int32_t
_plat__GetEntropy(
		  unsigned char       *entropy,           // output buffer
		  uint32_t             amount             // amount requested
		  );
/* C.8.4. From LocalityPlat.c */
/* C.8.4.1. _plat__LocalityGet() */
/* Get the most recent command locality in locality value form. This is an integer value for
   locality and not a locality structure The locality can be 0-4 or 32-255. 5-31 is not allowed. */
LIB_EXPORT unsigned char
_plat__LocalityGet(
		   void
		   );
/* C.8.4.2. _plat__LocalitySet() */
/* Set the most recent command locality in locality value form */
LIB_EXPORT void
_plat__LocalitySet(
		   unsigned char    locality
		   );
/* C.8.5. From NVMem.c */
#if 0 /* libtpms added */
/* C.8.5.1. _plat__NvErrors() */
/* This function is used by the simulator to set the error flags in the NV subsystem to simulate an error in the NV loading process */
LIB_EXPORT void
_plat__NvErrors(
		int              recoverable,
		int            unrecoverable
		);
#endif /* libtpms added */
/* C.8.5.2. _plat__NVEnable() */
/* Enable NV memory. */
/* This version just pulls in data from a file. In a real TPM, with NV on chip, this function would
   verify the integrity of the saved context. If the NV memory was not on chip but was in something
   like RPMB, the NV state would be read in, decrypted and integrity checked. */
/* The recovery from an integrity failure depends on where the error occurred. It it was in the
   state that is discarded by TPM Reset, then the error is recoverable if the TPM is
   reset. Otherwise, the TPM must go into failure mode. */
/* Return Values Meaning */
/* 0 if success */
/* > 0 if receive recoverable error */
/* <0 if unrecoverable error */
LIB_EXPORT int
_plat__NVEnable(
		void            *platParameter  // IN: platform specific parameters
		);
/* libtpms added begin */
LIB_EXPORT int
_plat__NVEnable_NVChipFile(
		void            *platParameter  // IN: platform specific parameters
		);
/* libtpms added end */
/* C.8.5.3. _plat__NVDisable() */
/* Disable NV memory */
LIB_EXPORT void
_plat__NVDisable(
		 int             delete           // IN: If TRUE, delete the NV contents.
		 );
/* C.8.6.4. _plat__IsNvAvailable() */
/* Check if NV is available */
/* Return Values Meaning */
/* 0 NV is available */
/* 1 NV is not available due to write failure */
/* 2 NV is not available due to rate limit */
LIB_EXPORT int
_plat__IsNvAvailable(
		     void
		     );
/* C.8.5.5. _plat__NvMemoryRead() */
/* Function: Read a chunk of NV memory */
LIB_EXPORT void
_plat__NvMemoryRead(
		    unsigned int     startOffset,   // IN: read start
		    unsigned int     size,          // IN: size of bytes to read
		    void            *data           // OUT: data buffer
		    );
/* C.8.5.6. _plat__NvIsDifferent() */
/* This function checks to see if the NV is different from the test value. This is so that NV will
   not be written if it has not changed. */
/* Return Values Meaning */
/* TRUE(1) the NV location is different from the test value */
/* FALSE(0) the NV location is the same as the test value */
LIB_EXPORT int
_plat__NvIsDifferent(
		     unsigned int     startOffset,   // IN: read start
		     unsigned int     size,          // IN: size of bytes to read
		     void            *data           // IN: data buffer
		     );
/* C.8.5.7. _plat__NvMemoryWrite() */
/* This function is used to update NV memory. The write is to a memory copy of NV. At the end of the
   current command, any changes are written to the actual NV memory. */
/* NOTE: A useful optimization would be for this code to compare the current contents of NV with the
   local copy and note the blocks that have changed. Then only write those blocks when
   _plat__NvCommit() is called. */
LIB_EXPORT int
_plat__NvMemoryWrite(
		     unsigned int     startOffset,   // IN: write start
		     unsigned int     size,          // IN: size of bytes to write
		     void            *data           // OUT: data buffer
		     );
/* C.8.6.8. _plat__NvMemoryClear() */
/* Function is used to set a range of NV memory bytes to an implementation-dependent value. The
   value represents the erase state of the memory. */
LIB_EXPORT void
_plat__NvMemoryClear(
		     unsigned int     start,         // IN: clear start
		     unsigned int     size           // IN: number of bytes to clear
		     );
/* C.8.5.9. _plat__NvMemoryMove() */
/* Function: Move a chunk of NV memory from source to destination This function should ensure that
   if there overlap, the original data is copied before it is written */
LIB_EXPORT void
_plat__NvMemoryMove(
		    unsigned int     sourceOffset,  // IN: source offset
		    unsigned int     destOffset,    // IN: destination offset
		    unsigned int     size           // IN: size of data being moved
		    );
/* C.8.5.10. _plat__NvCommit() */
// This function writes the local copy of NV to NV for permanent store. It will write NV_MEMORY_SIZE
// bytes to NV. If a file is use, the entire file is written.
/* Return Values Meaning */
/* 0 NV write success */
/* non-0 NV write fail */
LIB_EXPORT int
_plat__NvCommit(
		void
		);
/* C.8.5.11. _plat__SetNvAvail() */
/* Set the current NV state to available.  This function is for testing purpose only.  It is not
   part of the platform NV logic */
LIB_EXPORT void
_plat__SetNvAvail(
		  void
		  );
/* C.8.5.12. _plat__ClearNvAvail() */
/* Set the current NV state to unavailable.  This function is for testing purpose only.  It is not
   part of the platform NV logic */
LIB_EXPORT void
_plat__ClearNvAvail(
		    void
		    );

/* C.6.2.15.	_plat__NVNeedsManufacture() */
/* This function is used by the simulator to determine when the TPM's NV state needs to be manufactured. */

LIB_EXPORT int
_plat__NVNeedsManufacture(
			  void
			  );

/* C.8.6. From PowerPlat.c */
/* C.8.6.1. _plat__Signal_PowerOn() */
/* Signal platform power on */
LIB_EXPORT int
_plat__Signal_PowerOn(
		      void
		      );
/* C.8.6.2. _plat__WasPowerLost() */
/* Test whether power was lost before a _TPM_Init(). */
/* This function will clear the hardware indication of power loss before return. This means that
   there can only be one spot in the TPM code where this value gets read. This method is used here
   as it is the most difficult to manage in the TPM code and, if the hardware actually works this
   way, it is hard to make it look like anything else. So, the burden is placed on the TPM code
   rather than the platform code */
/* Return Values Meaning */
/* TRUE(1) power was lost */
/* FALSE(0) power was not lost */
LIB_EXPORT int
_plat__WasPowerLost(
		    void
		    );
/* C.8.6.3. _plat_Signal_Reset() */
/* This a TPM reset without a power loss. */
LIB_EXPORT int
_plat__Signal_Reset(
		    void
		    );
/* C.8.6.4. _plat__Signal_PowerOff() */
/* Signal platform power off */
LIB_EXPORT void
_plat__Signal_PowerOff(
		       void
		       );

/* C.8.7. From PPPlat.c */
/* C.8.7.1. _plat__PhysicalPresenceAsserted() */
/* Check if physical presence is signaled */
/* Return Values Meaning */
/* TRUE(1) if physical presence is signaled */
/* FALSE(0) if physical presence is not signaled */
LIB_EXPORT int
_plat__PhysicalPresenceAsserted(
				void
				);
#if 0 /* libtpms added */
/* C.8.7.2. _plat__Signal_PhysicalPresenceOn() */
/* Signal physical presence on */
LIB_EXPORT void
_plat__Signal_PhysicalPresenceOn(
				 void
				 );
/* C.8.7.3. _plat__Signal_PhysicalPresenceOff() */
/* Signal physical presence off */
LIB_EXPORT void
_plat__Signal_PhysicalPresenceOff(
				  void
				  );
#endif /* libtpms added */

/* C.8.8. From RunCommand.c */
/* C.8.8.1. _plat__RunCommand() */
/* This version of RunCommand() will set up a jum_buf and call ExecuteCommand(). If the command
   executes without failing, it will return and RunCommand() will return. If there is a failure in
   the command, then _plat__Fail() is called and it will longjump back to RunCommand() which will
   call ExecuteCommand() again. However, this time, the TPM will be in failure mode so
   ExecuteCommand() will simply build a failure response and return. */
LIB_EXPORT void
_plat__RunCommand(
		  uint32_t         requestSize,   // IN: command buffer size
		  unsigned char   *request,       // IN: command buffer
		  uint32_t        *responseSize,  // IN/OUT: response buffer size
		  unsigned char   **response      // IN/OUT: response buffer
		  );
/* C.8.8.2. _plat__Fail() */
/* This is the platform depended failure exit for the TPM. */
LIB_EXPORT NORETURN void
_plat__Fail(
	    void
	    );

/* C.8.9. From Unique.c */
/* C.8.9.1 _plat__GetUnique() */
/* This function is used to access the platform-specific unique value. This function places the
   unique value in the provided buffer (b) and returns the number of bytes transferred. The function
   will not copy more data than bSize. */
/* NOTE: If a platform unique value has unequal distribution of uniqueness and bSize is smaller than
   the size of the unique value, the bSize portion with the most uniqueness should be returned. */
LIB_EXPORT uint32_t
_plat__GetUnique(
		 uint32_t             which,         // authorities (0) or details
		 uint32_t             bSize,         // size of the buffer
		 unsigned char       *b              // output buffer
		 );

/* libtpms added begin */
#ifndef VBOX
#include <time.h>
#else
#undef CLOCK_REALTIME
#undef CLOCK_MONOTONIC
typedef enum
{
    CLOCK_REALTIME = 0,
    CLOCK_MONOTONIC
} TPM_CLOCK_ID;
#endif
void ClockAdjustPostResume(UINT64 backthen, BOOL timesAreRealtime);
#ifndef VBOX
uint64_t ClockGetTime(clockid_t clk_id);
#else
uint64_t ClockGetTime(TPM_CLOCK_ID clk_id);
#endif
/* libtpms added end */

#endif  // _PLATFORM_FP_H_
