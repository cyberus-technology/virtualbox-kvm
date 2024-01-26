/* $Id: Recording.cpp $ */
/** @file
 * Recording context code.
 *
 * This code employs a separate encoding thread per recording context
 * to keep time spent in EMT as short as possible. Each configured VM display
 * is represented by an own recording stream, which in turn has its own rendering
 * queue. Common recording data across all recording streams is kept in a
 * separate queue in the recording context to minimize data duplication and
 * multiplexing overhead in EMT.
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

#include <stdexcept>
#include <vector>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/path.h>
#include <iprt/semaphore.h>
#include <iprt/thread.h>
#include <iprt/time.h>

#include <VBox/err.h>
#include <VBox/com/VirtualBox.h>

#include "ConsoleImpl.h"
#include "Recording.h"
#include "RecordingInternals.h"
#include "RecordingStream.h"
#include "RecordingUtils.h"
#include "WebMWriter.h"

using namespace com;

#ifdef DEBUG_andy
/** Enables dumping audio / video data for debugging reasons. */
//# define VBOX_RECORDING_DUMP
#endif


/**
 * Recording context constructor.
 *
 * @note    Will throw vrc when unable to create.
 */
RecordingContext::RecordingContext(void)
    : m_pConsole(NULL)
    , m_enmState(RECORDINGSTS_UNINITIALIZED)
    , m_cStreamsEnabled(0)
{
    int vrc = RTCritSectInit(&m_CritSect);
    if (RT_FAILURE(vrc))
        throw vrc;
}

/**
 * Recording context constructor.
 *
 * @param   ptrConsole          Pointer to console object this context is bound to (weak pointer).
 * @param   Settings            Reference to recording settings to use for creation.
 *
 * @note    Will throw vrc when unable to create.
 */
RecordingContext::RecordingContext(Console *ptrConsole, const settings::RecordingSettings &Settings)
    : m_pConsole(NULL)
    , m_enmState(RECORDINGSTS_UNINITIALIZED)
    , m_cStreamsEnabled(0)
{
    int vrc = RTCritSectInit(&m_CritSect);
    if (RT_FAILURE(vrc))
        throw vrc;

    vrc = RecordingContext::createInternal(ptrConsole, Settings);
    if (RT_FAILURE(vrc))
        throw vrc;
}

RecordingContext::~RecordingContext(void)
{
    destroyInternal();

    if (RTCritSectIsInitialized(&m_CritSect))
        RTCritSectDelete(&m_CritSect);
}

/**
 * Worker thread for all streams of a recording context.
 *
 * For video frames, this also does the RGB/YUV conversion and encoding.
 */
DECLCALLBACK(int) RecordingContext::threadMain(RTTHREAD hThreadSelf, void *pvUser)
{
    RecordingContext *pThis = (RecordingContext *)pvUser;

    /* Signal that we're up and rockin'. */
    RTThreadUserSignal(hThreadSelf);

    LogRel2(("Recording: Thread started\n"));

    for (;;)
    {
        int vrc = RTSemEventWait(pThis->m_WaitEvent, RT_INDEFINITE_WAIT);
        AssertRCBreak(vrc);

        Log2Func(("Processing %zu streams\n", pThis->m_vecStreams.size()));

        /* Process common raw blocks (data which not has been encoded yet). */
        vrc = pThis->processCommonData(pThis->m_mapBlocksRaw, 100 /* ms timeout */);

        /** @todo r=andy This is inefficient -- as we already wake up this thread
         *               for every screen from Main, we here go again (on every wake up) through
         *               all screens.  */
        RecordingStreams::iterator itStream = pThis->m_vecStreams.begin();
        while (itStream != pThis->m_vecStreams.end())
        {
            RecordingStream *pStream = (*itStream);

            /* Hand-in common encoded blocks. */
            vrc = pStream->Process(pThis->m_mapBlocksEncoded);
            if (RT_FAILURE(vrc))
            {
                LogRel(("Recording: Processing stream #%RU16 failed (%Rrc)\n", pStream->GetID(), vrc));
                break;
            }

            ++itStream;
        }

        if (RT_FAILURE(vrc))
            LogRel(("Recording: Encoding thread failed (%Rrc)\n", vrc));

        /* Keep going in case of errors. */

        if (ASMAtomicReadBool(&pThis->m_fShutdown))
        {
            LogFunc(("Thread is shutting down ...\n"));
            break;
        }

    } /* for */

    LogRel2(("Recording: Thread ended\n"));
    return VINF_SUCCESS;
}

/**
 * Notifies a recording context's encoding thread.
 *
 * @returns VBox status code.
 */
int RecordingContext::threadNotify(void)
{
    return RTSemEventSignal(m_WaitEvent);
}

/**
 * Worker function for processing common block data.
 *
 * @returns VBox status code.
 * @param   mapCommon           Common block map to handle.
 * @param   msTimeout           Timeout to use for maximum time spending to process data.
 *                              Use RT_INDEFINITE_WAIT for processing all data.
 *
 * @note    Runs in recording thread.
 */
int RecordingContext::processCommonData(RecordingBlockMap &mapCommon, RTMSINTERVAL msTimeout)
{
    Log2Func(("Processing %zu common blocks (%RU32ms timeout)\n", mapCommon.size(), msTimeout));

    int vrc = VINF_SUCCESS;

    uint64_t const msStart = RTTimeMilliTS();
    RecordingBlockMap::iterator itCommonBlocks = mapCommon.begin();
    while (itCommonBlocks != mapCommon.end())
    {
        RecordingBlockList::iterator itBlock = itCommonBlocks->second->List.begin();
        while (itBlock != itCommonBlocks->second->List.end())
        {
            RecordingBlock *pBlockCommon = (RecordingBlock *)(*itBlock);
            switch (pBlockCommon->enmType)
            {
#ifdef VBOX_WITH_AUDIO_RECORDING
                case RECORDINGBLOCKTYPE_AUDIO:
                {
                    PRECORDINGAUDIOFRAME pAudioFrame = (PRECORDINGAUDIOFRAME)pBlockCommon->pvData;

                    RECORDINGFRAME Frame;
                    Frame.msTimestamp = pBlockCommon->msTimestamp;
                    Frame.Audio.pvBuf = pAudioFrame->pvBuf;
                    Frame.Audio.cbBuf = pAudioFrame->cbBuf;

                    vrc = recordingCodecEncode(&m_CodecAudio, &Frame, NULL, NULL);
                    break;
                }
#endif /* VBOX_WITH_AUDIO_RECORDING */
                default:
                    /* Skip unknown stuff. */
                    break;
            }

            itCommonBlocks->second->List.erase(itBlock);
            delete pBlockCommon;
            itBlock = itCommonBlocks->second->List.begin();

            if (RT_FAILURE(vrc) || RTTimeMilliTS() > msStart + msTimeout)
                break;
        }

        /* If no entries are left over in the block map, remove it altogether. */
        if (itCommonBlocks->second->List.empty())
        {
            delete itCommonBlocks->second;
            mapCommon.erase(itCommonBlocks);
            itCommonBlocks = mapCommon.begin();
        }
        else
            ++itCommonBlocks;

        if (RT_FAILURE(vrc))
            break;
    }

    return vrc;
}

/**
 * Writes common block data (i.e. shared / the same) in all streams.
 *
 * The multiplexing is needed to supply all recorded (enabled) screens with the same
 * data at the same given point in time.
 *
 * Currently this only is being used for audio data.
 *
 * @returns VBox status code.
 * @param   mapCommon       Common block map to write data to.
 * @param   pCodec          Pointer to codec instance which has written the data.
 * @param   pvData          Pointer to written data (encoded).
 * @param   cbData          Size (in bytes) of \a pvData.
 * @param   msTimestamp     Absolute PTS (in ms) of the written data.
 * @param   uFlags          Encoding flags of type RECORDINGCODEC_ENC_F_XXX.
 */
int RecordingContext::writeCommonData(RecordingBlockMap &mapCommon, PRECORDINGCODEC pCodec, const void *pvData, size_t cbData,
                                      uint64_t msTimestamp, uint32_t uFlags)
{
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData, VERR_INVALID_PARAMETER);

    LogFlowFunc(("pCodec=%p, cbData=%zu, msTimestamp=%zu, uFlags=%#x\n",
                 pCodec, cbData, msTimestamp, uFlags));

    /** @todo Optimize this! Three allocations in here! */

    RECORDINGBLOCKTYPE const enmType = pCodec->Parms.enmType == RECORDINGCODECTYPE_AUDIO
                                     ? RECORDINGBLOCKTYPE_AUDIO : RECORDINGBLOCKTYPE_UNKNOWN;

    AssertReturn(enmType != RECORDINGBLOCKTYPE_UNKNOWN, VERR_NOT_SUPPORTED);

    RecordingBlock *pBlock = new RecordingBlock();

    switch (enmType)
    {
        case RECORDINGBLOCKTYPE_AUDIO:
        {
            PRECORDINGAUDIOFRAME pFrame = (PRECORDINGAUDIOFRAME)RTMemAlloc(sizeof(RECORDINGAUDIOFRAME));
            AssertPtrReturn(pFrame, VERR_NO_MEMORY);

            pFrame->pvBuf = (uint8_t *)RTMemAlloc(cbData);
            AssertPtrReturn(pFrame->pvBuf, VERR_NO_MEMORY);
            pFrame->cbBuf = cbData;

            memcpy(pFrame->pvBuf, pvData, cbData);

            pBlock->enmType     = enmType;
            pBlock->pvData      = pFrame;
            pBlock->cbData      = sizeof(RECORDINGAUDIOFRAME) + cbData;
            pBlock->cRefs       = m_cStreamsEnabled;
            pBlock->msTimestamp = msTimestamp;
            pBlock->uFlags      = uFlags;

            break;
        }

        default:
            AssertFailed();
            break;
    }

    lock();

    int vrc;

    try
    {
        RecordingBlockMap::iterator itBlocks = mapCommon.find(msTimestamp);
        if (itBlocks == mapCommon.end())
        {
            RecordingBlocks *pRecordingBlocks = new RecordingBlocks();
            pRecordingBlocks->List.push_back(pBlock);

            mapCommon.insert(std::make_pair(msTimestamp, pRecordingBlocks));
        }
        else
            itBlocks->second->List.push_back(pBlock);

        vrc = VINF_SUCCESS;
    }
    catch (const std::exception &ex)
    {
        RT_NOREF(ex);
        vrc = VERR_NO_MEMORY;
    }

    unlock();

    if (RT_SUCCESS(vrc))
        vrc = threadNotify();

    return vrc;
}

#ifdef VBOX_WITH_AUDIO_RECORDING
/**
 * Callback function for writing encoded audio data into the common encoded block map.
 *
 * This is called by the audio codec when finishing encoding audio data.
 *
 * @copydoc RECORDINGCODECCALLBACKS::pfnWriteData
 */
/* static */
DECLCALLBACK(int) RecordingContext::audioCodecWriteDataCallback(PRECORDINGCODEC pCodec, const void *pvData, size_t cbData,
                                                                uint64_t msAbsPTS, uint32_t uFlags, void *pvUser)
{
    RecordingContext *pThis = (RecordingContext *)pvUser;
    return pThis->writeCommonData(pThis->m_mapBlocksEncoded, pCodec, pvData, cbData, msAbsPTS, uFlags);
}

/**
 * Initializes the audio codec for a (multiplexing) recording context.
 *
 * @returns VBox status code.
 * @param   screenSettings      Reference to recording screen settings to use for initialization.
 */
int RecordingContext::audioInit(const settings::RecordingScreenSettings &screenSettings)
{
    RecordingAudioCodec_T const enmCodec = screenSettings.Audio.enmCodec;

    if (enmCodec == RecordingAudioCodec_None)
    {
        LogRel2(("Recording: No audio codec configured, skipping audio init\n"));
        return VINF_SUCCESS;
    }

    RECORDINGCODECCALLBACKS Callbacks;
    Callbacks.pvUser       = this;
    Callbacks.pfnWriteData = RecordingContext::audioCodecWriteDataCallback;

    int vrc = recordingCodecCreateAudio(&m_CodecAudio, enmCodec);
    if (RT_SUCCESS(vrc))
        vrc = recordingCodecInit(&m_CodecAudio, &Callbacks, screenSettings);

    return vrc;
}
#endif /* VBOX_WITH_AUDIO_RECORDING */

/**
 * Creates a recording context.
 *
 * @returns VBox status code.
 * @param   ptrConsole          Pointer to console object this context is bound to (weak pointer).
 * @param   Settings            Reference to recording settings to use for creation.
 */
int RecordingContext::createInternal(Console *ptrConsole, const settings::RecordingSettings &Settings)
{
    int vrc = VINF_SUCCESS;

    /* Copy the settings to our context. */
    m_Settings = Settings;

#ifdef VBOX_WITH_AUDIO_RECORDING
    settings::RecordingScreenSettingsMap::const_iterator itScreen0 = m_Settings.mapScreens.begin();
    AssertReturn(itScreen0 != m_Settings.mapScreens.end(), VERR_WRONG_ORDER);

    /* We always use the audio settings from screen 0, as we multiplex the audio data anyway. */
    settings::RecordingScreenSettings const &screen0Settings = itScreen0->second;

    vrc = this->audioInit(screen0Settings);
    if (RT_FAILURE(vrc))
        return vrc;
#endif

    m_pConsole = ptrConsole;

    settings::RecordingScreenSettingsMap::const_iterator itScreen = m_Settings.mapScreens.begin();
    while (itScreen != m_Settings.mapScreens.end())
    {
        RecordingStream *pStream = NULL;
        try
        {
            pStream = new RecordingStream(this, itScreen->first /* Screen ID */, itScreen->second);
            m_vecStreams.push_back(pStream);
            if (itScreen->second.fEnabled)
                m_cStreamsEnabled++;
            LogFlowFunc(("pStream=%p\n", pStream));
        }
        catch (std::bad_alloc &)
        {
            vrc = VERR_NO_MEMORY;
            break;
        }
        catch (int vrc_thrown) /* Catch vrc thrown by constructor. */
        {
            vrc = vrc_thrown;
            break;
        }

        ++itScreen;
    }

    if (RT_SUCCESS(vrc))
    {
        m_tsStartMs = RTTimeMilliTS();
        m_enmState  = RECORDINGSTS_CREATED;
        m_fShutdown = false;

        vrc = RTSemEventCreate(&m_WaitEvent);
        AssertRCReturn(vrc, vrc);
    }

    if (RT_FAILURE(vrc))
        destroyInternal();

    return vrc;
}

/**
 * Starts a recording context by creating its worker thread.
 *
 * @returns VBox status code.
 */
int RecordingContext::startInternal(void)
{
    if (m_enmState == RECORDINGSTS_STARTED)
        return VINF_SUCCESS;

    Assert(m_enmState == RECORDINGSTS_CREATED);

    int vrc = RTThreadCreate(&m_Thread, RecordingContext::threadMain, (void *)this, 0,
                             RTTHREADTYPE_MAIN_WORKER, RTTHREADFLAGS_WAITABLE, "Record");

    if (RT_SUCCESS(vrc)) /* Wait for the thread to start. */
        vrc = RTThreadUserWait(m_Thread, RT_MS_30SEC /* 30s timeout */);

    if (RT_SUCCESS(vrc))
    {
        LogRel(("Recording: Started\n"));
        m_enmState = RECORDINGSTS_STARTED;
    }
    else
        Log(("Recording: Failed to start (%Rrc)\n", vrc));

    return vrc;
}

/**
 * Stops a recording context by telling the worker thread to stop and finalizing its operation.
 *
 * @returns VBox status code.
 */
int RecordingContext::stopInternal(void)
{
    if (m_enmState != RECORDINGSTS_STARTED)
        return VINF_SUCCESS;

    LogThisFunc(("Shutting down thread ...\n"));

    /* Set shutdown indicator. */
    ASMAtomicWriteBool(&m_fShutdown, true);

    /* Signal the thread and wait for it to shut down. */
    int vrc = threadNotify();
    if (RT_SUCCESS(vrc))
        vrc = RTThreadWait(m_Thread, RT_MS_30SEC /* 30s timeout */, NULL);

    lock();

    if (RT_SUCCESS(vrc))
    {
        LogRel(("Recording: Stopped\n"));
        m_enmState = RECORDINGSTS_CREATED;
    }
    else
        Log(("Recording: Failed to stop (%Rrc)\n", vrc));

    unlock();

    LogFlowThisFunc(("%Rrc\n", vrc));
    return vrc;
}

/**
 * Destroys a recording context, internal version.
 */
void RecordingContext::destroyInternal(void)
{
    lock();

    if (m_enmState == RECORDINGSTS_UNINITIALIZED)
    {
        unlock();
        return;
    }

    int vrc = stopInternal();
    AssertRCReturnVoid(vrc);

    vrc = RTSemEventDestroy(m_WaitEvent);
    AssertRCReturnVoid(vrc);

    m_WaitEvent = NIL_RTSEMEVENT;

    RecordingStreams::iterator it = m_vecStreams.begin();
    while (it != m_vecStreams.end())
    {
        RecordingStream *pStream = (*it);

        vrc = pStream->Uninit();
        AssertRC(vrc);

        delete pStream;
        pStream = NULL;

        m_vecStreams.erase(it);
        it = m_vecStreams.begin();
    }

    /* Sanity. */
    Assert(m_vecStreams.empty());
    Assert(m_mapBlocksRaw.size() == 0);
    Assert(m_mapBlocksEncoded.size() == 0);

    m_enmState = RECORDINGSTS_UNINITIALIZED;

    unlock();
}

/**
 * Returns a recording context's current settings.
 *
 * @returns The recording context's current settings.
 */
const settings::RecordingSettings &RecordingContext::GetConfig(void) const
{
    return m_Settings;
}

/**
 * Returns the recording stream for a specific screen.
 *
 * @returns Recording stream for a specific screen, or NULL if not found.
 * @param   uScreen             Screen ID to retrieve recording stream for.
 */
RecordingStream *RecordingContext::getStreamInternal(unsigned uScreen) const
{
    RecordingStream *pStream;

    try
    {
        pStream = m_vecStreams.at(uScreen);
    }
    catch (std::out_of_range &)
    {
        pStream = NULL;
    }

    return pStream;
}

/**
 * Locks the recording context for serializing access.
 *
 * @returns VBox status code.
 */
int RecordingContext::lock(void)
{
    int vrc = RTCritSectEnter(&m_CritSect);
    AssertRC(vrc);
    return vrc;
}

/**
 * Unlocks the recording context for serializing access.
 *
 * @returns VBox status code.
 */
int RecordingContext::unlock(void)
{
    int vrc = RTCritSectLeave(&m_CritSect);
    AssertRC(vrc);
    return vrc;
}

/**
 * Retrieves a specific recording stream of a recording context.
 *
 * @returns Pointer to recording stream if found, or NULL if not found.
 * @param   uScreen             Screen number of recording stream to look up.
 */
RecordingStream *RecordingContext::GetStream(unsigned uScreen) const
{
    return getStreamInternal(uScreen);
}

/**
 * Returns the number of configured recording streams for a recording context.
 *
 * @returns Number of configured recording streams.
 */
size_t RecordingContext::GetStreamCount(void) const
{
    return m_vecStreams.size();
}

/**
 * Creates a new recording context.
 *
 * @returns VBox status code.
 * @param   ptrConsole          Pointer to console object this context is bound to (weak pointer).
 * @param   Settings            Reference to recording settings to use for creation.
 */
int RecordingContext::Create(Console *ptrConsole, const settings::RecordingSettings &Settings)
{
    return createInternal(ptrConsole, Settings);
}

/**
 * Destroys a recording context.
 */
void RecordingContext::Destroy(void)
{
    destroyInternal();
}

/**
 * Starts a recording context.
 *
 * @returns VBox status code.
 */
int RecordingContext::Start(void)
{
    return startInternal();
}

/**
 * Stops a recording context.
 */
int RecordingContext::Stop(void)
{
    return stopInternal();
}

/**
 * Returns if a specific recoding feature is enabled for at least one of the attached
 * recording streams or not.
 *
 * @returns @c true if at least one recording stream has this feature enabled, or @c false if
 *          no recording stream has this feature enabled.
 * @param   enmFeature          Recording feature to check for.
 */
bool RecordingContext::IsFeatureEnabled(RecordingFeature_T enmFeature)
{
    lock();

    RecordingStreams::const_iterator itStream = m_vecStreams.begin();
    while (itStream != m_vecStreams.end())
    {
        if ((*itStream)->GetConfig().isFeatureEnabled(enmFeature))
        {
            unlock();
            return true;
        }
        ++itStream;
    }

    unlock();

    return false;
}

/**
 * Returns if this recording context is ready to start recording.
 *
 * @returns @c true if recording context is ready, @c false if not.
 */
bool RecordingContext::IsReady(void)
{
    lock();

    const bool fIsReady = m_enmState >= RECORDINGSTS_CREATED;

    unlock();

    return fIsReady;
}

/**
 * Returns if this recording context is ready to accept new recording data for a given screen.
 *
 * @returns @c true if the specified screen is ready, @c false if not.
 * @param   uScreen             Screen ID.
 * @param   msTimestamp         Timestamp (PTS, in ms). Currently not being used.
 */
bool RecordingContext::IsReady(uint32_t uScreen, uint64_t msTimestamp)
{
    RT_NOREF(msTimestamp);

    lock();

    bool fIsReady = false;

    if (m_enmState != RECORDINGSTS_STARTED)
    {
        const RecordingStream *pStream = getStreamInternal(uScreen);
        if (pStream)
            fIsReady = pStream->IsReady();

        /* Note: Do not check for other constraints like the video FPS rate here,
         *       as this check then also would affect other (non-FPS related) stuff
         *       like audio data. */
    }

    unlock();

    return fIsReady;
}

/**
 * Returns whether a given recording context has been started or not.
 *
 * @returns true if active, false if not.
 */
bool RecordingContext::IsStarted(void)
{
    lock();

    const bool fIsStarted = m_enmState == RECORDINGSTS_STARTED;

    unlock();

    return fIsStarted;
}

/**
 * Checks if a specified limit for recording has been reached.
 *
 * @returns true if any limit has been reached.
 */
bool RecordingContext::IsLimitReached(void)
{
    lock();

    LogFlowThisFunc(("cStreamsEnabled=%RU16\n", m_cStreamsEnabled));

    const bool fLimitReached = m_cStreamsEnabled == 0;

    unlock();

    return fLimitReached;
}

/**
 * Checks if a specified limit for recording has been reached.
 *
 * @returns true if any limit has been reached.
 * @param   uScreen             Screen ID.
 * @param   msTimestamp         Timestamp (PTS, in ms) to check for.
 */
bool RecordingContext::IsLimitReached(uint32_t uScreen, uint64_t msTimestamp)
{
    lock();

    bool fLimitReached = false;

    const RecordingStream *pStream = getStreamInternal(uScreen);
    if (   !pStream
        || pStream->IsLimitReached(msTimestamp))
    {
        fLimitReached = true;
    }

    unlock();

    return fLimitReached;
}

/**
 * Returns if a specific screen needs to be fed with an update or not.
 *
 * @returns @c true if an update is needed, @c false if not.
 * @param   uScreen             Screen ID to retrieve update stats for.
 * @param   msTimestamp         Timestamp (PTS, in ms).
 */
bool RecordingContext::NeedsUpdate( uint32_t uScreen, uint64_t msTimestamp)
{
    lock();

    bool fNeedsUpdate = false;

    if (m_enmState == RECORDINGSTS_STARTED)
    {
#ifdef VBOX_WITH_AUDIO_RECORDING
        if (   recordingCodecIsInitialized(&m_CodecAudio)
            && recordingCodecGetWritable(&m_CodecAudio, msTimestamp) > 0)
        {
            fNeedsUpdate = true;
        }
#endif /* VBOX_WITH_AUDIO_RECORDING */

        if (!fNeedsUpdate)
        {
            const RecordingStream *pStream = getStreamInternal(uScreen);
            if (pStream)
                fNeedsUpdate = pStream->NeedsUpdate(msTimestamp);
        }
    }

    unlock();

    return fNeedsUpdate;
}

DECLCALLBACK(int) RecordingContext::OnLimitReached(uint32_t uScreen, int vrc)
{
    RT_NOREF(uScreen, vrc);
    LogFlowThisFunc(("Stream %RU32 has reached its limit (%Rrc)\n", uScreen, vrc));

    lock();

    Assert(m_cStreamsEnabled);
    m_cStreamsEnabled--;

    LogFlowThisFunc(("cStreamsEnabled=%RU16\n", m_cStreamsEnabled));

    unlock();

    return VINF_SUCCESS;
}

/**
 * Sends an audio frame to the recording thread.
 *
 * @returns VBox status code.
 * @param   pvData              Audio frame data to send.
 * @param   cbData              Size (in bytes) of (encoded) audio frame data.
 * @param   msTimestamp         Timestamp (PTS, in ms) of audio playback.
 */
int RecordingContext::SendAudioFrame(const void *pvData, size_t cbData, uint64_t msTimestamp)
{
#ifdef VBOX_WITH_AUDIO_RECORDING
    return writeCommonData(m_mapBlocksRaw, &m_CodecAudio,
                           pvData, cbData, msTimestamp, RECORDINGCODEC_ENC_F_BLOCK_IS_KEY);
#else
    RT_NOREF(pvData, cbData, msTimestamp);
    return VERR_NOT_SUPPORTED;
#endif
}

/**
 * Sends a video frame to the recording thread.
 *
 * @thread  EMT
 *
 * @returns VBox status code.
 * @param   uScreen            Screen number to send video frame to.
 * @param   x                  Starting x coordinate of the video frame.
 * @param   y                  Starting y coordinate of the video frame.
 * @param   uPixelFormat       Pixel format.
 * @param   uBPP               Bits Per Pixel (BPP).
 * @param   uBytesPerLine      Bytes per scanline.
 * @param   uSrcWidth          Width of the video frame.
 * @param   uSrcHeight         Height of the video frame.
 * @param   puSrcData          Pointer to video frame data.
 * @param   msTimestamp        Timestamp (PTS, in ms).
 */
int RecordingContext::SendVideoFrame(uint32_t uScreen, uint32_t x, uint32_t y,
                                     uint32_t uPixelFormat, uint32_t uBPP, uint32_t uBytesPerLine,
                                     uint32_t uSrcWidth, uint32_t uSrcHeight, uint8_t *puSrcData,
                                     uint64_t msTimestamp)
{
    AssertReturn(uSrcWidth,  VERR_INVALID_PARAMETER);
    AssertReturn(uSrcHeight, VERR_INVALID_PARAMETER);
    AssertReturn(puSrcData,  VERR_INVALID_POINTER);

    lock();

    RecordingStream *pStream = getStreamInternal(uScreen);
    if (!pStream)
    {
        unlock();

        AssertFailed();
        return VERR_NOT_FOUND;
    }

    int vrc = pStream->SendVideoFrame(x, y, uPixelFormat, uBPP, uBytesPerLine, uSrcWidth, uSrcHeight, puSrcData, msTimestamp);

    unlock();

    if (   RT_SUCCESS(vrc)
        && vrc != VINF_RECORDING_THROTTLED) /* Only signal the thread if operation was successful. */
    {
        threadNotify();
    }

    return vrc;
}

