/* $Id: vbox-tpm.dsl $ */
/** @file
 * VirtualBox ACPI - TPM ACPI device.
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
 * SPDX-License-Identifier: GPL-3.0-only
 */

DefinitionBlock ("SSDT.aml", "SSDT", 1, "VBOX  ", "VBOXTPMT", 2)
{
    Scope (\_SB)
    {
        Device (TPM)
        {
            Method (_HID, 0, NotSerialized)
            {
                If (LEqual(IFID, One))
                {
                    Return ("PNP0C31")
                }
                Else
                {
                    Return ("MSFT0101")
                }
            }

            Method (_CID, 0, NotSerialized)
            {
                If (LEqual(IFID, One))
                {
                    Return ("PNP0C31")
                }
                Else
                {
                    Return ("MSFT0101")
                }
            }

            Method (_STR, 0, NotSerialized)
            {
                If (LEqual(IFID, One))
                {
                    Return (Unicode ("TPM 1.2 Device"))
                }
                Else
                {
                    Return (Unicode ("TPM 2.0 Device"))
                }
            }

            Method (_STA, 0, NotSerialized)
            {
                Return (0x0F)
            }

            OperationRegion (TPMR, SystemMemory, 0xFED40000, 0x5000)
            Field(TPMR, AnyAcc, NoLock, Preserve)
            {
                Offset(0x30),
                IFID,       1,
            }

            Name(RES, ResourceTemplate()
            {
                Memory32Fixed (ReadWrite, 0xfed40000, 0x5000, REG1)
            })

            Method (_CRS, 0, Serialized)
            {
               Return (RES)
            }
        }
    }
}

/*
 * Local Variables:
 * comment-start: "//"
 * End:
 */

