/** @file
 * GCM - Guest Compatibility Manager - All Contexts.
 */

/*
 * Copyright (C) 2022-2023 Oracle and/or its affiliates.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_GIM
#include <VBox/vmm/gcm.h>
#include <VBox/vmm/em.h>    /* For EMInterpretDisasCurrent */
#include "GCMInternal.h"
#include <VBox/vmm/vmcc.h>

#include <VBox/dis.h>       /* For DISCPUSTATE */
#include <iprt/errcore.h>
#include <iprt/string.h>


/**
 * Checks whether GCM is enabled for this VM.
 *
 * @retval  true if GCM is on.
 * @retval  false if no GCM fixer is enabled.
 *
 * @param   pVM       The cross context VM structure.
 */
VMMDECL(bool) GCMIsEnabled(PVM pVM)
{
    return pVM->gcm.s.enmFixerIds != GCMFIXER_NONE;
}


/**
 * Gets the GCM fixers configured for this VM.
 *
 * @returns The GCM provider Id.
 * @param   pVM     The cross context VM structure.
 */
VMMDECL(int32_t) GCMGetFixers(PVM pVM)
{
    return pVM->gcm.s.enmFixerIds;
}


/**
 * Whether \#DE exceptions in the guest should be intercepted by GCM and
 * possibly fixed up.
 *
 * @returns true if needed, false otherwise.
 * @param   pVCpu       The cross context virtual CPU structure.
 */
VMM_INT_DECL(bool) GCMShouldTrapXcptDE(PVMCPUCC pVCpu)
{
    LogFlowFunc(("entered\n"));
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    if (!GCMIsEnabled(pVM))
        return false;

    LogFunc(("GCM checking if #DE needs trapping\n"));

    /* See if the enabled fixers need to intercept #DE. */
    if (  pVM->gcm.s.enmFixerIds
        & (GCMFIXER_DBZ_DOS |  GCMFIXER_DBZ_OS2 | GCMFIXER_DBZ_WIN9X))
    {
        LogRel(("GCM: #DE should be trapped\n"));
        return true;
    }

    return false;
}


/**
 * Exception handler for \#DE when registered by GCM.
 *
 * @returns Strict VBox status code.
 * @retval  VINF_SUCCESS retry division and continue.
 * @retval  VERR_NOT_FOUND deliver exception to guest.
 *
 * @param   pVCpu       The cross context virtual CPU structure.
 * @param   pCtx        Pointer to the guest-CPU context.
 * @param   pDis        Pointer to the disassembled instruction state at RIP.
 *                      If NULL is passed, it implies the disassembly of the
 *                      the instruction at RIP is the
 *                      responsibility of GCM.
 * @param   pcbInstr    Where to store the instruction length of
 *                      the divide instruction. Optional, can be
 *                      NULL.
 *
 * @thread  EMT(pVCpu).
 */
VMM_INT_DECL(VBOXSTRICTRC) GCMXcptDE(PVMCPUCC pVCpu, PCPUMCTX pCtx, PDISCPUSTATE pDis, uint8_t *pcbInstr)
{
    PVMCC pVM = pVCpu->CTX_SUFF(pVM);
    Assert(GCMIsEnabled(pVM));
    Assert(pDis || pcbInstr);
    RT_NOREF(pDis);
    RT_NOREF(pcbInstr);

    LogRel(("GCM: Intercepted #DE at CS:RIP=%04x:%RX64 (%RX64 linear) RDX:RAX=%RX64:%RX64 RCX=%RX64 RBX=%RX64\n",
            pCtx->cs.Sel, pCtx->rip, pCtx->cs.u64Base + pCtx->rip, pCtx->rdx, pCtx->rax, pCtx->rcx, pCtx->rbx));

    if (pVM->gcm.s.enmFixerIds & GCMFIXER_DBZ_OS2)
    {
        if (pCtx->rcx == 0 && pCtx->rdx == 1 && pCtx->rax == 0x86a0)
        {
            /* OS/2 1.x drivers loaded during boot: DX:AX = 100,000, CX < 2 causes overflow. */
            /* Example: OS/2 1.0 KBD01.SYS, 16,945 bytes, dated 10/21/1987, div cx at offset 2:2ffeh */
            /* Code later merged into BASEDD01.SYS, crash fixed in OS/2 1.30.1; this should
             * fix all affected versions of OS/2 1.x.
             */
            pCtx->rcx = 2;
            return VINF_SUCCESS;
        }
        if ((uint16_t)pCtx->rbx == 0 && (uint16_t)pCtx->rdx == 0 && (uint16_t)pCtx->rax == 0x1000)
        {
            /* OS/2 2.1 and later boot loader: DX:AX = 0x1000, zero BX. May have junk in high words of all registers. */
            /* Example: OS/2 MCP2 OS2LDR, 44,544 bytes, dated 03/08/2002, idiv bx at offset 847ah */
            pCtx->rbx = (pCtx->rbx & ~0xffff) | 2;
            return VINF_SUCCESS;
        }
        if (pCtx->rbx == 0 && pCtx->rdx == 0 && pCtx->rax == 0x100)
        {
            /* OS/2 2.0 boot loader: DX:AX = 0x100, zero BX. May have junk in high words of registers. */
            /* Example: OS/2 2.0 OS2LDR, 32,256 bytes, dated 03/30/1992, idiv bx at offset 2298h */
            pCtx->rbx = 2;
            return VINF_SUCCESS;
        }
    }

    if (pVM->gcm.s.enmFixerIds & GCMFIXER_DBZ_DOS)
    {
        /* NB: For 16-bit DOS software, we must generally only compare 16-bit registers.
         * The contents of the high words may be unpredictable depending on the environment.
         * For 32-bit Windows 3.x code that is not the case.
         */
        if (pCtx->rcx == 0 && pCtx->rdx == 0 && pCtx->rax == 0x100000)
        {
            /* NDIS.386 in WfW 3.11: CalibrateStall, EDX:EAX = 0x100000, zero ECX.
             * Occurs when NDIS.386 loads.
             */
            pCtx->rcx = 0x20000;    /* Want a large divisor to shorten stalls. */
            return VINF_SUCCESS;
        }
        if (pCtx->rcx == 0 && pCtx->rdx == 0 && pCtx->rax > 0x100000)
        {
            /* NDIS.386 in WfW 3.11: NdisStallExecution, EDX:EAX = 0xYY00000, zero ECX.
             * EDX:EAX is variable, but low 20 bits of EAX must be zero and EDX is likely
             * to be zero as well.
             * Only occurs if NdisStallExecution is called to do a longish stall.
             */
            pCtx->rcx = 22;
            return VINF_SUCCESS;
        }
        if ((uint16_t)pCtx->rbx == 0 && (uint16_t)pCtx->rdx == 0 && (uint16_t)pCtx->rax == 0x64)
        {
            /* Norton Sysinfo or Diagnostics 8.0 DX:AX = 0x64 (100 decimal), zero BX. */
            pCtx->rbx = (pCtx->rbx & 0xffff0000) | 1;   /* BX = 1 */
            return VINF_SUCCESS;
        }
        if ((uint16_t)pCtx->rbx == 0 && (uint16_t)pCtx->rdx == 0 && (uint16_t)pCtx->rax == 0xff)
        {
            /* IBM PC LAN Program 1.3: DX:AX=0xff (255 decimal), zero BX. */
            /* NETWORK1.CMD, 64,324 bytes, dated 06/06/1988, div bx at offset 0xa400 in file. */
            pCtx->rbx = (pCtx->rbx & 0xffff0000) | 1;   /* BX = 1 */
            return VINF_SUCCESS;
        }
        if ((uint16_t)pCtx->rdx == 0xffff && (uint16_t)pCtx->rax == 0xffff && (uint16_t)pCtx->rcx == 0xa8c0)
        {
            /* QNX 2.15C: DX:AX=0xffffffff (-1), constant CX = 0xa8c0 (43200). */
            /* div cx at e.g. 2220:fa5 and 2220:10a0 in memory. */
            pCtx->rdx = (pCtx->rdx & 0xffff0000) | 8;   /* DX = 8 */
            return VINF_SUCCESS;
        }
        if ((uint16_t)pCtx->rax > 0x1800 && ((uint16_t)pCtx->rax & 0x3f) == 0 && (uint16_t)pCtx->rbx == 0x19)
        {
            /* 3C501.COM ODI driver v1.21: AX > ~0x1900 (-1), BX = 0x19 (25). */
            /* AX was shifted left by 6 bits so low bits must be zero. */
            /* div bl at e.g. 06b3:2f80 and offset 0x2E80 in file. */
            pCtx->rax = (pCtx->rax & 0xffff0000) | 0x8c0;   /* AX = 0x8c0 */
            return VINF_SUCCESS;
        }
        if ((uint16_t)pCtx->rcx == 0x37 && ((uint16_t)pCtx->rdx > 0x34))
        {
            /* Turbo Pascal, classic Runtime Error 200: CX = 55, DX > ~54, AX/BX variable. */
            /* div cx at variable offset in file. */
            pCtx->rdx = (pCtx->rdx & 0xffff0000) | 0x30;    /* DX = 48 */
            return VINF_SUCCESS;
        }
    }

    if (pVM->gcm.s.enmFixerIds & GCMFIXER_DBZ_WIN9X)
    {
        if (pCtx->rcx == 0 && pCtx->rdx == 0 && pCtx->rax == 0x100000)
        {
            /* NDIS.VXD in Win9x: EDX:EAX = 0x100000, zero ECX. */
            /* Example: Windows 95 NDIS.VXD, 99,084 bytes, dated 07/11/1994, div ecx at 28:Cxxxx80B */
            /* Crash fixed in Windows 98 SE. */
            pCtx->rcx = 0x20000;    /* Want a large divisor to shorten stalls. */
            return VINF_SUCCESS;
        }
        if (pCtx->rcx < 3 && pCtx->rdx == 2 && pCtx->rax == 0x540be400)
        {
            /* SCSI.PDR, ESDI506.PDR in Win95: EDX:EAX = 0x2540be400 (10,000,000,000 decimal), ECX < 3. */
            /* Example: Windows 95 SCSIPORT.PDR, 23,133 bytes, dated 07/11/1995, div ecx at 28:Cxxxx876  */
            /* Example: Win95 OSR2  ESDI506.PDR, 24,390 bytes, dated 04/24/1996, div ecx at 28:Cxxxx8E3 */
            /* Crash fixed in Windows 98. */
            pCtx->rcx = 1000;
            return VINF_SUCCESS;
        }
        if (pCtx->rcx == 0 && pCtx->rdx == 0x3d && pCtx->rax == 0x9000000)
        {
            /* Unknown source, Win9x shutdown, div ecx. */
            /* GCM: Intercepted #DE at CS:RIP=0028:c0050f8e RDX:RAX=3d:9000000 (250000*1024*1024) RCX=0 RBX=c19200e8 [RBX variable] */
            pCtx->rcx = 4096;
            return VINF_SUCCESS;
        }
    }

    /* If we got this far, deliver exception to guest. */
    return VERR_NOT_FOUND;
}
