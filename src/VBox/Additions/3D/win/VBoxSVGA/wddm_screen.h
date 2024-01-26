/* $Id: wddm_screen.h $ */
/** @file
 * VirtualBox Windows Guest Mesa3D - VMSVGA hardware driver.
 */

/*
 * Copyright (C) 2016-2023 Oracle and/or its affiliates.
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

#ifndef GA_INCLUDED_SRC_3D_win_VBoxSVGA_wddm_screen_h
#define GA_INCLUDED_SRC_3D_win_VBoxSVGA_wddm_screen_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBoxGaDriver.h>

#include "vmw_screen.h"

struct vmw_winsys_screen_wddm
{
    struct vmw_winsys_screen base;

    const WDDMGalliumDriverEnv *pEnv;
    VBOXGAHWINFOSVGA HwInfo;
};

#endif /* !GA_INCLUDED_SRC_3D_win_VBoxSVGA_wddm_screen_h */

