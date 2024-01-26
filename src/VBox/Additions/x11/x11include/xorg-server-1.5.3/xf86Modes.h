/*
 * Copyright © 2006 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#ifndef _XF86MODES_H_
#define _XF86MODES_H_

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "xf86.h"
#include "xorgVersion.h"
#include "edid.h"
#include "xf86Parser.h"
#if XF86_MODES_RENAME
#include "xf86Rename.h"
#endif

double xf86ModeHSync(DisplayModePtr mode);
double xf86ModeVRefresh(DisplayModePtr mode);
unsigned int xf86ModeBandwidth(DisplayModePtr mode, int depth);

int
xf86ModeWidth (DisplayModePtr mode, Rotation rotation);

int
xf86ModeHeight (DisplayModePtr mode, Rotation rotation);

DisplayModePtr xf86DuplicateMode(DisplayModePtr pMode);
DisplayModePtr xf86DuplicateModes(ScrnInfoPtr pScrn,
				       DisplayModePtr modeList);
void xf86SetModeDefaultName(DisplayModePtr mode);
void xf86SetModeCrtc(DisplayModePtr p, int adjustFlags);
Bool xf86ModesEqual(DisplayModePtr pMode1, DisplayModePtr pMode2);
void xf86PrintModeline(int scrnIndex,DisplayModePtr mode);
DisplayModePtr xf86ModesAdd(DisplayModePtr modes, DisplayModePtr new);

DisplayModePtr xf86DDCGetModes(int scrnIndex, xf86MonPtr DDC);
DisplayModePtr xf86CVTMode(int HDisplay, int VDisplay, float VRefresh,
			   Bool Reduced, Bool Interlaced);
DisplayModePtr xf86GTFMode(int h_pixels, int v_lines, float freq, int interlaced, int margins);

void
xf86ValidateModesFlags(ScrnInfoPtr pScrn, DisplayModePtr modeList,
		       int flags);

void
xf86ValidateModesClocks(ScrnInfoPtr pScrn, DisplayModePtr modeList,
			int *min, int *max, int n_ranges);

void
xf86ValidateModesSize(ScrnInfoPtr pScrn, DisplayModePtr modeList,
		      int maxX, int maxY, int maxPitch);

void
xf86ValidateModesSync(ScrnInfoPtr pScrn, DisplayModePtr modeList,
		      MonPtr mon);

void
xf86ValidateModesBandwidth(ScrnInfoPtr pScrn, DisplayModePtr modeList,
			   unsigned int bandwidth, int depth);

void
xf86ValidateModesReducedBlanking(ScrnInfoPtr pScrn, DisplayModePtr modeList);

void
xf86PruneInvalidModes(ScrnInfoPtr pScrn, DisplayModePtr *modeList,
		      Bool verbose);

void
xf86ValidateModesFlags(ScrnInfoPtr pScrn, DisplayModePtr modeList,
		       int flags);

void
xf86ValidateModesUserConfig(ScrnInfoPtr pScrn, DisplayModePtr modeList);

DisplayModePtr
xf86GetMonitorModes (ScrnInfoPtr pScrn, XF86ConfMonitorPtr conf_monitor);

DisplayModePtr
xf86GetDefaultModes (Bool interlaceAllowed, Bool doubleScanAllowed);

void
xf86DDCApplyQuirks(int scrnIndex, xf86MonPtr DDC);

#endif /* _XF86MODES_H_ */
