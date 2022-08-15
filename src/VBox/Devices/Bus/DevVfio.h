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

#pragma once

#include <cyberus/pci.h>

#include <VBox/err.h>
#include <VBox/pci.h>
#include <VBox/vmm/pdmdev.h>

#include <linux/vfio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <vector>

class VfioDevice
{
public:
    /*
     * The IRQ Type information, required for the interrupt handler.
     */
    enum class IrqType
    {
        VFIO_INTX = VFIO_PCI_INTX_IRQ_INDEX,
        VFIO_MSI  = VFIO_PCI_MSI_IRQ_INDEX,
        VFIO_MSIX = VFIO_PCI_MSIX_IRQ_INDEX,
        VFIO_NONE,
    };

    /**
     * Interrupt Handler function
     *
     * \param pDevIns The PCI Device Instance
     *
     * \return VBox status code
     */
    int handleInterrupts(PPDMDEVINS pDevIns);

    /**
     * Initialize the VfioDevice
     *
     * \param pDevIns The PCI Device Instance
     *
     * \return VBox status code
     */
    int init(PPDMDEVINS pDevIns, std::filesystem::path sysfsPath);

    /**
     * Initialize DMA
     * As the ram preallocation is required to initialize the DMA regions for the
     * VFIO device, the function have to be called **after** pgmR3RamPreAlloc
     *
     * \param pDevIns The PCI Device Instance
     *
     * \return VBox status code
     */
    int initializeDma(PPDMDEVINS pDevIns);

    /**
     *  Terminates the VFIO device and closes the file descriptors
     *
     *  \param pDevIns The PCI Device Instance
     *
     *  \return VBox status code
     */
    int terminate(PPDMDEVINS pDevIns);

    /**
     * Read from the Vfio Device file descriptor
     *
     * \param pData data to read
     * \param bytes count of bytes to read
     * \param uAddress address to read from
     *
     * \return VBOX status code
     */
    int readFromDevice(void* pData, unsigned bytes, uint64_t uAddress)
    {
        return handleDeviceAccess(pread64, pData, bytes, uAddress);
    }

    /**
     * Write to the Vfio Device file descriptor
     *
     * \param pData data to write
     * \param bytes count of bytes to write
     * \param uAddress address to write to
     *
     * \return VBOX status code
     */
    int writeToDevice(const void* pData, unsigned bytes, uint64_t uAddress)
    {
        return handleDeviceAccess(pwrite64, const_cast<void*>(pData), bytes, uAddress);
    }

    /**
     * Read from the actual PCI Config Space of the VFIO device
     *
     * \param data data to read
     * \param bytes count of bytes to read
     * \param uAddress address to read from
     *
     * \return VBOX status code
     */
    template <typename T>
    int readConfigSpace(T& data, unsigned bytes, uint64_t uAddress)
    {
        return readFromDevice(&data, bytes, mcfgOffset + uAddress);
    }

    /**
     * Write to the actual PCI Config Space of the VFIO device
     *
     * \param data data to write
     * \param bytes count of bytes to write
     * \param uAddress address to write to
     *
     * \return VBOX status code
     */
    template <typename T>
    int writeConfigSpace(T& data, unsigned bytes, uint64_t uAddress)
    {
        return writeToDevice(&data, bytes, mcfgOffset + uAddress);
    }

private:
    using LockGuard = std::lock_guard<std::mutex>;

    /**
     * The interrupt information structure is a bookkeeping structure for the
     * interrupt handling.
     * It maps the interrupt event file descriptor to an internal interrupt
     * index and contains the interrupt type (INTX, MSI, MSIX) for the handler thread.
     */
    struct InterruptInformation
    {
        int fd;
        uint32_t index;

        bool operator==(const InterruptInformation& o) const
        {
            return o.fd == fd and o.index == index;
        }
    };

    template<typename FN>
    int handleDeviceAccess(FN& fn, void* data, unsigned bytes, uint64_t uAddress)
    {
        AssertLogRelMsgReturn(vfioDeviceFd > 0, ("The Vfio Device is not open \n"), VERR_GENERAL_FAILURE);
        auto rc {fn(vfioDeviceFd, data, bytes, uAddress)};

        return rc < 0 ? VERR_ACCESS_DENIED : VINF_SUCCESS;
    }

    /**
     * Initialize VFIO container and device
     *
     * \param pDevIns The PCI Device Instance
     * \param sysfsPath path to the sysfs device
     *
     * \return VBox status code
     */
    int initializeVfio(PPDMDEVINS pDevIns, std::filesystem::path sysfsPath);

    /**
     * Initialize the VirtualBox PCI Device Information
     *
     * \param pDevIns The PCI Device Instance
     *
     * \return VBox status code
     */
    int initializePci(PPDMDEVINS pDevIns);

    /**
     * Initialize VFIO Memory Regions
     *
     * Such regions are either PCI Bar regions or VFIO specific regions to
     * provide device Information or device state such as graphics output
     *
     * \param pDevIns The PCI Instance Data
     * \param deviceInfo The vfio device information
     *
     * \return VBox status code
     */
    int initializeMemoryRegions(PPDMDEVINS pDevIns, vfio_device_info& deviceInfo);

    /**
     * Initialize interrupt handling
     *
     * \param pDevIns The PCI Device Instance
     *
     * \return VBox status code
     */
    int initializeInterrupts(PPDMDEVINS pDevIns);

    /**
     * Activate the corresponding interrupt type. The current interrupt type must be disabled before.
     *
     * \param pDevIns The PCI Device Instance
     * \param vfuiIrqIndexType the irq type that should be activated
     * \param irqCount count of irqs to register
     *
     * \return VBox status code
     */
    int activateInterrupts(PPDMDEVINS pDevIns, const IrqType vfioIrqIndexType, uint32_t irqCount = 1);

    /**
     * Disable the corresponding interrupt type
     *
     * \param pDevIns The PCI Device Instance
     *
     * \return VBox status code
     */
    int disableInterrupts(PPDMDEVINS pDevIns);

    /**
     * Inject a MSI
     *
     * \param pDevIns The PCI Device Instance
     * \param irqInfo The interrupt information of the pending interrupt
     *
     * \return VBOX status code
     */
    int injectMsi(PPDMDEVINS pDevIns, InterruptInformation& irqInfo);

    /**
     * Inject a MSIX
     *
     * \param pDevIns The PCI Device Instance
     * \param irqInfo The interrupt information of the pending interrupt
     *
     * \return VBOX status code
     */
    int injectMsix(PPDMDEVINS pDevIns, InterruptInformation& irqInfo);

    /**
     * The configuration space write handler.
     *
     * \param pDevIns The PCI Device Instance
     * \param uAddress offset in the configuration space to write
     * \param cb count of bytes to write
     * \param u32Value The value to write
     *
     * \return VBox status code
     */
    int configSpaceWriteHandler(PPDMDEVINS pDevIns, uint32_t uAddress, unsigned cb, uint32_t u32Value);

    /**
     * The memory mapped IO access handler function.
     *
     * \param pDevIns The PCI Device Instance
     * \param barRegion The reference to the PCI Bar region
     * \param barOffset The offset in the PCI bar
     * \param pv The pointer to the data to be read
     * \param cb The size of the data to be read
     * \param write Indicator of access direction
     *
     * \return Vbox Status code
     */
    int mmioAccessHandler(PPDMDEVINS pDevIns, PCIBarRegion& barRegion, RTGCPHYS barOffset, void* pv, unsigned cb, bool writeAccess);

    /**
     * Start inteception of Guest VM PCI Config Space Accesses
     *
     * \param pDevIns the VBox Device Instance
     *
     * \return VBox status code
     */
    int interceptConfigSpaceAccesses(PPDMDEVINS pDevIns);

    /**
     * Register a Guest Physical Memory range at the vfio container
     *
     * \param pVM Pointer to the VM structure
     * \param startGCPhys Guest physical address of the start of the ram range
     * \param endGCPhys Guest physical address of the end of the region
     *
     * \return VBOX status code
     */
    int registerDmaRange(PVM pVM, RTGCPHYS startGCPhys, RTGCPHYS endGCPhys);

    /**
     * Try handling of PCI Bar interception
     *
     * \param pDevIns PDM Device Instance
     * \param pciConfigCommandValue value of the command register of the PCI config space
     */
    void tryHandleBarInterception(PPDMDEVINS pDevIns, const uint32_t pciConfigCommandValue);

    /**
     * Register a PCI Bar at the corresponding subsystem (IO or MMIO).
     *
     * \param mapFn function used to map the Bar at the corrseponding Subsystem
     * \param unmapFn function to unmap the old Bar region if the bar was present before
     * \param pDevIns the PDM Device Instance Data structure
     * \param barRegion the region bookkeeping data structure
     * \param mapAddress the new address of the Bar
     */
    template <uint64_t INVALID_ELEM, typename MapFN, typename UnmapFN>
    void registerPCIBar(MapFN& mapFn, UnmapFN& unmapFn, PPDMDEVINS pDevIns, PCIBarRegion& barRegion, uint64_t mapAddress) {
        LogRel(("VFIO: RegisterBar %#llx \n", mapAddress));
        if (barRegion.address == mapAddress)
        {
            return;
        }

        if (barRegion.address != INVALID_ELEM)
        {
            unmapFn(pDevIns, barRegion.hRegion);
            barRegion.address = INVALID_ELEM;
        }

        mapFn(pDevIns, barRegion.hRegion, mapAddress);
        barRegion.address = mapAddress;
    }

    /**
    * Read the Bar value from the PCI config space
    *
    * \param barNumber The bar which value should be read
    *
    * \return PCIBar information
    */
    const PCIBar getBarInfo(unsigned barNumber);

    /**
     * Ioctl wrapper with meaningfull error return
     * \param fd file descriptor to interact with
     * \param request ioct request number
     * \param errorStr string to set in the log in case of an error
     * \param args variadic template args for the ioctl
     *
     * \return Vbox error code
     */
    template <typename ...ARGS>
    int vfioControl(PPDMDEVINS pDevIns, int fd, unsigned long request, const char* errorString, ARGS&& ...args)
    {
        if (ioctl(fd, request, std::forward<ARGS>(args) ...) < 0)
        {
            return PDMDEV_SET_ERROR(pDevIns, VERR_INVALID_PARAMETER, errorString);
        }

        return VINF_SUCCESS;
    }

    /**
     * Ioctl device wrapper for accesses on the vfio device file descriptor
     *
     * \param pDevIns the VBox Device Instance
     * \param request ioct request number
     * \param errorStr string to set in the log in case of an error
     * \param args variadic template args for the ioctl
     *
     * \return VBOX status code
     */
    template <typename ...ARGS>
    int deviceControl(PPDMDEVINS pDevIns, unsigned long request, const char* errorString, ARGS&& ...args)
    {
        AssertLogRelMsgReturn(vfioDeviceFd > 0, ("The Vfio Device is not open \n"), VERR_GENERAL_FAILURE);
        return vfioControl(pDevIns, vfioDeviceFd, request, errorString, std::forward<ARGS>(args)...);
    }

    /** Vfio File descriptors */
    int vfioContainerFd{-1};
    int vfioGroupFd{-1};
    int vfioDeviceFd {-1};

    /** PCI device members. */
    PPDMPCIDEV pPciDev;
    uint64_t mcfgOffset; ///< The offset to the PCI Config Space Page in the vfio device
    std::atomic<bool> pciConfigMemoryDecodingEnabled {false}; ///< The PCI Memory decoding indicator
    std::atomic<bool> pciConfigIODecodingEnabled {false}; ///< The PCI IO decoding indicator
    std::array<PCIBarRegion, VBOX_PCI_MAX_BARS> pciBars;

    /** IRQ handling */
    RTTHREAD hIrqDeliveryThread;
    // Even if only one INTX interrupt is supported handling it as a vector reduce the code complexity by a lot.
    std::vector<InterruptInformation> aIrqInformation;
    std::vector<MSIXTableEntry> aMsixTable;
    IrqType activeInterruptType {IrqType::VFIO_NONE};
    std::mutex irqDisable;

    std::optional<CapabilityList::CapabilityIterator> msiCapabilityIterator;
    std::optional<CapabilityList::CapabilityIterator> msixCapabilityIterator;

    std::atomic<bool> exit{false};
};
typedef VfioDevice VFIODEV;

typedef VFIODEV *PVFIODEV;
