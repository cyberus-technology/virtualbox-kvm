/* $Id: RecordingStream.cpp $ */
/** @file
 * Recording stream code.
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

#ifdef LOG_GROUP
# undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_RECORDING
#include "LoggingNew.h"

#include <iprt/path.h>

#ifdef VBOX_RECORDING_DUMP
# include <iprt/formats/bmp.h>
#endif

#ifdef VBOX_WITH_AUDIO_RECORDING
# include <VBox/vmm/pdmaudioinline.h>
#endif

#include "Recording.h"
#include "RecordingUtils.h"
#include "WebMWriter.h"


RecordingStream::RecordingStream(RecordingContext *a_pCtx, uint32_t uScreen, const settings::RecordingScreenSettings &Settings)
    : m_enmState(RECORDINGSTREAMSTATE_UNINITIALIZED)
{
    int vrc2 = initInternal(a_pCtx, uScreen, Settings);
    if (RT_FAILURE(vrc2))
        throw vrc2;
}

RecordingStream::~RecordingStream(void)
{
    int vrc2 = uninitInternal();
    AssertRC(vrc2);
}

/**
 * Opens a recording stream.
 *
 * @returns VBox status code.
 * @param   screenSettings      Recording settings to use.
 */
int RecordingStream::open(const settings::RecordingScreenSettings &screenSettings)
{
    /* Sanity. */
    Assert(screenSettings.enmDest != RecordingDestination_None);

    int vrc;

    switch (screenSettings.enmDest)
    {
        case RecordingDestination_File:
        {
            Assert(screenSettings.File.strName.isNotEmpty());

            const char *pszFile = screenSettings.File.strName.c_str();

            RTFILE hFile = NIL_RTFILE;
            vrc = RTFileOpen(&hFile, pszFile, RTFILE_O_CREATE_REPLACE | RTFILE_O_WRITE | RTFILE_O_DENY_WRITE);
            if (RT_SUCCESS(vrc))
            {
                LogRel2(("Recording: Opened file '%s'\n", pszFile));

                try
                {
                    Assert(File.m_pWEBM == NULL);
                    File.m_pWEBM = new WebMWriter();
                }
                catch (std::bad_alloc &)
                {
                    vrc = VERR_NO_MEMORY;
                }

                if (RT_SUCCESS(vrc))
                {
                    this->File.m_hFile = hFile;
                    m_ScreenSettings.File.strName = pszFile;
                }
            }
            else
                LogRel(("Recording: Failed to open file '%s' for screen %RU32, vrc=%Rrc\n",
                        pszFile ? pszFile : "<Unnamed>", m_uScreenID, vrc));

            if (RT_FAILURE(vrc))
            {
                if (hFile != NIL_RTFILE)
                    RTFileClose(hFile);
            }

            break;
        }

        default:
            vrc = VERR_NOT_IMPLEMENTED;
            break;
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Returns the recording stream's used configuration.
 *
 * @returns The recording stream's used configuration.
 */
const settings::RecordingScreenSettings &RecordingStream::GetConfig(void) const
{
    return m_ScreenSettings;
}

/**
 * Checks if a specified limit for a recording stream has been reached, internal version.
 *
 * @returns @c true if any limit has been reached, @c false if not.
 * @param   msTimestamp         Timestamp (PTS, in ms) to check for.
 */
bool RecordingStream::isLimitReachedInternal(uint64_t msTimestamp) const
{
    LogFlowThisFunc(("msTimestamp=%RU64, ulMaxTimeS=%RU32, tsStartMs=%RU64\n",
                     msTimestamp, m_ScreenSettings.ulMaxTimeS, m_tsStartMs));

    if (   m_ScreenSettings.ulMaxTimeS
        && msTimestamp >= m_tsStartMs + (m_ScreenSettings.ulMaxTimeS * RT_MS_1SEC))
    {
        LogRel(("Recording: Time limit for stream #%RU16 has been reached (%RU32s)\n",
                m_uScreenID, m_ScreenSettings.ulMaxTimeS));
        return true;
    }

    if (m_ScreenSettings.enmDest == RecordingDestination_File)
    {
        if (m_ScreenSettings.File.ulMaxSizeMB)
        {
            uint64_t sizeInMB = this->File.m_pWEBM->GetFileSize() / _1M;
            if(sizeInMB >= m_ScreenSettings.File.ulMaxSizeMB)
            {
                LogRel(("Recording: File size limit for stream #%RU16 has been reached (%RU64MB)\n",
                        m_uScreenID, m_ScreenSettings.File.ulMaxSizeMB));
                return true;
            }
        }

        /* Check for available free disk space */
        if (   this->File.m_pWEBM
            && this->File.m_pWEBM->GetAvailableSpace() < 0x100000) /** @todo r=andy WTF? Fix this. */
        {
            LogRel(("Recording: Not enough free storage space available, stopping recording\n"));
            return true;
        }
    }

    return false;
}

/**
 * Internal iteration main loop.
 * Does housekeeping and recording context notification.
 *
 * @returns VBox status code.
 * @param   msTimestamp         Timestamp (PTS, in ms).
 */
int RecordingStream::iterateInternal(uint64_t msTimestamp)
{
    if (!m_fEnabled)
        return VINF_SUCCESS;

    int vrc;

    if (isLimitReachedInternal(msTimestamp))
    {
        vrc = VINF_RECORDING_LIMIT_REACHED;
    }
    else
        vrc = VINF_SUCCESS;

    AssertPtr(m_pCtx);

    switch (vrc)
    {
        case VINF_RECORDING_LIMIT_REACHED:
        {
            m_fEnabled = false;

            int vrc2 = m_pCtx->OnLimitReached(m_uScreenID, VINF_SUCCESS /* vrc */);
            AssertRC(vrc2);
            break;
        }

        default:
            break;
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Checks if a specified limit for a recording stream has been reached.
 *
 * @returns @c true if any limit has been reached, @c false if not.
 * @param   msTimestamp         Timestamp (PTS, in ms) to check for.
 */
bool RecordingStream::IsLimitReached(uint64_t msTimestamp) const
{
    if (!IsReady())
        return true;

    return isLimitReachedInternal(msTimestamp);
}

/**
 * Returns whether a recording stream is ready (e.g. enabled and active) or not.
 *
 * @returns @c true if ready, @c false if not.
 */
bool RecordingStream::IsReady(void) const
{
    return m_fEnabled;
}

/**
 * Returns if a recording stream needs to be fed with an update or not.
 *
 * @returns @c true if an update is needed, @c false if not.
 * @param   msTimestamp         Timestamp (PTS, in ms).
 */
bool RecordingStream::NeedsUpdate(uint64_t msTimestamp) const
{
    return recordingCodecGetWritable((const PRECORDINGCODEC)&m_CodecVideo, msTimestamp) > 0;
}

/**
 * Processes a recording stream.
 * This function takes care of the actual encoding and writing of a certain stream.
 * As this can be very CPU intensive, this function usually is called from a separate thread.
 *
 * @returns VBox status code.
 * @param   mapBlocksCommon     Map of common block to process for this stream.
 *
 * @note    Runs in recording thread.
 */
int RecordingStream::Process(RecordingBlockMap &mapBlocksCommon)
{
    LogFlowFuncEnter();

    lock();

    if (!m_ScreenSettings.fEnabled)
    {
        unlock();
        return VINF_SUCCESS;
    }

    int vrc = VINF_SUCCESS;

    RecordingBlockMap::iterator itStreamBlocks = m_Blocks.Map.begin();
    while (itStreamBlocks != m_Blocks.Map.end())
    {
        uint64_t const   msTimestamp = itStreamBlocks->first;
        RecordingBlocks *pBlocks     = itStreamBlocks->second;

        AssertPtr(pBlocks);

        while (!pBlocks->List.empty())
        {
            RecordingBlock *pBlock = pBlocks->List.front();
            AssertPtr(pBlock);

            switch (pBlock->enmType)
            {
                case RECORDINGBLOCKTYPE_VIDEO:
                {
                    RECORDINGFRAME Frame;
                    Frame.VideoPtr    = (PRECORDINGVIDEOFRAME)pBlock->pvData;
                    Frame.msTimestamp = msTimestamp;

                    int vrc2 = recordingCodecEncode(&m_CodecVideo, &Frame, NULL, NULL);
                    AssertRC(vrc2);
                    if (RT_SUCCESS(vrc))
                        vrc = vrc2;

                    break;
                }

                default:
                    /* Note: Audio data already is encoded. */
                    break;
            }

            pBlocks->List.pop_front();
            delete pBlock;
        }

        Assert(pBlocks->List.empty());
        delete pBlocks;

        m_Blocks.Map.erase(itStreamBlocks);
        itStreamBlocks = m_Blocks.Map.begin();
    }

#ifdef VBOX_WITH_AUDIO_RECORDING
    /* Do we need to multiplex the common audio data to this stream? */
    if (m_ScreenSettings.isFeatureEnabled(RecordingFeature_Audio))
    {
        /* As each (enabled) screen has to get the same audio data, look for common (audio) data which needs to be
         * written to the screen's assigned recording stream. */
        RecordingBlockMap::iterator itCommonBlocks = mapBlocksCommon.begin();
        while (itCommonBlocks != mapBlocksCommon.end())
        {
            RecordingBlockList::iterator itBlock = itCommonBlocks->second->List.begin();
            while (itBlock != itCommonBlocks->second->List.end())
            {
                RecordingBlock *pBlockCommon = (RecordingBlock *)(*itBlock);
                switch (pBlockCommon->enmType)
                {
                    case RECORDINGBLOCKTYPE_AUDIO:
                    {
                        PRECORDINGAUDIOFRAME pAudioFrame = (PRECORDINGAUDIOFRAME)pBlockCommon->pvData;
                        AssertPtr(pAudioFrame);
                        AssertPtr(pAudioFrame->pvBuf);
                        Assert(pAudioFrame->cbBuf);

                        AssertPtr(this->File.m_pWEBM);
                        int vrc2 = this->File.m_pWEBM->WriteBlock(m_uTrackAudio, pAudioFrame->pvBuf, pAudioFrame->cbBuf, pBlockCommon->msTimestamp, pBlockCommon->uFlags);
                        AssertRC(vrc2);
                        if (RT_SUCCESS(vrc))
                            vrc = vrc2;
                        break;
                    }

                    default:
                        AssertFailed();
                        break;
                }

                Assert(pBlockCommon->cRefs);
                pBlockCommon->cRefs--;
                if (pBlockCommon->cRefs == 0)
                {
                    itCommonBlocks->second->List.erase(itBlock);
                    delete pBlockCommon;
                    itBlock = itCommonBlocks->second->List.begin();
                }
                else
                    ++itBlock;
            }

            /* If no entries are left over in the block map, remove it altogether. */
            if (itCommonBlocks->second->List.empty())
            {
                delete itCommonBlocks->second;
                mapBlocksCommon.erase(itCommonBlocks);
                itCommonBlocks = mapBlocksCommon.begin();
            }
            else
                ++itCommonBlocks;

            LogFunc(("Common blocks: %zu\n", mapBlocksCommon.size()));
        }
    }
#else
    RT_NOREF(mapBlocksCommon);
#endif /* VBOX_WITH_AUDIO_RECORDING */

    unlock();

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Sends a raw (e.g. not yet encoded) audio frame to the recording stream.
 *
 * @returns VBox status code.
 * @param   pvData              Pointer to audio data.
 * @param   cbData              Size (in bytes) of \a pvData.
 * @param   msTimestamp         Timestamp (PTS, in ms).
 */
int RecordingStream::SendAudioFrame(const void *pvData, size_t cbData, uint64_t msTimestamp)
{
    AssertPtrReturn(m_pCtx, VERR_WRONG_ORDER);
    AssertReturn(NeedsUpdate(msTimestamp), VINF_RECORDING_THROTTLED); /* We ASSUME that the caller checked that first. */

    Log3Func(("cbData=%zu, msTimestamp=%RU64\n", cbData, msTimestamp));

    /* As audio data is common across all streams, re-route this to the recording context, where
     * the data is being encoded and stored in the common blocks queue. */
    return m_pCtx->SendAudioFrame(pvData, cbData, msTimestamp);
}

/**
 * Sends a raw (e.g. not yet encoded) video frame to the recording stream.
 *
 * @returns VBox status code. Will return VINF_RECORDING_LIMIT_REACHED if the stream's recording
 *          limit has been reached or VINF_RECORDING_THROTTLED if the frame is too early for the current
 *          FPS setting.
 * @param   x                   Upper left (X) coordinate where the video frame starts.
 * @param   y                   Upper left (Y) coordinate where the video frame starts.
 * @param   uPixelFormat        Pixel format of the video frame.
 * @param   uBPP                Bits per pixel (BPP) of the video frame.
 * @param   uBytesPerLine       Bytes per line  of the video frame.
 * @param   uSrcWidth           Width (in pixels) of the video frame.
 * @param   uSrcHeight          Height (in pixels) of the video frame.
 * @param   puSrcData           Actual pixel data of the video frame.
 * @param   msTimestamp         Timestamp (PTS, in ms).
 */
int RecordingStream::SendVideoFrame(uint32_t x, uint32_t y, uint32_t uPixelFormat, uint32_t uBPP, uint32_t uBytesPerLine,
                                    uint32_t uSrcWidth, uint32_t uSrcHeight, uint8_t *puSrcData, uint64_t msTimestamp)
{
    AssertPtrReturn(m_pCtx, VERR_WRONG_ORDER);
    AssertReturn(NeedsUpdate(msTimestamp), VINF_RECORDING_THROTTLED); /* We ASSUME that the caller checked that first. */

    lock();

    Log3Func(("[%RU32 %RU32 %RU32 %RU32] msTimestamp=%RU64\n", x , y, uSrcWidth, uSrcHeight, msTimestamp));

    PRECORDINGVIDEOFRAME pFrame = NULL;

    int vrc = iterateInternal(msTimestamp);
    if (vrc != VINF_SUCCESS) /* Can return VINF_RECORDING_LIMIT_REACHED. */
    {
        unlock();
        return vrc;
    }

    do
    {
        int xDiff = ((int)m_ScreenSettings.Video.ulWidth - (int)uSrcWidth) / 2;
        uint32_t w = uSrcWidth;
        if ((int)w + xDiff + (int)x <= 0)  /* Nothing visible. */
        {
            vrc = VERR_INVALID_PARAMETER;
            break;
        }

        uint32_t destX;
        if ((int)x < -xDiff)
        {
            w += xDiff + x;
            x = -xDiff;
            destX = 0;
        }
        else
            destX = x + xDiff;

        uint32_t h = uSrcHeight;
        int yDiff = ((int)m_ScreenSettings.Video.ulHeight - (int)uSrcHeight) / 2;
        if ((int)h + yDiff + (int)y <= 0)  /* Nothing visible. */
        {
            vrc = VERR_INVALID_PARAMETER;
            break;
        }

        uint32_t destY;
        if ((int)y < -yDiff)
        {
            h += yDiff + (int)y;
            y = -yDiff;
            destY = 0;
        }
        else
            destY = y + yDiff;

        if (   destX > m_ScreenSettings.Video.ulWidth
            || destY > m_ScreenSettings.Video.ulHeight)
        {
            vrc = VERR_INVALID_PARAMETER;  /* Nothing visible. */
            break;
        }

        if (destX + w > m_ScreenSettings.Video.ulWidth)
            w = m_ScreenSettings.Video.ulWidth - destX;

        if (destY + h > m_ScreenSettings.Video.ulHeight)
            h = m_ScreenSettings.Video.ulHeight - destY;

        pFrame = (PRECORDINGVIDEOFRAME)RTMemAllocZ(sizeof(RECORDINGVIDEOFRAME));
        AssertBreakStmt(pFrame, vrc = VERR_NO_MEMORY);

        /* Calculate bytes per pixel and set pixel format. */
        const unsigned uBytesPerPixel = uBPP / 8;
        if (uPixelFormat == BitmapFormat_BGR)
        {
            switch (uBPP)
            {
                case 32:
                    pFrame->enmPixelFmt = RECORDINGPIXELFMT_RGB32;
                    break;
                case 24:
                    pFrame->enmPixelFmt = RECORDINGPIXELFMT_RGB24;
                    break;
                case 16:
                    pFrame->enmPixelFmt = RECORDINGPIXELFMT_RGB565;
                    break;
                default:
                    AssertMsgFailedBreakStmt(("Unknown color depth (%RU32)\n", uBPP), vrc = VERR_NOT_SUPPORTED);
                    break;
            }
        }
        else
            AssertMsgFailedBreakStmt(("Unknown pixel format (%RU32)\n", uPixelFormat), vrc = VERR_NOT_SUPPORTED);

        const size_t cbRGBBuf =   m_ScreenSettings.Video.ulWidth
                                * m_ScreenSettings.Video.ulHeight
                                * uBytesPerPixel;
        AssertBreakStmt(cbRGBBuf, vrc = VERR_INVALID_PARAMETER);

        pFrame->pu8RGBBuf = (uint8_t *)RTMemAlloc(cbRGBBuf);
        AssertBreakStmt(pFrame->pu8RGBBuf, vrc = VERR_NO_MEMORY);
        pFrame->cbRGBBuf  = cbRGBBuf;
        pFrame->uWidth    = uSrcWidth;
        pFrame->uHeight   = uSrcHeight;

        /* If the current video frame is smaller than video resolution we're going to encode,
         * clear the frame beforehand to prevent artifacts. */
        if (   uSrcWidth  < m_ScreenSettings.Video.ulWidth
            || uSrcHeight < m_ScreenSettings.Video.ulHeight)
        {
            RT_BZERO(pFrame->pu8RGBBuf, pFrame->cbRGBBuf);
        }

        /* Calculate start offset in source and destination buffers. */
        uint32_t offSrc = y * uBytesPerLine + x * uBytesPerPixel;
        uint32_t offDst = (destY * m_ScreenSettings.Video.ulWidth + destX) * uBytesPerPixel;

#ifdef VBOX_RECORDING_DUMP
        BMPFILEHDR fileHdr;
        RT_ZERO(fileHdr);

        BMPWIN3XINFOHDR coreHdr;
        RT_ZERO(coreHdr);

        fileHdr.uType       = BMP_HDR_MAGIC;
        fileHdr.cbFileSize = (uint32_t)(sizeof(BMPFILEHDR) + sizeof(BMPWIN3XINFOHDR) + (w * h * uBytesPerPixel));
        fileHdr.offBits    = (uint32_t)(sizeof(BMPFILEHDR) + sizeof(BMPWIN3XINFOHDR));

        coreHdr.cbSize         = sizeof(BMPWIN3XINFOHDR);
        coreHdr.uWidth         = w;
        coreHdr.uHeight        = h;
        coreHdr.cPlanes        = 1;
        coreHdr.cBits          = uBPP;
        coreHdr.uXPelsPerMeter = 5000;
        coreHdr.uYPelsPerMeter = 5000;

        char szFileName[RTPATH_MAX];
        RTStrPrintf2(szFileName, sizeof(szFileName), "/tmp/VideoRecFrame-%RU32.bmp", m_uScreenID);

        RTFILE fh;
        int vrc2 = RTFileOpen(&fh, szFileName,
                              RTFILE_O_CREATE_REPLACE | RTFILE_O_WRITE | RTFILE_O_DENY_NONE);
        if (RT_SUCCESS(vrc2))
        {
            RTFileWrite(fh, &fileHdr,    sizeof(fileHdr),    NULL);
            RTFileWrite(fh, &coreHdr, sizeof(coreHdr), NULL);
        }
#endif
        Assert(pFrame->cbRGBBuf >= w * h * uBytesPerPixel);

        /* Do the copy. */
        for (unsigned int i = 0; i < h; i++)
        {
            /* Overflow check. */
            Assert(offSrc + w * uBytesPerPixel <= uSrcHeight * uBytesPerLine);
            Assert(offDst + w * uBytesPerPixel <= m_ScreenSettings.Video.ulHeight * m_ScreenSettings.Video.ulWidth * uBytesPerPixel);

            memcpy(pFrame->pu8RGBBuf + offDst, puSrcData + offSrc, w * uBytesPerPixel);

#ifdef VBOX_RECORDING_DUMP
            if (RT_SUCCESS(rc2))
                RTFileWrite(fh, pFrame->pu8RGBBuf + offDst, w * uBytesPerPixel, NULL);
#endif
            offSrc += uBytesPerLine;
            offDst += m_ScreenSettings.Video.ulWidth * uBytesPerPixel;
        }

#ifdef VBOX_RECORDING_DUMP
        if (RT_SUCCESS(vrc2))
            RTFileClose(fh);
#endif

    } while (0);

    if (vrc == VINF_SUCCESS) /* Note: Also could be VINF_TRY_AGAIN. */
    {
        RecordingBlock *pBlock = new RecordingBlock();
        if (pBlock)
        {
            AssertPtr(pFrame);

            pBlock->enmType = RECORDINGBLOCKTYPE_VIDEO;
            pBlock->pvData  = pFrame;
            pBlock->cbData  = sizeof(RECORDINGVIDEOFRAME) + pFrame->cbRGBBuf;

            try
            {
                RecordingBlocks *pRecordingBlocks = new RecordingBlocks();
                pRecordingBlocks->List.push_back(pBlock);

                Assert(m_Blocks.Map.find(msTimestamp) == m_Blocks.Map.end());
                m_Blocks.Map.insert(std::make_pair(msTimestamp, pRecordingBlocks));
            }
            catch (const std::exception &ex)
            {
                RT_NOREF(ex);

                delete pBlock;
                vrc = VERR_NO_MEMORY;
            }
        }
        else
            vrc = VERR_NO_MEMORY;
    }

    if (RT_FAILURE(vrc))
        RecordingVideoFrameFree(pFrame);

    unlock();

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Initializes a recording stream.
 *
 * @returns VBox status code.
 * @param   pCtx                Pointer to recording context.
 * @param   uScreen             Screen number to use for this recording stream.
 * @param   Settings            Recording screen configuration to use for initialization.
 */
int RecordingStream::Init(RecordingContext *pCtx, uint32_t uScreen, const settings::RecordingScreenSettings &Settings)
{
    return initInternal(pCtx, uScreen, Settings);
}

/**
 * Initializes a recording stream, internal version.
 *
 * @returns VBox status code.
 * @param   pCtx                Pointer to recording context.
 * @param   uScreen             Screen number to use for this recording stream.
 * @param   screenSettings      Recording screen configuration to use for initialization.
 */
int RecordingStream::initInternal(RecordingContext *pCtx, uint32_t uScreen,
                                  const settings::RecordingScreenSettings &screenSettings)
{
    AssertReturn(m_enmState == RECORDINGSTREAMSTATE_UNINITIALIZED, VERR_WRONG_ORDER);

    m_pCtx         = pCtx;
    m_uTrackAudio    = UINT8_MAX;
    m_uTrackVideo    = UINT8_MAX;
    m_tsStartMs      = 0;
    m_uScreenID      = uScreen;
#ifdef VBOX_WITH_AUDIO_RECORDING
    /* We use the codec from the recording context, as this stream only receives multiplexed data (same audio for all streams). */
    m_pCodecAudio    = m_pCtx->GetCodecAudio();
#endif
    m_ScreenSettings = screenSettings;

    settings::RecordingScreenSettings *pSettings = &m_ScreenSettings;

    int vrc = RTCritSectInit(&m_CritSect);
    if (RT_FAILURE(vrc))
        return vrc;

    this->File.m_pWEBM = NULL;
    this->File.m_hFile = NIL_RTFILE;

    vrc = open(*pSettings);
    if (RT_FAILURE(vrc))
        return vrc;

    const bool fVideoEnabled = pSettings->isFeatureEnabled(RecordingFeature_Video);
    const bool fAudioEnabled = pSettings->isFeatureEnabled(RecordingFeature_Audio);

    if (fVideoEnabled)
    {
        vrc = initVideo(*pSettings);
        if (RT_FAILURE(vrc))
            return vrc;
    }

    switch (pSettings->enmDest)
    {
        case RecordingDestination_File:
        {
            Assert(pSettings->File.strName.isNotEmpty());
            const char *pszFile = pSettings->File.strName.c_str();

            AssertPtr(File.m_pWEBM);
            vrc = File.m_pWEBM->OpenEx(pszFile, &this->File.m_hFile,
                                     fAudioEnabled ? pSettings->Audio.enmCodec : RecordingAudioCodec_None,
                                     fVideoEnabled ? pSettings->Video.enmCodec : RecordingVideoCodec_None);
            if (RT_FAILURE(vrc))
            {
                LogRel(("Recording: Failed to create output file '%s' (%Rrc)\n", pszFile, vrc));
                break;
            }

            if (fVideoEnabled)
            {
                vrc = this->File.m_pWEBM->AddVideoTrack(&m_CodecVideo,
                                                      pSettings->Video.ulWidth, pSettings->Video.ulHeight, pSettings->Video.ulFPS,
                                                      &m_uTrackVideo);
                if (RT_FAILURE(vrc))
                {
                    LogRel(("Recording: Failed to add video track to output file '%s' (%Rrc)\n", pszFile, vrc));
                    break;
                }

                LogRel(("Recording: Recording video of screen #%u with %RU32x%RU32 @ %RU32 kbps, %RU32 FPS (track #%RU8)\n",
                        m_uScreenID, pSettings->Video.ulWidth, pSettings->Video.ulHeight,
                        pSettings->Video.ulRate, pSettings->Video.ulFPS, m_uTrackVideo));
            }

#ifdef VBOX_WITH_AUDIO_RECORDING
            if (fAudioEnabled)
            {
                AssertPtr(m_pCodecAudio);
                vrc = this->File.m_pWEBM->AddAudioTrack(m_pCodecAudio,
                                                      pSettings->Audio.uHz, pSettings->Audio.cChannels, pSettings->Audio.cBits,
                                                      &m_uTrackAudio);
                if (RT_FAILURE(vrc))
                {
                    LogRel(("Recording: Failed to add audio track to output file '%s' (%Rrc)\n", pszFile, vrc));
                    break;
                }

                LogRel(("Recording: Recording audio of screen #%u in %RU16Hz, %RU8 bit, %RU8 %s (track #%RU8)\n",
                        m_uScreenID, pSettings->Audio.uHz, pSettings->Audio.cBits, pSettings->Audio.cChannels,
                        pSettings->Audio.cChannels ? "channels" : "channel", m_uTrackAudio));
            }
#endif

            if (   fVideoEnabled
#ifdef VBOX_WITH_AUDIO_RECORDING
                || fAudioEnabled
#endif
               )
            {
                char szWhat[32] = { 0 };
                if (fVideoEnabled)
                    RTStrCat(szWhat, sizeof(szWhat), "video");
#ifdef VBOX_WITH_AUDIO_RECORDING
                if (fAudioEnabled)
                {
                    if (fVideoEnabled)
                        RTStrCat(szWhat, sizeof(szWhat), " + ");
                    RTStrCat(szWhat, sizeof(szWhat), "audio");
                }
#endif
                LogRel(("Recording: Recording %s of screen #%u to '%s'\n", szWhat, m_uScreenID, pszFile));
            }

            break;
        }

        default:
            AssertFailed(); /* Should never happen. */
            vrc = VERR_NOT_IMPLEMENTED;
            break;
    }

    if (RT_SUCCESS(vrc))
    {
        m_enmState  = RECORDINGSTREAMSTATE_INITIALIZED;
        m_fEnabled  = true;
        m_tsStartMs = RTTimeProgramMilliTS();

        return VINF_SUCCESS;
    }

    int vrc2 = uninitInternal();
    AssertRC(vrc2);

    LogRel(("Recording: Stream #%RU32 initialization failed with %Rrc\n", uScreen, vrc));
    return vrc;
}

/**
 * Closes a recording stream.
 * Depending on the stream's recording destination, this function closes all associated handles
 * and finalizes recording.
 *
 * @returns VBox status code.
 */
int RecordingStream::close(void)
{
    int vrc = VINF_SUCCESS;

    switch (m_ScreenSettings.enmDest)
    {
        case RecordingDestination_File:
        {
            if (this->File.m_pWEBM)
                vrc = this->File.m_pWEBM->Close();
            break;
        }

        default:
            AssertFailed(); /* Should never happen. */
            break;
    }

    m_Blocks.Clear();

    LogRel(("Recording: Recording screen #%u stopped\n", m_uScreenID));

    if (RT_FAILURE(vrc))
    {
        LogRel(("Recording: Error stopping recording screen #%u, vrc=%Rrc\n", m_uScreenID, vrc));
        return vrc;
    }

    switch (m_ScreenSettings.enmDest)
    {
        case RecordingDestination_File:
        {
            if (RTFileIsValid(this->File.m_hFile))
            {
                vrc = RTFileClose(this->File.m_hFile);
                if (RT_SUCCESS(vrc))
                {
                    LogRel(("Recording: Closed file '%s'\n", m_ScreenSettings.File.strName.c_str()));
                }
                else
                {
                    LogRel(("Recording: Error closing file '%s', vrc=%Rrc\n", m_ScreenSettings.File.strName.c_str(), vrc));
                    break;
                }
            }

            WebMWriter *pWebMWriter = this->File.m_pWEBM;
            AssertPtr(pWebMWriter);

            if (pWebMWriter)
            {
                /* If no clusters (= data) was written, delete the file again. */
                if (pWebMWriter->GetClusters() == 0)
                {
                    int vrc2 = RTFileDelete(m_ScreenSettings.File.strName.c_str());
                    AssertRC(vrc2); /* Ignore vrc on non-debug builds. */
                }

                delete pWebMWriter;
                pWebMWriter = NULL;

                this->File.m_pWEBM = NULL;
            }
            break;
        }

        default:
            vrc = VERR_NOT_IMPLEMENTED;
            break;
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Uninitializes a recording stream.
 *
 * @returns VBox status code.
 */
int RecordingStream::Uninit(void)
{
    return uninitInternal();
}

/**
 * Uninitializes a recording stream, internal version.
 *
 * @returns VBox status code.
 */
int RecordingStream::uninitInternal(void)
{
    if (m_enmState != RECORDINGSTREAMSTATE_INITIALIZED)
        return VINF_SUCCESS;

    int vrc = close();
    if (RT_FAILURE(vrc))
        return vrc;

#ifdef VBOX_WITH_AUDIO_RECORDING
    m_pCodecAudio = NULL;
#endif

    if (m_ScreenSettings.isFeatureEnabled(RecordingFeature_Video))
    {
        vrc = recordingCodecFinalize(&m_CodecVideo);
        if (RT_SUCCESS(vrc))
            vrc = recordingCodecDestroy(&m_CodecVideo);
    }

    if (RT_SUCCESS(vrc))
    {
        RTCritSectDelete(&m_CritSect);

        m_enmState = RECORDINGSTREAMSTATE_UNINITIALIZED;
        m_fEnabled = false;
    }

    return vrc;
}

/**
 * Writes encoded data to a WebM file instance.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec which has encoded the data.
 * @param   pvData              Encoded data to write.
 * @param   cbData              Size (in bytes) of \a pvData.
 * @param   msAbsPTS            Absolute PTS (in ms) of written data.
 * @param   uFlags              Encoding flags of type RECORDINGCODEC_ENC_F_XXX.
 */
int RecordingStream::codecWriteToWebM(PRECORDINGCODEC pCodec, const void *pvData, size_t cbData,
                                      uint64_t msAbsPTS, uint32_t uFlags)
{
    AssertPtr(this->File.m_pWEBM);
    AssertPtr(pvData);
    Assert   (cbData);

    WebMWriter::WebMBlockFlags blockFlags = VBOX_WEBM_BLOCK_FLAG_NONE;
    if (RT_LIKELY(uFlags != RECORDINGCODEC_ENC_F_NONE))
    {
        /* All set. */
    }
    else
    {
        if (uFlags & RECORDINGCODEC_ENC_F_BLOCK_IS_KEY)
            blockFlags |= VBOX_WEBM_BLOCK_FLAG_KEY_FRAME;
        if (uFlags & RECORDINGCODEC_ENC_F_BLOCK_IS_INVISIBLE)
            blockFlags |= VBOX_WEBM_BLOCK_FLAG_INVISIBLE;
    }

    return this->File.m_pWEBM->WriteBlock(  pCodec->Parms.enmType == RECORDINGCODECTYPE_AUDIO
                                        ? m_uTrackAudio : m_uTrackVideo,
                                        pvData, cbData, msAbsPTS, blockFlags);
}

/**
 * Codec callback for writing encoded data to a recording stream.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec which has encoded the data.
 * @param   pvData              Encoded data to write.
 * @param   cbData              Size (in bytes) of \a pvData.
 * @param   msAbsPTS            Absolute PTS (in ms) of written data.
 * @param   uFlags              Encoding flags of type RECORDINGCODEC_ENC_F_XXX.
 * @param   pvUser              User-supplied pointer.
 */
/* static */
DECLCALLBACK(int) RecordingStream::codecWriteDataCallback(PRECORDINGCODEC pCodec, const void *pvData, size_t cbData,
                                                          uint64_t msAbsPTS, uint32_t uFlags, void *pvUser)
{
    RecordingStream *pThis = (RecordingStream *)pvUser;
    AssertPtr(pThis);

    /** @todo For now this is hardcoded to always write to a WebM file. Add other stuff later. */
    return pThis->codecWriteToWebM(pCodec, pvData, cbData, msAbsPTS, uFlags);
}

/**
 * Initializes the video recording for a recording stream.
 *
 * @returns VBox status code.
 * @param   screenSettings      Screen settings to use.
 */
int RecordingStream::initVideo(const settings::RecordingScreenSettings &screenSettings)
{
    /* Sanity. */
    AssertReturn(screenSettings.Video.ulRate,   VERR_INVALID_PARAMETER);
    AssertReturn(screenSettings.Video.ulWidth,  VERR_INVALID_PARAMETER);
    AssertReturn(screenSettings.Video.ulHeight, VERR_INVALID_PARAMETER);
    AssertReturn(screenSettings.Video.ulFPS,    VERR_INVALID_PARAMETER);

    PRECORDINGCODEC pCodec = &m_CodecVideo;

    RECORDINGCODECCALLBACKS Callbacks;
    Callbacks.pvUser       = this;
    Callbacks.pfnWriteData = RecordingStream::codecWriteDataCallback;

    int vrc = recordingCodecCreateVideo(pCodec, screenSettings.Video.enmCodec);
    if (RT_SUCCESS(vrc))
        vrc = recordingCodecInit(pCodec, &Callbacks, screenSettings);

    if (RT_FAILURE(vrc))
        LogRel(("Recording: Initializing video codec failed with %Rrc\n", vrc));

    return vrc;
}

/**
 * Locks a recording stream.
 */
void RecordingStream::lock(void)
{
    int vrc = RTCritSectEnter(&m_CritSect);
    AssertRC(vrc);
}

/**
 * Unlocks a locked recording stream.
 */
void RecordingStream::unlock(void)
{
    int vrc = RTCritSectLeave(&m_CritSect);
    AssertRC(vrc);
}

