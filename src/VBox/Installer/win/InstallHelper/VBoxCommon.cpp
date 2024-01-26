/* $Id: VBoxCommon.cpp $ */
/** @file
 * VBoxCommon - Misc helper routines for install helper.
 *
 * This is used by internal/serial.cpp and VBoxInstallHelper.cpp.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/win/windows.h>
#include <msi.h>
#include <msiquery.h>

#include <iprt/string.h>
#include <iprt/utf16.h>


UINT VBoxGetMsiProp(MSIHANDLE hMsi, const WCHAR *pwszName, WCHAR *pwszValueBuf, DWORD cwcValueBuf)
{
    RT_BZERO(pwszValueBuf, cwcValueBuf * sizeof(pwszValueBuf[0]));

    /** @todo r=bird: why do we need to query the size first and then the data.
     *        The API should be perfectly capable of doing that without our help. */
    DWORD cwcNeeded = 0;
    UINT  uiRet = MsiGetPropertyW(hMsi, pwszName, L"", &cwcNeeded);
    if (uiRet == ERROR_MORE_DATA)
    {
        ++cwcNeeded;     /* On output does not include terminating null, so add 1. */

        if (cwcNeeded > cwcValueBuf)
            return ERROR_MORE_DATA;
        uiRet = MsiGetPropertyW(hMsi, pwszName, pwszValueBuf, &cwcNeeded);
    }
    return uiRet;
}

#if 0 /* unused */
/**
 * Retrieves a MSI property (in UTF-8).
 *
 * Convenience function for VBoxGetMsiProp().
 *
 * @returns VBox status code.
 * @param   hMsi                MSI handle to use.
 * @param   pcszName            Name of property to retrieve.
 * @param   ppszValue           Where to store the allocated value on success.
 *                              Must be free'd using RTStrFree() by the caller.
 */
int VBoxGetMsiPropUtf8(MSIHANDLE hMsi, const char *pcszName, char **ppszValue)
{
    PRTUTF16 pwszName;
    int rc = RTStrToUtf16(pcszName, &pwszName);
    if (RT_SUCCESS(rc))
    {
        WCHAR wszValue[1024]; /* 1024 should be enough for everybody (tm). */
        if (VBoxGetMsiProp(hMsi, pwszName, wszValue, sizeof(wszValue)) == ERROR_SUCCESS)
            rc = RTUtf16ToUtf8(wszValue, ppszValue);
        else
            rc = VERR_NOT_FOUND;

        RTUtf16Free(pwszName);
    }

    return rc;
}
#endif

UINT VBoxSetMsiProp(MSIHANDLE hMsi, const WCHAR *pwszName, const WCHAR *pwszValue)
{
    return MsiSetPropertyW(hMsi, pwszName, pwszValue);
}

UINT VBoxSetMsiPropDWORD(MSIHANDLE hMsi, const WCHAR *pwszName, DWORD dwVal)
{
    wchar_t wszTemp[32];
    RTUtf16Printf(wszTemp, RT_ELEMENTS(wszTemp), "%u", dwVal);
    return VBoxSetMsiProp(hMsi, pwszName, wszTemp);
}

