; $Id:
;; @file
; Protected-mode APM implementation.
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
; SPDX-License-Identifier: GPL-3.0-only
;


include commondefs.inc

;; 16-bit protected mode APM entry point

_TEXT		segment public 'CODE'

extern		_apm_function:near	; implemented in C code


public		apm_pm16_entry

SET_DEFAULT_CPU_286


; APM function dispatch table
apm_disp:
		dw	offset apmf_disconnect	; 04h
		dw	offset apmf_idle	; 05h
		dw	offset apmf_busy	; 06h
		dw	offset apmf_set_state	; 07h
		dw	offset apmf_enable	; 08h
		dw	offset apmf_restore	; 09h
		dw	offset apmf_get_status	; 0Ah
		dw	offset apmf_get_event	; 0Bh
		dw	offset apmf_pwr_state	; 0Ch
		dw	offset apmf_dev_pm	; 0Dh
		dw	offset apmf_version	; 0Eh
		dw	offset apmf_engage	; 0Fh
		dw	offset apmf_get_caps	; 10h
apm_disp_end:

;
; APM worker routine. Function code in AL; it is assumed that AL >= 4.
; Caller must preserve BP.
;
apm_worker	proc	near

		sti			; TODO ?? necessary ??

		push	ax		; check if function is supported...
		xor	ah, ah
		sub	al, 4
		mov	bp, ax
		shl	bp, 1
		cmp	al, (apm_disp_end - apm_disp) / 2
		pop	ax
		mov	ah, 53h		; put back APM function
		jae	apmw_bad_func	; validate function range

		jmp	apm_disp[bp]	; and dispatch

apmf_disconnect:			; function 04h
		jmp	apmw_success

apmf_idle:				; function 05h
                ;
                ; Windows 3.1 POWER.DRV in Standard mode calls into APM
                ; with CPL=3. If that happens, the HLT instruction will fault
                ; and Windows will crash. To prevent that, we check the CPL
                ; and do nothing (better than crashing).
                ;
                push    cs
                pop     ax
                test    ax, 3           ; CPL > 0?
                jnz     apmw_success
		sti
		hlt
		jmp	apmw_success

apmf_busy:				; function 06h
;		jmp	apmw_success

apmf_set_state:				; function 07h
;		jmp	apmw_success

apmf_enable:				; function 08h
		jmp	apmw_success

apmf_restore:				; function 09h
;		jmp	apmw_success

apmf_get_status:			; function 0Ah
		jmp	apmw_bad_func

apmf_get_event:				; function 0Bh
		mov	ah, 80h
		jmp	apmw_failure

apmf_pwr_state:				; function 0Ch

apmf_dev_pm:				; function 0Dh
		jmp	apmw_bad_func

apmf_version:				; function 0Eh
		mov	ax, 0102h
		jmp	apmw_success

apmf_engage:				; function 0Fh
		; TODO do something?
		jmp	apmw_success

apmf_get_caps:				; function 10h
		mov	bl, 0		; no batteries
		mov	cx, 0		; no special caps
		jmp	apmw_success

apmw_success:
		clc			; successful return
		ret

apmw_bad_func:
		mov	ah, 09h		; unrecognized device ID - generic

apmw_failure:
		stc			; error for unsupported functions
		ret

apm_worker	endp


;; 16-bit protected mode APM entry point

;; According to the APM spec, only CS (16-bit code selector) is defined.
;; The data selector can be derived from it.

apm_pm16_entry:

		mov	ah, 2		; mark as originating in 16-bit PM

					; fall through

apm_pm16_entry_from_32:

		push	ds		; save registers
		push	bp

		push	cs
		pop	bp
		add	bp, 8		; calculate data selector
		mov	ds, bp		; load data segment

		call	apm_worker	; call APM handler

		pop	bp
		pop	ds		; restore registers

		retf			; return to caller - 16-bit return
					; even to 32-bit thunk!

_TEXT		ends


if VBOX_BIOS_CPU ge 80386

.386

BIOS32		segment	public 'CODE' use32

public		apm_pm32_entry

;; 32-bit protected mode APM entry point and thunk

;; According to the APM spec, only CS (32-bit) is defined. 16-bit code
;; selector and the data selector can be derived from it.

;; WARNING: To simplify matters, we use 16-bit far return to go from 32-bit
;; code to 16-bit and back. As a consequence, the 32-bit APM code must lie
;; below 64K boundary in the 32-bit APM code segment.

apm_pm32_entry:

		push	ebp		; ebp is not used by APM

		mov	bp, cs		; return address for 16-bit code
		push	bp
		mov	ebp, apm_pm32_back
		push	bp		; Note: 16:16 address!

		push	cs
		pop	ebp
		add	ebp, 8		; calculate 16-bit code selector
		push	bp		; push 16-bit code selector

		mov	ebp, apm_pm16_entry_from_32
		push	bp		; push 16-bit offset

		mov	ah, 3		; mark as originating in 32-bit PM

		db	66h		; force a 16-bit return
		retf			; off to 16-bit code...

apm_pm32_back:				; return here from 16-bit code

		pop	ebp		; restore scratch register
		retf

BIOS32		ends

endif		; 32-bit code

		end
