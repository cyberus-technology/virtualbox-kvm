/** @file
 * MM - The Memory Manager.
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_vmm_mm_h
#define VBOX_INCLUDED_vmm_mm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <iprt/x86.h>
#include <VBox/sup.h>
#include <iprt/stdarg.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_mm       The Memory Manager API
 * @ingroup grp_vmm
 * @{
 */

/**
 * Memory Allocation Tags.
 * For use with MMHyperAlloc(), MMR3HeapAlloc(), MMR3HeapAllocEx(),
 * MMR3HeapAllocZ() and MMR3HeapAllocZEx().
 *
 * @remark Don't forget to update the dump command in MMHeap.cpp!
 */
typedef enum MMTAG
{
    MM_TAG_INVALID = 0,

    MM_TAG_CFGM,
    MM_TAG_CFGM_BYTES,
    MM_TAG_CFGM_STRING,
    MM_TAG_CFGM_USER,

    MM_TAG_CSAM,
    MM_TAG_CSAM_PATCH,

    MM_TAG_CPUM_CTX,
    MM_TAG_CPUM_CPUID,
    MM_TAG_CPUM_MSRS,

    MM_TAG_DBGF,
    MM_TAG_DBGF_AS,
    MM_TAG_DBGF_CORE_WRITE,
    MM_TAG_DBGF_INFO,
    MM_TAG_DBGF_LINE,
    MM_TAG_DBGF_LINE_DUP,
    MM_TAG_DBGF_MODULE,
    MM_TAG_DBGF_OS,
    MM_TAG_DBGF_REG,
    MM_TAG_DBGF_STACK,
    MM_TAG_DBGF_SYMBOL,
    MM_TAG_DBGF_SYMBOL_DUP,
    MM_TAG_DBGF_TYPE,
    MM_TAG_DBGF_TRACER,
    MM_TAG_DBGF_FLOWTRACE,

    MM_TAG_EM,

    MM_TAG_IEM,

    MM_TAG_IOM,
    MM_TAG_IOM_STATS,

    MM_TAG_MM,
    MM_TAG_MM_LOOKUP_GUEST,
    MM_TAG_MM_LOOKUP_PHYS,
    MM_TAG_MM_LOOKUP_VIRT,
    MM_TAG_MM_PAGE,

    MM_TAG_PARAV,

    MM_TAG_PATM,
    MM_TAG_PATM_PATCH,

    MM_TAG_PDM,
    MM_TAG_PDM_ASYNC_COMPLETION,
    MM_TAG_PDM_DEVICE,
    MM_TAG_PDM_DEVICE_DESC,
    MM_TAG_PDM_DEVICE_USER,
    MM_TAG_PDM_DRIVER,
    MM_TAG_PDM_DRIVER_DESC,
    MM_TAG_PDM_DRIVER_USER,
    MM_TAG_PDM_USB,
    MM_TAG_PDM_USB_DESC,
    MM_TAG_PDM_USB_USER,
    MM_TAG_PDM_LUN,
#ifdef VBOX_WITH_NETSHAPER
    MM_TAG_PDM_NET_SHAPER,
#endif /* VBOX_WITH_NETSHAPER */
    MM_TAG_PDM_QUEUE,
    MM_TAG_PDM_THREAD,

    MM_TAG_PGM,
    MM_TAG_PGM_CHUNK_MAPPING,
    MM_TAG_PGM_HANDLERS,
    MM_TAG_PGM_HANDLER_TYPES,
    MM_TAG_PGM_MAPPINGS,
    MM_TAG_PGM_PHYS,
    MM_TAG_PGM_POOL,

    MM_TAG_REM,

    MM_TAG_SELM,

    MM_TAG_SSM,

    MM_TAG_STAM,

    MM_TAG_TM,

    MM_TAG_TRPM,

    MM_TAG_VM,
    MM_TAG_VM_REQ,

    MM_TAG_VMM,

    MM_TAG_HM,

    MM_TAG_32BIT_HACK = 0x7fffffff
} MMTAG;




/** @defgroup grp_mm_hyper  Hypervisor Memory Management
 * @{ */

VMMDECL(RTR0PTR)    MMHyperR3ToR0(PVM pVM, RTR3PTR R3Ptr);

#ifndef IN_RING3
VMMDECL(void *)     MMHyperR3ToCC(PVM pVM, RTR3PTR R3Ptr);
#else
DECLINLINE(void *)  MMHyperR3ToCC(PVM pVM, RTR3PTR R3Ptr)
{
    NOREF(pVM);
    return R3Ptr;
}
#endif


/** @def MMHYPER_RC_ASSERT_RCPTR
 * Asserts that an address is either NULL or inside the hypervisor memory area.
 * This assertion only works while IN_RC, it's a NOP everywhere else.
 * @thread  The Emulation Thread.
 */
#define MMHYPER_RC_ASSERT_RCPTR(pVM, RCPtr)   do { } while (0)

/** @} */


#if defined(IN_RING3) || defined(DOXYGEN_RUNNING)
/** @defgroup grp_mm_r3    The MM Host Context Ring-3 API
 * @{
 */

VMMR3DECL(int)      MMR3InitUVM(PUVM pUVM);
VMMR3DECL(int)      MMR3Init(PVM pVM);
VMMR3DECL(int)      MMR3InitPaging(PVM pVM);
VMMR3DECL(int)      MMR3Term(PVM pVM);
VMMR3DECL(void)     MMR3TermUVM(PUVM pUVM);
VMMR3DECL(int)      MMR3ReserveHandyPages(PVM pVM, uint32_t cHandyPages);
VMMR3DECL(int)      MMR3IncreaseBaseReservation(PVM pVM, uint64_t cAddBasePages);
VMMR3DECL(int)      MMR3AdjustFixedReservation(PVM pVM, int32_t cDeltaFixedPages, const char *pszDesc);
VMMR3DECL(int)      MMR3UpdateShadowReservation(PVM pVM, uint32_t cShadowPages);
/** @} */


/** @defgroup grp_mm_phys   Guest Physical Memory Manager
 * @todo retire this group, elimintating or moving MMR3PhysGetRamSize to PGMPhys.
 * @{ */
VMMR3DECL(uint64_t) MMR3PhysGetRamSize(PVM pVM);
VMMR3DECL(uint32_t) MMR3PhysGetRamSizeBelow4GB(PVM pVM);
VMMR3DECL(uint64_t) MMR3PhysGetRamSizeAbove4GB(PVM pVM);
VMMR3DECL(uint32_t) MMR3PhysGet4GBRamHoleSize(PVM pVM);
/** @} */


/** @defgroup grp_mm_heap   Heap Manager
 * @{ */
VMMR3DECL(void *)   MMR3HeapAlloc(PVM pVM, MMTAG enmTag, size_t cbSize);
VMMR3DECL(void *)   MMR3HeapAllocU(PUVM pUVM, MMTAG enmTag, size_t cbSize);
VMMR3DECL(int)      MMR3HeapAllocEx(PVM pVM, MMTAG enmTag, size_t cbSize, void **ppv);
VMMR3DECL(int)      MMR3HeapAllocExU(PUVM pUVM, MMTAG enmTag, size_t cbSize, void **ppv);
VMMR3DECL(void *)   MMR3HeapAllocZ(PVM pVM, MMTAG enmTag, size_t cbSize);
VMMR3DECL(void *)   MMR3HeapAllocZU(PUVM pUVM, MMTAG enmTag, size_t cbSize);
VMMR3DECL(int)      MMR3HeapAllocZEx(PVM pVM, MMTAG enmTag, size_t cbSize, void **ppv);
VMMR3DECL(int)      MMR3HeapAllocZExU(PUVM pUVM, MMTAG enmTag, size_t cbSize, void **ppv);
VMMR3DECL(void *)   MMR3HeapRealloc(void *pv, size_t cbNewSize);
VMMR3DECL(char *)   MMR3HeapStrDup(PVM pVM, MMTAG enmTag, const char *psz);
VMMR3DECL(char *)   MMR3HeapStrDupU(PUVM pUVM, MMTAG enmTag, const char *psz);
VMMR3DECL(char *)   MMR3HeapAPrintf(PVM pVM, MMTAG enmTag, const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(3, 4);
VMMR3DECL(char *)   MMR3HeapAPrintfU(PUVM pUVM, MMTAG enmTag, const char *pszFormat, ...) RT_IPRT_FORMAT_ATTR(3, 4);
VMMR3DECL(char *)   MMR3HeapAPrintfV(PVM pVM, MMTAG enmTag, const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(3, 0);
VMMR3DECL(char *)   MMR3HeapAPrintfVU(PUVM pUVM, MMTAG enmTag, const char *pszFormat, va_list va) RT_IPRT_FORMAT_ATTR(3, 0);
VMMR3DECL(void)     MMR3HeapFree(void *pv);
/** @} */

#endif /* IN_RING3 || DOXYGEN_RUNNING */


/** @} */
RT_C_DECLS_END


#endif /* !VBOX_INCLUDED_vmm_mm_h */

