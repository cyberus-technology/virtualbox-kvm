;; @file
; VBoxGuest - VirtualBox Guest Additions Interface, MASM/ALP header.
;

;
; Copyright (C) 2006-2023 Oracle and/or its affiliates.
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


;; ASSUMES OS/2 for now.
VBOXGUEST_DEVICE_NAME_SHORT EQU "vboxgst$"

VBGLOS2ATTACHDD STRUC
u32Version         DD ?
u32Session         DD ?
pfnServiceEP       DD ?
fpfnServiceEP      DD ?
fpfnServiceAsmEP   DD ?
VBGLOS2ATTACHDD ENDS

VBGLREQHDR STRUC
cbIn               DD ?
uVersion           DD ?
uType              DD ?
rc                 DD ?
cbOut              DD ?
uReserved          DD ?
VBGLREQHDR ENDS

VBGL_IOC_VERSION        EQU     000010000h

; From VMMDev.h
VMMDEV_VERSION_MAJOR    EQU     1
VMMDEV_VERSION_MINOR    EQU     4
VMMDEV_VERSION          EQU     000010004h

