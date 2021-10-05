#pragma once

#include <VBox/pci.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmpcidev.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <limits>
#include <optional>
#include <type_traits>

typedef struct PCIBarRegion
{
    static_assert(std::is_same<IOMMMIOHANDLE, IOMIOPORTHANDLE>::value,
                  "IOMMMIOHANDLE and IOMIOPORTHANDLE have different types now please extend this struct for the "
                  "support of both!");
    IOMMMIOHANDLE hRegion;
    uint8_t iRegion;  ///< The bar index e.G Bar0.
    uint64_t offset;  ///< The bar offset into the vfio device.
    uint64_t size;    ///< The size of the bar.
    RTGCPHYS address; ///< Base address of the bar.
} PCIBARREGION;

typedef PCIBARREGION* PPCIBARREGION;

class PCIBar
{
public:
    PCIBar() = delete;

    PCIBar(uint64_t value_) : value(value_)
    {
        if (not is64BitBar()) {
            value &= std::numeric_limits<uint32_t>::max();
        }
    }

    bool isIoBar() const { return (value & PCI_BAR_TYPE_MASK) == PCI_ADDRESS_SPACE_IO; }
    bool isMmioBar() const { return (value & PCI_BAR_TYPE_MASK) == PCI_ADDRESS_SPACE_MEM; }
    bool is64BitBar() const { return (value & PCI_BAR_ADDRESS_MASK) == PCI_ADDRESS_SPACE_BAR64; }

    uint64_t getBarAddress() const
    {
        if (isIoBar()) {
            return value & ~PCI_CFG_IO_FLAGS_MASK;
        } else if (isMmioBar()) {
            return value & ~PCI_CFG_MMIO_FLAGS_MASK;
        }

        return 0;
    }

private:
    static constexpr uint64_t PCI_CFG_IO_FLAGS_MASK {0x3};
    static constexpr uint64_t PCI_CFG_MMIO_FLAGS_MASK {0xf};
    static constexpr uint64_t PCI_BAR_TYPE_MASK {0x1};
    static constexpr uint64_t PCI_BAR_ADDRESS_MASK {0x4};

    uint64_t value;
};

/**
 * Describes the generic part of a capability descriptor.
 */
struct __attribute__((__packed__)) CapabilityDescriptor
{
    uint8_t capID {0};
    uint8_t nextPtr {0};
};
static_assert(sizeof(CapabilityDescriptor) == 0x2,
              "The Capability Descriptor has incorrect size, did you forgot __attribute__ ((__packed__))");

/*
 * Read a specified type from the pci configuration space.
 *
 * \param offset offset into the pci configuration space
 * \param readFn The function that should be used to read from the pci
 *         configuration space.
 *
 * \return An object of the by template parameter specified type
 */
template <typename T>
T readType(PPDMDEVINS pDevIns, uint32_t offset, PFNPCICONFIGREAD readFn)
{
    T t;

    char* ptr {reinterpret_cast<char*>(&t)};

    // TODO: can be optimized to minimize cfg space read accesses as we could read 4 bytes at once
    for (size_t i = 0; i < sizeof(T); i++) {
        uint8_t data;
        readFn(pDevIns, nullptr, offset + i, 1u, reinterpret_cast<uint32_t*>(&data));
        memcpy(ptr + i, &data, sizeof(data));
    }

    return t;
}

/*
 * The pci configuration space capability list abstraction
 *
 * The abstraction makes an easy iteration of capabilities in the pci config space possible
 * Additionally, a conversion from the basic CapabilityDescriptor to a special capability is possible
 */
class CapabilityList
{
public:
    class CapabilityIterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = CapabilityDescriptor;
        using difference_type = size_t;
        using pointer = CapabilityDescriptor*;
        using reference = CapabilityDescriptor&;

        CapabilityIterator(uint32_t capListPtr, PFNPCICONFIGREAD readFn_, PPDMDEVINS pDevIns_)
            : offset(capListPtr), pDevIns(pDevIns_), readFn(readFn_)
        {}

        CapabilityIterator(const CapabilityIterator& o) : offset(o.offset), pDevIns(o.pDevIns), readFn(o.readFn) {}

        value_type operator*() const
        {
            assert(offset);
            return readType<CapabilityDescriptor>(pDevIns, offset, readFn);
        }

        CapabilityIterator& operator++()
        {
            assert(offset);
            static constexpr uint32_t CAP_PTR_MASK {0x3};
            auto capDescriptor {readType<CapabilityDescriptor>(pDevIns, offset, readFn)};
            offset = capDescriptor.nextPtr & (~CAP_PTR_MASK);
            return *this;
        }

        bool operator==(const CapabilityIterator& o) const { return offset == o.offset and readFn == o.readFn; }

        bool operator!=(const CapabilityIterator& o) const { return not operator==(o); }

        template <typename T>
        T getCapability() const
        {
            assert(offset);
            return readType<T>(pDevIns, offset, readFn);
        }

        uint32_t getOffset() const { return offset; }

    private:
        uint32_t offset;
        PPDMDEVINS pDevIns;
        PFNPCICONFIGREAD readFn;
    };

    CapabilityList(PFNPCICONFIGREAD readFn_, PPDMDEVINS pDevIns_ = nullptr) : pDevIns(pDevIns_), readFn(readFn_)
    {
        if (enabled()) {
            readFn(pDevIns, nullptr, VBOX_PCI_CAPABILITY_LIST, PCI_CAPABILITY_LIST_PTR_SIZE, &capListPtr);
        }
    }

    /**
     * The function checks if the PCI device has support for capabilities
     *
     * \param pciStatus The value of the status register of the pci config space.
     */
    bool enabled()
    {
        static constexpr uint32_t PCI_STATUS_REGISTER_SIZE {0x2};
        uint32_t pciStatus {0};

        auto rc {readFn(pDevIns, nullptr, VBOX_PCI_STATUS, PCI_STATUS_REGISTER_SIZE, &pciStatus)};

        return RT_SUCCESS(rc) ? (pciStatus & VBOX_PCI_STATUS_CAP_LIST) : false;
    }

    CapabilityIterator begin() { return {capListPtr, readFn, pDevIns}; }
    CapabilityIterator end() { return {0x0, readFn, pDevIns}; }

    std::optional<CapabilityIterator> getCapabilityIterator(uint8_t capId)
    {
        if (not enabled()) {
            return std::nullopt;
        }
        auto it {std::find_if(begin(), end(), [capId](CapabilityDescriptor desc) { return desc.capID == capId; })};

        if (it != end()) {
            return it;
        }

        return std::nullopt;
    }

private:
    static constexpr uint8_t PCI_CAPABILITY_LIST_PTR_SIZE {sizeof(uint8_t)};
    PPDMDEVINS pDevIns;
    PFNPCICONFIGREAD readFn;
    uint32_t capListPtr {0x0};
};

/**
 * MSI capability descriptor  based on the PCI Local Bus Specification REV 3.0
 */
class __attribute__((__packed__)) MSICapabilityDescriptor : public CapabilityDescriptor
{
private:
    using CapabilityIterator = CapabilityList::CapabilityIterator;

    uint16_t msgControl {0};
    uint32_t msgAddress {0};

    union __attribute__((__packed__))
    {
        uint16_t msgData32Bit;
        struct
        {
            uint32_t msgAddressHigh;
            uint16_t msgData;
        } msi64bit;
        struct
        {
            uint16_t msgData;
            uint16_t reserved;
            uint32_t maskBits;
            uint32_t pendingBits;
        } msiPerVectorMasking;
        struct
        {
            uint32_t msgAddressHigh;
            uint16_t msgData;
            uint16_t reserved;
            uint32_t maskBits;
            uint32_t pendingBits;
        } msi64BitPerVectorMasking {0, 0, 0, 0, 0};
    };

public:
    MSICapabilityDescriptor() = default;
    // We possibly read too much data here, if no all features of the MSI subsystem are supported.
    // We accept this and treat the feature variables that are not activated in msgControl as garbage
    MSICapabilityDescriptor(const CapabilityIterator& iterator)
        : MSICapabilityDescriptor(iterator.getCapability<MSICapabilityDescriptor>())
    {}

    MSICapabilityDescriptor(const MSICapabilityDescriptor& o)
        : msgControl(o.msgControl), msgAddress(o.msgAddress), msi64BitPerVectorMasking(o.msi64BitPerVectorMasking)
    {}

    bool enabled() const { return msgControl & VBOX_PCI_MSI_FLAGS_ENABLE; }

    bool isPerVectorMaskable() const { return msgControl & VBOX_PCI_MSI_FLAGS_MASKBIT; }

    bool is64Bit() const { return msgControl & VBOX_PCI_MSI_FLAGS_64BIT; }

    uint8_t maxCount() const
    {
        static constexpr uint8_t PCI_MSI_FLAGS_QMASK_SHIFT {1u};
        return 1 << ((msgControl & VBOX_PCI_MSI_FLAGS_QMASK) >> PCI_MSI_FLAGS_QMASK_SHIFT);
    }

    uint8_t count() const
    {
        static constexpr uint8_t PCI_MSI_FLAGS_QSIZE_SHIFT {4u};
        return 1 << ((msgControl & VBOX_PCI_MSI_FLAGS_QSIZE) >> PCI_MSI_FLAGS_QSIZE_SHIFT);
    }

    uint64_t messageAddress() const
    {
        return is64Bit() ? static_cast<uint64_t>(msi64bit.msgAddressHigh) << 32 | msgAddress : msgAddress;
    }

    uint16_t messageData() const { return is64Bit() ? msi64bit.msgData : msgData32Bit; }

    bool isMasked(uint32_t vector) const
    {
        if (not isPerVectorMaskable()) {
            return false;
        }

        uint32_t maskBits {0};
        if (is64Bit()) {
            maskBits = msi64BitPerVectorMasking.maskBits;
        } else {
            maskBits = msiPerVectorMasking.maskBits;
        }

        return maskBits & (1u << vector);
    }

    std::optional<uint32_t> maskBitOffset() const
    {
        if (not isPerVectorMaskable()) {
            return std::nullopt;
        }

        return is64Bit() ? 0x10 : 0xC;
    }

    std::optional<uint32_t> pendingBitOffset() const
    {
        if (not isPerVectorMaskable()) {
            return std::nullopt;
        }

        return is64Bit() ? 0x14 : 0x10;
    }
};
static_assert(sizeof(MSICapabilityDescriptor) == 0x18,
              "The MSI Capability Descriptor has incorrect size, did you forgot __attribute__ ((__packed__))");

/**
 * MSIX capability descriptor  based on the PCI Local Bus Specification REV 3.0
 */
class __attribute__((__packed__)) MSIXCapabilityDescriptor : public CapabilityDescriptor
{
private:
    using CapabilityIterator = CapabilityList::CapabilityIterator;

    uint16_t msgControl {0};
    uint32_t tableOffset {0};
    uint32_t pendingBitArrayOffset {0};

    static constexpr uint32_t MSIX_TABLE_OFFSET_MASK {~0x7u};

public:
    MSIXCapabilityDescriptor() = default;
    MSIXCapabilityDescriptor(const MSIXCapabilityDescriptor& o)
        : msgControl(o.msgControl), tableOffset(o.tableOffset), pendingBitArrayOffset(o.pendingBitArrayOffset)
    {}

    MSIXCapabilityDescriptor(const CapabilityIterator& iterator)
        : MSIXCapabilityDescriptor(iterator.getCapability<MSIXCapabilityDescriptor>())
    {}

    bool enabled() const { return msgControl & VBOX_PCI_MSIX_FLAGS_ENABLE; }

    bool allMasked() const { return msgControl & VBOX_PCI_MSIX_FLAGS_FUNCMASK; }

    uint16_t tableSize() const
    {
        // According to the PCI Local Bus Specification REV 3.0
        // the MSIX Table size is encoded as N-1  in the bits 0 to 10
        // of message control, so we need to add 1 to
        // get the actual table size.
        static constexpr uint16_t MSIX_TABLE_SIZE_MASK {0x7ff};
        return (msgControl & MSIX_TABLE_SIZE_MASK) + 1;
    }

    uint32_t getTableOffset() const { return tableOffset & MSIX_TABLE_OFFSET_MASK; }

    uint32_t getBarIndex() const { return tableOffset & ~MSIX_TABLE_OFFSET_MASK; }
};
static_assert(sizeof(MSIXCapabilityDescriptor) == 0xc,
              "The MSIX Capability Descriptor has incorrect size, did you forgot __attribute__ ((__packed__))");

/**
 * MSIX table entry based on the PCI Local Bus Specification REV 3.0
 */
class __attribute__((__packed__)) MSIXTableEntry
{
private:
    uint32_t msgAddressLow {0};
    uint32_t msgAddressHigh {0};
    uint32_t msgData {0};
    uint32_t vectorCtrl {0};

public:
    uint64_t messageAddress() const { return static_cast<uint64_t>(msgAddressHigh) << 32 | msgAddressLow; }

    uint32_t messageData() const { return msgData; }
};
static_assert(sizeof(MSIXTableEntry) == 0x10,
              "The MSIX Capability Descriptor has incorrect size, did you forgot __attribute__ ((__packed__))");

/**
 * This Function writes data to the PCI configuration space of VirtualBox
 * The function is required for pass through or semi emulated devices to handle pci capabilities such as
 * MSI support by VirtualBox.
 *
 * /param pPciDev The PCI device to which PCI configuration space should be written.
 * /param offset the Offset into the Configuration Space. Refer to PCI Local Bus Specification REV 3.0 Figure 6-1 for an
 * overview, /param cb The byte count to write, /param value The Value to write.
 */
inline void writePciConfigSpaceShadow(PPDMPCIDEV pPciDev, uint32_t offset, unsigned cb, uint64_t value)
{
    if (pPciDev) {
        switch (cb) {
        case sizeof(uint8_t): PDMPciDevSetByte(pPciDev, offset, value); break;
        case sizeof(uint16_t): PDMPciDevSetWord(pPciDev, offset, value); break;
        case sizeof(uint32_t): PDMPciDevSetDWord(pPciDev, offset, value); break;
        case sizeof(uint64_t): PDMPciDevSetQWord(pPciDev, offset, value); break;
        default:
            AssertLogRelMsgFailed(("SuperNova-PCI: Could not write PCI Config Space Shadow due to an unsupported byte "
                                   "count of %u bytes.\n",
                                   cb));
        };
    }
}

/**
 * Register the MSI(X) system for the pass through pci device in the VirtualBox PCI Subsystem.
 *
 * /param pDevIns The VirtualBox PCI Device instance data
 * /param msiCapabilityIterator The MSI Capability iterator of the pci device.
 * /param msixCapabilityIterator The MSIX Capability iterator of the pci device.
 */

inline int registerMsi(PPDMDEVINS pDevIns, std::optional<CapabilityList::CapabilityIterator> msiCapabilityIterator,
                       std::optional<CapabilityList::CapabilityIterator> msixCapabilityIterator)
{
    PDMMSIREG msiReg;
    RT_ZERO(msiReg);

    if (msiCapabilityIterator) {
        MSICapabilityDescriptor msiCap {*msiCapabilityIterator};

        msiReg.cMsiVectors = msiCap.maxCount();
        msiReg.iMsiCapOffset = msiCapabilityIterator->getOffset();
        msiReg.iMsiNextOffset = msiCap.nextPtr;
        msiReg.fMsi64bit = msiCap.is64Bit();
        msiReg.fMsiNoMasking = not msiCap.isPerVectorMaskable();
    }

    if (msixCapabilityIterator) {
        MSIXCapabilityDescriptor msixCap {*msiCapabilityIterator};
        msiReg.cMsixVectors = msixCap.tableSize();
        msiReg.iMsixCapOffset = msixCapabilityIterator->getOffset();
        msiReg.iMsixNextOffset = msixCap.nextPtr;
        msiReg.iMsixBar = msixCap.getBarIndex();
    }

    if (msiCapabilityIterator or msixCapabilityIterator) {
        return PDMDevHlpPCIRegisterMsi(pDevIns, &msiReg);
    }

    /*
     * If we end up here, the device either do not support MSI or MSIX or the Device Capabilitys are not present.
     */
    return VINF_SUCCESS;
}
