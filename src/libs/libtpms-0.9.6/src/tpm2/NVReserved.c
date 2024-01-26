/********************************************************************************/
/*										*/
/*		NV TPM persistent and state save data  				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: NVReserved.c $		*/
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

// 8.5	NVReserved.c
/* 8.5.2 Includes, Defines and Data Definitions */
#define NV_C
#include    "Tpm.h"
/* 8.5.3	Functions */
/*     8.5.3.1 NvInitStatic() */
/* This function initializes the static variables used in the NV subsystem. */
static void
NvInitStatic(
	     void
	     )
{
    // In some implementations, the end of NV is variable and is set at boot time.
    // This value will be the same for each boot, but is not necessarily known
    // at compile time.
    s_evictNvEnd = (NV_REF)NV_MEMORY_SIZE;
    return;
}
/* 8.5.3.2 NvCheckState() */
/* Function to check the NV state by accessing the platform-specific function to get the NV state.
   The result state is registered in s_NvIsAvailable that will be reported by NvIsAvailable(). */
/* This function is called at the beginning of ExecuteCommand() before any potential check of
   g_NvStatus. */
void
NvCheckState(
	     void
	     )
{
    int     func_return;
    //
    func_return = _plat__IsNvAvailable();
    if(func_return == 0)
	g_NvStatus = TPM_RC_SUCCESS;
    else if(func_return == 1)
	g_NvStatus = TPM_RC_NV_UNAVAILABLE;
    else
	g_NvStatus = TPM_RC_NV_RATE;
    return;
}
/* 8.5.3.3 NvCommit */
/* This is a wrapper for the platform function to commit pending NV writes. */
BOOL
NvCommit(
	 void
	 )
{
    return (_plat__NvCommit() == 0);
}
/* 8.5.3.4 NvPowerOn() */
/* This function is called at _TPM_Init() to initialize the NV environment. */
/* Return Values Meaning */
/* TRUE all NV was initialized */
/* FALSE the NV containing saved state had an error and TPM2_Startup(CLEAR) is required */
BOOL
NvPowerOn(
	  void
	  )
{
    int          nvError = 0;
    // If power was lost, need to re-establish the RAM data that is loaded from
    // NV and initialize the static variables
    if(g_powerWasLost)
	{
	    if((nvError = _plat__NVEnable(0)) < 0)
		LOG_FAILURE(FATAL_ERROR_NV_UNRECOVERABLE);  /* libtpms changed */
	    NvInitStatic();
	}
    return nvError == 0;
}
/* 8.5.3.5 NvManufacture() */
/* This function initializes the NV system at pre-install time. */
/* This function should only be called in a manufacturing environment or in a simulation. */
/* The layout of NV memory space is an implementation choice. */
void
NvManufacture(
	      void
	      )
{
#if SIMULATION
    // Simulate the NV memory being in the erased state.
    _plat__NvMemoryClear(0, NV_MEMORY_SIZE);
#endif
    // Initialize static variables
    NvInitStatic();
    // Clear the RAM used for Orderly Index data
    MemorySet(s_indexOrderlyRam, 0, RAM_INDEX_SPACE);
    // Write that Orderly Index data to NV
    NvUpdateIndexOrderlyData();
    // Initialize the next offset of the first entry in evict/index list to 0 (the
    // end of list marker) and the initial s_maxCounterValue;
    NvSetMaxCount(0);
    // Put the end of list marker at the end of memory. This contains the MaxCount
    // value as well as the end marker.
    NvWriteNvListEnd(NV_USER_DYNAMIC);
    return;
}
/* 8.5.3.6 NvRead() */
/* This function is used to move reserved data from NV memory to RAM. */
void
NvRead(
       void            *outBuffer,     // OUT: buffer to receive data
       UINT32           nvOffset,      // IN: offset in NV of value
       UINT32           size           // IN: size of the value to read
       )
{
    // Input type should be valid
    pAssert(nvOffset + size < NV_MEMORY_SIZE);
    _plat__NvMemoryRead(nvOffset, size, outBuffer);
    return;
}
/* 8.5.3.7 NvWrite() */
/* This function is used to post reserved data for writing to NV memory. Before the TPM completes
   the operation, the value will be written. */
BOOL
NvWrite(
	UINT32           nvOffset,      // IN: location in NV to receive data
	UINT32           size,          // IN: size of the data to move
	void            *inBuffer       // IN: location containing data to write
	)
{
    // Input type should be valid
    if(nvOffset + size <= NV_MEMORY_SIZE)
	{
	    // Set the flag that a NV write happened
	    SET_NV_UPDATE(UT_NV);
	    return _plat__NvMemoryWrite(nvOffset, size, inBuffer);
	}
    return FALSE;
}

#if 0 // libtpms added being (for Coverity)
/* 8.5.3.8 NvUpdatePersistent() */
/* This function is used to update a value in the PERSISTENT_DATA structure and commits the value to
   NV. */
void
NvUpdatePersistent(
		   UINT32           offset,        // IN: location in PERMANENT_DATA to be updated
		   UINT32           size,          // IN: size of the value
		   void            *buffer         // IN: the new data
		   )
{
    pAssert(offset + size <= sizeof(gp));
    MemoryCopy(&gp + offset, buffer, size);
    NvWrite(offset, size, buffer);
}
/* 8.5.3.9 NvClearPersistent() */
/* This function is used to clear a persistent data entry and commit it to NV */
void
NvClearPersistent(
		  UINT32           offset,        // IN: the offset in the PERMANENT_DATA
		  //     structure to be cleared (zeroed)
		  UINT32           size           // IN: number of bytes to clear
		  )
{
    pAssert(offset + size <= sizeof(gp));
    MemorySet((&gp) + offset, 0, size);
    NvWrite(offset, size, (&gp) + offset);
}
#endif // libtpms added end
/* 8.5.3.10 NvReadPersistent() */
/* This function reads persistent data to the RAM copy of the gp structure. */
void
NvReadPersistent(
		 void
		 )
{
    NvRead(&gp, NV_PERSISTENT_DATA, sizeof(gp));
    return;
}
