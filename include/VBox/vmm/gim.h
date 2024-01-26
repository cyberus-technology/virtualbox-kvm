/** @file
 * GIM - Guest Interface Manager.
 */

/*
 * Copyright (C) 2014-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_vmm_gim_h
#define VBOX_INCLUDED_vmm_gim_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/param.h>

#include <VBox/vmm/cpum.h>
#include <VBox/vmm/pdmifs.h>

/** The value used to specify that VirtualBox must use the newest
 *  implementation version of the GIM provider. */
#define GIM_VERSION_LATEST                  UINT32_C(0)

RT_C_DECLS_BEGIN

/** @defgroup grp_gim   The Guest Interface Manager API
 * @ingroup grp_vmm
 * @{
 */

/**
 * GIM Provider Identifiers.
 * @remarks Part of saved state!
 */
typedef enum GIMPROVIDERID
{
    /** None. */
    GIMPROVIDERID_NONE = 0,
    /** Minimal. */
    GIMPROVIDERID_MINIMAL,
    /** Microsoft Hyper-V. */
    GIMPROVIDERID_HYPERV,
    /** Linux KVM Interface. */
    GIMPROVIDERID_KVM
} GIMPROVIDERID;
AssertCompileSize(GIMPROVIDERID, sizeof(uint32_t));


/**
 * A GIM MMIO2 region record.
 */
typedef struct GIMMMIO2REGION
{
    /** The region index. */
    uint8_t             iRegion;
    /** Whether an RC mapping is required. */
    bool                fRCMapping;
    /** Whether this region has been registered. */
    bool                fRegistered;
    /** Whether this region is currently mapped. */
    bool                fMapped;
    /** Size of the region (must be page aligned). */
    uint32_t            cbRegion;
    /** The host ring-0 address of the first page in the region. */
    R0PTRTYPE(void *)   pvPageR0;
    /** The host ring-3 address of the first page in the region. */
    R3PTRTYPE(void *)   pvPageR3;
# ifdef VBOX_WITH_RAW_MODE_KEEP
    /** The ring-context address of the first page in the region. */
    RCPTRTYPE(void *)   pvPageRC;
    RTRCPTR             RCPtrAlignment0;
# endif
    /** The guest-physical address of the first page in the region. */
    RTGCPHYS            GCPhysPage;
    /** The MMIO2 handle. */
    PGMMMIO2HANDLE      hMmio2;
    /** The description of the region. */
    char                szDescription[32];
} GIMMMIO2REGION;
/** Pointer to a GIM MMIO2 region. */
typedef GIMMMIO2REGION *PGIMMMIO2REGION;
/** Pointer to a const GIM MMIO2 region. */
typedef GIMMMIO2REGION const *PCGIMMMIO2REGION;
AssertCompileMemberAlignment(GIMMMIO2REGION, pvPageR0, 8);
AssertCompileMemberAlignment(GIMMMIO2REGION, GCPhysPage, 8);

/**
 * Debug data buffer available callback over the GIM debug connection.
 *
 * @param   pVM             The cross context VM structure.
 */
typedef DECLCALLBACKTYPE(void, FNGIMDEBUGBUFAVAIL,(PVM pVM));
/** Pointer to GIM debug buffer available callback. */
typedef FNGIMDEBUGBUFAVAIL *PFNGIMDEBUGBUFAVAIL;

/**
 * GIM debug setup.
 *
 * These are parameters/options filled in by the GIM provider and passed along
 * to the GIM device.
 */
typedef struct GIMDEBUGSETUP
{
    /** The callback to invoke when the receive buffer has data. */
    PFNGIMDEBUGBUFAVAIL     pfnDbgRecvBufAvail;
    /** The size of the receive buffer as specified by the GIM provider. */
    uint32_t                cbDbgRecvBuf;
} GIMDEBUGSETUP;
/** Pointer to a GIM debug setup struct. */
typedef struct GIMDEBUGSETUP *PGIMDEBUGSETUP;
/** Pointer to a const GIM debug setup struct. */
typedef struct GIMDEBUGSETUP const *PCGGIMDEBUGSETUP;

/**
 * GIM debug structure (common to the GIM device and GIM).
 *
 * This is used to exchanging data between the GIM provider and the GIM device.
 */
typedef struct GIMDEBUG
{
    /** The receive buffer. */
    void                   *pvDbgRecvBuf;
    /** The debug I/O stream driver. */
    PPDMISTREAM             pDbgDrvStream;
    /** Number of bytes pending to be read from the receive buffer. */
    size_t                  cbDbgRecvBufRead;
    /** The flag synchronizing reads of the receive buffer from EMT. */
    volatile bool           fDbgRecvBufRead;
    /** The receive thread wakeup semaphore. */
    RTSEMEVENTMULTI         hDbgRecvThreadSem;
} GIMDEBUG;
/** Pointer to a GIM debug struct. */
typedef struct GIMDEBUG *PGIMDEBUG;
/** Pointer to a const GIM debug struct. */
typedef struct GIMDEBUG const *PCGIMDEBUG;


#ifdef IN_RC
/** @defgroup grp_gim_rc  The GIM Raw-mode Context API
 * @{
 */
/** @} */
#endif /* IN_RC */

#ifdef IN_RING0
/** @defgroup grp_gim_r0  The GIM Host Context Ring-0 API
 * @{
 */
VMMR0_INT_DECL(int)         GIMR0InitVM(PVMCC pVM);
VMMR0_INT_DECL(int)         GIMR0TermVM(PVMCC pVM);
VMMR0_INT_DECL(int)         GIMR0UpdateParavirtTsc(PVMCC pVM, uint64_t u64Offset);
/** @} */
#endif /* IN_RING0 */


#ifdef IN_RING3
/** @defgroup grp_gim_r3  The GIM Host Context Ring-3 API
 * @{
 */
VMMR3_INT_DECL(int)         GIMR3Init(PVM pVM);
VMMR3_INT_DECL(int)         GIMR3InitCompleted(PVM pVM);
VMMR3_INT_DECL(void)        GIMR3Relocate(PVM pVM, RTGCINTPTR offDelta);
VMMR3_INT_DECL(int)         GIMR3Term(PVM pVM);
VMMR3_INT_DECL(void)        GIMR3Reset(PVM pVM);
VMMR3DECL(void)             GIMR3GimDeviceRegister(PVM pVM, PPDMDEVINS pDevInsR3, PGIMDEBUG pDbg);
VMMR3DECL(int)              GIMR3GetDebugSetup(PVM pVM, PGIMDEBUGSETUP pDbgSetup);
/** @} */
#endif /* IN_RING3 */

VMMDECL(bool)               GIMIsEnabled(PVM pVM);
VMMDECL(GIMPROVIDERID)      GIMGetProvider(PVM pVM);
VMMDECL(PGIMMMIO2REGION)    GIMGetMmio2Regions(PVMCC pVM, uint32_t *pcRegions);
VMM_INT_DECL(bool)          GIMIsParavirtTscEnabled(PVMCC pVM);
VMM_INT_DECL(bool)          GIMAreHypercallsEnabled(PVMCPUCC pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)  GIMHypercall(PVMCPUCC pVCpu, PCPUMCTX pCtx);
VMM_INT_DECL(VBOXSTRICTRC)  GIMHypercallEx(PVMCPUCC pVCpu, PCPUMCTX pCtx, unsigned uDisOpcode, uint8_t cbInstr);
VMM_INT_DECL(VBOXSTRICTRC)  GIMExecHypercallInstr(PVMCPUCC pVCpu, PCPUMCTX pCtx, uint8_t *pcbInstr);
VMM_INT_DECL(VBOXSTRICTRC)  GIMXcptUD(PVMCPUCC pVCpu, PCPUMCTX pCtx, PDISCPUSTATE pDis, uint8_t *pcbInstr);
VMM_INT_DECL(bool)          GIMShouldTrapXcptUD(PVMCPUCC pVCpu);
VMM_INT_DECL(VBOXSTRICTRC)  GIMReadMsr(PVMCPUCC pVCpu, uint32_t idMsr, PCCPUMMSRRANGE pRange, uint64_t *puValue);
VMM_INT_DECL(VBOXSTRICTRC)  GIMWriteMsr(PVMCPUCC pVCpu, uint32_t idMsr, PCCPUMMSRRANGE pRange, uint64_t uValue, uint64_t uRawValue);
VMM_INT_DECL(int)           GIMQueryHypercallOpcodeBytes(PVM pVM, void *pvBuf, size_t cbBuf,
                                                         size_t *pcbWritten, uint16_t *puDisOpcode);
/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_gim_h */

