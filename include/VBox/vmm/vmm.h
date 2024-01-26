/** @file
 * VMM - The Virtual Machine Monitor.
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

#ifndef VBOX_INCLUDED_vmm_vmm_h
#define VBOX_INCLUDED_vmm_vmm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <VBox/vmm/vmapi.h>
#include <VBox/sup.h>
#include <VBox/log.h>
#include <iprt/stdarg.h>
#include <iprt/thread.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_vmm       The Virtual Machine Monitor
 * @{
 */

/** @defgroup grp_vmm_api   The Virtual Machine Monitor API
 * @{
 */


/**
 * Ring-0 assertion notification callback.
 *
 * @returns VBox status code.
 * @param   pVCpu           The cross context virtual CPU structure.
 * @param   pvUser          The user argument.
 */
typedef DECLCALLBACKTYPE(int, FNVMMR0ASSERTIONNOTIFICATION,(PVMCPUCC pVCpu, void *pvUser));
/** Pointer to a FNVMMR0ASSERTIONNOTIFICATION(). */
typedef FNVMMR0ASSERTIONNOTIFICATION *PFNVMMR0ASSERTIONNOTIFICATION;

/**
 * Rendezvous callback.
 *
 * @returns VBox strict status code - EM scheduling.  Do not return
 *          informational status code other than the ones used by EM for
 *          scheduling.
 *
 * @param   pVM     The cross context VM structure.
 * @param   pVCpu   The cross context virtual CPU structure of the calling EMT.
 * @param   pvUser  The user argument.
 */
typedef DECLCALLBACKTYPE(VBOXSTRICTRC, FNVMMEMTRENDEZVOUS,(PVM pVM, PVMCPU pVCpu, void *pvUser));
/** Pointer to a rendezvous callback function. */
typedef FNVMMEMTRENDEZVOUS *PFNVMMEMTRENDEZVOUS;

/**
 * Method table that the VMM uses to call back the user of the VMM.
 */
typedef struct VMM2USERMETHODS
{
    /** Magic value (VMM2USERMETHODS_MAGIC). */
    uint32_t    u32Magic;
    /** Structure version (VMM2USERMETHODS_VERSION). */
    uint32_t    u32Version;

    /**
     * Save the VM state.
     *
     * @returns VBox status code.
     * @param   pThis       Pointer to the callback method table.
     * @param   pUVM        The user mode VM handle.
     *
     * @remarks This member shall be set to NULL if the operation is not
     *          supported.
     */
    DECLR3CALLBACKMEMBER(int, pfnSaveState,(PCVMM2USERMETHODS pThis, PUVM pUVM));
    /** @todo Move pfnVMAtError and pfnCFGMConstructor here? */

    /**
     * EMT initialization notification callback.
     *
     * This is intended for doing per-thread initialization for EMTs (like COM
     * init).
     *
     * @param   pThis       Pointer to the callback method table.
     * @param   pUVM        The user mode VM handle.
     * @param   pUVCpu      The user mode virtual CPU handle.
     *
     * @remarks This is optional and shall be set to NULL if not wanted.
     */
    DECLR3CALLBACKMEMBER(void, pfnNotifyEmtInit,(PCVMM2USERMETHODS pThis, PUVM pUVM, PUVMCPU pUVCpu));

    /**
     * EMT termination notification callback.
     *
     * This is intended for doing per-thread cleanups for EMTs (like COM).
     *
     * @param   pThis       Pointer to the callback method table.
     * @param   pUVM        The user mode VM handle.
     * @param   pUVCpu      The user mode virtual CPU handle.
     *
     * @remarks This is optional and shall be set to NULL if not wanted.
     */
    DECLR3CALLBACKMEMBER(void, pfnNotifyEmtTerm,(PCVMM2USERMETHODS pThis, PUVM pUVM, PUVMCPU pUVCpu));

    /**
     * PDM thread initialization notification callback.
     *
     * This is intended for doing per-thread initialization (like COM init).
     *
     * @param   pThis       Pointer to the callback method table.
     * @param   pUVM        The user mode VM handle.
     *
     * @remarks This is optional and shall be set to NULL if not wanted.
     */
    DECLR3CALLBACKMEMBER(void, pfnNotifyPdmtInit,(PCVMM2USERMETHODS pThis, PUVM pUVM));

    /**
     * EMT termination notification callback.
     *
     * This is intended for doing per-thread cleanups for EMTs (like COM).
     *
     * @param   pThis       Pointer to the callback method table.
     * @param   pUVM        The user mode VM handle.
     *
     * @remarks This is optional and shall be set to NULL if not wanted.
     */
    DECLR3CALLBACKMEMBER(void, pfnNotifyPdmtTerm,(PCVMM2USERMETHODS pThis, PUVM pUVM));

    /**
     * Notification callback that that a VM reset will be turned into a power off.
     *
     * @param   pThis       Pointer to the callback method table.
     * @param   pUVM        The user mode VM handle.
     *
     * @remarks This is optional and shall be set to NULL if not wanted.
     */
    DECLR3CALLBACKMEMBER(void, pfnNotifyResetTurnedIntoPowerOff,(PCVMM2USERMETHODS pThis, PUVM pUVM));

    /**
     * Generic object query by UUID.
     *
     * @returns pointer to queried the object on success, NULL if not found.
     *
     * @param   pThis       Pointer to the callback method table.
     * @param   pUVM        The user mode VM handle.
     * @param   pUuid       The UUID of what's being queried.  The UUIDs and the
     *                      usage conventions are defined by the user.
     *
     * @remarks This is optional and shall be set to NULL if not wanted.
     */
    DECLR3CALLBACKMEMBER(void *, pfnQueryGenericObject,(PCVMM2USERMETHODS pThis, PUVM pUVM, PCRTUUID pUuid));

    /** Magic value (VMM2USERMETHODS_MAGIC) marking the end of the structure. */
    uint32_t    u32EndMagic;
} VMM2USERMETHODS;

/** Magic value of the VMM2USERMETHODS (Franz Kafka). */
#define VMM2USERMETHODS_MAGIC         UINT32_C(0x18830703)
/** The VMM2USERMETHODS structure version. */
#define VMM2USERMETHODS_VERSION       UINT32_C(0x00030000)


/**
 * Checks whether we've armed the ring-0 long jump machinery.
 *
 * @returns @c true / @c false
 * @param   a_pVCpu     The caller's cross context virtual CPU structure.
 * @thread  EMT
 * @sa      VMMR0IsLongJumpArmed
 */
#ifdef IN_RING0
# define VMMIsLongJumpArmed(a_pVCpu)                VMMR0IsLongJumpArmed(a_pVCpu)
#else
# define VMMIsLongJumpArmed(a_pVCpu)                (false)
#endif


VMMDECL(VMCPUID)            VMMGetCpuId(PVMCC pVM);
VMMDECL(PVMCPUCC)           VMMGetCpu(PVMCC pVM);
VMMDECL(PVMCPUCC)           VMMGetCpu0(PVMCC pVM);
VMMDECL(PVMCPUCC)           VMMGetCpuById(PVMCC pVM, VMCPUID idCpu);
VMMR3DECL(PVMCPUCC)         VMMR3GetCpuByIdU(PUVM pVM, VMCPUID idCpu);
VMM_INT_DECL(uint32_t)      VMMGetSvnRev(void);
VMM_INT_DECL(void)          VMMTrashVolatileXMMRegs(void);


/** @defgroup grp_vmm_api_r0    The VMM Host Context Ring 0 API
 * @{
 */

/**
 * The VMMR0Entry() codes.
 */
typedef enum VMMR0OPERATION
{
    /** Run guest code using the available hardware acceleration technology. */
    VMMR0_DO_HM_RUN = SUP_VMMR0_DO_HM_RUN,
    /** Official NOP that we use for profiling. */
    VMMR0_DO_NEM_RUN = SUP_VMMR0_DO_NEM_RUN,
    /** Official NOP that we use for profiling. */
    VMMR0_DO_NOP = SUP_VMMR0_DO_NOP,
    /** Official slow iocl NOP that we use for profiling. */
    VMMR0_DO_SLOW_NOP,

    /** Ask the GVMM to create a new VM. */
    VMMR0_DO_GVMM_CREATE_VM = 32,
    /** Ask the GVMM to destroy the VM. */
    VMMR0_DO_GVMM_DESTROY_VM,
    /** Call GVMMR0RegisterVCpu(). */
    VMMR0_DO_GVMM_REGISTER_VMCPU,
    /** Call GVMMR0DeregisterVCpu(). */
    VMMR0_DO_GVMM_DEREGISTER_VMCPU,
    /** Call GVMMR0RegisterWorkerThread(). */
    VMMR0_DO_GVMM_REGISTER_WORKER_THREAD,
    /** Call GVMMR0DeregisterWorkerThread(). */
    VMMR0_DO_GVMM_DEREGISTER_WORKER_THREAD,
    /** Call GVMMR0SchedHalt(). */
    VMMR0_DO_GVMM_SCHED_HALT,
    /** Call GVMMR0SchedWakeUp(). */
    VMMR0_DO_GVMM_SCHED_WAKE_UP,
    /** Call GVMMR0SchedPoke(). */
    VMMR0_DO_GVMM_SCHED_POKE,
    /** Call GVMMR0SchedWakeUpAndPokeCpus(). */
    VMMR0_DO_GVMM_SCHED_WAKE_UP_AND_POKE_CPUS,
    /** Call GVMMR0SchedPoll(). */
    VMMR0_DO_GVMM_SCHED_POLL,
    /** Call GVMMR0QueryStatistics(). */
    VMMR0_DO_GVMM_QUERY_STATISTICS,
    /** Call GVMMR0ResetStatistics(). */
    VMMR0_DO_GVMM_RESET_STATISTICS,

    /** Call VMMR0 Per VM Init. */
    VMMR0_DO_VMMR0_INIT = 64,
    /** Call VMMR0 Per VM EMT Init */
    VMMR0_DO_VMMR0_INIT_EMT,
    /** Call VMMR0 Per VM Termination. */
    VMMR0_DO_VMMR0_TERM,
    /** Copy logger settings from userland, VMMR0UpdateLoggersReq(). */
    VMMR0_DO_VMMR0_UPDATE_LOGGERS,
    /** Used by the log flusher, VMMR0LogFlusher.  */
    VMMR0_DO_VMMR0_LOG_FLUSHER,
    /** Used by EMTs to wait for the log flusher to finish, VMMR0LogWaitFlushed.  */
    VMMR0_DO_VMMR0_LOG_WAIT_FLUSHED,

    /** Setup hardware-assisted VM session. */
    VMMR0_DO_HM_SETUP_VM = 128,
    /** Attempt to enable or disable hardware-assisted mode. */
    VMMR0_DO_HM_ENABLE,

    /** Call PGMR0PhysAllocateHandyPages(). */
    VMMR0_DO_PGM_ALLOCATE_HANDY_PAGES = 192,
    /** Call PGMR0PhysFlushHandyPages(). */
    VMMR0_DO_PGM_FLUSH_HANDY_PAGES,
    /** Call PGMR0AllocateLargePage(). */
    VMMR0_DO_PGM_ALLOCATE_LARGE_PAGE,
    /** Call PGMR0PhysSetupIommu(). */
    VMMR0_DO_PGM_PHYS_SETUP_IOMMU,
    /** Call PGMR0PoolGrow(). */
    VMMR0_DO_PGM_POOL_GROW,
    /** Call PGMR0PhysHandlerInitReqHandler(). */
    VMMR0_DO_PGM_PHYS_HANDLER_INIT,

    /** Call GMMR0InitialReservation(). */
    VMMR0_DO_GMM_INITIAL_RESERVATION = 256,
    /** Call GMMR0UpdateReservation(). */
    VMMR0_DO_GMM_UPDATE_RESERVATION,
    /** Call GMMR0AllocatePages(). */
    VMMR0_DO_GMM_ALLOCATE_PAGES,
    /** Call GMMR0FreePages(). */
    VMMR0_DO_GMM_FREE_PAGES,
    /** Call GMMR0FreeLargePage(). */
    VMMR0_DO_GMM_FREE_LARGE_PAGE,
    /** Call GMMR0QueryHypervisorMemoryStatsReq(). */
    VMMR0_DO_GMM_QUERY_HYPERVISOR_MEM_STATS,
    /** Call GMMR0QueryMemoryStatsReq(). */
    VMMR0_DO_GMM_QUERY_MEM_STATS,
    /** Call GMMR0BalloonedPages(). */
    VMMR0_DO_GMM_BALLOONED_PAGES,
    /** Call GMMR0MapUnmapChunk(). */
    VMMR0_DO_GMM_MAP_UNMAP_CHUNK,
    /** Call GMMR0RegisterSharedModule. */
    VMMR0_DO_GMM_REGISTER_SHARED_MODULE,
    /** Call GMMR0UnregisterSharedModule. */
    VMMR0_DO_GMM_UNREGISTER_SHARED_MODULE,
    /** Call GMMR0ResetSharedModules. */
    VMMR0_DO_GMM_RESET_SHARED_MODULES,
    /** Call GMMR0CheckSharedModules. */
    VMMR0_DO_GMM_CHECK_SHARED_MODULES,
    /** Call GMMR0FindDuplicatePage. */
    VMMR0_DO_GMM_FIND_DUPLICATE_PAGE,
    /** Call GMMR0QueryStatistics(). */
    VMMR0_DO_GMM_QUERY_STATISTICS,
    /** Call GMMR0ResetStatistics(). */
    VMMR0_DO_GMM_RESET_STATISTICS,

    /** Call PDMR0DriverCallReqHandler. */
    VMMR0_DO_PDM_DRIVER_CALL_REQ_HANDLER = 320,
    /** Call PDMR0DeviceCreateReqHandler. */
    VMMR0_DO_PDM_DEVICE_CREATE,
    /** Call PDMR0DeviceGenCallReqHandler. */
    VMMR0_DO_PDM_DEVICE_GEN_CALL,
    /** Old style device compat: Set ring-0 critical section. */
    VMMR0_DO_PDM_DEVICE_COMPAT_SET_CRITSECT,
    /** Call PDMR0QueueCreateReqHandler. */
    VMMR0_DO_PDM_QUEUE_CREATE,

    /** Set a GVMM or GMM configuration value. */
    VMMR0_DO_GCFGM_SET_VALUE = 400,
    /** Query a GVMM or GMM configuration value. */
    VMMR0_DO_GCFGM_QUERY_VALUE,

    /** The start of the R0 service operations. */
    VMMR0_DO_SRV_START = 448,
    /** Call IntNetR0Open(). */
    VMMR0_DO_INTNET_OPEN,
    /** Call IntNetR0IfClose(). */
    VMMR0_DO_INTNET_IF_CLOSE,
    /** Call IntNetR0IfGetBufferPtrs(). */
    VMMR0_DO_INTNET_IF_GET_BUFFER_PTRS,
    /** Call IntNetR0IfSetPromiscuousMode(). */
    VMMR0_DO_INTNET_IF_SET_PROMISCUOUS_MODE,
    /** Call IntNetR0IfSetMacAddress(). */
    VMMR0_DO_INTNET_IF_SET_MAC_ADDRESS,
    /** Call IntNetR0IfSetActive(). */
    VMMR0_DO_INTNET_IF_SET_ACTIVE,
    /** Call IntNetR0IfSend(). */
    VMMR0_DO_INTNET_IF_SEND,
    /** Call IntNetR0IfWait(). */
    VMMR0_DO_INTNET_IF_WAIT,
    /** Call IntNetR0IfAbortWait(). */
    VMMR0_DO_INTNET_IF_ABORT_WAIT,

#if 0
    /** Forward call to the PCI driver */
    VMMR0_DO_PCIRAW_REQ = 512,
#endif

    /** The end of the R0 service operations. */
    VMMR0_DO_SRV_END,

    /** Call NEMR0InitVM() (host specific). */
    VMMR0_DO_NEM_INIT_VM = 576,
    /** Call NEMR0InitVMPart2() (host specific). */
    VMMR0_DO_NEM_INIT_VM_PART_2,
    /** Call NEMR0MapPages() (host specific). */
    VMMR0_DO_NEM_MAP_PAGES,
    /** Call NEMR0UnmapPages() (host specific). */
    VMMR0_DO_NEM_UNMAP_PAGES,
    /** Call NEMR0ExportState() (host specific). */
    VMMR0_DO_NEM_EXPORT_STATE,
    /** Call NEMR0ImportState() (host specific). */
    VMMR0_DO_NEM_IMPORT_STATE,
    /** Call NEMR0QueryCpuTick() (host specific). */
    VMMR0_DO_NEM_QUERY_CPU_TICK,
    /** Call NEMR0ResumeCpuTickOnAll() (host specific). */
    VMMR0_DO_NEM_RESUME_CPU_TICK_ON_ALL,
    /** Call NEMR0UpdateStatistics() (host specific). */
    VMMR0_DO_NEM_UPDATE_STATISTICS,
    /** Call NEMR0DoExperiment() (host specific, experimental, debug only). */
    VMMR0_DO_NEM_EXPERIMENT,

    /** Grow the I/O port registration tables. */
    VMMR0_DO_IOM_GROW_IO_PORTS = 640,
    /** Grow the I/O port statistics tables. */
    VMMR0_DO_IOM_GROW_IO_PORT_STATS,
    /** Grow the MMIO registration tables. */
    VMMR0_DO_IOM_GROW_MMIO_REGS,
    /** Grow the MMIO statistics tables. */
    VMMR0_DO_IOM_GROW_MMIO_STATS,
    /** Synchronize statistics indices for I/O ports and MMIO regions. */
    VMMR0_DO_IOM_SYNC_STATS_INDICES,

    /** Call DBGFR0TraceCreateReqHandler. */
    VMMR0_DO_DBGF_TRACER_CREATE = 704,
    /** Call DBGFR0TraceCallReqHandler. */
    VMMR0_DO_DBGF_TRACER_CALL_REQ_HANDLER,
    /** Call DBGFR0BpInitReqHandler(). */
    VMMR0_DO_DBGF_BP_INIT,
    /** Call DBGFR0BpChunkAllocReqHandler(). */
    VMMR0_DO_DBGF_BP_CHUNK_ALLOC,
    /** Call DBGFR0BpL2TblChunkAllocReqHandler(). */
    VMMR0_DO_DBGF_BP_L2_TBL_CHUNK_ALLOC,
    /** Call DBGFR0BpOwnerInitReqHandler(). */
    VMMR0_DO_DBGF_BP_OWNER_INIT,
    /** Call DBGFR0BpPortIoInitReqHandler(). */
    VMMR0_DO_DBGF_BP_PORTIO_INIT,

    /** Grow a timer queue. */
    VMMR0_DO_TM_GROW_TIMER_QUEUE = 768,

    /** Official call we use for testing Ring-0 APIs. */
    VMMR0_DO_TESTS = 2048,

    /** The usual 32-bit type blow up. */
    VMMR0_DO_32BIT_HACK = 0x7fffffff
} VMMR0OPERATION;


/**
 * Request buffer for VMMR0_DO_GCFGM_SET_VALUE and VMMR0_DO_GCFGM_QUERY_VALUE.
 * @todo Move got GCFGM.h when it's implemented.
 */
typedef struct GCFGMVALUEREQ
{
    /** The request header.*/
    SUPVMMR0REQHDR      Hdr;
    /** The support driver session handle. */
    PSUPDRVSESSION      pSession;
    /** The value.
     * This is input for the set request and output for the query. */
    uint64_t            u64Value;
    /** The variable name.
     * This is fixed sized just to make things simple for the mock-up. */
    char                szName[48];
} GCFGMVALUEREQ;
/** Pointer to a VMMR0_DO_GCFGM_SET_VALUE and VMMR0_DO_GCFGM_QUERY_VALUE request buffer.
 * @todo Move got GCFGM.h when it's implemented.
 */
typedef GCFGMVALUEREQ *PGCFGMVALUEREQ;


/**
 * Request package for VMMR0_DO_VMMR0_UPDATE_LOGGERS.
 *
 * In addition the u64Arg is selects the logger and indicates whether we're only
 * outputting to the parent VMM. See VMMR0UPDATELOGGER_F_XXX.
 */
typedef struct VMMR0UPDATELOGGERSREQ
{
    /** The request header. */
    SUPVMMR0REQHDR      Hdr;
    /** The current logger flags (RTLOGFLAGS). */
    uint64_t            fFlags;
    /** Groups, assuming same group layout as ring-3. */
    uint32_t            cGroups;
    /** CRC32 of the group names. */
    uint32_t            uGroupCrc32;
    /** Per-group settings, variable size. */
    RT_FLEXIBLE_ARRAY_EXTENSION
    uint32_t            afGroups[RT_FLEXIBLE_ARRAY];
} VMMR0UPDATELOGGERSREQ;
/** Pointer to a VMMR0_DO_VMMR0_UPDATE_LOGGERS request. */
typedef VMMR0UPDATELOGGERSREQ *PVMMR0UPDATELOGGERSREQ;

/** @name VMMR0UPDATELOGGER_F_XXX - u64Arg definitions for VMMR0_DO_VMMR0_UPDATE_LOGGERS.
 * @{ */
/** Logger index mask. */
#define VMMR0UPDATELOGGER_F_LOGGER_MASK         UINT64_C(0x0001)
/** Only flush to the parent VMM's debug log, don't return to ring-3. */
#define VMMR0UPDATELOGGER_F_TO_PARENT_VMM_DBG   UINT64_C(0x0002)
/** Only flush to the parent VMM's debug log, don't return to ring-3. */
#define VMMR0UPDATELOGGER_F_TO_PARENT_VMM_REL   UINT64_C(0x0004)
/** Valid flag mask. */
#define VMMR0UPDATELOGGER_F_VALID_MASK          UINT64_C(0x0007)
/** @} */

#if defined(IN_RING0) || defined(DOXYGEN_RUNNING)

/**
 * Structure VMMR0EmtPrepareToBlock uses to pass info to
 * VMMR0EmtResumeAfterBlocking.
 */
typedef struct VMMR0EMTBLOCKCTX
{
    /** Magic value (VMMR0EMTBLOCKCTX_MAGIC). */
    uint32_t    uMagic;
    /** Set if we were in HM context, clear if not. */
    bool        fWasInHmContext;
} VMMR0EMTBLOCKCTX;
/** Pointer to a VMMR0EmtPrepareToBlock context structure. */
typedef VMMR0EMTBLOCKCTX *PVMMR0EMTBLOCKCTX;
/** Magic value for VMMR0EMTBLOCKCTX::uMagic (Paul Desmond). */
#define VMMR0EMTBLOCKCTX_MAGIC          UINT32_C(0x19261125)
/** Magic value for VMMR0EMTBLOCKCTX::uMagic when its out of context. */
#define VMMR0EMTBLOCKCTX_MAGIC_DEAD     UINT32_C(0x19770530)

VMMR0DECL(void)      VMMR0EntryFast(PGVM pGVM, PVMCC pVM, VMCPUID idCpu, VMMR0OPERATION enmOperation);
VMMR0DECL(int)       VMMR0EntryEx(PGVM pGVM, PVMCC pVM, VMCPUID idCpu, VMMR0OPERATION enmOperation,
                                  PSUPVMMR0REQHDR pReq, uint64_t u64Arg, PSUPDRVSESSION);
VMMR0_INT_DECL(int)  VMMR0InitPerVMData(PGVM pGVM);
VMMR0_INT_DECL(int)  VMMR0TermVM(PGVM pGVM, VMCPUID idCpu);
VMMR0_INT_DECL(void) VMMR0CleanupVM(PGVM pGVM);
VMMR0_INT_DECL(bool) VMMR0IsLongJumpArmed(PVMCPUCC pVCpu);
VMMR0_INT_DECL(int)  VMMR0ThreadCtxHookCreateForEmt(PVMCPUCC pVCpu);
VMMR0_INT_DECL(void) VMMR0ThreadCtxHookDestroyForEmt(PVMCPUCC pVCpu);
VMMR0_INT_DECL(void) VMMR0ThreadCtxHookDisable(PVMCPUCC pVCpu);
VMMR0_INT_DECL(bool) VMMR0ThreadCtxHookIsEnabled(PVMCPUCC pVCpu);
VMMR0_INT_DECL(int)  VMMR0EmtPrepareToBlock(PVMCPUCC pVCpu, int rcBusy, const char *pszCaller, void *pvLock,
                                            PVMMR0EMTBLOCKCTX pCtx);
VMMR0_INT_DECL(void) VMMR0EmtResumeAfterBlocking(PVMCPUCC pVCpu, PVMMR0EMTBLOCKCTX pCtx);
VMMR0_INT_DECL(int)  VMMR0EmtWaitEventInner(PGVMCPU pGVCpu, uint32_t fFlags, RTSEMEVENT hEvent, RTMSINTERVAL cMsTimeout);
VMMR0_INT_DECL(int)  VMMR0EmtSignalSupEvent(PGVM pGVM, PGVMCPU pGVCpu, SUPSEMEVENT hEvent);
VMMR0_INT_DECL(int)  VMMR0EmtSignalSupEventByGVM(PGVM pGVM, SUPSEMEVENT hEvent);
VMMR0_INT_DECL(int)  VMMR0AssertionSetNotification(PVMCPUCC pVCpu, PFNVMMR0ASSERTIONNOTIFICATION pfnCallback, RTR0PTR pvUser);
VMMR0_INT_DECL(void) VMMR0AssertionRemoveNotification(PVMCPUCC pVCpu);
VMMR0_INT_DECL(bool) VMMR0AssertionIsNotificationSet(PVMCPUCC pVCpu);

/** @name VMMR0EMTWAIT_F_XXX - flags for VMMR0EmtWaitEventInner and friends.
 * @{ */
/** Try suppress VERR_INTERRUPTED for a little while (~10 sec). */
#define VMMR0EMTWAIT_F_TRY_SUPPRESS_INTERRUPTED     RT_BIT_32(0)
/** @} */
#endif /* IN_RING0 */

VMMR0_INT_DECL(PRTLOGGER) VMMR0GetReleaseLogger(PVMCPUCC pVCpu);
/** @} */


#if defined(IN_RING3) || defined(DOXYGEN_RUNNING)
/** @defgroup grp_vmm_api_r3    The VMM Host Context Ring 3 API
 * @{
 */
VMMR3DECL(PCVMMR3VTABLE) VMMR3GetVTable(void);
VMMR3_INT_DECL(int)     VMMR3Init(PVM pVM);
VMMR3_INT_DECL(int)     VMMR3InitR0(PVM pVM);
VMMR3_INT_DECL(int)     VMMR3InitCompleted(PVM pVM, VMINITCOMPLETED enmWhat);
VMMR3_INT_DECL(int)     VMMR3Term(PVM pVM);
VMMR3_INT_DECL(void)    VMMR3Relocate(PVM pVM, RTGCINTPTR offDelta);
VMMR3_INT_DECL(int)     VMMR3UpdateLoggers(PVM pVM);
VMMR3DECL(const char *) VMMR3GetRZAssertMsg1(PVM pVM);
VMMR3DECL(const char *) VMMR3GetRZAssertMsg2(PVM pVM);
VMMR3_INT_DECL(int)     VMMR3HmRunGC(PVM pVM, PVMCPU pVCpu);
VMMR3DECL(int)          VMMR3CallR0(PVM pVM, uint32_t uOperation, uint64_t u64Arg, PSUPVMMR0REQHDR pReqHdr);
VMMR3_INT_DECL(int)     VMMR3CallR0Emt(PVM pVM, PVMCPU pVCpu, VMMR0OPERATION enmOperation, uint64_t u64Arg, PSUPVMMR0REQHDR pReqHdr);
VMMR3_INT_DECL(VBOXSTRICTRC) VMMR3CallR0EmtFast(PVM pVM, PVMCPU pVCpu, VMMR0OPERATION enmOperation);
VMMR3DECL(void)         VMMR3FatalDump(PVM pVM, PVMCPU pVCpu, int rcErr);
VMMR3_INT_DECL(void)    VMMR3YieldSuspend(PVM pVM);
VMMR3_INT_DECL(void)    VMMR3YieldStop(PVM pVM);
VMMR3_INT_DECL(void)    VMMR3YieldResume(PVM pVM);
VMMR3_INT_DECL(void)    VMMR3SendStartupIpi(PVM pVM, VMCPUID idCpu, uint32_t uVector);
VMMR3_INT_DECL(void)    VMMR3SendInitIpi(PVM pVM, VMCPUID idCpu);
VMMR3DECL(int)          VMMR3RegisterPatchMemory(PVM pVM, RTGCPTR pPatchMem, unsigned cbPatchMem);
VMMR3DECL(int)          VMMR3DeregisterPatchMemory(PVM pVM, RTGCPTR pPatchMem, unsigned cbPatchMem);
VMMR3DECL(int)          VMMR3EmtRendezvous(PVM pVM, uint32_t fFlags, PFNVMMEMTRENDEZVOUS pfnRendezvous, void *pvUser);
/** @defgroup grp_VMMR3EmtRendezvous_fFlags     VMMR3EmtRendezvous flags
 *  @{ */
/** Execution type mask. */
#define VMMEMTRENDEZVOUS_FLAGS_TYPE_MASK            UINT32_C(0x00000007)
/** Invalid execution type. */
#define VMMEMTRENDEZVOUS_FLAGS_TYPE_INVALID         UINT32_C(0)
/** Let the EMTs execute the callback one by one (in no particular order).
 * Recursion from within the callback possible.  */
#define VMMEMTRENDEZVOUS_FLAGS_TYPE_ONE_BY_ONE      UINT32_C(1)
/** Let all the EMTs execute the callback at the same time.
 * Cannot recurse from the callback.  */
#define VMMEMTRENDEZVOUS_FLAGS_TYPE_ALL_AT_ONCE     UINT32_C(2)
/** Only execute the callback on one EMT (no particular one).
 * Recursion from within the callback possible.  */
#define VMMEMTRENDEZVOUS_FLAGS_TYPE_ONCE            UINT32_C(3)
/** Let the EMTs execute the callback one by one in ascending order.
 * Recursion from within the callback possible. */
#define VMMEMTRENDEZVOUS_FLAGS_TYPE_ASCENDING       UINT32_C(4)
/** Let the EMTs execute the callback one by one in descending order.
 * Recursion from within the callback possible. */
#define VMMEMTRENDEZVOUS_FLAGS_TYPE_DESCENDING      UINT32_C(5)
/** Stop after the first error.
 * This is not valid for any execution type where more than one EMT is active
 * at a time. */
#define VMMEMTRENDEZVOUS_FLAGS_STOP_ON_ERROR        UINT32_C(0x00000008)
/** Use VMREQFLAGS_PRIORITY when contacting the EMTs. */
#define VMMEMTRENDEZVOUS_FLAGS_PRIORITY             UINT32_C(0x00000010)
/** The valid flags. */
#define VMMEMTRENDEZVOUS_FLAGS_VALID_MASK           UINT32_C(0x0000001f)
/** @} */
VMMR3_INT_DECL(int)     VMMR3EmtRendezvousFF(PVM pVM, PVMCPU pVCpu);
VMMR3_INT_DECL(void)    VMMR3SetMayHaltInRing0(PVMCPU pVCpu, bool fMayHaltInRing0, uint32_t cNsSpinBlockThreshold);
VMMR3_INT_DECL(int)     VMMR3ReadR0Stack(PVM pVM, VMCPUID idCpu, RTHCUINTPTR R0Addr, void *pvBuf, size_t cbRead);
VMMR3_INT_DECL(void)    VMMR3InitR0StackUnwindState(PUVM pUVM, VMCPUID idCpu, PRTDBGUNWINDSTATE pState);
/** @} */
#endif /* IN_RING3 */


#if defined(IN_RC) || defined(IN_RING0) || defined(DOXYGEN_RUNNING)
/** @defgroup grp_vmm_api_rz    The VMM Raw-Mode and Ring-0 Context API
 * @{
 */
VMMRZDECL(void)     VMMRZCallRing3Disable(PVMCPUCC pVCpu);
VMMRZDECL(void)     VMMRZCallRing3Enable(PVMCPUCC pVCpu);
VMMRZDECL(bool)     VMMRZCallRing3IsEnabled(PVMCPUCC pVCpu);
/** @} */
#endif


/** Wrapper around AssertReleaseMsgReturn that avoid tripping up in the
 *  kernel when we don't have a setjmp in place. */
#ifdef IN_RING0
# define VMM_ASSERT_RELEASE_MSG_RETURN(a_pVM, a_Expr, a_Msg, a_rc) do { \
        if (RT_LIKELY(a_Expr)) { /* likely */ } \
        else \
        { \
            PVMCPUCC pVCpuAssert = VMMGetCpu(a_pVM); \
            if (pVCpuAssert && VMMR0IsLongJumpArmed(pVCpuAssert)) \
                AssertReleaseMsg(a_Expr, a_Msg); \
            else \
                AssertLogRelMsg(a_Expr, a_Msg); \
            return (a_rc); \
        } \
    } while (0)
#else
# define VMM_ASSERT_RELEASE_MSG_RETURN(a_pVM, a_Expr, a_Msg, a_rc) AssertReleaseMsgReturn(a_Expr, a_Msg, a_rc)
#endif

/** @} */

/** @} */
RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_vmm_h */
