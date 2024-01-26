/** @file
 * USBLib - Library for wrapping up the VBoxUSB functionality. (DEV,HDrv,Main)
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

#ifndef VBOX_INCLUDED_usblib_h
#define VBOX_INCLUDED_usblib_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/usb.h>
#include <VBox/usbfilter.h>
#include <iprt/ctype.h>
#include <iprt/string.h>

#ifdef RT_OS_WINDOWS
# include <VBox/usblib-win.h>
#endif
#ifdef RT_OS_SOLARIS
# include <VBox/usblib-solaris.h>
#endif
#ifdef RT_OS_DARWIN
# include <VBox/usblib-darwin.h>
#endif
/** @todo merge the usblib-win.h interface into the darwin and linux ports where suitable. */

RT_C_DECLS_BEGIN
/** @defgroup grp_usblib    USBLib - USB Support Library
 * This module implements the basic low-level OS interfaces and common USB code.
 * @{
 */

#ifdef IN_RING3
/**
 * Initializes the USBLib component.
 *
 * The USBLib keeps a per process connection to the kernel driver
 * and all USBLib users within a process will share the same
 * connection. USBLib does reference counting to make sure that
 * the connection remains open until all users has called USBLibTerm().
 *
 * @returns VBox status code.
 *
 * @remark  The users within the process are responsible for not calling
 *          this function at the same time (because I'm lazy).
 */
USBLIB_DECL(int) USBLibInit(void);

/**
 * Terminates the USBLib component.
 *
 * Must match successful USBLibInit calls.
 *
 * @returns VBox status code.
 */
USBLIB_DECL(int) USBLibTerm(void);

/**
 * Adds a filter.
 *
 * This function will validate and transfer the specified filter
 * to the kernel driver and make it start using it. The kernel
 * driver will return a filter id that this function passes on
 * to its caller.
 *
 * The kernel driver will associate the added filter with the
 * calling process and automatically remove all filters when
 * the process terminates the connection to it or dies.
 *
 * @returns Filter id for passing to USBLibRemoveFilter on success.
 * @returns NULL on failure.
 *
 * @param   pFilter     The filter to add.
 */
USBLIB_DECL(void *) USBLibAddFilter(PCUSBFILTER pFilter);

/**
 * Removes a filter.
 *
 * @param   pvId        The ID returned by USBLibAddFilter.
 */
USBLIB_DECL(void) USBLibRemoveFilter(void *pvId);

/**
 * Calculate the hash of the serial string.
 *
 * 64bit FNV1a, chosen because it is designed to hash in to a power of two
 * space, and is much quicker and simpler than, say, a half MD4.
 *
 * @returns the hash.
 * @param   pszSerial       The serial string.
 */
USBLIB_DECL(uint64_t) USBLibHashSerial(const char *pszSerial);

#endif /* IN_RING3 */

/**
 * Purge string of non-UTF-8 encodings and control characters.
 *
 * Control characters creates problems when presented to the user and currently
 * also when used in XML settings.  So, we must purge them in the USB vendor,
 * product, and serial number strings.
 *
 * @returns String length (excluding terminator).
 * @param   psz                 The string to purge.
 *
 * @remarks The return string may be shorter than the input, left over space
 *          after the end of the string will be filled with zeros.
 */
DECLINLINE(size_t) USBLibPurgeEncoding(char *psz)
{
    if (psz)
    {
        size_t offSrc;

        /* Beat it into valid UTF-8 encoding. */
        RTStrPurgeEncoding(psz);

        /* Look for control characters. */
        for (offSrc = 0; ; offSrc++)
        {
            char ch = psz[offSrc];
            if (RT_UNLIKELY(RT_C_IS_CNTRL(ch) && ch != '\0'))
            {
                /* Found a control character! Replace tab by space and remove all others. */
                size_t offDst = offSrc;
                for (;; offSrc++)
                {
                    ch = psz[offSrc];
                    if (RT_C_IS_CNTRL(ch) && ch != '\0')
                    {
                        if (ch == '\t')
                            ch = ' ';
                        else
                            continue;
                    }
                    psz[offDst++] = ch;
                    if (ch == '\0')
                        break;
                }

                /* Wind back to the zero terminator and zero fill any gap to make
                   USBFilterValidate happy.  (offSrc is at zero terminator too.) */
                offDst--;
                while (offSrc > offDst)
                    psz[offSrc--] = '\0';

                return offDst;
            }
            if (ch == '\0')
                break;
        }
        return offSrc;
    }
    return 0;
}


/** @} */
RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_usblib_h */

