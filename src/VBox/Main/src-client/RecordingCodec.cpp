/* $Id: RecordingCodec.cpp $ */
/** @file
 * Recording codec wrapper.
 */

/*
 * Copyright (C) 2022-2023 Oracle and/or its affiliates.
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

/* This code makes use of Vorbis (libvorbis):
 *
 * Copyright (c) 2002-2020 Xiph.org Foundation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_GROUP LOG_GROUP_RECORDING
#include "LoggingNew.h"

#include <VBox/com/string.h>
#include <VBox/err.h>
#include <VBox/vmm/pdmaudioifs.h>
#include <VBox/vmm/pdmaudioinline.h>

#include "RecordingInternals.h"
#include "RecordingUtils.h"
#include "WebMWriter.h"

#include <math.h>


/*********************************************************************************************************************************
*   VPX (VP8 / VP9) codec                                                                                                        *
*********************************************************************************************************************************/

#ifdef VBOX_WITH_LIBVPX
/** @copydoc RECORDINGCODECOPS::pfnInit */
static DECLCALLBACK(int) recordingCodecVPXInit(PRECORDINGCODEC pCodec)
{
    pCodec->cbScratch = _4K;
    pCodec->pvScratch = RTMemAlloc(pCodec->cbScratch);
    AssertPtrReturn(pCodec->pvScratch, VERR_NO_MEMORY);

    pCodec->Parms.csFrame  = 0;
    pCodec->Parms.cbFrame  = pCodec->Parms.Video.uWidth * pCodec->Parms.Video.uHeight * 4 /* 32-bit */;
    pCodec->Parms.msFrame  = 1; /* 1ms per frame. */

# ifdef VBOX_WITH_LIBVPX_VP9
    vpx_codec_iface_t *pCodecIface = vpx_codec_vp9_cx();
# else /* Default is using VP8. */
    vpx_codec_iface_t *pCodecIface = vpx_codec_vp8_cx();
# endif
    PRECORDINGCODECVPX pVPX = &pCodec->Video.VPX;

    vpx_codec_err_t rcv = vpx_codec_enc_config_default(pCodecIface, &pVPX->Cfg, 0 /* Reserved */);
    if (rcv != VPX_CODEC_OK)
    {
        LogRel(("Recording: Failed to get default config for VPX encoder: %s\n", vpx_codec_err_to_string(rcv)));
        return VERR_RECORDING_CODEC_INIT_FAILED;
    }

    /* Target bitrate in kilobits per second. */
    pVPX->Cfg.rc_target_bitrate = pCodec->Parms.uBitrate;
    /* Frame width. */
    pVPX->Cfg.g_w = pCodec->Parms.Video.uWidth;
    /* Frame height. */
    pVPX->Cfg.g_h = pCodec->Parms.Video.uHeight;
    /* ms per frame. */
    pVPX->Cfg.g_timebase.num = pCodec->Parms.msFrame;
    pVPX->Cfg.g_timebase.den = 1000;
    /* Disable multithreading. */
    pVPX->Cfg.g_threads      = 0;

    /* Initialize codec. */
    rcv = vpx_codec_enc_init(&pVPX->Ctx, pCodecIface, &pVPX->Cfg, 0 /* Flags */);
    if (rcv != VPX_CODEC_OK)
    {
        LogRel(("Recording: Failed to initialize VPX encoder: %s\n", vpx_codec_err_to_string(rcv)));
        return VERR_RECORDING_CODEC_INIT_FAILED;
    }

    if (!vpx_img_alloc(&pVPX->RawImage, VPX_IMG_FMT_I420,
                       pCodec->Parms.Video.uWidth, pCodec->Parms.Video.uHeight, 1))
    {
        LogRel(("Recording: Failed to allocate image %RU32x%RU32\n", pCodec->Parms.Video.uWidth, pCodec->Parms.Video.uHeight));
        return VERR_RECORDING_CODEC_INIT_FAILED;
    }

    /* Save a pointer to the first raw YUV plane. */
    pVPX->pu8YuvBuf = pVPX->RawImage.planes[0];

    return VINF_SUCCESS;
}

/** @copydoc RECORDINGCODECOPS::pfnDestroy */
static DECLCALLBACK(int) recordingCodecVPXDestroy(PRECORDINGCODEC pCodec)
{
    PRECORDINGCODECVPX pVPX = &pCodec->Video.VPX;

    vpx_img_free(&pVPX->RawImage);
    pVPX->pu8YuvBuf = NULL; /* Was pointing to VPX.RawImage. */

    vpx_codec_err_t rcv = vpx_codec_destroy(&pVPX->Ctx);
    Assert(rcv == VPX_CODEC_OK); RT_NOREF(rcv);

    return VINF_SUCCESS;
}

/** @copydoc RECORDINGCODECOPS::pfnParseOptions */
static DECLCALLBACK(int) recordingCodecVPXParseOptions(PRECORDINGCODEC pCodec, const com::Utf8Str &strOptions)
{
    size_t pos = 0;
    com::Utf8Str key, value;
    while ((pos = strOptions.parseKeyValue(key, value, pos)) != com::Utf8Str::npos)
    {
        if (key.compare("vc_quality", com::Utf8Str::CaseInsensitive) == 0)
        {
            const PRECORDINGCODECVPX pVPX = &pCodec->Video.VPX;

            if (value.compare("realtime", com::Utf8Str::CaseInsensitive) == 0)
                pVPX->uEncoderDeadline = VPX_DL_REALTIME;
            else if (value.compare("good", com::Utf8Str::CaseInsensitive) == 0)
            {
                AssertStmt(pCodec->Parms.Video.uFPS, pCodec->Parms.Video.uFPS = 25);
                pVPX->uEncoderDeadline = 1000000 / pCodec->Parms.Video.uFPS;
            }
            else if (value.compare("best", com::Utf8Str::CaseInsensitive) == 0)
                pVPX->uEncoderDeadline = VPX_DL_BEST_QUALITY;
            else
                pVPX->uEncoderDeadline = value.toUInt32();
        }
        else
            LogRel2(("Recording: Unknown option '%s' (value '%s'), skipping\n", key.c_str(), value.c_str()));
    } /* while */

    return VINF_SUCCESS;
}

/** @copydoc RECORDINGCODECOPS::pfnEncode */
static DECLCALLBACK(int) recordingCodecVPXEncode(PRECORDINGCODEC pCodec, PRECORDINGFRAME pFrame,
                                                 size_t *pcEncoded, size_t *pcbEncoded)
{
    RT_NOREF(pcEncoded, pcbEncoded);

    AssertPtrReturn(pFrame, VERR_INVALID_POINTER);

    PRECORDINGVIDEOFRAME pVideoFrame = pFrame->VideoPtr;

    int vrc = RecordingUtilsRGBToYUV(pVideoFrame->enmPixelFmt,
                                     /* Destination */
                                     pCodec->Video.VPX.pu8YuvBuf, pVideoFrame->uWidth, pVideoFrame->uHeight,
                                     /* Source */
                                     pVideoFrame->pu8RGBBuf, pCodec->Parms.Video.uWidth, pCodec->Parms.Video.uHeight);

    PRECORDINGCODECVPX pVPX = &pCodec->Video.VPX;

    /* Presentation TimeStamp (PTS). */
    vpx_codec_pts_t pts = pFrame->msTimestamp;
    vpx_codec_err_t rcv = vpx_codec_encode(&pVPX->Ctx,
                                           &pVPX->RawImage,
                                           pts                          /* Timestamp */,
                                           pCodec->Parms.Video.uDelayMs /* How long to show this frame */,
                                           0                            /* Flags */,
                                           pVPX->uEncoderDeadline       /* Quality setting */);
    if (rcv != VPX_CODEC_OK)
    {
        if (pCodec->State.cEncErrors++ < 64) /** @todo Make this configurable. */
            LogRel(("Recording: Failed to encode video frame: %s\n", vpx_codec_err_to_string(rcv)));
        return VERR_RECORDING_ENCODING_FAILED;
    }

    pCodec->State.cEncErrors = 0;

    vpx_codec_iter_t iter = NULL;
    vrc = VERR_NO_DATA;
    for (;;)
    {
        const vpx_codec_cx_pkt_t *pPkt = vpx_codec_get_cx_data(&pVPX->Ctx, &iter);
        if (!pPkt)
            break;

        switch (pPkt->kind)
        {
            case VPX_CODEC_CX_FRAME_PKT:
            {
                /* Calculate the absolute PTS of this frame (in ms). */
                uint64_t tsAbsPTSMs =   pPkt->data.frame.pts * 1000
                                      * (uint64_t)pCodec->Video.VPX.Cfg.g_timebase.num / pCodec->Video.VPX.Cfg.g_timebase.den;

                const bool fKeyframe = RT_BOOL(pPkt->data.frame.flags & VPX_FRAME_IS_KEY);

                uint32_t fFlags = RECORDINGCODEC_ENC_F_NONE;
                if (fKeyframe)
                    fFlags |= RECORDINGCODEC_ENC_F_BLOCK_IS_KEY;
                if (pPkt->data.frame.flags & VPX_FRAME_IS_INVISIBLE)
                    fFlags |= RECORDINGCODEC_ENC_F_BLOCK_IS_INVISIBLE;

                vrc = pCodec->Callbacks.pfnWriteData(pCodec, pPkt->data.frame.buf, pPkt->data.frame.sz,
                                                     tsAbsPTSMs, fFlags, pCodec->Callbacks.pvUser);
                break;
            }

            default:
                AssertFailed();
                LogFunc(("Unexpected video packet type %ld\n", pPkt->kind));
                break;
        }
    }

    return vrc;
}
#endif /* VBOX_WITH_LIBVPX */


/*********************************************************************************************************************************
*   Ogg Vorbis codec                                                                                                             *
*********************************************************************************************************************************/

#ifdef VBOX_WITH_LIBVORBIS
/** @copydoc RECORDINGCODECOPS::pfnInit */
static DECLCALLBACK(int) recordingCodecVorbisInit(PRECORDINGCODEC pCodec)
{
    pCodec->cbScratch = _4K;
    pCodec->pvScratch = RTMemAlloc(pCodec->cbScratch);
    AssertPtrReturn(pCodec->pvScratch, VERR_NO_MEMORY);

    const PPDMAUDIOPCMPROPS pPCMProps = &pCodec->Parms.Audio.PCMProps;

    /** @todo BUGBUG When left out this call, vorbis_block_init() does not find oggpack_writeinit and all goes belly up ... */
    oggpack_buffer b;
    oggpack_writeinit(&b);

    vorbis_info_init(&pCodec->Audio.Vorbis.info);

    int vorbis_rc;
    if (pCodec->Parms.uBitrate == 0) /* No bitrate management? Then go for ABR (Average Bit Rate) only. */
        vorbis_rc = vorbis_encode_init_vbr(&pCodec->Audio.Vorbis.info,
                                           PDMAudioPropsChannels(pPCMProps), PDMAudioPropsHz(pPCMProps),
                                           (float).4 /* Quality, from -.1 (lowest) to 1 (highest) */);
    else
        vorbis_rc = vorbis_encode_setup_managed(&pCodec->Audio.Vorbis.info, PDMAudioPropsChannels(pPCMProps), PDMAudioPropsHz(pPCMProps),
                                                -1 /* max bitrate (unset) */, pCodec->Parms.uBitrate /* kbps, nominal */, -1 /* min bitrate (unset) */);
    if (vorbis_rc)
    {
        LogRel(("Recording: Audio codec failed to setup %s mode (bitrate %RU32): %d\n",
                pCodec->Parms.uBitrate == 0 ? "VBR" : "bitrate management", pCodec->Parms.uBitrate, vorbis_rc));
        return VERR_RECORDING_CODEC_INIT_FAILED;
    }

    vorbis_rc = vorbis_encode_setup_init(&pCodec->Audio.Vorbis.info);
    if (vorbis_rc)
    {
        LogRel(("Recording: vorbis_encode_setup_init() failed (%d)\n", vorbis_rc));
        return VERR_RECORDING_CODEC_INIT_FAILED;
    }

    /* Initialize the analysis state and encoding storage. */
    vorbis_rc = vorbis_analysis_init(&pCodec->Audio.Vorbis.dsp_state, &pCodec->Audio.Vorbis.info);
    if (vorbis_rc)
    {
        vorbis_info_clear(&pCodec->Audio.Vorbis.info);
        LogRel(("Recording: vorbis_analysis_init() failed (%d)\n", vorbis_rc));
        return VERR_RECORDING_CODEC_INIT_FAILED;
    }

    vorbis_rc = vorbis_block_init(&pCodec->Audio.Vorbis.dsp_state, &pCodec->Audio.Vorbis.block_cur);
    if (vorbis_rc)
    {
        vorbis_info_clear(&pCodec->Audio.Vorbis.info);
        LogRel(("Recording: vorbis_block_init() failed (%d)\n", vorbis_rc));
        return VERR_RECORDING_CODEC_INIT_FAILED;
    }

    if (!pCodec->Parms.msFrame) /* No ms per frame defined? Use default. */
        pCodec->Parms.msFrame = VBOX_RECORDING_VORBIS_FRAME_MS_DEFAULT;

    return VINF_SUCCESS;
}

/** @copydoc RECORDINGCODECOPS::pfnDestroy */
static DECLCALLBACK(int) recordingCodecVorbisDestroy(PRECORDINGCODEC pCodec)
{
    PRECORDINGCODECVORBIS pVorbis = &pCodec->Audio.Vorbis;

    vorbis_block_clear(&pVorbis->block_cur);
    vorbis_dsp_clear  (&pVorbis->dsp_state);
    vorbis_info_clear (&pVorbis->info);

    return VINF_SUCCESS;
}

/** @copydoc RECORDINGCODECOPS::pfnEncode */
static DECLCALLBACK(int) recordingCodecVorbisEncode(PRECORDINGCODEC pCodec,
                                                    const PRECORDINGFRAME pFrame, size_t *pcEncoded, size_t *pcbEncoded)
{
    const PPDMAUDIOPCMPROPS pPCMProps = &pCodec->Parms.Audio.PCMProps;

    Assert      (pCodec->Parms.cbFrame);
    AssertReturn(pFrame->Audio.cbBuf % pCodec->Parms.cbFrame == 0, VERR_INVALID_PARAMETER);
    Assert      (pFrame->Audio.cbBuf);
    AssertReturn(pFrame->Audio.cbBuf % PDMAudioPropsFrameSize(pPCMProps) == 0, VERR_INVALID_PARAMETER);
    AssertReturn(pCodec->cbScratch >= pFrame->Audio.cbBuf, VERR_INVALID_PARAMETER);

    int vrc = VINF_SUCCESS;

    int const cbFrame = PDMAudioPropsFrameSize(pPCMProps);
    int const cFrames = (int)(pFrame->Audio.cbBuf / cbFrame);

    /* Write non-interleaved frames. */
    float  **buffer = vorbis_analysis_buffer(&pCodec->Audio.Vorbis.dsp_state, cFrames);
    int16_t *puSrc  = (int16_t *)pFrame->Audio.pvBuf; RT_NOREF(puSrc);

    /* Convert samples into floating point. */
    /** @todo This is sloooooooooooow! Optimize this! */
    uint8_t const cChannels = PDMAudioPropsChannels(pPCMProps);
    AssertReturn(cChannels == 2, VERR_NOT_SUPPORTED);

    float const div = 1.0f / 32768.0f;

    for(int f = 0; f < cFrames; f++)
    {
        buffer[0][f] = (float)puSrc[0] * div;
        buffer[1][f] = (float)puSrc[1] * div;
        puSrc += cChannels;
    }

    int vorbis_rc = vorbis_analysis_wrote(&pCodec->Audio.Vorbis.dsp_state, cFrames);
    if (vorbis_rc)
    {
        LogRel(("Recording: vorbis_analysis_wrote() failed (%d)\n", vorbis_rc));
        return VERR_RECORDING_ENCODING_FAILED;
    }

    if (pcEncoded)
        *pcEncoded = 0;
    if (pcbEncoded)
        *pcbEncoded = 0;

    size_t cBlocksEncoded = 0;
    size_t cBytesEncoded  = 0;

    uint8_t *puDst = (uint8_t *)pCodec->pvScratch;

    while (vorbis_analysis_blockout(&pCodec->Audio.Vorbis.dsp_state, &pCodec->Audio.Vorbis.block_cur) == 1 /* More available? */)
    {
        vorbis_rc = vorbis_analysis(&pCodec->Audio.Vorbis.block_cur, NULL);
        if (vorbis_rc < 0)
        {
            LogRel(("Recording: vorbis_analysis() failed (%d)\n", vorbis_rc));
            vorbis_rc = 0; /* Reset */
            vrc = VERR_RECORDING_ENCODING_FAILED;
            break;
        }

        vorbis_rc = vorbis_bitrate_addblock(&pCodec->Audio.Vorbis.block_cur);
        if (vorbis_rc < 0)
        {
            LogRel(("Recording: vorbis_bitrate_addblock() failed (%d)\n", vorbis_rc));
            vorbis_rc = 0; /* Reset */
            vrc = VERR_RECORDING_ENCODING_FAILED;
            break;
        }

        /* Vorbis expects us to flush packets one at a time directly to the container.
         *
         * If we flush more than one packet in a row, players can't decode this then. */
        ogg_packet op;
        while ((vorbis_rc = vorbis_bitrate_flushpacket(&pCodec->Audio.Vorbis.dsp_state, &op)) > 0)
        {
            cBytesEncoded += op.bytes;
            AssertBreakStmt(cBytesEncoded <= pCodec->cbScratch, vrc = VERR_BUFFER_OVERFLOW);
            cBlocksEncoded++;

            vrc = pCodec->Callbacks.pfnWriteData(pCodec, op.packet, (size_t)op.bytes, pCodec->State.tsLastWrittenMs,
                                                 RECORDINGCODEC_ENC_F_BLOCK_IS_KEY /* Every Vorbis frame is a key frame */,
                                                 pCodec->Callbacks.pvUser);
        }

        RT_NOREF(puDst);

        /* Note: When vorbis_rc is 0, this marks the last packet, a negative values means error. */
        if (vorbis_rc < 0)
        {
            LogRel(("Recording: vorbis_bitrate_flushpacket() failed (%d)\n", vorbis_rc));
            vorbis_rc = 0; /* Reset */
            vrc = VERR_RECORDING_ENCODING_FAILED;
            break;
        }
    }

    if (vorbis_rc < 0)
    {
        LogRel(("Recording: vorbis_analysis_blockout() failed (%d)\n", vorbis_rc));
        return VERR_RECORDING_ENCODING_FAILED;
    }

    if (pcbEncoded)
        *pcbEncoded = 0;
    if (pcEncoded)
        *pcEncoded  = 0;

    if (RT_FAILURE(vrc))
        LogRel(("Recording: Encoding Vorbis audio data failed, vrc=%Rrc\n", vrc));

    Log3Func(("cbSrc=%zu, cbDst=%zu, cEncoded=%zu, cbEncoded=%zu, vrc=%Rrc\n",
              pFrame->Audio.cbBuf, pCodec->cbScratch, cBlocksEncoded, cBytesEncoded, vrc));

    return vrc;
}

static DECLCALLBACK(int) recordingCodecVorbisFinalize(PRECORDINGCODEC pCodec)
{
    int vorbis_rc = vorbis_analysis_wrote(&pCodec->Audio.Vorbis.dsp_state, 0 /* Means finalize */);
    if (vorbis_rc)
    {
        LogRel(("Recording: vorbis_analysis_wrote() failed for finalizing stream (%d)\n", vorbis_rc));
        return VERR_RECORDING_ENCODING_FAILED;
    }

    return VINF_SUCCESS;
}
#endif /* VBOX_WITH_LIBVORBIS */


/*********************************************************************************************************************************
*   Codec API                                                                                                                    *
*********************************************************************************************************************************/

/**
 * Initializes an audio codec.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec instance to initialize.
 * @param   pCallbacks          Codec callback table to use for the codec.
 * @param   Settings            Screen settings to use for initialization.
 */
static int recordingCodecInitAudio(const PRECORDINGCODEC pCodec,
                                   const PRECORDINGCODECCALLBACKS pCallbacks, const settings::RecordingScreenSettings &Settings)
{
    AssertReturn(pCodec->Parms.enmType == RECORDINGCODECTYPE_AUDIO, VERR_INVALID_PARAMETER);

    com::Utf8Str strCodec;
    settings::RecordingScreenSettings::audioCodecToString(pCodec->Parms.enmAudioCodec, strCodec);
    LogRel(("Recording: Initializing audio codec '%s'\n", strCodec.c_str()));

    const PPDMAUDIOPCMPROPS pPCMProps = &pCodec->Parms.Audio.PCMProps;

    PDMAudioPropsInit(pPCMProps,
                      Settings.Audio.cBits / 8,
                      true /* fSigned */, Settings.Audio.cChannels, Settings.Audio.uHz);
    pCodec->Parms.uBitrate = 0; /** @todo No bitrate management for audio yet. */

    if (pCallbacks)
        memcpy(&pCodec->Callbacks, pCallbacks, sizeof(RECORDINGCODECCALLBACKS));

    int vrc = VINF_SUCCESS;

    if (pCodec->Ops.pfnParseOptions)
        vrc = pCodec->Ops.pfnParseOptions(pCodec, Settings.strOptions);

    if (RT_SUCCESS(vrc))
        vrc = pCodec->Ops.pfnInit(pCodec);

    if (RT_SUCCESS(vrc))
    {
        Assert(PDMAudioPropsAreValid(pPCMProps));

        uint32_t uBitrate = pCodec->Parms.uBitrate; /* Bitrate management could have been changed by pfnInit(). */

        LogRel2(("Recording: Audio codec is initialized with %RU32Hz, %RU8 channel(s), %RU8 bits per sample\n",
                 PDMAudioPropsHz(pPCMProps), PDMAudioPropsChannels(pPCMProps), PDMAudioPropsSampleBits(pPCMProps)));
        LogRel2(("Recording: Audio codec's bitrate management is %s (%RU32 kbps)\n", uBitrate ? "enabled" : "disabled", uBitrate));

        if (!pCodec->Parms.msFrame || pCodec->Parms.msFrame >= RT_MS_1SEC) /* Not set yet by codec stuff above? */
            pCodec->Parms.msFrame = 20; /* 20ms by default should be a sensible value; to prevent division by zero. */

        pCodec->Parms.csFrame  = PDMAudioPropsHz(pPCMProps) / (RT_MS_1SEC / pCodec->Parms.msFrame);
        pCodec->Parms.cbFrame  = PDMAudioPropsFramesToBytes(pPCMProps, pCodec->Parms.csFrame);

        LogFlowFunc(("cbSample=%RU32, msFrame=%RU32 -> csFrame=%RU32, cbFrame=%RU32, uBitrate=%RU32\n",
                     PDMAudioPropsSampleSize(pPCMProps), pCodec->Parms.msFrame, pCodec->Parms.csFrame, pCodec->Parms.cbFrame, pCodec->Parms.uBitrate));
    }
    else
        LogRel(("Recording: Error initializing audio codec (%Rrc)\n", vrc));

    return vrc;
}

/**
 * Initializes a video codec.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec instance to initialize.
 * @param   pCallbacks          Codec callback table to use for the codec.
 * @param   Settings            Screen settings to use for initialization.
 */
static int recordingCodecInitVideo(const PRECORDINGCODEC pCodec,
                                   const PRECORDINGCODECCALLBACKS pCallbacks, const settings::RecordingScreenSettings &Settings)
{
    com::Utf8Str strTemp;
    settings::RecordingScreenSettings::videoCodecToString(pCodec->Parms.enmVideoCodec, strTemp);
    LogRel(("Recording: Initializing video codec '%s'\n", strTemp.c_str()));

    pCodec->Parms.uBitrate       = Settings.Video.ulRate;
    pCodec->Parms.Video.uFPS     = Settings.Video.ulFPS;
    pCodec->Parms.Video.uWidth   = Settings.Video.ulWidth;
    pCodec->Parms.Video.uHeight  = Settings.Video.ulHeight;
    pCodec->Parms.Video.uDelayMs = RT_MS_1SEC / pCodec->Parms.Video.uFPS;

    if (pCallbacks)
        memcpy(&pCodec->Callbacks, pCallbacks, sizeof(RECORDINGCODECCALLBACKS));

    AssertReturn(pCodec->Parms.uBitrate, VERR_INVALID_PARAMETER);        /* Bitrate must be set. */
    AssertStmt(pCodec->Parms.Video.uFPS, pCodec->Parms.Video.uFPS = 25); /* Prevent division by zero. */

    AssertReturn(pCodec->Parms.Video.uHeight, VERR_INVALID_PARAMETER);
    AssertReturn(pCodec->Parms.Video.uWidth, VERR_INVALID_PARAMETER);
    AssertReturn(pCodec->Parms.Video.uDelayMs, VERR_INVALID_PARAMETER);

    int vrc = VINF_SUCCESS;

    if (pCodec->Ops.pfnParseOptions)
        vrc = pCodec->Ops.pfnParseOptions(pCodec, Settings.strOptions);

    if (   RT_SUCCESS(vrc)
        && pCodec->Ops.pfnInit)
        vrc = pCodec->Ops.pfnInit(pCodec);

    if (RT_SUCCESS(vrc))
    {
        pCodec->Parms.enmType       = RECORDINGCODECTYPE_VIDEO;
        pCodec->Parms.enmVideoCodec = RecordingVideoCodec_VP8; /** @todo No VP9 yet. */
    }
    else
        LogRel(("Recording: Error initializing video codec (%Rrc)\n", vrc));

    return vrc;
}

#ifdef VBOX_WITH_AUDIO_RECORDING
/**
 * Lets an audio codec parse advanced options given from a string.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec instance to parse options for.
 * @param   strOptions          Options string to parse.
 */
static DECLCALLBACK(int) recordingCodecAudioParseOptions(PRECORDINGCODEC pCodec, const com::Utf8Str &strOptions)
{
    AssertReturn(pCodec->Parms.enmType == RECORDINGCODECTYPE_AUDIO, VERR_INVALID_PARAMETER);

    size_t pos = 0;
    com::Utf8Str key, value;
    while ((pos = strOptions.parseKeyValue(key, value, pos)) != com::Utf8Str::npos)
    {
        if (key.compare("ac_profile", com::Utf8Str::CaseInsensitive) == 0)
        {
            if (value.compare("low", com::Utf8Str::CaseInsensitive) == 0)
            {
                PDMAudioPropsInit(&pCodec->Parms.Audio.PCMProps, 16, true /* fSigned */, 1 /* Channels */, 8000 /* Hz */);
            }
            else if (value.startsWith("med" /* "med[ium]" */, com::Utf8Str::CaseInsensitive) == 0)
            {
                /* Stay with the defaults. */
            }
            else if (value.compare("high", com::Utf8Str::CaseInsensitive) == 0)
            {
                PDMAudioPropsInit(&pCodec->Parms.Audio.PCMProps, 16, true /* fSigned */, 2 /* Channels */, 48000 /* Hz */);
            }
        }
        else
            LogRel(("Recording: Unknown option '%s' (value '%s'), skipping\n", key.c_str(), value.c_str()));

    } /* while */

    return VINF_SUCCESS;
}
#endif

static void recordingCodecReset(PRECORDINGCODEC pCodec)
{
    pCodec->State.tsLastWrittenMs = 0;

    pCodec->State.cEncErrors = 0;
#ifdef VBOX_WITH_STATISTICS
    pCodec->STAM.cEncBlocks  = 0;
    pCodec->STAM.msEncTotal  = 0;
#endif
}

/**
 * Common code for codec creation.
 *
 * @param   pCodec              Codec instance to create.
 */
static void recordingCodecCreateCommon(PRECORDINGCODEC pCodec)
{
    RT_ZERO(pCodec->Ops);
    RT_ZERO(pCodec->Callbacks);
}

/**
 * Creates an audio codec.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec instance to create.
 * @param   enmAudioCodec       Audio codec to create.
 */
int recordingCodecCreateAudio(PRECORDINGCODEC pCodec, RecordingAudioCodec_T enmAudioCodec)
{
    int vrc;

    recordingCodecCreateCommon(pCodec);

    switch (enmAudioCodec)
    {
# ifdef VBOX_WITH_LIBVORBIS
        case RecordingAudioCodec_OggVorbis:
        {
            pCodec->Ops.pfnInit         = recordingCodecVorbisInit;
            pCodec->Ops.pfnDestroy      = recordingCodecVorbisDestroy;
            pCodec->Ops.pfnParseOptions = recordingCodecAudioParseOptions;
            pCodec->Ops.pfnEncode       = recordingCodecVorbisEncode;
            pCodec->Ops.pfnFinalize     = recordingCodecVorbisFinalize;

            vrc = VINF_SUCCESS;
            break;
        }
# endif /* VBOX_WITH_LIBVORBIS */

        default:
            LogRel(("Recording: Selected codec is not supported!\n"));
            vrc = VERR_RECORDING_CODEC_NOT_SUPPORTED;
            break;
    }

    if (RT_SUCCESS(vrc))
    {
        pCodec->Parms.enmType       = RECORDINGCODECTYPE_AUDIO;
        pCodec->Parms.enmAudioCodec = enmAudioCodec;
    }

    return vrc;
}

/**
 * Creates a video codec.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec instance to create.
 * @param   enmVideoCodec       Video codec to create.
 */
int recordingCodecCreateVideo(PRECORDINGCODEC pCodec, RecordingVideoCodec_T enmVideoCodec)
{
    int vrc;

    recordingCodecCreateCommon(pCodec);

    switch (enmVideoCodec)
    {
# ifdef VBOX_WITH_LIBVPX
        case RecordingVideoCodec_VP8:
        {
            pCodec->Ops.pfnInit         = recordingCodecVPXInit;
            pCodec->Ops.pfnDestroy      = recordingCodecVPXDestroy;
            pCodec->Ops.pfnParseOptions = recordingCodecVPXParseOptions;
            pCodec->Ops.pfnEncode       = recordingCodecVPXEncode;

            vrc = VINF_SUCCESS;
            break;
        }
# endif /* VBOX_WITH_LIBVPX */

        default:
            vrc = VERR_RECORDING_CODEC_NOT_SUPPORTED;
            break;
    }

    if (RT_SUCCESS(vrc))
    {
        pCodec->Parms.enmType       = RECORDINGCODECTYPE_VIDEO;
        pCodec->Parms.enmVideoCodec = enmVideoCodec;
    }

    return vrc;
}

/**
 * Initializes a codec.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec to initialize.
 * @param   pCallbacks          Codec callback table to use. Optional and may be NULL.
 * @param   Settings            Settings to use for initializing the codec.
 */
int recordingCodecInit(const PRECORDINGCODEC pCodec, const PRECORDINGCODECCALLBACKS pCallbacks, const settings::RecordingScreenSettings &Settings)
{
    recordingCodecReset(pCodec);

    int vrc;
    if (pCodec->Parms.enmType == RECORDINGCODECTYPE_AUDIO)
        vrc = recordingCodecInitAudio(pCodec, pCallbacks, Settings);
    else if (pCodec->Parms.enmType == RECORDINGCODECTYPE_VIDEO)
        vrc = recordingCodecInitVideo(pCodec, pCallbacks, Settings);
    else
        AssertFailedStmt(vrc = VERR_NOT_SUPPORTED);

    return vrc;
}

/**
 * Destroys an audio codec.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec to destroy.
 */
static int recordingCodecDestroyAudio(PRECORDINGCODEC pCodec)
{
    AssertReturn(pCodec->Parms.enmType == RECORDINGCODECTYPE_AUDIO, VERR_INVALID_PARAMETER);

    return pCodec->Ops.pfnDestroy(pCodec);
}

/**
 * Destroys a video codec.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec to destroy.
 */
static int recordingCodecDestroyVideo(PRECORDINGCODEC pCodec)
{
    AssertReturn(pCodec->Parms.enmType == RECORDINGCODECTYPE_VIDEO, VERR_INVALID_PARAMETER);

    return pCodec->Ops.pfnDestroy(pCodec);
}

/**
 * Destroys the codec.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec to destroy.
 */
int recordingCodecDestroy(PRECORDINGCODEC pCodec)
{
    if (pCodec->Parms.enmType == RECORDINGCODECTYPE_INVALID)
        return VINF_SUCCESS;

    int vrc;

    if (pCodec->Parms.enmType == RECORDINGCODECTYPE_AUDIO)
        vrc = recordingCodecDestroyAudio(pCodec);
    else if (pCodec->Parms.enmType == RECORDINGCODECTYPE_VIDEO)
        vrc =recordingCodecDestroyVideo(pCodec);
    else
        AssertFailedReturn(VERR_NOT_SUPPORTED);

    if (RT_SUCCESS(vrc))
    {
        if (pCodec->pvScratch)
        {
            Assert(pCodec->cbScratch);
            RTMemFree(pCodec->pvScratch);
            pCodec->pvScratch = NULL;
            pCodec->cbScratch = 0;
        }

        pCodec->Parms.enmType       = RECORDINGCODECTYPE_INVALID;
        pCodec->Parms.enmVideoCodec = RecordingVideoCodec_None;
    }

    return vrc;
}

/**
 * Feeds the codec encoder with data to encode.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec to use.
 * @param   pFrame              Pointer to frame data to encode.
 * @param   pcEncoded           Where to return the number of encoded blocks in \a pvDst on success. Optional.
 * @param   pcbEncoded          Where to return the number of encoded bytes in \a pvDst on success. Optional.
 */
int recordingCodecEncode(PRECORDINGCODEC pCodec,
                         const PRECORDINGFRAME pFrame, size_t *pcEncoded, size_t *pcbEncoded)
{
    AssertPtrReturn(pCodec->Ops.pfnEncode, VERR_NOT_SUPPORTED);

    size_t cEncoded, cbEncoded;
    int vrc = pCodec->Ops.pfnEncode(pCodec, pFrame, &cEncoded, &cbEncoded);
    if (RT_SUCCESS(vrc))
    {
        pCodec->State.tsLastWrittenMs = pFrame->msTimestamp;

#ifdef VBOX_WITH_STATISTICS
        pCodec->STAM.cEncBlocks += cEncoded;
        pCodec->STAM.msEncTotal += pCodec->Parms.msFrame * cEncoded;
#endif
        if (pcEncoded)
            *pcEncoded = cEncoded;
        if (pcbEncoded)
            *pcbEncoded = cbEncoded;
    }

    return vrc;
}

/**
 * Tells the codec that has to finalize the stream.
 *
 * @returns VBox status code.
 * @param   pCodec              Codec to finalize stream for.
 */
int recordingCodecFinalize(PRECORDINGCODEC pCodec)
{
    if (pCodec->Ops.pfnFinalize)
        return pCodec->Ops.pfnFinalize(pCodec);
    return VINF_SUCCESS;
}

/**
 * Returns whether the codec has been initialized or not.
 *
 * @returns @c true if initialized, or @c false if not.
 * @param   pCodec              Codec to return initialization status for.
 */
bool recordingCodecIsInitialized(const PRECORDINGCODEC pCodec)
{
    return pCodec->Ops.pfnInit != NULL; /* pfnInit acts as a beacon for initialization status. */
}

/**
 * Returns the number of writable bytes for a given timestamp.
 *
 * This basically is a helper function to respect the set frames per second (FPS).
 *
 * @returns Number of writable bytes.
 * @param   pCodec              Codec to return number of writable bytes for.
 * @param   msTimestamp         Timestamp (PTS, in ms) return number of writable bytes for.
 */
uint32_t recordingCodecGetWritable(const PRECORDINGCODEC pCodec, uint64_t msTimestamp)
{
    Log3Func(("%RU64 -- tsLastWrittenMs=%RU64 + uDelayMs=%RU32\n",
              msTimestamp, pCodec->State.tsLastWrittenMs,pCodec->Parms.Video.uDelayMs));

    if (msTimestamp < pCodec->State.tsLastWrittenMs + pCodec->Parms.Video.uDelayMs)
        return 0; /* Too early for writing (respect set FPS). */

    /* For now we just return the complete frame space. */
    AssertMsg(pCodec->Parms.cbFrame, ("Codec not initialized yet\n"));
    return pCodec->Parms.cbFrame;
}
