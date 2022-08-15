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

#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmifs.h>
#include <VBox/vmm/pgm.h>

#include "DevVirtioGpuCmdHandler.hpp"
#include <VBox/Graphics/VBoxVideo.h>

#include <mutex>

/**
 * The Virtio Display Manager implementation.
 *
 */
class VirtioGpuDisplayManager final : public VirtioGpuCmdHandler::DisplayManager
{
public:
    VirtioGpuDisplayManager() = delete;

    /**
     * The Virtio GPU Display Manager.
     *
     * \param pDevINs_ The VBox PDM Device Instance Data
     * \param iLUN_ The device LUN of the GPU
     * \param iBase_ The Virtio GPU Interface Base
     * \param u32VRamSize The assigned VRAM
     * \param u32MonitorCount The maximum active monitor count
     */
    VirtioGpuDisplayManager(PPDMDEVINS pDevIns_, unsigned iLUN_, PDMIBASE& iBase_, uint32_t u32VRamSize,
                            uint32_t u32MonitorCount);

    ~VirtioGpuDisplayManager();

    /**
     * Reset the Display infrastructure
     */
    void reset();

    /**
     * Check whether the display index is managed by the Manager.
     *
     * \param displayIndex The display index.
     *
     * \return true if the display is managed, false otherwise
     */
    virtual bool isManaged(uint32_t displayIndex) final;

    /**
     * Obtain the dimensions of the given display.
     *
     * \param displayIndex The display index
     *
     * \return the dimension of the display or <0,0> if not managed
     */
    virtual Dimension displayDimension(uint32_t displayIndex) final;

    /**
     * Resize the given display to the given width and height.
     *
     * \param displayIndex The display index
     * \param uWidth display width
     * \param uHeigth display height
     * \param u32OriginX (Optional) The display x position
     * \param u32OriginY (Optional) The display y position
     */
    virtual void resize(uint32_t displayIndex, uint32_t uWidth, uint32_t uHeight, std::optional<int32_t> i32OriginX,
                        std::optional<int32_t> i32OriginY) final;

    /**
     * Attaches the virtio-gpu to the given display.
     *
     * \param displayIndex The display index
     */
    virtual int attachDisplay(uint32_t displayIndex) final;

    /**
     * Detaches the virtio-gpu from the given display.
     *
     * \param displayIndex The display index
     */
    virtual void detachDisplay(uint32_t displayIndex) final;

    /**
     * Check attachment status of the given display.
     *
     * \param displayIndex The display index
     *
     * \return true if the display is attached, false otherwise.
     */
    virtual bool isAttached(uint32_t displayIndex) final;
    /**
     * Displays the framebuffer content on the display.
     *
     * \param displayIndex The display index
     */
    virtual void display(uint32_t displayIndex) final;

    /**
     * Obtain backing store.
     *
     * The function activates the pDrv lock, to ensure consistency.
     *
     * \param displayIndex The display index
     *
     * \return Backing store information
     */
    virtual BackingStoreInfo acquireBackingStore(uint32_t displayIndex) final;

    /**
     * Release the backing store lock.
     */
    virtual void releaseBackingStore() final;

    /**
     * Take over the display driver from the default Graphics Adapter.
     *
     * \return VBox Status Code
     */
    int takeoverDriver();

    /**
     * Hand back the display driver to the default Graphics Adapter.
     */
    void handoverDriver();

    /**
     * Detach all attached displays.
     */
    void detachAllDisplays();

private:
    /**
     * Update the screen data at the VBox Video Acceleration infrastructure for
     * the desired display, including a resize, if necessary.
     *
     * \param displayIndex The desired display index
     * \param fResetInputMapping Whether to reset the input mapping or not.
     *
     * \return VBox Status Code
     */
    int resizeVBVA(uint32_t displayIndex, bool fResetInputMapping);

    /**
     * Enable the VirtualBox Video Acceleration for the desired display.
     *
     * \param displayIndex The desired display index
     *
     *
     * \return VBox Status Code
     */
    int enableVBVA(uint32_t displayIndex);

    /**
     * Disable the VirtualBox Video Acceleration for the desired display.
     *
     * \param displayIndex The desired display index
     */
    void disableVBVA(uint32_t displayIndex);

    /**
     * Check that all displays are detached.
     *
     * \return true if all displays are detached, false otherwise.
     */
    bool allDisplaysDetached();

    /**
     * The display internal management data structure
     */
    struct Display
    {
        VBVAINFOVIEW view;
        VBVAINFOSCREEN screen;
    };

    PPDMDEVINS pDevIns;
    unsigned iLUN;
    PDMIBASE& iBase;
    const uint32_t u32MonitorCount;

    PPDMIDISPLAYCONNECTOR pDrv {nullptr};
    PPDMIBASE pDrvBase {nullptr};

    // Once we have taken over the display this get's true and stays true
    // until guest reset or reboot.
    bool ownDisplay {false};

    std::vector<uint8_t> vram;
    std::vector<Display> displays;

    std::mutex driverMtx;
    using DriverGuard = std::lock_guard<std::mutex>;
};
