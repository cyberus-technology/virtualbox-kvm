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

#include <cstdint>

namespace virtioGpu
{

constexpr uint32_t INITIAL_WIDTH {1920u};
constexpr uint32_t INITIAL_HEIGHT {1080u};

/**
 * Virtio 1.2 - 4.1.2 PCI Device Discovery
 * The PCI Device ID is calculated by adding 0x1040 to the Virtio Device ID.
 */
enum : uint16_t
{
    DEVICE_ID = 16,
    PCI_DEVICE_ID = 0x1040 + DEVICE_ID,
    PCI_CLASS_BASE = 0x03, ///< GPU
    PCI_CLASS_SUB = 0x00,  ///< VGA compatible
    PCI_CLASS_PROG = 0x00, ///< Unspecified
    PCI_INTERRUPT_LINE = 0x00,
    PCI_INTERRUPT_PIN = 0x01,
};

/**
 * Virtio 1.2 - 5.7.1 GPU Device Feature bits
 */
enum Features : uint16_t
{
    VIRGIL = 1u << 0,        ///< virgl 3D mode is supported
    EDID = 1u << 1,          ///< EDID (Extended Display Identification Data) is supported
    RESOURCE_UUID = 1u << 2, ///< assigning resources UUIDs for export to other virtio devices is supported
    RESOURCE_BLOB = 1u << 3, ///< creating and using size-based blob resources is supported
    CONTEXT_INIT = 1u << 4,  ///< multiple context types and synchronization timelines supported
};

/**
 * Virtio 1.2 - 5.7.2 GPU Device Virtqueues
 */
constexpr unsigned NUM_VIRTQUEUES {2u};

enum VirtqIdx : uint16_t
{
    CONTROLQ = 0, ///< The index of the controlqueue
    CURSORQ = 1,  ///< The index of the cursorqueue
};

/**
 * Virtio 1.2 - 5.7.4 GPU Device configuration layout
 * Virtio GPU device-specific configuration
 */
struct Config
{
    uint32_t uEventsRead {0u};  ///< Signals pending events to the driver
    uint32_t uEventsClear {0u}; ///< Clears pending events in the device (write-to-clear)
    uint32_t uNumScanouts {0u}; ///< Maximum number of scanouts supported (between 1 and 16 inclusive)
    uint32_t uNumCapsets {0u};  ///< Maximum number of capability sets supported
};

static constexpr uint32_t EVENT_DISPLAY {
    1u << 0}; ///< display configuration has changed and should be fetched by the driver

/**
 * Virtio 1.2 - 5.7.6.7 GPU Device Device Operation: Request header
 */
struct CtrlType
{
    enum Cmd : uint32_t
    {
        /* 2d commands */
        GET_DISPLAY_INFO = 0x0100,
        RESOURCE_CREATE_2D,
        RESOURCE_UNREF,
        SET_SCANOUT,
        RESOURCE_FLUSH,
        TRANSFER_TO_HOST_2D,
        RESOURCE_ATTACH_BACKING,
        RESOURCE_DETACH_BACKING,
        GET_CAPSET_INFO,
        GET_CAPSET,
        GET_EDID,
        RESOURCE_ASSIGN_UUID,
        RESOURCE_ASSIGN_BLOB,
        SET_SCANOUT_BLOB,

        /* 3d commands */
        CTX_CREATE = 0x0200,
        CTX_DESTROY,
        CTX_ATTACH_RESOURCE,
        CTX_DETACH_RESOURCE,
        RESOURCE_CREATE_3D,
        TRANSFER_TO_HOST_3D,
        TRANSFER_FROM_HOST_3D,
        SUBMIT_3D,
        RESOURCE_MAP_BLOB,
        RESOURCE_UNMAP_BLOB,

        /* cursor commands */
        UPDATE_CURSOR = 0x0300,
        MOVE_CURSOR,
    };

    enum Response : uint32_t
    {
        /* success responses */
        OK_NODATA = 0x1100,
        OK_DISPLAY_INFO,
        OK_CAPSET_INFO,
        OK_CAPSET,
        OK_EDID,
        OK_RESOURCE_UUID,
        OK_MAP_INFO,

        /* error responses */
        ERR_UNSPEC = 0x1200,
        ERR_OUT_OF_MEMORY,
        ERR_INVALID_SCANOUT_ID,
        ERR_INVALID_RESOURCE_ID,
        ERR_INVALID_CONTEXT_ID,
        ERR_INVALID_PARAMETER,
    };
};

/**
 * Virtio 1.2 - 5.7.6.8 GPU Device Operation: controlq
 * VIRTIO_GPU_CMD_RESOURCE_CREATE_2D possible formats
 */
struct __attribute__((packed)) CtrlHdr
{
    uint32_t uType {0};    ///< type specifies the type of driver request or device response
    uint32_t uFlags {0};   ///< flags request/response flags
    uint64_t uFenceId {0}; ///< fence_id only important if FLAG_FENCE bit is set in flags
    uint32_t uCtxId {0};   ///< ctx_id rendering context (3D mode only)
    uint8_t uRingIdx {0};  ///< ring_idx
    uint8_t uPadding[3];

    CtrlHdr() = default;
    CtrlHdr(uint32_t uCmd) : uType(uCmd) {}
    CtrlHdr(CtrlType::Cmd uCmd) : uType(uCmd) {}

    enum class Flags : uint32_t
    {
        FENCE = 1u << 0,
        INFO_RING_IDX = 1u << 1,
    };

    /* Checks whether the given bit is set in uFlags */
    inline bool has_flag(Flags flag) const { return (uFlags & static_cast<uint32_t>(flag)) != 0; };

    /* Sets the given bit if fSet is true, otherwise clears it. */
    inline void set_flag(Flags flag, bool fSet)
    {
        const uint32_t mask {static_cast<uint32_t>(flag)};
        uFlags &= ~mask | fSet ? mask : 0u;
    };

    /* Transfers the fence-flag and uFenceId from other to this. */
    inline void transfer_fence(const CtrlHdr* other)
    {
        if (other->has_flag(Flags::FENCE)) {
            set_flag(Flags::FENCE, true);
            uFenceId = other->uFenceId;
        }
    }
};

/*
 * controlq command structure definitions
 * See Virtio 1.2 - 5.7.6.8
 */
static void* removeHeader(void* pThis)
{
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pThis) + sizeof(CtrlHdr));
}

struct __attribute__((packed)) Rect
{
    uint32_t x {0};
    uint32_t y {0};
    uint32_t width {0};
    uint32_t height {0};

    Rect() = default;
    Rect(uint32_t w, uint32_t h) : width(w), height(h) {}

    friend bool operator==(const Rect& lhs, const Rect& rhs)
    {
        return lhs.x == rhs.x and lhs.y == rhs.y and lhs.width == rhs.width and lhs.height == rhs.height;
    }

    friend bool operator!=(const Rect& lhs, const Rect& rhs) { return not(lhs == rhs); }
};

struct __attribute__((packed)) DisplayOne
{
    Rect r;
    uint32_t enabled {0};
    uint32_t flags {0};
};

struct __attribute__((packed)) ResponseDisplayInfo
{
private:
    static constexpr uint32_t NUM_MAX_SCANOUTS {16u};

public:
    CtrlHdr hdr {CtrlType::Response::OK_DISPLAY_INFO};
    DisplayOne pmodes[NUM_MAX_SCANOUTS];

    static size_t size(uint32_t num_scanouts) { return sizeof(CtrlHdr) + num_scanouts * sizeof(DisplayOne); }
};

struct __attribute__((packed)) GetEdid
{
    CtrlHdr hdr {CtrlType::Cmd::GET_EDID};
    uint32_t uScanout {0};
    uint32_t uPadding;

    void* payload() { return removeHeader(this); }
};

struct __attribute__((packed)) ResponseEdid
{
    CtrlHdr hdr {CtrlType::Response::OK_EDID};
    uint32_t uSize {0};
    uint32_t uPadding;
    uint8_t aEdid[1024] {0};
};

enum Format : uint32_t
{
    B8G8R8A8_UNORM = 1,
    B8G8R8X8_UNORM = 2,
    A8R8G8B8_UNORM = 3,
    X8R8G8B8_UNORM = 4,

    R8G8B8A8_UNORM = 67,
    X8B8G8R8_UNORM = 68,

    A8B8G8R8_UNORM = 121,
    R8G8B8X8_UNORM = 134,
};

struct __attribute__((packed)) ResourceCreate2d
{
    CtrlHdr hdr {CtrlType::Cmd::RESOURCE_CREATE_2D};
    uint32_t uResourceId {0};
    uint32_t uFormat {0};
    uint32_t uWidth {0};
    uint32_t uHeight {0};

    ResourceCreate2d() = default;
    ResourceCreate2d(uint32_t id) : uResourceId(id) {}
    ResourceCreate2d(uint32_t id, uint32_t w, uint32_t h) : uResourceId(id), uWidth(w), uHeight(h) {}

    void* payload() { return removeHeader(this); }
};

struct __attribute__((packed)) ResourceUnref
{
    CtrlHdr hdr {CtrlType::Cmd::RESOURCE_UNREF};
    uint32_t uResourceId {0};
    uint32_t uPadding;

    ResourceUnref() = default;
    ResourceUnref(uint32_t id) : uResourceId(id) {}

    void* payload() { return removeHeader(this); }
};

struct __attribute__((packed)) SetScanout
{
    CtrlHdr hdr {CtrlType::Cmd::SET_SCANOUT};
    Rect r;
    uint32_t uScanoutId {0};
    uint32_t uResourceId {0};

    SetScanout() = default;
    SetScanout(uint32_t scanoutId, uint32_t resId) : uScanoutId(scanoutId), uResourceId(resId) {}
    SetScanout(uint32_t scanoutId, uint32_t resId, uint32_t w, uint32_t h)
        : r(w, h), uScanoutId(scanoutId), uResourceId(resId)
    {}

    void* payload() { return removeHeader(this); }
};

struct __attribute__((packed)) ResourceFlush
{
    CtrlHdr hdr {CtrlType::Cmd::RESOURCE_FLUSH};
    Rect r;
    uint32_t uResourceId {0};
    uint32_t uPadding;

    void* payload() { return removeHeader(this); }
};

struct __attribute__((packed)) TransferToHost2d
{
    CtrlHdr hdr {CtrlType::Cmd::TRANSFER_TO_HOST_2D};
    Rect r;
    uint64_t uOffset {0};
    uint32_t uResourceId {0};
    uint32_t uPadding;

    TransferToHost2d() = default;
    TransferToHost2d(uint32_t resId) : uResourceId(resId) {}
    TransferToHost2d(uint32_t resId, uint32_t w, uint32_t h) : r(w, h), uResourceId(resId) {}

    void* payload() { return removeHeader(this); }
};

struct __attribute__((packed)) ResourceAttachBacking
{
    CtrlHdr hdr {CtrlType::Cmd::RESOURCE_ATTACH_BACKING};
    uint32_t uResourceId {0};
    uint32_t uNrEntries {0};

    void* payload() { return removeHeader(this); }
};

struct __attribute__((packed)) ResourceMemEntry
{
    uint64_t uAddr {0};
    uint32_t uLength {0};
    uint32_t uPadding;
};

struct __attribute__((packed)) ResourceDetachBacking
{
    CtrlHdr hdr {CtrlType::Cmd::RESOURCE_DETACH_BACKING};
    uint32_t uResourceId {0};
    uint32_t uPadding;

    ResourceDetachBacking() = default;
    ResourceDetachBacking(uint32_t id) : uResourceId(id) {}

    void* payload() { return removeHeader(this); }
};

} // namespace virtioGpu
