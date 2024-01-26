/* $Id: RecordingUtils.cpp $ */
/** @file
 * Recording utility code.
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

#include "Recording.h"
#include "RecordingUtils.h"

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/path.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>
#include <iprt/time.h>

#ifdef DEBUG
#include <iprt/file.h>
#include <iprt/formats/bmp.h>
#endif


/**
 * Convert an image to YUV420p format.
 *
 * @return \c true on success, \c false on failure.
 * @param  aDstBuf              The destination image buffer.
 * @param  aDstWidth            Width (in pixel) of destination buffer.
 * @param  aDstHeight           Height (in pixel) of destination buffer.
 * @param  aSrcBuf              The source image buffer.
 * @param  aSrcWidth            Width (in pixel) of source buffer.
 * @param  aSrcHeight           Height (in pixel) of source buffer.
 */
template <class T>
inline bool recordingUtilsColorConvWriteYUV420p(uint8_t *aDstBuf, unsigned aDstWidth, unsigned aDstHeight,
                                                uint8_t *aSrcBuf, unsigned aSrcWidth, unsigned aSrcHeight)
{
    RT_NOREF(aDstWidth, aDstHeight);

    AssertReturn(!(aSrcWidth & 1),  false);
    AssertReturn(!(aSrcHeight & 1), false);

    bool fRc = true;
    T iter1(aSrcWidth, aSrcHeight, aSrcBuf);
    T iter2 = iter1;
    iter2.skip(aSrcWidth);
    unsigned cPixels = aSrcWidth * aSrcHeight;
    unsigned offY = 0;
    unsigned offU = cPixels;
    unsigned offV = cPixels + cPixels / 4;
    unsigned const cyHalf = aSrcHeight / 2;
    unsigned const cxHalf = aSrcWidth  / 2;
    for (unsigned i = 0; i < cyHalf && fRc; ++i)
    {
        for (unsigned j = 0; j < cxHalf; ++j)
        {
            unsigned red, green, blue;
            fRc = iter1.getRGB(&red, &green, &blue);
            AssertReturn(fRc, false);
            aDstBuf[offY] = ((66 * red + 129 * green + 25 * blue + 128) >> 8) + 16;
            unsigned u = (((-38 * red - 74 * green + 112 * blue + 128) >> 8) + 128) / 4;
            unsigned v = (((112 * red - 94 * green -  18 * blue + 128) >> 8) + 128) / 4;

            fRc = iter1.getRGB(&red, &green, &blue);
            AssertReturn(fRc, false);
            aDstBuf[offY + 1] = ((66 * red + 129 * green + 25 * blue + 128) >> 8) + 16;
            u += (((-38 * red - 74 * green + 112 * blue + 128) >> 8) + 128) / 4;
            v += (((112 * red - 94 * green -  18 * blue + 128) >> 8) + 128) / 4;

            fRc = iter2.getRGB(&red, &green, &blue);
            AssertReturn(fRc, false);
            aDstBuf[offY + aSrcWidth] = ((66 * red + 129 * green + 25 * blue + 128) >> 8) + 16;
            u += (((-38 * red - 74 * green + 112 * blue + 128) >> 8) + 128) / 4;
            v += (((112 * red - 94 * green -  18 * blue + 128) >> 8) + 128) / 4;

            fRc = iter2.getRGB(&red, &green, &blue);
            AssertReturn(fRc, false);
            aDstBuf[offY + aSrcWidth + 1] = ((66 * red + 129 * green + 25 * blue + 128) >> 8) + 16;
            u += (((-38 * red - 74 * green + 112 * blue + 128) >> 8) + 128) / 4;
            v += (((112 * red - 94 * green -  18 * blue + 128) >> 8) + 128) / 4;

            aDstBuf[offU] = u;
            aDstBuf[offV] = v;
            offY += 2;
            ++offU;
            ++offV;
        }

        iter1.skip(aSrcWidth);
        iter2.skip(aSrcWidth);
        offY += aSrcWidth;
    }

    return true;
}

/**
 * Convert an image to RGB24 format.
 *
 * @returns true on success, false on failure.
 * @param aWidth    Width of image.
 * @param aHeight   Height of image.
 * @param aDestBuf  An allocated memory buffer large enough to hold the
 *                  destination image (i.e. width * height * 12bits).
 * @param aSrcBuf   The source image as an array of bytes.
 */
template <class T>
inline bool RecordingUtilsColorConvWriteRGB24(unsigned aWidth, unsigned aHeight,
                                              uint8_t *aDestBuf, uint8_t *aSrcBuf)
{
    enum { PIX_SIZE = 3 };
    bool fRc = true;
    AssertReturn(0 == (aWidth & 1), false);
    AssertReturn(0 == (aHeight & 1), false);
    T iter(aWidth, aHeight, aSrcBuf);
    unsigned cPixels = aWidth * aHeight;
    for (unsigned i = 0; i < cPixels && fRc; ++i)
    {
        unsigned red, green, blue;
        fRc = iter.getRGB(&red, &green, &blue);
        if (fRc)
        {
            aDestBuf[i * PIX_SIZE    ] = red;
            aDestBuf[i * PIX_SIZE + 1] = green;
            aDestBuf[i * PIX_SIZE + 2] = blue;
        }
    }
    return fRc;
}

/**
 * Converts a RGB to YUV buffer.
 *
 * @returns IPRT status code.
 * @param   enmPixelFormat      Pixel format to use for conversion.
 * @param   paDst               Pointer to destination buffer.
 * @param   uDstWidth           Width (X, in pixels) of destination buffer.
 * @param   uDstHeight          Height (Y, in pixels) of destination buffer.
 * @param   paSrc               Pointer to source buffer.
 * @param   uSrcWidth           Width (X, in pixels) of source buffer.
 * @param   uSrcHeight          Height (Y, in pixels) of source buffer.
 */
int RecordingUtilsRGBToYUV(RECORDINGPIXELFMT enmPixelFormat,
                           uint8_t *paDst, uint32_t uDstWidth, uint32_t uDstHeight,
                           uint8_t *paSrc, uint32_t uSrcWidth, uint32_t uSrcHeight)
{
    switch (enmPixelFormat)
    {
        case RECORDINGPIXELFMT_RGB32:
            if (!recordingUtilsColorConvWriteYUV420p<ColorConvBGRA32Iter>(paDst, uDstWidth, uDstHeight,
                                                                          paSrc, uSrcWidth, uSrcHeight))
                return VERR_INVALID_PARAMETER;
            break;
        case RECORDINGPIXELFMT_RGB24:
            if (!recordingUtilsColorConvWriteYUV420p<ColorConvBGR24Iter>(paDst, uDstWidth, uDstHeight,
                                                                         paSrc, uSrcWidth, uSrcHeight))
                return VERR_INVALID_PARAMETER;
            break;
        case RECORDINGPIXELFMT_RGB565:
            if (!recordingUtilsColorConvWriteYUV420p<ColorConvBGR565Iter>(paDst, uDstWidth, uDstHeight,
                                                                          paSrc, uSrcWidth, uSrcHeight))
                return VERR_INVALID_PARAMETER;
            break;
        default:
            AssertFailed();
            return VERR_NOT_SUPPORTED;
    }
    return VINF_SUCCESS;
}

#ifdef DEBUG
/**
 * Dumps a video recording frame to a bitmap (BMP) file, extended version.
 *
 * @returns VBox status code.
 * @param   pu8RGBBuf           Pointer to actual RGB frame data.
 * @param   cbRGBBuf            Size (in bytes) of \a pu8RGBBuf.
 * @param   pszPath             Absolute path to dump file to. Must exist.
 *                              Specify NULL to use the system's temp directory.
 *                              Existing frame files will be overwritten.
 * @param   pszPrefx            Naming prefix to use. Optional and can be NULL.
 * @param   uWidth              Width (in pixel) to write.
 * @param   uHeight             Height (in pixel) to write.
 * @param   uBPP                Bits in pixel.
 */
int RecordingUtilsDbgDumpFrameEx(const uint8_t *pu8RGBBuf, size_t cbRGBBuf, const char *pszPath, const char *pszPrefx,
                                 uint16_t uWidth, uint32_t uHeight, uint8_t uBPP)
{
    RT_NOREF(cbRGBBuf);

    const uint8_t  uBytesPerPixel = uBPP / 8 /* Bits */;
    const size_t   cbData         = uWidth * uHeight * uBytesPerPixel;

    if (!cbData) /* No data to write? Bail out early. */
        return VINF_SUCCESS;

    BMPFILEHDR fileHdr;
    RT_ZERO(fileHdr);

    BMPWIN3XINFOHDR coreHdr;
    RT_ZERO(coreHdr);

    fileHdr.uType      = BMP_HDR_MAGIC;
    fileHdr.cbFileSize = (uint32_t)(sizeof(BMPFILEHDR) + sizeof(BMPWIN3XINFOHDR) + cbData);
    fileHdr.offBits    = (uint32_t)(sizeof(BMPFILEHDR) + sizeof(BMPWIN3XINFOHDR));

    coreHdr.cbSize         = sizeof(BMPWIN3XINFOHDR);
    coreHdr.uWidth         = uWidth ;
    coreHdr.uHeight        = uHeight;
    coreHdr.cPlanes        = 1;
    coreHdr.cBits          = uBPP;
    coreHdr.uXPelsPerMeter = 5000;
    coreHdr.uYPelsPerMeter = 5000;

    static uint64_t s_iCount = 0;

    char szPath[RTPATH_MAX];
    if (!pszPath)
        RTPathTemp(szPath, sizeof(szPath));

    char szFileName[RTPATH_MAX];
    if (RTStrPrintf2(szFileName, sizeof(szFileName), "%s/RecDump-%04RU64-%s-w%RU16h%RU16.bmp",
                     pszPath ? pszPath : szPath, s_iCount, pszPrefx ? pszPrefx : "Frame", uWidth, uHeight) <= 0)
    {
        return VERR_BUFFER_OVERFLOW;
    }

    s_iCount++;

    RTFILE fh;
    int vrc = RTFileOpen(&fh, szFileName,
                         RTFILE_O_CREATE_REPLACE | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
    if (RT_SUCCESS(vrc))
    {
        RTFileWrite(fh, &fileHdr, sizeof(fileHdr), NULL);
        RTFileWrite(fh, &coreHdr, sizeof(coreHdr), NULL);

        /* Bitmaps (DIBs) are stored upside-down (thanks, OS/2), so work from the bottom up. */
        uint32_t offSrc = (uHeight * uWidth * uBytesPerPixel) - uWidth * uBytesPerPixel;

        /* Do the copy. */
        for (unsigned int i = 0; i < uHeight; i++)
        {
            RTFileWrite(fh, pu8RGBBuf + offSrc, uWidth * uBytesPerPixel, NULL);
            offSrc -= uWidth * uBytesPerPixel;
        }

        RTFileClose(fh);
    }

    return vrc;
}

/**
 * Dumps a video recording frame to a bitmap (BMP) file.
 *
 * @returns VBox status code.
 * @param   pFrame              Video frame to dump.
 */
int RecordingUtilsDbgDumpFrame(const PRECORDINGFRAME pFrame)
{
    AssertReturn(pFrame->enmType == RECORDINGFRAME_TYPE_VIDEO, VERR_INVALID_PARAMETER);
    return RecordingUtilsDbgDumpFrameEx(pFrame->Video.pu8RGBBuf, pFrame->Video.cbRGBBuf,
                                        NULL /*  Use temp directory */, NULL /* pszPrefix */,
                                        pFrame->Video.uWidth, pFrame->Video.uHeight, pFrame->Video.uBPP);
}
#endif /* DEBUG */

