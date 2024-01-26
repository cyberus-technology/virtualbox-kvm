; $Id: bs3-cmn-TestQueryCfgU32.asm $
;; @file
; BS3Kit - Bs3TestQueryCfgU8.
;

;
; Copyright (C) 2007-2023 Oracle and/or its affiliates.
;
; This file is part of VirtualBox base platform packages, as
; available from https://www.virtualbox.org.
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation, in version 3 of the
; License.
;
; This program is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, see <https://www.gnu.org/licenses>.
;
; The contents of this file may alternatively be used under the terms
; of the Common Development and Distribution License Version 1.0
; (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
; in the VirtualBox distribution, in which case the provisions of the
; CDDL are applicable instead of those of the GPL.
;
; You may elect to license modified versions of this file under the
; terms and conditions of either the GPL or the CDDL or both.
;
; SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
;

%include "bs3kit-template-header.mac"
%include "VBox/VMMDevTesting.mac"

BS3_EXTERN_DATA16 g_fbBs3VMMDevTesting
TMPL_BEGIN_TEXT

;;
; @cproto   BS3_DECL(uint32_t) Bs3TestQueryCfgU32(uint16_t uCfg);
;
BS3_PROC_BEGIN_CMN Bs3TestQueryCfgU32, BS3_PBC_HYBRID
        BS3_CALL_CONV_PROLOG 1
        push    xBP
        mov     xBP, xSP
TNOT16  push    xDX

        cmp     byte [BS3_DATA16_WRT(g_fbBs3VMMDevTesting)], 0
        je      .no_vmmdev

        ; Issue the query command.
        mov     dx, VMMDEV_TESTING_IOPORT_CMD
%if TMPL_BITS == 16
        mov     ax, VMMDEV_TESTING_CMD_QUERY_CFG - VMMDEV_TESTING_CMD_MAGIC_HI_WORD
        out     dx, ax
%else
        mov     eax, VMMDEV_TESTING_CMD_QUERY_CFG
        out     dx, eax
%endif

        ; Write what we wish to query.
        mov     ax, [xBP + xCB + cbCurRetAddr]
        mov     dx, VMMDEV_TESTING_IOPORT_DATA
        out     dx, ax

        ; Read back the result.
%if TMPL_BITS == 16
        in      ax, dx
        push    ax
        in      ax, dx
        mov     dx, ax
        pop     ax
%else
        in      eax, dx
%endif

.return:
TNOT16  pop     xDX
        pop     xBP
        BS3_CALL_CONV_EPILOG 1
        BS3_HYBRID_RET

.no_vmmdev:
        xor     xAX, xAX
%if TMPL_BITS == 16
        xor     xDX, xDX
%endif
        jmp     .return
BS3_PROC_END_CMN   Bs3TestQueryCfgU32

