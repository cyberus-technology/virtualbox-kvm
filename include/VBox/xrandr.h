/** @file
 * Module to dynamically load libXrandr and load all symbols which are needed by
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

#ifndef VBOX_INCLUDED_xrandr_h
#define VBOX_INCLUDED_xrandr_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/stdarg.h>

#ifndef __cplusplus
# error "This header requires C++ to avoid name clashes."
#endif

/* Define missing X11/XRandr structures, types and macros. */

#define Bool                        int
#define RRScreenChangeNotifyMask    (1L << 0)
#define RRScreenChangeNotify        0

struct _XDisplay;
typedef struct _XDisplay Display;

typedef unsigned long Atom;
typedef unsigned long XID;
typedef XID RROutput;
typedef XID Window;
typedef XID RROutput;
typedef XID RRCrtc;
typedef XID RRMode;
typedef unsigned long XRRModeFlags;
typedef unsigned long int Time;

struct XRRMonitorInfo
{
    Atom name;
    Bool primary;
    Bool automatic;
    int noutput;
    int x;
    int y;
    int width;
    int height;
    int mwidth;
    int mheight;
    RROutput *outputs;
};
typedef struct XRRMonitorInfo XRRMonitorInfo;

struct XRRModeInfo
{
    RRMode id;
    unsigned int width;
    unsigned int height;
    unsigned long dotClock;
    unsigned int hSyncStart;
    unsigned int hSyncEnd;
    unsigned int hTotal;
    unsigned int hSkew;
    unsigned int vSyncStart;
    unsigned int vSyncEnd;
    unsigned int vTotal;
    char *name;
    unsigned int nameLength;
    XRRModeFlags modeFlags;
};
typedef struct XRRModeInfo XRRModeInfo;

struct XRRScreenResources
{
    Time timestamp;
    Time configTimestamp;
    int ncrtc;
    RRCrtc *crtcs;
    int noutput;
    RROutput *outputs;
    int nmode;
    XRRModeInfo *modes;
};
typedef struct XRRScreenResources XRRScreenResources;

/* Declarations of the functions that we need from libXrandr. */
#define VBOX_XRANDR_GENERATE_HEADER

#include <VBox/xrandr-calls.h>

#undef VBOX_XRANDR_GENERATE_HEADER

#endif /* !VBOX_INCLUDED_xrandr_h */

