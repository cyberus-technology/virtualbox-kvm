/** @file
 * IOM - Input / Output Monitor.
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

#ifndef VBOX_INCLUDED_vmm_iom_h
#define VBOX_INCLUDED_vmm_iom_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <VBox/dis.h>
#include <VBox/vmm/dbgf.h>

RT_C_DECLS_BEGIN


/** @defgroup grp_iom   The Input / Ouput Monitor API
 * @ingroup grp_vmm
 * @{
 */

/** @def IOM_NO_PDMINS_CHECKS
 * Until all devices have been fully adjusted to PDM style, the pPdmIns
 * parameter is not checked by IOM.
 * @todo Check this again, now.
 */
#define IOM_NO_PDMINS_CHECKS

/**
 * Macro for checking if an I/O or MMIO emulation call succeeded.
 *
 * This macro shall only be used with the IOM APIs where it's mentioned
 * in the return value description.  And there it must be used to correctly
 * determine if the call succeeded and things like the RIP needs updating.
 *
 *
 * @returns Success indicator (true/false).
 *
 * @param   rc          The status code.  This may be evaluated
 *                      more than once!
 *
 * @remarks To avoid making assumptions about the layout of the
 *          VINF_EM_FIRST...VINF_EM_LAST range we're checking explicitly for
 *          each exact exception. However, for efficiency we ASSUME that the
 *          VINF_EM_LAST is smaller than most of the relevant status codes. We
 *          also ASSUME that the VINF_EM_RESCHEDULE_REM status code is the
 *          most frequent status code we'll enounter in this range.
 *
 * @todo    Will have to add VINF_EM_DBG_HYPER_BREAKPOINT if the
 *          I/O port and MMIO breakpoints should trigger before
 *          the I/O is done.  Currently, we don't implement these
 *          kind of breakpoints.
 */
#ifdef IN_RING3
# define IOM_SUCCESS(rc)   (   (rc) == VINF_SUCCESS \
                             || (   (rc) <= VINF_EM_LAST \
                                 && (rc) != VINF_EM_RESCHEDULE_REM \
                                 && (rc) >= VINF_EM_FIRST \
                                 && (rc) != VINF_EM_RESCHEDULE_RAW \
                                 && (rc) != VINF_EM_RESCHEDULE_HM \
                                ) \
                            )
#else
# define IOM_SUCCESS(rc)   (   (rc) == VINF_SUCCESS \
                             || (   (rc) <= VINF_EM_LAST \
                                 && (rc) != VINF_EM_RESCHEDULE_REM \
                                 && (rc) >= VINF_EM_FIRST \
                                 && (rc) != VINF_EM_RESCHEDULE_RAW \
                                 && (rc) != VINF_EM_RESCHEDULE_HM \
                                ) \
                             || (rc) == VINF_IOM_R3_IOPORT_COMMIT_WRITE \
                             || (rc) == VINF_IOM_R3_MMIO_COMMIT_WRITE \
                            )
#endif

/** @name IOMMMIO_FLAGS_XXX
 * @{ */
/** Pass all reads thru unmodified. */
#define IOMMMIO_FLAGS_READ_PASSTHRU                     UINT32_C(0x00000000)
/** All read accesses are DWORD sized (32-bit). */
#define IOMMMIO_FLAGS_READ_DWORD                        UINT32_C(0x00000001)
/** All read accesses are DWORD (32-bit) or QWORD (64-bit) sized.
 * Only accesses that are both QWORD sized and aligned are performed as QWORD.
 * All other access will be done DWORD fashion (because it is way simpler). */
#define IOMMMIO_FLAGS_READ_DWORD_QWORD                  UINT32_C(0x00000002)
/** The read access mode mask. */
#define IOMMMIO_FLAGS_READ_MODE                         UINT32_C(0x00000003)

/** Pass all writes thru unmodified. */
#define IOMMMIO_FLAGS_WRITE_PASSTHRU                    UINT32_C(0x00000000)
/** All write accesses are DWORD (32-bit) sized and unspecified bytes are
 * written as zero. */
#define IOMMMIO_FLAGS_WRITE_DWORD_ZEROED                UINT32_C(0x00000010)
/** All write accesses are either DWORD (32-bit) or QWORD (64-bit) sized,
 * missing bytes will be written as zero.  Only accesses that are both QWORD
 * sized and aligned are performed as QWORD, all other accesses will be done
 * DWORD fashion (because it's way simpler). */
#define IOMMMIO_FLAGS_WRITE_DWORD_QWORD_ZEROED          UINT32_C(0x00000020)
/** All write accesses are DWORD (32-bit) sized and unspecified bytes are
 * read from the device first as DWORDs.
 * @remarks This isn't how it happens on real hardware, but it allows
 *          simplifications of devices where reads doesn't change the device
 *          state in any way. */
#define IOMMMIO_FLAGS_WRITE_DWORD_READ_MISSING          UINT32_C(0x00000030)
/** All write accesses are DWORD (32-bit) or QWORD (64-bit) sized and
 * unspecified bytes are read from the device first as DWORDs.  Only accesses
 * that are both QWORD sized and aligned are performed as QWORD, all other
 * accesses will be done DWORD fashion (because it's way simpler).
 * @remarks This isn't how it happens on real hardware, but it allows
 *          simplifications of devices where reads doesn't change the device
 *          state in any way. */
#define IOMMMIO_FLAGS_WRITE_DWORD_QWORD_READ_MISSING    UINT32_C(0x00000040)
/** All write accesses are DWORD (32-bit) sized and aligned, attempts at other
 * accesses are ignored.
 * @remarks E1000, APIC */
#define IOMMMIO_FLAGS_WRITE_ONLY_DWORD                  UINT32_C(0x00000050)
/** All write accesses are DWORD (32-bit) or QWORD (64-bit) sized and aligned,
 * attempts at other accesses are ignored.
 * @remarks Seemingly required by AHCI (although I doubt it's _really_
 *          required as EM/REM doesn't do the right thing in ring-3 anyway,
 *          esp. not in raw-mode). */
#define IOMMMIO_FLAGS_WRITE_ONLY_DWORD_QWORD            UINT32_C(0x00000060)
/** The read access mode mask. */
#define IOMMMIO_FLAGS_WRITE_MODE                        UINT32_C(0x00000070)

/** Whether to do a DBGSTOP on complicated reads.
 * What this includes depends on the read mode, but generally all misaligned
 * reads as well as word and byte reads and maybe qword reads. */
#define IOMMMIO_FLAGS_DBGSTOP_ON_COMPLICATED_READ       UINT32_C(0x00000100)
/** Whether to do a DBGSTOP on complicated writes.
 * This depends on the write mode, but generally all writes where we have to
 * supply bytes (zero them or read them). */
#define IOMMMIO_FLAGS_DBGSTOP_ON_COMPLICATED_WRITE      UINT32_C(0x00000200)

/** Pass the absolute physical address (GC) to the callback rather than the
 * relative one.
 * @note New-style only, is implicit in old-style interface.  */
#define IOMMMIO_FLAGS_ABS                               UINT32_C(0x00001000)

/** Mask of valid flags. */
#define IOMMMIO_FLAGS_VALID_MASK                        UINT32_C(0x00001373)
/** @} */

/**
 * Checks whether the write mode allows aligned QWORD accesses to be passed
 * thru to the device handler.
 * @param   a_fFlags        The MMIO handler flags.
 */
#define IOMMMIO_DOES_WRITE_MODE_ALLOW_QWORD(a_fFlags) \
    (   ((a_fFlags) & IOMMMIO_FLAGS_WRITE_MODE) == IOMMMIO_FLAGS_WRITE_DWORD_QWORD_ZEROED \
     || ((a_fFlags) & IOMMMIO_FLAGS_WRITE_MODE) == IOMMMIO_FLAGS_WRITE_DWORD_QWORD_READ_MISSING \
     || ((a_fFlags) & IOMMMIO_FLAGS_WRITE_MODE) == IOMMMIO_FLAGS_WRITE_ONLY_DWORD_QWORD )


/**
 * Port I/O Handler for IN operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 * @returns VERR_IOM_IOPORT_UNUSED if the port is really unused and a ~0 value should be returned.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the IN operation.
 * @param   pu32        Where to store the result.  This is always a 32-bit
 *                      variable regardless of what @a cb might say.
 * @param   cb          Number of bytes read.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(int, FNIOMIOPORTIN,(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t *pu32, unsigned cb));
/** Pointer to a FNIOMIOPORTIN(). */
typedef FNIOMIOPORTIN *PFNIOMIOPORTIN;

/**
 * Port I/O Handler for string IN operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 * @returns VERR_IOM_IOPORT_UNUSED if the port is really unused and a ~0 value should be returned.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the IN operation.
 * @param   pbDst       Pointer to the destination buffer.
 * @param   pcTransfers Pointer to the number of transfer units to read, on
 *                      return remaining transfer units.
 * @param   cb          Size of the transfer unit (1, 2 or 4 bytes).
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(int, FNIOMIOPORTINSTRING,(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint8_t *pbDst,
                                                   uint32_t *pcTransfers, unsigned cb));
/** Pointer to a FNIOMIOPORTINSTRING(). */
typedef FNIOMIOPORTINSTRING *PFNIOMIOPORTINSTRING;

/**
 * Port I/O Handler for OUT operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the OUT operation.
 * @param   u32         The value to output.
 * @param   cb          The value size in bytes.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(int, FNIOMIOPORTOUT,(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, uint32_t u32, unsigned cb));
/** Pointer to a FNIOMIOPORTOUT(). */
typedef FNIOMIOPORTOUT *PFNIOMIOPORTOUT;

/**
 * Port I/O Handler for string OUT operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   uPort       Port number used for the OUT operation.
 * @param   pbSrc       Pointer to the source buffer.
 * @param   pcTransfers Pointer to the number of transfer units to write, on
 *                      return remaining transfer units.
 * @param   cb          Size of the transfer unit (1, 2 or 4 bytes).
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(int, FNIOMIOPORTOUTSTRING,(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT uPort, const uint8_t *pbSrc,
                                                    uint32_t *pcTransfers, unsigned cb));
/** Pointer to a FNIOMIOPORTOUTSTRING(). */
typedef FNIOMIOPORTOUTSTRING *PFNIOMIOPORTOUTSTRING;


/**
 * Port I/O Handler for IN operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 * @returns VERR_IOM_IOPORT_UNUSED if the port is really unused and a ~0 value should be returned.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   offPort     The port number if IOM_IOPORT_F_ABS is used, otherwise
 *                      relative to the mapping base.
 * @param   pu32        Where to store the result.  This is always a 32-bit
 *                      variable regardless of what @a cb might say.
 * @param   cb          Number of bytes read.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(VBOXSTRICTRC, FNIOMIOPORTNEWIN,(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort,
                                                         uint32_t *pu32, unsigned cb));
/** Pointer to a FNIOMIOPORTNEWIN(). */
typedef FNIOMIOPORTNEWIN *PFNIOMIOPORTNEWIN;

/**
 * Port I/O Handler for string IN operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 * @returns VERR_IOM_IOPORT_UNUSED if the port is really unused and a ~0 value should be returned.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   offPort     The port number if IOM_IOPORT_F_ABS is used, otherwise
 *                      relative to the mapping base.
 * @param   pbDst       Pointer to the destination buffer.
 * @param   pcTransfers Pointer to the number of transfer units to read, on
 *                      return remaining transfer units.
 * @param   cb          Size of the transfer unit (1, 2 or 4 bytes).
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(VBOXSTRICTRC, FNIOMIOPORTNEWINSTRING,(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort, uint8_t *pbDst,
                                                               uint32_t *pcTransfers, unsigned cb));
/** Pointer to a FNIOMIOPORTNEWINSTRING(). */
typedef FNIOMIOPORTNEWINSTRING *PFNIOMIOPORTNEWINSTRING;

/**
 * Port I/O Handler for OUT operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   offPort     The port number if IOM_IOPORT_F_ABS is used, otherwise
 *                      relative to the mapping base.
 * @param   u32         The value to output.
 * @param   cb          The value size in bytes.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(VBOXSTRICTRC, FNIOMIOPORTNEWOUT,(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort,
                                                          uint32_t u32, unsigned cb));
/** Pointer to a FNIOMIOPORTNEWOUT(). */
typedef FNIOMIOPORTNEWOUT *PFNIOMIOPORTNEWOUT;

/**
 * Port I/O Handler for string OUT operations.
 *
 * @returns VINF_SUCCESS or VINF_EM_*.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   offPort     The port number if IOM_IOPORT_F_ABS is used, otherwise
 *                      relative to the mapping base.
 * @param   pbSrc       Pointer to the source buffer.
 * @param   pcTransfers Pointer to the number of transfer units to write, on
 *                      return remaining transfer units.
 * @param   cb          Size of the transfer unit (1, 2 or 4 bytes).
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(VBOXSTRICTRC, FNIOMIOPORTNEWOUTSTRING,(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort,
                                                                const uint8_t *pbSrc, uint32_t *pcTransfers, unsigned cb));
/** Pointer to a FNIOMIOPORTNEWOUTSTRING(). */
typedef FNIOMIOPORTNEWOUTSTRING *PFNIOMIOPORTNEWOUTSTRING;

/**
 * I/O port description.
 *
 * If both pszIn and pszOut are NULL, the entry is considered a terminator.
 */
typedef struct IOMIOPORTDESC
{
    /** Brief description / name of the IN port. */
    const char *pszIn;
    /** Brief description / name of the OUT port. */
    const char *pszOut;
    /** Detailed description of the IN port, optional. */
    const char *pszInDetail;
    /** Detialed description of the OUT port, optional. */
    const char *pszOutDetail;
} IOMIOPORTDESC;
/** Pointer to an I/O port description. */
typedef IOMIOPORTDESC const *PCIOMIOPORTDESC;


/**
 * Memory mapped I/O Handler for read operations.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   GCPhysAddr  Physical address (in GC) where the read starts.
 * @param   pv          Where to store the result.
 * @param   cb          Number of bytes read.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(int, FNIOMMMIOREAD,(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb));
/** Pointer to a FNIOMMMIOREAD(). */
typedef FNIOMMMIOREAD *PFNIOMMMIOREAD;

/**
 * Memory mapped I/O Handler for write operations.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   GCPhysAddr  Physical address (in GC) where the read starts.
 * @param   pv          Where to fetch the result.
 * @param   cb          Number of bytes to write.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(int, FNIOMMMIOWRITE,(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void const *pv, unsigned cb));
/** Pointer to a FNIOMMMIOWRITE(). */
typedef FNIOMMMIOWRITE *PFNIOMMMIOWRITE;

/**
 * Memory mapped I/O Handler for memset operations, actually for REP STOS* instructions handling.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   GCPhysAddr  Physical address (in GC) where the write starts.
 * @param   u32Item     Byte/Word/Dword data to fill.
 * @param   cbItem      Size of data in u32Item parameter, restricted to 1/2/4 bytes.
 * @param   cItems      Number of iterations.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(int, FNIOMMMIOFILL,(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr,
                                             uint32_t u32Item, unsigned cbItem, unsigned cItems));
/** Pointer to a FNIOMMMIOFILL(). */
typedef FNIOMMMIOFILL *PFNIOMMMIOFILL;


/**
 * Memory mapped I/O Handler for read operations.
 *
 * @returns Strict VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   off         Offset into the mapping of the read,
 *                      or the physical address if IOMMMIO_FLAGS_ABS is active.
 * @param   pv          Where to store the result.
 * @param   cb          Number of bytes read.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(VBOXSTRICTRC, FNIOMMMIONEWREAD,(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS off, void *pv, uint32_t cb));
/** Pointer to a FNIOMMMIONEWREAD(). */
typedef FNIOMMMIONEWREAD *PFNIOMMMIONEWREAD;

/**
 * Memory mapped I/O Handler for write operations.
 *
 * @returns Strict VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   off         Offset into the mapping of the write,
 *                      or the physical address if IOMMMIO_FLAGS_ABS is active.
 * @param   pv          Where to fetch the result.
 * @param   cb          Number of bytes to write.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(VBOXSTRICTRC, FNIOMMMIONEWWRITE,(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS off,
                                                          void const *pv, uint32_t cb));
/** Pointer to a FNIOMMMIONEWWRITE(). */
typedef FNIOMMMIONEWWRITE *PFNIOMMMIONEWWRITE;

/**
 * Memory mapped I/O Handler for memset operations, actually for REP STOS* instructions handling.
 *
 * @returns Strict VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument.
 * @param   off         Offset into the mapping of the fill,
 *                      or the physical address if IOMMMIO_FLAGS_ABS is active.
 * @param   u32Item     Byte/Word/Dword data to fill.
 * @param   cbItem      Size of data in u32Item parameter, restricted to 1/2/4 bytes.
 * @param   cItems      Number of iterations.
 * @remarks Caller enters the device critical section.
 */
typedef DECLCALLBACKTYPE(VBOXSTRICTRC, FNIOMMMIONEWFILL,(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS off,
                                                         uint32_t u32Item, uint32_t cbItem, uint32_t cItems));
/** Pointer to a FNIOMMMIONEWFILL(). */
typedef FNIOMMMIONEWFILL *PFNIOMMMIONEWFILL;

VMMDECL(VBOXSTRICTRC)   IOMIOPortRead(PVMCC pVM, PVMCPU pVCpu, RTIOPORT Port, uint32_t *pu32Value, size_t cbValue);
VMMDECL(VBOXSTRICTRC)   IOMIOPortWrite(PVMCC pVM, PVMCPU pVCpu, RTIOPORT Port, uint32_t u32Value, size_t cbValue);
VMM_INT_DECL(VBOXSTRICTRC) IOMIOPortReadString(PVMCC pVM, PVMCPU pVCpu, RTIOPORT Port, void *pvDst,
                                               uint32_t *pcTransfers, unsigned cb);
VMM_INT_DECL(VBOXSTRICTRC) IOMIOPortWriteString(PVMCC pVM, PVMCPU pVCpu, RTIOPORT uPort, void const *pvSrc,
                                                uint32_t *pcTransfers, unsigned cb);
VMM_INT_DECL(VBOXSTRICTRC) IOMR0MmioPhysHandler(PVMCC pVM, PVMCPUCC pVCpu, uint32_t uErrorCode, RTGCPHYS GCPhysFault);
VMMDECL(int)            IOMMmioMapMmio2Page(PVMCC pVM, PPDMDEVINS pDevIns, IOMMMIOHANDLE hRegion, RTGCPHYS offRegion,
                                            uint64_t hMmio2, RTGCPHYS offMmio2, uint64_t fPageFlags);
VMMR0_INT_DECL(int)     IOMR0MmioMapMmioHCPage(PVMCC pVM, PVMCPUCC pVCpu, RTGCPHYS GCPhys, RTHCPHYS HCPhys, uint64_t fPageFlags);
VMMDECL(int)            IOMMmioResetRegion(PVMCC pVM, PPDMDEVINS pDevIns, IOMMMIOHANDLE hRegion);


/** @name IOM_IOPORT_F_XXX - Flags for IOMR3IoPortCreate() and PDMDevHlpIoPortCreateEx().
 * @{ */
/** Pass the absolute I/O port to the callback rather than the relative one.  */
#define IOM_IOPORT_F_ABS            RT_BIT_32(0)
/** Valid flags for IOMR3IoPortCreate(). */
#define IOM_IOPORT_F_VALID_MASK     UINT32_C(0x00000001)
/** @} */

#ifdef IN_RING3
/** @defgroup grp_iom_r3    The IOM Host Context Ring-3 API
 * @{
 */
VMMR3_INT_DECL(int)  IOMR3Init(PVM pVM);
VMMR3_INT_DECL(int)  IOMR3InitCompleted(PVM pVM, VMINITCOMPLETED enmWhat);
VMMR3_INT_DECL(void) IOMR3Reset(PVM pVM);
VMMR3_INT_DECL(void) IOMR3Relocate(PVM pVM, RTGCINTPTR offDelta);
VMMR3_INT_DECL(int)  IOMR3Term(PVM pVM);

VMMR3_INT_DECL(int)  IOMR3IoPortCreate(PVM pVM, PPDMDEVINS pDevIns, RTIOPORT cPorts, uint32_t fFlags, PPDMPCIDEV pPciDev,
                                       uint32_t iPciRegion, PFNIOMIOPORTNEWOUT pfnOut, PFNIOMIOPORTNEWIN pfnIn,
                                       PFNIOMIOPORTNEWOUTSTRING pfnOutStr, PFNIOMIOPORTNEWINSTRING pfnInStr, RTR3PTR pvUser,
                                       const char *pszDesc, PCIOMIOPORTDESC paExtDescs, PIOMIOPORTHANDLE phIoPorts);
VMMR3_INT_DECL(int)  IOMR3IoPortMap(PVM pVM, PPDMDEVINS pDevIns, IOMIOPORTHANDLE hIoPorts, RTIOPORT Port);
VMMR3_INT_DECL(int)  IOMR3IoPortUnmap(PVM pVM, PPDMDEVINS pDevIns, IOMIOPORTHANDLE hIoPorts);
VMMR3_INT_DECL(int)  IOMR3IoPortValidateHandle(PVM pVM, PPDMDEVINS pDevIns, IOMIOPORTHANDLE hIoPorts);
VMMR3_INT_DECL(uint32_t) IOMR3IoPortGetMappingAddress(PVM pVM, PPDMDEVINS pDevIns, IOMIOPORTHANDLE hIoPorts);

VMMR3_INT_DECL(int)  IOMR3MmioCreate(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS cbRegion, uint32_t fFlags, PPDMPCIDEV pPciDev,
                                     uint32_t iPciRegion, PFNIOMMMIONEWWRITE pfnWrite, PFNIOMMMIONEWREAD pfnRead,
                                     PFNIOMMMIONEWFILL pfnFill, void *pvUser, const char *pszDesc, PIOMMMIOHANDLE phRegion);
VMMR3_INT_DECL(int)  IOMR3MmioMap(PVM pVM, PPDMDEVINS pDevIns, IOMMMIOHANDLE hRegion, RTGCPHYS GCPhys);
VMMR3_INT_DECL(int)  IOMR3MmioUnmap(PVM pVM, PPDMDEVINS pDevIns, IOMMMIOHANDLE hRegion);
VMMR3_INT_DECL(int)  IOMR3MmioReduce(PVM pVM, PPDMDEVINS pDevIns, IOMMMIOHANDLE hRegion, RTGCPHYS cbRegion);
VMMR3_INT_DECL(int)  IOMR3MmioValidateHandle(PVM pVM, PPDMDEVINS pDevIns, IOMMMIOHANDLE hRegion);
VMMR3_INT_DECL(RTGCPHYS) IOMR3MmioGetMappingAddress(PVM pVM, PPDMDEVINS pDevIns, IOMMMIOHANDLE hRegion);

VMMR3_INT_DECL(VBOXSTRICTRC) IOMR3ProcessForceFlag(PVM pVM, PVMCPU pVCpu, VBOXSTRICTRC rcStrict);

VMMR3_INT_DECL(void) IOMR3NotifyBreakpointCountChange(PVM pVM, bool fPortIo, bool fMmio);
VMMR3_INT_DECL(void) IOMR3NotifyDebugEventChange(PVM pVM, DBGFEVENT enmEvent, bool fEnabled);
/** @} */
#endif /* IN_RING3 */


#if defined(IN_RING0) || defined(DOXYGEN_RUNNING)
/** @defgroup grpm_iom_r0   The IOM Host Context Ring-0 API
 * @{ */
VMMR0_INT_DECL(void) IOMR0InitPerVMData(PGVM pGVM);
VMMR0_INT_DECL(int)  IOMR0InitVM(PGVM pGVM);
VMMR0_INT_DECL(void) IOMR0CleanupVM(PGVM pGVM);

VMMR0_INT_DECL(int)  IOMR0IoPortSetUpContext(PGVM pGVM, PPDMDEVINS pDevIns, IOMIOPORTHANDLE hIoPorts,
                                             PFNIOMIOPORTNEWOUT pfnOut,  PFNIOMIOPORTNEWIN pfnIn,
                                             PFNIOMIOPORTNEWOUTSTRING pfnOutStr, PFNIOMIOPORTNEWINSTRING pfnInStr, void *pvUser);
VMMR0_INT_DECL(int)  IOMR0IoPortGrowRegistrationTables(PGVM pGVM, uint64_t cMinEntries);
VMMR0_INT_DECL(int)  IOMR0IoPortGrowStatisticsTable(PGVM pGVM, uint64_t cMinEntries);
VMMR0_INT_DECL(int)  IOMR0IoPortSyncStatisticsIndices(PGVM pGVM);

VMMR0_INT_DECL(int)  IOMR0MmioSetUpContext(PGVM pGVM, PPDMDEVINS pDevIns, IOMMMIOHANDLE hRegion, PFNIOMMMIONEWWRITE pfnWrite,
                                           PFNIOMMMIONEWREAD pfnRead, PFNIOMMMIONEWFILL pfnFill, void *pvUser);
VMMR0_INT_DECL(int)  IOMR0MmioGrowRegistrationTables(PGVM pGVM, uint64_t cMinEntries);
VMMR0_INT_DECL(int)  IOMR0MmioGrowStatisticsTable(PGVM pGVM, uint64_t cMinEntries);
VMMR0_INT_DECL(int)  IOMR0MmioSyncStatisticsIndices(PGVM pGVM);

/** @} */
#endif /* IN_RING0 || DOXYGEN_RUNNING */

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_iom_h */

