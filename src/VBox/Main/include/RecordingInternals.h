/* $Id: RecordingInternals.h $ */
/** @file
 * Recording internals header.
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

#ifndef MAIN_INCLUDED_RecordingInternals_h
#define MAIN_INCLUDED_RecordingInternals_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <list>

#include <iprt/assert.h>
#include <iprt/types.h> /* drag in stdint.h before vpx does it. */

#include "VBox/com/string.h"
#include "VBox/com/VirtualBox.h"
#include "VBox/settings.h"
#include <VBox/vmm/pdmaudioifs.h>

#ifdef VBOX_WITH_LIBVPX
# define VPX_CODEC_DISABLE_COMPAT 1
# include "vpx/vp8cx.h"
# include "vpx/vpx_image.h"
# include "vpx/vpx_encoder.h"
#endif /* VBOX_WITH_LIBVPX */

#ifdef VBOX_WITH_LIBVORBIS
# include "vorbis/vorbisenc.h"
#endif


/*********************************************************************************************************************************
*   Defines                                                                                                                      *
*********************************************************************************************************************************/
#define VBOX_RECORDING_VORBIS_HZ_MAX             48000   /**< Maximum sample rate (in Hz) Vorbis can handle. */
#define VBOX_RECORDING_VORBIS_FRAME_MS_DEFAULT   20      /**< Default Vorbis frame size (in ms). */


/*********************************************************************************************************************************
*   Prototypes                                                                                                                   *
*********************************************************************************************************************************/
struct RECORDINGCODEC;
typedef RECORDINGCODEC *PRECORDINGCODEC;

struct RECORDINGFRAME;
typedef RECORDINGFRAME *PRECORDINGFRAME;


/*********************************************************************************************************************************
*   Internal structures, defines and APIs                                                                                        *
*********************************************************************************************************************************/

/**
 * Enumeration for specifying a (generic) codec type.
 */
typedef enum RECORDINGCODECTYPE
{
    /** Invalid codec type. Do not use. */
    RECORDINGCODECTYPE_INVALID = 0,
    /** Video codec. */
    RECORDINGCODECTYPE_VIDEO,
    /** Audio codec. */
    RECORDINGCODECTYPE_AUDIO
} RECORDINGCODECTYPE;

/**
 * Structure for keeping a codec operations table.
 */
typedef struct RECORDINGCODECOPS
{
    /**
     * Initializes a codec.
     *
     * @returns VBox status code.
     * @param   pCodec              Codec instance to initialize.
     */
    DECLCALLBACKMEMBER(int, pfnInit,         (PRECORDINGCODEC pCodec));

    /**
     * Destroys a codec.
     *
     * @returns VBox status code.
     * @param   pCodec              Codec instance to destroy.
     */
    DECLCALLBACKMEMBER(int, pfnDestroy,      (PRECORDINGCODEC pCodec));

    /**
     * Parses an options string to configure advanced / hidden / experimental features of a recording stream.
     * Unknown values will be skipped. Optional.
     *
     * @returns VBox status code.
     * @param   pCodec              Codec instance to parse options for.
     * @param   strOptions          Options string to parse.
     */
    DECLCALLBACKMEMBER(int, pfnParseOptions, (PRECORDINGCODEC pCodec, const com::Utf8Str &strOptions));

    /**
     * Feeds the codec encoder with data to encode.
     *
     * @returns VBox status code.
     * @param   pCodec              Codec instance to use.
     * @param   pFrame              Pointer to frame data to encode.
     * @param   pcEncoded           Where to return the number of encoded blocks in \a pvDst on success. Optional.
     * @param   pcbEncoded          Where to return the number of encoded bytes in \a pvDst on success. Optional.
     */
    DECLCALLBACKMEMBER(int, pfnEncode,       (PRECORDINGCODEC pCodec, const PRECORDINGFRAME pFrame, size_t *pcEncoded, size_t *pcbEncoded));

    /**
     * Tells the codec to finalize the current stream. Optional.
     *
     * @returns VBox status code.
     * @param   pCodec              Codec instance to finalize stream for.
     */
    DECLCALLBACKMEMBER(int, pfnFinalize,     (PRECORDINGCODEC pCodec));
} RECORDINGCODECOPS, *PRECORDINGCODECOPS;

/** No encoding flags set. */
#define RECORDINGCODEC_ENC_F_NONE               UINT32_C(0)
/** Data block is a key block. */
#define RECORDINGCODEC_ENC_F_BLOCK_IS_KEY       RT_BIT_32(0)
/** Data block is invisible. */
#define RECORDINGCODEC_ENC_F_BLOCK_IS_INVISIBLE RT_BIT_32(1)
/** Encoding flags valid mask. */
#define RECORDINGCODEC_ENC_F_VALID_MASK         0x1

/**
 * Structure for keeping a codec callback table.
 */
typedef struct RECORDINGCODECCALLBACKS
{
    /**
     * Callback for notifying that encoded data has been written.
     *
     * @returns VBox status code.
     * @param   pCodec          Pointer to codec instance which has written the data.
     * @param   pvData          Pointer to written data (encoded).
     * @param   cbData          Size (in bytes) of \a pvData.
     * @param   msAbsPTS        Absolute PTS (in ms) of the written data.
     * @param   uFlags          Encoding flags of type RECORDINGCODEC_ENC_F_XXX.
     * @param   pvUser          User-supplied pointer.
     */
    DECLCALLBACKMEMBER(int, pfnWriteData, (PRECORDINGCODEC pCodec, const void *pvData, size_t cbData, uint64_t msAbsPTS, uint32_t uFlags, void *pvUser));
    /** User-supplied data pointer. */
    void                   *pvUser;
} RECORDINGCODECCALLBACKS, *PRECORDINGCODECCALLBACKS;

/**
 * Structure for keeping generic codec parameters.
 */
typedef struct RECORDINGCODECPARMS
{
    /** The generic codec type. */
    RECORDINGCODECTYPE          enmType;
    /** The specific codec type, based on \a enmType. */
    union
    {
        /** The container's video codec to use. */
        RecordingVideoCodec_T   enmVideoCodec;
        /** The container's audio codec to use. */
        RecordingAudioCodec_T   enmAudioCodec;
    };
    union
    {
        struct
        {
            /** Frames per second. */
            uint8_t             uFPS;
            /** Target width (in pixels) of encoded video image. */
            uint16_t            uWidth;
            /** Target height (in pixels) of encoded video image. */
            uint16_t            uHeight;
            /** Minimal delay (in ms) between two video frames.
             *  This value is based on the configured FPS rate. */
            uint32_t            uDelayMs;
        } Video;
        struct
        {
            /** The codec's used PCM properties. */
            PDMAUDIOPCMPROPS    PCMProps;
        } Audio;
    };
    /** Desired (average) bitrate (in kbps) to use, for codecs which support bitrate management.
     *  Set to 0 to use a variable bit rate (VBR) (if available, otherwise fall back to CBR). */
    uint32_t                    uBitrate;
    /** Time (in ms) the encoder expects us to send data to encode.
     *
     *  For Vorbis, valid frame sizes are powers of two from 64 to 8192 bytes.
     */
    uint32_t                    msFrame;
    /** The frame size in bytes (based on \a msFrame). */
    uint32_t                    cbFrame;
    /** The frame size in samples per frame (based on \a msFrame). */
    uint32_t                    csFrame;
} RECORDINGCODECPARMS, *PRECORDINGCODECPARMS;

#ifdef VBOX_WITH_LIBVPX
/**
 * VPX encoder state (needs libvpx).
 */
typedef struct RECORDINGCODECVPX
{
    /** VPX codec context. */
    vpx_codec_ctx_t     Ctx;
    /** VPX codec configuration. */
    vpx_codec_enc_cfg_t Cfg;
    /** VPX image context. */
    vpx_image_t         RawImage;
    /** Pointer to the codec's internal YUV buffer. */
    uint8_t            *pu8YuvBuf;
    /** The encoder's deadline (in ms).
     *  The more time the encoder is allowed to spend encoding, the better the encoded
     *  result, in exchange for higher CPU usage and time spent encoding. */
    unsigned int        uEncoderDeadline;
} RECORDINGCODECVPX;
/** Pointer to a VPX encoder state. */
typedef RECORDINGCODECVPX *PRECORDINGCODECVPX;
#endif /* VBOX_WITH_LIBVPX */

#ifdef VBOX_WITH_LIBVORBIS
/**
 * Vorbis encoder state (needs libvorbis + libogg).
 */
typedef struct RECORDINGCODECVORBIS
{
    /** Basic information about the audio in a Vorbis bitstream. */
    vorbis_info      info;
    /** Encoder state. */
    vorbis_dsp_state dsp_state;
    /** Current block being worked on. */
    vorbis_block     block_cur;
} RECORDINGCODECVORBIS;
/** Pointer to a Vorbis encoder state. */
typedef RECORDINGCODECVORBIS *PRECORDINGCODECVORBIS;
#endif /* VBOX_WITH_LIBVORBIS */

/**
 * Structure for keeping a codec's internal state.
 */
typedef struct RECORDINGCODECSTATE
{
    /** Timestamp Timestamp (PTS, in ms) of the last frame was encoded. */
    uint64_t            tsLastWrittenMs;
    /** Number of encoding errors. */
    uint64_t            cEncErrors;
} RECORDINGCODECSTATE;
/** Pointer to an internal encoder state. */
typedef RECORDINGCODECSTATE *PRECORDINGCODECSTATE;

/**
 * Structure for keeping codec-specific data.
 */
typedef struct RECORDINGCODEC
{
    /** Callback table for codec operations. */
    RECORDINGCODECOPS           Ops;
    /** Table for user-supplied callbacks. */
    RECORDINGCODECCALLBACKS     Callbacks;
    /** Generic codec parameters. */
    RECORDINGCODECPARMS         Parms;
    /** Generic codec parameters. */
    RECORDINGCODECSTATE         State;

#ifdef VBOX_WITH_LIBVPX
    union
    {
        RECORDINGCODECVPX       VPX;
    } Video;
#endif

#ifdef VBOX_WITH_AUDIO_RECORDING
    union
    {
# ifdef VBOX_WITH_LIBVORBIS
        RECORDINGCODECVORBIS    Vorbis;
# endif /* VBOX_WITH_LIBVORBIS */
    } Audio;
#endif /* VBOX_WITH_AUDIO_RECORDING */

    /** Internal scratch buffer for en-/decoding steps. */
    void               *pvScratch;
    /** Size (in bytes) of \a pvScratch. */
    uint32_t            cbScratch;

#ifdef VBOX_WITH_STATISTICS /** @todo Register these values with STAM. */
    struct
    {
        /** Number of frames encoded. */
        uint64_t        cEncBlocks;
        /** Total time (in ms) of already encoded audio data. */
        uint64_t        msEncTotal;
    } STAM;
#endif
} RECORDINGCODEC, *PRECORDINGCODEC;

/**
 * Enumeration for supported pixel formats.
 */
enum RECORDINGPIXELFMT
{
    /** Unknown pixel format. */
    RECORDINGPIXELFMT_UNKNOWN    = 0,
    /** RGB 24. */
    RECORDINGPIXELFMT_RGB24      = 1,
    /** RGB 24. */
    RECORDINGPIXELFMT_RGB32      = 2,
    /** RGB 565. */
    RECORDINGPIXELFMT_RGB565     = 3,
    /** The usual 32-bit hack. */
    RECORDINGPIXELFMT_32BIT_HACK = 0x7fffffff
};

/**
 * Enumeration for a recording frame type.
 */
enum RECORDINGFRAME_TYPE
{
    /** Invalid frame type; do not use. */
    RECORDINGFRAME_TYPE_INVALID   = 0,
    /** Frame is an audio frame. */
    RECORDINGFRAME_TYPE_AUDIO     = 1,
    /** Frame is an video frame. */
    RECORDINGFRAME_TYPE_VIDEO     = 2,
    /** Frame contains a video frame pointer. */
    RECORDINGFRAME_TYPE_VIDEO_PTR = 3
};

/**
 * Structure for keeping a single recording video frame.
 */
typedef struct RECORDINGVIDEOFRAME
{
    /** X origin  (in pixel) of this frame. */
    uint16_t            uX;
    /** X origin  (in pixel) of this frame. */
    uint16_t            uY;
    /** X resolution (in pixel) of this frame. */
    uint16_t            uWidth;
    /** Y resolution (in pixel)  of this frame. */
    uint16_t            uHeight;
    /** Bits per pixel (BPP). */
    uint8_t             uBPP;
    /** Pixel format of this frame. */
    RECORDINGPIXELFMT   enmPixelFmt;
    /** Bytes per scan line. */
    uint16_t            uBytesPerLine;
    /** RGB buffer containing the unmodified frame buffer data from Main's display. */
    uint8_t            *pu8RGBBuf;
    /** Size (in bytes) of the RGB buffer. */
    size_t              cbRGBBuf;
} RECORDINGVIDEOFRAME, *PRECORDINGVIDEOFRAME;

/**
 * Structure for keeping a single recording audio frame.
 */
typedef struct RECORDINGAUDIOFRAME
{
    /** Pointer to audio data. */
    uint8_t            *pvBuf;
    /** Size (in bytes) of audio data. */
    size_t              cbBuf;
} RECORDINGAUDIOFRAME, *PRECORDINGAUDIOFRAME;

/**
 * Structure for keeping a single recording audio frame.
 */
typedef struct RECORDINGFRAME
{
    /** List node. */
    RTLISTNODE              Node;
    /** Stream index (hint) where this frame should go to.
     *  Specify UINT16_MAX to broadcast to all streams. */
    uint16_t                idStream;
    /** The frame type. */
    RECORDINGFRAME_TYPE     enmType;
    /** Timestamp (PTS, in ms). */
    uint64_t                msTimestamp;
    union
    {
#ifdef VBOX_WITH_AUDIO_RECORDING
        /** Audio frame data. */
        RECORDINGAUDIOFRAME  Audio;
#endif
        /** Video frame data. */
        RECORDINGVIDEOFRAME  Video;
        /** A (weak) pointer to a video frame. */
        RECORDINGVIDEOFRAME *VideoPtr;
    };
} RECORDINGFRAME, *PRECORDINGFRAME;

/**
 * Enumeration for specifying a video recording block type.
 */
typedef enum RECORDINGBLOCKTYPE
{
    /** Uknown block type, do not use. */
    RECORDINGBLOCKTYPE_UNKNOWN = 0,
    /** The block is a video frame. */
    RECORDINGBLOCKTYPE_VIDEO,
    /** The block is an audio frame. */
    RECORDINGBLOCKTYPE_AUDIO
} RECORDINGBLOCKTYPE;

#ifdef VBOX_WITH_AUDIO_RECORDING
int RecordingVideoFrameInit(PRECORDINGVIDEOFRAME pFrame, int w, int h, uint8_t uBPP, RECORDINGPIXELFMT enmPixelFmt);
void RecordingVideoFrameDestroy(PRECORDINGVIDEOFRAME pFrame);
void RecordingAudioFrameFree(PRECORDINGAUDIOFRAME pFrame);
#endif
void RecordingVideoFrameFree(PRECORDINGVIDEOFRAME pFrame);
void RecordingFrameFree(PRECORDINGFRAME pFrame);

/**
 * Generic structure for keeping a single video recording (data) block.
 */
struct RecordingBlock
{
    RecordingBlock()
        : enmType(RECORDINGBLOCKTYPE_UNKNOWN)
        , cRefs(0)
        , uFlags(RECORDINGCODEC_ENC_F_NONE)
        , pvData(NULL)
        , cbData(0) { }

    virtual ~RecordingBlock()
    {
        Reset();
    }

    void Reset(void)
    {
        switch (enmType)
        {
            case RECORDINGBLOCKTYPE_UNKNOWN:
                break;

            case RECORDINGBLOCKTYPE_VIDEO:
                RecordingVideoFrameFree((PRECORDINGVIDEOFRAME)pvData);
                break;

#ifdef VBOX_WITH_AUDIO_RECORDING
            case RECORDINGBLOCKTYPE_AUDIO:
                RecordingAudioFrameFree((PRECORDINGAUDIOFRAME)pvData);
                break;
#endif
            default:
                AssertFailed();
                break;
        }

        enmType = RECORDINGBLOCKTYPE_UNKNOWN;
        cRefs   = 0;
        pvData  = NULL;
        cbData  = 0;
    }

    /** The block's type. */
    RECORDINGBLOCKTYPE enmType;
    /** Number of references held of this block. */
    uint16_t           cRefs;
    /** Block flags of type RECORDINGCODEC_ENC_F_XXX. */
    uint64_t           uFlags;
    /** The (absolute) timestamp (in ms, PTS) of this block. */
    uint64_t           msTimestamp;
    /** Opaque data block to the actual block data, depending on the block's type. */
    void              *pvData;
    /** Size (in bytes) of the (opaque) data block. */
    size_t             cbData;
};

/** List for keeping video recording (data) blocks. */
typedef std::list<RecordingBlock *> RecordingBlockList;

int recordingCodecCreateAudio(PRECORDINGCODEC pCodec, RecordingAudioCodec_T enmAudioCodec);
int recordingCodecCreateVideo(PRECORDINGCODEC pCodec, RecordingVideoCodec_T enmVideoCodec);
int recordingCodecInit(const PRECORDINGCODEC pCodec, const PRECORDINGCODECCALLBACKS pCallbacks, const settings::RecordingScreenSettings &Settings);
int recordingCodecDestroy(PRECORDINGCODEC pCodec);
int recordingCodecEncode(PRECORDINGCODEC pCodec, const PRECORDINGFRAME pFrame, size_t *pcEncoded, size_t *pcbEncoded);
int recordingCodecFinalize(PRECORDINGCODEC pCodec);
bool recordingCodecIsInitialized(const PRECORDINGCODEC pCodec);
uint32_t recordingCodecGetWritable(const PRECORDINGCODEC pCodec, uint64_t msTimestamp);
#endif /* !MAIN_INCLUDED_RecordingInternals_h */
