/* $Id: RecordingInternals.cpp $ */
/** @file
 * Recording internals code.
 */

/*
 * Copyright (C) 2012-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "RecordingInternals.h"

#include <iprt/assert.h>
#include <iprt/mem.h>

#ifdef VBOX_WITH_AUDIO_RECORDING

/**
 * Initializes a recording frame.
 *
 * @param   pFrame              Pointer to video frame to initialize.
 * @param   w                   Width (in pixel) of video frame.
 * @param   h                   Height (in pixel) of video frame.
 * @param   uBPP                Bits per pixel (BPP).
 * @param   enmPixelFmt         Pixel format to use.
 */
int RecordingVideoFrameInit(PRECORDINGVIDEOFRAME pFrame, int w, int h, uint8_t uBPP, RECORDINGPIXELFMT enmPixelFmt)
{
    /* Calculate bytes per pixel and set pixel format. */
    const unsigned uBytesPerPixel = uBPP / 8;
    const   size_t cbRGBBuf       = w * h * uBytesPerPixel;
    AssertReturn(cbRGBBuf, VERR_INVALID_PARAMETER);

    pFrame->pu8RGBBuf = (uint8_t *)RTMemAlloc(cbRGBBuf);
    AssertPtrReturn(pFrame->pu8RGBBuf, VERR_NO_MEMORY);
    pFrame->cbRGBBuf    = cbRGBBuf;

    pFrame->uX            = 0;
    pFrame->uY            = 0;
    pFrame->uWidth        = w;
    pFrame->uHeight       = h;
    pFrame->enmPixelFmt   = enmPixelFmt;
    pFrame->uBPP          = uBPP;
    pFrame->uBytesPerLine = w * uBytesPerPixel;

    return VINF_SUCCESS;
}

/**
 * Destroys a recording audio frame.
 *
 * @param   pFrame              Pointer to audio frame to destroy.
 */
static void recordingAudioFrameDestroy(PRECORDINGAUDIOFRAME pFrame)
{
    if (pFrame->pvBuf)
    {
        Assert(pFrame->cbBuf);
        RTMemFree(pFrame->pvBuf);
        pFrame->cbBuf = 0;
    }
}

/**
 * Frees a previously allocated recording audio frame.
 *
 * @param   pFrame              Audio frame to free. The pointer will be invalid after return.
 */
void RecordingAudioFrameFree(PRECORDINGAUDIOFRAME pFrame)
{
    if (!pFrame)
        return;

    recordingAudioFrameDestroy(pFrame);

    RTMemFree(pFrame);
    pFrame = NULL;
}

#endif /* VBOX_WITH_AUDIO_RECORDING */

/**
 * Destroys a recording video frame.
 *
 * @param   pFrame              Pointer to video frame to destroy.
 */
void RecordingVideoFrameDestroy(PRECORDINGVIDEOFRAME pFrame)
{
    if (pFrame->pu8RGBBuf)
    {
        Assert(pFrame->cbRGBBuf);
        RTMemFree(pFrame->pu8RGBBuf);
        pFrame->cbRGBBuf = 0;
    }
}

/**
 * Frees a recording video frame.
 *
 * @param   pFrame              Pointer to video frame to free. The pointer will be invalid after return.
 */
void RecordingVideoFrameFree(PRECORDINGVIDEOFRAME pFrame)
{
    if (!pFrame)
        return;

    RecordingVideoFrameDestroy(pFrame);

    RTMemFree(pFrame);
}

/**
 * Frees a recording frame.
 *
 * @param   pFrame              Pointer to recording frame to free. The pointer will be invalid after return.
 */
void RecordingFrameFree(PRECORDINGFRAME pFrame)
{
    if (!pFrame)
        return;

    switch (pFrame->enmType)
    {
#ifdef VBOX_WITH_AUDIO_RECORDING
        case RECORDINGFRAME_TYPE_AUDIO:
            recordingAudioFrameDestroy(&pFrame->Audio);
            break;
#endif
        case RECORDINGFRAME_TYPE_VIDEO:
            RecordingVideoFrameDestroy(&pFrame->Video);
            break;

        default:
            AssertFailed();
            break;
    }

    RTMemFree(pFrame);
    pFrame = NULL;
}

