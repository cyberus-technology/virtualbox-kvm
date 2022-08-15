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

#include <VBox/VMMDev.h>
#include <VBox/vmm/pdmdev.h>

#include "../VirtIO/VirtioCore.h"

#include <cstdint>

/*
 * VIRTIOCORER3
 */

/** VIRTIOCORER3::pfnStatusChanged */
DECLCALLBACK(void) virtioGpuStatusChanged(PVIRTIOCORE pVirtio, PVIRTIOCORECC, uint32_t fDriverOk);

/** VIRTIOCORER3::pfnDevCapRead */
DECLCALLBACK(int) virtioGpuDevCapRead(PPDMDEVINS pDevIns, uint32_t uOffset, void* pvBuf, uint32_t cbToRead);

/** VIRTIOCORER3::pfnDevCapWrite */
DECLCALLBACK(int) virtioGpuDevCapWrite(PPDMDEVINS pDevIns, uint32_t uOffset, const void* pvBuf, uint32_t cbToWrite);

/** VIRTIOCORER3::pfnVirtqNotified */
DECLCALLBACK(void) virtioGpuVirtqNotified(PPDMDEVINS pDevIns, PVIRTIOCORE, uint16_t uVirtqNbr);

DECLCALLBACK(void)
virtioGpuDisplayChanged(PPDMDEVINS pDevIns, uint32_t numDisplays, VMMDevDisplayDef* displayDefs);

DECLCALLBACK(void*) virtioGpuQueryInterface(PPDMIBASE pInterface, const char* pszIID);

DECLCALLBACK(void) virtioGpuReset(PPDMDEVINS);
DECLCALLBACK(int) virtioGpuAttach(PPDMDEVINS, unsigned, uint32_t);
DECLCALLBACK(void) virtioGpuDetach(PPDMDEVINS, unsigned, uint32_t);
DECLCALLBACK(void*) virtioGpuPortQueryInterface(PPDMIBASE, const char*);
DECLCALLBACK(void) virtioGpuPortSetRenderVRAM(PPDMIDISPLAYPORT, bool);
DECLCALLBACK(int) virtioGpuUpdateDisplay(PPDMIDISPLAYPORT);
DECLCALLBACK(int) virtioGpuPortUpdateDisplayAll(PPDMIDISPLAYPORT, bool);
DECLCALLBACK(int) virtioGpuPortQueryVideoMode(PPDMIDISPLAYPORT, uint32_t*, uint32_t*, uint32_t*);
DECLCALLBACK(int) virtioGpuPortSetRefreshRate(PPDMIDISPLAYPORT, uint32_t);
DECLCALLBACK(int) virtioGpuPortTakeScreenshot(PPDMIDISPLAYPORT, uint8_t**, size_t*, uint32_t*, uint32_t*);
DECLCALLBACK(void) virtioGpuPortFreeScreenshot(PPDMIDISPLAYPORT, uint8_t*);
DECLCALLBACK(void) virtioGpuPortUpdateDisplayRect(PPDMIDISPLAYPORT, int32_t, int32_t, uint32_t, uint32_t);
DECLCALLBACK(int) virtioGpuPortDisplayBlt(PPDMIDISPLAYPORT, const void*, uint32_t, uint32_t, uint32_t, uint32_t);
DECLCALLBACK(int)
virtioGpuPortCopyRect(PPDMIDISPLAYPORT, uint32_t, uint32_t, const uint8_t*, int32_t, int32_t, uint32_t, uint32_t,
                      uint32_t, uint32_t, uint8_t*, int32_t, int32_t, uint32_t, uint32_t, uint32_t, uint32_t);
DECLCALLBACK(void) vmsvgavirtioGpuPortSetViewport(PPDMIDISPLAYPORT, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
DECLCALLBACK(int)
vbvavirtioGpuPortSendModeHint(PPDMIDISPLAYPORT, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                              uint32_t);
DECLCALLBACK(void) vbvavirtioGpuPortReportHostCursorCapabilities(PPDMIDISPLAYPORT, bool, bool);
DECLCALLBACK(void) vbvavirtioGpuPortReportHostCursorPosition(PPDMIDISPLAYPORT, uint32_t, uint32_t, bool);
