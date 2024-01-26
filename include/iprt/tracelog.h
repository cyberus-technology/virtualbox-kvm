/** @file
 * IPRT - Binary trace log API.
 */

/*
 * Copyright (C) 2018-2023 Oracle and/or its affiliates.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_tracelog_h
#define IPRT_INCLUDED_tracelog_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/sg.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN


/** @defgroup grp_tracelog      RTTraceLog - Binary trace log API
 * @ingroup grp_rt
 * @{
 */

/**
 * Trace log item type.
 */
typedef enum RTTRACELOGTYPE
{
    /** Invalid first value. */
    RTTRACELOGTYPE_INVALID = 0,
    /** Boolean item type. */
    RTTRACELOGTYPE_BOOL,
    /** Unsigned 8bit integer type. */
    RTTRACELOGTYPE_UINT8,
    /** Signed 8bit integer type. */
    RTTRACELOGTYPE_INT8,
    /** Unsigned 16bit integer type. */
    RTTRACELOGTYPE_UINT16,
    /** Signed 16bit integer type. */
    RTTRACELOGTYPE_INT16,
    /** Unsigned 32bit integer type. */
    RTTRACELOGTYPE_UINT32,
    /** Signed 32bit integer type. */
    RTTRACELOGTYPE_INT32,
    /** Unsigned 64bit integer type. */
    RTTRACELOGTYPE_UINT64,
    /** Signed 64bit integer type. */
    RTTRACELOGTYPE_INT64,
    /** 32bit floating point type. */
    RTTRACELOGTYPE_FLOAT32,
    /** 64bit floating point type. */
    RTTRACELOGTYPE_FLOAT64,
    /** Raw binary data type. */
    RTTRACELOGTYPE_RAWDATA,
    /** Pointer data type. */
    RTTRACELOGTYPE_POINTER,
    /** size_t data type. */
    RTTRACELOGTYPE_SIZE,
    /** 32-bit hack. */
    RTTRACELOGTYPE_32BIT_HACK = 0x7fffffff
} RTTRACELOGTYPE;
/** Pointer to a trace log item type. */
typedef RTTRACELOGTYPE *PRTTRACELOGTYPE;
/** Pointer to a const trace log item type. */
typedef const RTTRACELOGTYPE *PCRTTRACELOGTYPE;


/**
 * Trace log event severity.
 */
typedef enum RTTRACELOGEVTSEVERITY
{
    /** Invalid severity. */
    RTTRACELOGEVTSEVERITY_INVALID = 0,
    /** Informational event. */
    RTTRACELOGEVTSEVERITY_INFO,
    /** Warning event. */
    RTTRACELOGEVTSEVERITY_WARNING,
    /** Error event. */
    RTTRACELOGEVTSEVERITY_ERROR,
    /** Fatal event. */
    RTTRACELOGEVTSEVERITY_FATAL,
    /** Debug event. */
    RTTRACELOGEVTSEVERITY_DEBUG,
    /** 32bit hack.*/
    RTTRACELOGEVTSEVERITY_32BIT_HACK = 0x7fffffff
} RTTRACELOGEVTSEVERITY;
/** Pointer to a event severity class. */
typedef RTTRACELOGEVTSEVERITY *PRTTRACELOGEVTSEVERITY;
/** Pointer to a const event severiy class. */
typedef RTTRACELOGEVTSEVERITY *PCRTTRACELOGEVTSEVERITY;


/**
 * Trace log reader event.
 */
typedef enum RTTRACELOGRDRPOLLEVT
{
    /** Invalid event. */
    RTTRACELOGRDRPOLLEVT_INVALID = 0,
    /** The header was received and valid. */
    RTTRACELOGRDRPOLLEVT_HDR_RECVD,
    /** Event data was fetched. */
    RTTRACELOGRDRPOLLEVT_TRACE_EVENT_RECVD,
    /** 32bit hack. */
    RTTRACELOGRDRPOLLEVT_32BIT_HACK = 0x7fffffff
} RTTRACELOGRDRPOLLEVT;
/** Pointer to a trace log reader event. */
typedef RTTRACELOGRDRPOLLEVT *PRTTRACELOGRDRPOLLEVT;


/**
 * Trace log event item descriptor.
 */
typedef struct RTTRACELOGEVTITEMDESC
{
    /** Event item name. */
    const char                     *pszName;
    /** Event item description. */
    const char                     *pszDesc;
    /** Event item type. */
    RTTRACELOGTYPE                 enmType;
    /** The size of the raw data if static for the item,
     * 0 otherwise (and given when the event is logged).
     * Only valid for the RTTRACELOGTYPE_RAWDATA type,
     * ignored otherwise. */
    size_t                         cbRawData;
} RTTRACELOGEVTITEMDESC;
/** Pointer to an trace log event item descriptor. */
typedef RTTRACELOGEVTITEMDESC *PRTTRACELOGEVTITEMDESC;
/** Pointer to a const trace log event item descriptor. */
typedef const RTTRACELOGEVTITEMDESC *PCRTTRACELOGEVTITEMDESC;
/** Pointer to a trace log event item descriptor pointer. */
typedef PRTTRACELOGEVTITEMDESC *PPRTTRACELOGEVTITEMDESC;
/** Pointer to a const trace log event item descriptor pointer. */
typedef PCRTTRACELOGEVTITEMDESC *PPCRTTRACELOGEVTITEMDESC;


/**
 * Trace log event descriptor.
 */
typedef struct RTTRACELOGEVTDESC
{
    /** Event identifier. */
    const char                     *pszId;
    /** Event description. */
    const char                     *pszDesc;
    /** Severity class of the event. */
    RTTRACELOGEVTSEVERITY          enmSeverity;
    /** Number of items recorded for an event. */
    uint32_t                       cEvtItems;
    /** Pointer to array of event item descriptors. */
    PCRTTRACELOGEVTITEMDESC        paEvtItemDesc;
} RTTRACELOGEVTDESC;
/** Pointer to a trace log event descriptor. */
typedef RTTRACELOGEVTDESC *PRTTRACELOGEVTDESC;
/** Pointer to a const trace log event descriptor. */
typedef const RTTRACELOGEVTDESC *PCRTTRACELOGEVTDESC;


/**
 * Trace log event item value.
 */
typedef struct RTTRACELOGEVTVAL
{
    /** Pointer to the corresponding event item descriptor. */
    PCRTTRACELOGEVTITEMDESC     pItemDesc;
    /** Value union. */
    union
    {
        bool                    f;
        uint8_t                 u8;
        int8_t                  i8;
        uint16_t                u16;
        int16_t                 i16;
        uint32_t                u32;
        int32_t                 i32;
        uint64_t                u64;
        int64_t                 i64;
        uint64_t                sz;
        uint64_t                uPtr;
        float                   f32;
        double                  f64;
        struct
        {
            size_t              cb;
            const uint8_t       *pb;
        } RawData;
    } u;
} RTTRACELOGEVTVAL;
/** Pointer to trace log event item value. */
typedef RTTRACELOGEVTVAL *PRTTRACELOGEVTVAL;
/** Pointer to a const trace log event item value. */
typedef const RTTRACELOGEVTVAL *PCRTTRACELOGEVTVAL;


/**
 * Item mapping descriptor.
 */
typedef struct RTTRACELOGRDRMAPITEM
{
    /** The item name. */
    const char                  *pszName;
    /** The value type to map the item to. */
    RTTRACELOGTYPE              enmType;
} RTTRACELOGRDRMAPITEM;
/** Pointer to a mapping item descriptor. */
typedef RTTRACELOGRDRMAPITEM *PRTTRACELOGRDRMAPITEM;
/** Pointer to a const mapping item descriptor. */
typedef const RTTRACELOGRDRMAPITEM *PCRTTRACELOGRDRMAPITEM;


/**
 * Event item to value mapping descriptor for RTTraceLogRdrEvtMapToStruct().
 */
typedef struct RTTRACELOGRDRMAPDESC
{
    /** The event ID this mapping describes. */
    const char                  *pszEvtId;
    /** Number of event items to extract. */
    uint32_t                    cEvtItems;
    /** Pointer to the event items to extract (in the given order). */
    PCRTTRACELOGRDRMAPITEM      paMapItems;
} RTTRACELOGRDRMAPDESC;
/** Pointer to a event mapping descriptor. */
typedef RTTRACELOGRDRMAPDESC *PRTTRACELOGRDRMAPDESC;
/** Pointer to a const event mapping descriptor. */
typedef const RTTRACELOGRDRMAPDESC *PCRTTRACELOGRDRMAPDESC;


/**
 * Header for an event mapped to a binary.
 */
typedef struct RTTRACELOGRDREVTHDR
{
    /** The mapping descriptor this event was mapped to. */
    PCRTTRACELOGRDRMAPDESC      pEvtMapDesc;
    /** The event descriptor as extracted from the event log. */
    PCRTTRACELOGEVTDESC         pEvtDesc;
    /** Sequence number of the descriptor. */
    uint64_t                    idSeqNo;
    /** The timestamp of the event. */
    uint64_t                    tsEvt;
    /** Pointer to the event data items. */
    PCRTTRACELOGEVTVAL          paEvtItems;
} RTTRACELOGRDREVTHDR;
/** Pointer to an event header. */
typedef RTTRACELOGRDREVTHDR *PRTTRACELOGRDREVTHDR;
/** Pointer to a const event header. */
typedef const RTTRACELOGRDREVTHDR *PCRTTRACELOGRDREVTHDR;


/** Event group ID. */
typedef uint64_t                   RTTRACELOGEVTGRPID;
/** Pointer to the event group ID. */
typedef RTTRACELOGEVTGRPID         *PRTTRACELOGEVTGRPID;
/** Trace log event handle. */
typedef uint64_t                   RTRACELOGEVT;
/** Pointer to a trace log event handle. */
typedef RTRACELOGEVT               *PRTRACELOGEVT;
/** Trace log writer handle. */
typedef struct RTTRACELOGWRINT     *RTTRACELOGWR;
/** Pointer to a trace log writer handle. */
typedef RTTRACELOGWR               *PRTTRACELOGWR;
/** NIL trace log writer handle value. */
#define NIL_RTTRACELOGWR           ((RTTRACELOGWR)0)
/** Trace log reader handle. */
typedef struct RTTRACELOGRDRINT    *RTTRACELOGRDR;
/** Pointer to a trace log reader handle. */
typedef RTTRACELOGRDR              *PRTTRACELOGRDR;
/** NIL trace log reader handle value. */
#define NIL_RTTRACELOGRDR          ((RTTRACELOGRDR)0)
/** Trace log reader iterator handle. */
typedef struct RTTRACELOGRDRITINT  *RTTRACELOGRDRIT;
/** Pointer to a trace log reader iterator handle. */
typedef RTTRACELOGRDRIT            *PRTTRACELOGRDRIT;
/** NIL trace log reader iterator handle. */
#define NIL_RTTRACELOGRDRIT        ((RTTRACELOGRDRIT)0)
/** Trace log reader event handle. */
typedef struct RTTRACELOGRDREVTINT *RTTRACELOGRDREVT;
/** Pointer to a trace log reader event handle. */
typedef RTTRACELOGRDREVT           *PRTTRACELOGRDREVT;
/** NIL trace log reader event handle. */
#define NIL_RTTRACELOGRDREVT       ((RTTRACELOGRDREVT)0)

/** A new grouped event is started. */
#define RTTRACELOG_WR_ADD_EVT_F_GRP_START  RT_BIT_32(0)
/** A grouped event is finished. */
#define RTTRACELOG_WR_ADD_EVT_F_GRP_FINISH RT_BIT_32(1)

/**
 * Callback to stream out data from the trace log writer.
 *
 * @returns IPRT status code.
 * @param   pvUser              Opaque user data passed on trace log writer creation.
 * @param   pvBuf               Pointer to the buffer to stream out.
 * @param   cbBuf               Number of bytes to stream.
 * @param   pcbWritten          Where to store the number of bytes written on success, optional.
 */
typedef DECLCALLBACKTYPE(int, FNRTTRACELOGWRSTREAM,(void *pvUser, const void *pvBuf, size_t cbBuf, size_t *pcbWritten));
/** Pointer to a writer stream callback. */
typedef FNRTTRACELOGWRSTREAM *PFNRTTRACELOGWRSTREAM;


/**
 * Callback to stream int data to the trace log reader.
 *
 * @returns IPRT status code.
 * @retval  VERR_EOF if the stream reached the end.
 * @retval  VERR_INTERRUPTED if waiting for something to arrive was interrupted.
 * @retval  VERR_TIMEOUT if the timeout was reached.
 * @param   pvUser              Opaque user data passed on trace log reader creation.
 * @param   pvBuf               Where to store the read data.
 * @param   cbBuf               Number of bytes the buffer can hold.
 * @param   pcbRead             Where to store the number of bytes read on success.
 * @param   cMsTimeout          How long to wait for something to arrive
 */
typedef DECLCALLBACKTYPE(int, FNRTTRACELOGRDRSTREAM,(void *pvUser, void *pvBuf, size_t cbBuf, size_t *pcbRead,
                                                     RTMSINTERVAL cMsTimeout));
/** Pointer to a writer stream callback. */
typedef FNRTTRACELOGRDRSTREAM *PFNRTTRACELOGRDRSTREAM;


/**
 * Callback to close the stream.
 *
 * @returns IPRT status code.
 * @param   pvUser              Opaque user data passed on trace log writer creation.
 */
typedef DECLCALLBACKTYPE(int, FNRTTRACELOGSTREAMCLOSE,(void *pvUser));
/** Pointer to a stream close callback. */
typedef FNRTTRACELOGSTREAMCLOSE *PFNRTTRACELOGSTREAMCLOSE;


/**
 * Creates a new trace log writer.
 *
 * @returns IPRT status code.
 * @param   phTraceLogWr        Where to store the handle to the trace log writer on success.
 * @param   pszDesc             Optional description to store in the header.
 * @param   pfnStreamOut        The callback to use for streaming the trace log data.
 * @param   pfnStreamClose      The callback to use for closing the stream.
 * @param   pvUser              Opaque user data to pass to the streaming callback.
 */
RTDECL(int) RTTraceLogWrCreate(PRTTRACELOGWR phTraceLogWr, const char *pszDesc,
                               PFNRTTRACELOGWRSTREAM pfnStreamOut,
                               PFNRTTRACELOGSTREAMCLOSE pfnStreamClose, void *pvUser);


/**
 * Creates a new trace log writer streaming data to the given file.
 *
 * @returns IPRT status code.
 * @param   phTraceLogWr        Where to store the handle to the trace log writer on success.
 * @param   pszDesc             Optional description to store in the header.
 * @param   pszFilename         The filename to stream the data to.
 */
RTDECL(int) RTTraceLogWrCreateFile(PRTTRACELOGWR phTraceLogWr, const char *pszDesc,
                                   const char *pszFilename);


/**
 * Creates a new TCP server style trace log writer waiting for the other end to connect to it.
 *
 * @returns IPRT status code.
 * @param   phTraceLogWr        Where to store the handle to the trace log writer on success.
 * @param   pszDesc             Optional description to store in the header.
 * @param   pszListen           The address to listen on, NULL to listen on all interfaces.
 * @param   uPort               The port to listen on.
 *
 * @note The writer will block here until a client has connected.
 */
RTDECL(int) RTTraceLogWrCreateTcpServer(PRTTRACELOGWR phTraceLogWr, const char *pszDesc,
                                        const char *pszListen, unsigned uPort);


/**
 * Creates a new TCP client style trace log writer connecting to the other end.
 *
 * @returns IPRT status code.
 * @param   phTraceLogWr        Where to store the handle to the trace log writer on success.
 * @param   pszDesc             Optional description to store in the header.
 * @param   pszAddress          The address to connect to.
 * @param   uPort               The port to connect to.
 *
 * @note An error is returned if no connection can be established.
 */
RTDECL(int) RTTraceLogWrCreateTcpClient(PRTTRACELOGWR phTraceLogWr, const char *pszDesc,
                                        const char *pszAddress, unsigned uPort);


/**
 * Destroys the given trace log writer instance.
 *
 * @returns IPRT status code.
 * @param   hTraceLogWr         The trace log writer instance handle.
 */
RTDECL(int) RTTraceLogWrDestroy(RTTRACELOGWR hTraceLogWr);


/**
 * Adds a given event structure descriptor to the given trace log writer instance
 * (for prepopulation).
 *
 * @returns IPRT status code.
 * @param   hTraceLogWr         The trace log writer instance handle.
 * @param   pEvtDesc            The event structure descriptor to add.
 *
 * @note The event descriptor is keyed by the pointer for faster lookup in subsequent calls,
 *       so don't free after this method finishes.
 */
RTDECL(int) RTTraceLogWrAddEvtDesc(RTTRACELOGWR hTraceLogWr, PCRTTRACELOGEVTDESC pEvtDesc);


/**
 * Adds a new event to the trace log.
 *
 * @returns IPRT status code.
 * @param   hTraceLogWr         The trace log writer instance handle.
 * @param   pEvtDesc            The event descriptor to use for formatting.
 * @param   fFlags              Flags to use for this event.y
 * @param   uGrpId              A unique group ID for grouped events.
 * @param   uParentGrpId        A parent group ID this event originated from.
 * @param   pvEvtData           Pointer to the raw event data.
 * @param   pacbRawData         Pointer to the array of size indicators for non static raw data in the event data stream.
 *
 * @note The event descriptor is keyed by the pointer for faster lookup in subsequent calls,
 *       so don't free after this method finishes.
 */
RTDECL(int) RTTraceLogWrEvtAdd(RTTRACELOGWR hTraceLogWr, PCRTTRACELOGEVTDESC pEvtDesc, uint32_t fFlags,
                               RTTRACELOGEVTGRPID uGrpId, RTTRACELOGEVTGRPID uParentGrpId,
                               const void *pvEvtData, size_t *pacbRawData);


/**
 * Adds a new event to the trace log.
 *
 * @returns IPRT status code.
 * @param   hTraceLogWr         The trace log writer instance handle.
 * @param   pEvtDesc            The event descriptor used for formatting the data.
 * @param   fFlags              Flags to use for this event.
 * @param   uGrpId              A unique group ID for grouped events.
 * @param   uParentGrpId        A parent group ID this event originated from.
 * @param   pSgBufEvtData       S/G buffer holding the raw event data.
 * @param   pacbRawData         Pointer to the array of size indicators for non static raw data in the event data stream.
 *
 * @note The event descriptor is keyed by the pointer for faster lookup in subsequent calls,
 *       so don't free after this method finishes.
 */
RTDECL(int) RTTraceLogWrEvtAddSg(RTTRACELOGWR hTraceLogWr, PCRTTRACELOGEVTDESC pEvtDesc, uint32_t fFlags,
                                 RTTRACELOGEVTGRPID uGrpId, RTTRACELOGEVTGRPID uParentGrpId,
                                 PRTSGBUF *pSgBufEvtData, size_t *pacbRawData);


/**
 * Adds a new event to the trace log - list variant.
 *
 * @returns IPRT status code.
 * @param   hTraceLogWr         The trace log writer instance handle.
 * @param   pEvtDesc            The event descriptor used for formatting the data.
 * @param   fFlags              Flags to use for this event.
 * @param   uGrpId              A unique group ID for grouped events.
 * @param   uParentGrpId        A parent group ID this event originated from.
 * @param   va                  The event data as single items as described by the descriptor.
 *
 * @note The event descriptor is keyed by the pointer for faster lookup in subsequent calls,
 *       so don't free after this method finishes.
 */
RTDECL(int) RTTraceLogWrEvtAddLV(RTTRACELOGWR hTraceLogWr, PCRTTRACELOGEVTDESC pEvtDesc, uint32_t fFlags,
                                 RTTRACELOGEVTGRPID uGrpId, RTTRACELOGEVTGRPID uParentGrpId, va_list va);


/**
 * Adds a new event to the trace log - list variant.
 *
 * @returns IPRT status code.
 * @param   hTraceLogWr         The trace log writer instance handle.
 * @param   pEvtDesc            The event descriptor used for formatting the data.
 * @param   fFlags              Flags to use for this event.
 * @param   uGrpId              A unique group ID for grouped events.
 * @param   uParentGrpId        A parent group ID this event originated from.
 * @param   ...                 The event data as single items as described by the descriptor.
 *
 * @note The event descriptor is keyed by the pointer for faster lookup in subsequent calls,
 *       so don't free after this method finishes.
 */
RTDECL(int) RTTraceLogWrEvtAddL(RTTRACELOGWR hTraceLogWr, PCRTTRACELOGEVTDESC pEvtDesc, uint32_t fFlags,
                                RTTRACELOGEVTGRPID uGrpId, RTTRACELOGEVTGRPID uParentGrpId, ...);


/**
 * Creates a new trace log reader instance.
 *
 * @returns IPRT status code.
 * @param   phTraceLogRdr       Where to store the handle to the trace log reader instance on success.
 * @param   pfnStreamIn         Callback to stream the data into the reader.
 * @param   pfnStreamClose      The callback to use for closing the stream.
 * @param   pvUser              Opaque user data passed to the stream callback.
 */
RTDECL(int) RTTraceLogRdrCreate(PRTTRACELOGRDR phTraceLogRdr, PFNRTTRACELOGRDRSTREAM pfnStreamIn,
                                PFNRTTRACELOGSTREAMCLOSE pfnStreamClose, void *pvUser);


/**
 * Creates a new trace log reader for the given file.
 *
 * @returns IPRT status code.
 * @param   phTraceLogRdr       Where to store the handle to the trace log reader instance on success.
 * @param   pszFilename         The file to read the trace log data from.
 */
RTDECL(int) RTTraceLogRdrCreateFromFile(PRTTRACELOGRDR phTraceLogRdr, const char *pszFilename);


/**
 * Destroys the given trace log reader instance.
 *
 * @returns IPRT status code.
 * @param   hTraceLogRdr        The trace log reader instance handle.
 */
RTDECL(int) RTTraceLogRdrDestroy(RTTRACELOGRDR hTraceLogRdr);


/**
 * Polls for an event on the trace log reader instance.
 *
 * @returns IPRT status code.
 * @retval  VERR_TIMEOUT if the timeout was reached.
 * @retval  VERR_INTERRUPTED if the poll was interrupted.
 * @param   hTraceLogRdr        The trace log reader instance handle.
 * @param   penmEvt             Where to store the event identifier.
 * @param   cMsTimeout          How long to poll for an event.
 */
RTDECL(int) RTTraceLogRdrEvtPoll(RTTRACELOGRDR hTraceLogRdr, RTTRACELOGRDRPOLLEVT *penmEvt, RTMSINTERVAL cMsTimeout);

/**
 * Queries the last received event from the trace log read instance.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if no event was received so far.
 * @param   hTraceLogRdr        The trace log reader instance handle.
 * @param   phRdrEvt            Where to store the event handle on success.
 */
RTDECL(int) RTTraceLogRdrQueryLastEvt(RTTRACELOGRDR hTraceLogRdr, PRTTRACELOGRDREVT phRdrEvt);

/**
 * Queries a new iterator for walking received events.
 *
 * @returns IPRT status code
 * @param   hTraceLogRdr        The trace log reader instance handle.
 * @param   phIt                Where to store the handle to iterator on success.
 */
RTDECL(int) RTTraceLogRdrQueryIterator(RTTRACELOGRDR hTraceLogRdr, PRTTRACELOGRDRIT phIt);


/**
 * Extracts the given number of events from the given trace log reader instance returning
 * and array of events with the values filled in from the mapping descriptor.
 *
 * @returns IPRT status code.
 * @param   hTraceLogRdr        The trace log reader instance handle.
 * @param   fFlags              Flags controlling the behavior, MBZ.
 * @param   cEvts               Number of events to extract, UINT32_MAX to map all immediately available events.
 * @param   paMapDesc           Pointer to an array of mapping descriptors describing how to map events.
 * @param   ppaEvtHdr           Where to return the pointer to the allocated array of event headers on success.
 * @param   pcEvts              Where to store the returned number of events on success.
 */
RTDECL(int) RTTraceLogRdrEvtMapToStruct(RTTRACELOGRDR hTraceLogRdr, uint32_t fFlags, uint32_t cEvts,
                                        PCRTTRACELOGRDRMAPDESC paMapDesc, PCRTTRACELOGRDREVTHDR *ppaEvtHdr,
                                        uint32_t *pcEvts);


/**
 * Frees all resources of the given array of event headers as allocated by RTTraceLogRdrEvtMapToStruct().
 *
 * @param   paEvtHdr            Pointer to the array of events as returned by RTTraceLogRdrEvtMapToStruct().
 * @param   cEvts               Number of events as returned by RTTraceLogRdrEvtMapToStruct().
 */
RTDECL(void) RTTraceLogRdrEvtMapFree(PCRTTRACELOGRDREVTHDR paEvtHdr, uint32_t cEvts);


/**
 * Frees a previously created iterator.
 *
 * @param   hIt                 The iterator handle to free.
 */
RTDECL(void) RTTraceLogRdrIteratorFree(RTTRACELOGRDRIT hIt);


/**
 * Advances to the next event.
 *
 * @returns IPRT status code
 * @retval  VERR_TRACELOG_READER_ITERATOR_END if the iterator reached the end.
 * @param   hIt                 The iterator handle.
 */
RTDECL(int) RTTraceLogRdrIteratorNext(RTTRACELOGRDRIT hIt);


/**
 * Queries the event at the current iterator position.
 *
 * @returns IPRT status code.
 * @param   hIt                 The iterator handle.
 * @param   phRdrEvt            Where to store the event handle on success.
 */
RTDECL(int) RTTraceLogRdrIteratorQueryEvent(RTTRACELOGRDRIT hIt, PRTTRACELOGRDREVT phRdrEvt);


/**
 * Returns the sequence number of the given event.
 *
 * @returns Sequence number of the given event.
 * @param   hRdrEvt             The reader event handle.
 */
RTDECL(uint64_t) RTTraceLogRdrEvtGetSeqNo(RTTRACELOGRDREVT hRdrEvt);


/**
 * Gets the timestamp of the given event.
 *
 * @returns Timestamp of the given event.
 * @param   hRdrEvt             The reader event handle.
 */
RTDECL(uint64_t) RTTraceLogRdrEvtGetTs(RTTRACELOGRDREVT hRdrEvt);


/**
 * Returns whether the given event is part of an event group.
 *
 * @returns Flag whether the event is part of a group.
 * @param   hRdrEvt             The reader event handle.
 */
RTDECL(bool) RTTraceLogRdrEvtIsGrouped(RTTRACELOGRDREVT hRdrEvt);


/**
 * Returns the event descriptor associated with the given event.
 *
 * @returns The trace log event descriptor associated with this event.
 * @param   hRdrEvt             The reader event handle.
 */
RTDECL(PCRTTRACELOGEVTDESC) RTTraceLogRdrEvtGetDesc(RTTRACELOGRDREVT hRdrEvt);


/**
 * Queries an event item by its name returning the value in the supplied buffer.
 *
 * @returns IPRT status code.
 * @retval  VERR_NOT_FOUND if the item name was not found for the given event.
 * @param   hRdrEvt             The reader event handle.
 * @param   pszName             The item name to query.
 * @param   pVal                The item value buffer to initialise.
 */
RTDECL(int) RTTraceLogRdrEvtQueryVal(RTTRACELOGRDREVT hRdrEvt, const char *pszName, PRTTRACELOGEVTVAL pVal);


/**
 * Fills the given value array using the values from the given event.
 *
 * @returns IPRT status code
 * @param   hRdrEvt             The reader event handle.
 * @param   idxItemStart        The index of the item to start filling the value in.
 * @param   paVals              Array of values to fill.
 * @param   cVals               Number of values the array is able to hold.
 * @param   pcVals              Where to store the number of values filled on success.
 */
RTDECL(int) RTTraceLogRdrEvtFillVals(RTTRACELOGRDREVT hRdrEvt, unsigned idxItemStart, PRTTRACELOGEVTVAL paVals,
                                     unsigned cVals, unsigned *pcVals);

RT_C_DECLS_END

/** @} */

#endif /* !IPRT_INCLUDED_tracelog_h */

