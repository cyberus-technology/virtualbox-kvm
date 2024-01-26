/* $Id: alloc-win.cpp $ */
/** @file
 * IPRT - Memory Allocation, Windows.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#ifdef IPRT_NO_CRT
# define USE_VIRTUAL_ALLOC
#endif
#define LOG_GROUP RTLOGGROUP_MEM
#include <iprt/win/windows.h>

#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/param.h>
#include <iprt/string.h>
#include <iprt/errcore.h>

#ifndef USE_VIRTUAL_ALLOC
# include <malloc.h>
#endif


RTDECL(void *) RTMemPageAllocTag(size_t cb, const char *pszTag) RT_NO_THROW_DEF
{
    RT_NOREF_PV(pszTag);

#ifdef USE_VIRTUAL_ALLOC
    void *pv = VirtualAlloc(NULL, RT_ALIGN_Z(cb, PAGE_SIZE), MEM_COMMIT, PAGE_READWRITE);
#else
    void *pv = _aligned_malloc(RT_ALIGN_Z(cb, PAGE_SIZE), PAGE_SIZE);
#endif
    AssertMsg(pv, ("cb=%d lasterr=%d\n", cb, GetLastError()));
    return pv;
}


RTDECL(void *) RTMemPageAllocExTag(size_t cb, uint32_t fFlags, const char *pszTag) RT_NO_THROW_DEF
{
    size_t const cbAligned = RT_ALIGN_Z(cb, PAGE_SIZE);
    RT_NOREF_PV(pszTag);
    AssertReturn(!(fFlags & ~RTMEMPAGEALLOC_F_VALID_MASK), NULL);

#ifdef USE_VIRTUAL_ALLOC
    void *pv = VirtualAlloc(NULL, cbAligned, MEM_COMMIT, PAGE_READWRITE);
#else
    void *pv = _aligned_malloc(cbAligned, PAGE_SIZE);
#endif
    AssertMsgReturn(pv, ("cb=%d lasterr=%d\n", cb, GetLastError()), NULL);

    if (fFlags & RTMEMPAGEALLOC_F_ADVISE_LOCKED)
    {
        /** @todo check why we get ERROR_WORKING_SET_QUOTA here. */
        BOOL const fOkay = VirtualLock(pv, cbAligned);
        AssertMsg(fOkay || GetLastError() == ERROR_WORKING_SET_QUOTA, ("pv=%p cb=%d lasterr=%d\n", pv, cb, GetLastError()));
        NOREF(fOkay);
    }

    if (fFlags & RTMEMPAGEALLOC_F_ZERO)
        RT_BZERO(pv, cbAligned);

    return pv;
}


RTDECL(void *) RTMemPageAllocZTag(size_t cb, const char *pszTag) RT_NO_THROW_DEF
{
    RT_NOREF_PV(pszTag);

#ifdef USE_VIRTUAL_ALLOC
    void *pv = VirtualAlloc(NULL, RT_ALIGN_Z(cb, PAGE_SIZE), MEM_COMMIT, PAGE_READWRITE);
#else
    void *pv = _aligned_malloc(RT_ALIGN_Z(cb, PAGE_SIZE), PAGE_SIZE);
#endif
    if (pv)
    {
        memset(pv, 0, RT_ALIGN_Z(cb, PAGE_SIZE));
        return pv;
    }
    AssertMsgFailed(("cb=%d lasterr=%d\n", cb, GetLastError()));
    return NULL;
}


RTDECL(void) RTMemPageFree(void *pv, size_t cb) RT_NO_THROW_DEF
{
    RT_NOREF_PV(cb);

    if (pv)
    {
#ifdef USE_VIRTUAL_ALLOC
        if (!VirtualFree(pv, 0, MEM_RELEASE))
            AssertMsgFailed(("pv=%p lasterr=%d\n", pv, GetLastError()));
#else
        _aligned_free(pv);
#endif
    }
}


RTDECL(int) RTMemProtect(void *pv, size_t cb, unsigned fProtect) RT_NO_THROW_DEF
{
    /*
     * Validate input.
     */
    if (cb == 0)
    {
        AssertMsgFailed(("!cb\n"));
        return VERR_INVALID_PARAMETER;
    }
    if (fProtect & ~(RTMEM_PROT_NONE | RTMEM_PROT_READ | RTMEM_PROT_WRITE | RTMEM_PROT_EXEC))
    {
        AssertMsgFailed(("fProtect=%#x\n", fProtect));
        return VERR_INVALID_PARAMETER;
    }

    /*
     * Convert the flags.
     */
    int fProt;
    Assert(!RTMEM_PROT_NONE);
    switch (fProtect & (RTMEM_PROT_NONE | RTMEM_PROT_READ | RTMEM_PROT_WRITE | RTMEM_PROT_EXEC))
    {
        case RTMEM_PROT_NONE:
            fProt = PAGE_NOACCESS;
            break;

        case RTMEM_PROT_READ:
            fProt = PAGE_READONLY;
            break;

        case RTMEM_PROT_READ | RTMEM_PROT_WRITE:
            fProt = PAGE_READWRITE;
            break;

        case RTMEM_PROT_READ | RTMEM_PROT_WRITE | RTMEM_PROT_EXEC:
            fProt = PAGE_EXECUTE_READWRITE;
            break;

        case RTMEM_PROT_READ | RTMEM_PROT_EXEC:
            fProt = PAGE_EXECUTE_READWRITE;
            break;

        case RTMEM_PROT_WRITE:
            fProt = PAGE_READWRITE;
            break;

        case RTMEM_PROT_WRITE | RTMEM_PROT_EXEC:
            fProt = PAGE_EXECUTE_READWRITE;
            break;

        case RTMEM_PROT_EXEC:
            fProt = PAGE_EXECUTE_READWRITE;
            break;

        /* If the compiler had any brains it would warn about this case. */
        default:
            AssertMsgFailed(("fProtect=%#x\n", fProtect));
            return VERR_INTERNAL_ERROR;
    }

    /*
     * Align the request.
     */
    cb += (uintptr_t)pv & PAGE_OFFSET_MASK;
    pv = (void *)((uintptr_t)pv & ~(uintptr_t)PAGE_OFFSET_MASK);

    /*
     * Change the page attributes.
     */
    DWORD fFlags = 0;
    if (VirtualProtect(pv, cb, fProt, &fFlags))
        return VINF_SUCCESS;
    return RTErrConvertFromWin32(GetLastError());
}

