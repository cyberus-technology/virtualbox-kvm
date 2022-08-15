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
#include "DevVirtioGpu.hpp"
#include "DevVirtioGpuVBoxStubs.hpp"

#include <VBox/log.h>
#include <VBox/vmm/pdmcommon.h>

#include <iprt/semaphore.h>
#include <iprt/string.h>

#include <array>
#include <numeric>

int VirtioGpuDevice::init(PPDMDEVINS pDevIns, int iInstance, uint32_t u32VRamSize, uint32_t cMonitorCount,
                          bool secondaryController)
{
    std::string sInstanceName = std::string {"VIRTIOGPU"} + std::to_string(iInstance);
    szInst = sInstanceName;

    int rc {VINF_SUCCESS};

    gpuConfig.uNumScanouts = cMonitorCount;

    rc = initializeVirtio(pDevIns);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    rc = initializeVirtQueues();
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    rc = initializeDisplay(u32VRamSize, cMonitorCount);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    pMemoryAdapter = std::make_unique<VirtioGpuMemoryAdapter>(virtio.pDevInsR3);
    AssertLogRelReturn(pMemoryAdapter != nullptr, VERR_NO_MEMORY);

    pCmdHandler = std::make_unique<VirtioGpuCmdHandler>(*pVirtioAdapter, *pDisplayManager, *pMemoryAdapter,
                                                        cMonitorCount, secondaryController);
    AssertLogRelReturn(pCmdHandler != nullptr, VERR_NO_MEMORY);

    return rc;
}

int VirtioGpuDevice::initializeVirtio(PPDMDEVINS pDevIns)
{
    VIRTIOPCIPARAMS virtioPciParams;
    virtioPciParams.uDeviceId = virtioGpu::PCI_DEVICE_ID;
    virtioPciParams.uSubsystemId =
        virtioGpu::PCI_DEVICE_ID; /* Virtio 1.2 - 4.1.2.1 subsystem id may reflect device id */
    virtioPciParams.uClassBase = virtioGpu::PCI_CLASS_BASE;
    virtioPciParams.uClassSub = virtioGpu::PCI_CLASS_SUB;
    virtioPciParams.uClassProg = virtioGpu::PCI_CLASS_PROG;

    virtioPciParams.uInterruptLine = virtioGpu::PCI_INTERRUPT_LINE;
    virtioPciParams.uInterruptPin = virtioGpu::PCI_INTERRUPT_PIN;

    PVIRTIOGPUDEVCC pThisCC {PDMDEVINS_2_DATA_CC(pDevIns, PVIRTIOGPUDEVCC)};

    pThisCC->virtio.pfnStatusChanged = virtioGpuStatusChanged;
    pThisCC->virtio.pfnDevCapRead = virtioGpuDevCapRead;
    pThisCC->virtio.pfnDevCapWrite = virtioGpuDevCapWrite;
    pThisCC->virtio.pfnVirtqNotified = virtioGpuVirtqNotified;

    int rc = virtioCoreR3Init(pDevIns, &this->virtio, &pThisCC->virtio, &virtioPciParams, szInst.c_str(),
                              FEATURES_OFFERED, 0, &this->gpuConfig, sizeof(gpuConfig));
    AssertLogRelReturn(RT_SUCCESS(rc), rc);

    pVirtioAdapter = std::make_unique<VirtioCoreVirtioAdapter>(&virtio);
    AssertLogRelReturn(pVirtioAdapter != nullptr, VERR_NO_MEMORY);

    return rc;
}

int VirtioGpuDevice::initializeVirtQueues()
{
    for (size_t uVirtqNbr {0u}; uVirtqNbr < virtioGpu::NUM_VIRTQUEUES; uVirtqNbr++) {
        PVIRTIOGPU_VIRTQ pVirtq {&aVirtqs[uVirtqNbr]};
        PVIRTIOGPU_WORKER pWorker {&aWorkers[uVirtqNbr]};

        int rc = RTSemEventCreate(&pWorker->hEvent);
        AssertLogRelReturn(RT_SUCCESS(rc), rc);

        std::string sVirtqName {uVirtqNbr == virtioGpu::VirtqIdx::CONTROLQ ? std::string {"controlq"}
                                                                           : std::string {"cursorq"}};
        pVirtq->szName = sVirtqName;
        pVirtq->uIdx = uVirtqNbr;
        pWorker->uIdx = uVirtqNbr;
    }
    return VINF_SUCCESS;
}

int VirtioGpuDevice::initializeDisplay(uint32_t u32VRamSize, uint32_t u32MonitorCount)
{
    PPDMDEVINS pDevIns {virtio.pDevInsR3};

    pDevIns->IBase.pfnQueryInterface = virtioGpuQueryInterface;
    IBase.pfnQueryInterface = virtioGpuPortQueryInterface;
    IPort.pfnUpdateDisplay = virtioGpuUpdateDisplay;

    IPort.pfnUpdateDisplayAll = virtioGpuPortUpdateDisplayAll;
    IPort.pfnQueryVideoMode = virtioGpuPortQueryVideoMode;
    IPort.pfnSetRefreshRate = virtioGpuPortSetRefreshRate;
    IPort.pfnTakeScreenshot = virtioGpuPortTakeScreenshot;
    IPort.pfnFreeScreenshot = virtioGpuPortFreeScreenshot;
    IPort.pfnDisplayBlt = virtioGpuPortDisplayBlt;
    IPort.pfnUpdateDisplayRect = virtioGpuPortUpdateDisplayRect;
    IPort.pfnCopyRect = virtioGpuPortCopyRect;
    IPort.pfnSetRenderVRAM = virtioGpuPortSetRenderVRAM;
    // used for SVGA only
    IPort.pfnSetViewport = NULL;
    IPort.pfnSendModeHint = vbvavirtioGpuPortSendModeHint;
    IPort.pfnReportHostCursorCapabilities = vbvavirtioGpuPortReportHostCursorCapabilities;
    IPort.pfnReportHostCursorPosition = vbvavirtioGpuPortReportHostCursorPosition;

    IVirtioGpuPort.pfnDisplayChanged = virtioGpuDisplayChanged;

    pDisplayManager =
        std::make_unique<VirtioGpuDisplayManager>(pDevIns, 0 /*iLUN*/, IBase, u32VRamSize, u32MonitorCount);
    AssertLogRelReturn(pDisplayManager != nullptr, VERR_NO_MEMORY);
    return VINF_SUCCESS;
}

int VirtioGpuDevice::terminate(PPDMDEVINS)
{
    int rc {VINF_SUCCESS};

    stop();
    for (size_t uVirtqNbr {0u}; uVirtqNbr < virtioGpu::NUM_VIRTQUEUES; uVirtqNbr++) {
        rc = RTSemEventDestroy(aWorkers[uVirtqNbr].hEvent);
        AssertLogRelReturn(RT_SUCCESS(rc), rc);
    }

    pCmdHandler.reset();
    pVirtioAdapter.reset();
    pDisplayManager.reset();
    pMemoryAdapter.reset();

    return rc;
}

int VirtioGpuDevice::start()
{
    fNegotiatedFeatures = virtioCoreGetNegotiatedFeatures(&virtio);
    return startVirtQueues();
}

int VirtioGpuDevice::stop()
{
    int rc {stopVirtQueues()};

    gpuConfig.uEventsRead = 0;
    gpuConfig.uEventsClear = 0;

    pCmdHandler->clearResources();
    return rc;
}

/*
 * virtioMmioRead and virtioMmioWrite both return VINF_IOM_MMIO_UNUSED_00 in case
 * of a bad access, thus we use this return value too.
 */

int VirtioGpuDevice::accessCap(uint32_t uOffset, std::function<void(uint32_t*)> accessFn)
{
    switch (uOffset) {
    case 0: accessFn(&gpuConfig.uEventsRead); return VINF_SUCCESS;
    case 4:
        accessFn(&gpuConfig.uEventsClear);
        /*
         * uEventsRead has write-to-clear semantics, i.e. when the driver writes
         * a bit to uEventsClear, we clear the bit in uEventsRead and clear uEventsClear
         */
        gpuConfig.uEventsRead &= ~gpuConfig.uEventsClear;
        gpuConfig.uEventsClear = 0u;
        return VINF_SUCCESS;
    case 8: accessFn(&gpuConfig.uNumScanouts); return VINF_SUCCESS;
    case 12: accessFn(&gpuConfig.uNumCapsets); return VINF_SUCCESS;
    default:
        LogRel(("%s: Invalid offset while accessing capabilties: %u\n", szInst.c_str(), uOffset));
        return VINF_IOM_MMIO_UNUSED_00;
    }
}

/*
 * Virtio 1.2 - 4.1.3.1: For device configuration access, the driver MUST use [...]
 * 32-bit wide and aligned accesses for 32-bit and 64-bit wide fields.
 */
int VirtioGpuDevice::readCap(uint32_t uOffset, void* pvBuf, uint32_t cbToRead)
{
    if (pvBuf == nullptr) {
        LogRel(("%s: readCap: buffer to write to is a nullptr.\n", szInst.c_str()));
        return VINF_IOM_MMIO_UNUSED_00;
    }
    if (cbToRead != sizeof(uint32_t)) {
        LogRel(("%s: readCap: invalid access size. Tried to read %u bytes.\n", szInst.c_str(), cbToRead));
        return VINF_IOM_MMIO_UNUSED_00;
    }

    std::function<void(uint32_t*)> accessFn = [pvBuf](uint32_t* pMember) { *static_cast<uint32_t*>(pvBuf) = *pMember; };

    return accessCap(uOffset, accessFn);
}

int VirtioGpuDevice::writeCap(uint32_t uOffset, const void* pvBuf, uint32_t cbToWrite)
{
    if (pvBuf == nullptr) {
        LogRel(("%s: writeCap: buffer to write to is a nullptr.\n", szInst.c_str()));
        return VINF_IOM_MMIO_UNUSED_00;
    }
    if (cbToWrite != sizeof(uint32_t)) {
        LogRel(("%s: writeCap: invalid access size. Tried to write %u bytes.\n", szInst.c_str(), cbToWrite));
        return VINF_IOM_MMIO_UNUSED_00;
    }
    if (uOffset != 4) {
        /* the driver is only allowed to write to uEventsClear */
        LogRel(("%s: writeCap: invalid access: the driver may only write to offset 4 (offset was %u).\n",
                szInst.c_str(), uOffset));
        return VINF_IOM_MMIO_UNUSED_00;
    }

    std::function<void(uint32_t*)> accessFn = [pvBuf](uint32_t* pMember) {
        *pMember = *static_cast<const uint32_t*>(pvBuf);
    };

    return accessCap(uOffset, accessFn);
}

void VirtioGpuDevice::displayChanged(uint32_t numDisplays, VMMDevDisplayDef* displayDefs)
{
    for (uint32_t i = 0; i < numDisplays; i++) {
        bool enabled {not(displayDefs[i].fDisplayFlags & VMMDEV_DISPLAY_DISABLED)};

        pCmdHandler->requestResize(i, enabled, displayDefs[i].cx, displayDefs[i].cy);
    }

    gpuConfig.uEventsRead |= virtioGpu::EVENT_DISPLAY;
    LogRel5(("%s: device configuration has changed.\n", szInst.c_str()));

    virtioCoreNotifyConfigChanged(&virtio);
}

void VirtioGpuDevice::wakeupWorker(uint16_t uVirtqNbr)
{
    if (uVirtqNbr != virtioGpu::VirtqIdx::CONTROLQ and uVirtqNbr != virtioGpu::VirtqIdx::CURSORQ) {
        LogRel(("%s: tried to wake up unrecognized queue number: %d.\n", szInst.c_str(), uVirtqNbr));
        return;
    }

    PVIRTIOGPU_WORKER pWorker {&aWorkers[uVirtqNbr]};

    if (not ASMAtomicXchgBool(&pWorker->fNotified, true)) {
        /*
         * Two atomic variables to avoid (at least some) unnecessary signals.
         * The fNotified variable should suffice to avoid lost signals.
         */
        if (ASMAtomicReadBool(&pWorker->fSleeping)) {
            int rc {RTSemEventSignal(pWorker->hEvent)};
            AssertRC(rc);
        }
    }
}

template <uint16_t uVirtqNbr>
static DECLCALLBACK(int) virtQueueHandleFn(RTTHREAD /*hSelf*/, void* pvUser)
{
    PVIRTIOGPUDEV pThis {static_cast<PVIRTIOGPUDEV>(pvUser)};
    return pThis->handleVirtQueue(uVirtqNbr);
}

auto controlQueueHandleFn {virtQueueHandleFn<virtioGpu::VirtqIdx::CONTROLQ>};
auto cursorQueueHandleFn {virtQueueHandleFn<virtioGpu::VirtqIdx::CURSORQ>};

int VirtioGpuDevice::startVirtQueues()
{
    fTerminateVirtQueues.store(false);

    for (size_t uVirtqNbr {0u}; uVirtqNbr < virtioGpu::NUM_VIRTQUEUES; uVirtqNbr++) {
        PVIRTIOGPU_VIRTQ pVirtq {&aVirtqs[uVirtqNbr]};
        PVIRTIOGPU_WORKER pWorker {&aWorkers[uVirtqNbr]};

        auto handlerFn = uVirtqNbr == virtioGpu::VirtqIdx::CONTROLQ ? controlQueueHandleFn : cursorQueueHandleFn;
        int rc {RTThreadCreate(&pWorker->hThread, handlerFn, this, 0, RTTHREADTYPE_IO, RTTHREADFLAGS_WAITABLE,
                               pVirtq->szName.c_str())};
        AssertLogRelReturn(RT_SUCCESS(rc), rc);
        pWorker->fAssigned = true;

        rc = virtioCoreR3VirtqAttach(&virtio, pVirtq->uIdx, pVirtq->szName.c_str());
        AssertLogRelReturn(RT_SUCCESS(rc), rc);
        virtioCoreVirtqEnableNotify(&virtio, pVirtq->uIdx, true);
        pVirtq->fAttachedToVirtioCore = true;
    }

    return VINF_SUCCESS;
}

int VirtioGpuDevice::stopVirtQueues()
{
    fTerminateVirtQueues.store(true);

    for (size_t uVirtqNbr {0u}; uVirtqNbr < virtioGpu::NUM_VIRTQUEUES; uVirtqNbr++) {
        PVIRTIOGPU_VIRTQ pVirtq {&aVirtqs[uVirtqNbr]};
        PVIRTIOGPU_WORKER pWorker {&aWorkers[uVirtqNbr]};

        if (not pWorker->fAssigned) {
            continue;
        }

        int rc = RTSemEventSignal(pWorker->hEvent);
        AssertLogRelReturn(RT_SUCCESS(rc), rc);

        rc = RTThreadWaitNoResume(pWorker->hThread, RT_INDEFINITE_WAIT, nullptr);
        AssertLogRelReturn(RT_SUCCESS(rc), rc);

        pWorker->fAssigned = false;
        pVirtq->fAttachedToVirtioCore = false;
    }

    return VINF_SUCCESS;
}

int VirtioGpuDevice::handleVirtQueue(uint16_t uVirtqNbr)
{
    PVIRTIOGPU_VIRTQ pVirtq {&aVirtqs[uVirtqNbr]};
    PVIRTIOGPU_WORKER pWorker {&aWorkers[uVirtqNbr]};
    PPDMDEVINS pDevIns {virtio.pDevInsR3};

    LogRel2(("%s: worker thread %d started for %s (virtq idx=%d).\n", szInst.c_str(), pWorker->uIdx,
             pVirtq->szName.c_str(), pVirtq->uIdx));

    auto is_virtq_empty = [this, &pDevIns, uVirtqNbr]() -> bool {
        return virtioCoreVirtqAvailBufCount(pDevIns, &virtio, uVirtqNbr) == 0;
    };

    while (not fTerminateVirtQueues.load()) {
        if (is_virtq_empty()) {
            /*
             * Two atomic variables to avoid (at least some) unnecessary signals.
             * The fNotified variable should suffice to avoid lost signals.
             */
            ASMAtomicWriteBool(&pWorker->fSleeping, true);
            if (not ASMAtomicXchgBool(&pWorker->fNotified, false)) {
                int rc {RTSemEventWait(pWorker->hEvent, RT_INDEFINITE_WAIT)};
                AssertLogRelReturn(RT_SUCCESS(rc) or rc == VERR_INTERRUPTED, rc);

                if (rc == VERR_INTERRUPTED) {
                    continue;
                }

                ASMAtomicWriteBool(&pWorker->fNotified, false);
            }
            ASMAtomicWriteBool(&pWorker->fSleeping, false);
        }

        if (RT_UNLIKELY(fTerminateVirtQueues.load())) {
            break;
        }

        if (is_virtq_empty()) {
            /*
             * It may happen that we got an unnecessary signal, thus we double-check
             * wether the virtq is empty.
             */
            continue;
        }

#ifdef VIRTIO_VBUF_ON_STACK
        PVIRTQBUF pVirtqBuf = virtioCoreR3VirtqBufAlloc();
        if (!pVirtqBuf) {
            LogRel(("Failed to allocate memory for VIRTQBUF\n"));
            break; /* No point in trying to allocate memory for other descriptor
                      chains */
        }
        int rc {virtioCoreR3VirtqAvailBufGet(pDevIns, &virtio, pVirtq->uIdx, pVirtqBuf, true)};
#else
        // The virtq is not empty, we take a buffer from it and handle it.
        PVIRTQBUF pVirtqBuf {nullptr};
        int rc {virtioCoreR3VirtqAvailBufGet(pDevIns, &virtio, pVirtq->uIdx, &pVirtqBuf, true)};
#endif

        if (rc == VERR_NOT_AVAILABLE) {
            continue;
        }

        pCmdHandler->handleBuffer(pVirtqBuf);
        virtioCoreR3VirtqBufRelease(&virtio, pVirtqBuf);
    }

    return VINF_SUCCESS;
}
