/** @file
 * tstDevice: Builtin tests.
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

#ifndef VBOX_INCLUDED_SRC_testcase_tstDeviceBuiltin_h
#define VBOX_INCLUDED_SRC_testcase_tstDeviceBuiltin_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/param.h>
#include <VBox/types.h>
#include <iprt/err.h>

#include "tstDevicePlugin.h"

RT_C_DECLS_BEGIN

extern const TSTDEVTESTCASEREG g_TestcaseSsmFuzz;
extern const TSTDEVTESTCASEREG g_TestcaseSsmLoadDbg;
extern const TSTDEVTESTCASEREG g_TestcaseIoFuzz;

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_SRC_testcase_tstDeviceBuiltin_h */
