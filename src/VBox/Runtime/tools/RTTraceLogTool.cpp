/* $Id: RTTraceLogTool.cpp $ */
/** @file
 * IPRT - Utility for reading/receiving and dissecting trace logs.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/tracelog.h>

#include <iprt/assert.h>
#include <iprt/errcore.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/message.h>
#include <iprt/mem.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/tcp.h>


/**
 * The tracelog tool TCP server/client state.
 */
typedef struct RTTRACELOGTOOLTCP
{
    /** Flag whether this is a server. */
    bool                        fIsServer;
    /** The TCP socket handle for the connection. */
    RTSOCKET                    hSock;
    /** The TCP server. */
    PRTTCPSERVER                pTcpSrv;
} RTTRACELOGTOOLTCP;
/** Pointer to the TCP server/client state. */
typedef RTTRACELOGTOOLTCP *PRTTRACELOGTOOLTCP;


static void rtTraceLogTcpDestroy(PRTTRACELOGTOOLTCP pTrcLogTcp)
{
    if (pTrcLogTcp->fIsServer)
        RTTcpServerDestroy(pTrcLogTcp->pTcpSrv);
    if (pTrcLogTcp->hSock != NIL_RTSOCKET)
    {
        if (pTrcLogTcp->fIsServer)
            RTTcpServerDisconnectClient2(pTrcLogTcp->hSock);
        else
            RTTcpClientClose(pTrcLogTcp->hSock);
    }
    RTMemFree(pTrcLogTcp);
}


static DECLCALLBACK(int) rtTraceLogToolTcpInput(void *pvUser, void *pvBuf, size_t cbBuf, size_t *pcbRead,
                                                RTMSINTERVAL cMsTimeout)
{
    PRTTRACELOGTOOLTCP pTrcLogTcp = (PRTTRACELOGTOOLTCP)pvUser;
    if (   pTrcLogTcp->fIsServer
        && pTrcLogTcp->hSock == NIL_RTSOCKET)
    {
        int rc = RTTcpServerListen2(pTrcLogTcp->pTcpSrv, &pTrcLogTcp->hSock);
        if (RT_FAILURE(rc))
            return rc;
    }

    int rc = RTTcpSelectOne(pTrcLogTcp->hSock, cMsTimeout);
    if (RT_SUCCESS(rc))
        rc = RTTcpReadNB(pTrcLogTcp->hSock, pvBuf, cbBuf, pcbRead);

    return rc;
}


static DECLCALLBACK(int) rtTraceLogToolTcpClose(void *pvUser)
{
    PRTTRACELOGTOOLTCP pTrcLogTcp = (PRTTRACELOGTOOLTCP)pvUser;
    rtTraceLogTcpDestroy(pTrcLogTcp);
    return VINF_SUCCESS;
}


/**
 * Tries to create a new trace log reader using the given input.
 *
 * @returns IPRT status code.
 * @param   phTraceLogRdr       Where to store the handle to the trace log reader instance on success.
 * @param   pszInput            The input path.
 * @param   pszSave             The optional path to save
 */
static int rtTraceLogToolReaderCreate(PRTTRACELOGRDR phTraceLogRdr, const char *pszInput, const char *pszSave)
{
    RT_NOREF(pszSave);

    /* Try treating the input as a file first. */
    int rc = RTTraceLogRdrCreateFromFile(phTraceLogRdr, pszInput);
    if (RT_FAILURE(rc))
    {
        /*
         * Check whether the input looks like a port number or an address:port pair.
         * The former will create a server listening on the port while the latter tries
         * to connect to the given address:port combination.
         */
        uint32_t     uPort     = 0;
        bool         fIsServer = false;
        PRTTCPSERVER pTcpSrv   = NULL;
        RTSOCKET     hSock     = NIL_RTSOCKET;
        rc = RTStrToUInt32Full(pszInput, 10, &uPort);
        if (rc == VINF_SUCCESS)
        {
            fIsServer = true;
            rc = RTTcpServerCreateEx(NULL, uPort, &pTcpSrv);
        }
        else
        {
            /* Try treating the input as an address:port pair. */
        }

        if (RT_SUCCESS(rc))
        {
            /* Initialize structure and reader. */
            PRTTRACELOGTOOLTCP pTrcLogTcp = (PRTTRACELOGTOOLTCP)RTMemAllocZ(sizeof(*pTrcLogTcp));
            if (pTrcLogTcp)
            {
                pTrcLogTcp->fIsServer = fIsServer;
                pTrcLogTcp->hSock     = hSock;
                pTrcLogTcp->pTcpSrv   = pTcpSrv;
                rc = RTTraceLogRdrCreate(phTraceLogRdr, rtTraceLogToolTcpInput, rtTraceLogToolTcpClose, pTrcLogTcp);
                if (RT_FAILURE(rc))
                    rtTraceLogTcpDestroy(pTrcLogTcp);
            }
            else
            {
                if (fIsServer)
                    RTTcpServerDestroy(pTcpSrv);
                else
                    RTSocketClose(hSock);
            }
        }
    }
    return rc;
}


int main(int argc, char **argv)
{
    int rc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(rc))
        return RTMsgInitFailure(rc);

    /*
     * Parse arguments.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--input",    'i', RTGETOPT_REQ_STRING },
        { "--save",     's', RTGETOPT_REQ_STRING },
        { "--help",     'h', RTGETOPT_REQ_NOTHING },
        { "--version",  'V', RTGETOPT_REQ_NOTHING },
    };

    RTEXITCODE      rcExit   = RTEXITCODE_SUCCESS;
    const char     *pszInput = NULL;
    const char     *pszSave  = NULL;

    RTGETOPTUNION   ValueUnion;
    RTGETOPTSTATE   GetState;
    RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, RTGETOPTINIT_FLAGS_OPTS_FIRST);
    while ((rc = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (rc)
        {
            case 'h':
                RTPrintf("Usage: %s [options]\n"
                         "\n"
                         "Options:\n"
                         "  -i,--input=<file|port|address:port>\n"
                         "      Input path, can be a file a port to start listening on for incoming connections or an address:port to connect to\n"
                         "  -s,--save=file\n"
                         "      Save the input to a file for later use\n"
                         "  -h, -?, --help\n"
                         "      Display this help text and exit successfully.\n"
                         "  -V, --version\n"
                         "      Display the revision and exit successfully.\n"
                         , RTPathFilename(argv[0]));
                return RTEXITCODE_SUCCESS;
            case 'V':
                RTPrintf("$Revision: 155244 $\n");
                return RTEXITCODE_SUCCESS;

            case 'i':
                pszInput = ValueUnion.psz;
                break;
            case 's':
                pszSave = ValueUnion.psz;
                break;
            default:
                return RTGetOptPrintError(rc, &ValueUnion);
        }
    }

    if (!pszInput)
    {
        RTPrintf("An input path must be given\n");
        return RTEXITCODE_FAILURE;
    }

    /*
     * Create trace log reader instance.
     */
    RTTRACELOGRDR hTraceLogRdr = NIL_RTTRACELOGRDR;
    rc = rtTraceLogToolReaderCreate(&hTraceLogRdr, pszInput, pszSave);
    if (RT_SUCCESS(rc))
    {
        do
        {
            RTTRACELOGRDRPOLLEVT enmEvt = RTTRACELOGRDRPOLLEVT_INVALID;
            rc = RTTraceLogRdrEvtPoll(hTraceLogRdr, &enmEvt, RT_INDEFINITE_WAIT);
            if (RT_SUCCESS(rc))
            {
                switch (enmEvt)
                {
                    case RTTRACELOGRDRPOLLEVT_HDR_RECVD:
                        RTMsgInfo("A valid header was received\n");
                        break;
                    case RTTRACELOGRDRPOLLEVT_TRACE_EVENT_RECVD:
                    {
                        RTTRACELOGRDREVT hTraceLogEvt;
                        rc = RTTraceLogRdrQueryLastEvt(hTraceLogRdr, &hTraceLogEvt);
                        if (RT_SUCCESS(rc))
                        {
                            PCRTTRACELOGEVTDESC pEvtDesc = RTTraceLogRdrEvtGetDesc(hTraceLogEvt);
                            RTMsgInfo("%llu        %llu        %s\n",
                                      RTTraceLogRdrEvtGetSeqNo(hTraceLogEvt),
                                      RTTraceLogRdrEvtGetTs(hTraceLogEvt),
                                      pEvtDesc->pszId);
                            for (unsigned i = 0; i < pEvtDesc->cEvtItems; i++)
                            {
                                RTTRACELOGEVTVAL Val;
                                unsigned cVals = 0;
                                rc = RTTraceLogRdrEvtFillVals(hTraceLogEvt, i, &Val, 1, &cVals);
                                if (RT_SUCCESS(rc))
                                {
                                    switch (Val.pItemDesc->enmType)
                                    {
                                        case RTTRACELOGTYPE_BOOL:
                                            RTMsgInfo("    %s: %s\n", Val.pItemDesc->pszName, Val.u.f ? "true" : "false");
                                            break;
                                        case RTTRACELOGTYPE_UINT8:
                                            RTMsgInfo("    %s: %u\n", Val.pItemDesc->pszName, Val.u.u8);
                                            break;
                                        case RTTRACELOGTYPE_INT8:
                                            RTMsgInfo("    %s: %d\n", Val.pItemDesc->pszName, Val.u.i8);
                                            break;
                                        case RTTRACELOGTYPE_UINT16:
                                            RTMsgInfo("    %s: %u\n", Val.pItemDesc->pszName, Val.u.u16);
                                            break;
                                        case RTTRACELOGTYPE_INT16:
                                            RTMsgInfo("    %s: %d\n", Val.pItemDesc->pszName, Val.u.i16);
                                            break;
                                        case RTTRACELOGTYPE_UINT32:
                                            RTMsgInfo("    %s: %u\n", Val.pItemDesc->pszName, Val.u.u32);
                                            break;
                                        case RTTRACELOGTYPE_INT32:
                                            RTMsgInfo("    %s: %d\n", Val.pItemDesc->pszName, Val.u.i32);
                                            break;
                                        case RTTRACELOGTYPE_UINT64:
                                            RTMsgInfo("    %s: %llu\n", Val.pItemDesc->pszName, Val.u.u64);
                                            break;
                                        case RTTRACELOGTYPE_INT64:
                                            RTMsgInfo("    %s: %lld\n", Val.pItemDesc->pszName, Val.u.i64);
                                            break;
                                        case RTTRACELOGTYPE_RAWDATA:
                                            RTMsgInfo("    %s:\n"
                                                      "%.*Rhxd\n", Val.pItemDesc->pszName, Val.u.RawData.cb, Val.u.RawData.pb);
                                            break;
                                        case RTTRACELOGTYPE_FLOAT32:
                                        case RTTRACELOGTYPE_FLOAT64:
                                            RTMsgInfo("    %s: Float32 and Float64 data not supported yet\n", Val.pItemDesc->pszName);
                                            break;
                                        case RTTRACELOGTYPE_POINTER:
                                            RTMsgInfo("    %s: %#llx\n", Val.pItemDesc->pszName, Val.u.uPtr);
                                            break;
                                        case RTTRACELOGTYPE_SIZE:
                                            RTMsgInfo("    %s: %llu\n", Val.pItemDesc->pszName, Val.u.sz);
                                            break;
                                        default:
                                            RTMsgError("    %s: Invalid type given %d\n", Val.pItemDesc->pszName, Val.pItemDesc->enmType);
                                    }
                                }
                                else
                                    RTMsgInfo("    Failed to retrieve event data with %Rrc\n", rc);
                            }
                        }
                        break;
                    }
                    default:
                        RTMsgInfo("Invalid event received: %d\n", enmEvt);
                }
            }
            else
                rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Polling for an event failed with %Rrc\n", rc);
        } while (RT_SUCCESS(rc));

        RTTraceLogRdrDestroy(hTraceLogRdr);
    }
    else
        rcExit = RTMsgErrorExit(RTEXITCODE_FAILURE, "Failed to create trace log reader with %Rrc\n", rc);

    return rcExit;
}

