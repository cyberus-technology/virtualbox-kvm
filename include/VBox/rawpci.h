/** @file
 * Raw PCI Devices (aka PCI pass-through). (VMM)
 */

/*
 * Copyright (C) 2010-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_rawpci_h
#define VBOX_INCLUDED_rawpci_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>
#include <VBox/sup.h>

RT_C_DECLS_BEGIN

/**
 * Handle for the raw PCI device.
 */
typedef uint32_t PCIRAWDEVHANDLE;

/**
 * Handle for the ISR.
 */
typedef uint32_t PCIRAWISRHANDLE;

/**
 * Physical memory action enumeration.
 */
typedef enum PCIRAWMEMINFOACTION
{
    /** Pages mapped. */
    PCIRAW_MEMINFO_MAP,
    /** Pages unmapped. */
    PCIRAW_MEMINFO_UNMAP,
    /** The usual 32-bit type blow up. */
    PCIRAW_MEMINFO_32BIT_HACK = 0x7fffffff
} PCIRAWMEMINFOACTION;

/**
 * Per-VM capability flag bits.
 */
typedef enum PCIRAWVMFLAGS
{
    /** If we can use IOMMU in this VM. */
    PCIRAW_VMFLAGS_HAS_IOMMU = (1 << 0),
    PCIRAW_VMFLAGS_32BIT_HACK = 0x7fffffff
} PCIRAWVMFLAGS;

/* Forward declaration. */
struct RAWPCIPERVM;

/**
 * Callback to notify raw PCI subsystem about mapping/unmapping of
 * host pages to the guest. Typical usecase is to register physical
 * RAM pages with IOMMU, so that it could allow DMA for PCI devices
 * directly from the guest RAM.
 * Region shall be one or more contigous (both host and guest) pages
 * of physical memory.
 *
 * @returns VBox status code.
 *
 * @param   pVmData         The per VM data.
 * @param   HCPhysStart     Physical address of region start on the host.
 * @param   GCPhysStart     Physical address of region start on the guest.
 * @param   cbMem           Region size in bytes.
 * @param   enmAction       Action performed (i.e. if page was mapped
 *                          or unmapped).
 */
typedef DECLCALLBACKTYPE(int, FNRAWPCICONTIGPHYSMEMINFO,(struct RAWPCIPERVM *pVmData, RTHCPHYS HCPhysStart,
                                                         RTGCPHYS GCPhysStart, uint64_t cbMem, PCIRAWMEMINFOACTION enmAction));
typedef FNRAWPCICONTIGPHYSMEMINFO *PFNRAWPCICONTIGPHYSMEMINFO;

/** Data being part of the VM structure. */
typedef struct RAWPCIPERVM
{
    /** Shall only be interpreted by the host PCI driver. */
    RTR0PTR                     pDriverData;
    /** Callback called when mapping of host pages to the guest changes. */
    PFNRAWPCICONTIGPHYSMEMINFO  pfnContigMemInfo;
    /** Flags describing VM capabilities (such as IOMMU presence). */
    uint32_t                    fVmCaps;
} RAWPCIPERVM;
typedef RAWPCIPERVM *PRAWPCIPERVM;

/** Parameters buffer for PCIRAWR0_DO_OPEN_DEVICE call */
typedef struct
{
    /* in */
    uint32_t        PciAddress;
    uint32_t        fFlags;
    /* out */
    PCIRAWDEVHANDLE Device;
    uint32_t        fDevFlags;
} PCIRAWREQOPENDEVICE;

/** Parameters buffer for PCIRAWR0_DO_CLOSE_DEVICE call */
typedef struct
{
    /* in */
    uint32_t fFlags;
} PCIRAWREQCLOSEDEVICE;

/** Parameters buffer for PCIRAWR0_DO_GET_REGION_INFO call */
typedef struct
{
    /* in */
    int32_t  iRegion;
    /* out */
    RTGCPHYS RegionStart;
    uint64_t u64RegionSize;
    bool     fPresent;
    uint32_t fFlags;
} PCIRAWREQGETREGIONINFO;

/** Parameters buffer for PCIRAWR0_DO_MAP_REGION call. */
typedef struct
{
    /* in */
    RTGCPHYS             StartAddress;
    uint64_t             iRegionSize;
    int32_t              iRegion;
    uint32_t             fFlags;
    /* out */
    RTR3PTR              pvAddressR3;
    RTR0PTR              pvAddressR0;
} PCIRAWREQMAPREGION;

/** Parameters buffer for PCIRAWR0_DO_UNMAP_REGION call. */
typedef struct
{
    /* in */
    RTGCPHYS             StartAddress;
    uint64_t             iRegionSize;
    RTR3PTR              pvAddressR3;
    RTR0PTR              pvAddressR0;
    int32_t              iRegion;
} PCIRAWREQUNMAPREGION;

/** Parameters buffer for PCIRAWR0_DO_PIO_WRITE call. */
typedef struct
{
    /* in */
    uint16_t             iPort;
    uint16_t             cb;
    uint32_t             iValue;
} PCIRAWREQPIOWRITE;

/** Parameters buffer for PCIRAWR0_DO_PIO_READ call. */
typedef struct
{
    /* in */
    uint16_t             iPort;
    uint16_t             cb;
    /* out */
    uint32_t             iValue;
} PCIRAWREQPIOREAD;

/** Memory operand. */
typedef struct
{
    union
    {
        uint8_t          u8;
        uint16_t         u16;
        uint32_t         u32;
        uint64_t         u64;
    } u;
    uint8_t cb;
} PCIRAWMEMLOC;

/** Parameters buffer for PCIRAWR0_DO_MMIO_WRITE call. */
typedef struct
{
    /* in */
    RTR0PTR              Address;
    PCIRAWMEMLOC         Value;
} PCIRAWREQMMIOWRITE;

/** Parameters buffer for PCIRAWR0_DO_MMIO_READ call. */
typedef struct
{
    /* in */
    RTR0PTR              Address;
    /* inout (Value.cb is in) */
    PCIRAWMEMLOC         Value;
} PCIRAWREQMMIOREAD;

/* Parameters buffer for PCIRAWR0_DO_PCICFG_WRITE call. */
typedef struct
{
    /* in */
    uint32_t             iOffset;
    PCIRAWMEMLOC         Value;
} PCIRAWREQPCICFGWRITE;

/** Parameters buffer for PCIRAWR0_DO_PCICFG_READ call. */
typedef struct
{
    /* in */
    uint32_t             iOffset;
    /* inout (Value.cb is in) */
    PCIRAWMEMLOC         Value;
} PCIRAWREQPCICFGREAD;

/** Parameters buffer for PCIRAWR0_DO_GET_IRQ call. */
typedef struct PCIRAWREQGETIRQ
{
    /* in */
    int64_t              iTimeout;
    /* out */
    int32_t              iIrq;
} PCIRAWREQGETIRQ;

/** Parameters buffer for PCIRAWR0_DO_POWER_STATE_CHANGE call. */
typedef struct PCIRAWREQPOWERSTATECHANGE
{
    /* in */
    uint32_t             iState;
    /* in/out */
    uint64_t             u64Param;
} PCIRAWREQPOWERSTATECHANGE;

/**
 * Request buffer use for communication with the driver.
 */
typedef struct PCIRAWSENDREQ
{
    /** The request header. */
    SUPVMMR0REQHDR  Hdr;
    /** Alternative to passing the taking the session from the VM handle.
     *  Either use this member or use the VM handle, don't do both.
     */
    PSUPDRVSESSION  pSession;
    /** Request type. */
    int32_t         iRequest;
    /** Host device request targetted to. */
    PCIRAWDEVHANDLE TargetDevice;
    /** Call parameters. */
    union
    {
        PCIRAWREQOPENDEVICE       aOpenDevice;
        PCIRAWREQCLOSEDEVICE      aCloseDevice;
        PCIRAWREQGETREGIONINFO    aGetRegionInfo;
        PCIRAWREQMAPREGION        aMapRegion;
        PCIRAWREQUNMAPREGION      aUnmapRegion;
        PCIRAWREQPIOWRITE         aPioWrite;
        PCIRAWREQPIOREAD          aPioRead;
        PCIRAWREQMMIOWRITE        aMmioWrite;
        PCIRAWREQMMIOREAD         aMmioRead;
        PCIRAWREQPCICFGWRITE      aPciCfgWrite;
        PCIRAWREQPCICFGREAD       aPciCfgRead;
        PCIRAWREQGETIRQ           aGetIrq;
        PCIRAWREQPOWERSTATECHANGE aPowerStateChange;
    } u;
} PCIRAWSENDREQ;
typedef PCIRAWSENDREQ *PPCIRAWSENDREQ;

/**
 * Operations performed by the driver.
 */
typedef enum PCIRAWR0OPERATION
{
    /* Open device. */
    PCIRAWR0_DO_OPEN_DEVICE,
    /* Close device. */
    PCIRAWR0_DO_CLOSE_DEVICE,
    /* Get PCI region info. */
    PCIRAWR0_DO_GET_REGION_INFO,
    /* Map PCI region into VM address space. */
    PCIRAWR0_DO_MAP_REGION,
    /* Unmap PCI region from VM address space. */
    PCIRAWR0_DO_UNMAP_REGION,
    /* Perform PIO write. */
    PCIRAWR0_DO_PIO_WRITE,
    /* Perform PIO read. */
    PCIRAWR0_DO_PIO_READ,
    /* Perform MMIO write. */
    PCIRAWR0_DO_MMIO_WRITE,
    /* Perform MMIO read. */
    PCIRAWR0_DO_MMIO_READ,
    /* Perform PCI config write. */
    PCIRAWR0_DO_PCICFG_WRITE,
    /* Perform PCI config read. */
    PCIRAWR0_DO_PCICFG_READ,
    /* Get next IRQ for the device. */
    PCIRAWR0_DO_GET_IRQ,
    /* Enable getting IRQs for the device. */
    PCIRAWR0_DO_ENABLE_IRQ,
    /* Disable getting IRQs for the device. */
    PCIRAWR0_DO_DISABLE_IRQ,
    /* Notify driver about guest power state change. */
    PCIRAWR0_DO_POWER_STATE_CHANGE,
    /** The usual 32-bit type blow up. */
    PCIRAWR0_DO_32BIT_HACK = 0x7fffffff
} PCIRAWR0OPERATION;

/**
 * Power state enumeration.
 */
typedef enum PCIRAWPOWERSTATE
{
    /* Power on. */
    PCIRAW_POWER_ON,
    /* Power off. */
    PCIRAW_POWER_OFF,
    /* Suspend. */
    PCIRAW_POWER_SUSPEND,
    /* Resume. */
    PCIRAW_POWER_RESUME,
     /* Reset. */
    PCIRAW_POWER_RESET,
    /** The usual 32-bit type blow up. */
    PCIRAW_POWER_32BIT_HACK = 0x7fffffff
} PCIRAWPOWERSTATE;


/** Forward declarations. */
typedef struct RAWPCIFACTORY *PRAWPCIFACTORY;
typedef struct RAWPCIDEVPORT *PRAWPCIDEVPORT;

/**
 * Interrupt service routine callback.
 *
 * @returns if interrupt was processed.
 *
 * @param   pvContext       Opaque user data passed to the handler.
 * @param   iIrq            Interrupt number.
 */
typedef DECLCALLBACKTYPE(bool, FNRAWPCIISR,(void *pvContext, int32_t iIrq));
typedef FNRAWPCIISR *PFNRAWPCIISR;

/**
 * This is the port on the device interface, i.e. the driver side which the
 * host device is connected to.
 *
 * This is only used for the in-kernel PCI device connections.
 */
typedef struct RAWPCIDEVPORT
{
    /** Structure version number. (RAWPCIDEVPORT_VERSION) */
    uint32_t u32Version;

    /**
     * Init device.
     *
     * @param   pPort     Pointer to this structure.
     * @param   fFlags    Initialization flags.
     */
    DECLR0CALLBACKMEMBER(int,  pfnInit,(PRAWPCIDEVPORT pPort,
                                        uint32_t       fFlags));


    /**
     * Deinit device.
     *
     * @param   pPort     Pointer to this structure.
     * @param   fFlags    Initialization flags.
     */
    DECLR0CALLBACKMEMBER(int,  pfnDeinit,(PRAWPCIDEVPORT pPort,
                                          uint32_t       fFlags));


    /**
     * Destroy device.
     *
     * @param   pPort     Pointer to this structure.
     */
    DECLR0CALLBACKMEMBER(int,  pfnDestroy,(PRAWPCIDEVPORT pPort));

    /**
     * Get PCI region info.
     *
     * @param   pPort     Pointer to this structure.
     * @param   iRegion   Region number.
     * @param   pRegionStart    Where to start the region address.
     * @param   pu64RegionSize  Where to store the region size.
     * @param   pfPresent   Where to store if the region is present.
     * @param   pfFlags     Where to store the flags.
     */
    DECLR0CALLBACKMEMBER(int,  pfnGetRegionInfo,(PRAWPCIDEVPORT pPort,
                                                 int32_t        iRegion,
                                                 RTHCPHYS       *pRegionStart,
                                                 uint64_t       *pu64RegionSize,
                                                 bool           *pfPresent,
                                                 uint32_t       *pfFlags));


    /**
     * Map PCI region.
     *
     * @param   pPort     Pointer to this structure.
     * @param   iRegion   Region number.
     * @param   RegionStart     Region start.
     * @param   u64RegionSize   Region size.
     * @param   fFlags    Flags.
     * @param   pRegionBaseR0   Where to store the R0 address.
     */
    DECLR0CALLBACKMEMBER(int,  pfnMapRegion,(PRAWPCIDEVPORT pPort,
                                             int32_t        iRegion,
                                             RTHCPHYS       RegionStart,
                                             uint64_t       u64RegionSize,
                                             int32_t        fFlags,
                                             RTR0PTR        *pRegionBaseR0));

    /**
     * Unmap PCI region.
     *
     * @param   pPort     Pointer to this structure.
     * @param   iRegion   Region number.
     * @param   RegionStart     Region start.
     * @param   u64RegionSize   Region size.
     * @param   RegionBase      Base address.
     */
    DECLR0CALLBACKMEMBER(int,  pfnUnmapRegion,(PRAWPCIDEVPORT pPort,
                                               int32_t        iRegion,
                                               RTHCPHYS       RegionStart,
                                               uint64_t       u64RegionSize,
                                               RTR0PTR        RegionBase));

    /**
     * Read device PCI register.
     *
     * @param   pPort     Pointer to this structure.
     * @param   Register  PCI register.
     * @param   pValue    Read value (with desired read width).
     */
    DECLR0CALLBACKMEMBER(int,  pfnPciCfgRead,(PRAWPCIDEVPORT pPort,
                                              uint32_t       Register,
                                              PCIRAWMEMLOC   *pValue));


    /**
     * Write device PCI register.
     *
     * @param   pPort     Pointer to this structure.
     * @param   Register  PCI register.
     * @param   pValue    Write value (with desired write width).
     */
    DECLR0CALLBACKMEMBER(int,  pfnPciCfgWrite,(PRAWPCIDEVPORT    pPort,
                                               uint32_t          Register,
                                               PCIRAWMEMLOC      *pValue));

    /**
     * Request to register interrupt handler.
     *
     * @param   pPort       Pointer to this structure.
     * @param   pfnHandler  Pointer to the handler.
     * @param   pIrqContext Context passed to the handler.
     * @param   phIsr       Handle for the ISR, .
     */
    DECLR0CALLBACKMEMBER(int,  pfnRegisterIrqHandler,(PRAWPCIDEVPORT    pPort,
                                                      PFNRAWPCIISR      pfnHandler,
                                                      void*             pIrqContext,
                                                      PCIRAWISRHANDLE   *phIsr));

    /**
     * Request to unregister interrupt handler.
     *
     * @param   pPort       Pointer to this structure.
     * @param   hIsr        Handle of ISR to unregister (retured by earlier pfnRegisterIrqHandler).
     */
    DECLR0CALLBACKMEMBER(int,  pfnUnregisterIrqHandler,(PRAWPCIDEVPORT  pPort,
                                                        PCIRAWISRHANDLE hIsr));

    /**
     * Power state change notification.
     *
     * @param   pPort       Pointer to this structure.
     * @param   aState      New power state.
     * @param   pu64Param   State-specific in/out parameter.
     */
    DECLR0CALLBACKMEMBER(int,  pfnPowerStateChange,(PRAWPCIDEVPORT    pPort,
                                                    PCIRAWPOWERSTATE  aState,
                                                    uint64_t          *pu64Param));

    /** Structure version number. (RAWPCIDEVPORT_VERSION) */
    uint32_t u32VersionEnd;
} RAWPCIDEVPORT;
/** Version number for the RAWPCIDEVPORT::u32Version and RAWPCIIFPORT::u32VersionEnd fields. */
#define RAWPCIDEVPORT_VERSION   UINT32_C(0xAFBDCC02)

/**
 * The component factory interface for create a raw PCI interfaces.
 */
typedef struct RAWPCIFACTORY
{
    /**
     * Release this factory.
     *
     * SUPR0ComponentQueryFactory (SUPDRVFACTORY::pfnQueryFactoryInterface to be precise)
     * will retain a reference to the factory and the caller has to call this method to
     * release it once the pfnCreateAndConnect call(s) has been done.
     *
     * @param   pFactory            Pointer to this structure.
     */
    DECLR0CALLBACKMEMBER(void, pfnRelease,(PRAWPCIFACTORY pFactory));

    /**
     * Create an instance for the specfied host PCI card and connects it
     * to the driver.
     *
     *
     * @returns VBox status code.
     *
     * @param   pFactory            Pointer to this structure.
     * @param   u32HostAddress      Address of PCI device on the host.
     * @param   fFlags              Creation flags.
     * @param   pVmCtx              Context of VM where device is created.
     * @param   ppDevPort           Where to store the pointer to the device port
     *                              on success.
     * @param   pfDevFlags          Where to store the device flags.
     *
     */
    DECLR0CALLBACKMEMBER(int, pfnCreateAndConnect,(PRAWPCIFACTORY       pFactory,
                                                   uint32_t             u32HostAddress,
                                                   uint32_t             fFlags,
                                                   PRAWPCIPERVM         pVmCtx,
                                                   PRAWPCIDEVPORT       *ppDevPort,
                                                   uint32_t             *pfDevFlags));


    /**
     * Initialize per-VM data related to PCI passthrough.
     *
     * @returns VBox status code.
     *
     * @param   pFactory    Pointer to this structure.
     * @param   pVM         The cross context VM structure.
     * @param   pVmData     Pointer to PCI data.
     */
    DECLR0CALLBACKMEMBER(int, pfnInitVm,(PRAWPCIFACTORY       pFactory,
                                         PVM                  pVM,
                                         PRAWPCIPERVM         pVmData));

    /**
     * Deinitialize per-VM data related to PCI passthrough.
     *
     * @param   pFactory    Pointer to this structure.
     * @param   pVM         The cross context VM structure.
     * @param   pVmData     Pointer to PCI data.
     */
    DECLR0CALLBACKMEMBER(void, pfnDeinitVm,(PRAWPCIFACTORY       pFactory,
                                            PVM                  pVM,
                                            PRAWPCIPERVM         pVmData));
} RAWPCIFACTORY;

#define RAWPCIFACTORY_UUID_STR   "ea089839-4171-476f-adfb-9e7ab1cbd0fb"

/**
 * Flags passed to pfnPciDeviceConstructStart(), to notify driver
 * about options to be used to open device.
 */
typedef enum PCIRAWDRIVERFLAGS
{
    /** If runtime shall  try to detach host driver. */
    PCIRAWDRIVERRFLAG_DETACH_HOST_DRIVER   =  (1 << 0),
    /** The usual 32-bit type blow up. */
    PCIRAWDRIVERRFLAG_32BIT_HACK = 0x7fffffff
} PCIRAWDRIVERFLAGS;

/**
 * Flags used to describe PCI region, matches to PCIADDRESSSPACE
 * in pci.h.
 */
typedef enum PCIRAWADDRESSSPACE
{
    /** Memory. */
    PCIRAW_ADDRESS_SPACE_MEM = 0x00,
    /** I/O space. */
    PCIRAW_ADDRESS_SPACE_IO = 0x01,
    /** 32-bit BAR. */
    PCIRAW_ADDRESS_SPACE_BAR32 = 0x00,
    /** 64-bit BAR. */
    PCIRAW_ADDRESS_SPACE_BAR64 = 0x04,
    /** Prefetch memory. */
    PCIRAW_ADDRESS_SPACE_MEM_PREFETCH = 0x08,
    /** The usual 32-bit type blow up. */
    PCIRAW_ADDRESS_SPACE_32BIT_HACK = 0x7fffffff
} PCIRAWADDRESSSPACE;

RT_C_DECLS_END

/* #define VBOX_WITH_SHARED_PCI_INTERRUPTS */

#endif /* !VBOX_INCLUDED_rawpci_h */
