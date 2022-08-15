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
#include <vector>

class VirtioGpuResource
{
    const uint32_t uResourceId_;
    uint32_t uFormat_ {0};
    uint32_t uWidth_ {0};
    uint32_t uHeight_ {0};

    uint32_t uScanoutId_ {0};

public:
    struct MemEntry
    {
        const uint64_t uAddr_; // guest physical address
        const uint32_t uLength_;

        MemEntry() = delete;
        MemEntry(uint64_t uAddr, uint32_t uLength) : uAddr_(uAddr), uLength_(uLength) {}
    };

    static constexpr unsigned BYTES_PER_PIXEL {4u};

private:
    std::vector<MemEntry> vBacking_ {};

public:
    VirtioGpuResource() = delete;
    VirtioGpuResource(uint32_t uResourceId) : uResourceId_(uResourceId) {}

    ~VirtioGpuResource() = default;

    uint32_t resourceId() const { return uResourceId_; }

    void format(uint32_t uFormat) { uFormat_ = uFormat; }
    uint32_t format() const { return uFormat_; }

    void size(uint32_t uWidth, uint32_t uHeight)
    {
        uWidth_ = uWidth;
        uHeight_ = uHeight;
    }

    uint32_t width() const { return uWidth_; }
    uint32_t height() const { return uHeight_; }

    void scanoutId(uint32_t uScanoutId) { uScanoutId_ = uScanoutId; }
    uint32_t scanoutId() { return uScanoutId_; }

    size_t memNeeded() const { return uWidth_ * uHeight_ * BYTES_PER_PIXEL; }

    void reserveBacking(size_t sz) { vBacking_.reserve(sz); }
    void clearBacking() { vBacking_.clear(); }
    void addBacking(uint64_t uAddr, uint32_t uLenght) { vBacking_.emplace_back(uAddr, uLenght); }

    MemEntry* getBacking(uint32_t idx) { return &vBacking_.at(idx); }
    const std::vector<MemEntry>& getBacking() { return vBacking_; }
};
