/* $Id: */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, host version check.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/string.h>
#include <VBox/log.h>

#ifdef RT_OS_WINDOWS
 #define WIN32_LEAN_AND_MEAN
 #include <iprt/win/windows.h>
#endif

#include "VBoxGuestR3LibInternal.h"


/**
 * Checks for a Guest Additions update by comparing the installed version on the
 * guest and the reported host version.
 *
 * @returns VBox status code
 *
 * @param   idClient            The client id returned by
 *                              VbglR3InfoSvcConnect().
 * @param   pfUpdate            Receives pointer to boolean flag indicating
 *                              whether an update was found or not.
 * @param   ppszHostVersion     Receives pointer of allocated version string.
 *                              The returned pointer must be freed using
 *                              VbglR3GuestPropReadValueFree().  Always set to
 *                              NULL.
 * @param   ppszGuestVersion    Receives pointer of allocated revision string.
 *                              The returned pointer must be freed using
 *                              VbglR3GuestPropReadValueFree().  Always set to
 *                              NULL.
 */
VBGLR3DECL(int) VbglR3HostVersionCheckForUpdate(HGCMCLIENTID idClient, bool *pfUpdate, char **ppszHostVersion, char **ppszGuestVersion)
{
#ifdef VBOX_WITH_GUEST_PROPS
    Assert(idClient > 0);
    AssertPtr(pfUpdate);
    AssertPtr(ppszHostVersion);
    AssertPtr(ppszGuestVersion);

    *ppszHostVersion = NULL;
    *ppszGuestVersion = NULL;

    /* We assume we have an update initially.
       Every block down below is allowed to veto */
    *pfUpdate = true;

    /* Do we need to do all this stuff? */
    char *pszCheckHostVersion;
    int rc = VbglR3GuestPropReadValueAlloc(idClient, "/VirtualBox/GuestAdd/CheckHostVersion", &pszCheckHostVersion);
    if (RT_FAILURE(rc))
    {
        if (rc == VERR_NOT_FOUND)
            rc = VINF_SUCCESS; /* If we don't find the value above we do the check by default */
        else
            LogFlow(("Could not read check host version flag! rc = %Rrc\n", rc));
    }
    else
    {
        /* Only don't do the check if we have a valid "0" in it */
        if (!strcmp(pszCheckHostVersion, "0"))
        {
            LogRel(("No host version update check performed (disabled).\n"));
            *pfUpdate = false;
        }
        VbglR3GuestPropReadValueFree(pszCheckHostVersion);
    }

    /* Collect all needed information */
    /* Make sure we only notify the user once by comparing the host version with
     * the last checked host version (if any) */
    if (RT_SUCCESS(rc) && *pfUpdate)
    {
        /* Look up host version */
        rc = VbglR3GuestPropReadValueAlloc(idClient, "/VirtualBox/HostInfo/VBoxVer", ppszHostVersion);
        if (RT_FAILURE(rc))
        {
            LogFlow(("Could not read VBox host version! rc = %Rrc\n", rc));
        }
        else
        {
            LogFlow(("Host version: %s\n", *ppszHostVersion));

            /* Get last checked host version */
            char *pszLastCheckedHostVersion;
            rc = VbglR3HostVersionLastCheckedLoad(idClient, &pszLastCheckedHostVersion);
            if (RT_SUCCESS(rc))
            {
                LogFlow(("Last checked host version: %s\n", pszLastCheckedHostVersion));
                if (strcmp(*ppszHostVersion, pszLastCheckedHostVersion) == 0)
                    *pfUpdate = false; /* We already notified this version, skip */
                VbglR3GuestPropReadValueFree(pszLastCheckedHostVersion);
            }
            else if (rc == VERR_NOT_FOUND) /* Never wrote a last checked host version before */
            {
                LogFlow(("Never checked a host version before.\n"));
                rc = VINF_SUCCESS;
            }
        }

        /* Look up guest version */
        if (RT_SUCCESS(rc))
        {
            rc = VbglR3GetAdditionsVersion(ppszGuestVersion, NULL /* Extended version not needed here */,
                                           NULL /* Revision not needed here */);
            if (RT_FAILURE(rc))
                LogFlow(("Could not read VBox guest version! rc = %Rrc\n", rc));
        }
    }

    /* Do the actual version comparison (if needed, see block(s) above) */
    if (RT_SUCCESS(rc) && *pfUpdate)
    {
        if (RTStrVersionCompare(*ppszHostVersion, *ppszGuestVersion) > 0) /* Is host version greater than guest add version? */
        {
            /* Yay, we have an update! */
            LogRel(("Guest Additions update found! Please upgrade this machine to the latest Guest Additions.\n"));
        }
        else
        {
            /* How sad ... */
            *pfUpdate = false;
        }
    }

    /* Cleanup on failure */
    if (RT_FAILURE(rc))
    {
        if (*ppszHostVersion)
        {
            VbglR3GuestPropReadValueFree(*ppszHostVersion);
            *ppszHostVersion = NULL;
        }
        if (*ppszGuestVersion)
        {
            VbglR3GuestPropReadValueFree(*ppszGuestVersion);
            *ppszGuestVersion = NULL;
        }
    }
    return rc;
#else /* !VBOX_WITH_GUEST_PROPS */
    RT_NOREF(idClient, pfUpdate, ppszHostVersion, ppszGuestVersion);
    return VERR_NOT_SUPPORTED;
#endif
}


/** Retrieves the last checked host version.
 *
 * @returns VBox status code.
 *
 * @param   idClient        The client id returned by VbglR3InfoSvcConnect().
 * @param   ppszVer         Receives pointer of allocated version string.
 *                          The returned pointer must be freed using RTStrFree() on VINF_SUCCESS.
 */
VBGLR3DECL(int) VbglR3HostVersionLastCheckedLoad(HGCMCLIENTID idClient, char **ppszVer)
{
#ifdef VBOX_WITH_GUEST_PROPS
    Assert(idClient > 0);
    AssertPtr(ppszVer);
    return VbglR3GuestPropReadValueAlloc(idClient, "/VirtualBox/GuestAdd/HostVerLastChecked", ppszVer);
#else /* !VBOX_WITH_GUEST_PROPS */
    RT_NOREF(idClient, ppszVer);
    return VERR_NOT_SUPPORTED;
#endif
}


/** Stores the last checked host version for later lookup.
 *  Requires strings in form of "majorVer.minorVer.build".
 *
 * @returns VBox status code.
 *
 * @param   idClient        The client id returned by VbglR3InfoSvcConnect().
 * @param   pszVer          Pointer to version string to store.
 */
VBGLR3DECL(int) VbglR3HostVersionLastCheckedStore(HGCMCLIENTID idClient, const char *pszVer)
{
#ifdef VBOX_WITH_GUEST_PROPS
    Assert(idClient > 0);
    AssertPtr(pszVer);
    return VbglR3GuestPropWriteValue(idClient, "/VirtualBox/GuestAdd/HostVerLastChecked", pszVer);
#else /* !VBOX_WITH_GUEST_PROPS */
    RT_NOREF(idClient, pszVer);
    return VERR_NOT_SUPPORTED;
#endif
}

