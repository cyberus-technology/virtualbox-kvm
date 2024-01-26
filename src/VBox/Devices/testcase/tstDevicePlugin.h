/** @file
 * tstDevice: Plugin API.
 */

/*
 * Copyright (C) 2017-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_SRC_testcase_tstDevicePlugin_h
#define VBOX_INCLUDED_SRC_testcase_tstDevicePlugin_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>

#include "tstDeviceCfg.h"


/** Device under test handle. */
typedef struct TSTDEVDUTINT *TSTDEVDUT;

/**
 * Testcase registration structure.
 */
typedef struct TSTDEVTESTCASEREG
{
    /** Testcase name. */
    char                szName[16];
    /** Testcase description. */
    const char          *pszDesc;
    /** Flags for this testcase. */
    uint32_t            fFlags;

    /**
     * Testcase entry point.
     *
     * @returns VBox status code.
     * @param   hDut      Handle of the device under test.
     * @param   paCfg     Pointer to the testcase config.
     * @param   cCfgItems Number of config items.
     */
    DECLR3CALLBACKMEMBER(int, pfnTestEntry, (TSTDEVDUT hDut, PCTSTDEVCFGITEM paCfg, uint32_t cCfgItems));
} TSTDEVTESTCASEREG;
/** Pointer to a testcase registration structure. */
typedef TSTDEVTESTCASEREG *PTSTDEVTESTCASEREG;
/** Pointer to a constant testcase registration structure. */
typedef const TSTDEVTESTCASEREG *PCTSTDEVTESTCASEREG;


/**
 * Testcase register callbacks structure.
 */
typedef struct TSTDEVPLUGINREGISTER
{
    /**
     * Registers a new testcase.
     *
     * @returns VBox status code.
     * @param   pvUser       Opaque user data given in the plugin load callback.
     * @param   pTestcaseReg The testcase descriptor to register.
     */
    DECLR3CALLBACKMEMBER(int, pfnRegisterTestcase, (void *pvUser, PCTSTDEVTESTCASEREG pTestcaseReg));

} TSTDEVPLUGINREGISTER;
/** Pointer to a backend register callbacks structure. */
typedef TSTDEVPLUGINREGISTER *PTSTDEVPLUGINREGISTER;


/**
 * Initialization entry point called by the device test framework when
 * a plugin is loaded.
 *
 * @returns VBox status code.
 * @param   pvUser             Opaque user data passed in the register callbacks.
 * @param   pRegisterCallbacks Pointer to the register callbacks structure.
 */
typedef DECLCALLBACKTYPE(int, FNTSTDEVPLUGINLOAD,(void *pvUser, PTSTDEVPLUGINREGISTER pRegisterCallbacks));
typedef FNTSTDEVPLUGINLOAD *PFNTSTDEVPLUGINLOAD;
#define TSTDEV_PLUGIN_LOAD_NAME "TSTDevPluginLoad"

#endif /* !VBOX_INCLUDED_SRC_testcase_tstDevicePlugin_h */
