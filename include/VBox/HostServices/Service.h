/** @file
 * Base class for an host-guest service.
 */

/*
 * Copyright (C) 2011-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_HostServices_Service_h
#define VBOX_INCLUDED_HostServices_Service_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/log.h>
#include <VBox/hgcmsvc.h>

#include <iprt/assert.h>
#include <iprt/alloc.h>
#include <iprt/cpp/utils.h>

#include <new>


namespace HGCM
{

/**
 * Structure for keeping a HGCM service context.
 */
typedef struct VBOXHGCMSVCTX
{
    /** HGCM helper functions. */
    PVBOXHGCMSVCHELPERS pHelpers;
    /**
     * Callback function supplied by the host for notification of updates
     * to properties.
     */
    PFNHGCMSVCEXT       pfnHostCallback;
    /** User data pointer to be supplied to the host callback function. */
    void               *pvHostData;
} VBOXHGCMSVCTX, *PVBOXHGCMSVCTX;

/**
 * Base class encapsulating and working with a HGCM message.
 */
class Message
{
public:
    Message(void);
    Message(uint32_t uMsg, uint32_t cParms, VBOXHGCMSVCPARM aParms[]);
    virtual ~Message(void);

    uint32_t    GetParamCount(void) const RT_NOEXCEPT;
    int         GetData(uint32_t uMsg, uint32_t cParms, VBOXHGCMSVCPARM aParms[]) const RT_NOEXCEPT;
    int         GetParmU32(uint32_t uParm, uint32_t *pu32Info) const RT_NOEXCEPT;
    int         GetParmU64(uint32_t uParm, uint64_t *pu64Info) const RT_NOEXCEPT;
    int         GetParmPtr(uint32_t uParm, void **ppvAddr, uint32_t *pcbSize) const RT_NOEXCEPT;
    uint32_t    GetType(void) const RT_NOEXCEPT;

public:
    static int  CopyParms(PVBOXHGCMSVCPARM paParmsDst, uint32_t cParmsDst,
                          PVBOXHGCMSVCPARM paParmsSrc, uint32_t cParmsSrc,
                          bool fDeepCopy) RT_NOEXCEPT;

protected:
    int         initData(uint32_t uMsg, uint32_t cParms, VBOXHGCMSVCPARM aParms[]) RT_NOEXCEPT;
    void        reset() RT_NOEXCEPT;

protected:

    /** Stored message type. */
    uint32_t         m_uMsg;
    /** Number of stored HGCM parameters. */
    uint32_t         m_cParms;
    /** Stored HGCM parameters. */
    PVBOXHGCMSVCPARM m_paParms;
};

/**
 * Class for keeping and tracking a HGCM client.
 */
class Client
{
public:
    Client(uint32_t idClient);
    virtual ~Client(void);

public:
    int         Complete(VBOXHGCMCALLHANDLE hHandle, int rcOp = VINF_SUCCESS) RT_NOEXCEPT;
    int         CompleteDeferred(int rcOp = VINF_SUCCESS) RT_NOEXCEPT;
    uint32_t    GetClientID(void) const RT_NOEXCEPT;
    VBOXHGCMCALLHANDLE GetHandle(void) const RT_NOEXCEPT;
    uint32_t    GetMsgType(void) const RT_NOEXCEPT;
    uint32_t    GetMsgParamCount(void) const RT_NOEXCEPT;
    bool        IsDeferred(void) const RT_NOEXCEPT;
    void        SetDeferred(VBOXHGCMCALLHANDLE hHandle, uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM paParms[]) RT_NOEXCEPT;
    void        SetSvcContext(const VBOXHGCMSVCTX &SvcCtx) RT_NOEXCEPT;

public:
    int         SetDeferredMsgInfo(uint32_t uMsg, uint32_t cParms) RT_NOEXCEPT;
    int         SetDeferredMsgInfo(const Message *pMessage) RT_NOEXCEPT;

protected:
    int         completeInternal(VBOXHGCMCALLHANDLE hHandle, int rcOp) RT_NOEXCEPT;
    void        reset(void) RT_NOEXCEPT;

protected:
    /** The client's HGCM client ID. */
    uint32_t           m_idClient;
    /** The HGCM service context this client is bound to. */
    VBOXHGCMSVCTX      m_SvcCtx;
    /** Flag indicating whether this client currently is deferred mode,
     *  meaning that it did not return to the caller yet. */
    bool               m_fDeferred;
    /** Structure for keeping the client's deferred state.
     *  A client is in a deferred state when it asks for the next HGCM message,
     *  but the service can't provide it yet. That way a client will block (on the guest side, does not return)
     *  until the service can complete the call. */
    struct
    {
        /** The client's HGCM call handle. Needed for completing a deferred call. */
        VBOXHGCMCALLHANDLE hHandle;
        /** Message type (function number) to use when completing the deferred call.
         * @todo r=bird: uType or uMsg? Make up your mind (Message::m_uMsg).  */
        uint32_t           uType;
        /** Parameter count to use when completing the deferred call. */
        uint32_t           cParms;
        /** Parameters to use when completing the deferred call. */
        PVBOXHGCMSVCPARM   paParms;
    } m_Deferred;
};

template <class T>
class AbstractService : public RTCNonCopyable
{
public:
    /**
     * @copydoc FNVBOXHGCMSVCLOAD
     */
    static DECLCALLBACK(int) svcLoad(VBOXHGCMSVCFNTABLE *pTable)
    {
        LogFlowFunc(("ptable = %p\n", pTable));
        int rc = VINF_SUCCESS;

        if (!RT_VALID_PTR(pTable))
            rc = VERR_INVALID_PARAMETER;
        else
        {
            LogFlowFunc(("ptable->cbSize = %d, ptable->u32Version = 0x%08X\n", pTable->cbSize, pTable->u32Version));

            if (   pTable->cbSize != sizeof (VBOXHGCMSVCFNTABLE)
                || pTable->u32Version != VBOX_HGCM_SVC_VERSION)
                rc = VERR_VERSION_MISMATCH;
            else
            {
                AbstractService *pService = NULL;
                /* No exceptions may propagate outside (callbacks like this one are nothrow/noexcept). */
                try { pService = new T(pTable->pHelpers); }
                catch (std::bad_alloc &) { rc = VERR_NO_MEMORY; }
                catch (...)              { rc = VERR_UNEXPECTED_EXCEPTION; }
                if (RT_SUCCESS(rc))
                {
                    /* We don't need an additional client data area on the host,
                       because we're a class which can have members for that :-). */
                    /** @todo r=bird: What the comment above says is that we can duplicate the
                     * work of associating data with a client ID already done by the HGCM and create
                     * additional bugs because we think that's cool.   It's not. Utterly
                     * appalling as well as inefficient.  Just a structure with a pointer to a
                     * client base class would go a long way here. */
                    pTable->cbClient              = 0;

                    /* These functions are mandatory */
                    pTable->pfnUnload             = svcUnload;
                    pTable->pfnConnect            = svcConnect;
                    pTable->pfnDisconnect         = svcDisconnect;
                    pTable->pfnCall               = svcCall;
                    /* Clear obligatory functions. */
                    pTable->pfnHostCall           = NULL;
                    pTable->pfnSaveState          = NULL;
                    pTable->pfnLoadState          = NULL;
                    pTable->pfnRegisterExtension  = NULL;

                    /* Let the service itself initialize. */
                    rc = pService->init(pTable);
                    if (RT_SUCCESS(rc))
                        pTable->pvService = pService;
                    else
                        delete pService;
                }
            }
        }

        LogFlowFunc(("returning %Rrc\n", rc));
        return rc;
    }
    virtual ~AbstractService() {};

protected:
    explicit AbstractService(PVBOXHGCMSVCHELPERS pHelpers)
    {
        RT_ZERO(m_SvcCtx);
        m_SvcCtx.pHelpers = pHelpers;
    }
    virtual int  init(VBOXHGCMSVCFNTABLE *ptable) RT_NOEXCEPT
    { RT_NOREF1(ptable); return VINF_SUCCESS; }
    virtual int  uninit()  RT_NOEXCEPT
    { return VINF_SUCCESS; }
    virtual int  clientConnect(uint32_t idClient, void *pvClient) RT_NOEXCEPT = 0;
    virtual int  clientDisconnect(uint32_t idClient, void *pvClient) RT_NOEXCEPT = 0;
    virtual void guestCall(VBOXHGCMCALLHANDLE callHandle, uint32_t idClient, void *pvClient, uint32_t eFunction,
                           uint32_t cParms, VBOXHGCMSVCPARM paParms[]) RT_NOEXCEPT = 0;
    virtual int  hostCall(uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[]) RT_NOEXCEPT
    { RT_NOREF3(eFunction, cParms, paParms); return VINF_SUCCESS; }

    /** Type definition for use in callback functions. */
    typedef AbstractService SELF;
    /** The HGCM service context this service is bound to. */
    VBOXHGCMSVCTX m_SvcCtx;

    /**
     * @copydoc VBOXHGCMSVCFNTABLE::pfnUnload
     * Simply deletes the service object
     */
    static DECLCALLBACK(int) svcUnload(void *pvService)
    {
        AssertLogRelReturn(RT_VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        int rc = pSelf->uninit();
        AssertRC(rc);
        if (RT_SUCCESS(rc))
            delete pSelf;
        return rc;
    }

    /**
     * @copydoc VBOXHGCMSVCFNTABLE::pfnConnect
     * Stub implementation of pfnConnect and pfnDisconnect.
     */
    static DECLCALLBACK(int) svcConnect(void *pvService,
                                        uint32_t idClient,
                                        void *pvClient,
                                        uint32_t fRequestor,
                                        bool fRestoring)
    {
        RT_NOREF(fRequestor, fRestoring);
        AssertLogRelReturn(RT_VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        LogFlowFunc(("pvService=%p, idClient=%u, pvClient=%p\n", pvService, idClient, pvClient));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        int rc = pSelf->clientConnect(idClient, pvClient);
        LogFlowFunc(("rc=%Rrc\n", rc));
        return rc;
    }

    /**
     * @copydoc VBOXHGCMSVCFNTABLE::pfnConnect
     * Stub implementation of pfnConnect and pfnDisconnect.
     */
    static DECLCALLBACK(int) svcDisconnect(void *pvService,
                                           uint32_t idClient,
                                           void *pvClient)
    {
        AssertLogRelReturn(RT_VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        LogFlowFunc(("pvService=%p, idClient=%u, pvClient=%p\n", pvService, idClient, pvClient));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        int rc = pSelf->clientDisconnect(idClient, pvClient);
        LogFlowFunc(("rc=%Rrc\n", rc));
        return rc;
    }

    /**
     * @copydoc VBOXHGCMSVCFNTABLE::pfnCall
     * Wraps to the call member function
     */
    static DECLCALLBACK(void) svcCall(void *pvService,
                                      VBOXHGCMCALLHANDLE callHandle,
                                      uint32_t idClient,
                                      void *pvClient,
                                      uint32_t u32Function,
                                      uint32_t cParms,
                                      VBOXHGCMSVCPARM paParms[],
                                      uint64_t tsArrival)
    {
        AssertLogRelReturnVoid(RT_VALID_PTR(pvService));
        LogFlowFunc(("pvService=%p, callHandle=%p, idClient=%u, pvClient=%p, u32Function=%u, cParms=%u, paParms=%p\n",
                     pvService, callHandle, idClient, pvClient, u32Function, cParms, paParms));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        pSelf->guestCall(callHandle, idClient, pvClient, u32Function, cParms, paParms);
        LogFlowFunc(("returning\n"));
        RT_NOREF_PV(tsArrival);
    }

    /**
     * @copydoc VBOXHGCMSVCFNTABLE::pfnHostCall
     * Wraps to the hostCall member function
     */
    static DECLCALLBACK(int) svcHostCall(void *pvService,
                                         uint32_t u32Function,
                                         uint32_t cParms,
                                         VBOXHGCMSVCPARM paParms[])
    {
        AssertLogRelReturn(RT_VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        LogFlowFunc(("pvService=%p, u32Function=%u, cParms=%u, paParms=%p\n", pvService, u32Function, cParms, paParms));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        int rc = pSelf->hostCall(u32Function, cParms, paParms);
        LogFlowFunc(("rc=%Rrc\n", rc));
        return rc;
    }

    /**
     * @copydoc VBOXHGCMSVCFNTABLE::pfnRegisterExtension
     * Installs a host callback for notifications of property changes.
     */
    static DECLCALLBACK(int) svcRegisterExtension(void *pvService,
                                                  PFNHGCMSVCEXT pfnExtension,
                                                  void *pvExtension)
    {
        AssertLogRelReturn(RT_VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        LogFlowFunc(("pvService=%p, pfnExtension=%p, pvExtention=%p\n", pvService, pfnExtension, pvExtension));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        pSelf->m_SvcCtx.pfnHostCallback = pfnExtension;
        pSelf->m_SvcCtx.pvHostData      = pvExtension;
        return VINF_SUCCESS;
    }

    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP(AbstractService);
};

}
#endif /* !VBOX_INCLUDED_HostServices_Service_h */

