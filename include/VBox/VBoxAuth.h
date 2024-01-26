/** @file
 * VirtualBox External Authentication Library Interface.
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

#ifndef VBOX_INCLUDED_VBoxAuth_h
#define VBOX_INCLUDED_VBoxAuth_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/** @defgroup grp_vboxauth VirtualBox External Authentication Library Interface
 * @{
 */

/* The following 2 enums are 32 bits values.*/
typedef enum AuthResult
{
    AuthResultAccessDenied    = 0,
    AuthResultAccessGranted   = 1,
    AuthResultDelegateToGuest = 2,
    AuthResultSizeHack        = 0x7fffffff
} AuthResult;

typedef enum AuthGuestJudgement
{
    AuthGuestNotAsked      = 0,
    AuthGuestAccessDenied  = 1,
    AuthGuestNoJudgement   = 2,
    AuthGuestAccessGranted = 3,
    AuthGuestNotReacted    = 4,
    AuthGuestSizeHack      = 0x7fffffff
} AuthGuestJudgement;

/** UUID memory representation. Array of 16 bytes.
 *
 * @note VirtualBox uses a consistent binary representation of UUIDs on all platforms. For this reason
 * the integer fields comprising the UUID are stored as little endian values. If you want to pass such
 * UUIDs to code which assumes that the integer fields are big endian (often also called network byte
 * order), you need to adjust the contents of the UUID to e.g. achieve the same string representation.
 *
 * The required changes are:
 *     - reverse the order of byte 0, 1, 2 and 3
 *     - reverse the order of byte 4 and 5
 *     - reverse the order of byte 6 and 7.
 *
 * Using this conversion you will get identical results when converting the binary UUID to the string
 * representation.
 */
typedef unsigned char AUTHUUID[16];
typedef AUTHUUID *PAUTHUUID;

/** The library entry point calling convention. */
#ifdef _MSC_VER
# define AUTHCALL __cdecl
#elif defined(__GNUC__)
# define AUTHCALL
#else
# error "Unsupported compiler"
#endif


/**
 * Authentication library entry point.
 *
 * @param  pUuid            Pointer to the UUID of the accessed virtual machine. Can be NULL.
 * @param  guestJudgement   Result of the guest authentication.
 * @param  pszUser          User name passed in by the client (UTF8).
 * @param  pszPassword      Password passed in by the client (UTF8).
 * @param  pszDomain        Domain passed in by the client (UTF8).
 *
 * Return code:
 *
 * @retval AuthAccessDenied    Client access has been denied.
 * @retval AuthAccessGranted   Client has the right to use the virtual machine.
 * @retval AuthDelegateToGuest Guest operating system must
 *                              authenticate the client and the
 *                              library must be called again with
 *                              the result of the guest
 *                              authentication.
 */
typedef AuthResult AUTHCALL FNAUTHENTRY(PAUTHUUID pUuid,
                                        AuthGuestJudgement guestJudgement,
                                        const char *pszUser,
                                        const char *pszPassword,
                                        const char *pszDomain);
/** Pointer to a FNAUTHENTRY function. */
typedef FNAUTHENTRY *PFNAUTHENTRY;
/** @deprecated   */
typedef FNAUTHENTRY  AUTHENTRY;
/** @deprecated   */
typedef PFNAUTHENTRY PAUTHENTRY;
/** Name of the FNAUTHENTRY entry point. */
#define AUTHENTRY_NAME "VRDPAuth"

/**
 * Authentication library entry point version 2.
 *
 * @param  pUuid            Pointer to the UUID of the accessed virtual machine. Can be NULL.
 * @param  guestJudgement   Result of the guest authentication.
 * @param  pszUser          User name passed in by the client (UTF8).
 * @param  pszPassword      Password passed in by the client (UTF8).
 * @param  pszDomain        Domain passed in by the client (UTF8).
 * @param  fLogon           Boolean flag. Indicates whether the entry point is
 *                          called for a client logon or the client disconnect.
 * @param  clientId         Server side unique identifier of the client.
 *
 * @retval AuthAccessDenied    Client access has been denied.
 * @retval AuthAccessGranted   Client has the right to use the virtual machine.
 * @retval AuthDelegateToGuest Guest operating system must
 *                             authenticate the client and the
 *                             library must be called again with
 *                             the result of the guest authentication.
 *
 * @note When @a fLogon is 0, only @a pUuid and @a clientId are valid and the
 *       return code is ignored.
 */
typedef AuthResult AUTHCALL FNAUTHENTRY2(PAUTHUUID pUuid,
                                         AuthGuestJudgement guestJudgement,
                                         const char *pszUser,
                                         const char *pszPassword,
                                         const char *pszDomain,
                                         int fLogon,
                                         unsigned clientId);
/** Pointer to a FNAUTHENTRY2 function. */
typedef FNAUTHENTRY2 *PFNAUTHENTRY2;
/** @deprecated   */
typedef FNAUTHENTRY2  AUTHENTRY2;
/** @deprecated   */
typedef PFNAUTHENTRY2 PAUTHENTRY2;
/** Name of the FNAUTHENTRY2 entry point. */
#define AUTHENTRY2_NAME "VRDPAuth2"

/**
 * Authentication library entry point version 3.
 *
 * @param  pszCaller        The name of the component which calls the library (UTF8).
 * @param  pUuid            Pointer to the UUID of the accessed virtual machine. Can be NULL.
 * @param  guestJudgement   Result of the guest authentication.
 * @param  pszUser          User name passed in by the client (UTF8).
 * @param  pszPassword      Password passed in by the client (UTF8).
 * @param  pszDomain        Domain passed in by the client (UTF8).
 * @param  fLogon           Boolean flag. Indicates whether the entry point is
 *                          called for a client logon or the client disconnect.
 * @param  clientId         Server side unique identifier of the client.
 *
 * @retval AuthResultAccessDenied    Client access has been denied.
 * @retval AuthResultAccessGranted   Client has the right to use the
 *                                   virtual machine.
 * @retval AuthResultDelegateToGuest Guest operating system must
 *                                   authenticate the client and the
 *                                   library must be called again with
 *                                   the result of the guest
 *                                   authentication.
 *
 * @note When @a fLogon is 0, only @a pszCaller, @a pUuid and @a clientId are
 *       valid and the return code is ignored.
 */
typedef AuthResult AUTHCALL FNAUTHENTRY3(const char *pszCaller,
                                         PAUTHUUID pUuid,
                                         AuthGuestJudgement guestJudgement,
                                         const char *pszUser,
                                         const char *pszPassword,
                                         const char *pszDomain,
                                         int fLogon,
                                         unsigned clientId);
/** Pointer to a FNAUTHENTRY3 function. */
typedef FNAUTHENTRY3 *PFNAUTHENTRY3;
/** @deprecated */
typedef FNAUTHENTRY3  AUTHENTRY3;
/** @deprecated */
typedef PFNAUTHENTRY3 PAUTHENTRY3;

/** Name of the FNAUTHENTRY3 entry point. */
#define AUTHENTRY3_NAME "AuthEntry"

/** @} */

#endif /* !VBOX_INCLUDED_VBoxAuth_h */
