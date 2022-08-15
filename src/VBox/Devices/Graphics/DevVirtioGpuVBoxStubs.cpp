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
#include "DevVirtioGpuVBoxStubs.hpp"
#include "DevVirtioGpu.hpp"

#include <VBox/log.h>
#include <VBox/msi.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pdmdev.h>

#ifdef IN_RING3
#    include <iprt/uuid.h>
#endif

static DECLCALLBACK(int) devVirtioGpuConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    // Check that the device instance and device helper structures are compatible.
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    PVIRTIOGPUDEV pThis {PDMDEVINS_2_DATA(pDevIns, PVIRTIOGPUDEV)};

    int rc {VINF_SUCCESS};
    bool secondaryController {false};
    unsigned cMonitorCount {0};
    uint32_t u32VRamSize {0};

    constexpr char validation[] = "secondaryController"
                                  "|MonitorCount"
                                  "|VRamSize";

    PDMDEV_VALIDATE_CONFIG_RETURN(pDevIns, validation, "Invalid Configuration");

    rc = pDevIns->pHlpR3->pfnCFGMQueryBoolDef(pCfg, "secondaryController", &secondaryController, false);
    if (RT_FAILURE(rc)) {
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Condfiguration error: Querying secondaryController asa a bool failed"));
    }

    rc = pDevIns->pHlpR3->pfnCFGMQueryU32Def(pCfg, "MonitorCount", &cMonitorCount, 1);
    if (RT_FAILURE(rc)) {
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Configuration error: Querying MonitorCount as uint32_t failed"));
    }

    rc = pDevIns->pHlpR3->pfnCFGMQueryU32Def(pCfg, "VRamSize", &u32VRamSize, 32 * _1M);
    if (RT_FAILURE(rc)) {
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Configuration error: Querying VRAM Size as uin32_t failed"));
    }

    rc = pThis->init(pDevIns, iInstance, u32VRamSize, cMonitorCount, secondaryController);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) devVirtioGpuDestruct(PPDMDEVINS pDevIns)
{
    // Check that the device instance and device helper structures are compatible again.
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    PVIRTIOGPUDEV pThis {PDMDEVINS_2_DATA(pDevIns, PVIRTIOGPUDEV)};

    int rc {pThis->terminate(pDevIns)};
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    return VINF_SUCCESS;
}

static DECLCALLBACK(void) devVirtioGpuReset(PPDMDEVINS pDevIns)
{
    PVIRTIOGPUDEV pThis {PDMDEVINS_2_DATA(pDevIns, PVIRTIOGPUDEV)};
    pThis->pDisplayManager->reset();
    pThis->pDisplayManager->handoverDriver();
}

static DECLCALLBACK(int) devVirtioGpuAttach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t)
{
    PVIRTIOGPUDEV pThis {PDMDEVINS_2_DATA(pDevIns, PVIRTIOGPUDEV)};

    /* we only support iLUN == 0 at the moment */
    if (iLUN != 0) {
        AssertLogRelMsgFailed(("Invalid LUN #%d\n", iLUN));
        return VERR_PDM_NO_SUCH_LUN;
    }

    int rc {pThis->pDisplayManager->takeoverDriver()};
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    return VINF_SUCCESS;
}

static DECLCALLBACK(void) devVirtioGpuDetach(PPDMDEVINS pDevIns, unsigned, uint32_t)
{
    PVIRTIOGPUDEV pThis {PDMDEVINS_2_DATA(pDevIns, PVIRTIOGPUDEV)};
    pThis->pDisplayManager->detachAllDisplays();
}

/**
 * Device registration structure.
 */
extern "C" const PDMDEVREG g_DeviceVirtioGpuDev = {
    /* .u32Version = */ PDM_DEVREG_VERSION,
    /* .uReserved0 = */ 0,
    /* .szName = */ "virtio-gpu",
    /* .fFlags = */ PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_NEW_STYLE,
    /* .fClass = */ PDM_DEVREG_CLASS_GRAPHICS,
    /* .cMaxInstances = */ 1u,
    /* .uSharedVersion = */ 42,
    /* .cbInstanceShared = */ sizeof(VIRTIOGPUDEV),
    /* .cbInstanceR0 = */ sizeof(VIRTIOGPUDEVCC),
    /* .cbInstanceRC = */ 0,
    /* .cMaxPciDevices = */ 1,
    /* .cMaxMsixVectors = */ VBOX_MSIX_MAX_ENTRIES,
    /* .pszDescription = */ "Virtio Host GPU.\n",
    /* .pszRCMod = */ "",
    /* .pszR0Mod = */ "",
    /* .pfnConstruct = */ devVirtioGpuConstruct,
    /* .pfnDestruct = */ devVirtioGpuDestruct,
    /* .pfnRelocate = */ NULL,
    /* .pfnMemSetup = */ NULL,
    /* .pfnPowerOn = */ NULL,
    /* .pfnReset = */ devVirtioGpuReset,
    /* .pfnSuspend = */ NULL,
    /* .pfnResume = */ NULL,
    /* .pfnAttach = */ devVirtioGpuAttach,
    /* .pfnDetach = */ devVirtioGpuDetach,
    /* .pfnQueryInterface. = */ NULL,
    /* .pfnInitComplete = */ NULL,
    /* .pfnPowerOff = */ NULL,
    /* .pfnSoftReset = */ NULL,
    /* .pfnReserved0 = */ NULL,
    /* .pfnReserved1 = */ NULL,
    /* .pfnReserved2 = */ NULL,
    /* .pfnReserved3 = */ NULL,
    /* .pfnReserved4 = */ NULL,
    /* .pfnReserved5 = */ NULL,
    /* .pfnReserved6 = */ NULL,
    /* .pfnReserved7 = */ NULL,
    /* .u32VersionEnd = */ PDM_DEVREG_VERSION};

DECLCALLBACK(void) virtioGpuStatusChanged(PVIRTIOCORE pVirtio, PVIRTIOCORECC, uint32_t fDriverOk)
{
    PVIRTIOGPUDEV pThis {RT_FROM_MEMBER(pVirtio, VIRTIOGPUDEV, virtio)};

    if (fDriverOk != 0) {
        int rc = pThis->start();
        AssertLogRel(RT_SUCCESS(rc));
    } else {
        int rc = pThis->stop();
        AssertLogRel(RT_SUCCESS(rc));
    }
}

DECLCALLBACK(int) virtioGpuDevCapRead(PPDMDEVINS pDevIns, uint32_t uOffset, void* pvBuf, uint32_t cbToRead)
{
    PVIRTIOGPUDEV pThis {PDMDEVINS_2_DATA(pDevIns, PVIRTIOGPUDEV)};
    return pThis->readCap(uOffset, pvBuf, cbToRead);
}

DECLCALLBACK(int) virtioGpuDevCapWrite(PPDMDEVINS pDevIns, uint32_t uOffset, const void* pvBuf, uint32_t cbToWrite)
{
    PVIRTIOGPUDEV pThis {PDMDEVINS_2_DATA(pDevIns, PVIRTIOGPUDEV)};
    return pThis->writeCap(uOffset, pvBuf, cbToWrite);
}

DECLCALLBACK(void) virtioGpuVirtqNotified(PPDMDEVINS pDevIns, PVIRTIOCORE, uint16_t uVirtqNbr)
{
    PVIRTIOGPUDEV pThis {PDMDEVINS_2_DATA(pDevIns, PVIRTIOGPUDEV)};
    pThis->wakeupWorker(uVirtqNbr);
}

DECLCALLBACK(void)
virtioGpuDisplayChanged(PPDMDEVINS pDevIns, uint32_t numDisplays, VMMDevDisplayDef* displayDefs)
{
    PVIRTIOGPUDEV pThis {PDMDEVINS_2_DATA(pDevIns, PVIRTIOGPUDEV)};
    pThis->displayChanged(numDisplays, displayDefs);
}

DECLCALLBACK(void*) virtioGpuQueryInterface(PPDMIBASE pInterface, const char* pszIID)
{
    PPDMDEVINS pDevIns {PDMIBASE_2_PDMDEV(pInterface)};
    PVIRTIOGPUDEV pThis {PDMDEVINS_2_DATA(pDevIns, PVIRTIOGPUDEV)};
    LogRel8(("%s: virtioGpuQueryInterface.\n", pThis->szInst.c_str()));
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIVIRTIOGPUPORT, &pThis->IVirtioGpuPort);
    return nullptr;
}

DECLCALLBACK(void*) virtioGpuPortQueryInterface(PPDMIBASE pInterface, const char* pszIID)
{
    PVIRTIOGPUDEV pThis {RT_FROM_MEMBER(pInterface, VIRTIOGPUDEV, IBase)};
    LogRel8(("%s: virtioGpuPortQueryInterface.\n", pThis->szInst.c_str()));
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIDISPLAYPORT, &pThis->IPort);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIDISPLAYVBVACALLBACKS, &pThis->IVBVACallbacks);
    return nullptr;
}

static PVIRTIOGPUDEV virtioGpuFromPPDMIDISPLAYPORT(PPDMIDISPLAYPORT pInterface)
{
    return reinterpret_cast<PVIRTIOGPUDEV>(reinterpret_cast<uintptr_t>(pInterface) - RT_OFFSETOF(VIRTIOGPUDEV, IPort));
}

DECLCALLBACK(void) virtioGpuPortSetRenderVRAM(PPDMIDISPLAYPORT pInterface, bool)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: virtioGpuPortSetRenderVRAM.\n", pThis->szInst.c_str()));
}

DECLCALLBACK(int) virtioGpuUpdateDisplay(PPDMIDISPLAYPORT pInterface)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: virtioGpuUpdateDisplay.\n", pThis->szInst.c_str()));
    return VINF_SUCCESS;
}

DECLCALLBACK(int) virtioGpuPortUpdateDisplayAll(PPDMIDISPLAYPORT pInterface, bool)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: virtioGpuPortUpdateDisplayAll.\n", pThis->szInst.c_str()));
    return VINF_SUCCESS;
}

DECLCALLBACK(int)
virtioGpuPortQueryVideoMode(PPDMIDISPLAYPORT pInterface, uint32_t* pcBits, uint32_t* pcx, uint32_t* pcy)
{
    PVIRTIOGPUDEV pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: virtioGpuPortQueryVideoMode. pcBits: %u, pcx: %u, pcy: %u.\n", pThis->szInst.c_str(), *pcBits, *pcx,
             *pcy));

    if (!pcBits) {
        return VERR_INVALID_PARAMETER;
    }

    *pcBits = 0;

    // CYBER-TODO: This should not always be scanout 0
    // When we have figured out how to handle multiple VBox-Windows, we have to
    // figure out how to get the index of the scanout here.
    auto maybeScanout {pThis->pCmdHandler->getCScanout(0)};
    if (not maybeScanout.has_value()) {
        return VINF_SUCCESS;
    }

    const auto currentScanout {maybeScanout->get()};
    if (pcx) {
        *pcx = currentScanout.uCurrentWidth;
    }

    if (pcy) {
        *pcy = currentScanout.uCurrentHeight;
    }

    return VINF_SUCCESS;
}

DECLCALLBACK(int) virtioGpuPortSetRefreshRate(PPDMIDISPLAYPORT pInterface, uint32_t)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: virtioGpuPortSetRefreshRate.\n", pThis->szInst.c_str()));
    return VINF_SUCCESS;
}

DECLCALLBACK(int) virtioGpuPortTakeScreenshot(PPDMIDISPLAYPORT pInterface, uint8_t**, size_t*, uint32_t*, uint32_t*)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: virtioGpuPortTakeScreenshot.\n", pThis->szInst.c_str()));
    return VINF_SUCCESS;
}

DECLCALLBACK(void) virtioGpuPortFreeScreenshot(PPDMIDISPLAYPORT pInterface, uint8_t*)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: virtioGpuPortFreeScreenshot.\n", pThis->szInst.c_str()));
}

DECLCALLBACK(void) virtioGpuPortUpdateDisplayRect(PPDMIDISPLAYPORT pInterface, int32_t, int32_t, uint32_t, uint32_t)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: virtioGpuPortUpdateDisplayRect.\n", pThis->szInst.c_str()));
}

DECLCALLBACK(int)
virtioGpuPortDisplayBlt(PPDMIDISPLAYPORT pInterface, const void*, uint32_t, uint32_t, uint32_t, uint32_t)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: virtioGpuPortDisplayBlt.\n", pThis->szInst.c_str()));
    return VINF_SUCCESS;
}

DECLCALLBACK(int)
virtioGpuPortCopyRect(PPDMIDISPLAYPORT pInterface, uint32_t, uint32_t, const uint8_t*, int32_t, int32_t, uint32_t,
                      uint32_t, uint32_t, uint32_t, uint8_t*, int32_t, int32_t, uint32_t, uint32_t, uint32_t, uint32_t)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: virtioGpuPortCopyRect.\n", pThis->szInst.c_str()));
    return VINF_SUCCESS;
}

DECLCALLBACK(void)
vmsvgavirtioGpuPortSetViewport(PPDMIDISPLAYPORT pInterface, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: SetViewport\n\n\n\n\n", pThis->szInst.c_str()));
}

DECLCALLBACK(int)
vbvavirtioGpuPortSendModeHint(PPDMIDISPLAYPORT pInterface, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                              uint32_t, uint32_t)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: vbvavirtioGpuPortSendModeHint.\n", pThis->szInst.c_str()));
    return VINF_SUCCESS;
}

DECLCALLBACK(void) vbvavirtioGpuPortReportHostCursorCapabilities(PPDMIDISPLAYPORT pInterface, bool, bool)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: vbvavirtioGpuPortReportHostCursorCapabilities.\n", pThis->szInst.c_str()));
}

DECLCALLBACK(void) vbvavirtioGpuPortReportHostCursorPosition(PPDMIDISPLAYPORT pInterface, uint32_t, uint32_t, bool)
{
    [[maybe_unused]] auto pThis {virtioGpuFromPPDMIDISPLAYPORT(pInterface)};
    LogRel8(("%s: vbvavirtioGpuPortReportHostCursorPosition.\n", pThis->szInst.c_str()));
}
