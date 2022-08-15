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
#include "DevVirtioGpuCmdHandler.hpp"

#include <iprt/assert.h>

#include <VBox/log.h>
#include <VBox/types.h>

#include <cyberus/edid.hpp>

#include <algorithm>
#include <cstring>

VirtioGpuCmdHandler::VirtioGpuCmdHandler(VirtioAdapter& vAdapter, DisplayManager& dManager, MemoryAdapter& mAdapter,
                                         uint32_t numScanouts, bool attachDisplayLater)
    : virtioAdapter_(vAdapter), displayManager_(dManager), memoryAdapter_(mAdapter), numScanouts_(numScanouts)
{
    for (unsigned currentScanout {0u}; currentScanout < numScanouts_; currentScanout++) {
        Scanout scanout {displayManager_};
        scanout.uScanoutId = currentScanout;

        /* if this is the only graphics controller we want to attach immediately to the display. */
        if (not attachDisplayLater and scanout.hasDisplay() and not scanout.isAttachedToDisplay()) {
            scanout.attachDisplay();
        }

        if (scanout.hasDisplay()) {
            auto [uWidth, uHeight] = scanout.isAttachedToDisplay() ? scanout.displayDimensions() : getDummySize();
            scanout.uCurrentWidth = uWidth;
            scanout.uCurrentHeight = uHeight;
        }

        activeScanouts.push_back(scanout);
    }

    LogRel2(("virtio-gpu cmd handler: created. Num of scanouts is %u.\n", activeScanouts.size()));
}

inline bool VirtioGpuCmdHandler::scanoutExists(uint32_t uScanout)
{
    return uScanout <= numScanouts_ - 1;
}

inline std::optional<VirtioGpuCmdHandler::ScanoutRef> VirtioGpuCmdHandler::getScanout(uint32_t uScanout)
{
    if (not scanoutExists(uScanout)) {
        return {};
    }

    return activeScanouts.at(uScanout);
}

inline std::vector<VirtioGpuCmdHandler::ScanoutRef> VirtioGpuCmdHandler::getScanoutsByResource(uint32_t uResourceId)
{
    std::vector<ScanoutRef> results;
    for (auto& scanout : activeScanouts) {
        if (not(scanout.uResourceId == uResourceId)) {
            continue;
        }

        results.emplace_back(std::ref(scanout));
    }

    return results;
}

std::optional<VirtioGpuCmdHandler::ScanoutCRef> VirtioGpuCmdHandler::getCScanout(uint32_t uScanout)
{
    auto maybeScanout {getScanout(uScanout)};

    if (not maybeScanout.has_value()) {
        return {};
    }

    return {std::cref(maybeScanout->get())};
}

void VirtioGpuCmdHandler::requestResize(uint32_t uScanout, bool enabled, uint32_t uWidth, uint32_t uHeight)
{
    auto maybeScanout {getScanout(uScanout)};
    if (not maybeScanout.has_value()) {
        LogRel(("virtio-gpu cmd handler: Scanout %d not available\n", uScanout));
        return;
    }

    auto& currentScanout {maybeScanout->get()};

    currentScanout.fActive = enabled;
    if (!enabled) {
        currentScanout.detachDisplay();
    }

    currentScanout.uResizedWidth = uWidth;
    currentScanout.uResizedHeight = uHeight;
    currentScanout.fResizeRequested = true;
}

inline void VirtioGpuCmdHandler::resizeScanout(uint32_t uScanout, uint32_t uWidth, uint32_t uHeight)
{
    auto maybeScanout {getScanout(uScanout)};
    if (not maybeScanout.has_value()) {
        return;
    }

    auto& currentScanout {maybeScanout->get()};
    if (uWidth != currentScanout.uCurrentWidth or uHeight != currentScanout.uCurrentHeight
        or currentScanout.fNeedsResize) {
        currentScanout.uCurrentWidth = uWidth;
        currentScanout.uCurrentHeight = uHeight;

        if (currentScanout.isAttachedToDisplay()) {
            currentScanout.fNeedsResize = false;
            currentScanout.resizeDisplay();
        }
    }
}

inline VirtioGpuResource* VirtioGpuCmdHandler::getResource(uint32_t uResourceId)
{
    auto result {std::find_if(std::begin(vResources_), std::end(vResources_),
                              [uResourceId](const auto& it) { return uResourceId == it->resourceId(); })};

    return result == std::end(vResources_) ? nullptr : result->get();
}

inline bool VirtioGpuCmdHandler::createResource(uint32_t uResourceId)
{
    if (getResource(uResourceId) != nullptr) {
        return false;
    }

    vResources_.emplace_back(new VirtioGpuResource(uResourceId));
    return true;
}

inline void VirtioGpuCmdHandler::removeResource(uint32_t uResourceId)
{
    auto result {std::find_if(std::begin(vResources_), std::end(vResources_),
                              [uResourceId](const auto& it) { return uResourceId == it->resourceId(); })};

    if (result != std::end(vResources_)) {
        vResources_.erase(result);
    }

    auto scanouts {getScanoutsByResource(uResourceId)};
    for (auto& scanout : scanouts) {
        scanout.get().uResourceId = 0;
    }
}

void VirtioGpuCmdHandler::handleBuffer(PVIRTQBUF pVirtqBuf)
{
    if (pVirtqBuf->cbPhysSend < sizeof(virtioGpu::CtrlHdr)) {
        LogRel(("virtio-gpu cmd handler: handleBuffer: request buffer of command in virtq %u too small\n",
                pVirtqBuf->uVirtq));
        returnResponseNoData(pVirtqBuf, nullptr, virtioGpu::CtrlType::Response::ERR_OUT_OF_MEMORY);
        return;
    }

    /*
     * This lock is a precaution to avoid race conditions. If done right, there are never more than two threads calling
     * this function, and those two threads shouldn't interfere even if they call this function at the same time.
     */
    RTCLock guard(mutex_);

    virtioGpu::CtrlHdr hdr;
    virtioAdapter_.virtqBufDrain(pVirtqBuf, &hdr, sizeof(hdr));

    switch (static_cast<virtioGpu::CtrlType::Cmd>(hdr.uType)) {
    case virtioGpu::CtrlType::Cmd::GET_DISPLAY_INFO: cmdGetDisplayInfo(pVirtqBuf, &hdr); break;
    case virtioGpu::CtrlType::Cmd::GET_EDID: cmdGetEdid(pVirtqBuf, &hdr); break;
    case virtioGpu::CtrlType::Cmd::RESOURCE_CREATE_2D: cmdResourceCreate2d(pVirtqBuf, &hdr); break;
    case virtioGpu::CtrlType::Cmd::RESOURCE_UNREF: cmdResourceUnref(pVirtqBuf, &hdr); break;
    case virtioGpu::CtrlType::Cmd::SET_SCANOUT: cmdSetScanout(pVirtqBuf, &hdr); break;
    case virtioGpu::CtrlType::Cmd::RESOURCE_FLUSH: cmdResourceFlush(pVirtqBuf, &hdr); break;
    case virtioGpu::CtrlType::Cmd::TRANSFER_TO_HOST_2D: cmdTransferToHost2d(pVirtqBuf, &hdr); break;
    case virtioGpu::CtrlType::Cmd::RESOURCE_ATTACH_BACKING: cmdResourceAttachBacking(pVirtqBuf, &hdr); break;
    case virtioGpu::CtrlType::Cmd::RESOURCE_DETACH_BACKING: cmdResourceDetachBacking(pVirtqBuf, &hdr); break;
    case virtioGpu::CtrlType::Cmd::UPDATE_CURSOR:
    case virtioGpu::CtrlType::Cmd::MOVE_CURSOR:
        if (pVirtqBuf->uVirtq != virtioGpu::VirtqIdx::CURSORQ) {
            /*
             * Not sure wether ERR_UNSPEC is the right thing here, but this is
             * also an odd error.
             */
            returnResponseNoData(pVirtqBuf, &hdr, virtioGpu::CtrlType::Response::ERR_UNSPEC);
        } else {
            returnResponseNoData(pVirtqBuf, &hdr, virtioGpu::CtrlType::Response::OK_NODATA);
        }
        break;
    default:
        returnResponseNoData(pVirtqBuf, &hdr, virtioGpu::CtrlType::Response::ERR_UNSPEC);
        LogRel(("virtio-gpu cmd handler: handleBuffer: got an unrecognized command in virtq %u: %#x\n",
                pVirtqBuf->uVirtq, hdr.uType));
    }
}

inline bool VirtioGpuCmdHandler::checkCtrlqCmd(const char* cmdName, PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr,
                                               size_t cbSend, size_t cbReturn)
{
    /*
     * we subtract sizeof(virtioGpu::CtrlHdr) from cbSend, because we want to know
     * wether we are able to drain to payload of a given command from pVirtqBuf.
     * That way we can write e.g. sizeof(virtioGpu::getEdid) as the fourth argument,
     * instead of writing sizeof(virtioGpu::getEdid) - sizeof(virtioGpu::CtrlHdr)
     * every time
     */
    cbSend = cbSend == 0 ? 0 : cbSend - sizeof(virtioGpu::CtrlHdr);

    if (pVirtqBuf->uVirtq != virtioGpu::VirtqIdx::CONTROLQ) {
        LogRel(("virtio-gpu cmd handler: %s: command was in the wrong virtq.\n", cmdName));
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::ERR_UNSPEC);
        return false;
    }

    if (cbSend > 0 and pVirtqBuf->cbPhysSend < cbSend) {
        LogRel(("virtio-gpu cmd handler: %s: request buffer was too small.\n", cmdName));
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::ERR_OUT_OF_MEMORY);
        return false;
    }

    if (cbReturn > 0 and pVirtqBuf->cbPhysReturn < cbReturn) {
        LogRel(("virtio-gpu cmd handler: %s: response buffer was too small.\n", cmdName));
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::ERR_OUT_OF_MEMORY);
        return false;
    }

    return true;
}

inline bool VirtioGpuCmdHandler::checkScanoutId(const char* cmdName, PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr,
                                                uint32_t uScanoutId)
{
    auto maybeScanout {getScanout(uScanoutId)};
    if (not maybeScanout.has_value()) {
        LogRel(("virtio-gpu cmd handler: %s: unknown scanout id %u\n", cmdName, uScanoutId));
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::ERR_INVALID_SCANOUT_ID);
        return false;
    }

    auto& currentScanout {maybeScanout->get()};
    if (not currentScanout.hasDisplay()) {
        LogRel(("virtio-gpu cmd handler: %s: scanout %u has no display.\n", cmdName, uScanoutId));
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::ERR_INVALID_SCANOUT_ID);
        return false;
    }

    return true;
}

inline bool VirtioGpuCmdHandler::checkResourceId(const char* cmdName, PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr,
                                                 uint32_t uResourceId)
{
    if (getResource(uResourceId) == nullptr) {
        LogRel(("virtio-gpu cmd handler: %s: resource id %u does not exist.\n", cmdName, uResourceId));
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::ERR_INVALID_RESOURCE_ID);
        return false;
    }
    return true;
}

inline void VirtioGpuCmdHandler::returnResponseOkEarly(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr)
{
    if (not pCtrlHdr->has_flag(virtioGpu::CtrlHdr::Flags::FENCE)) {
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::OK_NODATA);
    }
}

inline void VirtioGpuCmdHandler::returnResponseOkLate(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr)
{
    if (pCtrlHdr->has_flag(virtioGpu::CtrlHdr::Flags::FENCE)) {
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::OK_NODATA);
    }
}

inline void VirtioGpuCmdHandler::returnResponseNoData(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr,
                                                      virtioGpu::CtrlType::Response responseType)
{
    if (pVirtqBuf->cbPhysReturn < sizeof(virtioGpu::CtrlHdr)) {
        return;
    }

    virtioGpu::CtrlHdr response {responseType};

    if (pCtrlHdr != nullptr) {
        /*
         * It may happen that the caller of this functions passes a nullptr if
         * the request buffer of pVirtqBuf is too small for a header.
         */
        response.transfer_fence(pCtrlHdr);
    }

    returnResponseBuf(pVirtqBuf, &response, sizeof(virtioGpu::CtrlHdr));
}

inline void VirtioGpuCmdHandler::returnResponseBuf(PVIRTQBUF pVirtqBuf, void* pv, size_t cb)
{
    virtioAdapter_.virtqBufPut(pVirtqBuf, pv, cb);
    virtioAdapter_.virtqSyncRings(pVirtqBuf);
}

void VirtioGpuCmdHandler::cmdGetDisplayInfo(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr)
{
    if (not checkCtrlqCmd("GetDisplayInfo", pVirtqBuf, pCtrlHdr, 0,
                          virtioGpu::ResponseDisplayInfo::size(numScanouts_))) {
        return;
    }

    LogRel7(("virtio-gpu cmd handler: Got GET_DISPLAY_INFO command.\n"));
    virtioGpu::ResponseDisplayInfo response;

    for (unsigned i {0}; i < numScanouts_; i++) {
        auto& pmode {response.pmodes[i]};
        if (scanoutExists(i)) {
            auto& currentScanout {getScanout(i)->get()};

            /*
             * Here we should only report scanouts that are already attached to a
             * display. But this doesn't work if a driver is started later, because
             * then it wouldn't see any scanouts.
             */
            if (not currentScanout.hasDisplay()) {
                LogRel7(("virtio-gpu cmd handler: Scanout %u has no display.\n", i));
                continue;
            }

            if (currentScanout.fResizeRequested) {
                resizeScanout(i, currentScanout.uResizedWidth, currentScanout.uResizedHeight);
                currentScanout.fResizeRequested = false;
            }

            pmode.r.width = currentScanout.uCurrentWidth;
            pmode.r.height = currentScanout.uCurrentHeight;

            pmode.enabled = currentScanout.fActive;

            continue;
        }
    }

    returnResponseBuf(pVirtqBuf, &response, virtioGpu::ResponseDisplayInfo::size(numScanouts_));
}

template <typename VIRTIO_GPU_CMD>
static size_t sizeofPayload(VIRTIO_GPU_CMD cmd)
{
    return sizeof(cmd) - sizeof(virtioGpu::CtrlHdr);
}

void VirtioGpuCmdHandler::cmdGetEdid(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr)
{
    if (not checkCtrlqCmd("GetEdid", pVirtqBuf, pCtrlHdr, sizeof(virtioGpu::GetEdid),
                          sizeof(virtioGpu::ResponseEdid))) {
        return;
    }

    virtioGpu::GetEdid request;
    virtioAdapter_.virtqBufDrain(pVirtqBuf, request.payload(), sizeofPayload(request));
    LogRel7(("virtio-gpu cmd handler: Got GET_EDID command for scanout %u.\n", request.uScanout));

    if (not checkScanoutId("GetEdid", pVirtqBuf, pCtrlHdr, request.uScanout)) {
        return;
    }

    virtioGpu::ResponseEdid response;

    auto& currentScanout {getScanout(request.uScanout)->get()};
    if (currentScanout.fResizeRequested) {
        resizeScanout(0, currentScanout.uResizedWidth, currentScanout.uResizedHeight);
        currentScanout.fResizeRequested = false;
    }

    auto edid {generateExtendedEdid(currentScanout.uResizedWidth, currentScanout.uResizedHeight)};
    AssertReleaseMsg(sizeof(edid) <= sizeof(response.aEdid),
                     ("virtio-gpu cmd handler: GetEdid: Given EDID is too big to be returned to the driver!"));
    response.uSize = std::min(sizeof(edid), sizeof(response.aEdid));
    std::memcpy(&response.aEdid, &edid, response.uSize);

    returnResponseBuf(pVirtqBuf, &response, sizeof(virtioGpu::ResponseEdid));
}

void VirtioGpuCmdHandler::cmdResourceCreate2d(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr)
{
    if (not checkCtrlqCmd("ResourceCreate2D", pVirtqBuf, pCtrlHdr, sizeof(virtioGpu::ResourceCreate2d),
                          sizeof(virtioGpu::CtrlHdr))) {
        return;
    }

    virtioGpu::ResourceCreate2d request;
    virtioAdapter_.virtqBufDrain(pVirtqBuf, request.payload(), sizeofPayload(request));
    LogRel7(("virtio-gpu cmd handler: Got RESOURCE_CREATE_2D command. (resource=%u, format=%u, width=%u, height=%u)\n",
             request.uResourceId, request.uFormat, request.uWidth, request.uHeight));

    if (request.uResourceId == 0) {
        /*
         * The driver can disable a scanout in SET_SCANOUT by setting uResourceId to 0. Thus
         * (even though the specification doesn't say anything about this) we disallow creating
         * resources with an Id of 0 here.
         */
        LogRel(("virtio-gpu cmd handler: ResourceCreate2D: resource id %u can not be used.\n", request.uResourceId));
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::ERR_INVALID_RESOURCE_ID);
        return;
    }

    if (not createResource(request.uResourceId)) {
        LogRel(("virtio-gpu cmd handler: ResourceCreate2D: resource id %u already in use.\n", request.uResourceId));
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::ERR_INVALID_RESOURCE_ID);
        return;
    }

    /* We currently only support the B8G8R8X8_UNORM pixel format. Thus, in case
     * the driver uses another format, we print a message to the log.
     * For some reason, the customers driver uses the B8G8R8A8_UNORM in the first
     * resource it creates. Thus this format has to be enabled too.
     */
    if ((request.uFormat & (virtioGpu::Format::B8G8R8A8_UNORM | virtioGpu::Format::B8G8R8X8_UNORM)) == 0) {
        LogRel(("virtio-gpu cmd handler: ResourceCreate2D: An unsupported pixel-format has been set. This virtio-gpu "
                "currently only supports B8G8R8X8_UNORM.\n"));
    }

    returnResponseOkEarly(pVirtqBuf, pCtrlHdr);

    auto* resource {getResource(request.uResourceId)};
    resource->format(request.uFormat);
    resource->size(request.uWidth, request.uHeight);

    returnResponseOkLate(pVirtqBuf, pCtrlHdr);
}

void VirtioGpuCmdHandler::cmdResourceUnref(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr)
{
    if (not checkCtrlqCmd("ResourceUnref", pVirtqBuf, pCtrlHdr, sizeof(virtioGpu::ResourceUnref),
                          sizeof(virtioGpu::CtrlHdr))) {
        return;
    }

    virtioGpu::ResourceUnref request;
    virtioAdapter_.virtqBufDrain(pVirtqBuf, request.payload(), sizeofPayload(request));
    LogRel7(("virtio-gpu cmd handler: Got RESOURCE_UNREF command. (resource=%u)\n", request.uResourceId));

    if (not checkResourceId("ResourceUnref", pVirtqBuf, pCtrlHdr, request.uResourceId)) {
        return;
    }

    returnResponseOkEarly(pVirtqBuf, pCtrlHdr);

    removeResource(request.uResourceId);

    returnResponseOkLate(pVirtqBuf, pCtrlHdr);
}

void VirtioGpuCmdHandler::cmdSetScanout(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr)
{
    if (not checkCtrlqCmd("SetScanout", pVirtqBuf, pCtrlHdr, sizeof(virtioGpu::SetScanout),
                          sizeof(virtioGpu::CtrlHdr))) {
        return;
    }

    virtioGpu::SetScanout request;
    virtioAdapter_.virtqBufDrain(pVirtqBuf, request.payload(), sizeofPayload(request));
    LogRel7(("virtio-gpu cmd handler: Got SET_SCANOUT command. (scanout=%u, resource=%u, rect=w:%u,h:%u,x:%u,y:%u)\n",
             request.uScanoutId, request.uResourceId, request.r.width, request.r.height, request.r.x, request.r.y));

    if (not checkScanoutId("SetScanout", pVirtqBuf, pCtrlHdr, request.uScanoutId)) {
        return;
    }

    auto& currentScanout {getScanout(request.uScanoutId)->get()};
    if (request.uResourceId == 0) {
        LogRel2(("virtio-gpu cmd handler: SetScanout: Driver disabled scanout %u\n", request.uScanoutId));
        currentScanout.fActive = false;
        currentScanout.fNeedsResize = true;
        currentScanout.detachDisplay();
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::OK_NODATA);
        return;
    }

    if (not checkResourceId("SetScanout", pVirtqBuf, pCtrlHdr, request.uResourceId)) {
        return;
    }

    returnResponseOkEarly(pVirtqBuf, pCtrlHdr);

    currentScanout.fActive = true;
    currentScanout.uResourceId = request.uResourceId;
    if (not currentScanout.isAttachedToDisplay()) {
        currentScanout.attachDisplay();
        currentScanout.fNeedsResize = true;
    }

    resizeScanout(request.uScanoutId, request.r.width, request.r.height);

    returnResponseOkLate(pVirtqBuf, pCtrlHdr);
}

void VirtioGpuCmdHandler::cmdResourceFlush(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr)
{
    if (not checkCtrlqCmd("ResourceFlush", pVirtqBuf, pCtrlHdr, sizeof(virtioGpu::ResourceFlush),
                          sizeof(virtioGpu::CtrlHdr))) {
        return;
    }

    virtioGpu::ResourceFlush request;
    virtioAdapter_.virtqBufDrain(pVirtqBuf, request.payload(), sizeofPayload(request));
    LogRel7(("virtio-gpu cmd handler: Got RESOURCE_FLUSH command. (resource=%u, rect=w:%u,h:%u,x:%u,y:%u)\n",
             request.uResourceId, request.r.width, request.r.height, request.r.x, request.r.y));

    if (not checkResourceId("ResourceFlush", pVirtqBuf, pCtrlHdr, request.uResourceId)) {
        return;
    }

    auto scanouts {getScanoutsByResource(request.uResourceId)};
    if (scanouts.empty()) {
        LogRel(
            ("virtio-gpu cmd handler: ResourceFlush: No scanout is assigned to resource %u.\n", request.uResourceId));
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::ERR_INVALID_RESOURCE_ID);
        return;
    }

    returnResponseOkEarly(pVirtqBuf, pCtrlHdr);

    for (auto& scanout : scanouts) {
        if (scanout.get().hasDisplay()) {
            scanout.get().flush();
        }
    }

    returnResponseOkLate(pVirtqBuf, pCtrlHdr);
}

void VirtioGpuCmdHandler::cmdTransferToHost2d(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr)
{
    if (not checkCtrlqCmd("TransferToHost2D", pVirtqBuf, pCtrlHdr, sizeof(virtioGpu::TransferToHost2d),
                          sizeof(virtioGpu::CtrlHdr))) {
        return;
    }

    virtioGpu::TransferToHost2d request;
    virtioAdapter_.virtqBufDrain(pVirtqBuf, request.payload(), sizeofPayload(request));
    LogRel7(("virtio-gpu cmd handler: Got TRANSFER_TO_HOST_2D command. (resource=%u, offset=%lu, "
             "rect=w:%u,h:%u,x:%u,y:%u)\n",
             request.uResourceId, request.uOffset, request.r.width, request.r.height, request.r.x, request.r.y));

    if (not checkResourceId("TransferToHost2D", pVirtqBuf, pCtrlHdr, request.uResourceId)) {
        return;
    }

    auto scanouts {getScanoutsByResource(request.uResourceId)};
    if (scanouts.empty()) {
        LogRel(
            ("virtio-gpu cmd handler: ResourceFlush: No scanout is assigned to resource %u.\n", request.uResourceId));
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::ERR_INVALID_RESOURCE_ID);
        return;
    }

    returnResponseOkEarly(pVirtqBuf, pCtrlHdr);

    auto* resource {getResource(request.uResourceId)};
    auto vMapping {memoryAdapter_.mapGCPhys2HCVirt(resource->getBacking())};

    PRTSGSEG paSegments {static_cast<PRTSGSEG>(RTMemAllocZ(sizeof(RTSGSEG) * vMapping.size()))};
    for (auto idx {0u}; idx < vMapping.size(); idx++) {
        const auto& mapping {vMapping.at(idx)};
        paSegments[idx].pvSeg = mapping.uAddr_;
        paSegments[idx].cbSeg = mapping.uLength_;
    }

    PRTSGBUF pSegBuf {static_cast<PRTSGBUF>(RTMemAllocZ(sizeof(RTSGBUF)))};

    for (auto& wrappedScanout : scanouts) {
        auto& scanout {wrappedScanout.get()};
        if (not scanout.fActive) {
            LogRel(("virtio-gpu cmd handler: TransferToHost2D: Prevented copying into disabled scanout %u.\n",
                    scanout.uScanoutId));
            continue;
        }

        /*
         * If the size is 64x64, then this is the resource of the mouse cursor.
         * As we currently ignore the cursorq, we just do nothing in this case.
         */
        if ((resource->width() > 64u and resource->height() > 64u) and scanout.hasDisplay()
            and resource->getBacking().size() > 0
            /*
             * TODO: at the moment we always assume that offset=0 and r.x=0 and r.y=0,
             * i.e. the driver always sends a full frame, not just parts of a frame.
             * This is currently only used by Linux and not by the customers driver,
             * thus we ignore cases where this assumption isn't true.
             */
            and request.r == virtioGpu::Rect {resource->width(), resource->height()}) {
            RTSgBufInit(pSegBuf, paSegments, vMapping.size());
            auto [pFrameBuffer, cbFrameBuffer] {scanout.rDisplayManager.acquireBackingStore(scanout.uScanoutId)};
            if (pFrameBuffer != nullptr) {
                RTSgBufCopyToBuf(pSegBuf, pFrameBuffer, cbFrameBuffer);
            }
            scanout.rDisplayManager.releaseBackingStore();
        }
    }

    RTMemFree(pSegBuf);
    RTMemFree(paSegments);

    memoryAdapter_.releaseMappings(vMapping);

    returnResponseOkLate(pVirtqBuf, pCtrlHdr);
}

void VirtioGpuCmdHandler::cmdResourceAttachBacking(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr)
{
    if (not checkCtrlqCmd("ResourceAttachBacking", pVirtqBuf, pCtrlHdr, sizeof(virtioGpu::ResourceAttachBacking),
                          sizeof(virtioGpu::CtrlHdr))) {
        return;
    }

    virtioGpu::ResourceAttachBacking request;
    virtioAdapter_.virtqBufDrain(pVirtqBuf, request.payload(), sizeofPayload(request));
    LogRel7(("virtio-gpu cmd handler: Got RESOURCE_ATTACH_BACKING command. (resource=%u)\n", request.uResourceId));

    if (pVirtqBuf->cbPhysSend < (request.uNrEntries * sizeof(virtioGpu::ResourceMemEntry))) {
        LogRel(("virtio-gpu cmd handler: ResourceAttachBacking: request buffer too small for all memory entries.\n"));
        returnResponseNoData(pVirtqBuf, pCtrlHdr, virtioGpu::CtrlType::Response::ERR_OUT_OF_MEMORY);
        return;
    }

    if (not checkResourceId("ResourceAttachBacking", pVirtqBuf, pCtrlHdr, request.uResourceId)) {
        return;
    }

    returnResponseOkEarly(pVirtqBuf, pCtrlHdr);

    auto* resource {getResource(request.uResourceId)};
    resource->reserveBacking(request.uNrEntries);

    const size_t cbEntries {sizeof(virtioGpu::ResourceMemEntry) * request.uNrEntries};
    virtioGpu::ResourceMemEntry* pEntries {static_cast<virtioGpu::ResourceMemEntry*>(RTMemAlloc(cbEntries))};
    virtioAdapter_.virtqBufDrain(pVirtqBuf, pEntries, cbEntries);

    for (auto idx {0u}; idx < request.uNrEntries; idx++) {
        resource->addBacking(pEntries[idx].uAddr, pEntries[idx].uLength);
    }

    RTMemFree(pEntries);

    returnResponseOkLate(pVirtqBuf, pCtrlHdr);
}

void VirtioGpuCmdHandler::cmdResourceDetachBacking(PVIRTQBUF pVirtqBuf, virtioGpu::CtrlHdr* pCtrlHdr)
{
    if (not checkCtrlqCmd("ResourceDetachBacking", pVirtqBuf, pCtrlHdr, sizeof(virtioGpu::ResourceDetachBacking),
                          sizeof(virtioGpu::CtrlHdr))) {
        return;
    }

    virtioGpu::ResourceDetachBacking request;
    virtioAdapter_.virtqBufDrain(pVirtqBuf, request.payload(), sizeofPayload(request));
    LogRel7(("virtio-gpu cmd handler: Got RESOURCE_DETACH_BACKING command. (resource=%u)\n", request.uResourceId));

    if (not checkResourceId("ResourceDetachBacking", pVirtqBuf, pCtrlHdr, request.uResourceId)) {
        return;
    }

    returnResponseOkEarly(pVirtqBuf, pCtrlHdr);

    auto* resource {getResource(request.uResourceId)};
    resource->clearBacking();

    returnResponseOkLate(pVirtqBuf, pCtrlHdr);
}
