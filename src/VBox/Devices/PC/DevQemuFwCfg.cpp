/* $Id: DevQemuFwCfg.cpp $ */
/** @file
 * DevQemuFwCfg - QEMU firmware configuration compatible device.
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

/** @page pg_qemufwcfg   The QEMU firmware configuration Device.
 *
 * The QEMU firmware configuration device is a custom device emulation
 * to convey information about the VM to the guests firmware (UEFI for example).
 * In the case of VirtualBox it is used to directly load a compatible kernel
 * and initrd image like Linux from the host into the guest and boot it. This allows
 * efficiently testing/debugging of multiple Linux kernels without having to install
 * a guest OS. On VirtualBox the EFI firmware supports this interface, the BIOS is
 * currently unsupported (and probably never will be).
 *
 * @section sec_qemufwcfg_config    Configuration
 *
 * To use this interface for a particular VM the following extra data needs to be
 * set besides enabling the EFI firmware:
 *
 *     VBoxManage setextradata <VM name> "VBoxInternal/Devices/qemu-fw-cfg/0/Config/KernelImage" /path/to/kernel
 *     VBoxManage setextradata <VM name> "VBoxInternal/Devices/qemu-fw-cfg/0/Config/InitrdImage" /path/to/initrd
 *     VBoxManage setextradata <VM name> "VBoxInternal/Devices/qemu-fw-cfg/0/Config/CmdLine"     "<cmd line string>"
 *
 * The only mandatory item is the KernelImage one, the others are optional if the
 * kernel is configured to not require it. If the kernel is not an EFI compatible
 * executable (CONFIG_EFI_STUB=y for Linux) a dedicated setup image might be required
 * which can be set with:
 *
 *     VBoxManage setextradata <VM name> "VBoxInternal/Devices/qemu-fw-cfg/0/Config/SetupImage" /path/to/setup_image
 *
 * @section sec_qemufwcfg_dma    DMA
 *
 * The QEMU firmware configuration device supports an optional DMA interface to speed up transferring the data into the guest.
 * It currently is not enabled by default but needs to be enabled with:
 *
 *     VBoxManage setextradata <VM name> "VBoxInternal/Devices/qemu-fw-cfg/0/Config/DmaEnabled" 1
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_QEMUFWCFG
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pgm.h>
#include <VBox/log.h>
#include <iprt/errcore.h>
#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/string.h>
#include <iprt/vfs.h>
#include <iprt/zero.h>

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/

/** Start of the I/O port region. */
#define QEMU_FW_CFG_IO_PORT_START                   0x510
/** Number of I/O ports reserved for this device. */
#define QEMU_FW_CFG_IO_PORT_SIZE                    12
/** Offset of the config item selector register from the start. */
#define QEMU_FW_CFG_OFF_SELECTOR                    0
/** Offset of the data port from the start. */
#define QEMU_FW_CFG_OFF_DATA                        1
/** Offset of the high 32bit of the DMA address. */
#define QEMU_FW_CFG_OFF_DMA_HIGH                    4
/** Offset of the low 32bit of the DMA address. */
#define QEMU_FW_CFG_OFF_DMA_LOW                     8


/** Set if legacy interface is supported (always set).*/
#define QEMU_FW_CFG_VERSION_LEGACY                  RT_BIT_32(0)
/** Set if DMA is supported.*/
#define QEMU_FW_CFG_VERSION_DMA                     RT_BIT_32(1)


/** Error happened during the DMA access. */
#define QEMU_FW_CFG_DMA_ERROR                       RT_BIT_32(0)
/** Read requested. */
#define QEMU_FW_CFG_DMA_READ                        RT_BIT_32(1)
/** Skipping bytes requested. */
#define QEMU_FW_CFG_DMA_SKIP                        RT_BIT_32(2)
/** The config item is selected. */
#define QEMU_FW_CFG_DMA_SELECT                      RT_BIT_32(3)
/** Write requested. */
#define QEMU_FW_CFG_DMA_WRITE                       RT_BIT_32(4)
/** Extracts the selected config item. */
#define QEMU_FW_CFG_DMA_GET_CFG_ITEM(a_Control)     ((uint16_t)((a_Control) >> 16))


/** @name Known config items.
 * @{ */
#define QEMU_FW_CFG_ITEM_SIGNATURE                  UINT16_C(0x0000)
#define QEMU_FW_CFG_ITEM_VERSION                    UINT16_C(0x0001)
#define QEMU_FW_CFG_ITEM_SYSTEM_UUID                UINT16_C(0x0002)
#define QEMU_FW_CFG_ITEM_RAM_SIZE                   UINT16_C(0x0003)
#define QEMU_FW_CFG_ITEM_GRAPHICS_ENABLED           UINT16_C(0x0004)
#define QEMU_FW_CFG_ITEM_SMP_CPU_COUNT              UINT16_C(0x0005)
#define QEMU_FW_CFG_ITEM_MACHINE_ID                 UINT16_C(0x0006)
#define QEMU_FW_CFG_ITEM_KERNEL_ADDRESS             UINT16_C(0x0007)
#define QEMU_FW_CFG_ITEM_KERNEL_SIZE                UINT16_C(0x0008)
#define QEMU_FW_CFG_ITEM_KERNEL_CMD_LINE            UINT16_C(0x0009)
#define QEMU_FW_CFG_ITEM_INITRD_ADDRESS             UINT16_C(0x000a)
#define QEMU_FW_CFG_ITEM_INITRD_SIZE                UINT16_C(0x000b)
#define QEMU_FW_CFG_ITEM_BOOT_DEVICE                UINT16_C(0x000c)
#define QEMU_FW_CFG_ITEM_NUMA_DATA                  UINT16_C(0x000d)
#define QEMU_FW_CFG_ITEM_BOOT_MENU                  UINT16_C(0x000e)
#define QEMU_FW_CFG_ITEM_MAX_CPU_COUNT              UINT16_C(0x000f)
#define QEMU_FW_CFG_ITEM_KERNEL_ENTRY               UINT16_C(0x0010)
#define QEMU_FW_CFG_ITEM_KERNEL_DATA                UINT16_C(0x0011)
#define QEMU_FW_CFG_ITEM_INITRD_DATA                UINT16_C(0x0012)
#define QEMU_FW_CFG_ITEM_CMD_LINE_ADDRESS           UINT16_C(0x0013)
#define QEMU_FW_CFG_ITEM_CMD_LINE_SIZE              UINT16_C(0x0014)
#define QEMU_FW_CFG_ITEM_CMD_LINE_DATA              UINT16_C(0x0015)
#define QEMU_FW_CFG_ITEM_KERNEL_SETUP_ADDRESS       UINT16_C(0x0016)
#define QEMU_FW_CFG_ITEM_KERNEL_SETUP_SIZE          UINT16_C(0x0017)
#define QEMU_FW_CFG_ITEM_KERNEL_SETUP_DATA          UINT16_C(0x0018)
#define QEMU_FW_CFG_ITEM_FILE_DIR                   UINT16_C(0x0019)
/** @} */


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * QEMU firmware config DMA descriptor.
 */
typedef struct QEMUFWDMADESC
{
    /** Control field. */
    uint32_t                    u32Ctrl;
    /** Length of the transfer in bytes. */
    uint32_t                    u32Length;
    /** Address of the buffer to transfer from/to. */
    uint64_t                    u64GCPhysBuf;
} QEMUFWDMADESC;
AssertCompileSize(QEMUFWDMADESC, 2 * 4 + 8);
/** Pointer to a QEMU firmware config DMA descriptor. */
typedef QEMUFWDMADESC *PQEMUFWDMADESC;
/** Pointer to a const QEMU firmware config DMA descriptor. */
typedef const QEMUFWDMADESC *PCQEMUFWDMADESC;


/** Pointer to a const configuration item descriptor. */
typedef const struct QEMUFWCFGITEM *PCQEMUFWCFGITEM;

/**
 * QEMU firmware config instance data structure.
 */
typedef struct DEVQEMUFWCFG
{
    /** Pointer back to the device instance. */
    PPDMDEVINS                  pDevIns;
    /** The configuration handle. */
    PCFGMNODE                   pCfg;
    /** Pointer to the currently selected item. */
    PCQEMUFWCFGITEM             pCfgItem;
    /** Offset of the next byte to read from the start of the data item. */
    uint32_t                    offCfgItemNext;
    /** How many bytes are left for transfer. */
    uint32_t                    cbCfgItemLeft;
    /** Version register. */
    uint32_t                    u32Version;
    /** Guest physical address of the DMA descriptor. */
    RTGCPHYS                    GCPhysDma;

    /** Scratch buffer for config item specific data. */
    union
    {
        uint8_t                 u8;
        uint16_t                u16;
        uint32_t                u32;
        uint64_t                u64;
        /** VFS file handle. */
        RTVFSFILE               hVfsFile;
        /** Byte view. */
        uint8_t                 ab[8];
    } u;
} DEVQEMUFWCFG;
/** Pointer to the QEMU firmware config device instance. */
typedef DEVQEMUFWCFG *PDEVQEMUFWCFG;


/**
 * A supported configuration item descriptor.
 */
typedef struct QEMUFWCFGITEM
{
    /** The config tiem value. */
    uint16_t                    uCfgItem;
    /** Name of the item. */
    const char                  *pszItem;
    /** Optional CFGM key to lookup the content. */
    const char                  *pszCfgmKey;
    /**
     * Setup callback for when the guest writes the selector.
     *
     * @returns VBox status code.
     * @param   pThis           The QEMU fw config device instance.
     * @param   pItem           Pointer to the selected item.
     * @param   pcbItem         Where to store the size of the item on success.
     */
    DECLCALLBACKMEMBER(int, pfnSetup, (PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem, uint32_t *pcbItem));
    /**
     * Read callback to return the data.
     *
     * @returns VBox status code.
     * @param   pThis           The QEMU fw config device instance.
     * @param   pItem           Pointer to the selected item.
     * @param   off             Where to start reading from.
     * @param   pvBuf           Where to store the read data.
     * @param   cbToRead        How much to read.
     * @param   pcbRead         Where to store the amount of bytes read.
     */
    DECLCALLBACKMEMBER(int, pfnRead, (PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem, uint32_t off, void *pvBuf,
                                      uint32_t cbToRead, uint32_t *pcbRead));

    /**
     * Cleans up any allocated resources when the item is de-selected.
     *
     * @param   pThis           The QEMU fw config device instance.
     * @param   pItem           Pointer to the selected item.
     */
    DECLCALLBACKMEMBER(void, pfnCleanup, (PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem));
} QEMUFWCFGITEM;
/** Pointer to a configuration item descriptor. */
typedef QEMUFWCFGITEM *PQEMUFWCFGITEM;



/**
 * @interface_method_impl{QEMUFWCFGITEM,pfnSetup, Sets up the data for the signature configuration item.}
 */
static DECLCALLBACK(int) qemuFwCfgR3SetupSignature(PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem, uint32_t *pcbItem)
{
    RT_NOREF(pThis, pItem);
    uint8_t abSig[] = { 'Q', 'E', 'M', 'U' };
    memcpy(&pThis->u.ab[0], &abSig[0], sizeof(abSig));
    *pcbItem = sizeof(abSig);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{QEMUFWCFGITEM,pfnSetup, Sets up the data for the version configuration item.}
 */
static DECLCALLBACK(int) qemuFwCfgR3SetupVersion(PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem, uint32_t *pcbItem)
{
    RT_NOREF(pThis, pItem);
    memcpy(&pThis->u.ab[0], &pThis->u32Version, sizeof(pThis->u32Version));
    *pcbItem = sizeof(pThis->u32Version);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{QEMUFWCFGITEM,pfnSetup, Sets up the data for the file directory configuration item.}
 */
static DECLCALLBACK(int) qemuFwCfgR3SetupFileDir(PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem, uint32_t *pcbItem)
{
    RT_NOREF(pThis, pItem);
    memset(&pThis->u.ab[0], 0, sizeof(uint32_t)); /** @todo Implement */
    *pcbItem = sizeof(uint32_t);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{QEMUFWCFGITEM,pfnSetup, Sets up the data for the size config item belonging to a VFS file type configuration item.}
 */
static DECLCALLBACK(int) qemuFwCfgR3SetupCfgmFileSz(PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem, uint32_t *pcbItem)
{
    PCPDMDEVHLPR3 pHlp  = pThis->pDevIns->pHlpR3;

    /* Query the path from the CFGM key. */
    char *pszFilePath = NULL;
    int rc = pHlp->pfnCFGMQueryStringAlloc(pThis->pCfg, pItem->pszCfgmKey, &pszFilePath);
    if (RT_SUCCESS(rc))
    {
        RTVFSFILE hVfsFile = NIL_RTVFSFILE;
        rc = RTVfsFileOpenNormal(pszFilePath, RTFILE_O_READ | RTFILE_O_DENY_NONE | RTFILE_O_OPEN, &hVfsFile);
        if (RT_SUCCESS(rc))
        {
            uint64_t cbFile = 0;
            rc = RTVfsFileQuerySize(hVfsFile, &cbFile);
            if (RT_SUCCESS(rc))
            {
                if (cbFile < _4G)
                {
                    pThis->u.u32 = (uint32_t)cbFile;
                    *pcbItem = sizeof(uint32_t);
                }
                else
                {
                    rc = VERR_BUFFER_OVERFLOW;
                    LogRel(("QemuFwCfg: File \"%s\" exceeds 4G limit (%llu)\n", pszFilePath, cbFile));
                }
            }
            else
                LogRel(("QemuFwCfg: Failed to query file size from \"%s\" -> %Rrc\n", pszFilePath, rc));
            RTVfsFileRelease(hVfsFile);
        }
        else
            LogRel(("QemuFwCfg: Failed to open file \"%s\" -> %Rrc\n", pszFilePath, rc));
        PDMDevHlpMMHeapFree(pThis->pDevIns, pszFilePath);
    }
    else
        LogRel(("QemuFwCfg: Failed to query \"%s\" -> %Rrc\n", pItem->pszCfgmKey, rc));

    return rc;
}


/**
 * @interface_method_impl{QEMUFWCFGITEM,pfnSetup, Sets up the data for the size config item belonging to a string type configuration item.}
 */
static DECLCALLBACK(int) qemuFwCfgR3SetupCfgmStrSz(PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem, uint32_t *pcbItem)
{
    PCPDMDEVHLPR3 pHlp  = pThis->pDevIns->pHlpR3;

    /* Query the string from the CFGM key. */
    char sz[_4K];
    int rc = pHlp->pfnCFGMQueryString(pThis->pCfg, pItem->pszCfgmKey, &sz[0], sizeof(sz));
    if (RT_SUCCESS(rc))
    {
        pThis->u.u32 = (uint32_t)strlen(&sz[0]) + 1;
        *pcbItem = sizeof(uint32_t);
    }
    else
        LogRel(("QemuFwCfg: Failed to query \"%s\" -> %Rrc\n", pItem->pszCfgmKey, rc));

    return rc;
}


/**
 * @interface_method_impl{QEMUFWCFGITEM,pfnSetup, Sets up the data for a string type configuration item gathered from CFGM.}
 */
static DECLCALLBACK(int) qemuFwCfgR3SetupCfgmStr(PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem, uint32_t *pcbItem)
{
    PCPDMDEVHLPR3 pHlp  = pThis->pDevIns->pHlpR3;

    /* Query the string from the CFGM key. */
    char sz[_4K];
    int rc = pHlp->pfnCFGMQueryString(pThis->pCfg, pItem->pszCfgmKey, &sz[0], sizeof(sz));
    if (RT_SUCCESS(rc))
        *pcbItem = (uint32_t)strlen(&sz[0]) + 1;
    else
        LogRel(("QemuFwCfg: Failed to query \"%s\" -> %Rrc\n", pItem->pszCfgmKey, rc));

    return rc;
}


/**
 * @interface_method_impl{QEMUFWCFGITEM,pfnSetup, Sets up the data for a VFS file type configuration item.}
 */
static DECLCALLBACK(int) qemuFwCfgR3SetupCfgmFile(PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem, uint32_t *pcbItem)
{
    PCPDMDEVHLPR3 pHlp  = pThis->pDevIns->pHlpR3;

    /* Query the path from the CFGM key. */
    char *pszFilePath = NULL;
    int rc = pHlp->pfnCFGMQueryStringAlloc(pThis->pCfg, pItem->pszCfgmKey, &pszFilePath);
    if (RT_SUCCESS(rc))
    {
        rc = RTVfsFileOpenNormal(pszFilePath, RTFILE_O_READ | RTFILE_O_DENY_NONE | RTFILE_O_OPEN, &pThis->u.hVfsFile);
        if (RT_SUCCESS(rc))
        {
            uint64_t cbFile = 0;
            rc = RTVfsFileQuerySize(pThis->u.hVfsFile, &cbFile);
            if (RT_SUCCESS(rc))
            {
                if (cbFile < _4G)
                    *pcbItem = (uint32_t)cbFile;
                else
                {
                    rc = VERR_BUFFER_OVERFLOW;
                    LogRel(("QemuFwCfg: File \"%s\" exceeds 4G limit (%llu)\n", pszFilePath, cbFile));
                }
            }
            else
                LogRel(("QemuFwCfg: Failed to query file size from \"%s\" -> %Rrc\n", pszFilePath, rc));
        }
        else
            LogRel(("QemuFwCfg: Failed to open file \"%s\" -> %Rrc\n", pszFilePath, rc));
        PDMDevHlpMMHeapFree(pThis->pDevIns, pszFilePath);
    }
    else
        LogRel(("QemuFwCfg: Failed to query \"%s\" -> %Rrc\n", pItem->pszCfgmKey, rc));

    return rc;
}


/**
 * @interface_method_impl{QEMUFWCFGITEM,pfnRead, Reads data from a configuration item having its data stored in the scratch buffer.}
 */
static DECLCALLBACK(int) qemuFwCfgR3ReadSimple(PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem, uint32_t off, void *pvBuf,
                                               uint32_t cbToRead, uint32_t *pcbRead)
{
    RT_NOREF(pThis, pItem);
    memcpy(pvBuf, &pThis->u.ab[off], cbToRead);
    *pcbRead = cbToRead;
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{QEMUFWCFGITEM,pfnRead, Reads data from a VFS file type configuration item.}
 */
static DECLCALLBACK(int) qemuFwCfgR3ReadVfsFile(PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem, uint32_t off, void *pvBuf,
                                                uint32_t cbToRead, uint32_t *pcbRead)
{
    RT_NOREF(pItem);
    size_t cbRead = 0;
    int rc = RTVfsFileReadAt(pThis->u.hVfsFile, off, pvBuf, cbToRead, &cbRead);
    if (RT_SUCCESS(rc))
        *pcbRead = (uint32_t)cbRead;

    return rc;
}


/**
 * @interface_method_impl{QEMUFWCFGITEM,pfnRead, Reads a string item gathered from CFGM.}
 */
static DECLCALLBACK(int) qemuFwCfgR3ReadStr(PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem, uint32_t off, void *pvBuf,
                                            uint32_t cbToRead, uint32_t *pcbRead)
{
    PCPDMDEVHLPR3 pHlp  = pThis->pDevIns->pHlpR3;

    /* Query the string from the CFGM key. */
    char sz[_4K];
    int rc = pHlp->pfnCFGMQueryString(pThis->pCfg, pItem->pszCfgmKey, &sz[0], sizeof(sz));
    if (RT_SUCCESS(rc))
    {
        uint32_t cch = (uint32_t)strlen(sz) + 1;
        if (off < cch)
        {
            uint32_t cbRead = RT_MIN(cbToRead, off - cch);
            memcpy(pvBuf, &sz[off], cbRead);
            *pcbRead = cbRead;
        }
        else
            rc = VERR_BUFFER_OVERFLOW;
    }
    else
        LogRel(("QemuFwCfg: Failed to query \"%s\" -> %Rrc\n", pItem->pszCfgmKey, rc));

    return rc;
}


/**
 * @interface_method_impl{QEMUFWCFGITEM,pfnCleanup, Cleans up a VFS file type configuration item.}
 */
static DECLCALLBACK(void) qemuFwCfgR3CleanupVfsFile(PDEVQEMUFWCFG pThis, PCQEMUFWCFGITEM pItem)
{
    RT_NOREF(pItem);
    RTVfsFileRelease(pThis->u.hVfsFile);
    pThis->u.hVfsFile = NIL_RTVFSFILE;
}


/**
 * Supported config items.
 */
static const QEMUFWCFGITEM g_aQemuFwCfgItems[] =
{
    /** u16Selector                         pszItem         pszCfgmKey           pfnSetup                    pfnRead                         pfnCleanup */
    { QEMU_FW_CFG_ITEM_SIGNATURE,           "Signature",    NULL,                qemuFwCfgR3SetupSignature,  qemuFwCfgR3ReadSimple,          NULL                      },
    { QEMU_FW_CFG_ITEM_VERSION,             "Version",      NULL,                qemuFwCfgR3SetupVersion,    qemuFwCfgR3ReadSimple,          NULL                      },
    { QEMU_FW_CFG_ITEM_KERNEL_SIZE,         "KrnlSz",       "KernelImage",       qemuFwCfgR3SetupCfgmFileSz, qemuFwCfgR3ReadSimple,          NULL                      },
    { QEMU_FW_CFG_ITEM_KERNEL_DATA,         "KrnlDat",      "KernelImage",       qemuFwCfgR3SetupCfgmFile,   qemuFwCfgR3ReadVfsFile,         qemuFwCfgR3CleanupVfsFile },
    { QEMU_FW_CFG_ITEM_INITRD_SIZE,         "InitrdSz",     "InitrdImage",       qemuFwCfgR3SetupCfgmFileSz, qemuFwCfgR3ReadSimple,          NULL                      },
    { QEMU_FW_CFG_ITEM_KERNEL_DATA,         "InitrdDat",    "InitrdImage",       qemuFwCfgR3SetupCfgmFile,   qemuFwCfgR3ReadVfsFile,         qemuFwCfgR3CleanupVfsFile },
    { QEMU_FW_CFG_ITEM_KERNEL_SETUP_SIZE,   "SetupSz",      "SetupImage",        qemuFwCfgR3SetupCfgmFileSz, qemuFwCfgR3ReadSimple,          NULL                      },
    { QEMU_FW_CFG_ITEM_KERNEL_SETUP_DATA,   "SetupDat",     "SetupImage",        qemuFwCfgR3SetupCfgmFile,   qemuFwCfgR3ReadVfsFile,         qemuFwCfgR3CleanupVfsFile },
    { QEMU_FW_CFG_ITEM_CMD_LINE_SIZE,       "CmdLineSz",    "CmdLine",           qemuFwCfgR3SetupCfgmStrSz,  qemuFwCfgR3ReadSimple,          NULL                      },
    { QEMU_FW_CFG_ITEM_CMD_LINE_DATA,       "CmdLineDat",   "CmdLine",           qemuFwCfgR3SetupCfgmStr,    qemuFwCfgR3ReadStr,             NULL                      },
    { QEMU_FW_CFG_ITEM_FILE_DIR,            "FileDir",      NULL,                qemuFwCfgR3SetupFileDir,    qemuFwCfgR3ReadSimple,          NULL                      }
};


/**
 * Resets the currently selected item.
 *
 * @param   pThis               The QEMU fw config device instance.
 */
static void qemuFwCfgR3ItemReset(PDEVQEMUFWCFG pThis)
{
    if (   pThis->pCfgItem
        && pThis->pCfgItem->pfnCleanup)
        pThis->pCfgItem->pfnCleanup(pThis, pThis->pCfgItem);

    pThis->pCfgItem       = NULL;
    pThis->offCfgItemNext = 0;
    pThis->cbCfgItemLeft  = 0;
}


/**
 * Selects the given config item.
 *
 * @returns VBox status code.
 * @param   pThis               The QEMU fw config device instance.
 * @param   uCfgItem            The configuration item to select.
 */
static int qemuFwCfgItemSelect(PDEVQEMUFWCFG pThis, uint16_t uCfgItem)
{
    qemuFwCfgR3ItemReset(pThis);

    for (uint32_t i = 0; i < RT_ELEMENTS(g_aQemuFwCfgItems); i++)
    {
        PCQEMUFWCFGITEM pCfgItem = &g_aQemuFwCfgItems[i];

        if (pCfgItem->uCfgItem == uCfgItem)
        {
            uint32_t cbItem = 0;
            int rc = pCfgItem->pfnSetup(pThis, pCfgItem, &cbItem);
            if (RT_SUCCESS(rc))
            {
                pThis->pCfgItem      = pCfgItem;
                pThis->cbCfgItemLeft = cbItem;
                return VINF_SUCCESS;
            }

            return rc;
        }
    }

    return VERR_NOT_FOUND;
}


/**
 * Processes a DMA transfer.
 *
 * @param   pThis               The QEMU fw config device instance.
 * @param   GCPhysDma           The guest physical address of the DMA descriptor.
 */
static void qemuFwCfgDmaXfer(PDEVQEMUFWCFG pThis, RTGCPHYS GCPhysDma)
{
    QEMUFWDMADESC DmaDesc; RT_ZERO(DmaDesc);

    LogFlowFunc(("pThis=%p GCPhysDma=%RGp\n", pThis, GCPhysDma));

    PDMDevHlpPhysReadMeta(pThis->pDevIns, GCPhysDma, &DmaDesc, sizeof(DmaDesc));

    /* Convert from big endianess to host endianess. */
    DmaDesc.u32Ctrl      = RT_BE2H_U32(DmaDesc.u32Ctrl);
    DmaDesc.u32Length    = RT_BE2H_U32(DmaDesc.u32Length);
    DmaDesc.u64GCPhysBuf = RT_BE2H_U64(DmaDesc.u64GCPhysBuf);

    LogFlowFunc(("u32Ctrl=%#x u32Length=%u u64GCPhysBuf=%llx\n",
                 DmaDesc.u32Ctrl, DmaDesc.u32Length, DmaDesc.u64GCPhysBuf));

    /* If the select bit is set a select is performed. */
    int rc = VINF_SUCCESS;
    if (DmaDesc.u32Ctrl & QEMU_FW_CFG_DMA_SELECT)
        rc = qemuFwCfgItemSelect(pThis, QEMU_FW_CFG_DMA_GET_CFG_ITEM(DmaDesc.u32Ctrl));

    if (RT_SUCCESS(rc))
    {
        /* We don't support any writes right now. */
        if (DmaDesc.u32Ctrl & QEMU_FW_CFG_DMA_WRITE)
            rc = VERR_INVALID_PARAMETER;
        else if (   !pThis->pCfgItem
                 || !pThis->cbCfgItemLeft)
        {
            if (DmaDesc.u32Ctrl & QEMU_FW_CFG_DMA_READ)
            {
                /* Item is not supported, just zero out the indicated area. */
                RTGCPHYS GCPhysCur = DmaDesc.u64GCPhysBuf;
                uint32_t cbLeft = DmaDesc.u32Length;

                while (   RT_SUCCESS(rc)
                       && cbLeft)
                {
                    uint32_t cbZero = RT_MIN(_64K, cbLeft);

                    PDMDevHlpPhysWriteMeta(pThis->pDevIns, GCPhysCur, &g_abRTZero64K[0], cbZero);

                    cbLeft    -= cbZero;
                    GCPhysCur += cbZero;
                }
            }
            /* else: Assume Skip */
        }
        else
        {
            /* Read or skip. */
            RTGCPHYS GCPhysCur = DmaDesc.u64GCPhysBuf;
            uint32_t cbLeft = RT_MIN(DmaDesc.u32Length, pThis->cbCfgItemLeft);

            while (   RT_SUCCESS(rc)
                   && cbLeft)
            {
                uint8_t abTmp[_1K];
                uint32_t cbThisRead = RT_MIN(sizeof(abTmp), cbLeft);
                uint32_t cbRead;

                rc = pThis->pCfgItem->pfnRead(pThis, pThis->pCfgItem, pThis->offCfgItemNext, &abTmp[0],
                                               cbThisRead, &cbRead);
                if (RT_SUCCESS(rc))
                {
                    if (DmaDesc.u32Ctrl & QEMU_FW_CFG_DMA_READ)
                        PDMDevHlpPhysWriteMeta(pThis->pDevIns, GCPhysCur, &abTmp[0], cbRead);
                    /* else: Assume Skip */

                    cbLeft    -= cbRead;
                    GCPhysCur += cbRead;

                    pThis->offCfgItemNext += cbRead;
                    pThis->cbCfgItemLeft  -= cbRead;
                }
            }
        }
    }

    LogFlowFunc(("pThis=%p GCPhysDma=%RGp -> %Rrc\n", pThis, GCPhysDma, rc));

    /* Write back the control field. */
    uint32_t u32Resp = RT_SUCCESS(rc) ? 0 : RT_H2BE_U32(QEMU_FW_CFG_DMA_ERROR);
    PDMDevHlpPhysWriteMeta(pThis->pDevIns, GCPhysDma, &u32Resp, sizeof(u32Resp));
}


/**
 * @callback_method_impl{FNIOMIOPORTNEWOUT, QEMU firmware configuration write.}
 */
static DECLCALLBACK(VBOXSTRICTRC) qemuFwCfgIoPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort, uint32_t u32, unsigned cb)
{
    int rc = VINF_SUCCESS;
    PDEVQEMUFWCFG pThis = PDMDEVINS_2_DATA(pDevIns, PDEVQEMUFWCFG);
    NOREF(pvUser);

    LogFlowFunc(("offPort=%RTiop u32=%#x cb=%u\n", offPort, u32, cb));

    switch (offPort)
    {
        case QEMU_FW_CFG_OFF_SELECTOR:
        {
            if (cb == 2)
                qemuFwCfgItemSelect(pThis, (uint16_t)u32);
            break;
        }
        case QEMU_FW_CFG_OFF_DATA: /* Readonly, ignore */
            break;
        case QEMU_FW_CFG_OFF_DMA_HIGH:
        {
            if (cb == 4)
                pThis->GCPhysDma = ((RTGCPHYS)RT_BE2H_U32(u32)) << 32;
            break;
        }
        case QEMU_FW_CFG_OFF_DMA_LOW:
        {
            if (cb == 4)
            {
                pThis->GCPhysDma |= ((RTGCPHYS)RT_BE2H_U32(u32));
                qemuFwCfgDmaXfer(pThis, pThis->GCPhysDma);
                pThis->GCPhysDma = 0;
            }
            break;
        }
        default:
            rc = PDMDevHlpDBGFStop(pDevIns, RT_SRC_POS, "Port=%#x cb=%d u32=%#x\n", offPort, cb, u32);
            break;
    }

    LogFlowFunc((" -> rc=%Rrc\n", rc));
    return rc;
}


/**
 * @callback_method_impl{FNIOMIOPORTNEWIN, QEMU firmware configuration read.}
 */
static DECLCALLBACK(VBOXSTRICTRC) qemuFwCfgIoPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort, uint32_t *pu32, unsigned cb)
{
    int rc = VINF_SUCCESS;
    PDEVQEMUFWCFG pThis = PDMDEVINS_2_DATA(pDevIns, PDEVQEMUFWCFG);
    NOREF(pvUser);

    *pu32 = 0;

    LogFlowFunc(("offPort=%RTiop cb=%u\n", offPort, cb));

    switch (offPort)
    {
        /* Selector (Writeonly, ignore). */
        case QEMU_FW_CFG_OFF_SELECTOR:
            break;
        case QEMU_FW_CFG_OFF_DATA:
        {
            if (cb == 1)
            {
                if (   pThis->cbCfgItemLeft
                    && pThis->pCfgItem)
                {
                    uint8_t bRead = 0;
                    uint32_t cbRead = 0;
                    int rc2 = pThis->pCfgItem->pfnRead(pThis, pThis->pCfgItem, pThis->offCfgItemNext, &bRead,
                                                       sizeof(bRead), &cbRead);
                    if (   RT_SUCCESS(rc2)
                        && cbRead == sizeof(bRead))
                    {
                        pThis->offCfgItemNext += cbRead;
                        pThis->cbCfgItemLeft  -= cbRead;
                        *pu32 = bRead;
                    }
                }
            }
            else
                rc = PDMDevHlpDBGFStop(pDevIns, RT_SRC_POS, "Port=%#x cb=%d\n", offPort, cb);
            break;
        }

        default:
            rc = PDMDevHlpDBGFStop(pDevIns, RT_SRC_POS, "Port=%#x cb=%d\n", offPort, cb);
            break;
    }

    LogFlowFunc(("offPort=%RTiop cb=%u -> rc=%Rrc u32=%#x\n", offPort, cb, rc, *pu32));

    return rc;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 */
static DECLCALLBACK(void) qemuFwCfgReset(PPDMDEVINS pDevIns)
{
    PDEVQEMUFWCFG pThis = PDMDEVINS_2_DATA(pDevIns, PDEVQEMUFWCFG);

    qemuFwCfgR3ItemReset(pThis);
    pThis->GCPhysDma = 0;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) qemuFwCfgDestruct(PPDMDEVINS pDevIns)
{
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);
    PDEVQEMUFWCFG pThis = PDMDEVINS_2_DATA(pDevIns, PDEVQEMUFWCFG);

    qemuFwCfgR3ItemReset(pThis);
    pThis->GCPhysDma = 0;

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) qemuFwCfgConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    PDEVQEMUFWCFG pThis = PDMDEVINS_2_DATA(pDevIns, PDEVQEMUFWCFG);
    PCPDMDEVHLPR3 pHlp  = pDevIns->pHlpR3;
    Assert(iInstance == 0); RT_NOREF(iInstance);

    /*
     * Validate configuration.
     */
    PDMDEV_VALIDATE_CONFIG_RETURN(pDevIns, "DmaEnabled"
                                           "|KernelImage"
                                           "|InitrdImage"
                                           "|SetupImage"
                                           "|CmdLine",
                                           "");

    bool fDmaEnabled = false;
    int rc = pHlp->pfnCFGMQueryBoolDef(pCfg, "DmaEnabled", &fDmaEnabled, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Configuration error: Failed to read \"DmaEnabled\""));

    /*
     * Init the data.
     */
    pThis->pDevIns        = pDevIns;
    pThis->pCfg           = pCfg;
    pThis->u32Version     = QEMU_FW_CFG_VERSION_LEGACY | (fDmaEnabled ? QEMU_FW_CFG_VERSION_DMA : 0);
    pThis->GCPhysDma      = 0;

    qemuFwCfgR3ItemReset(pThis);

    /*
     * Register I/O Ports
     */
    IOMIOPORTHANDLE hIoPorts;
    rc = PDMDevHlpIoPortCreateFlagsAndMap(pDevIns, QEMU_FW_CFG_IO_PORT_START, QEMU_FW_CFG_IO_PORT_SIZE, 0 /*fFlags*/,
                                          qemuFwCfgIoPortWrite, qemuFwCfgIoPortRead,
                                          "QEMU firmware configuration", NULL /*paExtDescs*/, &hIoPorts);
    AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceQemuFwCfg =
{
    /* .u32Version = */             PDM_DEVREG_VERSION,
    /* .uReserved0 = */             0,
    /* .szName = */                 "qemu-fw-cfg",
    /* .fFlags = */                 PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_NEW_STYLE,
    /* .fClass = */                 PDM_DEVREG_CLASS_ARCH,
    /* .cMaxInstances = */          1,
    /* .uSharedVersion = */         42,
    /* .cbInstanceShared = */       sizeof(DEVQEMUFWCFG),
    /* .cbInstanceCC = */           0,
    /* .cbInstanceRC = */           0,
    /* .cMaxPciDevices = */         0,
    /* .cMaxMsixVectors = */        0,
    /* .pszDescription = */         "QEMU Firmware Config compatible device",
#if defined(IN_RING3)
    /* .pszRCMod = */               "",
    /* .pszR0Mod = */               "",
    /* .pfnConstruct = */           qemuFwCfgConstruct,
    /* .pfnDestruct = */            qemuFwCfgDestruct,
    /* .pfnRelocate = */            NULL,
    /* .pfnMemSetup = */            NULL,
    /* .pfnPowerOn = */             NULL,
    /* .pfnReset = */               qemuFwCfgReset,
    /* .pfnSuspend = */             NULL,
    /* .pfnResume = */              NULL,
    /* .pfnAttach = */              NULL,
    /* .pfnDetach = */              NULL,
    /* .pfnQueryInterface = */      NULL,
    /* .pfnInitComplete = */        NULL,
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
#elif defined(IN_RING0)
    /* .pfnEarlyConstruct = */      NULL,
    /* .pfnConstruct = */           NULL,
    /* .pfnDestruct = */            NULL,
    /* .pfnFinalDestruct = */       NULL,
    /* .pfnRequest = */             NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RC)
    /* .pfnConstruct = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#else
# error "Not in IN_RING3, IN_RING0 or IN_RC!"
#endif
    /* .u32VersionEnd = */          PDM_DEVREG_VERSION
};

