/** @file
 * TM - Time Manager.
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

#ifndef VBOX_INCLUDED_vmm_tm_h
#define VBOX_INCLUDED_vmm_tm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#ifdef IN_RING3
# include <iprt/time.h>
#endif

RT_C_DECLS_BEGIN

/** @defgroup grp_tm        The Time Manager API
 * @ingroup grp_vmm
 * @{
 */

/** Enable a timer hack which improves the timer response/resolution a bit. */
#define VBOX_HIGH_RES_TIMERS_HACK


/**
 * Clock type.
 */
typedef enum TMCLOCK
{
    /** Real host time.
     * This clock ticks all the time, so use with care. */
    TMCLOCK_REAL = 0,
    /** Virtual guest time.
     * This clock only ticks when the guest is running.  It's implemented
     * as an offset to monotonic real time (GIP). */
    TMCLOCK_VIRTUAL,
    /** Virtual guest synchronized timer time.
     * This is a special clock and timer queue for synchronizing virtual timers
     * and virtual time sources.  This clock is trying to keep up with
     * TMCLOCK_VIRTUAL, but will wait for timers to be executed.  If it lags
     * too far behind TMCLOCK_VIRTUAL, it will try speed up to close the
     * distance.
     * @remarks Do not use this unless you really *must*. */
    TMCLOCK_VIRTUAL_SYNC,
    /** Virtual CPU timestamp.
     * By default this is a function of TMCLOCK_VIRTUAL_SYNC and the virtual
     * CPU frequency. */
    TMCLOCK_TSC,
    /** Number of clocks. */
    TMCLOCK_MAX
} TMCLOCK;


/** @defgroup grp_tm_timer_flags Timer flags.
 * @{ */
/** Use the default critical section for the class of timers. */
#define TMTIMER_FLAGS_DEFAULT_CRIT_SECT 0
/** No critical section needed or a custom one is set using
 *  TMR3TimerSetCritSect(). */
#define TMTIMER_FLAGS_NO_CRIT_SECT      RT_BIT_32(0)
/** Used in ring-0.  Must set this or TMTIMER_FLAGS_NO_RING0. */
#define TMTIMER_FLAGS_RING0             RT_BIT_32(1)
/** Not used in ring-0 (for refactoring and doc purposes). */
#define TMTIMER_FLAGS_NO_RING0          RT_BIT_32(31)
/** @} */


VMMDECL(void)           TMNotifyStartOfExecution(PVMCC pVM, PVMCPUCC pVCpu);
VMMDECL(void)           TMNotifyEndOfExecution(PVMCC pVM, PVMCPUCC pVCpu, uint64_t uTsc);
VMM_INT_DECL(void)      TMNotifyStartOfHalt(PVMCPUCC pVCpu);
VMM_INT_DECL(void)      TMNotifyEndOfHalt(PVMCPUCC pVCpu);
#ifdef IN_RING3
VMMR3DECL(int)          TMR3NotifySuspend(PVM pVM, PVMCPU pVCpu);
VMMR3DECL(int)          TMR3NotifyResume(PVM pVM, PVMCPU pVCpu);
VMMR3DECL(int)          TMR3SetWarpDrive(PUVM pUVM, uint32_t u32Percent);
VMMR3DECL(uint32_t)     TMR3GetWarpDrive(PUVM pUVM);
#endif
VMM_INT_DECL(uint32_t)  TMCalcHostTimerFrequency(PVMCC pVM, PVMCPUCC pVCpu);
#ifdef IN_RING3
VMMR3DECL(int)          TMR3GetCpuLoadTimes(PVM pVM, VMCPUID idCpu, uint64_t *pcNsTotal, uint64_t *pcNsExecuting,
                                            uint64_t *pcNsHalted, uint64_t *pcNsOther);
VMMR3DECL(int)          TMR3GetCpuLoadPercents(PUVM pVUM, VMCPUID idCpu, uint64_t *pcMsInterval, uint8_t *pcPctExecuting,
                                               uint8_t *pcPctHalted, uint8_t *pcPctOther);
#endif


/** @name Real Clock Methods
 * @{
 */
VMM_INT_DECL(uint64_t)  TMRealGet(PVM pVM);
VMM_INT_DECL(uint64_t)  TMRealGetFreq(PVM pVM);
/** @} */


/** @name Virtual Clock Methods
 * @{
 */
VMM_INT_DECL(uint64_t)  TMVirtualGet(PVMCC pVM);
VMM_INT_DECL(uint64_t)  TMVirtualGetNoCheck(PVMCC pVM);
VMM_INT_DECL(uint64_t)  TMVirtualSyncGetLag(PVMCC pVM);
VMM_INT_DECL(uint32_t)  TMVirtualSyncGetCatchUpPct(PVMCC pVM);
VMM_INT_DECL(uint64_t)  TMVirtualGetFreq(PVM pVM);
VMM_INT_DECL(uint64_t)  TMVirtualSyncGet(PVMCC pVM);
VMM_INT_DECL(uint64_t)  TMVirtualSyncGetNoCheck(PVMCC pVM);
VMM_INT_DECL(uint64_t)  TMVirtualSyncGetNoCheckWithTsc(PVMCC pVM, uint64_t *puTscNow);
VMM_INT_DECL(uint64_t)  TMVirtualSyncGetEx(PVMCC pVM, bool fCheckTimers);
VMM_INT_DECL(uint64_t)  TMVirtualSyncGetWithDeadlineNoCheck(PVMCC pVM, uint64_t *pcNsToDeadline,
                                                            uint64_t *puDeadlineVersion, uint64_t *puTscNow);
VMMDECL(uint64_t)       TMVirtualSyncGetNsToDeadline(PVMCC pVM, uint64_t *puDeadlineVersion, uint64_t *puTscNow);
VMM_INT_DECL(bool)      TMVirtualSyncIsCurrentDeadlineVersion(PVMCC pVM, uint64_t uDeadlineVersion);
VMM_INT_DECL(uint64_t)  TMVirtualToNano(PVM pVM, uint64_t u64VirtualTicks);
VMM_INT_DECL(uint64_t)  TMVirtualToMicro(PVM pVM, uint64_t u64VirtualTicks);
VMM_INT_DECL(uint64_t)  TMVirtualToMilli(PVM pVM, uint64_t u64VirtualTicks);
VMM_INT_DECL(uint64_t)  TMVirtualFromNano(PVM pVM, uint64_t u64NanoTS);
VMM_INT_DECL(uint64_t)  TMVirtualFromMicro(PVM pVM, uint64_t u64MicroTS);
VMM_INT_DECL(uint64_t)  TMVirtualFromMilli(PVM pVM, uint64_t u64MilliTS);
VMM_INT_DECL(bool)      TMVirtualIsTicking(PVM pVM);

VMMR3DECL(uint64_t)     TMR3TimeVirtGet(PUVM pUVM);
VMMR3DECL(uint64_t)     TMR3TimeVirtGetMilli(PUVM pUVM);
VMMR3DECL(uint64_t)     TMR3TimeVirtGetMicro(PUVM pUVM);
VMMR3DECL(uint64_t)     TMR3TimeVirtGetNano(PUVM pUVM);
/** @} */


/** @name CPU Clock Methods
 * @{
 */
VMMDECL(uint64_t)       TMCpuTickGet(PVMCPUCC pVCpu);
VMM_INT_DECL(uint64_t)  TMCpuTickGetNoCheck(PVMCPUCC pVCpu);
VMM_INT_DECL(bool)      TMCpuTickCanUseRealTSC(PVMCC pVM, PVMCPUCC pVCpu, uint64_t *poffRealTSC, bool *pfParavirtTsc);
VMM_INT_DECL(uint64_t)  TMCpuTickGetDeadlineAndTscOffset(PVMCC pVM, PVMCPUCC pVCpu, uint64_t *poffRealTsc,
                                                         bool *pfOffsettedTsc, bool *pfParavirtTsc,
                                                         uint64_t *puTscNow, uint64_t *puDeadlineVersion);
VMM_INT_DECL(int)       TMCpuTickSet(PVMCC pVM, PVMCPUCC pVCpu, uint64_t u64Tick);
VMM_INT_DECL(int)       TMCpuTickSetLastSeen(PVMCPUCC pVCpu, uint64_t u64LastSeenTick);
VMM_INT_DECL(uint64_t)  TMCpuTickGetLastSeen(PVMCPUCC pVCpu);
VMMDECL(uint64_t)       TMCpuTicksPerSecond(PVMCC pVM);
VMM_INT_DECL(bool)      TMCpuTickIsTicking(PVMCPUCC pVCpu);
/** @} */


/** @name Timer Methods
 * @{
 */
/**
 * Device timer callback function.
 *
 * @param   pDevIns         Device instance of the device which registered the timer.
 * @param   hTimer          The timer handle.
 * @param   pvUser          User argument specified upon timer creation.
 */
typedef DECLCALLBACKTYPE(void, FNTMTIMERDEV,(PPDMDEVINS pDevIns, TMTIMERHANDLE hTimer, void *pvUser));
/** Pointer to a device timer callback function. */
typedef FNTMTIMERDEV *PFNTMTIMERDEV;

/**
 * USB device timer callback function.
 *
 * @param   pUsbIns         The USB device instance the timer is associated
 *                          with.
 * @param   hTimer          The timer handle.
 * @param   pvUser          User argument specified upon timer creation.
 */
typedef DECLCALLBACKTYPE(void, FNTMTIMERUSB,(PPDMUSBINS pUsbIns, TMTIMERHANDLE hTimer, void *pvUser));
/** Pointer to a timer callback for a USB device. */
typedef FNTMTIMERUSB *PFNTMTIMERUSB;

/**
 * Driver timer callback function.
 *
 * @param   pDrvIns         Device instance of the device which registered the timer.
 * @param   hTimer          The timer handle.
 * @param   pvUser          User argument specified upon timer creation.
 */
typedef DECLCALLBACKTYPE(void, FNTMTIMERDRV,(PPDMDRVINS pDrvIns, TMTIMERHANDLE hTimer, void *pvUser));
/** Pointer to a driver timer callback function. */
typedef FNTMTIMERDRV *PFNTMTIMERDRV;

/**
 * Service timer callback function.
 *
 * @param   pSrvIns         Service instance of the device which registered the timer.
 * @param   hTimer          The timer handle.
 */
typedef DECLCALLBACKTYPE(void, FNTMTIMERSRV,(PPDMSRVINS pSrvIns, TMTIMERHANDLE hTimer));
/** Pointer to a service timer callback function. */
typedef FNTMTIMERSRV *PFNTMTIMERSRV;

/**
 * Internal timer callback function.
 *
 * @param   pVM             The cross context VM structure.
 * @param   hTimer          The timer handle.
 * @param   pvUser          User argument specified upon timer creation.
 */
typedef DECLCALLBACKTYPE(void, FNTMTIMERINT,(PVM pVM, TMTIMERHANDLE hTimer, void *pvUser));
/** Pointer to internal timer callback function. */
typedef FNTMTIMERINT *PFNTMTIMERINT;

/**
 * External timer callback function.
 *
 * @param   pvUser          User argument as specified when the timer was created.
 */
typedef DECLCALLBACKTYPE(void, FNTMTIMEREXT,(void *pvUser));
/** Pointer to an external timer callback function. */
typedef FNTMTIMEREXT *PFNTMTIMEREXT;

VMMDECL(int)            TMTimerLock(PVMCC pVM, TMTIMERHANDLE hTimer, int rcBusy);
VMMDECL(void)           TMTimerUnlock(PVMCC pVM, TMTIMERHANDLE hTimer);
VMMDECL(bool)           TMTimerIsLockOwner(PVMCC pVM, TMTIMERHANDLE hTimer);
VMMDECL(int)            TMTimerSet(PVMCC pVM, TMTIMERHANDLE hTimer, uint64_t u64Expire);
VMMDECL(int)            TMTimerSetRelative(PVMCC pVM, TMTIMERHANDLE hTimer, uint64_t cTicksToNext, uint64_t *pu64Now);
VMMDECL(int)            TMTimerSetFrequencyHint(PVMCC pVM, TMTIMERHANDLE hTimer, uint32_t uHz);
VMMDECL(uint64_t)       TMTimerGet(PVMCC pVM, TMTIMERHANDLE hTimer);
VMMDECL(int)            TMTimerStop(PVMCC pVM, TMTIMERHANDLE hTimer);
VMMDECL(bool)           TMTimerIsActive(PVMCC pVM, TMTIMERHANDLE hTimer);

VMMDECL(int)            TMTimerSetMillies(PVMCC pVM, TMTIMERHANDLE hTimer, uint32_t cMilliesToNext);
VMMDECL(int)            TMTimerSetMicro(PVMCC pVM, TMTIMERHANDLE hTimer, uint64_t cMicrosToNext);
VMMDECL(int)            TMTimerSetNano(PVMCC pVM, TMTIMERHANDLE hTimer, uint64_t cNanosToNext);
VMMDECL(uint64_t)       TMTimerGetNano(PVMCC pVM, TMTIMERHANDLE hTimer);
VMMDECL(uint64_t)       TMTimerGetMicro(PVMCC pVM, TMTIMERHANDLE hTimer);
VMMDECL(uint64_t)       TMTimerGetMilli(PVMCC pVM, TMTIMERHANDLE hTimer);
VMMDECL(uint64_t)       TMTimerGetFreq(PVMCC pVM, TMTIMERHANDLE hTimer);
VMMDECL(uint64_t)       TMTimerGetExpire(PVMCC pVM, TMTIMERHANDLE hTimer);
VMMDECL(uint64_t)       TMTimerToNano(PVMCC pVM, TMTIMERHANDLE hTimer, uint64_t cTicks);
VMMDECL(uint64_t)       TMTimerToMicro(PVMCC pVM, TMTIMERHANDLE hTimer, uint64_t cTicks);
VMMDECL(uint64_t)       TMTimerToMilli(PVMCC pVM, TMTIMERHANDLE hTimer, uint64_t cTicks);
VMMDECL(uint64_t)       TMTimerFromNano(PVMCC pVM, TMTIMERHANDLE hTimer, uint64_t cNanoSecs);
VMMDECL(uint64_t)       TMTimerFromMicro(PVMCC pVM, TMTIMERHANDLE hTimer, uint64_t cMicroSecs);
VMMDECL(uint64_t)       TMTimerFromMilli(PVMCC pVM, TMTIMERHANDLE hTimer, uint64_t cMilliSecs);

VMMDECL(bool)           TMTimerPollBool(PVMCC pVM, PVMCPUCC pVCpu);
VMM_INT_DECL(void)      TMTimerPollVoid(PVMCC pVM, PVMCPUCC pVCpu);
VMM_INT_DECL(uint64_t)  TMTimerPollGIP(PVMCC pVM, PVMCPUCC pVCpu, uint64_t *pu64Delta);
/** @} */


/** @defgroup grp_tm_r3     The TM Host Context Ring-3 API
 * @{
 */
VMM_INT_DECL(int)       TMR3Init(PVM pVM);
VMM_INT_DECL(int)       TMR3InitFinalize(PVM pVM);
VMM_INT_DECL(void)      TMR3Relocate(PVM pVM, RTGCINTPTR offDelta);
VMM_INT_DECL(int)       TMR3Term(PVM pVM);
VMM_INT_DECL(void)      TMR3Reset(PVM pVM);
VMM_INT_DECL(int)       TMR3TimerCreateDevice(PVM pVM, PPDMDEVINS pDevIns, TMCLOCK enmClock, PFNTMTIMERDEV pfnCallback,
                                              void *pvUser, uint32_t fFlags, const char *pszName, PTMTIMERHANDLE phTimer);
VMM_INT_DECL(int)       TMR3TimerCreateUsb(PVM pVM, PPDMUSBINS pUsbIns, TMCLOCK enmClock, PFNTMTIMERUSB pfnCallback,
                                           void *pvUser, uint32_t fFlags, const char *pszName, PTMTIMERHANDLE phTimer);
VMM_INT_DECL(int)       TMR3TimerCreateDriver(PVM pVM, PPDMDRVINS pDrvIns, TMCLOCK enmClock, PFNTMTIMERDRV pfnCallback,
                                              void *pvUser, uint32_t fFlags, const char *pszName, PTMTIMERHANDLE phTimer);
VMMR3DECL(int)          TMR3TimerCreate(PVM pVM, TMCLOCK enmClock, PFNTMTIMERINT pfnCallback, void *pvUser, uint32_t fFlags,
                                        const char *pszName, PTMTIMERHANDLE phTimer);
VMMR3DECL(int)          TMR3TimerDestroy(PVM pVM, TMTIMERHANDLE hTimer);
VMM_INT_DECL(int)       TMR3TimerDestroyDevice(PVM pVM, PPDMDEVINS pDevIns);
VMM_INT_DECL(int)       TMR3TimerDestroyUsb(PVM pVM, PPDMUSBINS pUsbIns);
VMM_INT_DECL(int)       TMR3TimerDestroyDriver(PVM pVM, PPDMDRVINS pDrvIns);
VMMR3DECL(int)          TMR3TimerSave(PVMCC pVM, TMTIMERHANDLE hTimer, PSSMHANDLE pSSM);
VMMR3DECL(int)          TMR3TimerLoad(PVMCC pVM, TMTIMERHANDLE hTimer, PSSMHANDLE pSSM);
VMMR3DECL(int)          TMR3TimerSkip(PSSMHANDLE pSSM, bool *pfActive);
VMMR3DECL(int)          TMR3TimerSetCritSect(PVMCC pVM, TMTIMERHANDLE hTimer, PPDMCRITSECT pCritSect);
VMMR3DECL(void)         TMR3TimerQueuesDo(PVM pVM);
VMMR3_INT_DECL(void)    TMR3VirtualSyncFF(PVM pVM, PVMCPU pVCpu);
VMMR3_INT_DECL(PRTTIMESPEC) TMR3UtcNow(PVM pVM, PRTTIMESPEC pTime);

VMMR3_INT_DECL(int)     TMR3CpuTickParavirtEnable(PVM pVM);
VMMR3_INT_DECL(int)     TMR3CpuTickParavirtDisable(PVM pVM);
VMMR3_INT_DECL(bool)    TMR3CpuTickIsFixedRateMonotonic(PVM pVM, bool fWithParavirtEnabled);
/** @} */


/** @defgroup grp_tm_r0     The TM Host Context Ring-0 API
 * @{
 */
VMMR0_INT_DECL(void)    TMR0InitPerVMData(PGVM pGVM);
VMMR0_INT_DECL(void)    TMR0CleanupVM(PGVM pGVM);
VMMR0_INT_DECL(int)     TMR0TimerQueueGrow(PGVM pGVM, uint32_t idxQueue, uint32_t cMinTimers);
/** @} */


/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_tm_h */

