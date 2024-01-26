/** @file
 * Virtualization Infrastructure Driver (VID) API.
 */

/*
 * Copyright (C) 2018-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_nt_vid_h
#define IPRT_INCLUDED_nt_vid_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "hyperv.h"


/**
 * Output from VidMessageSlotMap.
 */
typedef struct VID_MAPPED_MESSAGE_SLOT
{
    /** The message block mapping. */
    struct _HV_MESSAGE *pMsgBlock;
    /** Copy of input iCpu. */
    uint32_t            iCpu;
    /** Explicit padding.   */
    uint32_t            uParentAdvisory;
} VID_MAPPED_MESSAGE_SLOT;
/** Pointer to VidMessageSlotMap output structure. */
typedef VID_MAPPED_MESSAGE_SLOT *PVID_MAPPED_MESSAGE_SLOT;


/** @name VID_MESSAGE_MAPPING_HEADER::enmVidMsgType values (wild guess).
 * @{ */
/** Type mask, strips flags. */
#define VID_MESSAGE_TYPE_MASK                   UINT32_C(0x00ffffff)
/** No return message necessary. */
#define VID_MESSAGE_TYPE_FLAG_NO_RETURN         UINT32_C(0x01000000)
/** Observed message values. */
typedef enum
{
    /** Invalid zero value. */
    VidMessageInvalid = 0,
    /** Guessing this means a message from the hypervisor.  */
    VidMessageHypervisorMessage   = 0x00000c | VID_MESSAGE_TYPE_FLAG_NO_RETURN,
    /** Guessing this means stop request completed.  Message length is 1 byte. */
    VidMessageStopRequestComplete = 0x00000d | VID_MESSAGE_TYPE_FLAG_NO_RETURN,
} VID_MESSAGE_TYPE;
AssertCompileSize(VID_MESSAGE_TYPE, 4);
/** @} */

/**
 * Header of the message mapping returned by VidMessageSlotMap.
 */
typedef struct VID_MESSAGE_MAPPING_HEADER
{
    /** Current guess is that this is VID_MESSAGE_TYPE. */
    VID_MESSAGE_TYPE    enmVidMsgType;
    /** The message size or so it seems (0x100). */
    uint32_t            cbMessage;
    /** So far these have been zero. */
    uint32_t            aZeroPPadding[2+4];
} VID_MESSAGE_MAPPING_HEADER;
AssertCompileSize(VID_MESSAGE_MAPPING_HEADER, 32);

/**
 * VID processor status (VidGetVirtualProcessorRunningStatus).
 *
 * @note This is used internally in VID.SYS, in 17101 it's at offset 8 in their
 *       'pVCpu' structure.
 */
typedef enum
{
    VidProcessorStatusStopped = 0,
    VidProcessorStatusRunning,
    VidProcessorStatusSuspended,
    VidProcessorStatusUndefined = 0xffff
} VID_PROCESSOR_STATUS;
AssertCompileSize(VID_PROCESSOR_STATUS, 4);


/** I/O control input for VidMessageSlotHandleAndGetNext. */
typedef struct VID_IOCTL_INPUT_MESSAGE_SLOT_HANDLE_AND_GET_NEXT
{
    HV_VP_INDEX         iCpu;
    uint32_t            fFlags;         /**< VID_MSHAGN_F_GET_XXX*/
    uint32_t            cMillies;       /**< Not present in build 17758 as the API changed to always to infinite waits. */
} VID_IOCTL_INPUT_MESSAGE_SLOT_HANDLE_AND_GET_NEXT;
/** Pointer to input for VidMessageSlotHandleAndGetNext. */
typedef VID_IOCTL_INPUT_MESSAGE_SLOT_HANDLE_AND_GET_NEXT *PVID_IOCTL_INPUT_MESSAGE_SLOT_HANDLE_AND_GET_NEXT;
/** Pointer to const input for VidMessageSlotHandleAndGetNext. */
typedef VID_IOCTL_INPUT_MESSAGE_SLOT_HANDLE_AND_GET_NEXT const *PCVID_IOCTL_INPUT_MESSAGE_SLOT_HANDLE_AND_GET_NEXT;

/** @name VID_MSHAGN_F_GET_XXX - Flags for VidMessageSlotHandleAndGetNext
 * @{ */
/** This will try get the next message, waiting if necessary.
 * It is subject to NtAlertThread processing when it starts waiting.  */
#define VID_MSHAGN_F_GET_NEXT_MESSAGE   RT_BIT_32(0)
/** ACK the message as handled and resume execution/whatever.
 * This is executed before VID_MSHAGN_F_GET_NEXT_MESSAGE and should not be
 * subject to NtAlertThread side effects. */
#define VID_MSHAGN_F_HANDLE_MESSAGE     RT_BIT_32(1)
/** Cancel VP execution (no other bit set).
 * @since about build 17758. */
#define VID_MSHAGN_F_CANCEL             RT_BIT_32(2)
/** @} */

/** A 64-bit version of HV_PARTITION_PROPERTY_CODE. */
typedef int64_t VID_PARTITION_PROPERTY_CODE;


#ifdef IN_RING3
RT_C_DECLS_BEGIN

/** Calling convention. */
#ifndef WINAPI
# define VIDAPI __stdcall
#else
# define VIDAPI WINAPI
#endif

/** Partition handle. */
#ifndef WINAPI
typedef void *VID_PARTITION_HANDLE;
#else
typedef HANDLE VID_PARTITION_HANDLE;
#endif

/**
 * Gets the partition ID.
 *
 * The partition ID is the numeric identifier used when making hypercalls to the
 * hypervisor.
 *
 * @note Starting with Windows 11 (or possibly earlier), this does not work on
 *       Exo partition as created by WHvCreatePartition.  It returns a
 *       STATUS_NOT_IMPLEMENTED as the I/O control code is not allowed through.
 *       All partitions has an ID though, so just pure annoying blockheadedness
 *       sprung upon us w/o any chance of doing a memory managment rewrite in
 *       time.
 */
DECLIMPORT(BOOL) VIDAPI VidGetHvPartitionId(VID_PARTITION_HANDLE hPartition, HV_PARTITION_ID *pidPartition);

/**
 * Get a partition property.
 *
 * @returns Success indicator (details in LastErrorValue).
 * @param   hPartition  The partition handle.
 * @param   enmProperty The property to get.  Is a HV_PARTITION_PROPERTY_CODE
 *                      type, but seems to be passed around as a 64-bit integer
 *                      for some reason.
 * @param   puValue     Where to return the property value.
 */
DECLIMPORT(BOOL) VIDAPI VidGetPartitionProperty(VID_PARTITION_HANDLE hPartition, VID_PARTITION_PROPERTY_CODE enmProperty,
                                                PHV_PARTITION_PROPERTY puValue);

/**
 * @copydoc VidGetPartitionProperty
 * @note Currently (Windows 11 GA) identical to VidGetPartitionProperty.
 */
DECLIMPORT(BOOL) VIDAPI VidGetExoPartitionProperty(VID_PARTITION_HANDLE hPartition, VID_PARTITION_PROPERTY_CODE enmProperty,
                                                   PHV_PARTITION_PROPERTY puValue);

/**
 * Starts asynchronous execution of a virtual CPU.
 */
DECLIMPORT(BOOL) VIDAPI VidStartVirtualProcessor(VID_PARTITION_HANDLE hPartition, HV_VP_INDEX iCpu);

/**
 * Stops the asynchronous execution of a virtual CPU.
 *
 * @retval ERROR_VID_STOP_PENDING if busy with intercept, check messages.
 */
DECLIMPORT(BOOL) VIDAPI VidStopVirtualProcessor(VID_PARTITION_HANDLE hPartition, HV_VP_INDEX iCpu);

/**
 * WHvCreateVirtualProcessor boils down to a call to VidMessageSlotMap and
 * some internal WinHvPlatform state fiddling.
 *
 * Looks like it maps memory and returns the pointer to it.
 * VidMessageSlotHandleAndGetNext is later used to wait for the next message and
 * put (??) it into that memory mapping.
 *
 * @returns Success indicator (details in LastErrorValue).
 *
 * @param   hPartition  The partition handle.
 * @param   pOutput     Where to return the pointer to the message memory
 *                      mapping.  The CPU index is also returned here.
 * @param   iCpu        The CPU to wait-and-get messages for.
 */
DECLIMPORT(BOOL) VIDAPI VidMessageSlotMap(VID_PARTITION_HANDLE hPartition, PVID_MAPPED_MESSAGE_SLOT pOutput, HV_VP_INDEX iCpu);

/**
 * This is used by WHvRunVirtualProcessor to wait for the next exit msg.
 *
 * The message appears in the memory mapping returned by VidMessageSlotMap.
 *
 * @returns Success indicator (details only in LastErrorValue - LastStatusValue
 *          is not set).
 * @retval  STATUS_TIMEOUT for STATUS_TIMEOUT as well as STATUS_USER_APC and
 *          STATUS_ALERTED.
 *
 * @param   hPartition  The partition handle.
 * @param   iCpu        The CPU to wait-and-get messages for.
 * @param   fFlags      Flags, VID_MSHAGN_F_XXX.
 *
 *                      When starting or resuming execution, at least one of
 *                      VID_MSHAGN_F_GET_NEXT_MESSAGE (bit 0) and
 *                      VID_MSHAGN_F_HANDLE_MESSAGE (bit 1) must be set.
 *
 *                      When cancelling execution only VID_MSHAGN_F_CANCEL (big 2)
 *                      must be set.
 *
 * @param   cMillies    The timeout, presumably in milliseconds.  This parameter
 *                      was dropped about build 17758.
 *
 * @todo    Would be awfully nice if someone at Microsoft could hit at the
 *          flags here.
 */
DECLIMPORT(BOOL) VIDAPI VidMessageSlotHandleAndGetNext(VID_PARTITION_HANDLE hPartition, HV_VP_INDEX iCpu,
                                                       uint32_t fFlags, uint32_t cMillies);
/**
 * Gets the processor running status.
 *
 * This is probably only available in special builds, as one of the early I/O
 * control dispatching routines will not let it thru.  Lower down routines does
 * implement it, so it's possible to patch it into working.  This works for
 * build 17101: eb vid+12180 0f 84 98 00 00 00
 *
 * @retval  ERROR_NOT_IMPLEMENTED
 *
 * @remarks VidExoFastIoControlPartition probably disapproves of this too.  It
 *          could be very handy for debugging upon occation.
 */
DECLIMPORT(BOOL) VIDAPI VidGetVirtualProcessorRunningStatus(VID_PARTITION_HANDLE hPartition, HV_VP_INDEX iCpu,
                                                            VID_PROCESSOR_STATUS *penmStatus);

/**
 * For query virtual processor registers and other state information.
 *
 * @returns Success indicator (details in LastErrorValue).
 */
DECLIMPORT(BOOL) VIDAPI VidGetVirtualProcessorState(VID_PARTITION_HANDLE hPartition, HV_VP_INDEX iCpu,
                                                    HV_REGISTER_NAME const *paRegNames, uint32_t cRegisters,
                                                    HV_REGISTER_VALUE *paRegValues);

/**
 * For setting virtual processor registers and other state information.
 *
 * @returns Success indicator (details in LastErrorValue).
 */
DECLIMPORT(BOOL) VIDAPI VidSetVirtualProcessorState(VID_PARTITION_HANDLE hPartition, HV_VP_INDEX iCpu,
                                                    HV_REGISTER_NAME const *paRegNames, uint32_t cRegisters,
                                                    HV_REGISTER_VALUE const *paRegValues);

/**
 * Wrapper around the HvCallGetMemoryBalance hypercall.
 *
 * When VID.SYS processes the request, it will also query
 * HvPartitionPropertyVirtualTlbPageCount, so we're passing a 3rd return
 * parameter in case the API is ever extended to match the I/O control.
 *
 * @returns Success indicator (details in LastErrorValue).
 * @retval  ERROR_NOT_IMPLEMENTED for exo partitions.
 *
 * @param   hPartition          The partition handle.
 * @param   pcPagesAvailable    Where to return the number of unused pages
 *                              still available to the partition.
 * @param   pcPagesInUse        Where to return the number of pages currently
 *                              in use by the partition.
 * @param   pReserved           Pointer to dummy value, just in case they
 *                              modify the API to include the nested TLB size.
 *
 * @note    Not available for exo partitions, unfortunately.  The
 *          VidExoFastIoControlPartition function deflects it, failing it with
 *          STATUS_NOT_IMPLEMENTED / ERROR_NOT_IMPLEMENTED.
 */
DECLIMPORT(BOOL) VIDAPI VidGetHvMemoryBalance(VID_PARTITION_HANDLE hPartition, uint64_t *pcPagesAvailable,
                                              uint64_t *pcPagesInUse, uint64_t *pReserved);

RT_C_DECLS_END
#endif /* IN_RING3 */

#endif /* !IPRT_INCLUDED_nt_vid_h */

