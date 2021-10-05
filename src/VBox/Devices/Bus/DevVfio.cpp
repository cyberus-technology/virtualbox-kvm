/*
 * Copyright (C) Cyberus Technology GmbH.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define LOG_GROUP LOG_GROUP_DEV_VFIO
#include "DevVfio.h"

#include <VBox/log.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pdmdev.h>

#include <string>

static DECLCALLBACK(int) devVfioConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    /*
     * Check that the device instance and device helper structures are compatible.
     */
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    PVFIODEV pThis {PDMDEVINS_2_DATA(pDevIns, PVFIODEV)};
    PCPDMDEVHLPR3 pHlp {pDevIns->pHlpR3};
    int rc;
    uint16_t bus, device, function;
    char* sysfsPath;

    constexpr char validation[] = "sysfsPath"
                                  "|GuestPCIBusNo"
                                  "|GuestPCIDeviceNo"
                                  "|GuestPCIFunctionNo";

    PDMDEV_VALIDATE_CONFIG_RETURN(pDevIns, validation, "Invalid configuration");
    rc = pHlp->pfnCFGMQueryStringAlloc(pCfg, "sysfsPath", &sysfsPath);
    if (RT_FAILURE(rc))
    {
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Configuration error: Querying sysfsPath as a string failed"));
    }

    std::string sysfsPathString {sysfsPath};
    MMR3HeapFree(sysfsPath);

    rc = pHlp->pfnCFGMQueryU16(pCfg, "GuestPCIBusNo", &bus);
    if (RT_FAILURE(rc))
    {
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Configuration error: Querying GuestPCIBusNo as a uint16_t failed"));
    }

    rc = pHlp->pfnCFGMQueryU16(pCfg, "GuestPCIDeviceNo", &device);
    if (RT_FAILURE(rc))
    {
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Configuration error: Querying GuestPCIDeviceNo as a uint16_t failed"));
    }

    rc = pHlp->pfnCFGMQueryU16(pCfg, "GuestPCIFunctionNo", &function);
    if (RT_FAILURE(rc))
    {
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Configuration error: Querying GuestPCIFunctionNo as a uint16_t failed"));
    }

    LogRel(("VFIO: Constructing VFIO PCI device with path %s Guest BDF: %02hx:%02hx.%hx\n",
            sysfsPathString.c_str(), bus, device, function));

    rc = pThis->init(pDevIns, sysfsPathString);
    AssertLogRelReturn(RT_SUCCESS(rc), rc);


    NOREF(iInstance);

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) devVfioDestruct(PPDMDEVINS pDevIns)
{
    /*
     * Check the versions here as well since the destructor is *always* called.
     */
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);

    PVFIODEV pThis {PDMDEVINS_2_DATA(pDevIns, PVFIODEV)};

    pThis->terminate(pDevIns);

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) devVfioInitComplete(PPDMDEVINS pDevIns)
{
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);

    PVFIODEV pThis {PDMDEVINS_2_DATA(pDevIns, PVFIODEV)};

    return pThis->initializeDma(pDevIns);
}

/**
 * The device registration structure.
 */
extern "C" const PDMDEVREG g_DeviceVfioDev =
{
    /* .u32Version = */             PDM_DEVREG_VERSION,
    /* .uReserved0 = */             0,
    /* .szName = */                 "VfioDev",
    /* .fFlags = */                 PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_NEW_STYLE,

    /* .fClass = */                 PDM_DEVREG_CLASS_HOST_DEV,
    /* .cMaxInstances = */          1,
    /* .uSharedVersion = */         1,
    /* .cbInstanceShared = */       sizeof(VFIODEV),
    /* .cbInstanceR0 = */           0,
    /* .cbInstanceRC = */           0,
    /* .cMaxPciDevices = */         1,
    /* .cMaxMsixVectors = */        0,
    /* .pszDescription = */         "VirtualBox Vfio Passthrough Device\n",
    /* .pszRCMod = */               "",
    /* .pszR0Mod = */               "",
    /* .pfnConstruct = */           devVfioConstruct,
    /* .pfnDestruct = */            devVfioDestruct,
    /* .pfnRelocate = */            NULL,
    /* .pfnMemSetup = */            NULL,
    /* .pfnPowerOn = */             NULL,
    /* .pfnReset = */               NULL,
    /* .pfnSuspend = */             NULL,
    /* .pfnResume = */              NULL,
    /* .pfnAttach = */              NULL,
    /* .pfnDetach = */              NULL,
    /* .pfnQueryInterface. = */     NULL,
    /* .pfnInitComplete = */        devVfioInitComplete,
    /* .pfnPowerOff = */            NULL,
    /* .pfnSoftReset = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
    /* .u32VersionEnd = */          PDM_DEVREG_VERSION
};
