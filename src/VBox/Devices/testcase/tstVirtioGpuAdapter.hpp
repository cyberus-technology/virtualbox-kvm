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

#include "../Graphics/DevVirtioGpuCmdHandler.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <cstdlib>
#include <numeric>
#include <tuple>
#include <vector>

static constexpr uint32_t TST_VIOGPU_MAX_SCANOUTS {2u};

class tstVirtioAdapter final : public VirtioGpuCmdHandler::VirtioAdapter
{
public:
    uint8_t* sendBuf_ {nullptr}; // During each virtqBufDrain, we will read from this pointer into pv
    uint8_t* recvBuf_ {nullptr}; // During each virtqBufPut, we will write to this pointer

    template <typename SendType, typename ReceiveType>
    void prepareCommand(SendType* send, ReceiveType* recv, uint16_t uVirtq, PVIRTQBUF pVirtqBuf)
    {
        prepareCommand(send, sizeof(SendType), recv, sizeof(ReceiveType), uVirtq, pVirtqBuf);
    }

    void prepareCommand(void* sendBuf, size_t sendSz, void* recvBuf, size_t recvSz, uint16_t uVirtq, PVIRTQBUF pVirtqBuf)
    {
        sendBuf_ = reinterpret_cast<uint8_t*>(sendBuf);
        pVirtqBuf->cbPhysSend = sendSz;
        recvBuf_ = reinterpret_cast<uint8_t*>(recvBuf);
        pVirtqBuf->cbPhysReturn = recvSz;
        pVirtqBuf->uVirtq = uVirtq;
    }

    void virtqBufDrain(PVIRTQBUF pVirtqBuf, void *pv, size_t cb) final
    {
        REQUIRE(pVirtqBuf != nullptr);
        REQUIRE(pv != nullptr);
        REQUIRE(cb != 0);

        // The cmdHandler has to check wether the size of the src buffer is sufficient for the drain-request
        REQUIRE(pVirtqBuf->cbPhysSend >= cb);
        std::memcpy(pv, sendBuf_, cb);
        pVirtqBuf->cbPhysSend -= cb;
        sendBuf_ += cb;
    }

    void virtqBufPut(PVIRTQBUF pVirtqBuf, void *pv, size_t cb) final
    {
        REQUIRE(pVirtqBuf != nullptr);
        REQUIRE(pv != nullptr);
        REQUIRE(cb != 0);

        // The cmdHandler has to check wether the size of the dst buffer is sufficient for the put-request
        REQUIRE(pVirtqBuf->cbPhysReturn >= cb);
        std::memcpy(recvBuf_, pv, cb);
        pVirtqBuf->cbPhysReturn -= cb;
        recvBuf_ += cb;
    }

    void virtqSyncRings(PVIRTQBUF pVirtqBuf) final
    {
        REQUIRE(pVirtqBuf != 0);
    }
};

class tstDisplayAdapter final : public VirtioGpuCmdHandler::displayAdapter
{
public:
    std::vector<uint8_t> framebuf;
    uint32_t displayIdx {0};
    bool fAttached {false};
    bool fFlushed  {false};
    uint32_t uCurrentWidth {virtioGpu::INITIAL_WIDTH};
    uint32_t uCurrentHeight {virtioGpu::INITIAL_HEIGHT};

    tstDisplayAdapter() = default;

    void reset()
    {
        framebuf.clear();
        fAttached = false;
        fFlushed = false;
        uCurrentWidth = virtioGpu::INITIAL_WIDTH;
        uCurrentHeight = virtioGpu::INITIAL_HEIGHT;
    }

    void resize(uint32_t uWidth, uint32_t uHeight) final
    {
        uCurrentWidth = uWidth;
        uCurrentHeight = uHeight;

        framebuf.resize(cbFrameBuffer());
    }

    std::tuple<uint32_t, uint32_t> size() final
    {
        return std::make_tuple(uCurrentWidth, uCurrentHeight);
    }

    void attachDisplay(unsigned iLUN) final
    {
        REQUIRE(iLUN == displayIdx);
        fAttached = true;
    }

    void detachDisplay(unsigned iLUN) final
    {
        REQUIRE(iLUN == displayIdx);
        fAttached = false;
    }

    bool isAttachedToDisplay() final
    {
        return fAttached;
    }

    void flush(uint32_t /*uWidth*/, uint32_t /*uHeight*/) final
    {
        fFlushed = true;
    }

    void* pFrameBuffer() final { return framebuf.data(); }
    size_t cbFrameBuffer() final { return uCurrentWidth * uCurrentHeight * virtioGpuResource::BYTES_PER_PIXEL; }
};

class tstDisplayManager final : public VirtioGpuCmdHandler::DisplayManager
{
    std::array<tstDisplayAdapter, TST_VIOGPU_MAX_SCANOUTS> displayAdapters;
public:
    tstDisplayManager()
    {
        uint32_t displayIdx {0u};
        for (auto& displayAdapterOne : displayAdapters) {
            displayAdapterOne.displayIdx = displayIdx;
            displayIdx++;
        }
    }

    tstDisplayAdapter* display(uint32_t idx) final
    {
        if (idx > TST_VIOGPU_MAX_SCANOUTS-1) {
            return nullptr;
        }
        return &displayAdapters.at(idx);
    }
};

class tstMemoryAdapter final : public VirtioGpuCmdHandler::MemoryAdapter
{
public:
    VecMappings mapGCPhys2HCVirt(const vecMemEntries& vBacking) final
    {
        vecMappings vMapping;
        for (const auto& backing : vBacking) {
            void* uAddr {reinterpret_cast<void*>(backing.uAddr_)};
            vMapping.emplace_back(uAddr, backing.uLength_, nullptr);
        }
        return vMapping;
    }

    void releaseMappings(const vecMappings& /*vMapping*/) final { return ; }
};
