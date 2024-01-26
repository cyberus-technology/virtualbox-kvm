/** @file
 * tstDevice: Configuration handling.
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

#ifndef VBOX_INCLUDED_SRC_testcase_tstDeviceCfg_h
#define VBOX_INCLUDED_SRC_testcase_tstDeviceCfg_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/param.h>
#include <VBox/types.h>
#include <iprt/err.h>

RT_C_DECLS_BEGIN

/**
 * Config item type.
 */
typedef enum TSTDEVCFGITEMTYPE
{
    /** Invalid type. */
    TSTDEVCFGITEMTYPE_INVALID = 0,
    /** String type. */
    TSTDEVCFGITEMTYPE_STRING,
    /** Integer value encoded in the string. */
    TSTDEVCFGITEMTYPE_INTEGER,
    /** Raw bytes. */
    TSTDEVCFGITEMTYPE_BYTES,
    /** 32bit hack. */
    TSTDEVCFGITEMTYPE_32BIT_HACK = 0x7fffffff
} TSTDEVCFGITEMTYPE;
/** Pointer to a config item type. */
typedef TSTDEVCFGITEMTYPE *PTSTDEVCFGITEMTYPE;


/**
 * Testcase config item.
 */
typedef struct TSTDEVCFGITEM
{
    /** The key of the item. */
    const char          *pszKey;
    /** Type of the config item. */
    TSTDEVCFGITEMTYPE   enmType;
    /** Type dependent data. */
    union
    {
        /** String value. */
        const char      *psz;
        /** Integer value. */
        int64_t         i64;
        /** Raw bytes. */
        struct
        {
            /** Size of the byte buffer. */
            size_t      cb;
            /** Pointer to the raw buffer. */
            const void  *pv;
        } RawBytes;
    } u;
} TSTDEVCFGITEM;
/** Pointer to a testcase config item. */
typedef TSTDEVCFGITEM *PTSTDEVCFGITEM;
/** Pointer to a constant testcase config item. */
typedef const TSTDEVCFGITEM *PCTSTDEVCFGITEM;


/**
 * A single test.
 */
typedef struct TSTDEVTEST
{
    /** Flag whether to enable the R0 part for testing. */
    bool                        fR0Enabled;
    /** Flag whether to enable the RC part for testing. */
    bool                        fRCEnabled;
    /** Number of configuration items for the device. */
    uint32_t                    cCfgItems;
    /** Pointer to array of configuration items for the device. */
    PCTSTDEVCFGITEM             paCfgItems;
    /** Number of testcases to run with that device instance. */
    uint32_t                    cTestcases;
    /** Pointer to the array of testcase IDs. */
    const char                  **papszTestcaseIds;
    /** Pointer to the array of testcase configuration item numbers. */
    uint32_t                    *pacTestcaseCfgItems;
    /** Pointer to the array of configuration item array pointers for each testcase. */
    PCTSTDEVCFGITEM             *papTestcaseCfg;
} TSTDEVTEST;
/** Pointer to a single test. */
typedef TSTDEVTEST *PTSTDEVTEST;
/** Pointer to a const single test. */
typedef const TSTDEVTEST *PCTSTDEVTEST;


/**
 * A device test configuration.
 */
typedef struct TSTDEVCFG
{
    /** The identifier of the device to test. */
    const char                  *pszDevName;

    /** R3 PDM module to load containing the device to test. */
    const char                  *pszPdmR3Mod;
    /** R0 PDM module to load containing the device to test. */
    const char                  *pszPdmR0Mod;
    /** RC PDM module to load containing the device to test. */
    const char                  *pszPdmRCMod;

    /** Testcase module to load. */
    const char                  *pszTstDevMod;

    /** Number of tests configured in the config. */
    uint32_t                    cTests;
    /** The array of tests to execute for the given device - variable in size. */
    TSTDEVTEST                  aTests[1];
} TSTDEVCFG;
/** Pointer to a device test configuration. */
typedef TSTDEVCFG *PTSTDEVCFG;
/** Pointer to a const device test configuration. */
typedef const TSTDEVCFG *PCTSTDEVCFG;
/** Pointer to a device test configuration pointer. */
typedef TSTDEVCFG *PPTSTDEVCFG;


/**
 * Loads the config from the given file returning the configuration structure on success.
 *
 * @returns VBox status code.
 * @param   pszCfgFilename          The configuration file path to load.
 * @param   pErrInfo                Where to store additional error information if loading the config fails, optional.
 * @param   ppDevTstCfg             Where to store the pointer to the created test configuration on success.
 */
DECLHIDDEN(int) tstDevCfgLoad(const char *pszCfgFilename, PRTERRINFO pErrInfo, PCTSTDEVCFG *ppDevTstCfg);

/**
 * Destroys the given test configuration freeing all allocated resources.
 *
 * @param   pDevTstCfg              The test configuration to destroy.
 */
DECLHIDDEN(void) tstDevCfgDestroy(PCTSTDEVCFG pDevTstCfg);

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_SRC_testcase_tstDeviceCfg_h */
