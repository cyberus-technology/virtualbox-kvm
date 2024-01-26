/** @file
 * PCI - The PCI Controller And Devices. (DEV)
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

#ifndef VBOX_INCLUDED_vmm_pdmpcidev_h
#define VBOX_INCLUDED_vmm_pdmpcidev_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/pci.h>
#include <iprt/assert.h>


/** @defgroup grp_pdm_pcidev       PDM PCI Device
 * @ingroup grp_pdm_device
 * @{
 */

/**
 * Callback function for intercept reading from the PCI configuration space.
 *
 * @returns VINF_SUCCESS or PDMDevHlpDBGFStop status (maybe others later).
 * @retval  VINF_PDM_PCI_DO_DEFAULT to do default read (same as calling
 *          PDMDevHlpPCIConfigRead()).
 *
 * @param   pDevIns         Pointer to the device instance the PCI device
 *                          belongs to.
 * @param   pPciDev         Pointer to PCI device. Use pPciDev->pDevIns to get the device instance.
 * @param   uAddress        The configuration space register address. [0..4096]
 * @param   cb              The register size. [1,2,4]
 * @param   pu32Value       Where to return the register value.
 *
 * @remarks Called with the PDM lock held.  The device lock is NOT take because
 *          that is very likely be a lock order violation.
 */
typedef DECLCALLBACKTYPE(VBOXSTRICTRC, FNPCICONFIGREAD,(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev,
                                                        uint32_t uAddress, unsigned cb, uint32_t *pu32Value));
/** Pointer to a FNPCICONFIGREAD() function. */
typedef FNPCICONFIGREAD *PFNPCICONFIGREAD;
#if !RT_CLANG_PREREQ(11, 0) /* Clang 11 (at least) has trouble with nothrow and pointers to function pointers. */
/** Pointer to a PFNPCICONFIGREAD. */
typedef PFNPCICONFIGREAD *PPFNPCICONFIGREAD;
#endif

/**
 * Callback function for writing to the PCI configuration space.
 *
 * @returns VINF_SUCCESS or PDMDevHlpDBGFStop status (maybe others later).
 * @retval  VINF_PDM_PCI_DO_DEFAULT to do default read (same as calling
 *          PDMDevHlpPCIConfigWrite()).
 *
 * @param   pDevIns         Pointer to the device instance the PCI device
 *                          belongs to.
 * @param   pPciDev         Pointer to PCI device. Use pPciDev->pDevIns to get the device instance.
 * @param   uAddress        The configuration space register address. [0..4096]
 * @param   cb              The register size. [1,2,4]
 * @param   u32Value        The value that's being written. The number of bits actually used from
 *                          this value is determined by the cb parameter.
 *
 * @remarks Called with the PDM lock held.  The device lock is NOT take because
 *          that is very likely be a lock order violation.
 */
typedef DECLCALLBACKTYPE(VBOXSTRICTRC, FNPCICONFIGWRITE,(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev,
                                                         uint32_t uAddress, unsigned cb, uint32_t u32Value));
/** Pointer to a FNPCICONFIGWRITE() function. */
typedef FNPCICONFIGWRITE *PFNPCICONFIGWRITE;
#if !RT_CLANG_PREREQ(11, 0) /* Clang 11 (at least) has trouble with nothrow and pointers to function pointers. */
/** Pointer to a PFNPCICONFIGWRITE. */
typedef PFNPCICONFIGWRITE *PPFNPCICONFIGWRITE;
#endif

/**
 * Callback function for mapping an PCI I/O region.
 *
 * This is called when a PCI I/O region is mapped, and for new-style devices
 * also when unmapped (address set to NIL_RTGCPHYS).  For new-style devices,
 * this callback is optional as the PCI bus calls IOM to map and unmap the
 * regions.
 *
 * Old style devices have to call IOM to map the region themselves, while
 * unmapping is done by the PCI bus like with the new style devices.
 *
 * @returns VBox status code.
 * @retval  VINF_PCI_MAPPING_DONE if the caller already did the mapping and the
 *          PCI bus should not use the handle it got to do the registration
 *          again.  (Only allowed when @a GCPhysAddress is not NIL_RTGCPHYS.)
 *
 * @param   pDevIns         Pointer to the device instance the PCI device
 *                          belongs to.
 * @param   pPciDev         Pointer to the PCI device.
 * @param   iRegion         The region number.
 * @param   GCPhysAddress   Physical address of the region.  If @a enmType is
 *                          PCI_ADDRESS_SPACE_IO, this is an I/O port, otherwise
 *                          it's a physical address.
 *
 *                          NIL_RTGCPHYS indicates that a mapping is about to be
 *                          unmapped and that the device deregister access
 *                          handlers for it and update its internal state to
 *                          reflect this.
 *
 * @param   cb              Size of the region in bytes.
 * @param   enmType         One of the PCI_ADDRESS_SPACE_* values.
 *
 * @remarks Called with the PDM lock held.  The device lock is NOT take because
 *          that is very likely be a lock order violation.
 */
typedef DECLCALLBACKTYPE(int, FNPCIIOREGIONMAP,(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                                RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType));
/** Pointer to a FNPCIIOREGIONMAP() function. */
typedef FNPCIIOREGIONMAP *PFNPCIIOREGIONMAP;


/**
 * Sets the size and type for old saved states from within a
 * PDMPCIDEV::pfnRegionLoadChangeHookR3 callback.
 *
 * @returns VBox status code.
 * @param   pPciDev         Pointer to the PCI device.
 * @param   iRegion         The region number.
 * @param   cbRegion        The region size.
 * @param   enmType         Combination of the PCI_ADDRESS_SPACE_* values.
 */
typedef DECLCALLBACKTYPE(int, FNPCIIOREGIONOLDSETTER,(PPDMPCIDEV pPciDev, uint32_t iRegion, RTGCPHYS cbRegion,
                                                      PCIADDRESSSPACE enmType));
/** Pointer to a FNPCIIOREGIONOLDSETTER() function. */
typedef FNPCIIOREGIONOLDSETTER *PFNPCIIOREGIONOLDSETTER;

/**
 * Swaps two PCI I/O regions from within a PDMPCIDEV::pfnRegionLoadChangeHookR3
 * callback.
 *
 * @returns VBox status code.
 * @param   pPciDev         Pointer to the PCI device.
 * @param   iRegion         The region number.
 * @param   iOtherRegion    The number of the region swap with.
 * @sa      @bugref{9359}
 */
typedef DECLCALLBACKTYPE(int, FNPCIIOREGIONSWAP,(PPDMPCIDEV pPciDev, uint32_t iRegion, uint32_t iOtherRegion));
/** Pointer to a FNPCIIOREGIONSWAP() function. */
typedef FNPCIIOREGIONSWAP *PFNPCIIOREGIONSWAP;


/*
 * Hack to include the PDMPCIDEVINT structure at the right place
 * to avoid duplications of FNPCIIOREGIONMAP and such.
 */
#ifdef PDMPCIDEV_INCLUDE_PRIVATE
# include "pdmpcidevint.h"
#endif

/**
 * PDM PCI Device structure.
 *
 * A PCI device belongs to a PDM device.  A PDM device may have zero or more PCI
 * devices associated with it.  The first PCI device that it registers
 * automatically becomes the default PCI device and can be used implicitly
 * with the device helper APIs.  Subsequent PCI devices must be specified
 * explicitly to the device helper APIs when used.
 */
typedef struct PDMPCIDEV
{
    /** @name Read only data.
     * @{
     */
    /** Magic number (PDMPCIDEV_MAGIC). */
    uint32_t                u32Magic;
    /** PCI device number [11:3] and function [2:0] on the pci bus.
     * @sa VBOX_PCI_DEVFN_MAKE, VBOX_PCI_DEVFN_FUN_MASK, VBOX_PCI_DEVFN_DEV_SHIFT */
    uint32_t                uDevFn;
    /** Size of the valid config space (we always allocate 4KB). */
    uint16_t                cbConfig;
    /** Size of the MSI-X state data optionally following the config space. */
    uint16_t                cbMsixState;
    /** Index into the PDMDEVINS::apPciDev array. */
    uint16_t                idxSubDev;
    uint16_t                u16Padding;
    /** Device name. */
    R3PTRTYPE(const char *) pszNameR3;
    /** @} */

    /**
     * Callback for dealing with size changes.
     *
     * This is set by the PCI device when needed.  It is only needed if any changes
     * in the PCI resources have been made that may be incompatible with saved state
     * (i.e. does not reflect configuration, but configuration defaults changed).
     *
     * The implementation can use PDMDevHlpMMIOExReduce to adjust the resource
     * allocation down in size.  There is currently no way of growing resources.
     * Dropping a resource is automatic.
     *
     * @returns VBox status code.
     * @param   pDevIns         Pointer to the device instance the PCI device
     *                          belongs to.
     * @param   pPciDev         Pointer to the PCI device.
     * @param   iRegion         The region number or UINT32_MAX if old saved state call.
     * @param   cbRegion        The size being loaded, RTGCPHYS_MAX if old saved state
     *                          call, or 0 for dummy 64-bit top half region.
     * @param   enmType         The type being loaded, -1 if old saved state call, or
     *                          0xff if dummy 64-bit top half region.
     * @param   pfnOldSetter    Callback for setting size and type for call
     *                          regarding old saved states.  NULL otherwise.
     * @param   pfnSwapRegions  Used to swaps two regions. The second one must be a
     *                          higher number than @a iRegion.  NULL if old saved
     *                          state.
     */
    DECLR3CALLBACKMEMBER(int, pfnRegionLoadChangeHookR3,(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion,
                                                         uint64_t cbRegion, PCIADDRESSSPACE enmType,
                                                         PFNPCIIOREGIONOLDSETTER pfnOldSetter,
                                                         PFNPCIIOREGIONSWAP pfnSwapRegion));

    /** Reserved for future stuff. */
    uint64_t au64Reserved[4 + (R3_ARCH_BITS == 32 ? 1 : 0)];

    /** Internal data. */
    union
    {
#ifdef PDMPCIDEVINT_DECLARED
        PDMPCIDEVINT        s;
#endif
        uint8_t             padding[0x180];
    } Int;

    /** PCI config space.
     * This is either 256 or 4096 in size.  In the latter case it may be
     * followed by a MSI-X state area. */
    uint8_t                 abConfig[4096];
    /** The MSI-X state data.  Optional. */
    RT_FLEXIBLE_ARRAY_EXTENSION
    uint8_t                 abMsixState[RT_FLEXIBLE_ARRAY];
} PDMPCIDEV;
#ifdef PDMPCIDEVINT_DECLARED
AssertCompile(RT_SIZEOFMEMB(PDMPCIDEV, Int.s) <= RT_SIZEOFMEMB(PDMPCIDEV, Int.padding));
#endif
/** Magic number of PDMPCIDEV::u32Magic (Margaret Eleanor Atwood). */
#define PDMPCIDEV_MAGIC     UINT32_C(0x19391118)

/** Checks that the PCI device structure is valid and belongs to the device
 *  instance, but does not return. */
#ifdef VBOX_STRICT
# define PDMPCIDEV_ASSERT_VALID(a_pDevIns, a_pPciDev) \
    do { \
        uintptr_t const offPciDevInTable = (uintptr_t)(a_pPciDev) - (uintptr_t)pDevIns->apPciDevs[0]; \
        uint32_t  const cbPciDevTmp      = pDevIns->cbPciDev; \
        ASMCompilerBarrier(); \
        AssertMsg(   offPciDevInTable < pDevIns->cPciDevs * cbPciDevTmp \
                  && cbPciDevTmp >= RT_UOFFSETOF(PDMPCIDEV, abConfig) + 256 \
                  && offPciDevInTable % cbPciDevTmp == 0, \
                  ("pPciDev=%p apPciDevs[0]=%p offPciDevInTable=%p cPciDevs=%#x cbPciDev=%#x\n", \
                   (a_pPciDev), pDevIns->apPciDevs[0], offPciDevInTable, pDevIns->cPciDevs, cbPciDevTmp)); \
        AssertPtr((a_pPciDev)); \
        AssertMsg((a_pPciDev)->u32Magic == PDMPCIDEV_MAGIC, ("%#x\n", (a_pPciDev)->u32Magic)); \
    } while (0)
#else
# define PDMPCIDEV_ASSERT_VALID(a_pDevIns, a_pPciDev) do { } while (0)
#endif

/** Checks that the PCI device structure is valid, belongs to the device
 *  instance and that it is registered, but does not return. */
#ifdef VBOX_STRICT
# define PDMPCIDEV_ASSERT_VALID_AND_REGISTERED(a_pDevIns, a_pPciDev) \
    do { \
        PDMPCIDEV_ASSERT_VALID(a_pDevIns, a_pPciDev); \
        Assert((a_pPciDev)->Int.s.fRegistered); \
    } while (0)
#else
# define PDMPCIDEV_ASSERT_VALID_AND_REGISTERED(a_pDevIns, a_pPciDev) do { } while (0)
#endif

/** Checks that the PCI device structure is valid and belongs to the device
 *  instance, returns appropriate status code if not valid. */
#define PDMPCIDEV_ASSERT_VALID_RET(a_pDevIns, a_pPciDev) \
    do { \
        uintptr_t const offPciDevInTable = (uintptr_t)(a_pPciDev) - (uintptr_t)pDevIns->apPciDevs[0]; \
        uint32_t  const cbPciDevTmp      = pDevIns->cbPciDev; \
        ASMCompilerBarrier(); \
        AssertMsgReturn(   offPciDevInTable < pDevIns->cPciDevs * cbPciDevTmp \
                        && cbPciDevTmp >= RT_UOFFSETOF(PDMPCIDEV, abConfig) + 256  \
                        && offPciDevInTable % cbPciDevTmp == 0, \
                        ("pPciDev=%p apPciDevs[0]=%p offPciDevInTable=%p cPciDevs=%#x cbPciDev=%#x\n", \
                        (a_pPciDev), pDevIns->apPciDevs[0], offPciDevInTable, pDevIns->cPciDevs, cbPciDevTmp), \
                        VERR_PDM_NOT_PCI_DEVICE); \
        AssertMsgReturn((a_pPciDev)->u32Magic == PDMPCIDEV_MAGIC, ("%#x\n", (a_pPciDev)->u32Magic), VERR_PDM_NOT_PCI_DEVICE); \
        AssertReturn((a_pPciDev)->Int.s.fRegistered, VERR_PDM_NOT_PCI_DEVICE); \
    } while (0)



/** @name PDM PCI config space accessor function.
 * @{
 */

/** @todo handle extended space access. */

DECLINLINE(void)     PDMPciDevSetByte(PPDMPCIDEV pPciDev, uint32_t offReg, uint8_t u8Value)
{
    Assert(offReg < sizeof(pPciDev->abConfig));
    pPciDev->abConfig[offReg] = u8Value;
}

DECLINLINE(uint8_t)  PDMPciDevGetByte(PCPDMPCIDEV pPciDev, uint32_t offReg)
{
    Assert(offReg < sizeof(pPciDev->abConfig));
    return pPciDev->abConfig[offReg];
}

DECLINLINE(void)     PDMPciDevSetWord(PPDMPCIDEV pPciDev, uint32_t offReg, uint16_t u16Value)
{
    Assert(offReg <= sizeof(pPciDev->abConfig) - sizeof(uint16_t));
    *(uint16_t*)&pPciDev->abConfig[offReg] = RT_H2LE_U16(u16Value);
}

DECLINLINE(uint16_t) PDMPciDevGetWord(PCPDMPCIDEV pPciDev, uint32_t offReg)
{
    uint16_t u16Value;
    Assert(offReg <= sizeof(pPciDev->abConfig) - sizeof(uint16_t));
    u16Value = *(uint16_t*)&pPciDev->abConfig[offReg];
    return RT_H2LE_U16(u16Value);
}

DECLINLINE(void)     PDMPciDevSetDWord(PPDMPCIDEV pPciDev, uint32_t offReg, uint32_t u32Value)
{
    Assert(offReg <= sizeof(pPciDev->abConfig) - sizeof(uint32_t));
    *(uint32_t*)&pPciDev->abConfig[offReg] = RT_H2LE_U32(u32Value);
}

DECLINLINE(uint32_t) PDMPciDevGetDWord(PCPDMPCIDEV pPciDev, uint32_t offReg)
{
    uint32_t u32Value;
    Assert(offReg <= sizeof(pPciDev->abConfig) - sizeof(uint32_t));
    u32Value = *(uint32_t*)&pPciDev->abConfig[offReg];
    return RT_H2LE_U32(u32Value);
}

DECLINLINE(void)     PDMPciDevSetQWord(PPDMPCIDEV pPciDev, uint32_t offReg, uint64_t u64Value)
{
    Assert(offReg <= sizeof(pPciDev->abConfig) - sizeof(uint64_t));
    *(uint64_t*)&pPciDev->abConfig[offReg] = RT_H2LE_U64(u64Value);
}

DECLINLINE(uint64_t) PDMPciDevGetQWord(PCPDMPCIDEV pPciDev, uint32_t offReg)
{
    uint64_t u64Value;
    Assert(offReg <= sizeof(pPciDev->abConfig) - sizeof(uint64_t));
    u64Value = *(uint64_t*)&pPciDev->abConfig[offReg];
    return RT_H2LE_U64(u64Value);
}

/**
 * Sets the vendor id config register.
 * @param   pPciDev         The PCI device.
 * @param   u16VendorId     The vendor id.
 */
DECLINLINE(void) PDMPciDevSetVendorId(PPDMPCIDEV pPciDev, uint16_t u16VendorId)
{
    PDMPciDevSetWord(pPciDev, VBOX_PCI_VENDOR_ID, u16VendorId);
}

/**
 * Gets the vendor id config register.
 * @returns the vendor id.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint16_t) PDMPciDevGetVendorId(PCPDMPCIDEV pPciDev)
{
    return PDMPciDevGetWord(pPciDev, VBOX_PCI_VENDOR_ID);
}


/**
 * Sets the device id config register.
 * @param   pPciDev         The PCI device.
 * @param   u16DeviceId     The device id.
 */
DECLINLINE(void) PDMPciDevSetDeviceId(PPDMPCIDEV pPciDev, uint16_t u16DeviceId)
{
    PDMPciDevSetWord(pPciDev, VBOX_PCI_DEVICE_ID, u16DeviceId);
}

/**
 * Gets the device id config register.
 * @returns the device id.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint16_t) PDMPciDevGetDeviceId(PCPDMPCIDEV pPciDev)
{
    return PDMPciDevGetWord(pPciDev, VBOX_PCI_DEVICE_ID);
}

/**
 * Sets the command config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u16Command      The command register value.
 */
DECLINLINE(void) PDMPciDevSetCommand(PPDMPCIDEV pPciDev, uint16_t u16Command)
{
    PDMPciDevSetWord(pPciDev, VBOX_PCI_COMMAND, u16Command);
}


/**
 * Gets the command config register.
 * @returns The command register value.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint16_t) PDMPciDevGetCommand(PCPDMPCIDEV pPciDev)
{
    return PDMPciDevGetWord(pPciDev, VBOX_PCI_COMMAND);
}

/**
 * Checks if the given PCI device is a bus master.
 * @returns true if the device is a bus master, false if not.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(bool) PDMPciDevIsBusmaster(PCPDMPCIDEV pPciDev)
{
    return (PDMPciDevGetCommand(pPciDev) & VBOX_PCI_COMMAND_MASTER) != 0;
}

/**
 * Checks if INTx interrupts disabled in the command config register.
 * @returns true if disabled.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(bool) PDMPciDevIsIntxDisabled(PCPDMPCIDEV pPciDev)
{
    return (PDMPciDevGetCommand(pPciDev) & VBOX_PCI_COMMAND_INTX_DISABLE) != 0;
}

/**
 * Gets the status config register.
 *
 * @returns status config register.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint16_t) PDMPciDevGetStatus(PCPDMPCIDEV pPciDev)
{
    return PDMPciDevGetWord(pPciDev, VBOX_PCI_STATUS);
}

/**
 * Sets the status config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u16Status       The status register value.
 */
DECLINLINE(void) PDMPciDevSetStatus(PPDMPCIDEV pPciDev, uint16_t u16Status)
{
    PDMPciDevSetWord(pPciDev, VBOX_PCI_STATUS, u16Status);
}


/**
 * Sets the revision id config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8RevisionId    The revision id.
 */
DECLINLINE(void) PDMPciDevSetRevisionId(PPDMPCIDEV pPciDev, uint8_t u8RevisionId)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_REVISION_ID, u8RevisionId);
}


/**
 * Sets the register level programming class config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8ClassProg     The new value.
 */
DECLINLINE(void) PDMPciDevSetClassProg(PPDMPCIDEV pPciDev, uint8_t u8ClassProg)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_CLASS_PROG, u8ClassProg);
}


/**
 * Sets the sub-class (aka device class) config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8SubClass      The sub-class.
 */
DECLINLINE(void) PDMPciDevSetClassSub(PPDMPCIDEV pPciDev, uint8_t u8SubClass)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_CLASS_SUB, u8SubClass);
}


/**
 * Sets the base class config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8BaseClass     The base class.
 */
DECLINLINE(void) PDMPciDevSetClassBase(PPDMPCIDEV pPciDev, uint8_t u8BaseClass)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_CLASS_BASE, u8BaseClass);
}

/**
 * Sets the header type config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8HdrType       The header type.
 */
DECLINLINE(void) PDMPciDevSetHeaderType(PPDMPCIDEV pPciDev, uint8_t u8HdrType)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_HEADER_TYPE, u8HdrType);
}

/**
 * Gets the header type config register.
 *
 * @param   pPciDev         The PCI device.
 * @returns u8HdrType       The header type.
 */
DECLINLINE(uint8_t) PDMPciDevGetHeaderType(PCPDMPCIDEV pPciDev)
{
    return PDMPciDevGetByte(pPciDev, VBOX_PCI_HEADER_TYPE);
}

/**
 * Sets the BIST (built-in self-test) config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8Bist          The BIST value.
 */
DECLINLINE(void) PDMPciDevSetBIST(PPDMPCIDEV pPciDev, uint8_t u8Bist)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_BIST, u8Bist);
}

/**
 * Gets the BIST (built-in self-test) config register.
 *
 * @param   pPciDev         The PCI device.
 * @returns u8Bist          The BIST.
 */
DECLINLINE(uint8_t) PDMPciDevGetBIST(PCPDMPCIDEV pPciDev)
{
    return PDMPciDevGetByte(pPciDev, VBOX_PCI_BIST);
}


/**
 * Sets a base address config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   iReg            Base address register number (0..5).
 * @param   fIOSpace        Whether it's I/O (true) or memory (false) space.
 * @param   fPrefetchable   Whether the memory is prefetachable. Must be false if fIOSpace == true.
 * @param   f64Bit          Whether the memory can be mapped anywhere in the 64-bit address space. Otherwise restrict to 32-bit.
 * @param   u32Addr         The address value.
 */
DECLINLINE(void) PDMPciDevSetBaseAddress(PPDMPCIDEV pPciDev, uint8_t iReg, bool fIOSpace, bool fPrefetchable, bool f64Bit,
                                         uint32_t u32Addr)
{
    if (fIOSpace)
    {
        Assert(!(u32Addr & 0x3)); Assert(!fPrefetchable); Assert(!f64Bit);
        u32Addr |= RT_BIT_32(0);
    }
    else
    {
        Assert(!(u32Addr & 0xf));
        if (fPrefetchable)
            u32Addr |= RT_BIT_32(3);
        if (f64Bit)
            u32Addr |= 0x2 << 1;
    }
    switch (iReg)
    {
        case 0: iReg = VBOX_PCI_BASE_ADDRESS_0; break;
        case 1: iReg = VBOX_PCI_BASE_ADDRESS_1; break;
        case 2: iReg = VBOX_PCI_BASE_ADDRESS_2; break;
        case 3: iReg = VBOX_PCI_BASE_ADDRESS_3; break;
        case 4: iReg = VBOX_PCI_BASE_ADDRESS_4; break;
        case 5: iReg = VBOX_PCI_BASE_ADDRESS_5; break;
        default: AssertFailedReturnVoid();
    }

    PDMPciDevSetDWord(pPciDev, iReg, u32Addr);
}

/**
 * Please document me. I don't seem to be getting as much as calculating
 * the address of some PCI region.
 */
DECLINLINE(uint32_t) PDMPciDevGetRegionReg(uint32_t iRegion)
{
    return iRegion == VBOX_PCI_ROM_SLOT
         ? VBOX_PCI_ROM_ADDRESS : (VBOX_PCI_BASE_ADDRESS_0 + iRegion * 4);
}

/**
 * Sets the sub-system vendor id config register.
 *
 * @param   pPciDev             The PCI device.
 * @param   u16SubSysVendorId   The sub-system vendor id.
 */
DECLINLINE(void) PDMPciDevSetSubSystemVendorId(PPDMPCIDEV pPciDev, uint16_t u16SubSysVendorId)
{
    PDMPciDevSetWord(pPciDev, VBOX_PCI_SUBSYSTEM_VENDOR_ID, u16SubSysVendorId);
}

/**
 * Gets the sub-system vendor id config register.
 * @returns the sub-system vendor id.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint16_t) PDMPciDevGetSubSystemVendorId(PCPDMPCIDEV pPciDev)
{
    return PDMPciDevGetWord(pPciDev, VBOX_PCI_SUBSYSTEM_VENDOR_ID);
}


/**
 * Sets the sub-system id config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u16SubSystemId  The sub-system id.
 */
DECLINLINE(void) PDMPciDevSetSubSystemId(PPDMPCIDEV pPciDev, uint16_t u16SubSystemId)
{
    PDMPciDevSetWord(pPciDev, VBOX_PCI_SUBSYSTEM_ID, u16SubSystemId);
}

/**
 * Gets the sub-system id config register.
 * @returns the sub-system id.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint16_t) PDMPciDevGetSubSystemId(PCPDMPCIDEV pPciDev)
{
    return PDMPciDevGetWord(pPciDev, VBOX_PCI_SUBSYSTEM_ID);
}

/**
 * Sets offset to capability list.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8Offset        The offset to capability list.
 */
DECLINLINE(void) PDMPciDevSetCapabilityList(PPDMPCIDEV pPciDev, uint8_t u8Offset)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_CAPABILITY_LIST, u8Offset);
}

/**
 * Returns offset to capability list.
 *
 * @returns offset to capability list.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint8_t) PDMPciDevGetCapabilityList(PCPDMPCIDEV pPciDev)
{
    return PDMPciDevGetByte(pPciDev, VBOX_PCI_CAPABILITY_LIST);
}

/**
 * Sets the interrupt line config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8Line          The interrupt line.
 */
DECLINLINE(void) PDMPciDevSetInterruptLine(PPDMPCIDEV pPciDev, uint8_t u8Line)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_INTERRUPT_LINE, u8Line);
}

/**
 * Gets the interrupt line config register.
 *
 * @returns The interrupt line.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint8_t) PDMPciDevGetInterruptLine(PCPDMPCIDEV pPciDev)
{
    return PDMPciDevGetByte(pPciDev, VBOX_PCI_INTERRUPT_LINE);
}

/**
 * Sets the interrupt pin config register.
 *
 * @param   pPciDev         The PCI device.
 * @param   u8Pin           The interrupt pin.
 */
DECLINLINE(void) PDMPciDevSetInterruptPin(PPDMPCIDEV pPciDev, uint8_t u8Pin)
{
    PDMPciDevSetByte(pPciDev, VBOX_PCI_INTERRUPT_PIN, u8Pin);
}

/**
 * Gets the interrupt pin config register.
 *
 * @returns The interrupt pin.
 * @param   pPciDev         The PCI device.
 */
DECLINLINE(uint8_t) PDMPciDevGetInterruptPin(PCPDMPCIDEV pPciDev)
{
    return PDMPciDevGetByte(pPciDev, VBOX_PCI_INTERRUPT_PIN);
}

/** @} */

/** @name Aliases for old function names.
 * @{
 */
#if !defined(PDMPCIDEVICE_NO_DEPRECATED) || defined(DOXYGEN_RUNNING)
# define PCIDevSetByte               PDMPciDevSetByte
# define PCIDevGetByte               PDMPciDevGetByte
# define PCIDevSetWord               PDMPciDevSetWord
# define PCIDevGetWord               PDMPciDevGetWord
# define PCIDevSetDWord              PDMPciDevSetDWord
# define PCIDevGetDWord              PDMPciDevGetDWord
# define PCIDevSetQWord              PDMPciDevSetQWord
# define PCIDevGetQWord              PDMPciDevGetQWord
# define PCIDevSetVendorId           PDMPciDevSetVendorId
# define PCIDevGetVendorId           PDMPciDevGetVendorId
# define PCIDevSetDeviceId           PDMPciDevSetDeviceId
# define PCIDevGetDeviceId           PDMPciDevGetDeviceId
# define PCIDevSetCommand            PDMPciDevSetCommand
# define PCIDevGetCommand            PDMPciDevGetCommand
# define PCIDevIsBusmaster           PDMPciDevIsBusmaster
# define PCIDevIsIntxDisabled        PDMPciDevIsIntxDisabled
# define PCIDevGetStatus             PDMPciDevGetStatus
# define PCIDevSetStatus             PDMPciDevSetStatus
# define PCIDevSetRevisionId         PDMPciDevSetRevisionId
# define PCIDevSetClassProg          PDMPciDevSetClassProg
# define PCIDevSetClassSub           PDMPciDevSetClassSub
# define PCIDevSetClassBase          PDMPciDevSetClassBase
# define PCIDevSetHeaderType         PDMPciDevSetHeaderType
# define PCIDevGetHeaderType         PDMPciDevGetHeaderType
# define PCIDevSetBIST               PDMPciDevSetBIST
# define PCIDevGetBIST               PDMPciDevGetBIST
# define PCIDevSetBaseAddress        PDMPciDevSetBaseAddress
# define PCIDevGetRegionReg          PDMPciDevGetRegionReg
# define PCIDevSetSubSystemVendorId  PDMPciDevSetSubSystemVendorId
# define PCIDevGetSubSystemVendorId  PDMPciDevGetSubSystemVendorId
# define PCIDevSetSubSystemId        PDMPciDevSetSubSystemId
# define PCIDevGetSubSystemId        PDMPciDevGetSubSystemId
# define PCIDevSetCapabilityList     PDMPciDevSetCapabilityList
# define PCIDevGetCapabilityList     PDMPciDevGetCapabilityList
# define PCIDevSetInterruptLine      PDMPciDevSetInterruptLine
# define PCIDevGetInterruptLine      PDMPciDevGetInterruptLine
# define PCIDevSetInterruptPin       PDMPciDevSetInterruptPin
# define PCIDevGetInterruptPin       PDMPciDevGetInterruptPin
#endif
/** @} */


/** @name PDMIICH9BRIDGEPDMPCIDEV_IID - Ugly 3rd party bridge/raw PCI hack.
 *
 * When querying this IID via IBase::pfnQueryInterface on a ICH9 bridge, you
 * will get a pointer to a PDMPCIDEV rather pointer to an interface function
 * table as is the custom.  This was needed by some unusual 3rd-party raw and/or
 * pass-through implementation which need to provide different PCI configuration
 * space content for bridges (as long as we don't allow pass-through of bridges
 * or custom bridge device implementations).  So, HACK ALERT to all of this!
 * @{ */
#define PDMIICH9BRIDGEPDMPCIDEV_IID "785c74b1-8510-4458-9422-56750bf221db"
typedef PPDMPCIDEV PPDMIICH9BRIDGEPDMPCIDEV;
typedef PDMPCIDEV  PDMIICH9BRIDGEPDMPCIDEV;
/** @} */


/** @} */

#endif /* !VBOX_INCLUDED_vmm_pdmpcidev_h */
