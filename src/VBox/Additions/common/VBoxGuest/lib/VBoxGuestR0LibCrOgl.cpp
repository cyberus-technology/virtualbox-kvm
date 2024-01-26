/* $Id: VBoxGuestR0LibCrOgl.cpp $ */
/** @file
 * VBoxGuestLib - Ring-3 Support Library for VirtualBox guest additions, Chromium OpenGL Service.
 */

/*
 * Copyright (C) 2012-2023 Oracle and/or its affiliates.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/string.h>
#include "VBoxGuestR0LibInternal.h"

#ifdef VBGL_VBOXGUEST
# error "This file shouldn't be part of the VBoxGuestR0LibBase library that is linked into VBoxGuest.  It's client code."
#endif


DECLR0VBGL(int) VbglR0CrCtlCreate(VBGLCRCTLHANDLE *phCtl)
{
    int rc;

    if (phCtl)
    {
        struct VBGLHGCMHANDLEDATA *pHandleData = vbglR0HGCMHandleAlloc();
        if (pHandleData)
        {
            rc = VbglR0IdcOpen(&pHandleData->IdcHandle,
                               VBGL_IOC_VERSION /*uReqVersion*/,
                               VBGL_IOC_VERSION & UINT32_C(0xffff0000) /*uMinVersion*/,
                               NULL /*puSessionVersion*/, NULL /*puDriverVersion*/, NULL /*uDriverRevision*/);
            if (RT_SUCCESS(rc))
            {
                *phCtl = pHandleData;
                return VINF_SUCCESS;
            }

            vbglR0HGCMHandleFree(pHandleData);
        }
        else
            rc = VERR_NO_MEMORY;

        *phCtl = NULL;
    }
    else
        rc = VERR_INVALID_PARAMETER;

    return rc;
}

DECLR0VBGL(int) VbglR0CrCtlDestroy(VBGLCRCTLHANDLE hCtl)
{
    VbglR0IdcClose(&hCtl->IdcHandle);

    vbglR0HGCMHandleFree(hCtl);

    return VINF_SUCCESS;
}

DECLR0VBGL(int) VbglR0CrCtlConConnect(VBGLCRCTLHANDLE hCtl, HGCMCLIENTID *pidClient)
{
    VBGLIOCHGCMCONNECT info;
    int rc;

    if (!hCtl || !pidClient)
        return VERR_INVALID_PARAMETER;

    RT_ZERO(info);
    VBGLREQHDR_INIT(&info.Hdr, HGCM_CONNECT);
    info.u.In.Loc.type = VMMDevHGCMLoc_LocalHost_Existing;
    RTStrCopy(info.u.In.Loc.u.host.achName, sizeof(info.u.In.Loc.u.host.achName), "VBoxSharedCrOpenGL");
    rc = VbglR0IdcCall(&hCtl->IdcHandle, VBGL_IOCTL_HGCM_CONNECT, &info.Hdr, sizeof(info));
    if (RT_SUCCESS(rc))
    {
        Assert(info.u.Out.idClient);
        *pidClient = info.u.Out.idClient;
        return rc;
    }

    AssertRC(rc);
    *pidClient = 0;
    return rc;
}

DECLR0VBGL(int) VbglR0CrCtlConDisconnect(VBGLCRCTLHANDLE hCtl, HGCMCLIENTID idClient)
{
    VBGLIOCHGCMDISCONNECT info;
    VBGLREQHDR_INIT(&info.Hdr, HGCM_DISCONNECT);
    info.u.In.idClient = idClient;
    return VbglR0IdcCall(&hCtl->IdcHandle, VBGL_IOCTL_HGCM_DISCONNECT, &info.Hdr, sizeof(info));
}

DECLR0VBGL(int) VbglR0CrCtlConCallRaw(VBGLCRCTLHANDLE hCtl, PVBGLIOCHGCMCALL pCallInfo, int cbCallInfo)
{
    return VbglR0IdcCallRaw(&hCtl->IdcHandle, VBGL_IOCTL_HGCM_CALL(cbCallInfo), &pCallInfo->Hdr, cbCallInfo);
}

DECLR0VBGL(int) VbglR0CrCtlConCall(VBGLCRCTLHANDLE hCtl, PVBGLIOCHGCMCALL pCallInfo, int cbCallInfo)
{
    int rc = VbglR0IdcCallRaw(&hCtl->IdcHandle, VBGL_IOCTL_HGCM_CALL(cbCallInfo), &pCallInfo->Hdr, cbCallInfo);
    if (RT_SUCCESS(rc))
        rc = pCallInfo->Hdr.rc;
    return rc;
}

DECLR0VBGL(int) VbglR0CrCtlConCallUserDataRaw(VBGLCRCTLHANDLE hCtl, PVBGLIOCHGCMCALL pCallInfo, int cbCallInfo)
{
    return VbglR0IdcCallRaw(&hCtl->IdcHandle, VBGL_IOCTL_HGCM_CALL_WITH_USER_DATA(cbCallInfo), &pCallInfo->Hdr, cbCallInfo);
}

