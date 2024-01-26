/** @file
 * IPRT - Build Configuration Information
 */

/*
 * Copyright (C) 2009-2023 Oracle and/or its affiliates.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_buildconfig_h
#define IPRT_INCLUDED_buildconfig_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_buildconfig    RTBldCfg - Build Configuration Information
 * @ingroup grp_rt
 * @{
 */

/**
 * Gets the source code management revision of the IPRT build.
 * @returns Source code management revision number.
 */
RTDECL(uint32_t)     RTBldCfgRevision(void);

/**
 * Gets the source code management revision of the IPRT build.
 * @returns Read only string containing the revision number.
 */
RTDECL(const char *) RTBldCfgRevisionStr(void);

/**
 * Gets the product version string.
 *
 * This will be a string on the form "x.y.z[_string]".
 *
 * @returns Read only version string.
 *
 * @remarks This is a build time configuration thing that the product using IPRT
 *          will set.  It is therefore not any IPRT version, but rather the
 *          version of that product.
 */
RTDECL(const char *) RTBldCfgVersion(void);

/**
 * Gets the major product version number.
 * @returns Major product version number.
 * @remarks See RTBldCfgVersion.
 */
RTDECL(uint32_t)     RTBldCfgVersionMajor(void);

/**
 * Gets the minor product version number.
 * @returns Minor product version number.
 * @remarks See RTBldCfgVersion.
 */
RTDECL(uint32_t)     RTBldCfgVersionMinor(void);

/**
 * Gets the product build number.
 * @returns Product build number.
 * @remarks See RTBldCfgVersion.
 */
RTDECL(uint32_t)     RTBldCfgVersionBuild(void);

/**
 * Gets the build target name.
 *
 * @returns Read only build target string.
 */
RTDECL(const char *) RTBldCfgTarget(void);

/**
 * Gets the build target architecture name.
 *
 * @returns Read only build target architecture string.
 */
RTDECL(const char *) RTBldCfgTargetArch(void);

/**
 * Gets the build target-dot-architecture name.
 *
 * @returns Read only build target-dot-architecture string.
 */
RTDECL(const char *) RTBldCfgTargetDotArch(void);

/**
 * Gets the build type name.
 *
 * @returns Read only build type string.
 */
RTDECL(const char *) RTBldCfgType(void);

/**
 * Gets the name of the compiler used for building IPRT.
 *
 * @returns Read only compiler name.
 */
RTDECL(const char *) RTBldCfgCompiler(void);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_buildconfig_h */

