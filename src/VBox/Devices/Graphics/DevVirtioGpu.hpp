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

#include <iprt/mem.h>

#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/vm.h>

#include "../VirtIO/VirtioCore.h"
#include "DevVirtioGpuDefinitions.hpp"
#include "DevVirtioGpuDisplayManager.hpp"
#include "DevVirtioGpuCmdHandler.hpp"
#include "DevVirtioGpuResource.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

class VirtioCoreVirtioAdapter;
class VirtioGpuMemoryAdapter;

/**
 * Logging-Level rules for anything inside LOG_GROUP_DEV_VIRTIO_GPU:
 *   LogRel  - conditions that lead to functions returning early or returning anything other than VINF_SUCCESS
 *   LogRel2 - logging of things that should only happen once or very few, e.g. creation of a queue or taking over the
 * driver LogRel5 - informative logging from the virtio-gpu (only DevVirtioGpu.hpp and DevVirtioGpu.cpp, but not the
 * adapters) LogRel6 - informative logging from any adapter LogRel7 - informative logging from the cmd-handler LogRel8 -
 * informative logging inside the VBox-Stubs (DevVirtioGpuVBoxStubs.hpp and DevVirtioGpuVBoxStubs.cpp)
 *
 * Enabling logging levels 1 and 2 shouldn't lead to too much output (common sense applies), while the other logging
 * levels may lead to a lot of output.
 *
 * If you have access to VirtioGpuDevice::szInst, start your messages with ("%s: ...", virtioGpuDevice.szInst.c_str()).
 * Otherwise start your messages with a prefix for easy grepping.
 *
 * Enable logging for only the virtio-gpu with: 'export VBOX_RELEASE_LOG="-all+dev_virtio_gpu.e.lA.lB" where A and B are
 * the desired logging levels. You can of course add more logging levels with ".lC.lD...". ".e" automatically enables
 * logging level 1, i.e. LogRel.
 *
 */

class VirtioGpuDevice
{
    static constexpr uint16_t FEATURES_OFFERED {virtioGpu::Features::EDID}; ///< The features offered to the guest
                                                                            ///
public:
    /**
     * Initialize the Virtio GPU
     *
     * \param pDevIns The PCI Device Instance
     * \param iInstance The instance number.
     * \param u32VRamSize The size of the VRam.
     * \param cMonitorCount The amount of displays configured for the VM.
     * \param secondaryController True if this is a secondary graphics controller, e.g. if the active graphics
     * controller is VGAWithVirtioGpu, false if this is the only graphics controller.
     *
     * \return VBox status code
     */
    int init(PPDMDEVINS pDevIns, int iInstance, uint32_t u32VRamSize, uint32_t cMonitorCount, bool secondaryController);

    /**
     * Terminates the Virtio GPU.
     *
     * \param pDevIns The PCI Device Instance
     *
     * \return VBox status code
     */
    int terminate(PPDMDEVINS pDevIns);

    /**
     * Start the Virtio GPU. This function is called when the driver calls
     * pfnStatusChanged with fDriverOk != 0.
     *
     * \return VBox Status Code
     */
    int start();

    /**
     * Stop the Virtio GPU. This function is called when the driver calls
     * pfnStatusChanged with fDriverOk == 0.
     *
     * \return VBox Status Code
     */
    int stop();

    /**
     * Read from the device-specific configuration.
     *
     * \param uOffset The offset into the device-specific configuration
     * \param pvBuf The buffer in which to save the read data
     * \param cbToRead The number of bytes to read
     *
     * \return VBox Status Code
     */
    int readCap(uint32_t uOffset, void* pvBuf, uint32_t cbToRead);

    /**
     * Write to the device-specific configuration.
     *
     * \param uOffset The offset into the device-specific configuration
     * \param pvBuf The buffer with the bytes to write
     * \param cbToWrite The number of bytes to write
     *
     * \return VBox Status Code
     */
    int writeCap(uint32_t uOffset, const void* pvBuf, uint32_t cbToWrite);

    /**
     * Informs the worker of a virtqueue that it has new buffers.
     *
     * \param uVirtqNbr The number of the virtqueue that has new buffers.
     */
    void wakeupWorker(uint16_t uVirtqNbr);

    /**
     * The handler function for the virtqueues.
     *
     * \param uVirtqNbr The index of the associcated virtqueue
     * \param pDevIns The PCI Device Instance
     *
     * \return VBox status code
     */
    int handleVirtQueue(uint16_t uVirtqNbr);

private:
    /**
     * Initialize the Virtio-Core part of the Virtio GPU
     *
     * \param pDevIns The PCI Device Instance
     *
     * \return VBox status code
     */
    int initializeVirtio(PPDMDEVINS pDevIns);

    /**
     * Initialize the virtqueues, but do NOT start them.
     *
     * \return VBox status code.
     */
    int initializeVirtQueues();

    /**
     * Initializes the display, i.e. assigns functions to the driver etc.
     *
     * \param u32VRamSize The size of the VRam
     * \param u32MonitorCount The maximum of attachable monitors
     *
     * \return VBox status code.
     */
    int initializeDisplay(uint32_t u32VRamSize, uint32_t u32MonitorCount);

    /**
     * Start the virtqueues, i.e. start the worker-threads and attach the
     * virtqueues to virtio core.
     *
     * \return VBox status code
     */
    int startVirtQueues();

    /**
     * Stop the virtqueues, i.e. stop the worker-threads.
     *
     * \return VBox status code
     */
    int stopVirtQueues();

    /**
     * Accesses the device-specific configuration at the given offset using the
     * given function.
     *
     * \param uOffset The offset into the device-specific configuration
     * \param accessFn The function that is used to access the device-specific configuration
     *
     * \return VBox status code
     */
    int accessCap(uint32_t uOffset, std::function<void(uint32_t*)> accessFn);

public:
    /* device-specific queue info */
    typedef struct VirtioGpuVirtQueue
    {
        uint16_t uIdx; ///< The index of this virtqueue
        uint16_t uPadding;
        std::string szName;         ///< The name of this virtqueue
        bool fHasWorker;            ///< If set this virtqueue has an associated worker
        bool fAttachedToVirtioCore; ///< If set this virtqueue is attached to virtio core
    } VIRTIOGPU_VIRTQ, *PVIRTIOGPU_VIRTQ;

    /* a worker thread of a virtqueue */
    typedef struct VirtioGpuWorker
    {
        RTSEMEVENT hEvent;       ///< The handle of the associated sleep/wake-up semaphore
        RTTHREAD hThread;        ///< The handle of the associated worker-thread
        uint16_t uIdx;           ///< The index of this worker (should be the same as the index of the associated virtq)
        bool volatile fSleeping; ///< If set this thread is sleeping
        bool volatile fNotified; ///< If set this thread has been notified that there is work to do
        bool fAssigned;          ///< If set this thread has been set up
    } VIRTIOGPU_WORKER, *PVIRTIOGPU_WORKER;

public:
    /* virtio core requires that members are public.*/
    VIRTIOCORE virtio;           // core virtio state
    virtioGpu::Config gpuConfig; // device specific configuration of the virtio GPU

    VirtioGpuVirtQueue aVirtqs[virtioGpu::NUM_VIRTQUEUES];
    VirtioGpuWorker aWorkers[virtioGpu::NUM_VIRTQUEUES];
    std::atomic<bool> fTerminateVirtQueues;

    std::string szInst; // instance name

    uint64_t fNegotiatedFeatures; // features negotiated with the guest

    /* The commands send by the driver are handled by the VirtioGpuCmdHandler. To be able
     * to test this class using unit-tests, the handler needs a few adapters to be able to control
     * how pages are mapped, displays are handled and the commands are read.
     */
    std::unique_ptr<VirtioGpuCmdHandler> pCmdHandler;
    std::unique_ptr<VirtioCoreVirtioAdapter> pVirtioAdapter;
    std::unique_ptr<VirtioGpuDisplayManager> pDisplayManager;
    std::unique_ptr<VirtioGpuMemoryAdapter> pMemoryAdapter;

public:
    PDMIBASE IBase;
    PDMIDISPLAYPORT IPort;
    PDMIDISPLAYVBVACALLBACKS IVBVACallbacks;
    PDMIVIRTIOGPUPORT IVirtioGpuPort;

    /**
     * Signals to the driver that the resolution or the monitor status
     * (enabled, disabled) has changed.
     *
     * \param numDisplays Number of displays available
     * \param displayDefs Array of display definitions describing the
     *                    configuration of each display
     */
    void displayChanged(uint32_t numDisplays, VMMDevDisplayDef* displayDefs);

    /**
     * Attaches the Virtio-GPU to the VBox-window.
     *
     * \param iLUN The LUN to attach to. This must be 0.
     *
     * \return VBox status code
     */
    int attachDisplay(unsigned iLUN);

    /**
     * Detaches the Virtio-GPU driver.
     *
     * \param iLUN The LUN to detach from.
     */
    void detachDisplay(unsigned iLUN);
};

/**
 *  VirtioCore needs a separate class that holds the R3 state
 */
class VirtioGpuDeviceR3
{
public:
    VIRTIOCORER3 virtio; // core virtio state R3
};

typedef VirtioGpuDevice VIRTIOGPUDEV;
typedef VIRTIOGPUDEV* PVIRTIOGPUDEV;

typedef VirtioGpuDeviceR3 VIRTIOGPUDEVCC;
typedef VIRTIOGPUDEVCC* PVIRTIOGPUDEVCC;

class VirtioCoreVirtioAdapter final : public VirtioGpuCmdHandler::VirtioAdapter
{
    PVIRTIOCORE pVirtio_ {nullptr};

public:
    VirtioCoreVirtioAdapter() = delete;
    VirtioCoreVirtioAdapter(PVIRTIOCORE pVirtio) : pVirtio_(pVirtio) {}

    void virtqBufDrain(PVIRTQBUF pVirtqBuf, void* pv, size_t cb) final
    {
        virtioCoreR3VirtqBufDrain(pVirtio_, pVirtqBuf, pv, cb);
    }

    void virtqBufPut(PVIRTQBUF pVirtqBuf, void* pv, size_t cb) final
    {
        PRTSGSEG pReturnSeg = static_cast<PRTSGSEG>(RTMemAllocZ(sizeof(RTSGSEG)));
        AssertReleaseMsg(pReturnSeg != nullptr, ("Out of memory"));

        pReturnSeg->pvSeg = RTMemAllocZ(cb);
        AssertReleaseMsg(pReturnSeg->pvSeg != nullptr, ("Out of memory"));
        memcpy(pReturnSeg->pvSeg, pv, cb);
        pReturnSeg->cbSeg = cb;

        PRTSGBUF pReturnSegBuf = static_cast<PRTSGBUF>(RTMemAllocZ(sizeof(RTSGBUF)));
        AssertReleaseMsg(pReturnSegBuf, ("Out of memory"));

        RTSgBufInit(pReturnSegBuf, pReturnSeg, 1);

        virtioCoreR3VirtqUsedBufPut(pVirtio_->pDevInsR3, pVirtio_, pVirtqBuf->uVirtq, pReturnSegBuf, pVirtqBuf,
                                    true /* fFence */);

        RTMemFree(pReturnSeg->pvSeg);
        RTMemFree(pReturnSeg);
        RTMemFree(pReturnSegBuf);
    }

    void virtqSyncRings(PVIRTQBUF pVirtqBuf) final
    {
        virtioCoreVirtqUsedRingSync(pVirtio_->pDevInsR3, pVirtio_, pVirtqBuf->uVirtq);
    }
};

class VirtioGpuMemoryAdapter final : public VirtioGpuCmdHandler::MemoryAdapter
{
    PPDMDEVINS pDevIns_ {nullptr};

public:
    VirtioGpuMemoryAdapter() = delete;
    VirtioGpuMemoryAdapter(PPDMDEVINS pDevIns) : pDevIns_(pDevIns) {}

    VecMappings mapGCPhys2HCVirt(const VecMemEntries& vBacking)
    {
        VecMappings vMapping;
        vMapping.reserve(vBacking.size());

        for (const auto& backing : vBacking) {
            size_t currSize {backing.uLength_};
            /*
             * PDMDevHlpPhysGCPhys2CCPtr always maps exactly one page, thus it may
             * happen that we need multiple mappings for one backing-entry
             */
            while (currSize != 0) {
                PPGMPAGEMAPLOCK pLock {new PGMPAGEMAPLOCK};
                void* vAddr {nullptr};
                int rc {PDMDevHlpPhysGCPhys2CCPtr(pDevIns_, backing.uAddr_, 0, &vAddr, pLock)};
                AssertRC(rc);
                vMapping.emplace_back(vAddr, PAGE_SIZE, pLock);
                currSize -= PAGE_SIZE;
            }
        }

        return vMapping;
    }

    void releaseMappings(const VecMappings& vMapping)
    {
        for (const auto& mapping : vMapping) {
            PPGMPAGEMAPLOCK pLock {reinterpret_cast<PPGMPAGEMAPLOCK>(mapping.pv_)};
            PDMDevHlpPhysReleasePageMappingLock(pDevIns_, pLock);
            delete pLock;
        }
    }
};
