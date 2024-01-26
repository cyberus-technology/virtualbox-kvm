/** @file
 * GMM - The Global Memory Manager.
 */

/*
 * Copyright (C) 2007-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_vmm_gmm_h
#define VBOX_INCLUDED_vmm_gmm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/vmm/gvmm.h>
#include <VBox/sup.h>
#include <VBox/param.h>
#include <VBox/ostypes.h>
#include <iprt/avl.h>


RT_C_DECLS_BEGIN

/** @defgroup   grp_gmm     GMM - The Global Memory Manager
 * @ingroup grp_vmm
 * @{
 */

/** @def IN_GMM_R0
 * Used to indicate whether we're inside the same link module as the ring 0
 * part of the Global Memory Manager or not.
 */
#ifdef DOXYGEN_RUNNING
# define IN_GMM_R0
#endif
/** @def GMMR0DECL
 * Ring 0 GMM export or import declaration.
 * @param   type    The return type of the function declaration.
 */
#ifdef IN_GMM_R0
# define GMMR0DECL(type)    DECLEXPORT(type) VBOXCALL
#else
# define GMMR0DECL(type)    DECLIMPORT(type) VBOXCALL
#endif

/** @def IN_GMM_R3
 * Used to indicate whether we're inside the same link module as the ring 3
 * part of the Global Memory Manager or not.
 */
#ifdef DOXYGEN_RUNNING
# define IN_GMM_R3
#endif
/** @def GMMR3DECL
 * Ring 3 GMM export or import declaration.
 * @param   type    The return type of the function declaration.
 */
#ifdef IN_GMM_R3
# define GMMR3DECL(type)    DECLEXPORT(type) VBOXCALL
#else
# define GMMR3DECL(type)    DECLIMPORT(type) VBOXCALL
#endif


/** The chunk shift. (2^21 = 2 MB) */
#define GMM_CHUNK_SHIFT                 21
/** The allocation chunk size. */
#define GMM_CHUNK_SIZE                  (1U << GMM_CHUNK_SHIFT)
/** The allocation chunk size in (guest) pages. */
#define GMM_CHUNK_NUM_PAGES             (1U << (GMM_CHUNK_SHIFT - GUEST_PAGE_SHIFT))
/** The shift factor for converting a page id into a chunk id. */
#define GMM_CHUNKID_SHIFT               (GMM_CHUNK_SHIFT - GUEST_PAGE_SHIFT)
/** The last valid Chunk ID value. */
#define GMM_CHUNKID_LAST                (GMM_PAGEID_LAST >> GMM_CHUNKID_SHIFT)
/** The last valid Page ID value. */
#define GMM_PAGEID_LAST                 UINT32_C(0xfffffff0)
/** Mask out the page index from the Page ID. */
#define GMM_PAGEID_IDX_MASK             ((1U << GMM_CHUNKID_SHIFT) - 1)
/** The NIL Chunk ID value. */
#define NIL_GMM_CHUNKID                 0
/** The NIL Page ID value. */
#define NIL_GMM_PAGEID                  0

#if 0 /* wrong - these are guest page pfns and not page ids! */
/** Special Page ID used by unassigned pages. */
#define GMM_PAGEID_UNASSIGNED           0x0fffffffU
/** Special Page ID used by unsharable pages.
 * Like MMIO2, shadow and heap. This is for later, obviously. */
#define GMM_PAGEID_UNSHARABLE           0x0ffffffeU
/** The end of the valid Page IDs. This is the first special one. */
#define GMM_PAGEID_END                  0x0ffffff0U
#endif


/** @def GMM_GCPHYS_LAST
 * The last of the valid guest physical address as it applies to GMM pages.
 *
 * This must reflect the constraints imposed by the RTGCPHYS type and
 * the guest page frame number used internally in GMMPAGE.
 *
 * @note    Note this corresponds to GMM_PAGE_PFN_LAST. */
#if HC_ARCH_BITS == 64
# define GMM_GCPHYS_LAST            UINT64_C(0x00000fffffff0000)    /* 2^44 (16TB) - 0x10000 */
#else
# define GMM_GCPHYS_LAST            UINT64_C(0x0000000fffff0000)    /* 2^36 (64GB) - 0x10000 */
#endif

/**
 * Over-commitment policy.
 */
typedef enum GMMOCPOLICY
{
    /** The usual invalid 0 value. */
    GMMOCPOLICY_INVALID = 0,
    /** No over-commitment, fully backed.
     * The GMM guarantees that it will be able to allocate all of the
     * guest RAM for a VM with OC policy. */
    GMMOCPOLICY_NO_OC,
    /** to-be-determined. */
    GMMOCPOLICY_TBD,
    /** The end of the valid policy range. */
    GMMOCPOLICY_END,
    /** The usual 32-bit hack. */
    GMMOCPOLICY_32BIT_HACK = 0x7fffffff
} GMMOCPOLICY;

/**
 * VM / Memory priority.
 */
typedef enum GMMPRIORITY
{
    /** The usual invalid 0 value. */
    GMMPRIORITY_INVALID = 0,
    /** High.
     * When ballooning, ask these VMs last.
     * When running out of memory, try not to interrupt these VMs. */
    GMMPRIORITY_HIGH,
    /** Normal.
     * When ballooning, don't wait to ask these.
     * When running out of memory, pause, save and/or kill these VMs. */
    GMMPRIORITY_NORMAL,
    /** Low.
     * When ballooning, maximize these first.
     * When running out of memory, save or kill these VMs. */
    GMMPRIORITY_LOW,
    /** The end of the valid priority range. */
    GMMPRIORITY_END,
    /** The custom 32-bit type blowup. */
    GMMPRIORITY_32BIT_HACK = 0x7fffffff
} GMMPRIORITY;


/**
 * GMM Memory Accounts.
 */
typedef enum GMMACCOUNT
{
    /** The customary invalid zero entry. */
    GMMACCOUNT_INVALID = 0,
    /** Account with the base allocations. */
    GMMACCOUNT_BASE,
    /** Account with the shadow allocations. */
    GMMACCOUNT_SHADOW,
    /** Account with the fixed allocations. */
    GMMACCOUNT_FIXED,
    /** The end of the valid values. */
    GMMACCOUNT_END,
    /** The usual 32-bit value to finish it off. */
    GMMACCOUNT_32BIT_HACK = 0x7fffffff
} GMMACCOUNT;


/**
 * Balloon actions.
 */
typedef enum
{
    /** Invalid zero entry. */
    GMMBALLOONACTION_INVALID = 0,
    /** Inflate the balloon. */
    GMMBALLOONACTION_INFLATE,
    /** Deflate the balloon. */
    GMMBALLOONACTION_DEFLATE,
    /** Puncture the balloon because of VM reset. */
    GMMBALLOONACTION_RESET,
    /** End of the valid actions. */
    GMMBALLOONACTION_END,
    /** hack forcing the size of the enum to 32-bits. */
    GMMBALLOONACTION_MAKE_32BIT_HACK = 0x7fffffff
} GMMBALLOONACTION;


/**
 * A page descriptor for use when freeing pages.
 * See GMMR0FreePages, GMMR0BalloonedPages.
 */
typedef struct GMMFREEPAGEDESC
{
    /** The Page ID of the page to be freed. */
    uint32_t idPage;
} GMMFREEPAGEDESC;
/** Pointer to a page descriptor for freeing pages. */
typedef GMMFREEPAGEDESC *PGMMFREEPAGEDESC;


/**
 * A page descriptor for use when updating and allocating pages.
 *
 * This is a bit complicated because we want to do as much as possible
 * with the same structure.
 */
typedef struct GMMPAGEDESC
{
    /** The physical address of the page.
     *
     * @input   GMMR0AllocateHandyPages expects the guest physical address
     *          to update the GMMPAGE structure with. Pass GMM_GCPHYS_UNSHAREABLE
     *          when appropriate and NIL_GMMPAGEDESC_PHYS when the page wasn't used
     *          for any specific guest address.
     *
     *          GMMR0AllocatePage expects the guest physical address to put in
     *          the GMMPAGE structure for the page it allocates for this entry.
     *          Pass NIL_GMMPAGEDESC_PHYS and GMM_GCPHYS_UNSHAREABLE as above.
     *
     * @output  The host physical address of the allocated page.
     *          NIL_GMMPAGEDESC_PHYS on allocation failure.
     *
     * ASSUMES: sizeof(RTHCPHYS) >= sizeof(RTGCPHYS) and that physical addresses are
     *          limited to 63 or fewer bits (52 by AMD64 arch spec).
     */
    RT_GCC_EXTENSION
    RTHCPHYS                    HCPhysGCPhys : 63;
    /** Set if the memory was zeroed. */
    RT_GCC_EXTENSION
    RTHCPHYS                    fZeroed : 1;

    /** The Page ID.
     *
     * @input   GMMR0AllocateHandyPages expects the Page ID of the page to
     *          update here. NIL_GMM_PAGEID means no page should be updated.
     *
     *          GMMR0AllocatePages requires this to be initialized to
     *          NIL_GMM_PAGEID currently.
     *
     * @output  The ID of the page, NIL_GMM_PAGEID if the allocation failed.
     */
    uint32_t                    idPage;

    /** The Page ID of the shared page was replaced by this page.
     *
     * @input   GMMR0AllocateHandyPages expects this to indicate a shared
     *          page that has been replaced by this page and should have its
     *          reference counter decremented and perhaps be freed up. Use
     *          NIL_GMM_PAGEID if no shared page was involved.
     *
     *          All other APIs expects NIL_GMM_PAGEID here.
     *
     * @output  All APIs sets this to NIL_GMM_PAGEID.
     */
    uint32_t                    idSharedPage;
} GMMPAGEDESC;
AssertCompileSize(GMMPAGEDESC, 16);
/** Pointer to a page allocation. */
typedef GMMPAGEDESC *PGMMPAGEDESC;

/** Special NIL value for GMMPAGEDESC::HCPhysGCPhys. */
#define NIL_GMMPAGEDESC_PHYS        UINT64_C(0x7fffffffffffffff)

/** GMMPAGEDESC::HCPhysGCPhys value that indicates that the page is unsharable.
 * @note    This corresponds to GMM_PAGE_PFN_UNSHAREABLE. */
#if HC_ARCH_BITS == 64
# define GMM_GCPHYS_UNSHAREABLE     UINT64_C(0x00000fffffff1000)
#else
# define GMM_GCPHYS_UNSHAREABLE     UINT64_C(0x0000000fffff1000)
#endif


/**
 * The allocation sizes.
 */
typedef struct GMMVMSIZES
{
    /** The number of pages of base memory.
     * This is the sum of RAM, ROMs and handy pages. */
    uint64_t        cBasePages;
    /** The number of pages for the shadow pool. (Can be squeezed for memory.) */
    uint32_t        cShadowPages;
    /** The number of pages for fixed allocations like MMIO2 and the hyper heap. */
    uint32_t        cFixedPages;
} GMMVMSIZES;
/** Pointer to a GMMVMSIZES. */
typedef GMMVMSIZES *PGMMVMSIZES;


/**
 * GMM VM statistics.
 */
typedef struct GMMVMSTATS
{
    /** The reservations. */
    GMMVMSIZES          Reserved;
    /** The actual allocations.
     * This includes both private and shared page allocations. */
    GMMVMSIZES          Allocated;

    /** The current number of private pages. */
    uint64_t            cPrivatePages;
    /** The current number of shared pages. */
    uint64_t            cSharedPages;
    /** The current number of ballooned pages. */
    uint64_t            cBalloonedPages;
    /** The max number of pages that can be ballooned. */
    uint64_t            cMaxBalloonedPages;
    /** The number of pages we've currently requested the guest to give us.
     * This is 0 if no pages currently requested. */
    uint64_t            cReqBalloonedPages;
    /** The number of pages the guest has given us in response to the request.
     * This is not reset on request completed and may be used in later decisions. */
    uint64_t            cReqActuallyBalloonedPages;
    /** The number of pages we've currently requested the guest to take back. */
    uint64_t            cReqDeflatePages;
    /** The number of shareable module tracked by this VM. */
    uint32_t            cShareableModules;

    /** The current over-commitment policy. */
    GMMOCPOLICY         enmPolicy;
    /** The VM priority for arbitrating VMs in low and out of memory situation.
     * Like which VMs to start squeezing first. */
    GMMPRIORITY         enmPriority;
    /** Whether ballooning is enabled or not. */
    bool                fBallooningEnabled;
    /** Whether shared paging is enabled or not. */
    bool                fSharedPagingEnabled;
    /** Whether the VM is allowed to allocate memory or not.
     * This is used when the reservation update request fails or when the VM has
     * been told to suspend/save/die in an out-of-memory case. */
    bool                fMayAllocate;
    /** Explicit alignment. */
    bool                afReserved[1];


} GMMVMSTATS;


/**
 * The GMM statistics.
 */
typedef struct GMMSTATS
{
    /** The maximum number of pages we're allowed to allocate
     * (GMM::cMaxPages). */
    uint64_t            cMaxPages;
    /** The number of pages that has been reserved (GMM::cReservedPages). */
    uint64_t            cReservedPages;
    /** The number of pages that we have over-committed in reservations
     * (GMM::cOverCommittedPages). */
    uint64_t            cOverCommittedPages;
    /** The number of actually allocated (committed if you like) pages
     * (GMM::cAllocatedPages). */
    uint64_t            cAllocatedPages;
    /** The number of pages that are shared. A subset of cAllocatedPages.
     * (GMM::cSharedPages) */
    uint64_t            cSharedPages;
    /** The number of pages that are actually shared between VMs.
     * (GMM:cDuplicatePages) */
    uint64_t            cDuplicatePages;
    /** The number of pages that are shared that has been left behind by
     * VMs not doing proper cleanups (GMM::cLeftBehindSharedPages). */
    uint64_t            cLeftBehindSharedPages;
    /** The number of current ballooned pages (GMM::cBalloonedPages). */
    uint64_t            cBalloonedPages;
    /** The number of allocation chunks (GMM::cChunks). */
    uint32_t            cChunks;
    /** The number of freed chunks ever (GMM::cFreedChunks). */
    uint32_t            cFreedChunks;
    /** The number of shareable modules (GMM:cShareableModules). */
    uint64_t            cShareableModules;
    /** The current chunk freeing generation use by the per-VM TLB validation (GMM::idFreeGeneration). */
    uint64_t            idFreeGeneration;
    /** Space reserved for later. */
    uint64_t            au64Reserved[1];

    /** Statistics for the specified VM. (Zero filled if not requested.) */
    GMMVMSTATS          VMStats;
} GMMSTATS;
/** Pointer to the GMM statistics. */
typedef GMMSTATS *PGMMSTATS;
/** Const pointer to the GMM statistics. */
typedef const GMMSTATS *PCGMMSTATS;


GMMR0DECL(int)  GMMR0Init(void);
GMMR0DECL(void) GMMR0Term(void);
GMMR0DECL(int)  GMMR0InitPerVMData(PGVM pGVM);
GMMR0DECL(void) GMMR0CleanupVM(PGVM pGVM);
GMMR0DECL(int)  GMMR0InitialReservation(PGVM pGVM, VMCPUID idCpu, uint64_t cBasePages, uint32_t cShadowPages, uint32_t cFixedPages,
                                        GMMOCPOLICY enmPolicy, GMMPRIORITY enmPriority);
GMMR0DECL(int)  GMMR0UpdateReservation(PGVM pGVM, VMCPUID idCpu, uint64_t cBasePages, uint32_t cShadowPages, uint32_t cFixedPages);
GMMR0DECL(int)  GMMR0AllocateHandyPages(PGVM pGVM, VMCPUID idCpu, uint32_t cPagesToUpdate,
                                        uint32_t cPagesToAlloc, PGMMPAGEDESC paPages);
GMMR0DECL(int)  GMMR0AllocatePages(PGVM pGVM, VMCPUID idCpu, uint32_t cPages, PGMMPAGEDESC paPages, GMMACCOUNT enmAccount);
GMMR0DECL(int)  GMMR0AllocateLargePage(PGVM pGVM, VMCPUID idCpu, uint32_t cbPage, uint32_t *pIdPage, RTHCPHYS *pHCPhys);
GMMR0DECL(int)  GMMR0FreePages(PGVM pGVM, VMCPUID idCpu, uint32_t cPages, PGMMFREEPAGEDESC paPages, GMMACCOUNT enmAccount);
GMMR0DECL(int)  GMMR0FreeLargePage(PGVM pGVM, VMCPUID idCpu, uint32_t idPage);
GMMR0DECL(int)  GMMR0BalloonedPages(PGVM pGVM, VMCPUID idCpu, GMMBALLOONACTION enmAction, uint32_t cBalloonedPages);
GMMR0DECL(int)  GMMR0MapUnmapChunk(PGVM pGVM, uint32_t idChunkMap, uint32_t idChunkUnmap, PRTR3PTR ppvR3);
GMMR0DECL(int)  GMMR0PageIdToVirt(PGVM pGVM, uint32_t idPage, void **ppv);
GMMR0DECL(int)  GMMR0RegisterSharedModule(PGVM pGVM, VMCPUID idCpu, VBOXOSFAMILY enmGuestOS, char *pszModuleName,
                                          char *pszVersion, RTGCPTR GCBaseAddr,  uint32_t cbModule, uint32_t cRegions,
                                          struct VMMDEVSHAREDREGIONDESC const *paRegions);
GMMR0DECL(int)  GMMR0UnregisterSharedModule(PGVM pGVM, VMCPUID idCpu, char *pszModuleName, char *pszVersion,
                                            RTGCPTR GCBaseAddr, uint32_t cbModule);
GMMR0DECL(int)  GMMR0UnregisterAllSharedModules(PGVM pGVM, VMCPUID idCpu);
GMMR0DECL(int)  GMMR0CheckSharedModules(PGVM pGVM, VMCPUID idCpu);
GMMR0DECL(int)  GMMR0ResetSharedModules(PGVM pGVM, VMCPUID idCpu);
GMMR0DECL(int)  GMMR0QueryStatistics(PGMMSTATS pStats, PSUPDRVSESSION pSession);
GMMR0DECL(int)  GMMR0ResetStatistics(PCGMMSTATS pStats, PSUPDRVSESSION pSession);

/**
 * Request buffer for GMMR0InitialReservationReq / VMMR0_DO_GMM_INITIAL_RESERVATION.
 * @see GMMR0InitialReservation
 */
typedef struct GMMINITIALRESERVATIONREQ
{
    /** The header. */
    SUPVMMR0REQHDR  Hdr;
    uint64_t        cBasePages;         /**< @see GMMR0InitialReservation */
    uint32_t        cShadowPages;       /**< @see GMMR0InitialReservation */
    uint32_t        cFixedPages;        /**< @see GMMR0InitialReservation */
    GMMOCPOLICY     enmPolicy;          /**< @see GMMR0InitialReservation */
    GMMPRIORITY     enmPriority;        /**< @see GMMR0InitialReservation */
} GMMINITIALRESERVATIONREQ;
/** Pointer to a GMMR0InitialReservationReq / VMMR0_DO_GMM_INITIAL_RESERVATION request buffer. */
typedef GMMINITIALRESERVATIONREQ *PGMMINITIALRESERVATIONREQ;

GMMR0DECL(int)  GMMR0InitialReservationReq(PGVM pGVM, VMCPUID idCpu, PGMMINITIALRESERVATIONREQ pReq);


/**
 * Request buffer for GMMR0UpdateReservationReq / VMMR0_DO_GMM_UPDATE_RESERVATION.
 * @see GMMR0UpdateReservation
 */
typedef struct GMMUPDATERESERVATIONREQ
{
    /** The header. */
    SUPVMMR0REQHDR  Hdr;
    uint64_t        cBasePages;         /**< @see GMMR0UpdateReservation */
    uint32_t        cShadowPages;       /**< @see GMMR0UpdateReservation */
    uint32_t        cFixedPages;        /**< @see GMMR0UpdateReservation */
} GMMUPDATERESERVATIONREQ;
/** Pointer to a GMMR0InitialReservationReq / VMMR0_DO_GMM_INITIAL_RESERVATION request buffer. */
typedef GMMUPDATERESERVATIONREQ *PGMMUPDATERESERVATIONREQ;

GMMR0DECL(int)  GMMR0UpdateReservationReq(PGVM pGVM, VMCPUID idCpu, PGMMUPDATERESERVATIONREQ pReq);


/**
 * Request buffer for GMMR0AllocatePagesReq / VMMR0_DO_GMM_ALLOCATE_PAGES.
 * @see GMMR0AllocatePages.
 */
typedef struct GMMALLOCATEPAGESREQ
{
    /** The header. */
    SUPVMMR0REQHDR  Hdr;
    /** The account to charge the allocation to. */
    GMMACCOUNT      enmAccount;
    /** The number of pages to allocate. */
    uint32_t        cPages;
    /** Array of page descriptors. */
    GMMPAGEDESC     aPages[1];
} GMMALLOCATEPAGESREQ;
/** Pointer to a GMMR0AllocatePagesReq / VMMR0_DO_GMM_ALLOCATE_PAGES request buffer. */
typedef GMMALLOCATEPAGESREQ *PGMMALLOCATEPAGESREQ;

GMMR0DECL(int)  GMMR0AllocatePagesReq(PGVM pGVM, VMCPUID idCpu, PGMMALLOCATEPAGESREQ pReq);


/**
 * Request buffer for GMMR0FreePagesReq / VMMR0_DO_GMM_FREE_PAGES.
 * @see GMMR0FreePages.
 */
typedef struct GMMFREEPAGESREQ
{
    /** The header. */
    SUPVMMR0REQHDR  Hdr;
    /** The account this relates to. */
    GMMACCOUNT      enmAccount;
    /** The number of pages to free. */
    uint32_t        cPages;
    /** Array of free page descriptors. */
    GMMFREEPAGEDESC aPages[1];
} GMMFREEPAGESREQ;
/** Pointer to a GMMR0FreePagesReq / VMMR0_DO_GMM_FREE_PAGES request buffer. */
typedef GMMFREEPAGESREQ *PGMMFREEPAGESREQ;

GMMR0DECL(int)  GMMR0FreePagesReq(PGVM pGVM, VMCPUID idCpu, PGMMFREEPAGESREQ pReq);

/**
 * Request buffer for GMMR0BalloonedPagesReq / VMMR0_DO_GMM_BALLOONED_PAGES.
 * @see GMMR0BalloonedPages.
 */
typedef struct GMMBALLOONEDPAGESREQ
{
    /** The header. */
    SUPVMMR0REQHDR      Hdr;
    /** The number of ballooned pages. */
    uint32_t            cBalloonedPages;
    /** Inflate or deflate the balloon. */
    GMMBALLOONACTION    enmAction;
} GMMBALLOONEDPAGESREQ;
/** Pointer to a GMMR0BalloonedPagesReq / VMMR0_DO_GMM_BALLOONED_PAGES request buffer. */
typedef GMMBALLOONEDPAGESREQ *PGMMBALLOONEDPAGESREQ;

GMMR0DECL(int)  GMMR0BalloonedPagesReq(PGVM pGVM, VMCPUID idCpu, PGMMBALLOONEDPAGESREQ pReq);


/**
 * Request buffer for GMMR0QueryHypervisorMemoryStatsReq / VMMR0_DO_GMM_QUERY_VMM_MEM_STATS.
 * @see GMMR0QueryHypervisorMemoryStatsReq.
 */
typedef struct GMMMEMSTATSREQ
{
    /** The header. */
    SUPVMMR0REQHDR      Hdr;
    /** The number of allocated pages (out). */
    uint64_t            cAllocPages;
    /** The number of free pages (out). */
    uint64_t            cFreePages;
    /** The number of ballooned pages (out). */
    uint64_t            cBalloonedPages;
    /** The number of shared pages (out). */
    uint64_t            cSharedPages;
    /** Maximum nr of pages (out). */
    uint64_t            cMaxPages;
} GMMMEMSTATSREQ;
/** Pointer to a GMMR0QueryHypervisorMemoryStatsReq / VMMR0_DO_GMM_QUERY_HYPERVISOR_MEM_STATS request buffer. */
typedef GMMMEMSTATSREQ *PGMMMEMSTATSREQ;

GMMR0DECL(int)  GMMR0QueryHypervisorMemoryStatsReq(PGMMMEMSTATSREQ pReq);
GMMR0DECL(int)  GMMR0QueryMemoryStatsReq(PGVM pGVM, VMCPUID idCpu, PGMMMEMSTATSREQ pReq);

/**
 * Request buffer for GMMR0MapUnmapChunkReq / VMMR0_DO_GMM_MAP_UNMAP_CHUNK.
 * @see GMMR0MapUnmapChunk
 */
typedef struct GMMMAPUNMAPCHUNKREQ
{
    /** The header. */
    SUPVMMR0REQHDR  Hdr;
    /** The chunk to map, NIL_GMM_CHUNKID if unmap only. (IN) */
    uint32_t        idChunkMap;
    /** The chunk to unmap, NIL_GMM_CHUNKID if map only. (IN) */
    uint32_t        idChunkUnmap;
    /** Where the mapping address is returned. (OUT) */
    RTR3PTR         pvR3;
} GMMMAPUNMAPCHUNKREQ;
/** Pointer to a GMMR0MapUnmapChunkReq / VMMR0_DO_GMM_MAP_UNMAP_CHUNK request buffer. */
typedef GMMMAPUNMAPCHUNKREQ *PGMMMAPUNMAPCHUNKREQ;

GMMR0DECL(int)  GMMR0MapUnmapChunkReq(PGVM pGVM, PGMMMAPUNMAPCHUNKREQ pReq);


/**
 * Request buffer for GMMR0FreeLargePageReq / VMMR0_DO_GMM_FREE_LARGE_PAGE.
 * @see GMMR0FreeLargePage.
 */
typedef struct GMMFREELARGEPAGEREQ
{
    /** The header. */
    SUPVMMR0REQHDR  Hdr;
    /** The Page ID. */
    uint32_t        idPage;
} GMMFREELARGEPAGEREQ;
/** Pointer to a GMMR0FreePagesReq / VMMR0_DO_GMM_FREE_PAGES request buffer. */
typedef GMMFREELARGEPAGEREQ *PGMMFREELARGEPAGEREQ;

GMMR0DECL(int) GMMR0FreeLargePageReq(PGVM pGVM, VMCPUID idCpu, PGMMFREELARGEPAGEREQ pReq);

/** Maximum length of the shared module name string, terminator included. */
#define GMM_SHARED_MODULE_MAX_NAME_STRING       128
/** Maximum length of the shared module version string, terminator included. */
#define GMM_SHARED_MODULE_MAX_VERSION_STRING    16

/**
 * Request buffer for GMMR0RegisterSharedModuleReq / VMMR0_DO_GMM_REGISTER_SHARED_MODULE.
 * @see GMMR0RegisterSharedModule.
 */
typedef struct GMMREGISTERSHAREDMODULEREQ
{
    /** The header. */
    SUPVMMR0REQHDR              Hdr;
    /** Shared module size. */
    uint32_t                    cbModule;
    /** Number of included region descriptors */
    uint32_t                    cRegions;
    /** Base address of the shared module. */
    RTGCPTR64                   GCBaseAddr;
    /** Guest OS type. */
    VBOXOSFAMILY                enmGuestOS;
    /** return code. */
    uint32_t                    rc;
    /** Module name */
    char                        szName[GMM_SHARED_MODULE_MAX_NAME_STRING];
    /** Module version */
    char                        szVersion[GMM_SHARED_MODULE_MAX_VERSION_STRING];
    /** Shared region descriptor(s). */
    VMMDEVSHAREDREGIONDESC      aRegions[1];
} GMMREGISTERSHAREDMODULEREQ;
/** Pointer to a GMMR0RegisterSharedModuleReq / VMMR0_DO_GMM_REGISTER_SHARED_MODULE request buffer. */
typedef GMMREGISTERSHAREDMODULEREQ *PGMMREGISTERSHAREDMODULEREQ;

GMMR0DECL(int) GMMR0RegisterSharedModuleReq(PGVM pGVM, VMCPUID idCpu, PGMMREGISTERSHAREDMODULEREQ pReq);

/**
 * Shared region descriptor
 */
typedef struct GMMSHAREDREGIONDESC
{
    /** The page offset where the region starts. */
    uint32_t                    off;
    /** Region size - adjusted by the region offset and rounded up to a
     * page. */
    uint32_t                    cb;
    /** Pointer to physical GMM page ID array. */
    uint32_t                   *paidPages;
} GMMSHAREDREGIONDESC;
/** Pointer to a GMMSHAREDREGIONDESC. */
typedef GMMSHAREDREGIONDESC *PGMMSHAREDREGIONDESC;


/**
 * Shared module registration info (global)
 */
typedef struct GMMSHAREDMODULE
{
    /** Tree node (keyed by a hash of name & version). */
    AVLLU32NODECORE             Core;
    /** Shared module size. */
    uint32_t                    cbModule;
    /** Number of included region descriptors */
    uint32_t                    cRegions;
    /** Number of users (VMs). */
    uint32_t                    cUsers;
    /** Guest OS family type. */
    VBOXOSFAMILY                enmGuestOS;
    /** Module name */
    char                        szName[GMM_SHARED_MODULE_MAX_NAME_STRING];
    /** Module version */
    char                        szVersion[GMM_SHARED_MODULE_MAX_VERSION_STRING];
    /** Shared region descriptor(s). */
    GMMSHAREDREGIONDESC         aRegions[1];
} GMMSHAREDMODULE;
/** Pointer to a GMMSHAREDMODULE. */
typedef GMMSHAREDMODULE *PGMMSHAREDMODULE;

/**
 * Page descriptor for GMMR0SharedModuleCheckRange
 */
typedef struct GMMSHAREDPAGEDESC
{
    /** HC Physical address (in/out) */
    RTHCPHYS                    HCPhys;
    /** GC Physical address (in) */
    RTGCPHYS                    GCPhys;
    /** GMM page id. (in/out) */
    uint32_t                    idPage;
    /** CRC32 of the page in strict builds (0 if page not available).
     * In non-strict build this serves as structure alignment. */
    uint32_t                    u32StrictChecksum;
} GMMSHAREDPAGEDESC;
/** Pointer to a GMMSHAREDPAGEDESC. */
typedef GMMSHAREDPAGEDESC *PGMMSHAREDPAGEDESC;

GMMR0DECL(int) GMMR0SharedModuleCheckPage(PGVM pGVM, PGMMSHAREDMODULE pModule, uint32_t idxRegion, uint32_t idxPage,
                                          PGMMSHAREDPAGEDESC pPageDesc);

/**
 * Request buffer for GMMR0UnregisterSharedModuleReq / VMMR0_DO_GMM_UNREGISTER_SHARED_MODULE.
 * @see GMMR0UnregisterSharedModule.
 */
typedef struct GMMUNREGISTERSHAREDMODULEREQ
{
    /** The header. */
    SUPVMMR0REQHDR              Hdr;
    /** Shared module size. */
    uint32_t                    cbModule;
    /** Align at 8 byte boundary. */
    uint32_t                    u32Alignment;
    /** Base address of the shared module. */
    RTGCPTR64                   GCBaseAddr;
    /** Module name */
    char                        szName[GMM_SHARED_MODULE_MAX_NAME_STRING];
    /** Module version */
    char                        szVersion[GMM_SHARED_MODULE_MAX_VERSION_STRING];
} GMMUNREGISTERSHAREDMODULEREQ;
/** Pointer to a GMMR0UnregisterSharedModuleReq / VMMR0_DO_GMM_UNREGISTER_SHARED_MODULE request buffer. */
typedef GMMUNREGISTERSHAREDMODULEREQ *PGMMUNREGISTERSHAREDMODULEREQ;

GMMR0DECL(int) GMMR0UnregisterSharedModuleReq(PGVM pGVM, VMCPUID idCpu, PGMMUNREGISTERSHAREDMODULEREQ pReq);

#if defined(VBOX_STRICT) && HC_ARCH_BITS == 64
/**
 * Request buffer for GMMR0FindDuplicatePageReq / VMMR0_DO_GMM_FIND_DUPLICATE_PAGE.
 * @see GMMR0FindDuplicatePage.
 */
typedef struct GMMFINDDUPLICATEPAGEREQ
{
    /** The header. */
    SUPVMMR0REQHDR              Hdr;
    /** Page id. */
    uint32_t                    idPage;
    /** Duplicate flag (out) */
    bool                        fDuplicate;
} GMMFINDDUPLICATEPAGEREQ;
/** Pointer to a GMMR0FindDuplicatePageReq / VMMR0_DO_GMM_FIND_DUPLICATE_PAGE request buffer. */
typedef GMMFINDDUPLICATEPAGEREQ *PGMMFINDDUPLICATEPAGEREQ;

GMMR0DECL(int) GMMR0FindDuplicatePageReq(PGVM pGVM, PGMMFINDDUPLICATEPAGEREQ pReq);
#endif /* VBOX_STRICT && HC_ARCH_BITS == 64 */


/**
 * Request buffer for GMMR0QueryStatisticsReq / VMMR0_DO_GMM_QUERY_STATISTICS.
 * @see GMMR0QueryStatistics.
 */
typedef struct GMMQUERYSTATISTICSSREQ
{
    /** The header. */
    SUPVMMR0REQHDR  Hdr;
    /** The support driver session. */
    PSUPDRVSESSION  pSession;
    /** The statistics. */
    GMMSTATS        Stats;
} GMMQUERYSTATISTICSSREQ;
/** Pointer to a GMMR0QueryStatisticsReq / VMMR0_DO_GMM_QUERY_STATISTICS
 *  request buffer. */
typedef GMMQUERYSTATISTICSSREQ *PGMMQUERYSTATISTICSSREQ;

GMMR0DECL(int)      GMMR0QueryStatisticsReq(PGVM pGVM, PGMMQUERYSTATISTICSSREQ pReq);


/**
 * Request buffer for GMMR0ResetStatisticsReq / VMMR0_DO_GMM_RESET_STATISTICS.
 * @see GMMR0ResetStatistics.
 */
typedef struct GMMRESETSTATISTICSSREQ
{
    /** The header. */
    SUPVMMR0REQHDR  Hdr;
    /** The support driver session. */
    PSUPDRVSESSION  pSession;
    /** The statistics to reset.
     * Any non-zero entry will be reset (if permitted). */
    GMMSTATS        Stats;
} GMMRESETSTATISTICSSREQ;
/** Pointer to a GMMR0ResetStatisticsReq / VMMR0_DO_GMM_RESET_STATISTICS
 *  request buffer. */
typedef GMMRESETSTATISTICSSREQ *PGMMRESETSTATISTICSSREQ;

GMMR0DECL(int)      GMMR0ResetStatisticsReq(PGVM pGVM, PGMMRESETSTATISTICSSREQ pReq);



#ifdef IN_RING3
/** @defgroup grp_gmm_r3    The Global Memory Manager Ring-3 API Wrappers
 * @{
 */
GMMR3DECL(int)  GMMR3InitialReservation(PVM pVM, uint64_t cBasePages, uint32_t cShadowPages, uint32_t cFixedPages,
                                        GMMOCPOLICY enmPolicy, GMMPRIORITY enmPriority);
GMMR3DECL(int)  GMMR3UpdateReservation(PVM pVM, uint64_t cBasePages, uint32_t cShadowPages, uint32_t cFixedPages);
GMMR3DECL(int)  GMMR3AllocatePagesPrepare(PVM pVM, PGMMALLOCATEPAGESREQ *ppReq, uint32_t cPages, GMMACCOUNT enmAccount);
GMMR3DECL(int)  GMMR3AllocatePagesPerform(PVM pVM, PGMMALLOCATEPAGESREQ pReq);
GMMR3DECL(void) GMMR3AllocatePagesCleanup(PGMMALLOCATEPAGESREQ pReq);
GMMR3DECL(int)  GMMR3FreePagesPrepare(PVM pVM, PGMMFREEPAGESREQ *ppReq, uint32_t cPages, GMMACCOUNT enmAccount);
GMMR3DECL(void) GMMR3FreePagesRePrep(PVM pVM, PGMMFREEPAGESREQ pReq, uint32_t cPages, GMMACCOUNT enmAccount);
GMMR3DECL(int)  GMMR3FreePagesPerform(PVM pVM, PGMMFREEPAGESREQ pReq, uint32_t cActualPages);
GMMR3DECL(void) GMMR3FreePagesCleanup(PGMMFREEPAGESREQ pReq);
GMMR3DECL(void) GMMR3FreeAllocatedPages(PVM pVM, GMMALLOCATEPAGESREQ const *pAllocReq);
GMMR3DECL(int)  GMMR3AllocateLargePage(PVM pVM,  uint32_t cbPage);
GMMR3DECL(int)  GMMR3FreeLargePage(PVM pVM,  uint32_t idPage);
GMMR3DECL(int)  GMMR3MapUnmapChunk(PVM pVM, uint32_t idChunkMap, uint32_t idChunkUnmap, PRTR3PTR ppvR3);
GMMR3DECL(int)  GMMR3QueryHypervisorMemoryStats(PVM pVM, uint64_t *pcTotalAllocPages, uint64_t *pcTotalFreePages, uint64_t *pcTotalBalloonPages, uint64_t *puTotalBalloonSize);
GMMR3DECL(int)  GMMR3QueryMemoryStats(PVM pVM, uint64_t *pcAllocPages, uint64_t *pcMaxPages, uint64_t *pcBalloonPages);
GMMR3DECL(int)  GMMR3BalloonedPages(PVM pVM, GMMBALLOONACTION enmAction, uint32_t cBalloonedPages);
GMMR3DECL(int)  GMMR3RegisterSharedModule(PVM pVM, PGMMREGISTERSHAREDMODULEREQ pReq);
GMMR3DECL(int)  GMMR3UnregisterSharedModule(PVM pVM, PGMMUNREGISTERSHAREDMODULEREQ pReq);
GMMR3DECL(int)  GMMR3CheckSharedModules(PVM pVM);
GMMR3DECL(int)  GMMR3ResetSharedModules(PVM pVM);

# if defined(VBOX_STRICT) && HC_ARCH_BITS == 64
GMMR3DECL(bool) GMMR3IsDuplicatePage(PVM pVM, uint32_t idPage);
# endif

/** @} */
#endif /* IN_RING3 */

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_gmm_h */

