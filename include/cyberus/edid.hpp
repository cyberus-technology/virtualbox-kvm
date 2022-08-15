#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <string>

/*
 * The Extended Display Identification Data structure Version 1.4 Structure Definitions
 * The EDID structures are implemented based on the VESA-EEDID-A2 Specification
 * from https://glenwing.github.io/docs/VESA-EEDID-A2.pdf
 */

/*
 * The EDID Standard Timings Definition (Section 3.9 VESA EEDID A2 Specification)
 * For a list of Standard Codes please refer to the VESA DMT 1.13 Specification.
 * Link: https://glenwing.github.io/docs/VESA-DMT-1.13.pdf
 * The horizontal addressable pixels are calculated by the following formula:
 * horizontalPixels = (pixelcount / 8) - 31;
 */
struct __attribute__((__packed__)) EdidStandardTiming
{
    uint8_t horizontalPixels;

    /*
     * The aspectRatio is stored in bits 6 and 7
     */
    enum __attribute__((__packed__)) AspectRatio : uint8_t
    {
        AR_16_10 = 0x0,
        AR_4_3 = 0x1 << 6,
        AR_5_4 = 0x2 << 6,
        AR_16_9 = 0x3 << 6
    };

    /*
     * The refresh rate is stored in the lower 5 bits and stored using this
     * formula: refreshRate = <value> + 60 HZ
     * Example 85 HZ: 0x19 + 60 = 85 hz; refresh rate = 0x19;
     */
    uint8_t aspectRatioAndRefreshRate;

    EdidStandardTiming() = default;

    EdidStandardTiming(uint32_t horizontalPixels_, AspectRatio ratio, uint8_t refreshRate)
    {
        assert(refreshRate >= 60);
        horizontalPixels = (horizontalPixels_ / 8) - 31;
        aspectRatioAndRefreshRate = (ratio & 0xc0) | ((refreshRate - 60) & 0x3f);
    }
};

/*
 * The Detailed Timing Descriptor (Section 3.10.2)
 *
 * The default values are extracted from a running GVT with its default EDID
 */
struct __attribute__((__packed__)) EdidDetailedTimingDescriptor
{
    uint16_t pixelClock {0};
    uint8_t hVideoLow {0x80};
    uint8_t hBlankingLow {0xa0};
    uint8_t hVideoBlankingHigh {0x70};
    uint8_t vVideoLow {0xb0};
    uint8_t vVBlankingLow {0x23};
    uint8_t vVideoBlankingHigh {0x40};
    uint8_t hFrontPorchLow {0x30};
    uint8_t hSyncPulseWidthLow {0x20};
    uint8_t vFrontPorchSyncPulseWidthlow {0x36};
    uint8_t vhFrontPorchSyncPulseHigh {0x00};
    uint8_t hVideoImageSizeLow {0x06};
    uint8_t vVideoImageSizeLow {0x44};
    uint8_t vhVideoImageSizeHigh {0x21};
    uint8_t horizontalBorder {0x00};  // (Section 3.12)
    uint8_t verticalBorder {0x00};    // (Section 3.12)
    uint8_t signalDefinitions {0x1a}; // (Table 3.22)
};

static_assert(sizeof(EdidDetailedTimingDescriptor) == 18,
              "The size of the EdidDetailedTimingDescriptor must be 18 bytes!");

/*
 * The Display Descriptor Definitions (Section 3.10.3)
 */
struct __attribute__((__packed__)) EdidDisplayDescriptorDefinitions
{
    uint16_t reserved {0};
    uint8_t reserved1 {0};

    enum __attribute__((__packed__)) : uint8_t
    {
        DisplayProductName = 0xFC,
        DisplayRangeLimits = 0xFD,
        DisplaySerialNumber = 0xFF,
    } tag {DisplayProductName};

    union __attribute__((__packed__))
    {
        uint8_t reserved2 {0};    // for Serial Number and Product Name
        uint8_t rangeLimitOffset; // for Display Range Limits (Table 3.26)
    };

    union __attribute__((__packed__))
    {
        char productName[13] {"CBS Display"};
        char serialNumber[13];
        struct __attribute__((__packed__))
        {
            uint8_t minimumVerticalRate;
            uint8_t maximumVerticalRate;
            uint8_t minimumHorizontalRate;
            uint8_t maximumHorizontalRate;
            uint8_t maximumPixelClock;
            uint8_t videoTimingSupportFlags;
            uint8_t videoTimingDataOrLineFeed; // (Table 3.27 and 3.28
            uint8_t videoTimingDataOrSpace[6];
        } rangeLimitsTimingDescriptor;
    };
};

static_assert(sizeof(EdidDisplayDescriptorDefinitions) == 18,
              "The size of the EdidDisplayDescriptorDefinitions must be 18 bytes!");

/*
 * The EDID Base Block.
 * All definitions in this structure are based on the VESA-EEDID-A2 Specification.
 * The structure is implemented Based on Table 3.1 in Section 3.i EDID Format Overview
 */
struct __attribute__((__packed__)) EdidBaseBlock
{
    uint64_t header {0x00ffffffffffff00}; // (Section 3.3)

    /*
     * Vendor and Product ID (Section 3.4)
     */
    uint16_t manufacturerName {0x530c};   // "CBS" (Section 3.4.1)
    uint16_t productCode {0x1};           // (Section 3.4.2)
    uint32_t serialNumber {0x1337};       // (Section 3.4.3)
    uint16_t manufacturingDates {0x262d}; // (Section 3.4.4) ->(WW45 2022)
                                          //
    /*
     * EDID Version and Revision (Section 3.5)
     */
    uint8_t version {0x1};
    uint8_t revision {0x4};

    /*
     * Basic Display Parameters and Features (Section 3.6)
     */
    uint8_t videoInputDefinition {0xa5}; // (Section 3.6.1 and Table 3.11)

    /*
     * The Aspect ratio or screen size (Section 3.6.2 and Table 3.12) As we can't
     * determine the aspect ratio nor the horizontal and vertical screen size
     * We set the value to 0 which is per Specification used for variable or
     * unknown ARs and Screen sizes.
     */
    uint16_t aspectRatio {0x0000};
    uint8_t displayTransferCharacteristic {0x78}; // (Section 3.6.3)
    uint8_t supportedFeatures {0x23};             // (Section 3.6.4 and Table 3.14)

    /*
     * Color Characteristics (Section 3.7)
     * Note: these values are copied and modified from
     * src/VBox/Additions/linux/drm/vbox_mode.c:vbox_set_edid
     */
    uint8_t redGreenLowOrder {0xfc};
    uint8_t blueWhiteLowOrder {0x81};
    uint8_t redXHighOrder {0xa4};
    uint8_t redYHighOrder {0x55};
    uint8_t greenXHighOrder {0x4d};
    uint8_t greenYHighOrder {0x9d};
    uint8_t blueXHighOrder {0x25};
    uint8_t blueYHighOrder {0x12};
    uint8_t whiteXHighOrder {0x50};
    uint8_t whiteYHighOrder {0x54};

    /*
     * Established Timings (Section 3.8 and Table 3.18)
     */
    uint8_t establishedTimings1 {0x21}; // 640x480,60HZ; 800x600,60HZ
    uint8_t establishedTimings2 {0x8};  // 1024x768,60HZ
    uint8_t manufacturersTimings {0x0};

    /*
     * Standard Timings (Section 3.9)
     */
    EdidStandardTiming standardTimings[8];

    /*
     * The 18 Byte Descriptors (Section 3.10)
     */
    EdidDetailedTimingDescriptor preferredTimingMode {};

    /*
     * The Second to 4th 18 byte descriptor.
     * At the Moment we use Display Descriptor Definitions only, but
     * DetailedTimingDescriptors are possible here as well.
     */
    EdidDisplayDescriptorDefinitions displayDescriptors[3];

    uint8_t extensionBlockCount {0};

    uint8_t checksum;
};

constexpr uint32_t EDID_LENGTH {sizeof(EdidBaseBlock)};

/*
 * The EIA/CEA 861F Extended EDID Structures.
 *
 * The EDID contains the EDID Base Block and an additional EIA/CEA 861F
 * Compliant EDID Block.
 *
 * The EIA/CEA 861G Block Layouts are specified in the CEA-861-F Specification (Section 7.5).
 * Link:
 * https://web.archive.org/web/20171201033424/https://standards.cta.tech/kwspub/published_docs/CTA-861-G_FINAL_revised_2017.pdf
 */

/*
 *  The CEA Data Block Header Type Byte structure. (Table 54  CEA 861-G)
 */
struct __attribute__((__packed__)) CEADataBlockHeader
{
    /*
     * The CEA Data Block Codes (Table 55 EIA/CEA 861-G Specification)
     */
    enum __attribute__((__packed__)) DataBlockTagCode : uint8_t
    {
        Audio = 1 << 5,
        Video = 2 << 5,
        VendorSpecific = 3 << 5,
        SpeakerAllocation = 4 << 5,
        VESADisplayTransferCharacteristic = 5 << 5,
        UseExtendedTag = 7 << 5,
    };

    enum __attribute__((__packed__)) : uint8_t
    {
        LENGTH_MASK = 0x1F,
        TAG_MASK = 0xE0,
    };

    uint8_t tagAndLength;
};

/*
 * The Video Data Block of the EIA/CEA 861-G Specification (Section 7.5.1)
 */
struct __attribute__((__packed__)) CEAVideoDataBlock : public CEADataBlockHeader
{
    static constexpr uint8_t MAX_SHORT_VIDEO_DESCRIPTORS {0x1F};
    std::array<uint8_t, MAX_SHORT_VIDEO_DESCRIPTORS> shortVideoDescriptors;
};

/*
 * The CEA Timing Extension.
 *
 * The Timing Extension can contain the following CEA Data Blocks:
 * - Video Data Block
 * - Audio Data Block
 * - Speaker Allocation Data Block
 * - Vendor Specific Data Block
 *
 * Our current use case requires Video Data Blocks only.
 * For simplicity the Structure contains Video Data Blocks only.
 *
 *
 * The Basic Layout can be seen in Table 52 and Table 53 of the EIA/CEA-861-G Specification
 */
struct __attribute__((__packed__)) CEAExtendedEdid : public EdidBaseBlock
{
    uint8_t eiaCeaTag {0x2};
    uint8_t eiaCeaRevision {0x3};
    uint8_t eiaCeaDetailedTimingDescriptorOffset {0x0};
    uint8_t eiaCeaNativeFormatsandFeatures {0x0};
    CEAVideoDataBlock videoDataBlock;

    /* padding or other data blocks or DTD's */
    std::array<uint8_t, sizeof(EdidBaseBlock) - sizeof(videoDataBlock) - 5> padding;
    uint8_t eiaCeaChecksum {0x0};
};

static_assert(sizeof(CEAExtendedEdid) == 256, "The CEAExtendedEdid is not 256 bytes large.");

/**
 * Generates a EDID (Extended Display Identification Data) where the Preferred
 * Timing Mode has the given resolution.
 *
 * The mechanism works following way:
 * - unplug the virtual display from the vGPU
 * - set the new EDID generated from the given resolution
 * - plug in the virtual display
 *
 * For the guest OS it looks like a new monitor is connected to the graphics
 * controller.
 *
 *  The EDID is implemented based on the VESA-EEDID-A2 Specification.
 *  https://glenwing.github.io/docs/VESA-EEDID-A2.pdf
 *
 * \param xRes horizontal resolution of the virtual display
 * \param yRes vertical resolution of the virtual display
 * \return A Edid where the Preferred Timing Mode has the given resolution
 */
template <typename EDID>
static inline EDID prepareEdid(uint32_t xRes, uint32_t yRes, uint32_t extensionBlockCount = 0)
{
    EDID edid;

    edid.standardTimings[0] = {1920, EdidStandardTiming::AR_16_10, 60}; // 1920x1200, 60hz
    edid.standardTimings[1] = {1920, EdidStandardTiming::AR_16_9, 60};  // 1920x1080, 60hz
    edid.standardTimings[2] = {1680, EdidStandardTiming::AR_16_10, 60}; // 1680x1050, 60hz
    edid.standardTimings[3] = {1600, EdidStandardTiming::AR_16_9, 60};  // 1600x900, 60hz
    edid.standardTimings[4] = {1600, EdidStandardTiming::AR_4_3, 60};   // 1600x1200, 60hz
    edid.standardTimings[5] = {1024, EdidStandardTiming::AR_4_3, 60};   // 1024x768, 60hz
    edid.standardTimings[6] = {800, EdidStandardTiming::AR_4_3, 60};    // 800x600, 60hz
    edid.standardTimings[7] = {640, EdidStandardTiming::AR_4_3, 60};    // 640x480, 60hz

    const uint32_t hblank =
        ((edid.preferredTimingMode.hVideoBlankingHigh & 0x0f) << 8) | edid.preferredTimingMode.hBlankingLow;
    const uint32_t vblank =
        ((edid.preferredTimingMode.vVideoBlankingHigh & 0x0f) << 8) | edid.preferredTimingMode.vVBlankingLow;
    const uint8_t refresh_rate = 60;
    uint16_t clock = (xRes + hblank) * (yRes + vblank) * refresh_rate / 10000;

    edid.preferredTimingMode.pixelClock = clock;

    edid.preferredTimingMode.hVideoLow = xRes & 0xff;
    edid.preferredTimingMode.hVideoBlankingHigh &= 0xf;
    edid.preferredTimingMode.hVideoBlankingHigh |= (xRes >> 4) & 0xf0;

    edid.preferredTimingMode.vVideoLow = yRes & 0xff;
    edid.preferredTimingMode.vVideoBlankingHigh &= 0xf;
    edid.preferredTimingMode.vVideoBlankingHigh |= (yRes >> 4) & 0xf0;

    edid.displayDescriptors[0].tag = EdidDisplayDescriptorDefinitions::DisplayRangeLimits;
    edid.displayDescriptors[0].rangeLimitOffset = 0x0;
    edid.displayDescriptors[0].rangeLimitsTimingDescriptor.minimumVerticalRate = 0x18;
    edid.displayDescriptors[0].rangeLimitsTimingDescriptor.maximumVerticalRate = 0x3c;
    edid.displayDescriptors[0].rangeLimitsTimingDescriptor.minimumHorizontalRate = 0x18;
    edid.displayDescriptors[0].rangeLimitsTimingDescriptor.maximumHorizontalRate = 0x50;
    edid.displayDescriptors[0].rangeLimitsTimingDescriptor.maximumPixelClock = 0x11;
    edid.displayDescriptors[0].rangeLimitsTimingDescriptor.videoTimingSupportFlags = 0x0;
    edid.displayDescriptors[0].rangeLimitsTimingDescriptor.videoTimingDataOrLineFeed = 0x0a;
    std::memset(&edid.displayDescriptors[0].rangeLimitsTimingDescriptor.videoTimingDataOrSpace, 0x20,
                sizeof(edid.displayDescriptors[0].rangeLimitsTimingDescriptor.videoTimingDataOrSpace));

    edid.displayDescriptors[1].tag = EdidDisplayDescriptorDefinitions::DisplayProductName;

    edid.displayDescriptors[2].tag = EdidDisplayDescriptorDefinitions::DisplaySerialNumber;
    /* The EDID requires a different serial number on change, thus we add the x Resolution to the serial number. */
    std::string serialNumber = "Cyberus " + std::to_string(xRes);
    std::strncpy(&edid.displayDescriptors[2].serialNumber[0], serialNumber.c_str(),
                 std::min(serialNumber.length(), sizeof(edid.displayDescriptors[2].serialNumber)));

    edid.extensionBlockCount = extensionBlockCount;

    const uint8_t edidChecksumIndex {offsetof(EdidBaseBlock, checksum)};
    auto edidPtr {reinterpret_cast<uint8_t*>(&edid)};
    auto sum {std::accumulate(edidPtr, edidPtr + edidChecksumIndex, 0)};
    edid.checksum = (0x100 - (sum & 0xff)) & 0xff;

    return edid;
}

static inline std::array<uint8_t, EDID_LENGTH> generateEdid(uint32_t xRes, uint32_t yRes)
{
    auto edid {prepareEdid<EdidBaseBlock>(xRes, yRes)};

    std::array<uint8_t, sizeof(edid)> array;
    std::memcpy(array.data(), &edid, array.size());

    return array;
}

static inline CEAExtendedEdid generateExtendedEdid(uint32_t xRes, uint32_t yRes)
{
    CEAExtendedEdid edid {prepareEdid<CEAExtendedEdid>(xRes, yRes, 1)};

    uint8_t timingCount {0};

    /*
     * All timings that can be used in Short Video Descriptors are defined
     * in (Table 3: Video Formats CEA-861-G Specifications) indexed by their Video ID Code (VIC)
     */
    auto add_timing = [&timingCount, &edid](const uint8_t vic, const bool native = false) {
        /*
         * The Video Data Block is able to support up to 0x1
         */
        if (timingCount <= CEAVideoDataBlock::MAX_SHORT_VIDEO_DESCRIPTORS) {
            /*
             * For timings with VIC < 65 a native indicator can be set (Refer Section 7.2.3 CEA 861-F Spec.)
             * This requires a special handling.
             */
            if (vic < 65 and native) {
                edid.videoDataBlock.shortVideoDescriptors[timingCount++] = (1 << 7) | (vic & 0x7f);
            } else {
                edid.videoDataBlock.shortVideoDescriptors[timingCount++] = vic;
            }
        }
    };

    /*
     * The VIC numbers taken from (Table 3: Video Formats CEA 861-G Specification)
     *
     * A Native resolution basically means the displays standard resolution
     */
    add_timing(5, true); // 1920x1080, 60hz
    add_timing(90);      // 2560x1080, 60hz
    add_timing(97);      // 3840x2160, 60hz

    edid.videoDataBlock.tagAndLength =
        CEADataBlockHeader::DataBlockTagCode::Video | (timingCount & CEADataBlockHeader::LENGTH_MASK);
    edid.eiaCeaDetailedTimingDescriptorOffset = 4 + sizeof(CEAVideoDataBlock);
    const uint8_t checksumIndex {sizeof(CEAExtendedEdid) - sizeof(EdidBaseBlock) - 1};
    auto edidPtr {reinterpret_cast<uint8_t*>(&edid) + sizeof(EdidBaseBlock)};
    auto sum {std::accumulate(edidPtr, edidPtr + checksumIndex, 0)};
    edid.eiaCeaChecksum = (0x100 - (sum & 0xff)) & 0xff;

    return edid;
}
