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

#include <VBox/types.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/stam.h>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#undef LOG_GROUP
#include "../Graphics/DevVirtioGpuDefinitions.hpp"
#include "../Graphics/DevVirtioGpuCmdHandler.hpp"
#include "tstVirtioGpuAdapter.hpp"

#include <algorithm>

// virtioAdapter and memoryAdapter both have no state, thus we can reuse them
static tstVirtioAdapter virtioAdapter;
static tstMemoryAdapter memoryAdapter;

static VIRTQBUF virtqBuf;

static constexpr uint32_t RESOURCE_ID_ONE {1u};
static constexpr uint32_t RESOURCE_ID_TWO {2u};

static constexpr uint32_t SCANOUT_ID_ONE {0u};
static constexpr uint32_t SCANOUT_ID_TWO {1u};

static constexpr uint32_t RESOURCE_WIDTH {1920u};
static constexpr uint32_t RESOURCE_HEIGHT {1080u};
static constexpr uint32_t RESIZED_WIDTH {800u};
static constexpr uint32_t RESIZED_HEIGHT {600u};

static constexpr uint32_t NUM_BACKINGS {4u};
static constexpr size_t BACKING_SIZE {X86_PAGE_SIZE};
static constexpr size_t SIZE_FRAMEBUFFER {NUM_BACKINGS * BACKING_SIZE};

static constexpr size_t ATTACH_BACKING_STRUCT_SIZE {sizeof(virtioGpu::resourceAttachBacking) + NUM_BACKINGS * sizeof(virtioGpu::resourceMemEntry)};

TEST_CASE("handler returns out-of-memory error if request-buffer is too small")
{
    tstDisplayManager displayManager;
    VirtioGpuCmdHandler handler(virtioAdapter, displayManager, memoryAdapter, TST_VIOGPU_MAX_SCANOUTS, false);
    virtioGpu::ctrlHdr recvHdr;

    virtqBuf.cbPhysReturn = sizeof(recvHdr);
    virtqBuf.uVirtq = virtioGpu::virtqIdx::CONTROLQ;
    virtioAdapter.recvBuf_ = reinterpret_cast<uint8_t *>(&recvHdr);

    handler.handleBuffer(&virtqBuf);
    REQUIRE(recvHdr.uType == virtioGpu::ctrlType::resp::ERR_OUT_OF_MEMORY);
}

TEST_CASE("handler returns unspec error if the ctrl-type is unknown")
{
    tstDisplayManager displayManager;
    VirtioGpuCmdHandler handler(virtioAdapter, displayManager, memoryAdapter, TST_VIOGPU_MAX_SCANOUTS, false);
    virtioGpu::ctrlHdr recvHdr;

    // GET_DISPLAY_INFO is the command with the lowest value, thus GET_DISPLAY_INFO is an invalid command.
    virtioGpu::ctrlHdr sendHdr {virtioGpu::ctrlType::cmd::GET_DISPLAY_INFO - 1};

    virtioAdapter.prepareCommand(&sendHdr, &recvHdr, virtioGpu::virtqIdx::CONTROLQ, &virtqBuf);
    handler.handleBuffer(&virtqBuf);
    REQUIRE(recvHdr.uType == virtioGpu::ctrlType::resp::ERR_UNSPEC);
}

TEST_CASE("handler returns unspec error if a command is in the wrong queue")
{
    tstDisplayManager displayManager;
    VirtioGpuCmdHandler handler(virtioAdapter, displayManager, memoryAdapter, TST_VIOGPU_MAX_SCANOUTS, false);

    for (uint32_t cmd {virtioGpu::ctrlType::cmd::GET_DISPLAY_INFO}; cmd < virtioGpu::ctrlType::cmd::RESOURCE_DETACH_BACKING; cmd++) {
        virtioGpu::ctrlHdr sendHdr {cmd};
        virtioGpu::ctrlHdr recvHdr;
        virtioAdapter.prepareCommand(&sendHdr, &recvHdr, virtioGpu::virtqIdx::CURSORQ, &virtqBuf);
        handler.handleBuffer(&virtqBuf);
        REQUIRE(recvHdr.uType == virtioGpu::ctrlType::resp::ERR_UNSPEC);
    }

    {
        virtioGpu::ctrlHdr sendHdr {virtioGpu::ctrlType::cmd::GET_EDID};
        virtioGpu::ctrlHdr recvHdr;
        virtioAdapter.prepareCommand(&sendHdr, &recvHdr, virtioGpu::virtqIdx::CURSORQ, &virtqBuf);
        handler.handleBuffer(&virtqBuf);
        REQUIRE(recvHdr.uType == virtioGpu::ctrlType::resp::ERR_UNSPEC);
    }

    for (uint32_t cmd {virtioGpu::ctrlType::cmd::UPDATE_CURSOR}; cmd < virtioGpu::ctrlType::cmd::MOVE_CURSOR; cmd++) {
        virtioGpu::ctrlHdr sendHdr {cmd};
        virtioGpu::ctrlHdr recvHdr;
        virtioAdapter.prepareCommand(&sendHdr, &recvHdr, virtioGpu::virtqIdx::CONTROLQ, &virtqBuf);
        handler.handleBuffer(&virtqBuf);
        REQUIRE(recvHdr.uType == virtioGpu::ctrlType::resp::ERR_UNSPEC);
    }
}

SCENARIO("The command handler respects the attachDisplayLater flag")
{
    GIVEN("A display adapter and a GET_DISPLAY_INFO command") {
        tstDisplayManager displayManager;
        virtioGpu::ctrlHdr sendHdr {virtioGpu::ctrlType::cmd::GET_DISPLAY_INFO};
        virtioGpu::respDisplayInfo displayInfo;
        virtioAdapter.prepareCommand(&sendHdr, sizeof(sendHdr), &displayInfo, virtioGpu::respDisplayInfo::size(TST_VIOGPU_MAX_SCANOUTS), virtioGpu::virtqIdx::CONTROLQ, &virtqBuf);

        WHEN("The command handler is created with attachDisplayLater set to false and GET_DISPLAY_INFO is called") {
            VirtioGpuCmdHandler handler(virtioAdapter, displayManager, memoryAdapter, TST_VIOGPU_MAX_SCANOUTS, false);
            handler.handleBuffer(&virtqBuf);

            THEN("Display 0 is enabled, has the initial resolution and is attached") {
                REQUIRE(displayInfo.hdr.uType == virtioGpu::ctrlType::resp::OK_DISPLAY_INFO);
                REQUIRE(displayInfo.pmodes[0].enabled != 0);
                REQUIRE(displayInfo.pmodes[0].r.width == virtioGpu::INITIAL_WIDTH);
                REQUIRE(displayInfo.pmodes[0].r.height == virtioGpu::INITIAL_HEIGHT);
                REQUIRE(displayManager.display(0)->fAttached == true);
            }
        }

        WHEN("The command handler is created with attachDisplayLater set to true and GET_DISPLAY_INFO is called") {
            VirtioGpuCmdHandler handler(virtioAdapter, displayManager, memoryAdapter, TST_VIOGPU_MAX_SCANOUTS, true);
            handler.handleBuffer(&virtqBuf);

            THEN("Display 0 is enabled and has the initial resolution, but the display is not attached") {
                // We do this because if a driver attaches later to the virtio-gpu but immerdiately asks for the display info,
                // the driver should still see all available scanouts.
                REQUIRE(displayInfo.hdr.uType == virtioGpu::ctrlType::resp::OK_DISPLAY_INFO);
                REQUIRE(displayInfo.pmodes[0].enabled != 0);
                REQUIRE(displayInfo.pmodes[0].r.width == virtioGpu::INITIAL_WIDTH);
                REQUIRE(displayInfo.pmodes[0].r.height == virtioGpu::INITIAL_HEIGHT);
                REQUIRE(displayManager.display(0)->fAttached == false);
            }
        }
    }
}

SCENARIO("Creation and deletion of resources is handled correctly") {
    GIVEN("A fresh command handler") {
        tstDisplayManager displayManager;
        VirtioGpuCmdHandler handler(virtioAdapter, displayManager, memoryAdapter, TST_VIOGPU_MAX_SCANOUTS, false);

        WHEN("A resource with ID 0 should be allocated") {
            virtioGpu::ctrlHdr recvHdr;
            virtioGpu::resourceCreate2d createResource {0u};

            virtioAdapter.prepareCommand(&createResource, &recvHdr, virtioGpu::virtqIdx::CONTROLQ, &virtqBuf);
            handler.handleBuffer(&virtqBuf);

            THEN("ERR_INVALID_RESOURCE_ID is returned.") {
                // The driver disables a scanout be using the resource ID 0 in SET_SCANOUT.
                // Thus the cmd handler should not allocate resources with ID 0.
                REQUIRE(recvHdr.uType == virtioGpu::ctrlType::resp::ERR_INVALID_RESOURCE_ID);
            }
        }

        WHEN("A resource with a valid ID is allocated") {
            virtioGpu::ctrlHdr recvHdr;
            virtioGpu::resourceCreate2d createResource {RESOURCE_ID_ONE, RESOURCE_WIDTH, RESOURCE_HEIGHT};

            virtioAdapter.prepareCommand(&createResource, &recvHdr, virtioGpu::virtqIdx::CONTROLQ, &virtqBuf);
            handler.handleBuffer(&virtqBuf);

            THEN("OK_NODATA is returned.") {
                REQUIRE(recvHdr.uType == virtioGpu::ctrlType::resp::OK_NODATA);
            }

            AND_WHEN("A resource with the same ID is allocated") {
                recvHdr.uType = 0;

                virtioAdapter.prepareCommand(&createResource, &recvHdr, virtioGpu::virtqIdx::CONTROLQ, &virtqBuf);
                handler.handleBuffer(&virtqBuf);

                THEN("ERR_INVALID_RESOURCE_ID is returned.") {
                    REQUIRE(recvHdr.uType == virtioGpu::ctrlType::resp::ERR_INVALID_RESOURCE_ID);
                }
            }

            AND_WHEN("The resource is deleted") {
                recvHdr.uType = 0;
                virtioGpu::resourceUnref unrefResource {RESOURCE_ID_ONE};

                virtioAdapter.prepareCommand(&unrefResource, &recvHdr, virtioGpu::virtqIdx::CONTROLQ, &virtqBuf);
                handler.handleBuffer(&virtqBuf);

                THEN("OK_NODATA is returned.") {
                    REQUIRE(recvHdr.uType == virtioGpu::ctrlType::resp::OK_NODATA);
                }

                AND_WHEN("The resource is deleted again") {
                    recvHdr.uType = 0;

                    virtioAdapter.prepareCommand(&unrefResource, &recvHdr, virtioGpu::virtqIdx::CONTROLQ, &virtqBuf);
                    handler.handleBuffer(&virtqBuf);

                    THEN("ERR_INVALID_RESOURCE_ID is returned") {
                        REQUIRE(recvHdr.uType == virtioGpu::ctrlType::resp::ERR_INVALID_RESOURCE_ID);
                    }
                }
            }
        }
    }
}

SCENARIO("Complex tests") {
    GIVEN("A command handler with two resources with IDs 1 and 2 and attached backings to both resources") {
        tstDisplayManager displayManager;
        VirtioGpuCmdHandler handler(virtioAdapter, displayManager, memoryAdapter, TST_VIOGPU_MAX_SCANOUTS, false);

        // 'Simple', because I just put in the two pointers
        auto runSimpleCommand = [&handler](auto* pSend, auto* pRecv) -> void {
            pRecv->hdr.uType = 0;
            virtioAdapter.prepareCommand(pSend, pRecv, virtioGpu::virtqIdx::CONTROLQ, &virtqBuf);
            handler.handleBuffer(&virtqBuf);
        };

        auto runSimpleCommandAndCheck = [&handler, &runSimpleCommand](auto* pSend, auto* pRecv,
                    virtioGpu::ctrlType::resp response = virtioGpu::ctrlType::resp::OK_NODATA) -> void {
            runSimpleCommand(pSend, pRecv);
            REQUIRE(pRecv->hdr.uType == response);
        };

        // 'Complex', because I have to provide the size
        auto runComplexCommand = [&handler](void* pSend, size_t cbSend, auto* pRecv) -> void {
            pRecv->hdr.uType = 0;
            virtioAdapter.prepareCommand(pSend, cbSend, pRecv, sizeof(*pRecv), virtioGpu::virtqIdx::CONTROLQ, &virtqBuf);
            handler.handleBuffer(&virtqBuf);
        };

        auto runComplexCommandAndCheck = [&handler, &runComplexCommand](void* pSend, size_t cbSend, auto* pRecv,
                    virtioGpu::ctrlType::resp response = virtioGpu::ctrlType::resp::OK_NODATA) -> void {
            runComplexCommand(pSend, cbSend, pRecv);
            REQUIRE(pRecv->hdr.uType == response);
        };

        struct { virtioGpu::ctrlHdr hdr; } recvHdr;
        virtioGpu::respDisplayInfo recvDisplayInfo;

        virtioGpu::ctrlHdr getDisplayInfo {virtioGpu::ctrlType::cmd::GET_DISPLAY_INFO};

        virtioGpu::resourceCreate2d createResourceOne {RESOURCE_ID_ONE, RESOURCE_WIDTH, RESOURCE_HEIGHT};
        virtioGpu::setScanout setScanoutOne {SCANOUT_ID_ONE, RESOURCE_ID_ONE, RESOURCE_WIDTH, RESOURCE_HEIGHT};
        virtioGpu::setScanout disableScanoutOne {SCANOUT_ID_ONE, 0u};
        virtioGpu::transferToHost2d transfer2HostOne {RESOURCE_ID_ONE, RESOURCE_WIDTH, RESOURCE_HEIGHT};
        virtioGpu::resourceDetachBacking detachBackingOne {RESOURCE_ID_ONE};

        virtioGpu::resourceCreate2d createResourceTwo {RESOURCE_ID_TWO, RESOURCE_WIDTH, RESOURCE_HEIGHT};
        virtioGpu::setScanout setScanoutTwo {SCANOUT_ID_TWO, RESOURCE_ID_TWO, RESOURCE_WIDTH, RESOURCE_HEIGHT};
        virtioGpu::transferToHost2d transfer2HostTwo {RESOURCE_ID_TWO, RESOURCE_WIDTH, RESOURCE_HEIGHT};

        std::vector<void*> backingPagesOne;
        std::vector<uint8_t> attachBackingMemOne;
        virtioGpu::resourceAttachBacking* pAttachBackingOne;
        virtioGpu::resourceMemEntry* pMemEntriesOne;

        std::vector<void*> backingPagesTwo;
        std::vector<uint8_t> attachBackingMemTwo;
        virtioGpu::resourceAttachBacking* pAttachBackingTwo;
        virtioGpu::resourceMemEntry* pMemEntriesTwo;

        const uint8_t FRAME_BYTE_ONE {0x55};
        const uint8_t FRAME_BYTE_TWO {0xaa};
        auto initializeBacking = [](std::vector<uint8_t>& attachBackingMem,
                                    std::vector<void*>& backingPages,
                                    virtioGpu::resourceAttachBacking*& pAttachBacking,
                                    virtioGpu::resourceMemEntry*& pMemEntries,
                                    uint32_t uResourceId, uint8_t frame_byte) {
            attachBackingMem.resize(ATTACH_BACKING_STRUCT_SIZE);
            pAttachBacking = reinterpret_cast<virtioGpu::resourceAttachBacking*>(attachBackingMem.data());
            pMemEntries = reinterpret_cast<virtioGpu::resourceMemEntry*>(attachBackingMem.data()+sizeof(virtioGpu::resourceAttachBacking));
            pAttachBacking->hdr.uType = virtioGpu::ctrlType::cmd::RESOURCE_ATTACH_BACKING;
            pAttachBacking->uResourceId = uResourceId;
            pAttachBacking->uNrEntries = NUM_BACKINGS;

            for (auto idx {0u}; idx < NUM_BACKINGS; idx++) {
                void* backingPtr {RTMemAlloc(BACKING_SIZE)};
                backingPages.emplace_back(backingPtr);

                pMemEntries[idx].uAddr = reinterpret_cast<uint64_t>(backingPtr);
                pMemEntries[idx].uLength = BACKING_SIZE;

                std::memset(backingPages.at(idx), frame_byte, BACKING_SIZE);
            }
        };

        initializeBacking(attachBackingMemOne, backingPagesOne, pAttachBackingOne, pMemEntriesOne, RESOURCE_ID_ONE, FRAME_BYTE_ONE);
        initializeBacking(attachBackingMemTwo, backingPagesTwo, pAttachBackingTwo, pMemEntriesTwo, RESOURCE_ID_TWO, FRAME_BYTE_TWO);

        auto frameBufPage = [&displayManager](int32_t displayIdx, size_t idx) -> void* {
            if (displayManager.display(displayIdx)->pFrameBuffer() == nullptr) {
                return nullptr;
            }
            return reinterpret_cast<uint8_t*>(displayManager.display(displayIdx)->pFrameBuffer())+idx*BACKING_SIZE;
        };

        // Returns true if the framebuffer and the backing have the same values in them
        auto compareFramebufBacking = [&frameBufPage](std::vector<void*> backingPages, uint32_t displayIdx) -> bool {
            if (frameBufPage(displayIdx, 0) == nullptr) {
                return false;
            }

            std::vector<int> results;
            for (auto idx {0u}; idx < NUM_BACKINGS; idx++) {
                results.emplace_back(std::memcmp(backingPages.at(idx), frameBufPage(displayIdx, idx), BACKING_SIZE));
            }
            return std::all_of(std::begin(results), std::end(results), [](int res) { return res == 0; });
        };

        runSimpleCommandAndCheck(&createResourceOne, &recvHdr);
        runComplexCommandAndCheck(pAttachBackingOne, ATTACH_BACKING_STRUCT_SIZE, &recvHdr);

        runSimpleCommandAndCheck(&createResourceTwo, &recvHdr);
        runComplexCommandAndCheck(pAttachBackingTwo, ATTACH_BACKING_STRUCT_SIZE, &recvHdr);

        /*
         * TESTING - SINGLE MONITOR
         */

        WHEN("TRANSFER_2_HOST is called") {
            runSimpleCommand(&transfer2HostOne, &recvHdr);

            THEN("The scanout is enabled and has its initial width and height") {
                REQUIRE(displayManager.display(0)->fAttached == true);
                REQUIRE(displayManager.display(0)->uCurrentWidth == virtioGpu::INITIAL_WIDTH);
                REQUIRE(displayManager.display(0)->uCurrentHeight == virtioGpu::INITIAL_HEIGHT);
            }

            AND_THEN("The transferring fails because no scanout is assigned to the resource") {
                REQUIRE(recvHdr.hdr.uType == virtioGpu::ctrlType::resp::ERR_INVALID_RESOURCE_ID);
                REQUIRE(not compareFramebufBacking(backingPagesOne, SCANOUT_ID_ONE));
            }
        }

        WHEN("SET_SCANOUT is called with the resource Id of an existing resource") {
            runSimpleCommandAndCheck(&setScanoutOne, &recvHdr);

            THEN("The scanout is enabled and has the given dimension") {
                REQUIRE(displayManager.display(0)->fAttached == true);
                REQUIRE(displayManager.display(0)->uCurrentWidth == RESOURCE_WIDTH);
                REQUIRE(displayManager.display(0)->uCurrentHeight == RESOURCE_HEIGHT);
            }

            AND_WHEN("SET_SCANOUT is called with a resource Id of 0") {
                runSimpleCommand(&disableScanoutOne, &recvHdr);

                THEN("The scanout is disabled") {
                    REQUIRE(displayManager.display(0)->fAttached == false);
                }
            }

            AND_WHEN("TRANSFER_2_HOST ist called") {
                runSimpleCommandAndCheck(&transfer2HostOne, &recvHdr);

                THEN("The transferring is successful") {
                    REQUIRE(compareFramebufBacking(backingPagesOne, SCANOUT_ID_ONE));
                }
            }

            AND_WHEN("DETACH_BACKING and TRANSFER_2_HOST are called") {
                // we again want to compare the framebuffer and the backing, thus we have to clear the framebuffer
                std::memset(displayManager.display(0)->pFrameBuffer(), 0, displayManager.display(0)->cbFrameBuffer());

                runSimpleCommandAndCheck(&detachBackingOne, &recvHdr);
                runSimpleCommandAndCheck(&transfer2HostOne, &recvHdr);

                THEN("No data is transferred") {
                    REQUIRE(not compareFramebufBacking(backingPagesOne, SCANOUT_ID_ONE));
                }
            }

            AND_THEN("GET_DISPLAY_INFO also reports the given resolution") {
                virtioAdapter.prepareCommand(&getDisplayInfo, sizeof(getDisplayInfo), &recvDisplayInfo, virtioGpu::respDisplayInfo::size(TST_VIOGPU_MAX_SCANOUTS), virtioGpu::virtqIdx::CONTROLQ, &virtqBuf);
                handler.handleBuffer(&virtqBuf);
                REQUIRE(recvDisplayInfo.hdr.uType == virtioGpu::ctrlType::resp::OK_DISPLAY_INFO);

                REQUIRE(recvDisplayInfo.pmodes[0].enabled != 0);
                REQUIRE(recvDisplayInfo.pmodes[0].r.width == RESOURCE_WIDTH);
                REQUIRE(recvDisplayInfo.pmodes[0].r.height == RESOURCE_HEIGHT);
            }

            AND_WHEN("requestResize is called because the screen has changed") {
                handler.requestResize(0, RESIZED_WIDTH, RESIZED_HEIGHT);

                THEN("the display reports still the same size") {
                    REQUIRE(displayManager.display(0)->uCurrentWidth == RESOURCE_WIDTH);
                    REQUIRE(displayManager.display(0)->uCurrentHeight == RESOURCE_HEIGHT);
                }

                AND_WHEN("GET_DISPLAY_INFO is used to receive the new resolution") {
                    runSimpleCommandAndCheck(&getDisplayInfo, &recvDisplayInfo, virtioGpu::ctrlType::resp::OK_DISPLAY_INFO);

                    THEN("the driver receives the new resolution and the display reports the new size") {
                        REQUIRE(recvDisplayInfo.pmodes[0].enabled != 0);
                        REQUIRE(recvDisplayInfo.pmodes[0].r.width == RESIZED_WIDTH);
                        REQUIRE(recvDisplayInfo.pmodes[0].r.height == RESIZED_HEIGHT);
                        REQUIRE(displayManager.display(0)->uCurrentWidth == RESIZED_WIDTH);
                        REQUIRE(displayManager.display(0)->uCurrentHeight == RESIZED_HEIGHT);
                    }
                }
            }
        }

        /*
         * TESTING - MULTI MONITOR
         */

        // Mirroring
        WHEN("A single framebuffer is linked to two monitors and TRANSFER_TO_HOST is called") {
            virtioGpu::setScanout setScanoutTwo2One {SCANOUT_ID_TWO, RESOURCE_ID_ONE, RESOURCE_WIDTH, RESOURCE_HEIGHT};
            runSimpleCommandAndCheck(&setScanoutOne, &recvDisplayInfo);
            runSimpleCommandAndCheck(&setScanoutTwo2One, &recvDisplayInfo);

            runSimpleCommandAndCheck(&transfer2HostOne, &recvHdr);

            THEN("Both scanouts have the same data in their framebuffers") {
                REQUIRE(compareFramebufBacking(backingPagesOne, SCANOUT_ID_ONE));
                REQUIRE(compareFramebufBacking(backingPagesOne, SCANOUT_ID_TWO));
            }
        }

        // Join Displays 1
        WHEN("Different framebuffers are linked to two monitors and TRANSFER_TO_HOST is called") {
            runSimpleCommandAndCheck(&setScanoutOne, &recvDisplayInfo);
            runSimpleCommandAndCheck(&setScanoutTwo, &recvDisplayInfo);

            runSimpleCommandAndCheck(&transfer2HostOne, &recvHdr);
            runSimpleCommandAndCheck(&transfer2HostTwo, &recvHdr);

            THEN("Both scanouts have the expected data in their framebuffers") {
                REQUIRE(compareFramebufBacking(backingPagesOne, SCANOUT_ID_ONE));
                REQUIRE(compareFramebufBacking(backingPagesTwo, SCANOUT_ID_TWO));
            }
        }
    }
}
