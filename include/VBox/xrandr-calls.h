/** @file
 * Stubs for dynamically loading libXrandr and the symbols which are needed by
 * VirtualBox.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
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

/** The file name of the libXrandr library. */
#define RT_RUNTIME_LOADER_LIB_NAME  "libXrandr.so.2"

/** The name of the loader function. */
#define RT_RUNTIME_LOADER_FUNCTION  RTXrandrLoadLib

/** The following are the symbols which we need from the Xrandr library. */
#define RT_RUNTIME_LOADER_INSERT_SYMBOLS \
 RT_PROXY_STUB(XRRQueryExtension, Bool, (Display *dpy, int *event_base_return, int *error_base_return), \
                 (dpy, event_base_return, error_base_return)) \
 RT_PROXY_STUB(XRRQueryVersion, Bool, (Display *dpy, int *major_version_return, int *minor_version_return), \
                 (dpy, major_version_return, minor_version_return)) \
 RT_PROXY_STUB(XRRSelectInput, void, (Display *dpy, Window window, int mask), \
                 (dpy, window, mask)) \
 RT_PROXY_STUB(XRRGetMonitors, XRRMonitorInfo *, (Display *dpy, Window window, Bool get_active, int *nmonitors), \
                 (dpy, window, get_active, nmonitors)) \
 RT_PROXY_STUB(XRRFreeMonitors, void, (XRRMonitorInfo *monitors), \
                 (monitors)) \
 RT_PROXY_STUB(XRRGetScreenResources, XRRScreenResources *, (Display *dpy, Window window), \
                 (dpy, window)) \
 RT_PROXY_STUB(XRRFreeScreenResources, void, (XRRScreenResources *resources), \
                 (resources)) \
 RT_PROXY_STUB(XRRSetOutputPrimary, void, (Display *dpy, Window window, RROutput output), \
                 (dpy, window, output))

#ifdef VBOX_XRANDR_GENERATE_HEADER
# define RT_RUNTIME_LOADER_GENERATE_HEADER
# define RT_RUNTIME_LOADER_GENERATE_DECLS
# include <iprt/runtime-loader.h>
# undef RT_RUNTIME_LOADER_GENERATE_HEADER
# undef RT_RUNTIME_LOADER_GENERATE_DECLS

#elif defined(VBOX_XRANDR_GENERATE_BODY)
# define RT_RUNTIME_LOADER_GENERATE_BODY_STUBS
# include <iprt/runtime-loader.h>
# undef RT_RUNTIME_LOADER_GENERATE_BODY_STUBS

#else
# error This file should only be included to generate stubs for loading the Xrandr library at runtime
#endif

#undef RT_RUNTIME_LOADER_LIB_NAME
#undef RT_RUNTIME_LOADER_INSERT_SYMBOLS

