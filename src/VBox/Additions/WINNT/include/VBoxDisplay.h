/** @file
 * VBoxDisplay - private windows additions display header
 */

/*
 * Copyright (C) 2006-2023 Oracle and/or its affiliates.
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

#ifndef GA_INCLUDED_WINNT_VBoxDisplay_h
#define GA_INCLUDED_WINNT_VBoxDisplay_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/types.h>
#include <iprt/assert.h>

#define VBOXESC_SETVISIBLEREGION            0xABCD9001
#define VBOXESC_ISVRDPACTIVE                0xABCD9002
#ifdef VBOX_WITH_WDDM
# define VBOXESC_REINITVIDEOMODES           0xABCD9003
# define VBOXESC_GETVBOXVIDEOCMCMD          0xABCD9004
# define VBOXESC_DBGPRINT                   0xABCD9005
# define VBOXESC_SCREENLAYOUT               0xABCD9006
// obsolete                                 0xABCD9007
// obsolete                                 0xABCD9008
// obsolete                                 0xABCD9009
// obsolete                                 0xABCD900A
// obsolete                                 0xABCD900B
// obsolete                                 0xABCD900C
# define VBOXESC_DBGDUMPBUF                 0xABCD900D
// obsolete                                 0xABCD900E
// obsolete                                 0xABCD900F
# define VBOXESC_REINITVIDEOMODESBYMASK     0xABCD9010
# define VBOXESC_ADJUSTVIDEOMODES           0xABCD9011
// obsolete                                 0xABCD9012
# define VBOXESC_CONFIGURETARGETS           0xABCD9013
# define VBOXESC_SETALLOCHOSTID             0xABCD9014
// obsolete                                 0xABCD9015
# define VBOXESC_UPDATEMODES                0xABCD9016
# define VBOXESC_GUEST_DISPLAYCHANGED       0xABCD9017
# define VBOXESC_TARGET_CONNECTIVITY        0xABCD9018
#endif /* #ifdef VBOX_WITH_WDDM */

# define VBOXESC_ISANYX                     0xABCD9200

typedef struct VBOXDISPIFESCAPE
{
    int32_t escapeCode;
    uint32_t u32CmdSpecific;
} VBOXDISPIFESCAPE, *PVBOXDISPIFESCAPE;

/* ensure command body is always 8-byte-aligned*/
AssertCompile((sizeof (VBOXDISPIFESCAPE) & 7) == 0);

#define VBOXDISPIFESCAPE_DATA_OFFSET() ((sizeof (VBOXDISPIFESCAPE) + 7) & ~7)
#define VBOXDISPIFESCAPE_DATA(_pHead, _t) ( (_t*)(((uint8_t*)(_pHead)) + VBOXDISPIFESCAPE_DATA_OFFSET()))
#define VBOXDISPIFESCAPE_DATA_SIZE(_s) ( (_s) < VBOXDISPIFESCAPE_DATA_OFFSET() ? 0 : (_s) - VBOXDISPIFESCAPE_DATA_OFFSET() )
#define VBOXDISPIFESCAPE_SIZE(_cbData) ((_cbData) ? VBOXDISPIFESCAPE_DATA_OFFSET() + (_cbData) : sizeof (VBOXDISPIFESCAPE))

#define IOCTL_VIDEO_VBOX_SETVISIBLEREGION \
    CTL_CODE(FILE_DEVICE_VIDEO, 0xA01, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_VBOX_ISANYX \
    CTL_CODE(FILE_DEVICE_VIDEO, 0xA02, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct VBOXDISPIFESCAPE_ISANYX
{
    VBOXDISPIFESCAPE EscapeHdr;
    uint32_t u32IsAnyX;
} VBOXDISPIFESCAPE_ISANYX, *PVBOXDISPIFESCAPE_ISANYX;

#ifdef VBOX_WITH_WDDM

/* Enables code which performs (un)plugging of virtual displays in VBOXESC_UPDATEMODES.
 * The code has been disabled as part of #8244.
 */
//#define VBOX_WDDM_REPLUG_ON_MODE_CHANGE

/* for VBOX_VIDEO_MAX_SCREENS definition */
#include <VBoxVideo.h>

typedef struct VBOXWDDM_RECOMMENDVIDPN_SOURCE
{
    RTRECTSIZE Size;
} VBOXWDDM_RECOMMENDVIDPN_SOURCE;

typedef struct VBOXWDDM_RECOMMENDVIDPN_TARGET
{
    int32_t iSource;
} VBOXWDDM_RECOMMENDVIDPN_TARGET;

typedef struct
{
    VBOXWDDM_RECOMMENDVIDPN_SOURCE aSources[VBOX_VIDEO_MAX_SCREENS];
    VBOXWDDM_RECOMMENDVIDPN_TARGET aTargets[VBOX_VIDEO_MAX_SCREENS];
} VBOXWDDM_RECOMMENDVIDPN, *PVBOXWDDM_RECOMMENDVIDPN;

#define VBOXWDDM_SCREENMASK_SIZE ((VBOX_VIDEO_MAX_SCREENS + 7) >> 3)

typedef struct VBOXDISPIFESCAPE_UPDATEMODES
{
    VBOXDISPIFESCAPE EscapeHdr;
    uint32_t u32TargetId;
    RTRECTSIZE Size;
} VBOXDISPIFESCAPE_UPDATEMODES;

typedef struct VBOXDISPIFESCAPE_TARGETCONNECTIVITY
{
    VBOXDISPIFESCAPE EscapeHdr;
    uint32_t u32TargetId;
    uint32_t fu32Connect;
} VBOXDISPIFESCAPE_TARGETCONNECTIVITY;

#endif /* VBOX_WITH_WDDM */

#endif /* !GA_INCLUDED_WINNT_VBoxDisplay_h */

