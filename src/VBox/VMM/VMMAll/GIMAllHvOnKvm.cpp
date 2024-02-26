/*
 * Copyright (C) Cyberus Technology GmbH.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define LOG_GROUP LOG_GROUP_GIM
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/gim.h>
#include "GIMInternal.h"
#include <VBox/vmm/vm.h>

#include <iprt/assert.h>

/**
 * With GIMHvOnKvm, userspace does not need to do any HyperV emulation because
 * it all happens inside the kernel module. These stubs are merely here to make
 * GIM.cpp happy.
 */

VMM_INT_DECL(void) gimHvStartStimer(PVMCPUCC pVCpu, PCGIMHVSTIMER pHvStimer)
{
    NOREF(pVCpu); NOREF(pHvStimer);
    AssertLogRelMsg(false, ("%s", __PRETTY_FUNCTION__));
}

VMM_INT_DECL(VBOXSTRICTRC) gimHvHypercall(PVMCPUCC pVCpu, PCPUMCTX pCtx)
{
    NOREF(pVCpu); NOREF(pCtx);
    AssertLogRelMsgReturn(false, ("%s", __PRETTY_FUNCTION__), VERR_NOT_SUPPORTED);
}

VMM_INT_DECL(VBOXSTRICTRC) gimHvHypercallEx(PVMCPUCC pVCpu, PCPUMCTX pCtx, unsigned uDisOpcode, uint8_t cbInstr)
{
    NOREF(pVCpu); NOREF(pCtx); NOREF(uDisOpcode); NOREF(cbInstr);
    AssertLogRelMsgReturn(false, ("%s", __PRETTY_FUNCTION__), VERR_NOT_SUPPORTED);
}

VMM_INT_DECL(PGIMMMIO2REGION) gimHvGetMmio2Regions(PVM pVM, uint32_t *pcRegions)
{
    NOREF(pVM); NOREF(pcRegions);
    return nullptr;
}

VMM_INT_DECL(bool) gimHvAreHypercallsEnabled(PCVM pVM)
{
    NOREF(pVM);
    return false;
}

VMM_INT_DECL(bool) gimHvIsParavirtTscEnabled(PVM pVM)
{
    NOREF(pVM);
    return false;
}

VMM_INT_DECL(bool) gimHvShouldTrapXcptUD(PVMCPU pVCpu)
{
    NOREF(pVCpu);
    return false;
}

VMM_INT_DECL(VBOXSTRICTRC) gimHvXcptUD(PVMCPUCC pVCpu, PCPUMCTX pCtx, PDISCPUSTATE pDis, uint8_t *pcbInstr)
{
    NOREF(pVCpu); NOREF(pCtx); NOREF(pDis); NOREF(pcbInstr);
    AssertLogRelMsgReturn(false, ("%s", __PRETTY_FUNCTION__), VERR_NOT_SUPPORTED);
}

VMM_INT_DECL(VBOXSTRICTRC) gimHvReadMsr(PVMCPUCC pVCpu, uint32_t idMsr, PCCPUMMSRRANGE pRange, uint64_t *puValue)
{
    NOREF(pRange);

    PVMCC   pVM = pVCpu->CTX_SUFF(pVM);
    PCGIMHV pHv = &pVM->gim.s.u.Hv;

    switch (idMsr)
    {
        case MSR_GIM_HV_CRASH_CTL:
            *puValue = pHv->uCrashCtlMsr;
            return VINF_SUCCESS;

        case MSR_GIM_HV_CRASH_P0: *puValue = pHv->uCrashP0Msr;   return VINF_SUCCESS;
        case MSR_GIM_HV_CRASH_P1: *puValue = pHv->uCrashP1Msr;   return VINF_SUCCESS;
        case MSR_GIM_HV_CRASH_P2: *puValue = pHv->uCrashP2Msr;   return VINF_SUCCESS;
        case MSR_GIM_HV_CRASH_P3: *puValue = pHv->uCrashP3Msr;   return VINF_SUCCESS;
        case MSR_GIM_HV_CRASH_P4: *puValue = pHv->uCrashP4Msr;   return VINF_SUCCESS;
        default: break;
    }

    AssertLogRelMsgReturn(false, ("%s", __PRETTY_FUNCTION__), VERR_NOT_SUPPORTED);
}

VMM_INT_DECL(VBOXSTRICTRC) gimHvWriteMsr(PVMCPUCC pVCpu, uint32_t idMsr, PCCPUMMSRRANGE pRange, uint64_t uRawValue)
{
    NOREF(pRange);

    PVMCC  pVM = pVCpu->CTX_SUFF(pVM);
    PGIMHV pHv = &pVM->gim.s.u.Hv;

    switch (idMsr) {
        case MSR_GIM_HV_CRASH_CTL:
        {
            if (uRawValue & MSR_GIM_HV_CRASH_CTL_NOTIFY)
            {
                LogRel(("GIM: HyperV: Guest indicates a fatal condition! P0=%#RX64 P1=%#RX64 P2=%#RX64 P3=%#RX64 P4=%#RX64\n",
                        pHv->uCrashP0Msr, pHv->uCrashP1Msr, pHv->uCrashP2Msr, pHv->uCrashP3Msr, pHv->uCrashP4Msr));
                DBGFR3ReportBugCheck(pVM, pVCpu, DBGFEVENT_BSOD_MSR, pHv->uCrashP0Msr, pHv->uCrashP1Msr,
                                     pHv->uCrashP2Msr, pHv->uCrashP3Msr, pHv->uCrashP4Msr);
            }
            return VINF_SUCCESS;
        }
        case MSR_GIM_HV_CRASH_P0:  pHv->uCrashP0Msr = uRawValue;  return VINF_SUCCESS;
        case MSR_GIM_HV_CRASH_P1:  pHv->uCrashP1Msr = uRawValue;  return VINF_SUCCESS;
        case MSR_GIM_HV_CRASH_P2:  pHv->uCrashP2Msr = uRawValue;  return VINF_SUCCESS;
        case MSR_GIM_HV_CRASH_P3:  pHv->uCrashP3Msr = uRawValue;  return VINF_SUCCESS;
        case MSR_GIM_HV_CRASH_P4:  pHv->uCrashP4Msr = uRawValue;  return VINF_SUCCESS;
        default: break;
    }

    AssertLogRelMsgReturn(false, ("%s", __PRETTY_FUNCTION__), VERR_NOT_SUPPORTED);
}
