/* $Id: tstPin.cpp $ */
/** @file
 * SUP Testcase - Memory locking interface (ring 3).
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
#include <VBox/sup.h>
#include <VBox/param.h>
#include <iprt/errcore.h>
#include <iprt/initterm.h>
#include <iprt/stream.h>
#include <iprt/thread.h>
#include <iprt/string.h>

#include "../SUPLibInternal.h"


int main(int argc, char **argv)
{
    int         rc;
    int         rcRet = 0;
    RTHCPHYS    HCPhys;

    RTR3InitExe(argc, &argv, RTR3INIT_FLAGS_TRY_SUPLIB);
    rc = SUPR3Init(NULL);
    RTPrintf("SUPR3Init -> rc=%d\n", rc);
    rcRet += rc != 0;
    if (!rc)
    {
        /*
         * Simple test.
         */
        void *pv;
        rc = SUPR3PageAlloc(1, 0, &pv);
        AssertRC(rc);
        RTPrintf("pv=%p\n", pv);
        SUPPAGE aPages[1];
        rc = supR3PageLock(pv, 1, &aPages[0]);
        RTPrintf("rc=%d pv=%p aPages[0]=%RHp\n", rc, pv, aPages[0]);
        RTThreadSleep(1500);
#if 0
        RTPrintf("Unlocking...\n");
        RTThreadSleep(250);
        rc = SUPPageUnlock(pv);
        RTPrintf("rc=%d\n", rc);
        RTThreadSleep(1500);
#endif

        /*
         * More extensive.
         */
        static struct
        {
            void       *pv;
            void       *pvAligned;
            SUPPAGE     aPages[16];
        } aPinnings[500];
        for (unsigned i = 0; i < sizeof(aPinnings) / sizeof(aPinnings[0]); i++)
        {
            aPinnings[i].pv = NULL;
            SUPR3PageAlloc(0x10000 >> PAGE_SHIFT, 0, &aPinnings[i].pv);
            aPinnings[i].pvAligned = RT_ALIGN_P(aPinnings[i].pv, PAGE_SIZE);
            rc = supR3PageLock(aPinnings[i].pvAligned, 0xf000 >> PAGE_SHIFT, &aPinnings[i].aPages[0]);
            if (!rc)
            {
                RTPrintf("i=%d: pvAligned=%p pv=%p:\n", i, aPinnings[i].pvAligned, aPinnings[i].pv);
                memset(aPinnings[i].pv, 0xfa, 0x10000);
                unsigned c4GPluss = 0;
                for (unsigned j = 0; j < (0xf000 >> PAGE_SHIFT); j++)
                    if (aPinnings[i].aPages[j].Phys >= _4G)
                    {
                        RTPrintf("%2d: vrt=%p phys=%RHp\n", j, (char *)aPinnings[i].pvAligned + (j << PAGE_SHIFT), aPinnings[i].aPages[j].Phys);
                        c4GPluss++;
                    }
                RTPrintf("i=%d: c4GPluss=%d\n", i, c4GPluss);
            }
            else
            {
                RTPrintf("SUPPageLock -> rc=%d\n", rc);
                rcRet++;
                SUPR3PageFree(aPinnings[i].pv, 0x10000 >> PAGE_SHIFT);
                aPinnings[i].pv = aPinnings[i].pvAligned = NULL;
                break;
            }
        }

        for (unsigned i = 0; i < sizeof(aPinnings) / sizeof(aPinnings[0]); i += 2)
        {
            if (aPinnings[i].pvAligned)
            {
                rc = supR3PageUnlock(aPinnings[i].pvAligned);
                if (rc)
                {
                    RTPrintf("SUPPageUnlock(%p) -> rc=%d\n", aPinnings[i].pvAligned, rc);
                    rcRet++;
                }
                memset(aPinnings[i].pv, 0xaf, 0x10000);
            }
        }

        for (unsigned i = 0; i < sizeof(aPinnings) / sizeof(aPinnings[0]); i += 2)
        {
            if (aPinnings[i].pv)
            {
                memset(aPinnings[i].pv, 0xcc, 0x10000);
                SUPR3PageFree(aPinnings[i].pv, 0x10000 >> PAGE_SHIFT);
                aPinnings[i].pv = NULL;
            }
        }


/* Support for allocating Ring-0 executable memory with contiguous physical backing isn't implemented on Solaris. */
#if !defined(RT_OS_SOLARIS)
        /*
         * Allocate a bit of contiguous memory.
         */
        pv = SUPR3ContAlloc(RT_ALIGN_Z(15003, PAGE_SIZE) >> PAGE_SHIFT, NULL, &HCPhys);
        rcRet += pv == NULL || HCPhys == 0;
        if (pv && HCPhys)
        {
            RTPrintf("SUPR3ContAlloc(15003) -> HCPhys=%llx pv=%p\n", HCPhys, pv);
            void *pv0 = pv;
            memset(pv0, 0xaf, 15003);
            pv = SUPR3ContAlloc(RT_ALIGN_Z(12999, PAGE_SIZE) >> PAGE_SHIFT, NULL, &HCPhys);
            rcRet += pv == NULL || HCPhys == 0;
            if (pv && HCPhys)
            {
                RTPrintf("SUPR3ContAlloc(12999) -> HCPhys=%llx pv=%p\n", HCPhys, pv);
                memset(pv, 0xbf, 12999);
                rc = SUPR3ContFree(pv, RT_ALIGN_Z(12999, PAGE_SIZE) >> PAGE_SHIFT);
                rcRet += rc != 0;
                if (rc)
                    RTPrintf("SUPR3ContFree failed! rc=%d\n", rc);
            }
            else
                RTPrintf("SUPR3ContAlloc (2nd) failed!\n");
            memset(pv0, 0xaf, 15003);
            /* pv0 is intentionally not freed! */
        }
        else
            RTPrintf("SUPR3ContAlloc failed!\n");
#endif

        /*
         * Allocate a big chunk of virtual memory and then lock it.
         */
        #define BIG_SIZE    72*1024*1024
        #define BIG_SIZEPP  (BIG_SIZE + PAGE_SIZE)
        pv = NULL;
        SUPR3PageAlloc(BIG_SIZEPP >> PAGE_SHIFT, 0, &pv);
        if (pv)
        {
            static SUPPAGE s_aPages[BIG_SIZE >> PAGE_SHIFT];
            void *pvAligned = RT_ALIGN_P(pv, PAGE_SIZE);
            rc = supR3PageLock(pvAligned, BIG_SIZE >> PAGE_SHIFT, &s_aPages[0]);
            if (!rc)
            {
                /* dump */
                RTPrintf("SUPPageLock(%p,%d,) succeeded!\n", pvAligned, BIG_SIZE);
                memset(pv, 0x42, BIG_SIZEPP);
                #if 0
                for (unsigned j = 0; j < (BIG_SIZE >> PAGE_SHIFT); j++)
                    RTPrintf("%2d: vrt=%p phys=%08x\n", j, (char *)pvAligned + (j << PAGE_SHIFT), (uintptr_t)s_aPages[j].pvPhys);
                #endif

                /* unlock */
                rc = supR3PageUnlock(pvAligned);
                if (rc)
                {
                    RTPrintf("SUPPageUnlock(%p) -> rc=%d\n", pvAligned, rc);
                    rcRet++;
                }
                memset(pv, 0xcc, BIG_SIZEPP);
            }
            else
            {
                RTPrintf("SUPPageLock(%p) -> rc=%d\n", pvAligned, rc);
                rcRet++;
            }
            SUPR3PageFree(pv, BIG_SIZEPP >> PAGE_SHIFT);
        }

        rc = SUPR3Term(false /*fForced*/);
        RTPrintf("SUPR3Term -> rc=%d\n", rc);
        rcRet += rc != 0;
    }

    return rcRet;
}
