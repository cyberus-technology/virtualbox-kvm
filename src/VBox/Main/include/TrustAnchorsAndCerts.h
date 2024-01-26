/* $Id: TrustAnchorsAndCerts.h $ */
/** @file
 * Main - Collection of trust anchors and certificates included in VBoxSVC.
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

#ifndef MAIN_INCLUDED_TrustAnchorsAndCerts_h
#define MAIN_INCLUDED_TrustAnchorsAndCerts_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>

RT_C_DECLS_BEGIN

extern const unsigned char g_abUefiMicrosoftKek[];
extern const unsigned g_cbUefiMicrosoftKek;

extern const unsigned char g_abUefiMicrosoftCa[];
extern const unsigned g_cbUefiMicrosoftCa;

extern const unsigned char g_abUefiMicrosoftProPca[];
extern const unsigned g_cbUefiMicrosoftProPca;

extern const unsigned char g_abUefiOracleDefPk[];
extern const unsigned g_cbUefiOracleDefPk;

RT_C_DECLS_END

#endif /* !MAIN_INCLUDED_TrustAnchorsAndCerts_h */

