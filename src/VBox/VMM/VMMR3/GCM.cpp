/** @file
 * GCM - Guest Compatibility Manager.
 */

/*
 * Copyright (C) 2022-2023 Oracle and/or its affiliates.
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

/** @page pg_gcm        GCM - The Guest Compatibility Manager
 *
 * The Guest Compatibility Manager provides run-time compatibility fixes
 * for certain known guest bugs.
 *
 * @see grp_gcm
 *
 *
 * @section sec_gcm_fixer   Fixers
 *
 * A GCM fixer implements a collection of run-time helpers/patches suitable for
 * a specific guest type. Several fixers can be active at the same time; for
 * example OS/2 or Windows 9x need their own fixers, but can also runs DOS
 * applications which need DOS-specific fixers.
 *
 * The concept of fixers exists to reduce the number of false positives to a
 * minimum. Heuristics are used to decide whether a particular fix should be
 * applied or not; restricting the number of applicable fixes minimizes the
 * chance that a fix could be misapplied.
 *
 * The fixers are invisible to a guest. A common problem is division by zero
 * caused by a software timing loop which cannot deal with fast CPUs (where
 * "fast" very much depends on the era when the software was written). A fixer
 * intercepts division by zero, recognizes known register contents and code
 * sequence, modifies one or more registers to avoid a divide error, and
 * restarts the instruction.
 *
 * It is not expected that the set of active fixers would be changed during
 * the lifetime of the VM.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_GIM
#include <VBox/vmm/gcm.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/pdmdev.h>
#include "GCMInternal.h"
#include <VBox/vmm/vm.h>

#include <VBox/log.h>

#include <iprt/err.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static FNSSMINTSAVEEXEC  gcmR3Save;
static FNSSMINTLOADEXEC  gcmR3Load;


/**
 * Initializes the GCM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) GCMR3Init(PVM pVM)
{
    LogFlow(("GCMR3Init\n"));

    /*
     * Assert alignment and sizes.
     */
    AssertCompile(sizeof(pVM->gcm.s) <= sizeof(pVM->gcm.padding));

    /*
     * Register the saved state data unit.
     */
    int rc = SSMR3RegisterInternal(pVM, "GCM", 0 /* uInstance */, GCM_SAVED_STATE_VERSION, sizeof(GCM),
                                   NULL /* pfnLivePrep */, NULL /* pfnLiveExec */, NULL /* pfnLiveVote*/,
                                   NULL /* pfnSavePrep */, gcmR3Save,              NULL /* pfnSaveDone */,
                                   NULL /* pfnLoadPrep */, gcmR3Load,              NULL /* pfnLoadDone */);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Read configuration.
     */
    PCFGMNODE pCfgNode = CFGMR3GetChild(CFGMR3GetRoot(pVM), "GCM/");

    /*
     * Validate the GCM settings.
     */
    rc = CFGMR3ValidateConfig(pCfgNode, "/GCM/",    /* pszNode */
                              "FixerSet",           /* pszValidValues */
                              "",                   /* pszValidNodes */
                              "GCM",                /* pszWho */
                              0);                   /* uInstance */
    if (RT_FAILURE(rc))
        return rc;

#if 1
    /** @cfgm{/GCM/FixerSet, uint32_t, 0}
     * The set (bit mask) of enabled fixers. See GCMFIXERID.
     */
    uint32_t    u32FixerIds;
    rc = CFGMR3QueryU32Def(pCfgNode, "FixerSet", &u32FixerIds, 0);
    AssertRCReturn(rc, rc);

    /* Check for unknown bits. */
    uint32_t    u32BadBits = u32FixerIds & ~(GCMFIXER_DBZ_DOS | GCMFIXER_DBZ_OS2 | GCMFIXER_DBZ_WIN9X);

    if (u32BadBits)
    {
        rc = VMR3SetError(pVM->pUVM, VERR_CFGM_CONFIG_UNKNOWN_VALUE, RT_SRC_POS, "Unsupported GCM fixer bits (%#x) set.", u32BadBits);
    }
    else
    {
        pVM->gcm.s.enmFixerIds = u32FixerIds;
    }
#else
    pVM->gcm.s.enmFixerIds = GCMFIXER_DBZ_OS2 | GCMFIXER_DBZ_DOS | GCMFIXER_DBZ_WIN9X;
#endif
    LogRel(("GCM: Initialized (fixer bits: %#x)\n", u32FixerIds));

    return rc;
}


/**
 * Finalize the GCM initialization.
 *
 * This is called after initializing HM and most other VMM components.
 *
 * @returns VBox status code.
 * @param   pVM                 The cross context VM structure.
 * @thread  EMT(0)
 */
VMMR3_INT_DECL(int) GCMR3InitCompleted(PVM pVM)
{
    RT_NOREF(pVM);
    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNSSMINTSAVEEXEC}
 */
static DECLCALLBACK(int) gcmR3Save(PVM pVM, PSSMHANDLE pSSM)
{
    AssertReturn(pVM,  VERR_INVALID_PARAMETER);
    AssertReturn(pSSM, VERR_SSM_INVALID_STATE);

    int rc = VINF_SUCCESS;

    /*
     * Save per-VM data.
     */
    SSMR3PutU32(pSSM, pVM->gcm.s.enmFixerIds);

    return rc;
}


/**
 * @callback_method_impl{FNSSMINTLOADEXEC}
 */
static DECLCALLBACK(int) gcmR3Load(PVM pVM, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    if (uPass != SSM_PASS_FINAL)
        return VINF_SUCCESS;
    if (uVersion != GCM_SAVED_STATE_VERSION)
        return VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION;

    int rc;

    /*
     * Load per-VM data.
     */
    uint32_t uFixerIds;

    rc = SSMR3GetU32(pSSM, &uFixerIds);
    AssertRCReturn(rc, rc);

    if ((GCMFIXERID)uFixerIds != pVM->gcm.s.enmFixerIds)
        return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Saved GCM fixer set %#X differs from the configured one (%#X)."),
                                uFixerIds, pVM->gcm.s.enmFixerIds);

    return VINF_SUCCESS;
}


/**
 * Terminates the GCM.
 *
 * Termination means cleaning up and freeing all resources,
 * the VM itself is, at this point, powered off or suspended.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) GCMR3Term(PVM pVM)
{
    RT_NOREF(pVM);
    return VINF_SUCCESS;
}


/**
 * Applies relocations to data and code managed by this
 * component. This function will be called at init and
 * whenever the VMM need to relocate itself inside the GC.
 *
 * @param   pVM         The cross context VM structure.
 * @param   offDelta    Relocation delta relative to old location.
 */
VMMR3_INT_DECL(void) GCMR3Relocate(PVM pVM, RTGCINTPTR offDelta)
{
    RT_NOREF(pVM);
    RT_NOREF(offDelta);
}


/**
 * The VM is being reset.
 *
 * Do whatever fixer-specific resetting that needs to be done.
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(void) GCMR3Reset(PVM pVM)
{
    RT_NOREF(pVM);
}

