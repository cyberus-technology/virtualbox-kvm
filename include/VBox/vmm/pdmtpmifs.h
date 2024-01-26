/** @file
 * PDM - Pluggable Device Manager, TPM related interfaces.
 */

/*
 * Copyright (C) 2021-2023 Oracle and/or its affiliates.
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

#ifndef VBOX_INCLUDED_vmm_pdmtpmifs_h
#define VBOX_INCLUDED_vmm_pdmtpmifs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_pdm_ifs_tpm       PDM TPM Interfaces
 * @ingroup grp_pdm_interfaces
 * @{
 */


/** Pointer to a TPM port interface. */
typedef struct PDMITPMPORT *PPDMITPMPORT;
/**
 * TPM port interface (down).
 */
typedef struct PDMITPMPORT
{
    /**
     * @todo
     */
    DECLR3CALLBACKMEMBER(int, pfnDummy, (PPDMITPMPORT pInterface));

} PDMITPMPORT;
/** PDMITPMPORT interface ID. */
#define PDMITPMPORT_IID                       "1e57710f-f820-47ec-afa6-2713195f8f94"


/**
 * TPM version enumeration.
 */
typedef enum TPMVERSION
{
    /** Invalid TPM version, don't use. */
    TPMVERSION_INVALID = 0,
    /** TPM works according to version 1.2 of the specification. */
    TPMVERSION_1_2,
    /** TPM works according to version 2.0 of the specification. */
    TPMVERSION_2_0,
    /** TPM version is unknown. */
    TPMVERSION_UNKNOWN
} TPMVERSION;


/** Pointer to a TPM interface. */
typedef struct PDMITPMCONNECTOR *PPDMITPMCONNECTOR;
/**
 * TPM interface (up).
 * Pairs with PDMITPMPORT.
 */
typedef struct PDMITPMCONNECTOR
{
    /**
     * Returns the version of the TPM implemented by the driver below.
     *
     * @returns The TPM version.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     */
    DECLR3CALLBACKMEMBER(TPMVERSION, pfnGetVersion, (PPDMITPMCONNECTOR pInterface));

    /**
     * Returns the maximum supported locality of the driver below.
     *
     * @returns The maximum supported locality (0-4).
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnGetLocalityMax, (PPDMITPMCONNECTOR pInterface));

    /**
     * Returns the command/response buffer size of the driver below.
     *
     * @returns Buffer size in bytes.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnGetBufferSize, (PPDMITPMCONNECTOR pInterface));

    /**
     * Returns the status of the established flag.
     *
     * @returns Status of the established flag.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     */
    DECLR3CALLBACKMEMBER(bool, pfnGetEstablishedFlag, (PPDMITPMCONNECTOR pInterface));

    /**
     * Resets the TPM established flag.
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   bLoc                The locality issuing this request.
     */
    DECLR3CALLBACKMEMBER(int, pfnResetEstablishedFlag, (PPDMITPMCONNECTOR pInterface, uint8_t bLoc));

    /**
     * Executes the given command.
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     * @param   bLoc                The locality the command is issued from.
     * @param   pvCmd               Pointer to the command data.
     * @param   cbCmd               Size of the command in bytes.
     * @param   pvResp              Where to store the response data.
     * @param   cbResp              Size of the response buffer in bytes.
     */
    DECLR3CALLBACKMEMBER(int, pfnCmdExec, (PPDMITPMCONNECTOR pInterface, uint8_t bLoc, const void *pvCmd, size_t cbCmd, void *pvResp, size_t cbResp));

    /**
     * Cancels the currently executed command.
     *
     * @returns VBox status code.
     * @param   pInterface          Pointer to the interface structure containing the called function pointer.
     */
    DECLR3CALLBACKMEMBER(int, pfnCmdCancel, (PPDMITPMCONNECTOR pInterface));

} PDMITPMCONNECTOR;
/** PDMITPMCONNECTOR interface ID. */
#define PDMITPMCONNECTOR_IID                  "30afefd8-c11f-4e2a-a746-424e3d99fa86"

/** @} */

RT_C_DECLS_END

#endif /* !VBOX_INCLUDED_vmm_pdmtpmifs_h */
