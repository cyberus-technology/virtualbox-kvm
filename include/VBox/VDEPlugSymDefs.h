/** @file
 * Symbols from libvdeplug.so to be loaded at runtime for DrvVDE.cpp
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

/** The file name of the DBus library */
#define VBOX_LIB_VDE_PLUG_NAME "libvdeplug.so"
#define RT_RUNTIME_LOADER_LIB_NAME VBOX_LIB_VDE_PLUG_NAME

/** The name of the loader function */
#define RT_RUNTIME_LOADER_FUNCTION DrvVDELoadVDEPlug

/** The following are the symbols which we need from the library. */
#define RT_RUNTIME_LOADER_INSERT_SYMBOLS \
 RT_PROXY_STUB(vde_open_real, VDECONN *, \
               (const char *vde_switch, const char *descr, int interface_version, struct vde_open_args *open_args), \
               (vde_switch, descr, interface_version, open_args)) \
 RT_PROXY_STUB(vde_recv, size_t, \
               (VDECONN *conn, void *buf,size_t len, int flags), \
               (conn, buf, len, flags)) \
 RT_PROXY_STUB(vde_send, size_t, \
               (VDECONN *conn, const void *buf, size_t len, int flags), \
               (conn, buf, len, flags)) \
 RT_PROXY_STUB(vde_datafd, int, (VDECONN *conn), (conn)) \
 RT_PROXY_STUB(vde_close, void, (VDECONN *conn), (conn))

#ifdef VDEPLUG_GENERATE_HEADER
# define RT_RUNTIME_LOADER_GENERATE_HEADER
# define RT_RUNTIME_LOADER_GENERATE_DECLS
# include <iprt/runtime-loader.h>
# undef RT_RUNTIME_LOADER_GENERATE_HEADER
# undef RT_RUNTIME_LOADER_GENERATE_DECLS
#elif defined (VDEPLUG_GENERATE_BODY)
# define RT_RUNTIME_LOADER_GENERATE_BODY_STUBS
# include <iprt/runtime-loader.h>
# undef RT_RUNTIME_LOADER_GENERATE_BODY_STUBS
#else
# error This file should only be included to generate stubs for loading the libvdeplug library at runtime
#endif

#undef RT_RUNTIME_LOADER_LIB_NAME
#undef RT_RUNTIME_LOADER_INSERT_SYMBOLS

