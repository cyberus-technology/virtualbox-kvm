/* $Id: VBoxMPHGSMI.h $ */
/** @file
 * VBox Miniport HGSMI related header
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
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

#ifndef GA_INCLUDED_SRC_WINNT_Graphics_Video_mp_common_VBoxMPHGSMI_h
#define GA_INCLUDED_SRC_WINNT_Graphics_Video_mp_common_VBoxMPHGSMI_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "VBoxMPDevExt.h"

RT_C_DECLS_BEGIN
void VBoxSetupDisplaysHGSMI(PVBOXMP_COMMON pCommon, PHYSICAL_ADDRESS phVRAM, uint32_t ulApertureSize, uint32_t cbVRAM, uint32_t fCaps);
void VBoxFreeDisplaysHGSMI(PVBOXMP_COMMON pCommon);
RT_C_DECLS_END

#endif /* !GA_INCLUDED_SRC_WINNT_Graphics_Video_mp_common_VBoxMPHGSMI_h */
