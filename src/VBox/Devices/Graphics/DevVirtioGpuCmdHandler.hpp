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

#include <VBox/types.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/stam.h>

#include <iprt/mem.h>
#include <iprt/cpp/lock.h>

#include "../VirtIO/VirtioCore.h"

#include "DevVirtioGpuResource.hpp"
#include "DevVirtioGpuDefinitions.hpp"

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

class VirtioGpuCmdHandler
{
public:
    /**
     * A VirtioAdapter encapsulates functions to receive data from a virtq,
     * put data into a virtq and signal to the guest that there is new data
     * in a virtq.
     */
    class VirtioAdapter
    {
    public:
        /* Drains cb bytes from the pVirtqBuf into pv. */
        virtual void virtqBufDrain(PVIRTQBUF pVirtqBuf, void* pv, size_t cb) = 0;

        /* Puts pv into the used ring and thus sends data to the guest. */
        virtual void virtqBufPut(PVIRTQBUF pVirtqBuf, void* pv, size_t cb) = 0;

        /* Informs the guest driver about new data in the virtq. */
        virtual void virtqSyncRings(PVIRTQBUF pVirtqBuf) = 0;
    };

    /**
     * The DisplayManager is the interface to the virtual displays provided by
     * the hypervisor platform.
     * It is responsible for updating the dimensions and the frames of the
     * virtual displays according to the data in the scanouts.
     */
    class DisplayManager
    {
    public:
        /**
         * The Backing store information
         */
        using BackingStoreInfo = std::pair<void*, size_t>;
        using Dimension = std::pair<uint32_t, uint32_t>;

        /**
         * Check whether the display index is managed by the Manager.
         *
         * \param displayIndex The display index.
         *
         * \return true if the display is managed, false otherwise
         */
        virtual bool isManaged(uint32_t displayIndex) = 0;

        /**
         * Obtain the dimensions of the given display.
         *
         * \param displayIndex The display index
         *
         * \return the dimension of the display or <0,0> if not managed
         */
        virtual Dimension displayDimension(uint32_t displayIndex) = 0;

        /**
         * Resize the given display to the given width and height.
         *
         * \param displayIndex The display index
         * \param uWidth display width
         * \param uHeigth display height
         * \param i32OriginX The x position of the display to resize
         * \param i32OriginY The y position of the display to resize
         */
        virtual void resize(uint32_t displayIndex, uint32_t uWidth, uint32_t uHeight,
                            std::optional<int32_t> i32OriginX = std::nullopt,
                            std::optional<int32_t> i32OriginY = std::nullopt) = 0;

        /**
         * Attaches the virtio-gpu to the given display.
         *
         * \param displayIndex The display index
         */
        virtual int attachDisplay(uint32_t displayIndex) = 0;

        /**
         * Detaches the virtio-gpu from the given display.
         *
         * \param displayIndex The display index
         */
        virtual void detachDisplay(uint32_t displayIndex) = 0;

        /**
         * Check attachment status of the given display.
         *
         * \param displayIndex The display index
         *
         * \return true if the display is attached, false otherwise.
         */
        virtual bool isAttached(uint32_t displayIndex) = 0;

        /**
         * Displays the framebuffer content on the display.
         *
         * \param displayIndex The display index
         */
        virtual void display(uint32_t displayIndex) = 0;

        /**
         * Obtain backing store information.
         *
         * \param displayIndex The display index
         *
         * \return Backing store information
         */
        virtual BackingStoreInfo acquireBackingStore(uint32_t displayIndex) = 0;

        /**
         * Releases the backing store, after an update of the frame buffer is done.
         */
        virtual void releaseBackingStore() = 0;
    };

    /**
     * A memoryAdapter is used to map guest physical addresses into the host's
     * address space and to also unmap these mappings.
     */
    class MemoryAdapter
    {
    protected:
        struct VirtioGpuMapping
        {
            void* uAddr_;            // The host virtual address
            const uint32_t uLength_; // The size of the mapping
            void* pv_;               // A pointer to data the adapter may need to free this mapping later

            VirtioGpuMapping() = delete;
            VirtioGpuMapping(void* uAddr, const uint32_t uLength, void* pv) : uAddr_(uAddr), uLength_(uLength), pv_(pv)
            {}
        };

        using VecMemEntries = std::vector<VirtioGpuResource::MemEntry>;
        using VecMappings = std::vector<VirtioGpuMapping>;

    public:
        /**
         * Translates the guest physical addresses given in vBacking into host
         * virtual addresses.
         */
        virtual VecMappings mapGCPhys2HCVirt(const VecMemEntries& vBacking) = 0;

        /**
         * Returns the mappings to the adapter so the adapter can do whatever
         * is necessary.
         */
        virtual void releaseMappings(const VecMappings& vMapping) = 0;
    };

private:
    VirtioAdapter& virtioAdapter_;
    DisplayManager& displayManager_;
    MemoryAdapter& memoryAdapter_;
    const uint32_t numScanouts_;
    RTCLockMtx mutex_;

    struct Scanout
    {
        uint32_t uScanoutId {0};
        uint32_t uResourceId {0};
        uint32_t uCurrentWidth {0};
        uint32_t uCurrentHeight {0};
        uint32_t uResizedWidth {0u};
        uint32_t uResizedHeight {0u};
        bool fActive {false};
        bool fNeedsResize {true};
        bool fResizeRequested {false};

        DisplayManager& rDisplayManager;

        Scanout() = delete;
        Scanout(DisplayManager& dManager) : rDisplayManager(dManager) {}

        bool hasDisplay() const { return rDisplayManager.isManaged(uScanoutId); }

        bool isAttachedToDisplay() const { return hasDisplay() and rDisplayManager.isAttached(uScanoutId); }

        void attachDisplay() const
        {
            if (hasDisplay() and not isAttachedToDisplay()) {
                rDisplayManager.attachDisplay(uScanoutId);
            }
        }

        void detachDisplay() const
        {
            if (isAttachedToDisplay()) {
                rDisplayManager.detachDisplay(uScanoutId);
            }
        }

        DisplayManager::Dimension displayDimensions() const { return rDisplayManager.displayDimension(uScanoutId); }

        void resizeDisplay() { rDisplayManager.resize(uScanoutId, uCurrentWidth, uCurrentHeight); }

        void flush() { rDisplayManager.display(uScanoutId); }
    };

    using ScanoutRef = std::reference_wrapper<Scanout>;
    using ScanoutCRef = std::reference_wrapper<const Scanout>;

    std::vector<std::unique_ptr<VirtioGpuResource>> vResources_;

    /* Returns a pointer to the resource with the given ID, or a nullptr if there is no resource with the given ID. */
    inline VirtioGpuResource* getResource(uint32_t uResourceId);

    /* Creates a resource with the given ID. Returns false if the ID was already in use, true otherwise. */
    inline bool createResource(uint32_t uResourceId);

    /* Removes the resource with the given ID. */
    inline void removeResource(uint32_t uResourceId);

    std::vector<Scanout> activeScanouts;

    /* Returns true if the given scanout is in the range of existing scanouts (0 to NUM_MAX_SCANOUTS-1), false otherwise
     */
    inline bool scanoutExists(uint32_t uScanout);

    /* Returns a reference to a scanout if the given scanout exists, an empty optional otherwise. */
    inline std::optional<ScanoutRef> getScanout(uint32_t uScanout);

    /**
     * Returns a vector filled with references to all scanouts with the given resourceId, or an empty optional if no
     * scanout is assigned to the given resourceId.
     */
    inline std::vector<ScanoutRef> getScanoutsByResource(uint32_t uResourceId);

    /* Returns a dummy size to initialize the scanouts in case we are a secondary graphics controller. */
    inline std::tuple<uint32_t, uint32_t> getDummySize()
    {
        return std::make_tuple(virtioGpu::INITIAL_WIDTH, virtioGpu::INITIAL_HEIGHT);
    }

public:
    VirtioGpuCmdHandler() = delete;
    VirtioGpuCmdHandler(VirtioAdapter& vAdapter, DisplayManager& dManager, MemoryAdapter& mAdapter,
                        uint32_t numScanouts, bool attachDisplayLater);

    ~VirtioGpuCmdHandler() = default;

    /* Destroys all existing resources. */
    void clearResources() { vResources_.clear(); }

    /**
     * Returns a const reference to the scanout with the given ID if it exists.
     * That way another class working with the cmdHandler can't modify the scanouts.
     */
    std::optional<ScanoutCRef> getCScanout(uint32_t uScanout);

    /**
     * Requests resizing of the monitor. Sets the uResizedWidth and uResizedHeight variables of the associated scanout
     * and sets scanout.fResizeRequested to true. The next getDisplayInfo or getEdid will then use the new resolution.
     */
    void requestResize(uint32_t uScanout, bool enabled, uint32_t uWidth, uint32_t uHeight);

    /** Updates the current width and height of the given scanout and resizes display if necessary. */
    inline void resizeScanout(uint32_t uScanout, uint32_t uWidth, uint32_t uHeight);

    /* Handles the given virtq buffer and returns a response to the driver. */
    void handleBuffer(PVIRTQBUF pVirtqBuf);

    /* Does some basic sanity checking and returns an appropriate error to the guest if something is broken. */
    inline bool checkCtrlqCmd(const char* cmdName, PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr, size_t cbSend,
                              size_t cbReturn);

    /* Checks wether the given scanout id is valid and returns a ERR_INVALID_SCANOUT_ID header to the guest if it isn't.
     */
    inline bool checkScanoutId(const char* cmdName, PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr,
                               uint32_t uScanoutId);

    /* Checks wether the given resource id is valid and returns a ERR_INVALID_RESOURCE_ID header to the guest if it
     * isn't. */
    inline bool checkResourceId(const char* cmdName, PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr,
                                uint32_t uResourceId);

    /* Returns a CtrlType::Response::OK_NODATA if the FENCE-flag is not set */
    inline void returnResponseOkEarly(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr);

    /* Returns a CtrlType::Response::OK_NODATA if the FENCE-flag is set */
    inline void returnResponseOkLate(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr);

    /* Returns a header with the given resonseType to the driver. */
    inline void returnResponseNoData(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr,
                                     virtioGpu::CtrlType::Response responseType);

    /* Returns a response buffer to the driver. */
    inline void returnResponseBuf(PVIRTQBUF pVirtqBuf, void* pv, size_t cb);

    /* Returns the current output configureation to the driver. */
    void cmdGetDisplayInfo(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr);

    /* Return the EDID data for a given scanout to the driver. */
    void cmdGetEdid(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr);

    /* Creates a 2D resource on the host. */
    void cmdResourceCreate2d(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr);

    /* Destroys a 2D resource. */
    void cmdResourceUnref(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr);

    /* Sets the scanout parameters for a given output. */
    void cmdSetScanout(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr);

    /* Flushes a resource to the screen. */
    void cmdResourceFlush(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr);

    /* Transfers guest memory to host memory. */
    void cmdTransferToHost2d(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr);

    /* Attaches backing pages to a resource. */
    void cmdResourceAttachBacking(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr);

    /* Removes backing pages from a resource. */
    void cmdResourceDetachBacking(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr);
};
