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

#include "DevVfio.h"

#include <iprt/mem.h>
#include <VBox/log.h>
#include <VBox/vmm/pgm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmpcidevint.h>
#include "DevPciInternal.h"

#include <fcntl.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstring>
#include <optional>

namespace {
    using IrqType = VfioDevice::IrqType;

    VBOXSTRICTRC vfioConfigSpaceRead(PPDMDEVINS pDev, PPDMPCIDEV pPciDev, uint32_t uAddress, unsigned cb, uint32_t* pu32Value)
    {
        PVFIODEV pThis {PDMDEVINS_2_DATA(pDev, PVFIODEV)};

        AssertLogRelMsgReturn(pu32Value, ("VFIO: PCi config space read: value pointer is zero!"), VERR_INVALID_POINTER);

        int rc { pThis->readConfigSpace(*pu32Value, cb, uAddress) };
        writePciConfigSpaceShadow(pPciDev, uAddress, cb, *pu32Value);
        return rc;
    }

    std::underlying_type_t<IrqType> toUnderlying(const IrqType& t)
    {
        return static_cast<std::underlying_type_t<IrqType>>(t);
    }

}

int VfioDevice::initializeVfio(PPDMDEVINS pDevIns, std::filesystem::path sysfsPath)
{
    namespace fs = std::filesystem;
    const std::filesystem::path VFIO_PATH {"/dev/vfio"};

    int rc {VINF_SUCCESS};

    vfioContainerFd = open((VFIO_PATH / "vfio").c_str(), O_RDWR | O_CLOEXEC);
    AssertLogRelMsgReturn(vfioContainerFd > 0, ("VFIO: Could not open VFIO Container\n"), VERR_INVALID_PARAMETER);

    const int vfioApiVersion {ioctl(vfioContainerFd, VFIO_GET_API_VERSION)};

    LogRel(("VFIO: Detected VFIO Api Version %d\n", vfioApiVersion));

    const int iommuTypePresent {ioctl(vfioContainerFd, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)};
    AssertLogRelMsgReturn(iommuTypePresent, ("VFIO: Requested IOMMU type is not supported.\n"), VERR_NOT_AVAILABLE);

    const auto iommuGroupLink {fs::read_symlink(sysfsPath / "iommu_group")};
    vfioGroupFd = open((VFIO_PATH / iommuGroupLink.filename()).c_str(), O_RDWR, O_CLOEXEC);
    AssertLogRelMsgReturn(vfioGroupFd > 0, ("VFIO: Could not open VFIO Container\n"), VERR_INVALID_PARAMETER);

    rc = vfioControl(pDevIns, vfioGroupFd, VFIO_GROUP_SET_CONTAINER,
                    "VFIO: Unable to assign the VFIO container to the VFIO Group \n", &vfioContainerFd);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    rc = vfioControl(pDevIns, vfioContainerFd, VFIO_SET_IOMMU, "VFIO: Unable to set VFIO IOMMU Type \n", VFIO_TYPE1_IOMMU);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    vfioDeviceFd = ioctl(vfioGroupFd, VFIO_GROUP_GET_DEVICE_FD, sysfsPath.filename().c_str());
    AssertLogRelMsgReturn(vfioDeviceFd > 0, ("VFIO: Unable to open VFIO device \n"), VERR_INVALID_PARAMETER);

    rc = deviceControl(pDevIns, VFIO_DEVICE_RESET, "Unable to reset VFIO device");
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    return rc;
}

int VfioDevice::initializePci(PPDMDEVINS pDevIns)
{
    int rc {VINF_SUCCESS};

    pPciDev = pDevIns->apPciDevs[0];
    PDMPCIDEV_ASSERT_VALID(pDevIns, pPciDev);

    vfio_region_info regionInfo;
    regionInfo.argsz = sizeof(regionInfo);
    regionInfo.index = VFIO_PCI_CONFIG_REGION_INDEX;

    rc = deviceControl(pDevIns, VFIO_DEVICE_GET_REGION_INFO, "VFIO: Could not retrieve VFIO Device MCFG region\n", &regionInfo);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);
    AssertLogRelMsgReturn(regionInfo.size != 0, ("VFIO: MCFG Region size is zero\n"), VERR_INVALID_PARAMETER);

    mcfgOffset = regionInfo.offset;

    uint16_t vendorId, deviceId;
    uint8_t classBase, classSub, headerType, interruptPin, interruptLine;

    readConfigSpace(vendorId, sizeof(vendorId), VBOX_PCI_VENDOR_ID);
    readConfigSpace(deviceId, sizeof(deviceId), VBOX_PCI_DEVICE_ID);
    readConfigSpace(classBase, sizeof(classBase), VBOX_PCI_CLASS_BASE);
    readConfigSpace(classSub, sizeof(classSub), VBOX_PCI_CLASS_SUB);
    readConfigSpace(headerType, sizeof(headerType), VBOX_PCI_HEADER_TYPE);
    readConfigSpace(interruptLine, sizeof(interruptLine), VBOX_PCI_INTERRUPT_LINE);
    readConfigSpace(interruptPin, sizeof(interruptPin), VBOX_PCI_INTERRUPT_PIN);

    PDMPciDevSetVendorId(pPciDev, vendorId);
    PDMPciDevSetDeviceId(pPciDev, deviceId);
    PDMPciDevSetClassBase(pPciDev, classBase);
    PDMPciDevSetClassSub(pPciDev, classSub);
    PDMPciDevSetHeaderType(pPciDev, headerType);
    PDMPciDevSetInterruptLine(pPciDev, interruptLine);
    PDMPciDevSetInterruptPin(pPciDev, interruptPin);

    rc = PDMDevHlpPCIRegister(pDevIns, pPciDev);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    CapabilityList capList {vfioConfigSpaceRead, pDevIns};
    msiCapabilityIterator = capList.getCapabilityIterator(VBOX_PCI_CAP_ID_MSI);
    msixCapabilityIterator = capList.getCapabilityIterator(VBOX_PCI_CAP_ID_MSIX);


    if (msiCapabilityIterator)
    {
        MSICapabilityDescriptor msiCap {*msiCapabilityIterator};
        AssertLogRelMsgReturn(msiCap.maxCount() == 1, ("VFIO: Multiple Message MSI supporting devices are not supported yet!\n"), VERR_NOT_SUPPORTED);
    }


    return rc;
}

int VfioDevice::initializeMemoryRegions(PPDMDEVINS pDevIns, vfio_device_info& deviceInfo)
{
    int rc {VINF_SUCCESS};
    for (auto i {0u}; i < deviceInfo.num_regions; ++i)
    {
        /**
         * Currently only PCI Bar regions are supported.
         * VFIO places the bar region information at indices
         * 0 <= i <= VBOX_PCI_MAX_BARS, so we can stop if the
         * limit is reached
         *
         * TODO implement special region handling
         */
        if (i >= VBOX_PCI_MAX_BARS)
        {
            break;
        }

        vfio_region_info regionInfo;
        regionInfo.argsz = sizeof(regionInfo);
        regionInfo.index = i;

        rc = deviceControl(pDevIns, VFIO_DEVICE_GET_REGION_INFO, "VFIO: Unable to retrieve VFIO region info",  &regionInfo);
        AssertLogRelReturn(RT_SUCCESS(rc), rc);

        if (regionInfo.size == 0)
        {
            continue;
        }

        const auto barInfo {getBarInfo(i)};

        PCIBarRegion& region {pciBars[i]};
        region.offset = regionInfo.offset;
        region.size = regionInfo.size;
        region.iRegion = i;

        if (barInfo.isIoBar())
        {
            auto portIoRead = [](PPDMDEVINS pDev, void* pvUser , RTIOPORT offsetPort, uint32_t* pu32, unsigned cb) -> VBOXSTRICTRC
            {
                PVFIODEV pThis {PDMDEVINS_2_DATA(pDev, PVFIODEV)};
                auto pBar {static_cast<PPCIBARREGION>(pvUser)};

                AssertLogRelReturn(pu32, VERR_INVALID_POINTER);
                AssertLogRelReturn(pBar, VERR_INVALID_POINTER);

                return pThis->readFromDevice(pu32, cb, pBar->offset + offsetPort);
            };

            auto portIoWrite = [](PPDMDEVINS pDev, void* pvUser, RTIOPORT offsetPort, uint32_t u32, unsigned cb) -> VBOXSTRICTRC
            {
                PVFIODEV pThis {PDMDEVINS_2_DATA(pDev, PVFIODEV)};
                auto pBar {static_cast<PPCIBARREGION>(pvUser)};

                AssertLogRelReturn(pBar, VERR_INVALID_POINTER);

                return pThis->writeToDevice(&u32, cb, pBar->offset + offsetPort);
            };

            rc = PDMDevHlpPCIIORegionCreateIo(pDevIns, i, region.size, portIoWrite, portIoRead,
                                              &region, "VFIO Port IO", nullptr, &region.hRegion);
            AssertLogRelReturn(RT_SUCCESS(rc), rc);
        }
        else if (barInfo.isMmioBar())
        {
            region.address = NIL_RTGCPHYS;

            auto mmioRead = [](PPDMDEVINS pDev, void* pvUser, RTGCPHYS barOffset, void* pv, unsigned cb) -> VBOXSTRICTRC
            {
                PVFIODEV pThis {PDMDEVINS_2_DATA(pDev, PVFIODEV)};
                auto pBar {static_cast<PPCIBARREGION>(pvUser)};

                AssertLogRelReturn(pBar, VERR_INVALID_POINTER);

                return pThis->mmioAccessHandler(pDev, *pBar, barOffset, pv, cb, false);
            };

            auto mmioWrite = [](PPDMDEVINS pDev, void* pvUser, RTGCPHYS barOffset, const void * pv, unsigned cb) -> VBOXSTRICTRC
            {
                PVFIODEV pThis {PDMDEVINS_2_DATA(pDev, PVFIODEV)};
                auto pBar {static_cast<PPCIBARREGION>(pvUser)};

                AssertLogRelReturn(pBar, VERR_INVALID_POINTER);

                return pThis->mmioAccessHandler(pDev, *pBar, barOffset, const_cast<void*>(pv), cb, true);
            };

            rc = PDMDevHlpMmioCreate(pDevIns,
                                     region.size,
                                     NULL,
                                     UINT32_MAX,
                                     mmioWrite,
                                     mmioRead,
                                     &region,
                                     IOMMMIO_FLAGS_READ_PASSTHRU | IOMMMIO_FLAGS_WRITE_PASSTHRU,
                                     "VFIO MMIO BAR",
                                     &region.hRegion);
        }

    }
    return rc;
}

int VfioDevice::handleInterrupts(PPDMDEVINS pDevIns)
{
    // Waits for input on a file descriptor with a given timeout.
    // Taken from https://www.gnu.org/software/libc/manual/html_node/Waiting-for-I_002fO.html
    // Returns the first file descriptor that has input
    auto waitForInput = [&] (std::chrono::microseconds delay) -> std::optional<InterruptInformation>
    {
        fd_set set;
        struct timeval timeout {0, 0};

        /* Initialize the file descriptor set. */
        FD_ZERO(&set);

        /*
         * We use a copy of the interrupts here to avoid firing interrupts that are deactivated already.
         */
        irqDisable.lock();
        std::vector<InterruptInformation> aCurrentIrqInformation {aIrqInformation};
        irqDisable.unlock();

        for (const auto efd : aCurrentIrqInformation)
        {
            if (efd.fd > 0)
            {
                FD_SET(efd.fd, &set);
            }
        }

        /* Initialize the timeout data structure. */
        const auto seconds {std::chrono::duration_cast<std::chrono::seconds>(delay)};
        const auto us {std::chrono::duration_cast<std::chrono::microseconds>(delay - seconds)};

        timeout.tv_sec = seconds.count();
        timeout.tv_usec = us.count();

        /* select returns 0 if timeout, 1 if input available, -1 if error. */
        int error = TEMP_FAILURE_RETRY(select(FD_SETSIZE,
                                       &set, NULL, NULL,
                                       &timeout));

        if (error == -1)
        {
            perror("select on fds failed");
        }

        Assert(error != -1);

        {
            LockGuard _ {irqDisable};

            /*
             * skip delivering non active interrupts
             */
            if (aCurrentIrqInformation != aIrqInformation)
            {
                return std::nullopt;
            }

            for (const auto efd : aCurrentIrqInformation)
            {
                if (efd.fd >= 0 and FD_ISSET(efd.fd, &set))
                {
                    return efd;
                }
            }
        }

        return std::nullopt;
    };

    while (not exit.load())
    {
        if (auto irqInfo = waitForInput(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::seconds(1))); irqInfo)
        {
            uint64_t value;
            const ssize_t s {read(irqInfo->fd, &value, sizeof(value))};
            AssertLogRelMsgReturn(s == sizeof(value), ("VFIO: Read on event FD returned wrong size."), VERR_GENERAL_FAILURE);
            AssertLogRelReturn(value != 0, VERR_INTERRUPTED);
            int rc {VINF_SUCCESS};
            switch (activeInterruptType)
            {
            case IrqType::VFIO_INTX:
                PDMDevHlpPCISetIrqNoWait(pDevIns, 0, PDM_IRQ_LEVEL_FLIP_FLOP);
                break;
            case IrqType::VFIO_MSI:
                rc = injectMsi(pDevIns, *irqInfo);
                break;
            case IrqType::VFIO_MSIX:
                rc = injectMsix(pDevIns, *irqInfo);
                break;
            default:
                AssertLogRelMsgFailedReturn(("VFIO: Unsupported interrupt type in IRQ delivery thread detected %u\n", toUnderlying(activeInterruptType)), VERR_NOT_SUPPORTED);
            }
            AssertLogRelReturn(RT_SUCCESS(rc), rc);
        }
    }

    return VINF_SUCCESS;
}

int VfioDevice::initializeInterrupts(PPDMDEVINS pDevIns)
{
    int rc {activateInterrupts(pDevIns, IrqType::VFIO_INTX)};
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    /*
     *  We need to shadow the MSIX table, as a read access on the table returns invalid data.
     *  Thus we need to allocate MSIX table entries upfront, to be able to handle MSIX table writes.
     */
    PDMMSIREG MsiReg;
    RT_ZERO(MsiReg);

    if (msiCapabilityIterator)
    {
        MSICapabilityDescriptor msiCap {*msiCapabilityIterator};

        MsiReg.cMsiVectors = msiCap.maxCount();
        MsiReg.iMsiCapOffset = msiCapabilityIterator->getOffset();
        MsiReg.iMsiNextOffset = msiCap.nextPtr;
        MsiReg.fMsi64bit = msiCap.is64Bit();
        MsiReg.fMsiNoMasking = not msiCap.isPerVectorMaskable();
    }

    // if (msixCapabilityIterator)
    // {
        // MSIXCapabilityDescriptor msixCap {*msixCapabilityIterator};
        // aMsixTable.resize(msixCap.tableSize());
        // MsiReg.cMsixVectors = msixCap.tableSize();
        // MsiReg.iMsixCapOffset = msixCapabilityIterator->getOffset();
        // MsiReg.iMsixNextOffset = msixCap.nextPtr;
        // MsiReg.iMsixBar = msixCap.getBarIndex();
    // }

    if (msixCapabilityIterator or msiCapabilityIterator)
    {
        rc = PDMDevHlpPCIRegisterMsi(pDevIns, &MsiReg);
    }

    auto handleIrqs = [](RTTHREAD /*hSelf*/, void* pvUser) -> int
    {
        PPDMDEVINS pDev {static_cast<PPDMDEVINS>(pvUser)};
        PVFIODEV pThis {PDMDEVINS_2_DATA(pDev, PVFIODEV)};

        return pThis->handleInterrupts(pDev);
    };

    rc = RTThreadCreate(&hIrqDeliveryThread, handleIrqs, pDevIns, 0, RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE, "vfio IRQ");
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    return rc;
}

int VfioDevice::activateInterrupts(PPDMDEVINS pDevIns, const IrqType irqType, uint32_t irqCount)
{
    LockGuard _ {irqDisable};

    int rc;
    vfio_irq_info irqInfo;
    irqInfo.argsz = sizeof(irqInfo);
    irqInfo.index = toUnderlying(irqType);

    /**
     * The call of this function requires that the interrupts are disabled.
     */
    AssertLogRelMsgReturn(aIrqInformation.size() == 0,
        ("VFIO: Trying to activate interrupts without deactivating the previous irqs! Disable irqs before activate new ones!"),
        VERR_NOT_SUPPORTED);

    /**
     * If the IRQ is not enabled in the VFIO device the call will return unsuccessful
     * and we don't need to set up something for this IRQ and can just continue
     */
    if (RT_FAILURE(deviceControl(pDevIns, VFIO_DEVICE_GET_IRQ_INFO, "", &irqInfo)))
    {
        return VERR_NOT_AVAILABLE;
    }

    /**
     * Some devices, (e.G SRIOV virtual functions does not have legacy interrupts enabled.
     * We can skip interrupt activation if we find a device without legacy interrupts.
     */
    if (irqType == IrqType::VFIO_INTX and irqInfo.count == 0)
    {
        uint8_t interruptPin;
        readConfigSpace(interruptPin, sizeof(interruptPin), VBOX_PCI_INTERRUPT_PIN);
        AssertLogRelMsgReturn(interruptPin == 0, ("VFIO: Found device without INTX information, but INTX is marked as supported in the PCI Config space"), VERR_NOT_AVAILABLE);
        return VINF_SUCCESS;
    }

    /**
     * If we try to activate an interrupt type that is not enabled, or supported by vfio we get an interrupt count of 0
     * We bail out here, as we are not able to enable an interrupt type with no interrupts.
     */
    if (irqInfo.count == 0)
    {
        LogRel(("VFIO: Trying to activate IRQ type %u, but no IRQs of that type are configured\n", toUnderlying(irqType)));
        return VERR_NOT_AVAILABLE;
    }

    /**
     * Sanity check: If we request a larger number of interrutps, the VFIO device is able to support we bail out here.
     */
    if (irqInfo.count < irqCount)
    {
        LogRel(("VFIO: Trying to register %lu irqs, but %lu are supported for type %u.\n", irqCount, irqInfo.count, toUnderlying(irqType)));
        return VERR_NOT_SUPPORTED;
    }

    AssertLogRelReturn(irqInfo.flags & VFIO_IRQ_INFO_EVENTFD, VERR_NOT_AVAILABLE);

    const auto setSize {sizeof(vfio_irq_set) + sizeof(int) * irqCount};
    std::vector<uint8_t> buf(setSize);
    vfio_irq_set& irqSet {*reinterpret_cast<vfio_irq_set*>(buf.data())};

    irqSet.argsz = setSize;
    irqSet.flags = VFIO_IRQ_SET_DATA_EVENTFD | VFIO_IRQ_SET_ACTION_TRIGGER;
    irqSet.index = irqInfo.index;
    irqSet.start = 0;
    irqSet.count = irqCount;

    /*
     * Logging and sanity checking only.
     */
    switch (irqType)
    {
    case IrqType::VFIO_INTX:
        AssertLogRelMsgReturn(irqInfo.count == 1,
                              ("VFIO: Only a single INTX is supported! Detected Count: %u\n", irqInfo.count),
                              VERR_NOT_IMPLEMENTED);
        LogRel(("VFIO: Activate INTX\n"));
        break;
    case IrqType::VFIO_MSI:
        LogRel(("VFIO: Activate MSI count: %u\n", irqCount));
        break;
    case IrqType::VFIO_MSIX:
        LogRel(("VFIO: Activate MSIX: count %u\n", irqCount));
        break;
    default:
        AssertLogRelMsgFailedReturn(("VFIO: Found unsupported vfio IRQ type: %u, count: %u\n", irqInfo.index, irqInfo.count), VERR_NOT_IMPLEMENTED);
    }

    activeInterruptType = irqType;

    for (uint32_t i {0ul}; i < irqCount; ++i)
    {
        int eventFd {eventfd(0, 0)};

        AssertLogRelMsgReturn(eventFd > 0,("VFIO: could not request additional eventfds\n"), VERR_ACCESS_DENIED);
        aIrqInformation.push_back({eventFd, i});
    }

    for (auto i {0ul}; i < aIrqInformation.size(); ++i)
    {
        reinterpret_cast<int*>(irqSet.data)[i] = aIrqInformation[i].fd;
    }

    rc = deviceControl(pDevIns, VFIO_DEVICE_SET_IRQS, "VFIO: Could not set irq info\n", &irqSet);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    return rc;
}

int VfioDevice::disableInterrupts(PPDMDEVINS pDevIns)
{
    LockGuard _ {irqDisable};

    if (aIrqInformation.size() != 0 and activeInterruptType != IrqType::VFIO_NONE)
    {
        const auto setSize {sizeof(vfio_irq_set)};
        std::vector<uint8_t> buf(setSize);
        vfio_irq_set& irqSet {*reinterpret_cast<vfio_irq_set*>(buf.data())};

        irqSet.argsz = setSize;
        irqSet.flags = VFIO_IRQ_SET_DATA_NONE | VFIO_IRQ_SET_ACTION_TRIGGER;
        irqSet.index = toUnderlying(activeInterruptType);
        irqSet.start = 0;
        irqSet.count = 0;

        int rc {deviceControl(pDevIns, VFIO_DEVICE_SET_IRQS, "VFIO: Could not set irq info for deactivation\n", &irqSet)};
        AssertLogRelReturn(RT_SUCCESS(rc), rc);

        for(auto info: aIrqInformation)
        {
            close(info.fd);
        }

        aIrqInformation.clear();
    }

    return VINF_SUCCESS;
}


int VfioDevice::injectMsi(PPDMDEVINS pDevIns, InterruptInformation& irqInfo)
{

    AssertLogRelMsgReturn(msiCapabilityIterator, ("VFIO: Pending MSI, but the capability is not provided \n"), VERR_NOT_SUPPORTED);
    MSICapabilityDescriptor cap(*msiCapabilityIterator);

    AssertLogRelMsgReturn(cap.enabled(), ("VFIO: Pending MSI, but the capability is disabled \n"), VERR_NOT_SUPPORTED);

    if (not cap.isMasked(irqInfo.index))
    {
        PDMDevHlpPCISetIrqNoWait(pDevIns, 0, PDM_IRQ_LEVEL_HIGH);
    }

    return VINF_SUCCESS;
}

int VfioDevice::injectMsix(PPDMDEVINS pDevIns, InterruptInformation& irqInfo)
{
    AssertLogRelMsgReturn(msixCapabilityIterator, ("VFIO: Pending MSIX, but the capability is not provided \n"), VERR_NOT_SUPPORTED);
    MSIXCapabilityDescriptor cap(*msixCapabilityIterator);

    AssertLogRelMsgReturn(cap.enabled(), ("VFIO: Pending MSIX, but the capability is disabled \n"), VERR_NOT_SUPPORTED);

    PDMDevHlpPCISetIrqNoWait(pDevIns,  irqInfo.index, PDM_IRQ_LEVEL_HIGH);
    return VINF_SUCCESS;
}

int VfioDevice::configSpaceWriteHandler(PPDMDEVINS pDevIns, uint32_t uAddress, unsigned cb, uint32_t u32Value)
{
    int rc {VINF_SUCCESS};

    if (uAddress == VBOX_PCI_COMMAND)
    {
        tryHandleBarInterception(pDevIns, u32Value);
    }
    else if (msiCapabilityIterator and  (uAddress >= msiCapabilityIterator->getOffset() and uAddress < (msiCapabilityIterator->getOffset() + sizeof(MSICapabilityDescriptor))))
    {
        MSICapabilityDescriptor lastCap {*msiCapabilityIterator};

        MSICapabilityDescriptor updatedCap {*msiCapabilityIterator};
        std::memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(&updatedCap) + (uAddress - msiCapabilityIterator->getOffset())), &u32Value, cb);

        if (not updatedCap.enabled() and lastCap.enabled())
        {
            rc = disableInterrupts(pDevIns);
            AssertLogRelReturn(RT_SUCCESS(rc), rc);
            rc = activateInterrupts(pDevIns, IrqType::VFIO_INTX);
            AssertLogRelReturn(RT_SUCCESS(rc), rc);
        }
        else if (updatedCap.enabled())
        {
            rc = disableInterrupts(pDevIns);
            AssertLogRelReturn(RT_SUCCESS(rc), rc);
            rc = activateInterrupts(pDevIns, IrqType::VFIO_MSI, updatedCap.count());
            AssertLogRelReturn(RT_SUCCESS(rc), rc);
        }
    }
    else if (msixCapabilityIterator and (uAddress >= msixCapabilityIterator->getOffset() and uAddress < (msixCapabilityIterator->getOffset() + sizeof(MSIXCapabilityDescriptor))))
    {
        MSIXCapabilityDescriptor lastCap {*msixCapabilityIterator};

        MSIXCapabilityDescriptor updatedCap {lastCap};
        std::memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(&updatedCap) + (uAddress - msixCapabilityIterator->getOffset())), &u32Value, cb);

        if (not updatedCap.enabled() and lastCap.enabled())
        {
            rc = disableInterrupts(pDevIns);
            AssertLogRelReturn(RT_SUCCESS(rc), rc);
            rc = activateInterrupts(pDevIns, IrqType::VFIO_INTX);
            AssertLogRelReturn(RT_SUCCESS(rc), rc);
        }
        else if (updatedCap.enabled())
        {
            rc = disableInterrupts(pDevIns);
            AssertLogRelReturn(RT_SUCCESS(rc), rc);
            rc = activateInterrupts(pDevIns, IrqType::VFIO_MSIX, updatedCap.tableSize());
            AssertLogRelReturn(RT_SUCCESS(rc), rc);
        }
    }

    return writeConfigSpace(u32Value, cb, uAddress);
}

int VfioDevice::mmioAccessHandler(PPDMDEVINS /*pDevIns*/, PCIBarRegion& barRegion, RTGCPHYS barOffset, void* pv, unsigned cb, bool writeAccess)
{
    if (msixCapabilityIterator)
    {
        MSIXCapabilityDescriptor cap {*msixCapabilityIterator};

        if (cap.getBarIndex() == barRegion.iRegion and barOffset >= cap.getTableOffset()
            and barOffset < cap.getTableOffset() + (sizeof(MSIXTableEntry) * cap.tableSize()))
        {
            AssertLogRelMsgReturn(cap.tableSize() == aMsixTable.size(),
                ("VFIO: The MSIX table size mismatches the hardware table size. Assumed table size: %hu Hardware table size: %hu\n",
                    aMsixTable.size(),
                    cap.tableSize()),
                VERR_NOT_SUPPORTED);
            uint64_t msixTableEntryOffset {barOffset - cap.getTableOffset()};
            void* shadowMsixTableOffset {reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(aMsixTable.data()) + msixTableEntryOffset)};
            if (writeAccess)
            {
                /*
                 * We need to shadow the MSIX table as explained in the else path, but we need to provide the VFIO device
                 * with data written to the VFIO device.
                 * Because of this we need to write the data through.
                 */
                std::memcpy(shadowMsixTableOffset, pv, cb);
            }
            else
            {
                std::memcpy(pv, shadowMsixTableOffset, cb);
                /**
                 * The VFIO Device returns invalid data in case of a read from the MSIX table.
                 * Because of this, we need to shadow the table and return early without reading
                 * from the actual VFIO device here.
                 */
                return VINF_SUCCESS;
            }
        }
    }

    if (writeAccess)
    {
        return writeToDevice(pv, cb, barRegion.offset + barOffset);
    }
    else
    {
       return readFromDevice(pv, cb, barRegion.offset + barOffset);
    }

}

int VfioDevice::interceptConfigSpaceAccesses(PPDMDEVINS pDevIns)
{
    int rc {VINF_SUCCESS};

    auto configSpaceWrite = [](PPDMDEVINS pDev, PPDMPCIDEV pPciDev_, uint32_t uAddress, unsigned cb, uint32_t u32Value) -> VBOXSTRICTRC
    {
        PVFIODEV pThis {PDMDEVINS_2_DATA(pDev, PVFIODEV)};
        writePciConfigSpaceShadow(pPciDev_, uAddress, cb, u32Value);
        return pThis->configSpaceWriteHandler(pDev, uAddress, cb, u32Value);
    };

    rc = PDMDevHlpPCIInterceptConfigAccesses(pDevIns, pPciDev, vfioConfigSpaceRead, configSpaceWrite);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    return rc;
}

int VfioDevice::init(PPDMDEVINS pDevIns, std::filesystem::path sysfsPath)
{
    int rc {VINF_SUCCESS};

    rc = initializeVfio(pDevIns, sysfsPath);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    rc = initializePci(pDevIns);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    vfio_group_status groupStatus;
    groupStatus.argsz = sizeof(groupStatus);

    rc = vfioControl(pDevIns, vfioGroupFd, VFIO_GROUP_GET_STATUS, "VFIO: Unable to retrieve VFIO group status\n" , &groupStatus);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    vfio_device_info deviceInfo;
    deviceInfo.argsz = sizeof(deviceInfo);

    rc = deviceControl(pDevIns, VFIO_DEVICE_GET_INFO, "VFIO: Unable to retrieve VFIO Device information\n", &deviceInfo);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    LogRel(("VFIO: Successfully opened VFIO Device: Group Status Flags: %#x Device Flags: %#x, Num BARs: %u, Num IRQ's %u \n",
           groupStatus.flags, deviceInfo.flags, deviceInfo.num_regions, deviceInfo.num_irqs));

    rc = initializeMemoryRegions(pDevIns, deviceInfo);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    rc = initializeInterrupts(pDevIns);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    rc = interceptConfigSpaceAccesses(pDevIns);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    return rc;
}

int VfioDevice::registerDmaRange(PVM pVM, RTGCPHYS startGCPhys, RTGCPHYS endGCPhys)
{
    AssertLogRelReturn(RT_VALID_ALIGNED_PTR(startGCPhys, PAGE_SIZE) || startGCPhys == 0, VERR_INVALID_POINTER);
    AssertLogRelReturn(RT_VALID_ALIGNED_PTR(endGCPhys + 1 , PAGE_SIZE), VERR_INVALID_POINTER);

    auto registerDma = [](uintptr_t hva, RTGCPHYS gpa, uint64_t size, int containerFd) -> int
    {
        struct vfio_iommu_type1_dma_map dma;
        dma.argsz = sizeof(dma);
        dma.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;
        dma.vaddr = hva;
        dma.iova = gpa;
        dma.size = static_cast<uint64_t>(size);
        int rc  {ioctl(containerFd, VFIO_IOMMU_MAP_DMA, &dma)};
        AssertLogRelMsgReturn(rc == 0, ("VFIO: Could not acquire enough memory to map the Guest Physical address space. Adapt your ulimit\n"), VERR_NO_MEMORY);

        return VINF_SUCCESS;
    };

    uintptr_t continousPagesStart {0};
    RTGCPHYS continousPagesStartGCPhys {0};
    uintptr_t continousPagesLast {0};
    uint64_t continousRangeSize {0};

    auto reset = [&]()
    {
        continousRangeSize = 0;
        continousPagesStart = 0;
        continousPagesLast = 0;
        continousPagesStartGCPhys = 0;
    };

    for (RTGCPHYS pageAddress {startGCPhys}; pageAddress < endGCPhys; pageAddress += PAGE_SIZE)
    {
        void* ptr;
        if (RT_SUCCESS(PGMR3PhysTlbGCPhys2Ptr(pVM, pageAddress, true, &ptr)))
        {
            uintptr_t hcVirt(reinterpret_cast<uintptr_t>(ptr));
            if (continousRangeSize > 0 and continousPagesLast + PAGE_SIZE == hcVirt)
            {
                continousPagesLast = hcVirt;
                continousRangeSize += PAGE_SIZE;
                continue;
            }
            else if (continousRangeSize != 0)
            {
                int rc {registerDma(continousPagesStart, continousPagesStartGCPhys, continousRangeSize, vfioContainerFd)};
                AssertLogRelReturn(RT_SUCCESS(rc), rc);
            }
            continousPagesStart = hcVirt;
            continousPagesLast = hcVirt;
            continousPagesStartGCPhys = pageAddress;
            continousRangeSize = PAGE_SIZE;
        }
        else if (continousRangeSize != 0)
        {
            int rc {registerDma(continousPagesStart, continousPagesStartGCPhys, continousRangeSize, vfioContainerFd)};
            AssertLogRelReturn(RT_SUCCESS(rc), rc);
            reset();
        }
    }

    if (continousRangeSize != 0)
    {
        int rc {registerDma(continousPagesStart, continousPagesStartGCPhys, continousRangeSize, vfioContainerFd)};
        AssertLogRelReturn(RT_SUCCESS(rc), rc);
        reset();
    }

    return VINF_SUCCESS;
}

int VfioDevice::initializeDma(PPDMDEVINS pDevIns)
{
    auto pVM {PDMDevHlpGetVM(pDevIns)};
    uint32_t ramRangeCount {PGMR3PhysGetRamRangeCount(pVM)};

    for (uint32_t i {0u}; i < ramRangeCount; ++i)
    {
        RTGCPHYS start, end;
        bool isMMioRange;
        if (RT_SUCCESS(PGMR3PhysGetRange(pVM, i, &start, &end, nullptr, &isMMioRange)) and not isMMioRange)
        {
            int rc {registerDmaRange(pVM, start, end)};
            AssertLogRelReturn(RT_SUCCESS(rc), rc);
        }

    }

    return VINF_SUCCESS;
}

int VfioDevice::terminate(PPDMDEVINS pDevIns)
{
    int rc {VINF_SUCCESS};

    rc = disableInterrupts(pDevIns);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    exit = true;
    rc = RTThreadWaitNoResume(hIrqDeliveryThread, RT_INDEFINITE_WAIT, nullptr);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);
    exit = false;

    aMsixTable.clear();
    msiCapabilityIterator = std::nullopt;
    msixCapabilityIterator = std::nullopt;

    rc = close(vfioDeviceFd);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);
    rc = close(vfioGroupFd);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);
    rc = close(vfioContainerFd);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    vfioDeviceFd = -1;
    vfioGroupFd = -1;
    vfioContainerFd = -1;


    return rc;
}

void VfioDevice::tryHandleBarInterception(PPDMDEVINS pDevIns, uint32_t pciConfigCommandValue)
{
    if (pciConfigCommandValue & VBOX_PCI_COMMAND_IO and not pciConfigIODecodingEnabled)
    {
        pciConfigIODecodingEnabled = true;
    }
    else
    {
        pciConfigIODecodingEnabled = false;
    }

    if (pciConfigCommandValue & VBOX_PCI_COMMAND_MEMORY and not pciConfigMemoryDecodingEnabled)
    {
        pciConfigMemoryDecodingEnabled = true;
    }
    else
    {
        pciConfigMemoryDecodingEnabled = false;
    }

    if (pciConfigIODecodingEnabled or pciConfigMemoryDecodingEnabled)
    {
        for (auto i {0u}; i < VBOX_PCI_MAX_BARS; ++i)
        {
            const auto barInfo {getBarInfo(i)};
            if (not (pciBars[i].hRegion == NIL_IOMMMIOHANDLE or pciBars[i].hRegion == 0))
            {
                if (barInfo.isIoBar() and pciConfigIODecodingEnabled)
                {
                    registerPCIBar<0>(PDMDevHlpIoPortMap, PDMDevHlpIoPortUnmap, pDevIns, pciBars[i], barInfo.getBarAddress());
                }
                else if (barInfo.isMmioBar() and pciConfigMemoryDecodingEnabled)
                {
                    registerPCIBar<NIL_RTGCPHYS>(PDMDevHlpMmioMap, PDMDevHlpMmioUnmap, pDevIns, pciBars[i], barInfo.getBarAddress());
                }
            }
        }
    }
}

const PCIBar VfioDevice::getBarInfo(unsigned barNumber)
{
    uint64_t barOffset { VBOX_PCI_BASE_ADDRESS_0 + barNumber * sizeof(uint32_t)};
    uint64_t barValue;

    readConfigSpace(barValue, sizeof(barValue), barOffset);

    PCIBar bar {barValue};

    if (bar.is64BitBar()) {
        return bar;
    }

    return {barValue & std::numeric_limits<uint32_t>::max()};
}
