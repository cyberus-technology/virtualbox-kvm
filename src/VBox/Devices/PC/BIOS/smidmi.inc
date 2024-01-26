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
; SPDX-License-Identifier: GPL-3.0-only
; --------------------------------------------------------------------
;
; This code is based on:
;
;  ROM BIOS for use with Bochs/Plex86/QEMU emulation environment
;
;  Copyright (C) 2002  MandrakeSoft S.A.
;
;    MandrakeSoft S.A.
;    43, rue d'Aboukir
;    75002 Paris - France
;    http://www.linux-mandrake.com/
;    http://www.mandrakesoft.com/
;
;  This library is free software; you can redistribute it and/or
;  modify it under the terms of the GNU Lesser General Public
;  License as published by the Free Software Foundation; either
;  version 2 of the License, or (at your option) any later version.
;
;  This library is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;  Lesser General Public License for more details.
;
;  You should have received a copy of the GNU Lesser General Public
;  License along with this library; if not, write to the Free Software
;  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
;
;

; Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
; other than GPL or LGPL is available it will apply instead, Oracle elects to use only
; the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
; a choice of LGPL license versions is made available with the language indicating
; that LGPLv2 or any later version may be used, or where a choice of which version
; of the LGPL is applied is otherwise unspecified.

include vbdmismi.inc

align 16
                ;;
                ;; NOTE! These are placeholders, also required for correct BIOS
                ;; checksum calculation. Overwritten by DevPcBios.
                ;;

		;; The SMBIOS header
		db	'_SM_'		; signature
		db	0		; checksum (set by biossums)
		db	01Fh		; EPS length, defined by standard
		db	VBOX_SMBIOS_MAJOR_VER	; SMBIOS major version
		db	VBOX_SMBIOS_MINOR_VER	; SMBIOS minor version
		dw	VBOX_SMBIOS_MAXSS	; Maximum structure size
		db	0		; Entry point revision
		db	0, 0, 0, 0, 0	; 16 bytes total

		;; The DMI header
		db	'_DMI_'		; signature
		db	0		; checksum (set by biossums)
		dw	0	        ; DMI tables length (set by DevPcBios)
		dd	VBOX_DMI_TABLE_BASE ; DMI tables base
		dw	0 ; DMI tables entries (set by DevPcBios)
		db	VBOX_DMI_TABLE_VER ; DMI version
		db	0		; Just for alignment (16 bytes total)

