/********************************************************************************/
/*										*/
/*			     				*/
/*			     Written by Ken Goldman				*/
/*		       IBM Thomas J. Watson Research Center			*/
/*            $Id: NV.h $			*/
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

#ifndef NV_H
#define NV_H

/* 5.14.1	Index Type Definitions */
/* These definitions allow the same code to be used pre and post 1.21. The main action is to
   redefine the index type values from the bit values. Use TPM_NT_ORDINARY to indicate if the TPM_NT
   type is defined */
#ifdef     TPM_NT_ORDINARY
/* If TPM_NT_ORDINARY is defined, then the TPM_NT field is present in a TPMA_NV */
#   define GET_TPM_NT(attributes) GET_ATTRIBUTE(attributes, TPMA_NV, TPM_NT)
#else
/* If TPM_NT_ORDINARY is not defined, then need to synthesize it from the attributes */
#   define GetNv_TPM_NV(attributes)					\
    (   IS_ATTRIBUTE(attributes, TPMA_NV, COUNTER)			\
	+   (IS_ATTRIBUTE(attributes, TPMA_NV, BITS) << 1)		\
	+   (IS_ATTRIBUTE(attributes, TPMA_NV, EXTEND) << 2)		\
	)
#   define TPM_NT_ORDINARY (0)
#   define TPM_NT_COUNTER  (1)
#   define TPM_NT_BITS     (2)
#   define TPM_NT_EXTEND   (4)
#endif
/* 5.14.2	Attribute Macros */
/* These macros are used to isolate the differences in the way that the index type changed in
   version 1.21 of the specification */
#   define IsNvOrdinaryIndex(attributes)				\
    (GET_TPM_NT(attributes) == TPM_NT_ORDINARY)
#   define  IsNvCounterIndex(attributes)				\
    (GET_TPM_NT(attributes) == TPM_NT_COUNTER)
#   define  IsNvBitsIndex(attributes)					\
    (GET_TPM_NT(attributes) == TPM_NT_BITS)
#   define  IsNvExtendIndex(attributes)					\
    (GET_TPM_NT(attributes) == TPM_NT_EXTEND)
#ifdef TPM_NT_PIN_PASS
#   define  IsNvPinPassIndex(attributes)				\
    (GET_TPM_NT(attributes) == TPM_NT_PIN_PASS)
#endif
#ifdef TPM_NT_PIN_FAIL
#   define  IsNvPinFailIndex(attributes)				\
    (GET_TPM_NT(attributes) == TPM_NT_PIN_FAIL)
#endif
typedef struct {
    UINT32      size;
    TPM_HANDLE  handle;
} NV_ENTRY_HEADER;
#define NV_EVICT_OBJECT_SIZE						\
    (sizeof(UINT32)  + sizeof(TPM_HANDLE) + sizeof(OBJECT))
#define NV_INDEX_COUNTER_SIZE						\
    (sizeof(UINT32) + sizeof(NV_INDEX) + sizeof(UINT64))
#define NV_RAM_INDEX_COUNTER_SIZE			\
    (sizeof(NV_RAM_HEADER) + sizeof(UINT64))
typedef struct {
    UINT32          size;
    TPM_HANDLE      handle;
    TPMA_NV         attributes;
} NV_RAM_HEADER;
/* Defines the end-of-list marker for NV. The list terminator is a UINT32 of zero, followed by the
   current value of s_maxCounter which is a 64-bit value. The structure is defined as an array of 3
   UINT32 values so that there is no padding between the UINT32 list end marker and the UINT64
   maxCounter value. */
typedef UINT32 NV_LIST_TERMINATOR[3];
/* 5.14.3	Orderly RAM Values */
/* The following defines are for accessing orderly RAM values. This is the initialize for the RAM
   reference iterator. */
#define     NV_RAM_REF_INIT         0
/* This is the starting address of the RAM space used for orderly data */
#define     RAM_ORDERLY_START			\
    (&s_indexOrderlyRam[0])
/* This is the offset within NV that is used to save the orderly data on an orderly shutdown. */
#define     NV_ORDERLY_START			\
    (NV_INDEX_RAM_DATA)
/* This is the end of the orderly RAM space. It is actually the first byte after the last byte of
   orderly RAM data */
#define     RAM_ORDERLY_END						\
    (RAM_ORDERLY_START + sizeof(s_indexOrderlyRam))
/* This is the end of the orderly space in NV memory. As with RAM_ORDERLY_END, it is actually the
   offset of the first byte after the end of the NV orderly data. */
#define     NV_ORDERLY_END						\
    (NV_ORDERLY_START + sizeof(s_indexOrderlyRam))
/* Macro to check that an orderly RAM address is with range. */
#define ORDERLY_RAM_ADDRESS_OK(start, offset)				\
    ((start >= RAM_ORDERLY_START) && ((start + offset - 1) < RAM_ORDERLY_END))
#define RETURN_IF_NV_IS_NOT_AVAILABLE			    \
    {							    \
	if(g_NvStatus != TPM_RC_SUCCESS)			    \
	    return g_NvStatus;					    \
    }
/* Routinely have to clear the orderly flag and fail if the NV is not available so that it can be
   cleared. */
#define RETURN_IF_ORDERLY				    \
    {							    \
	if(NvClearOrderly() != TPM_RC_SUCCESS)			    \
	    return g_NvStatus;					    \
    }
#define NV_IS_AVAILABLE     (g_NvStatus == TPM_RC_SUCCESS)
#define IS_ORDERLY(value)   (value < SU_DA_USED_VALUE)
#define NV_IS_ORDERLY       (IS_ORDERLY(gp.orderlyState))
/* Macro to set the NV UPDATE_TYPE. This deals with the fact that the update is possibly a
   combination of UT_NV and UT_ORDERLY. */
#define SET_NV_UPDATE(type)     g_updateNV |= (type)
#endif  // _NV_H_
