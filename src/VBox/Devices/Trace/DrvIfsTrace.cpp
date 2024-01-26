/* $Id: DrvIfsTrace.cpp $ */
/** @file
 * VBox interface callback tracing driver.
 */

/*
 * Copyright (C) 2020-2023 Oracle and/or its affiliates.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_MISC
#include <VBox/log.h>
#include <VBox/version.h>

#include <iprt/errcore.h>
#include <iprt/buildconfig.h>
#include <iprt/tracelog.h>
#include <iprt/uuid.h>

#include "VBoxDD.h"
#include "DrvIfsTraceInternal.h"


/*
 *
 * IBase Implementation.
 *
 */


static DECLCALLBACK(void *) drvIfTraceIBase_QueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS  pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVIFTRACE pThis   = PDMINS_2_DATA(pDrvIns, PDRVIFTRACE);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    if (pThis->pISerialConBelow)
        PDMIBASE_RETURN_INTERFACE(pszIID, PDMISERIALCONNECTOR, &pThis->ISerialConnector);
    if (pThis->pISerialPortAbove)
        PDMIBASE_RETURN_INTERFACE(pszIID, PDMISERIALPORT, &pThis->ISerialPort);

    return NULL;
}


/*
 *
 * PDMDRVREG Methods
 *
 */

/**
 * Destroys a interface filter driver instance.
 *
 * @copydoc FNPDMDRVDESTRUCT
 */
static DECLCALLBACK(void) drvIfTrace_Destruct(PPDMDRVINS pDrvIns)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);
    PDRVIFTRACE pThis = PDMINS_2_DATA(pDrvIns, PDRVIFTRACE);
    LogFlow(("%s: iInstance=%d\n", __FUNCTION__, pDrvIns->iInstance));

    if (pThis->hTraceLog != NIL_RTTRACELOGWR)
    {
        RTTraceLogWrDestroy(pThis->hTraceLog);
        pThis->hTraceLog = NIL_RTTRACELOGWR;
    }

    if (pThis->pszTraceFilePath)
    {
        PDMDrvHlpMMHeapFree(pDrvIns, pThis->pszTraceFilePath);
        pThis->pszTraceFilePath = NULL;
    }
}


/**
 * Construct a interface filter driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvIfTrace_Construct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVIFTRACE     pThis = PDMINS_2_DATA(pDrvIns, PDRVIFTRACE);
    PCPDMDRVHLPR3   pHlp  = pDrvIns->pHlpR3;


    /*
     * Initialize the instance data.
     */
    pThis->pDrvIns   = pDrvIns;
    pThis->hTraceLog = NIL_RTTRACELOGWR;
    pDrvIns->IBase.pfnQueryInterface             = drvIfTraceIBase_QueryInterface;

    drvIfsTrace_SerialIfInit(pThis);

    /*
     * Validate and read config.
     */
    PDMDRV_VALIDATE_CONFIG_RETURN(pDrvIns, "TraceFilePath|", "");

    int rc = pHlp->pfnCFGMQueryStringAlloc(pCfg, "TraceFilePath", &pThis->pszTraceFilePath);
    AssertLogRelRCReturn(rc, rc);

    /* Try to create a file based trace log. */
    rc = RTTraceLogWrCreateFile(&pThis->hTraceLog, RTBldCfgVersion(), pThis->pszTraceFilePath);
    AssertLogRelRCReturn(rc, rc);

    /*
     * Query interfaces from the driver/device above us.
     */
    pThis->pISerialPortAbove = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMISERIALPORT);

    /*
     * Attach driver below us.
     */
    PPDMIBASE pIBaseBelow;
    rc = PDMDrvHlpAttach(pDrvIns, fFlags, &pIBaseBelow);
    AssertLogRelRCReturn(rc, rc);

    pThis->pISerialConBelow = PDMIBASE_QUERY_INTERFACE(pIBaseBelow, PDMISERIALCONNECTOR);

    return VINF_SUCCESS;
}


/**
 * Storage filter driver registration record.
 */
const PDMDRVREG g_DrvIfTrace =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "IfTrace",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Interface callback tracing driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_STATUS,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVIFTRACE),
    /* pfnConstruct */
    drvIfTrace_Construct,
    /* pfnDestruct */
    drvIfTrace_Destruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};

