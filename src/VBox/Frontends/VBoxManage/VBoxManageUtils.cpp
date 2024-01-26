/* $Id: VBoxManageUtils.cpp $ */
/** @file
 * VBoxManageUtils.h - VBoxManage utility functions.
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

#include "VBoxManageUtils.h"
#include "VBoxManage.h"

#include <iprt/message.h>
#include <iprt/string.h>

#include <VBox/com/array.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/string.h>

using namespace com;

DECLARE_TRANSLATION_CONTEXT(Utils);

unsigned int getMaxNics(const ComPtr<IVirtualBox> &pVirtualBox,
                        const ComPtr<IMachine> &pMachine)
{
    ULONG NetworkAdapterCount = 0;
    do {
        HRESULT hrc;

        ComPtr<ISystemProperties> info;
        CHECK_ERROR_BREAK(pVirtualBox, COMGETTER(SystemProperties)(info.asOutParam()));

        ChipsetType_T aChipset;
        CHECK_ERROR_BREAK(pMachine, COMGETTER(ChipsetType)(&aChipset));

        CHECK_ERROR_BREAK(info, GetMaxNetworkAdapters(aChipset, &NetworkAdapterCount));
    } while (0);

    return (unsigned int)NetworkAdapterCount;
}


/**
 * API does NOT verify that whether the interface name set as the
 * bridged or host-only interface of a NIC is valid.  Warn the user if
 * IHost doesn't seem to know about it (non-fatal).
 */
void verifyHostNetworkInterfaceName(const ComPtr<IVirtualBox> &pVirtualBox,
                                    const char *pszTargetName,
                                    HostNetworkInterfaceType_T enmTargetType)
{
    HRESULT hrc;

    AssertReturnVoid(   enmTargetType == HostNetworkInterfaceType_Bridged
                     || enmTargetType == HostNetworkInterfaceType_HostOnly);

    ComPtr<IHost> host;
    hrc = pVirtualBox->COMGETTER(Host)(host.asOutParam());
    if (FAILED(hrc))
        return;

    SafeIfaceArray<IHostNetworkInterface> ifs;
    hrc = host->COMGETTER(NetworkInterfaces)(ComSafeArrayAsOutParam(ifs));
    if (FAILED(hrc))
        return;

    for (size_t i = 0; i < ifs.size(); ++i)
    {
        const ComPtr<IHostNetworkInterface> iface = ifs[i];

        Bstr bstrName;
        hrc = iface->COMGETTER(Name)(bstrName.asOutParam());
        if (FAILED(hrc))
            return;

        if (!bstrName.equals(pszTargetName))
            continue;

        /* we found the interface but is it the right type? */
        HostNetworkInterfaceType_T enmType;
        hrc = iface->COMGETTER(InterfaceType)(&enmType);
        if (FAILED(hrc))
            return;

        if (enmType == enmTargetType)
            return;             /* seems ok */

        const char *pszTypeName;
        char a_szUnknownTypeBuf[32];
        switch (enmType)
        {
            case HostNetworkInterfaceType_Bridged:
                pszTypeName = Utils::tr("type bridged");
                break;

            case HostNetworkInterfaceType_HostOnly:
                pszTypeName = Utils::tr("type host-only");
                break;

            default:
                RTStrPrintf(a_szUnknownTypeBuf, sizeof(a_szUnknownTypeBuf),
                            Utils::tr("unknown type %RU32"), enmType);
                pszTypeName = a_szUnknownTypeBuf;
                break;
        }

        RTMsgWarning(Utils::tr("Interface \"%s\" is of %s"), pszTargetName, pszTypeName);
        return;
    }

    RTMsgWarning(Utils::tr("Interface \"%s\" doesn't seem to exist"), pszTargetName);
}
