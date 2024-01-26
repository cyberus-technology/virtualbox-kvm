/* $Id: DrvTpmEmuTpms.cpp $ */
/** @file
 * TPM emulation driver based on libtpms.
 */

/*
 * Copyright (C) 2021-2023 Oracle and/or its affiliates.
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
#define LOG_GROUP LOG_GROUP_DRV_TPM_EMU
#include <VBox/vmm/pdmdrv.h>
#include <VBox/vmm/pdmtpmifs.h>
#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>
#include <iprt/uuid.h>
#include <iprt/vfs.h>
#include <iprt/zip.h>

#include <libtpms/tpm_library.h>
#include <libtpms/tpm_error.h>
#include <libtpms/tpm_tis.h>
#include <libtpms/tpm_nvfilename.h>

#include <iprt/formats/tpm.h>

#if 0
#include <unistd.h>
#endif
#include <stdlib.h>

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * TPM emulation driver instance data.
 *
 * @implements PDMITPMCONNECTOR
 */
typedef struct DRVTPMEMU
{
    /** The stream interface. */
    PDMITPMCONNECTOR    ITpmConnector;
    /** Pointer to the driver instance. */
    PPDMDRVINS          pDrvIns;
    /** The VFS interface of the driver below for NVRAM/TPM state loading and storing. */
    PPDMIVFSCONNECTOR   pDrvVfs;

    /** The TPM version we are emulating. */
    TPMVERSION          enmVersion;
    /** The buffer size the TPM advertises. */
    uint32_t            cbBuffer;
    /** Currently set locality. */
    uint8_t             bLoc;
} DRVTPMEMU;
/** Pointer to the TPM emulator instance data. */
typedef DRVTPMEMU *PDRVTPMEMU;

/** The special no current locality selected value. */
#define TPM_NO_LOCALITY_SELECTED    0xff


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Pointer to the (only) instance data in this driver. */
static PDRVTPMEMU g_pDrvTpmEmuTpms = NULL;


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/

/* -=-=-=-=- PDMITPMCONNECTOR interface callabcks. -=-=-=-=- */


/** @interface_method_impl{PDMITPMCONNECTOR,pfnGetVersion} */
static DECLCALLBACK(TPMVERSION) drvTpmEmuTpmsGetVersion(PPDMITPMCONNECTOR pInterface)
{
    PDRVTPMEMU pThis = RT_FROM_MEMBER(pInterface, DRVTPMEMU, ITpmConnector);
    return pThis->enmVersion;
}


/** @interface_method_impl{PDMITPMCONNECTOR,pfnGetLocalityMax} */
static DECLCALLBACK(uint32_t) drvTpmEmuGetLocalityMax(PPDMITPMCONNECTOR pInterface)
{
    RT_NOREF(pInterface);
    return 4;
}


/** @interface_method_impl{PDMITPMCONNECTOR,pfnGetBufferSize} */
static DECLCALLBACK(uint32_t) drvTpmEmuGetBufferSize(PPDMITPMCONNECTOR pInterface)
{
    PDRVTPMEMU pThis = RT_FROM_MEMBER(pInterface, DRVTPMEMU, ITpmConnector);
    return pThis->cbBuffer;
}


/** @interface_method_impl{PDMITPMCONNECTOR,pfnGetEstablishedFlag} */
static DECLCALLBACK(bool) drvTpmEmuTpmsGetEstablishedFlag(PPDMITPMCONNECTOR pInterface)
{
    RT_NOREF(pInterface);

    TPM_BOOL fTpmEst = FALSE;
    TPM_RESULT rcTpm = TPM_IO_TpmEstablished_Get(&fTpmEst);
    if (RT_LIKELY(rcTpm == TPM_SUCCESS))
        return RT_BOOL(fTpmEst);

    return false;
}


/** @interface_method_impl{PDMITPMCONNECTOR,pfnResetEstablishedFlag} */
static DECLCALLBACK(int) drvTpmEmuTpmsResetEstablishedFlag(PPDMITPMCONNECTOR pInterface, uint8_t bLoc)
{
    PDRVTPMEMU pThis = RT_FROM_MEMBER(pInterface, DRVTPMEMU, ITpmConnector);
    uint8_t bLocOld = pThis->bLoc;

    pThis->bLoc = bLoc;
    TPM_RESULT rcTpm = TPM_IO_TpmEstablished_Reset();
    pThis->bLoc = bLocOld;

    if (RT_LIKELY(rcTpm == TPM_SUCCESS))
        return VINF_SUCCESS;

    LogRelMax(10, ("DrvTpmEmuTpms#%u: Failed to reset the established flag with %#x\n",
                   pThis->pDrvIns->iInstance, rcTpm));
    return VERR_DEV_IO_ERROR;
}


/** @interface_method_impl{PDMITPMCONNECTOR,pfnCmdExec} */
static DECLCALLBACK(int) drvTpmEmuTpmsCmdExec(PPDMITPMCONNECTOR pInterface, uint8_t bLoc, const void *pvCmd, size_t cbCmd, void *pvResp, size_t cbResp)
{
    PDRVTPMEMU pThis = RT_FROM_MEMBER(pInterface, DRVTPMEMU, ITpmConnector);

    pThis->bLoc = bLoc;

    uint8_t *pbRespBuf = NULL;
    uint32_t cbRespBuf = 0;
    uint32_t cbRespActual = 0;
    TPM_RESULT rcTpm = TPMLIB_Process(&pbRespBuf, &cbRespActual, &cbRespBuf, (uint8_t *)pvCmd, (uint32_t)cbCmd);
    if (RT_LIKELY(rcTpm == TPM_SUCCESS))
    {
        memcpy(pvResp, pbRespBuf, RT_MIN(cbResp, cbRespActual));
        free(pbRespBuf);
        return VINF_SUCCESS;
    }

    LogRelMax(10, ("DrvTpmEmuTpms#%u: Failed to execute command with %#x\n",
                   pThis->pDrvIns->iInstance, rcTpm));
    return VERR_DEV_IO_ERROR;
}


/** @interface_method_impl{PDMITPMCONNECTOR,pfnCmdCancel} */
static DECLCALLBACK(int) drvTpmEmuTpmsCmdCancel(PPDMITPMCONNECTOR pInterface)
{
    PDRVTPMEMU pThis = RT_FROM_MEMBER(pInterface, DRVTPMEMU, ITpmConnector);

    TPM_RESULT rcTpm = TPMLIB_CancelCommand();
    if (RT_LIKELY(rcTpm == TPM_SUCCESS))
        return VINF_SUCCESS;

    LogRelMax(10, ("DrvTpmEmuTpms#%u: Failed to cancel outstanding command with %#x\n",
                   pThis->pDrvIns->iInstance, rcTpm));
    return VERR_DEV_IO_ERROR;
}


/** @interface_method_impl{PDMIBASE,pfnQueryInterface} */
static DECLCALLBACK(void *) drvTpmEmuTpmsQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS      pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVTPMEMU     pThis   = PDMINS_2_DATA(pDrvIns, PDRVTPMEMU);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMITPMCONNECTOR, &pThis->ITpmConnector);
    return NULL;
}


/* -=-=-=-=- libtpms_callbacks -=-=-=-=- */


static DECLCALLBACK(TPM_RESULT) drvTpmEmuTpmsCbkNvRamInit(void)
{
    PDRVTPMEMU pThis = g_pDrvTpmEmuTpms;
    RT_NOREF(pThis);

    return TPM_SUCCESS;
}


static DECLCALLBACK(TPM_RESULT) drvTpmEmuTpmsCbkNvRamLoadData(uint8_t **ppvData, uint32_t *pcbLength,
                                                              uint32_t idTpm, const char *pszName)
{
    PDRVTPMEMU pThis = g_pDrvTpmEmuTpms;

    AssertReturn(idTpm == 0, TPM_FAIL);

    uint64_t cbState = 0;
    int rc = pThis->pDrvVfs->pfnQuerySize(pThis->pDrvVfs, pThis->pDrvIns->pReg->szName, pszName, &cbState);
    if (   RT_SUCCESS(rc)
        && cbState == (uint32_t)cbState)
    {
        void *pvData = malloc(cbState);
        if (RT_LIKELY(pvData))
        {
            rc = pThis->pDrvVfs->pfnReadAll(pThis->pDrvVfs, pThis->pDrvIns->pReg->szName, pszName,
                                            pvData, cbState);
            if (RT_SUCCESS(rc))
            {
                *ppvData = (uint8_t *)pvData;
                *pcbLength = (uint32_t)cbState;
                return VINF_SUCCESS;
            }

            free(pvData);
        }
    }
    else if (rc == VERR_NOT_FOUND)
        return TPM_RETRY; /* This is fine for the first start of a new VM. */

    return TPM_FAIL;
}


static DECLCALLBACK(TPM_RESULT) drvTpmEmuTpmsCbkNvRamStoreData(const uint8_t *pvData, uint32_t cbLength,
                                                               uint32_t idTpm, const char *pszName)
{
    PDRVTPMEMU pThis = g_pDrvTpmEmuTpms;

    AssertReturn(idTpm == 0, TPM_FAIL);

    int rc = pThis->pDrvVfs->pfnWriteAll(pThis->pDrvVfs, pThis->pDrvIns->pReg->szName, pszName,
                                         pvData, cbLength);
    if (RT_SUCCESS(rc))
        return TPM_SUCCESS;

    return TPM_FAIL;
}


static DECLCALLBACK(TPM_RESULT) drvTpmEmuTpmsCbkNvRamDeleteName(uint32_t idTpm, const char *pszName, TPM_BOOL fMustExist)
{
    PDRVTPMEMU pThis = g_pDrvTpmEmuTpms;

    AssertReturn(idTpm == 0, TPM_FAIL);

    int rc = pThis->pDrvVfs->pfnDelete(pThis->pDrvVfs, pThis->pDrvIns->pReg->szName, pszName);
    if (   RT_SUCCESS(rc)
        || (   rc == VERR_NOT_FOUND
            && !fMustExist))
        return TPM_SUCCESS;

    return TPM_FAIL;
}


static DECLCALLBACK(TPM_RESULT) drvTpmEmuTpmsCbkIoInit(void)
{
    return TPM_SUCCESS;
}


static DECLCALLBACK(TPM_RESULT) drvTpmEmuTpmsCbkIoGetLocality(TPM_MODIFIER_INDICATOR *pLocalityModifier, uint32_t idTpm)
{
    PDRVTPMEMU pThis = g_pDrvTpmEmuTpms;

    AssertReturn(idTpm == 0, TPM_FAIL);

    *pLocalityModifier = pThis->bLoc;
    return TPM_SUCCESS;
}


static DECLCALLBACK(TPM_RESULT) drvTpmEmuTpmsCbkIoGetPhysicalPresence(TPM_BOOL *pfPhysicalPresence, uint32_t idTpm)
{
    AssertReturn(idTpm == 0, TPM_FAIL);

    *pfPhysicalPresence = TRUE;
    return TPM_SUCCESS;
}


/* -=-=-=-=- PDMDRVREG -=-=-=-=- */

/**
 * @interface_method_impl{PDMDRVREG,pfnPowerOn}
 */
static DECLCALLBACK(void) drvTpmEmuTpmsPowerOn(PPDMDRVINS pDrvIns)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);

    TPM_RESULT rcTpm = TPMLIB_MainInit();
    if (RT_UNLIKELY(rcTpm != TPM_SUCCESS))
    {
        LogRel(("DrvTpmEmuTpms#%u: Failed to initialize TPM emulation with %#x\n",
                pDrvIns->iInstance, rcTpm));
        PDMDrvHlpVMSetError(pDrvIns, VERR_INVALID_PARAMETER, RT_SRC_POS, "Failed to startup the TPM with %u", rcTpm);
    }
}


/**
 * @interface_method_impl{PDMDRVREG,pfnReset}
 */
static DECLCALLBACK(void) drvTpmEmuTpmsReset(PPDMDRVINS pDrvIns)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);

    TPMLIB_Terminate();
    TPM_RESULT rcTpm = TPMLIB_MainInit();
    if (RT_UNLIKELY(rcTpm != TPM_SUCCESS))
    {
        LogRel(("DrvTpmEmuTpms#%u: Failed to reset TPM emulation with %#x\n",
                pDrvIns->iInstance, rcTpm));
        PDMDrvHlpVMSetError(pDrvIns, VERR_INVALID_PARAMETER, RT_SRC_POS, "Failed to startup the TPM with %u", rcTpm);
    }
}


/**
 * @interface_method_impl{PDMDRVREG,pfnPowerOff}
 */
static DECLCALLBACK(void) drvTpmEmuTpmsPowerOff(PPDMDRVINS pDrvIns)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);

    TPMLIB_Terminate();
}


/** @copydoc FNPDMDRVCONSTRUCT */
static DECLCALLBACK(int) drvTpmEmuTpmsConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVTPMEMU      pThis = PDMINS_2_DATA(pDrvIns, PDRVTPMEMU);
    PCPDMDRVHLPR3   pHlp  = pDrvIns->pHlpR3;

    /*
     * Init the static parts.
     */
    pThis->pDrvIns                                  = pDrvIns;
    pThis->enmVersion                               = TPMVERSION_UNKNOWN;
    pThis->bLoc                                     = TPM_NO_LOCALITY_SELECTED;

    /* IBase */
    pDrvIns->IBase.pfnQueryInterface                = drvTpmEmuTpmsQueryInterface;
    /* ITpmConnector */
    pThis->ITpmConnector.pfnGetVersion              = drvTpmEmuTpmsGetVersion;
    pThis->ITpmConnector.pfnGetLocalityMax          = drvTpmEmuGetLocalityMax;
    pThis->ITpmConnector.pfnGetBufferSize           = drvTpmEmuGetBufferSize;
    pThis->ITpmConnector.pfnGetEstablishedFlag      = drvTpmEmuTpmsGetEstablishedFlag;
    pThis->ITpmConnector.pfnResetEstablishedFlag    = drvTpmEmuTpmsResetEstablishedFlag;
    pThis->ITpmConnector.pfnCmdExec                 = drvTpmEmuTpmsCmdExec;
    pThis->ITpmConnector.pfnCmdCancel               = drvTpmEmuTpmsCmdCancel;

    /*
     * Validate and read the configuration.
     */
    PDMDRV_VALIDATE_CONFIG_RETURN(pDrvIns, "TpmVersion|BufferSize", "");

#if 0
    TPMLIB_SetDebugFD(STDERR_FILENO);
    TPMLIB_SetDebugLevel(~0);
#endif

    /*
     * Try attach the VFS driver below and query it's VFS interface.
     */
    PPDMIBASE pBase = NULL;
    int rc = PDMDrvHlpAttach(pDrvIns, fFlags, &pBase);
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("Failed to attach driver below us! %Rrc"), rc);
    pThis->pDrvVfs = PDMIBASE_QUERY_INTERFACE(pBase, PDMIVFSCONNECTOR);
    if (!pThis->pDrvVfs)
        return PDMDRV_SET_ERROR(pDrvIns, VERR_PDM_MISSING_INTERFACE_BELOW,
                                N_("No VFS interface below"));

    TPMLIB_TPMVersion enmVersion = TPMLIB_TPM_VERSION_2;
    uint32_t uTpmVersion = 0;
    rc = pHlp->pfnCFGMQueryU32Def(pCfg, "TpmVersion", &uTpmVersion, 2);
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("Configuration error: querying \"TpmVersion\" resulted in %Rrc"), rc);

    switch (uTpmVersion)
    {
        case 1:
            enmVersion = TPMLIB_TPM_VERSION_1_2;
            pThis->enmVersion = TPMVERSION_1_2;
            break;
        case 2:
            enmVersion = TPMLIB_TPM_VERSION_2;
            pThis->enmVersion = TPMVERSION_2_0;
            break;
        default:
            return PDMDrvHlpVMSetError(pDrvIns, VERR_NOT_SUPPORTED, RT_SRC_POS,
                                       N_("Configuration error: \"TpmVersion\" %u is not supported"), uTpmVersion);
    }

    TPM_RESULT rcTpm = TPMLIB_ChooseTPMVersion(enmVersion);
    if (rcTpm != TPM_SUCCESS)
        return PDMDrvHlpVMSetError(pDrvIns, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                   N_("Failed to set the TPM version for the emulated TPM with %d"), rcTpm);

    int cbBufferMax = 0;
    rcTpm = TPMLIB_GetTPMProperty(TPMPROP_TPM_BUFFER_MAX, &cbBufferMax);
    if (rcTpm != TPM_SUCCESS)
        return PDMDrvHlpVMSetError(pDrvIns, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                   N_("Querying the maximum supported buffer size failed with %u"), rcTpm);

    rc = pHlp->pfnCFGMQueryU32Def(pCfg, "BufferSize", &pThis->cbBuffer, (uint32_t)cbBufferMax);
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("Configuration error: querying \"BufferSize\" resulted in %Rrc"), rc);

    uint32_t cbBufferMin = 0;
    uint32_t cbBuffer = TPMLIB_SetBufferSize(pThis->cbBuffer, &cbBufferMin, NULL /*max_size*/);
    if (pThis->cbBuffer != cbBuffer)
        return PDMDrvHlpVMSetError(pDrvIns, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                   N_("Failed to set buffer size (%u) of the emulated TPM with %u (min %u, max %d)"),
                                   pThis->cbBuffer, cbBuffer, cbBufferMin, cbBufferMax);

    struct libtpms_callbacks Callbacks;
    Callbacks.sizeOfStruct               = sizeof(Callbacks);
    Callbacks.tpm_nvram_init             = drvTpmEmuTpmsCbkNvRamInit;
    Callbacks.tpm_nvram_loaddata         = drvTpmEmuTpmsCbkNvRamLoadData;
    Callbacks.tpm_nvram_storedata        = drvTpmEmuTpmsCbkNvRamStoreData;
    Callbacks.tpm_nvram_deletename       = drvTpmEmuTpmsCbkNvRamDeleteName;
    Callbacks.tpm_io_init                = drvTpmEmuTpmsCbkIoInit;
    Callbacks.tpm_io_getlocality         = drvTpmEmuTpmsCbkIoGetLocality;
    Callbacks.tpm_io_getphysicalpresence = drvTpmEmuTpmsCbkIoGetPhysicalPresence;
    rcTpm = TPMLIB_RegisterCallbacks(&Callbacks);
    if (rcTpm != TPM_SUCCESS)
        return PDMDrvHlpVMSetError(pDrvIns, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                   N_("Failed to register callbacks with the TPM emulation: %u"),
                                   rcTpm);

    /* We can only have one instance of the TPM emulation and require the global variable for the callbacks unfortunately. */
    g_pDrvTpmEmuTpms = pThis;
    return VINF_SUCCESS;
}


/**
 * TPM libtpms emulator driver registration record.
 */
const PDMDRVREG g_DrvTpmEmuTpms =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "TpmEmuTpms",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "TPM emulation driver based on libtpms.",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_STREAM,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(DRVTPMEMU),
    /* pfnConstruct */
    drvTpmEmuTpmsConstruct,
    /* pfnDestruct */
    NULL,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    drvTpmEmuTpmsPowerOn,
    /* pfnReset */
    drvTpmEmuTpmsReset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnPowerOff */
    drvTpmEmuTpmsPowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};

