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

#define LOG_GROUP LOG_GROUP_DEV_VIRTIO_GPU
#include "DevVirtioGpuDisplayManager.hpp"

#include "DevVirtioGpuResource.hpp"
#include <VBox/log.h>
#include <VBox/types.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/vm.h>

#include <cyberus/edid.hpp>

#include <algorithm>

VirtioGpuDisplayManager::VirtioGpuDisplayManager(PPDMDEVINS pDevIns_, unsigned iLUN_, PDMIBASE& iBase_,
                                                 uint32_t u32VRamSize, uint32_t u32MonitorCount_)
    : pDevIns(pDevIns_), iLUN(iLUN_), iBase(iBase_), u32MonitorCount(u32MonitorCount_)
{
    vram.resize(u32VRamSize);

    uint32_t viewIndex {0ul};

    auto generateDisplays = [&viewIndex, &u32VRamSize]() {
        Display display;
        display.view.u32ViewIndex = viewIndex;
        display.view.u32ViewSize = u32VRamSize;
        /*
         * We allow the free use of the assigned VRAM, so it is irrelevant if
         * there are multiple monitors of a mid size resolution or one single monitor with a huge resolution
         */
        display.view.u32MaxScreenSize = u32VRamSize;

        display.screen.u32ViewIndex = viewIndex;
        display.screen.u16BitsPerPixel = VirtioGpuResource::BYTES_PER_PIXEL * __CHAR_BIT__;
        display.screen.u32Width = virtioGpu::INITIAL_WIDTH;
        display.screen.u32Height = virtioGpu::INITIAL_HEIGHT;

        display.screen.i32OriginX = viewIndex * virtioGpu::INITIAL_WIDTH;
        display.screen.i32OriginY = 0;

        display.screen.u16Flags = VBVA_SCREEN_F_DISABLED;

        viewIndex++;

        return display;
    };

    std::generate_n(std::back_insert_iterator<std::vector<Display>>(displays), u32MonitorCount, generateDisplays);
    std::memset(vram.data(), 0, vram.size());
}

VirtioGpuDisplayManager::~VirtioGpuDisplayManager()
{}

void VirtioGpuDisplayManager::reset()
{
    DriverGuard _ {driverMtx};

    if (not pDrv) {
        return;
    }

    if (pDrv->pfnReset != nullptr) {
        pDrv->pfnReset(pDrv);
    }
}

bool VirtioGpuDisplayManager::isManaged(uint32_t displayIndex)
{
    return displayIndex < u32MonitorCount;
}

VirtioGpuDisplayManager::Dimension VirtioGpuDisplayManager::displayDimension(uint32_t displayIndex)
{
    if (not isManaged(displayIndex)) {
        return {0, 0};
    }

    DriverGuard _ {driverMtx};

    auto& display {displays.at(displayIndex)};

    return {display.screen.u32Width, display.screen.u32Height};
}

void VirtioGpuDisplayManager::resize(uint32_t displayIndex, uint32_t uWidth, uint32_t uHeight,
                                     std::optional<int32_t> i32OriginX, std::optional<int32_t> i32OriginY)
{
    if (not isAttached(displayIndex)) {
        return;
    }

    DriverGuard _ {driverMtx};

    auto& screen {displays.at(displayIndex).screen};

    const uint32_t bytesPerPixel {screen.u16BitsPerPixel / static_cast<unsigned>(__CHAR_BIT__)};

    int32_t newOriginX {i32OriginX.value_or(screen.i32OriginX)};
    int32_t newOriginY {i32OriginY.value_or(screen.i32OriginY)};

    screen.u32LineSize = uWidth * bytesPerPixel;
    screen.u32Width = uWidth;
    screen.u32Height = uHeight;
    screen.i32OriginX = newOriginX;
    screen.i32OriginY = newOriginY;

    /*
     * The Framebuffers of all displays are handled in a consecutive buffer of memory by VBox, which is called the VRAM
     * During a Monitor resize, the portion of memory used for the desired monitor changes.
     * Thus we need to adjust the start offset of the next monitor to avoid wrong graphics output.
     */
    for (uint32_t i {displayIndex + 1}; i < displays.size(); ++i) {
        auto& currentScreen {displays.at(i).screen};
        auto& previousScreen {displays.at(i - 1).screen};

        auto calculateScreenSize = [](auto& screen_) -> uint32_t {
            return screen_.u32Width * screen_.u32Height
                   * (screen_.u16BitsPerPixel / static_cast<unsigned>(__CHAR_BIT__));
        };

        uint32_t previousScreenSize {calculateScreenSize(previousScreen)};
        uint32_t newStartOffset {previousScreen.u32StartOffset + previousScreenSize};

        currentScreen.u32StartOffset = newStartOffset;

        int64_t xOffset {previousScreen.i32OriginX + previousScreen.u32Width};

        currentScreen.i32OriginX = xOffset;
        resizeVBVA(i, true);

        uint32_t screenSize {calculateScreenSize(currentScreen)};
        AssertLogRelMsg((currentScreen.u32StartOffset + screenSize) < vram.size(),
                        ("VirtioGpuDisplayManager: The framebuffer for the displays starting with index %u does not "
                         "fit into VRAM, monitorCount %u \n",
                         i, displays.size()));
    }

    resizeVBVA(displayIndex, true);
}

int VirtioGpuDisplayManager::attachDisplay(uint32_t displayIndex)
{
    int rc {VINF_SUCCESS};
    if (isAttached(displayIndex)) {
        return VINF_SUCCESS;
    }

    if (not isManaged(displayIndex)) {
        return VERR_NOT_AVAILABLE;
    }

    {
        DriverGuard _ {driverMtx};

        auto& display {displays.at(displayIndex)};

        LogRel6(("VirtioGpuDisplayManager: attaching monitor %u .\n", display.view.u32ViewIndex));

        display.screen.u16Flags = VBVA_SCREEN_F_ACTIVE;
    }

    if (not allDisplaysDetached() and not ownDisplay) {
        PVM pVM {PDMDevHlpGetVM(pDevIns)};

        rc = PDMR3DriverDetach(pVM->pUVM, "vga", 0, 0, NULL, 0, PDM_TACH_FLAGS_NOT_HOT_PLUG);
        AssertLogRel(RT_SUCCESS(rc));

        rc = PDMR3DriverAttach(pVM->pUVM, "vga", 0, 0, PDM_ATTACH_DUMMY_DRIVER, NULL);

        if (not pDrv) {
            rc = takeoverDriver();
            AssertLogRel(RT_SUCCESS(rc));
        }
        ownDisplay = true;
        return rc;
    }

    rc = enableVBVA(displayIndex);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    rc = resizeVBVA(displayIndex, false);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    return rc;
}

void VirtioGpuDisplayManager::detachDisplay(uint32_t displayIndex)
{
    if (not isAttached(displayIndex)) {
        LogRel(("Display %d not attached. Not going to do anything\n", displayIndex));
        return;
    }

    DriverGuard _ {driverMtx};

    if (not isManaged(displayIndex)) {
        LogRel(("Display %d not managed. Not going to do anything\n", displayIndex));
        return;
    }

    auto& display {displays.at(displayIndex)};

    LogRel6(("VirtioGpuDisplayManager: detaching monitor %u.\n", displayIndex));

    display.screen.u16Flags = VBVA_SCREEN_F_DISABLED;

    /**
     * On display termination, the pDrv is handed back to VBox already. Thus
     * the error code VERR_NOT_AVAILABLE is reported here.
     */
    resizeVBVA(displayIndex, false);

    disableVBVA(displayIndex);
}

bool VirtioGpuDisplayManager::isAttached(uint32_t displayIndex)
{
    DriverGuard _ {driverMtx};

    if (not isManaged(displayIndex)) {
        return false;
    }

    auto& display {displays.at(displayIndex)};

    return display.screen.u16Flags & VBVA_SCREEN_F_ACTIVE;
}

void VirtioGpuDisplayManager::display(uint32_t displayIndex)
{
    if (not isAttached(displayIndex)) {
        return;
    }

    DriverGuard _ {driverMtx};
    if (pDrv != nullptr) {
        auto& screen {displays.at(displayIndex).screen};
        VBVACMDHDR cmd;

        cmd.x = screen.i32OriginX;
        cmd.y = screen.i32OriginY;
        cmd.w = screen.u32Width;
        cmd.h = screen.u32Height;

        pDrv->pfnVBVAUpdateBegin(pDrv, screen.u32ViewIndex);
        pDrv->pfnVBVAUpdateProcess(pDrv, screen.u32ViewIndex, &cmd, sizeof(cmd));
        pDrv->pfnVBVAUpdateEnd(pDrv, screen.u32ViewIndex, screen.i32OriginX, screen.i32OriginY, screen.u32Width,
                               screen.u32Height);
    }
}

VirtioGpuDisplayManager::BackingStoreInfo VirtioGpuDisplayManager::acquireBackingStore(uint32_t displayIndex)
{
    driverMtx.lock();

    if (not isManaged(displayIndex) or pDrv == nullptr) {
        return {nullptr, 0};
    }

    auto& screen {displays.at(displayIndex).screen};

    uint8_t* pFramebuffer {vram.data() + screen.u32StartOffset};
    size_t cbFramebuffer {screen.u32Width * screen.u32Height
                          * (screen.u16BitsPerPixel / static_cast<unsigned>(__CHAR_BIT__))};

    return {pFramebuffer, cbFramebuffer};
}

void VirtioGpuDisplayManager::releaseBackingStore()
{
    driverMtx.unlock();
}

bool VirtioGpuDisplayManager::allDisplaysDetached()
{
    auto attachementFn = [](Display& display) { return display.screen.u16Flags & VBVA_SCREEN_F_ACTIVE; };

    return std::find_if(displays.begin(), displays.end(), attachementFn) == displays.end();
}

int VirtioGpuDisplayManager::takeoverDriver()
{
    int rc {VINF_SUCCESS};

    if (pDrvBase == nullptr) {
        rc = PDMDevHlpDriverAttach(pDevIns, iLUN, &iBase, &pDrvBase, "Display Port");
    }

    if (rc == VERR_PDM_NO_ATTACHED_DRIVER) {
        AssertLogRelMsgFailed(("VirtioGpuDisplayManager: %s/%d: warning: no driver attached to LUN #0!\n",
                               pDevIns->pReg->szName, pDevIns->iInstance));
        return VINF_SUCCESS;
    } else if (not RT_SUCCESS(rc)) {
        AssertLogRelMsgFailed(("VirtioGpuDisplayManager: failed to attach LUN #0! rc=%Rrc\n", rc));
        return rc;
    }

    /* rc == VINF_SUCCESS, i.e. pDrvBase is attached to iLUN */

    pDrv = PDMIBASE_QUERY_INTERFACE(pDrvBase, PDMIDISPLAYCONNECTOR);
    if (pDrv != nullptr) {
        if (pDrv->pfnRefresh == nullptr or pDrv->pfnResize == nullptr or pDrv->pfnUpdateRect == nullptr) {
            Assert(pDrv->pfnRefresh != nullptr);
            Assert(pDrv->pfnResize != nullptr);
            Assert(pDrv->pfnUpdateRect != nullptr);
            pDrv = nullptr;
            pDrvBase = nullptr;
            rc = VERR_INTERNAL_ERROR;
        }
    } else {
        AssertLogRelMsgFailed(
            ("VirtioGpuDisplayManager: LUN #0 doesn't have a display connector interface! rc=%Rrc\n", rc));
        pDrvBase = nullptr;
        rc = VERR_PDM_MISSING_INTERFACE;
    }

    LogRel2(("VirtioGpuDisplayDriver: Display Port Driver attached\n"));

    /*
     * We deactivate the rendering of the mouse cursor by VBox, as the intel driver of the Windows
     * VM renders the mouse cursor for the VM already.
     */
    pDrv->pfnVBVAMousePointerShape(pDrv, false, false, 0, 0, 0, 0, nullptr);
    return rc;
}

void VirtioGpuDisplayManager::handoverDriver()
{
    PVM pVM {PDMDevHlpGetVM(pDevIns)};
    PDMR3DriverDetach(pVM->pUVM, "virtio-gpu", 0, 0, NULL, 0, 0);
    PDMR3DriverAttach(pVM->pUVM, "vga", 0, 0, PDM_TACH_FLAGS_NOT_HOT_PLUG, NULL);

    pDrv = nullptr;
    pDrvBase = nullptr;
    ownDisplay = false;

    LogRel2(("VirtioGpuDisplayDriver: Display Port Driver detached\n"));
}

void VirtioGpuDisplayManager::detachAllDisplays()
{
    for (auto i {0ul}; i < displays.size(); ++i) {
        detachDisplay(i);
    }
}

int VirtioGpuDisplayManager::resizeVBVA(uint32_t displayIndex, bool fResetInputMapping)
{
    AssertLogRelMsgReturn(isManaged(displayIndex),
                          ("VirtioGpuDisplayManager: UpdateVBVA: The Display %u is not managed! \n", displayIndex),
                          VERR_INVALID_PARAMETER);

    int rc {VERR_NOT_AVAILABLE};

    if (pDrv != nullptr and pDrv->pfnVBVAResize != nullptr) {
        AssertRelease(isManaged(displayIndex));

        auto& display {displays.at(displayIndex)};

        return pDrv->pfnVBVAResize(pDrv, &display.view, &display.screen, vram.data(), fResetInputMapping);
    }

    LogRel6(("VirtioGpuDisplayManager: tried to update VBVA for display %u. Return Code: %Rrc.\n", displayIndex, rc));

    return rc;
}

int VirtioGpuDisplayManager::enableVBVA(uint32_t displayIndex)
{
    AssertLogRelMsgReturn(isManaged(displayIndex),
                          ("VirtioGpuDisplayManager: EnableVBVA: The display %u is not managed! \n", displayIndex),
                          VERR_INVALID_PARAMETER);

    int rc {VERR_NOT_AVAILABLE};

    if (pDrv != nullptr and pDrv->pfnVBVAEnable != nullptr) {
        rc = pDrv->pfnVBVAEnable(pDrv, displayIndex, 0);
    }

    LogRel6(("VirtioGpuDisplayManager: tried to enable VBVA for display %u. Return Code: %Rrc.\n", displayIndex, rc));

    return rc;
}

void VirtioGpuDisplayManager::disableVBVA(uint32_t displayIndex)
{
    AssertReleaseMsg(isManaged(displayIndex),
                     ("VirtioGpuDisplayManager: disableVBVA: The display %u is not managed! \n", displayIndex));

    if (pDrv != nullptr and pDrv->pfnVBVADisable != nullptr) {
        pDrv->pfnVBVADisable(pDrv, displayIndex);
        LogRel6(("VirtioGpuDisplayManager: disabled VBVA for display %u.\n", displayIndex));
    }
}
