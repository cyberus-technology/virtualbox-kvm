/** @file
 * Host-Guest Communication Manager (HGCM) - Service library definitions.
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

#ifndef VBOX_INCLUDED_hgcmsvc_h
#define VBOX_INCLUDED_hgcmsvc_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/assert.h>
#include <iprt/stdarg.h>
#include <iprt/string.h>
#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <iprt/err.h>
#ifdef IN_RING3
# include <iprt/mem.h>
# include <VBox/err.h>
# include <VBox/vmm/stam.h>
# include <VBox/vmm/dbgf.h>
# include <VBox/vmm/ssm.h>
#endif
#ifdef VBOX_TEST_HGCM_PARMS
# include <iprt/test.h>
#endif

/** @todo proper comments. */

/**
 * Service interface version.
 *
 * Includes layout of both VBOXHGCMSVCFNTABLE and VBOXHGCMSVCHELPERS.
 *
 * A service can work with these structures if major version
 * is equal and minor version of service is <= version of the
 * structures.
 *
 * For example when a new helper is added at the end of helpers
 * structure, then the minor version will be increased. All older
 * services still can work because they have their old helpers
 * unchanged.
 *
 * Revision history.
 * 1.1->2.1 Because the pfnConnect now also has the pvClient parameter.
 * 2.1->2.2 Because pfnSaveState and pfnLoadState were added
 * 2.2->3.1 Because pfnHostCall is now synchronous, returns rc, and parameters were changed
 * 3.1->3.2 Because pfnRegisterExtension was added
 * 3.2->3.3 Because pfnDisconnectClient helper was added
 * 3.3->4.1 Because the pvService entry and parameter was added
 * 4.1->4.2 Because the VBOX_HGCM_SVC_PARM_CALLBACK parameter type was added
 * 4.2->5.1 Removed the VBOX_HGCM_SVC_PARM_CALLBACK parameter type, as
 *          this problem is already solved by service extension callbacks
 * 5.1->6.1 Because pfnCall got a new parameter. Also new helpers. (VBox 6.0)
 * 6.1->6.2 Because pfnCallComplete starts returning a status code (VBox 6.0).
 * 6.2->6.3 Because pfnGetRequestor was added (VBox 6.0).
 * 6.3->6.4 Because pfnConnect got an additional parameter (VBox 6.0).
 * 6.4->6.5 Because pfnGetVMMDevSessionId was added pfnLoadState got the version
 *          parameter (VBox 6.0).
 * 6.5->7.1 Because pfnNotify was added (VBox 6.0).
 * 7.1->8.1 Because pfnCancelled & pfnIsCallCancelled were added (VBox 6.0).
 * 8.1->9.1 Because pfnDisconnectClient was (temporarily) removed, and
 *          acMaxClients and acMaxCallsPerClient added (VBox 6.1.26).
 * 9.1->10.1 Because pfnDisconnectClient was added back (VBox 6.1.28).
 * 10.1->11.1 Because pVMM added to pfnSaveState & pfnLoadState (VBox 7.0).
 */
#define VBOX_HGCM_SVC_VERSION_MAJOR (0x000b)
#define VBOX_HGCM_SVC_VERSION_MINOR (0x0001)
#define VBOX_HGCM_SVC_VERSION ((VBOX_HGCM_SVC_VERSION_MAJOR << 16) + VBOX_HGCM_SVC_VERSION_MINOR)


/** Typed pointer to distinguish a call to service. */
struct VBOXHGCMCALLHANDLE_TYPEDEF;
typedef struct VBOXHGCMCALLHANDLE_TYPEDEF *VBOXHGCMCALLHANDLE;

/** Service helpers pointers table. */
typedef struct VBOXHGCMSVCHELPERS
{
    /** The service has processed the Call request. */
    DECLR3CALLBACKMEMBER(int, pfnCallComplete, (VBOXHGCMCALLHANDLE callHandle, int32_t vrc));

    void *pvInstance;

    /**
     * The service disconnects the client.
     *
     * This can only be used during VBOXHGCMSVCFNTABLE::pfnConnect or
     * VBOXHGCMSVCFNTABLE::pfnDisconnect and will fail if called out side that
     * context.  Using this on the new client during VBOXHGCMSVCFNTABLE::pfnConnect
     * is not advisable, it would be better to just return a failure status for that
     * and it will be done automatically.  (It is not possible to call this method
     * on a client passed to VBOXHGCMSVCFNTABLE::pfnDisconnect.)
     *
     * There will be no VBOXHGCMSVCFNTABLE::pfnDisconnect callback for a client
     * disconnected in this manner.
     *
     * @returns VBox status code.
     * @retval  VERR_NOT_FOUND if the client ID was not found.
     * @retval  VERR_INVALID_CONTEXT if not called during connect or disconnect.
     *
     * @remarks Used by external parties, so don't remove just because we don't use
     *          it ourselves.
     */
    DECLR3CALLBACKMEMBER(int, pfnDisconnectClient, (void *pvInstance, uint32_t idClient));

    /**
     * Check if the @a callHandle is for a call restored and re-submitted from saved state.
     *
     * @returns true if restored, false if not.
     * @param   callHandle      The call we're checking up on.
     */
    DECLR3CALLBACKMEMBER(bool, pfnIsCallRestored, (VBOXHGCMCALLHANDLE callHandle));

    /**
     * Check if the @a callHandle is for a cancelled call.
     *
     * @returns true if cancelled, false if not.
     * @param   callHandle      The call we're checking up on.
     */
    DECLR3CALLBACKMEMBER(bool, pfnIsCallCancelled, (VBOXHGCMCALLHANDLE callHandle));

    /** Access to STAMR3RegisterV. */
    DECLR3CALLBACKMEMBER(int, pfnStamRegisterV,(void *pvInstance, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                                                STAMUNIT enmUnit, const char *pszDesc, const char *pszName, va_list va)
                                                RT_IPRT_FORMAT_ATTR(7, 0));
    /** Access to STAMR3DeregisterV. */
    DECLR3CALLBACKMEMBER(int, pfnStamDeregisterV,(void *pvInstance, const char *pszPatFmt, va_list va) RT_IPRT_FORMAT_ATTR(2, 0));

    /** Access to DBGFR3InfoRegisterExternal. */
    DECLR3CALLBACKMEMBER(int, pfnInfoRegister,(void *pvInstance, const char *pszName, const char *pszDesc,
                                               PFNDBGFHANDLEREXT pfnHandler, void *pvUser));
    /** Access to DBGFR3InfoDeregisterExternal. */
    DECLR3CALLBACKMEMBER(int, pfnInfoDeregister,(void *pvInstance, const char *pszName));

    /**
     * Retrieves the VMMDevRequestHeader::fRequestor value.
     *
     * @returns The field value, VMMDEV_REQUESTOR_LEGACY if not supported by the
     *          guest, VMMDEV_REQUESTOR_LOWEST if invalid call.
     * @param   hCall       The call we're checking up on.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnGetRequestor, (VBOXHGCMCALLHANDLE hCall));

    /**
     * Retrieves VMMDevState::idSession.
     *
     * @returns current VMMDev session ID value.
     */
    DECLR3CALLBACKMEMBER(uint64_t, pfnGetVMMDevSessionId, (void *pvInstance));

} VBOXHGCMSVCHELPERS;

typedef VBOXHGCMSVCHELPERS *PVBOXHGCMSVCHELPERS;

#if defined(IN_RING3) || defined(IN_SLICKEDIT)

/** Wrapper around STAMR3RegisterF. */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(7, 8)
HGCMSvcHlpStamRegister(PVBOXHGCMSVCHELPERS pHlp, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                       STAMUNIT enmUnit, const char *pszDesc, const char *pszName, ...)
{
    int rc;
    va_list va;
    va_start(va, pszName);
    rc = pHlp->pfnStamRegisterV(pHlp->pvInstance, pvSample, enmType, enmVisibility, enmUnit, pszDesc, pszName, va);
    va_end(va);
    return rc;
}

/** Wrapper around STAMR3RegisterV. */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(7, 0)
HGCMSvcHlpStamRegisterV(PVBOXHGCMSVCHELPERS pHlp, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                        STAMUNIT enmUnit, const char *pszDesc, const char *pszName, va_list va)
{
    return pHlp->pfnStamRegisterV(pHlp->pvInstance, pvSample, enmType, enmVisibility, enmUnit, pszDesc, pszName, va);
}

/** Wrapper around STAMR3DeregisterF. */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(2, 3) HGCMSvcHlpStamDeregister(PVBOXHGCMSVCHELPERS pHlp, const char *pszPatFmt, ...)
{
    int rc;
    va_list va;
    va_start(va, pszPatFmt);
    rc = pHlp->pfnStamDeregisterV(pHlp->pvInstance, pszPatFmt, va);
    va_end(va);
    return rc;
}

/** Wrapper around STAMR3DeregisterV. */
DECLINLINE(int) RT_IPRT_FORMAT_ATTR(2, 0) HGCMSvcHlpStamDeregisterV(PVBOXHGCMSVCHELPERS pHlp, const char *pszPatFmt, va_list va)
{
    return pHlp->pfnStamDeregisterV(pHlp->pvInstance, pszPatFmt, va);
}

/** Wrapper around DBGFR3InfoRegisterExternal. */
DECLINLINE(int) HGCMSvcHlpInfoRegister(PVBOXHGCMSVCHELPERS pHlp, const char *pszName, const char *pszDesc,
                                       PFNDBGFHANDLEREXT pfnHandler, void *pvUser)
{
    return pHlp->pfnInfoRegister(pHlp->pvInstance, pszName, pszDesc, pfnHandler, pvUser);
}

/** Wrapper around DBGFR3InfoDeregisterExternal. */
DECLINLINE(int) HGCMSvcHlpInfoDeregister(PVBOXHGCMSVCHELPERS pHlp, const char *pszName)
{
    return pHlp->pfnInfoDeregister(pHlp->pvInstance, pszName);
}

#endif /* IN_RING3 */


#define VBOX_HGCM_SVC_PARM_INVALID (0U)
#define VBOX_HGCM_SVC_PARM_32BIT   (1U)
#define VBOX_HGCM_SVC_PARM_64BIT   (2U)
#define VBOX_HGCM_SVC_PARM_PTR     (3U)
#define VBOX_HGCM_SVC_PARM_PAGES   (4U)

/** VBOX_HGCM_SVC_PARM_PAGES specific data. */
typedef struct VBOXHGCMSVCPARMPAGES
{
    uint32_t    cb;
    uint16_t    cPages;
    uint16_t    u16Padding;
    void      **papvPages;
} VBOXHGCMSVCPARMPAGES;
typedef VBOXHGCMSVCPARMPAGES *PVBOXHGCMSVCPARMPAGES;

typedef struct VBOXHGCMSVCPARM
{
    /** VBOX_HGCM_SVC_PARM_* values. */
    uint32_t type;

    union
    {
        uint32_t uint32;
        uint64_t uint64;
        struct
        {
            uint32_t size;
            void *addr;
        } pointer;
        /** VBOX_HGCM_SVC_PARM_PAGES */
        VBOXHGCMSVCPARMPAGES Pages;
    } u;
} VBOXHGCMSVCPARM;

/** Extract an uint32_t value from an HGCM parameter structure. */
DECLINLINE(int) HGCMSvcGetU32(VBOXHGCMSVCPARM *pParm, uint32_t *pu32)
{
    int rc = VINF_SUCCESS;
    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(pu32, VERR_INVALID_POINTER);
    if (pParm->type != VBOX_HGCM_SVC_PARM_32BIT)
        rc = VERR_INVALID_PARAMETER;
    if (RT_SUCCESS(rc))
        *pu32 = pParm->u.uint32;
    return rc;
}

/** Extract an uint64_t value from an HGCM parameter structure. */
DECLINLINE(int) HGCMSvcGetU64(VBOXHGCMSVCPARM *pParm, uint64_t *pu64)
{
    int rc = VINF_SUCCESS;
    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(pu64, VERR_INVALID_POINTER);
    if (pParm->type != VBOX_HGCM_SVC_PARM_64BIT)
        rc = VERR_INVALID_PARAMETER;
    if (RT_SUCCESS(rc))
        *pu64 = pParm->u.uint64;
    return rc;
}

/** Extract an pointer value from an HGCM parameter structure. */
DECLINLINE(int) HGCMSvcGetPv(VBOXHGCMSVCPARM *pParm, void **ppv, uint32_t *pcb)
{
    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(ppv, VERR_INVALID_POINTER);
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);
    if (pParm->type == VBOX_HGCM_SVC_PARM_PTR)
    {
        *ppv = pParm->u.pointer.addr;
        *pcb = pParm->u.pointer.size;
        return VINF_SUCCESS;
    }

    return VERR_INVALID_PARAMETER;
}

/** Extract a constant pointer value from an HGCM parameter structure. */
DECLINLINE(int) HGCMSvcGetPcv(VBOXHGCMSVCPARM *pParm, const void **ppv, uint32_t *pcb)
{
    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(ppv, VERR_INVALID_POINTER);
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);
    if (pParm->type == VBOX_HGCM_SVC_PARM_PTR)
    {
        *ppv = (const void *)pParm->u.pointer.addr;
        *pcb = pParm->u.pointer.size;
        return VINF_SUCCESS;
    }

    return VERR_INVALID_PARAMETER;
}

/** Extract a valid pointer to a non-empty buffer from an HGCM parameter
 * structure. */
DECLINLINE(int) HGCMSvcGetBuf(VBOXHGCMSVCPARM *pParm, void **ppv, uint32_t *pcb)
{
    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(ppv, VERR_INVALID_POINTER);
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);
    if (   pParm->type == VBOX_HGCM_SVC_PARM_PTR
        && RT_VALID_PTR(pParm->u.pointer.addr)
        && pParm->u.pointer.size > 0)
    {
        *ppv = pParm->u.pointer.addr;
        *pcb = pParm->u.pointer.size;
        return VINF_SUCCESS;
    }

    return VERR_INVALID_PARAMETER;
}

/** Extract a valid pointer to a non-empty constant buffer from an HGCM
 * parameter structure. */
DECLINLINE(int) HGCMSvcGetCBuf(VBOXHGCMSVCPARM *pParm, const void **ppv, uint32_t *pcb)
{
    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(ppv, VERR_INVALID_POINTER);
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);
    if (   pParm->type == VBOX_HGCM_SVC_PARM_PTR
        && RT_VALID_PTR(pParm->u.pointer.addr)
        && pParm->u.pointer.size > 0)
    {
        *ppv = (const void *)pParm->u.pointer.addr;
        *pcb = pParm->u.pointer.size;
        return VINF_SUCCESS;
    }

    return VERR_INVALID_PARAMETER;
}

/** Extract a string value from an HGCM parameter structure. */
DECLINLINE(int) HGCMSvcGetStr(VBOXHGCMSVCPARM *pParm, char **ppch, uint32_t *pcb)
{
    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(ppch, VERR_INVALID_POINTER);
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);
    if (   pParm->type == VBOX_HGCM_SVC_PARM_PTR
        && RT_VALID_PTR(pParm->u.pointer.addr)
        && pParm->u.pointer.size > 0)
    {
        int rc = RTStrValidateEncodingEx((char *)pParm->u.pointer.addr,
                                         pParm->u.pointer.size,
                                         RTSTR_VALIDATE_ENCODING_ZERO_TERMINATED);
        if (RT_FAILURE(rc))
            return rc;
        *ppch = (char *)pParm->u.pointer.addr;
        *pcb = pParm->u.pointer.size;
        return VINF_SUCCESS;
    }

    return VERR_INVALID_PARAMETER;
}

/** Extract a constant string value from an HGCM parameter structure. */
DECLINLINE(int) HGCMSvcGetCStr(VBOXHGCMSVCPARM *pParm, const char **ppch, uint32_t *pcb)
{
    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(ppch, VERR_INVALID_POINTER);
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);
    if (   pParm->type == VBOX_HGCM_SVC_PARM_PTR
        && RT_VALID_PTR(pParm->u.pointer.addr)
        && pParm->u.pointer.size > 0)
    {
        int rc = RTStrValidateEncodingEx((char *)pParm->u.pointer.addr,
                                         pParm->u.pointer.size,
                                         RTSTR_VALIDATE_ENCODING_ZERO_TERMINATED);
        if (RT_FAILURE(rc))
            return rc;
        *ppch = (char *)pParm->u.pointer.addr;
        *pcb = pParm->u.pointer.size;
        return VINF_SUCCESS;
    }

    return VERR_INVALID_PARAMETER;
}

/** Extract a constant string value from an HGCM parameter structure. */
DECLINLINE(int) HGCMSvcGetPsz(VBOXHGCMSVCPARM *pParm, const char **ppch, uint32_t *pcb)
{
    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(ppch, VERR_INVALID_POINTER);
    AssertPtrReturn(pcb, VERR_INVALID_POINTER);
    if (   pParm->type == VBOX_HGCM_SVC_PARM_PTR
        && RT_VALID_PTR(pParm->u.pointer.addr)
        && pParm->u.pointer.size > 0)
    {
        int rc = RTStrValidateEncodingEx((const char *)pParm->u.pointer.addr,
                                         pParm->u.pointer.size,
                                         RTSTR_VALIDATE_ENCODING_ZERO_TERMINATED);
        if (RT_FAILURE(rc))
            return rc;
        *ppch = (const char *)pParm->u.pointer.addr;
        *pcb = pParm->u.pointer.size;
        return VINF_SUCCESS;
    }

    return VERR_INVALID_PARAMETER;
}

/** Set a uint32_t value to an HGCM parameter structure */
DECLINLINE(void) HGCMSvcSetU32(VBOXHGCMSVCPARM *pParm, uint32_t u32)
{
    AssertPtr(pParm);
    pParm->type = VBOX_HGCM_SVC_PARM_32BIT;
    pParm->u.uint32 = u32;
}

/** Set a uint64_t value to an HGCM parameter structure */
DECLINLINE(void) HGCMSvcSetU64(VBOXHGCMSVCPARM *pParm, uint64_t u64)
{
    AssertPtr(pParm);
    pParm->type = VBOX_HGCM_SVC_PARM_64BIT;
    pParm->u.uint64 = u64;
}

/** Set a pointer value to an HGCM parameter structure */
DECLINLINE(void) HGCMSvcSetPv(VBOXHGCMSVCPARM *pParm, void *pv, uint32_t cb)
{
    AssertPtr(pParm);
    pParm->type = VBOX_HGCM_SVC_PARM_PTR;
    pParm->u.pointer.addr = pv;
    pParm->u.pointer.size = cb;
}

/** Set a pointer value to an HGCM parameter structure */
DECLINLINE(void) HGCMSvcSetStr(VBOXHGCMSVCPARM *pParm, const char *psz)
{
    AssertPtr(pParm);
    pParm->type = VBOX_HGCM_SVC_PARM_PTR;
    pParm->u.pointer.addr = (void *)psz;
    pParm->u.pointer.size = (uint32_t)strlen(psz) + 1;
}

#ifdef __cplusplus
# ifdef IPRT_INCLUDED_cpp_ministring_h
/** Set a const string value to an HGCM parameter structure */
DECLINLINE(void) HGCMSvcSetRTCStr(VBOXHGCMSVCPARM *pParm, const RTCString &rString)
{
    AssertPtr(pParm);
    pParm->type = VBOX_HGCM_SVC_PARM_PTR;
    pParm->u.pointer.addr = (void *)rString.c_str();
    pParm->u.pointer.size = (uint32_t)rString.length() + 1;
}
# endif
#endif

#if defined(IN_RING3) && defined(VBOX_INCLUDED_vmm_vmmr3vtable_h)

/**
 * Puts (serializes) a VBOXHGCMSVCPARM struct into SSM.
 *
 * @returns VBox status code.
 * @param   pParm   VBOXHGCMSVCPARM to serialize.
 * @param   pSSM    SSM handle to serialize to.
 * @param   pVMM    The VMM vtable.
 */
DECLINLINE(int) HGCMSvcSSMR3Put(VBOXHGCMSVCPARM *pParm, PSSMHANDLE pSSM, PCVMMR3VTABLE pVMM)
{
    int rc;

    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(pSSM,  VERR_INVALID_POINTER);

    rc = pVMM->pfnSSMR3PutU32(pSSM, sizeof(VBOXHGCMSVCPARM));
    AssertRCReturn(rc, rc);
    rc = pVMM->pfnSSMR3PutU32(pSSM, pParm->type);
    AssertRCReturn(rc, rc);

    switch (pParm->type)
    {
        case VBOX_HGCM_SVC_PARM_32BIT:
            rc = pVMM->pfnSSMR3PutU32(pSSM, pParm->u.uint32);
            break;
        case VBOX_HGCM_SVC_PARM_64BIT:
            rc = pVMM->pfnSSMR3PutU64(pSSM, pParm->u.uint64);
            break;
        case VBOX_HGCM_SVC_PARM_PTR:
            rc = pVMM->pfnSSMR3PutU32(pSSM, pParm->u.pointer.size);
            if (RT_SUCCESS(rc))
                rc = pVMM->pfnSSMR3PutMem(pSSM, pParm->u.pointer.addr, pParm->u.pointer.size);
            break;
        default:
            AssertMsgFailed(("Paramter type %RU32 not implemented yet\n", pParm->type));
            rc = VERR_NOT_IMPLEMENTED;
            break;
    }

    return rc;
}

/**
 * Gets (loads) a VBOXHGCMSVCPARM struct from SSM.
 *
 * @returns VBox status code.
 * @param   pParm   VBOXHGCMSVCPARM to load into. Must be zero'ed.
 * @param   pSSM    SSM handle to load from.
 * @param   pVMM    The VMM vtable.
 */
DECLINLINE(int) HGCMSvcSSMR3Get(VBOXHGCMSVCPARM *pParm, PSSMHANDLE pSSM, PCVMMR3VTABLE pVMM)
{
    uint32_t cbParm;
    int rc;

    AssertPtrReturn(pParm, VERR_INVALID_POINTER);
    AssertPtrReturn(pSSM,  VERR_INVALID_POINTER);

    rc = pVMM->pfnSSMR3GetU32(pSSM, &cbParm);
    AssertRCReturn(rc, rc);
    AssertReturn(cbParm == sizeof(VBOXHGCMSVCPARM), VERR_SSM_DATA_UNIT_FORMAT_CHANGED);

    rc = pVMM->pfnSSMR3GetU32(pSSM, &pParm->type);
    AssertRCReturn(rc, rc);

    switch (pParm->type)
    {
        case VBOX_HGCM_SVC_PARM_32BIT:
        {
            rc = pVMM->pfnSSMR3GetU32(pSSM, &pParm->u.uint32);
            AssertRCReturn(rc, rc);
            break;
        }

        case VBOX_HGCM_SVC_PARM_64BIT:
        {
            rc = pVMM->pfnSSMR3GetU64(pSSM, &pParm->u.uint64);
            AssertRCReturn(rc, rc);
            break;
        }

        case VBOX_HGCM_SVC_PARM_PTR:
        {
            AssertMsgReturn(pParm->u.pointer.size == 0,
                            ("Pointer size parameter already in use (or not initialized)\n"), VERR_INVALID_PARAMETER);

            rc = pVMM->pfnSSMR3GetU32(pSSM, &pParm->u.pointer.size);
            AssertRCReturn(rc, rc);

            AssertMsgReturn(pParm->u.pointer.addr == NULL,
                            ("Pointer parameter already in use (or not initialized)\n"), VERR_INVALID_PARAMETER);

            pParm->u.pointer.addr = RTMemAlloc(pParm->u.pointer.size);
            AssertPtrReturn(pParm->u.pointer.addr, VERR_NO_MEMORY);
            rc = pVMM->pfnSSMR3GetMem(pSSM, pParm->u.pointer.addr, pParm->u.pointer.size);

            AssertRCReturn(rc, rc);
            break;
        }

        default:
            AssertMsgFailedReturn(("Paramter type %RU32 not implemented yet\n", pParm->type),
                                  VERR_NOT_IMPLEMENTED);
            break;
    }

    return VINF_SUCCESS;
}

#endif /* IN_RING3 */

typedef VBOXHGCMSVCPARM *PVBOXHGCMSVCPARM;


/** Service specific extension callback.
 *  This callback is called by the service to perform service specific operation.
 *
 * @param pvExtension The extension pointer.
 * @param u32Function What the callback is supposed to do.
 * @param pvParm      The function parameters.
 * @param cbParms     The size of the function parameters.
 */
typedef DECLCALLBACKTYPE(int, FNHGCMSVCEXT,(void *pvExtension, uint32_t u32Function, void *pvParm, uint32_t cbParms));
typedef FNHGCMSVCEXT *PFNHGCMSVCEXT;

/**
 * Notification event.
 */
typedef enum HGCMNOTIFYEVENT
{
    HGCMNOTIFYEVENT_INVALID = 0,
    HGCMNOTIFYEVENT_POWER_ON,
    HGCMNOTIFYEVENT_RESUME,
    HGCMNOTIFYEVENT_SUSPEND,
    HGCMNOTIFYEVENT_RESET,
    HGCMNOTIFYEVENT_POWER_OFF,
    HGCMNOTIFYEVENT_END,
    HGCMNOTIFYEVENT_32BIT_HACK = 0x7fffffff
} HGCMNOTIFYEVENT;

/** @name HGCM_CLIENT_CATEGORY_XXX - Client categories
 * @{ */
#define HGCM_CLIENT_CATEGORY_KERNEL 0   /**< Guest kernel mode and legacy client. */
#define HGCM_CLIENT_CATEGORY_ROOT   1   /**< Guest root or admin client. */
#define HGCM_CLIENT_CATEGORY_USER   2   /**< Regular guest user client. */
#define HGCM_CLIENT_CATEGORY_MAX    3   /**< Max number of categories. */
/** @} */


/** The Service DLL entry points.
 *
 *  HGCM will call the DLL "VBoxHGCMSvcLoad"
 *  function and the DLL must fill in the VBOXHGCMSVCFNTABLE
 *  with function pointers.
 *
 *  @note The structure is used in separately compiled binaries so an explicit
 *        packing is required.
 */
typedef struct VBOXHGCMSVCFNTABLE
{
    /** @name Filled by HGCM
     * @{ */

    /** Size of the structure. */
    uint32_t                cbSize;

    /** Version of the structure, including the helpers. (VBOX_HGCM_SVC_VERSION) */
    uint32_t                u32Version;

    PVBOXHGCMSVCHELPERS     pHelpers;
    /** @} */

    /** @name Filled in by the service.
     * @{ */

    /** Size of client information the service want to have. */
    uint32_t                cbClient;

    /** The maximum number of clients per category.  Leave entries as zero for defaults. */
    uint32_t                acMaxClients[HGCM_CLIENT_CATEGORY_MAX];
    /** The maximum number of concurrent calls per client for each category.
     *  Leave entries as as zero for default. */
    uint32_t                acMaxCallsPerClient[HGCM_CLIENT_CATEGORY_MAX];
    /** The HGCM_CLIENT_CATEGORY_XXX value for legacy clients.
     *  Defaults to HGCM_CLIENT_CATEGORY_KERNEL. */
    uint32_t                idxLegacyClientCategory;

    /** Uninitialize service */
    DECLR3CALLBACKMEMBER(int, pfnUnload, (void *pvService));

    /** Inform the service about a client connection. */
    DECLR3CALLBACKMEMBER(int, pfnConnect, (void *pvService, uint32_t u32ClientID, void *pvClient, uint32_t fRequestor, bool fRestoring));

    /** Inform the service that the client wants to disconnect. */
    DECLR3CALLBACKMEMBER(int, pfnDisconnect, (void *pvService, uint32_t u32ClientID, void *pvClient));

    /** Service entry point.
     *  Return code is passed to pfnCallComplete callback.
     */
    DECLR3CALLBACKMEMBER(void, pfnCall, (void *pvService, VBOXHGCMCALLHANDLE callHandle, uint32_t u32ClientID, void *pvClient,
                                         uint32_t function, uint32_t cParms, VBOXHGCMSVCPARM paParms[], uint64_t tsArrival));
    /** Informs the service that a call was cancelled by the guest (optional).
     *
     * This is called for guest calls, connect requests and disconnect requests.
     * There is unfortunately no way of obtaining the call handle for a guest call
     * or otherwise identify the request, so that's left to the service to figure
     * out using VBOXHGCMSVCHELPERS::pfnIsCallCancelled.  Because this is an
     * asynchronous call, the service may have completed the request already.
     */
    DECLR3CALLBACKMEMBER(void, pfnCancelled, (void *pvService, uint32_t idClient, void *pvClient));

    /** Host Service entry point meant for privileged features invisible to the guest.
     *  Return code is passed to pfnCallComplete callback.
     */
    DECLR3CALLBACKMEMBER(int, pfnHostCall, (void *pvService, uint32_t function, uint32_t cParms, VBOXHGCMSVCPARM paParms[]));

    /** Inform the service about a VM save operation. */
    DECLR3CALLBACKMEMBER(int, pfnSaveState, (void *pvService, uint32_t u32ClientID, void *pvClient,
                                             PSSMHANDLE pSSM, PCVMMR3VTABLE pVMM));

    /** Inform the service about a VM load operation. */
    DECLR3CALLBACKMEMBER(int, pfnLoadState, (void *pvService, uint32_t u32ClientID, void *pvClient,
                                             PSSMHANDLE pSSM, PCVMMR3VTABLE pVMM, uint32_t uVersion));

    /** Register a service extension callback. */
    DECLR3CALLBACKMEMBER(int, pfnRegisterExtension, (void *pvService, PFNHGCMSVCEXT pfnExtension, void *pvExtension));

    /** Notification (VM state). */
    DECLR3CALLBACKMEMBER(void, pfnNotify, (void *pvService, HGCMNOTIFYEVENT enmEvent));

    /** User/instance data pointer for the service. */
    void *pvService;

    /** @} */
} VBOXHGCMSVCFNTABLE;


/** @name HGCM saved state
 * @note Need to be here so we can add saved to service which doesn't have it.
 * @{ */
/** HGCM saved state version. */
#define HGCM_SAVED_STATE_VERSION        3
/** HGCM saved state version w/o client state indicators. */
#define HGCM_SAVED_STATE_VERSION_V2     2
/** @} */


/** Service initialization entry point. */
typedef DECLCALLBACKTYPE(int, FNVBOXHGCMSVCLOAD,(VBOXHGCMSVCFNTABLE *ptable));
typedef FNVBOXHGCMSVCLOAD *PFNVBOXHGCMSVCLOAD;
#define VBOX_HGCM_SVCLOAD_NAME "VBoxHGCMSvcLoad"

#endif /* !VBOX_INCLUDED_hgcmsvc_h */
