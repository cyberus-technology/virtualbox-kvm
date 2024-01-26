;; ============================================================================================
;;
;;  Copyright (C) 2002 Jeroen Janssen
;;
;;  This library is free software; you can redistribute it and/or
;;  modify it under the terms of the GNU Lesser General Public
;;  License as published by the Free Software Foundation; either
;;  version 2 of the License, or (at your option) any later version.
;;
;;  This library is distributed in the hope that it will be useful,
;;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;;  Lesser General Public License for more details.
;;
;;  You should have received a copy of the GNU Lesser General Public
;;  License along with this library; if not, write to the Free Software
;;  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
;;
;; ============================================================================================
;;
;;  This VBE is part of the VGA Bios specific to the plex86/bochs Emulated VGA card.
;;  You can NOT drive any physical vga card with it.
;;
;; ============================================================================================
;;
;;  This VBE Bios is based on information taken from :
;;   - VESA BIOS EXTENSION (VBE) Core Functions Standard Version 3.0 located at www.vesa.org
;;
;; ============================================================================================


; Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
; other than GPL or LGPL is available it will apply instead, Oracle elects to use only
; the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
; a choice of LGPL license versions is made available with the language indicating
; that LGPLv2 or any later version may be used, or where a choice of which version
; of the LGPL is applied is otherwise unspecified.

include vgadefs.inc
include commondefs.inc

public  _vga_compat_setup
public  dispi_set_enable_
public  dispi_set_bank_
public  _dispi_set_bank_farcall
public  _dispi_get_max_bpp
public  _vbe_has_vbe_display

public  vbe_biosfn_return_current_mode
public  vbe_biosfn_display_window_control
public  vbe_biosfn_set_get_display_start
public  vbe_biosfn_set_get_dac_palette_format
public  vbe_biosfn_set_get_palette_data
public  vbe_biosfn_return_protected_mode_interface

VGAROM  segment public 'CODE'

SET_DEFAULT_CPU

VBE_BYTEWISE_IO EQU 1

;; Bytewise in/out
ifdef VBE_BYTEWISE_IO

public  do_out_dx_ax
public  do_in_ax_dx

do_out_dx_ax:
  xchg ah, al
  out  dx, al
  xchg ah, al
  out  dx, al
  ret

do_in_ax_dx:
  in   al, dx
  xchg ah, al
  in   al, dx
  ret

  out_dx_ax     EQU     call do_out_dx_ax
  in_ax_dx      EQU     call do_in_ax_dx
else
  out_dx_ax     EQU     out     dx, ax
  in_ax_dx      EQU     in      ax, dx
endif

;; Vertical retrace waiting
wait_vsync:
  push ax
  push dx
  mov  dx, 03DAh        ; @todo use a symbolic constant!
wv_loop:
  in   al, dx
  test al, 8
  jz   wv_loop
  pop  dx
  pop  ax
  ret

wait_not_vsync:
  push ax
  push dx
  mov  dx, 03DAh        ; @todo use a symbolic constant!
wnv_loop:
  in   al, dx
  test al, 8
  jnz  wnv_loop
  pop  dx
  pop  ax
  ret


; AL = bits per pixel / AH = bytes per pixel
dispi_get_bpp:
  push dx
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_BPP
  out_dx_ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  in_ax_dx
  cmp  al, 4
  jbe  get_bpp_noinc
  mov  ah, al
if VBOX_BIOS_CPU gt 8086
  shr  ah, 3
else
  shr  ah, 1
  shr  ah, 1
  shr  ah, 1
endif
  test al, 07
  jz   get_bpp_noinc
  inc  ah
get_bpp_noinc:
  pop  dx
  ret

; get display capabilities

_dispi_get_max_bpp:
  push dx
  push bx
  call dispi_get_enable
  mov  bx, ax
  or   ax, VBE_DISPI_GETCAPS
  call dispi_set_enable_
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_BPP
  out_dx_ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  in_ax_dx
  push ax
  mov  ax, bx
  call dispi_set_enable_
  pop  ax
  pop  bx
  pop  dx
  ret

dispi_set_enable_:
  push dx
  push ax
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_ENABLE
  out_dx_ax
  pop  ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  out_dx_ax
  pop  dx
  ret

dispi_get_enable:
  push dx
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_ENABLE
  out_dx_ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  in_ax_dx
  pop  dx
  ret

dispi_set_bank_:
  push dx
  push ax
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_BANK
  out_dx_ax
  pop  ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  out_dx_ax
  pop  dx
  ret

dispi_get_bank:
  push dx
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_BANK
  out_dx_ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  in_ax_dx
  pop  dx
  ret

_dispi_set_bank_farcall:
  cmp bx, 0100h
  je dispi_set_bank_farcall_get
  or bx,bx
  jnz dispi_set_bank_farcall_error
  mov ax, dx
  push dx
  push ax
  mov ax, VBE_DISPI_INDEX_BANK
  mov dx, VBE_DISPI_IOPORT_INDEX
  out_dx_ax
  pop ax
  mov dx, VBE_DISPI_IOPORT_DATA
  out_dx_ax
  in_ax_dx
  pop dx
  cmp dx,ax
  jne dispi_set_bank_farcall_error
  mov ax, 004Fh
  retf
dispi_set_bank_farcall_get:
  mov ax, VBE_DISPI_INDEX_BANK
  mov dx, VBE_DISPI_IOPORT_INDEX
  out_dx_ax
  mov dx, VBE_DISPI_IOPORT_DATA
  in_ax_dx
  mov dx,ax
  retf
dispi_set_bank_farcall_error:
  mov ax, 014Fh
  retf

dispi_set_x_offset:
  push dx
  push ax
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_X_OFFSET
  out_dx_ax
  pop  ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  out_dx_ax
  pop  dx
  ret

dispi_get_x_offset:
  push dx
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_X_OFFSET
  out_dx_ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  in_ax_dx
  pop  dx
  ret

dispi_set_y_offset:
  push dx
  push ax
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_Y_OFFSET
  out_dx_ax
  pop  ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  out_dx_ax
  pop  dx
  ret

dispi_get_y_offset:
  push dx
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_Y_OFFSET
  out_dx_ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  in_ax_dx
  pop  dx
  ret

vga_set_virt_width:
  push ax
  push bx
  push dx
  mov  bx, ax
  call dispi_get_bpp
  cmp  al, 4
  ja   set_width_svga
  shr  bx, 1
set_width_svga:
if VBOX_BIOS_CPU gt 8086
  shr  bx, 3
else
  shr  bx, 1
  shr  bx, 1
  shr  bx, 1
endif
  mov  dx, VGAREG_VGA_CRTC_ADDRESS
  mov  ah, bl
  mov  al, 13h
  out  dx, ax
  pop  dx
  pop  bx
  pop  ax
  ret

_vga_compat_setup:
  push ax
  push dx

  ; set CRT X resolution
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_XRES
  out_dx_ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  in_ax_dx
  push ax
  mov  dx, VGAREG_VGA_CRTC_ADDRESS
  mov  ax, 0011h
  out  dx, ax
  pop  ax
  push ax
if VBOX_BIOS_CPU gt 8086
  shr  ax, 3
else
  shr  ax, 1
  shr  ax, 1
  shr  ax, 1
endif
  dec  ax
  mov  ah, al
  mov  al, 01
  out  dx, ax
  pop  ax
  call vga_set_virt_width

  ; set CRT Y resolution
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_YRES
  out_dx_ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  in_ax_dx
  dec  ax
  push ax
  mov  dx, VGAREG_VGA_CRTC_ADDRESS
  mov  ah, al
  mov  al, 12h
  out  dx, ax
  pop  ax
  mov  al, 07
  out  dx, al
  inc  dx
  in   al, dx
  and  al, 0BDh
  test ah, 01
  jz   bit8_clear
  or   al, 02
bit8_clear:
  test ah, 02
  jz   bit9_clear
  or   al, 40h
bit9_clear:
  out  dx, al

  ; other settings
  mov  dx, VGAREG_VGA_CRTC_ADDRESS
  mov  ax, 0009
  out  dx, al
  mov  dx, VGAREG_VGA_CRTC_DATA
  in   al, dx
  and  al, 60h     ; clear double scan bit and cell height
  out  dx, al
  mov  dx, VGAREG_VGA_CRTC_ADDRESS
  mov  al, 17h
  out  dx, al
  mov  dx, VGAREG_VGA_CRTC_DATA
  in   al, dx
  or   al, 03
  out  dx, al
  mov  dx, VGAREG_ACTL_RESET
  in   al, dx
  mov  dx, VGAREG_ACTL_ADDRESS
  mov  al, 10h
  out  dx, al
  mov  dx, VGAREG_ACTL_READ_DATA
  in   al, dx
  or   al, 01
  mov  dx, VGAREG_ACTL_ADDRESS
  out  dx, al
  mov  al, 20h
  out  dx, al
  mov  dx, VGAREG_GRDC_ADDRESS
  mov  ax, 0506h
  out  dx, ax
  mov  dx, VGAREG_SEQU_ADDRESS
  mov  ax, 0F02h
  out  dx, ax

  ; settings for >= 8bpp
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_BPP
  out_dx_ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  in_ax_dx
  cmp  al, 08
  jb   vga_compat_end
  mov  dx, VGAREG_VGA_CRTC_ADDRESS
  mov  al, 14h
  out  dx, al
  mov  dx, VGAREG_VGA_CRTC_DATA
  in   al, dx
  or   al, 40h
  out  dx, al
  mov  dx, VGAREG_ACTL_RESET
  in   al, dx
  mov  dx, VGAREG_ACTL_ADDRESS
  mov  al, 10h
  out  dx, al
  mov  dx, VGAREG_ACTL_READ_DATA
  in   al, dx
  or   al, 40h
  mov  dx, VGAREG_ACTL_ADDRESS
  out  dx, al
  mov  al, 20h
  out  dx, al
  mov  dx, VGAREG_SEQU_ADDRESS
  mov  al, 04
  out  dx, al
  mov  dx, VGAREG_SEQU_DATA
  in   al, dx
  or   al, 08
  out  dx, al
  mov  dx, VGAREG_GRDC_ADDRESS
  mov  al, 05
  out  dx, al
  mov  dx, VGAREG_GRDC_DATA
  in   al, dx
  and  al, 9Fh
  or   al, 40h
  out  dx, al

vga_compat_end:
  pop  dx
  pop  ax


; Has VBE display - Returns true if VBE display detected

_vbe_has_vbe_display:
  push ds
  push bx
  mov  ax, BIOSMEM_SEG
  mov  ds, ax
  mov  bx, BIOSMEM_VBE_FLAG
  mov  al, [bx]
  and  al, 01
  xor  ah, ah
  pop  bx
  pop  ds
  ret


; Function 03h - Return Current VBE Mode
;
; Input:
;              AX      = 4F03h
; Output:
;              AX      = VBE Return Status
;              BX      = Current VBE Mode
;
;
vbe_biosfn_return_current_mode:
  push ds
  mov  ax, BIOSMEM_SEG
  mov  ds, ax
  call dispi_get_enable
  and  ax, VBE_DISPI_ENABLED
  jz   no_vbe_mode
  mov  bx, BIOSMEM_VBE_MODE
  mov  ax, [bx]
  mov  bx, ax
  jnz  vbe_03_ok
no_vbe_mode:
  mov  bx, BIOSMEM_CURRENT_MODE
  mov  al, [bx]
  mov  bl, al
  xor  bh, bh
vbe_03_ok:
  mov  ax, 004Fh
  pop  ds
  ret


; Function 05h - Display Window Control
;
; Input:
;              AX      = 4F05h
;     (16-bit) BH      = 00h Set memory window
;                      = 01h Get memory window
;              BL      = Window number
;                      = 00h Window A
;                      = 01h Window B
;              DX      = Window number in video memory in window
;                        granularity units (Set Memory Window only)
; Note:
;              If this function is called while in a linear frame buffer mode,
;              this function must fail with completion code AH=03h
;
; Output:
;              AX      = VBE Return Status
;              DX      = Window number in window granularity units
;                        (Get Memory Window only)

vbe_biosfn_display_window_control:
  cmp  bl, 0
  jne  vbe_05_failed
  cmp  bh, 1
  je   get_display_window
  jb   set_display_window
  mov  ax, 0100h
  ret
set_display_window:
  mov  ax, dx
  call dispi_set_bank_
  call dispi_get_bank
  cmp  ax, dx
  jne  vbe_05_failed
  mov  ax, 004Fh
  ret
get_display_window:
  call dispi_get_bank
  mov  dx, ax
  mov  ax, 004Fh
  ret
vbe_05_failed:
  mov  ax, 014Fh
  ret


; Function 07h - Set/Get Display Start
;
; Input(16-bit):
;              AX      = 4F07h
;              BH      = 00h Reserved and must be 00h
;              BL      = 00h Set Display Start
;                      = 01h Get Display Start
;                      = 02h Schedule Display Start (Alternate)
;                      = 03h Schedule Stereoscopic Display Start
;                      = 04h Get Scheduled Display Start Status
;                      = 05h Enable Stereoscopic Mode
;                      = 06h Disable Stereoscopic Mode
;                      = 80h Set Display Start during Vertical Retrace
;                      = 82h Set Display Start during Vertical Retrace (Alternate)
;                      = 83h Set Stereoscopic Display Start during Vertical Retrace
;              ECX     = If BL=02h/82h Display Start Address in bytes
;                        If BL=03h/83h Left Image Start Address in bytes
;              EDX     = If BL=03h/83h Right Image Start Address in bytes
;              CX      = If BL=00h/80h First Displayed Pixel In Scan Line
;              DX      = If BL=00h/80h First Displayed Scan Line
;
; Output:
;              AX      = VBE Return Status
;              BH      = If BL=01h Reserved and will be 0
;              CX      = If BL=01h First Displayed Pixel In Scan Line
;                        If BL=04h 0 if flip has not occurred, not 0 if it has
;              DX      = If BL=01h First Displayed Scan Line
;
; Input(32-bit):
;              BH      = 00h Reserved and must be 00h
;              BL      = 00h Set Display Start
;                      = 80h Set Display Start during Vertical Retrace
;              CX      = Bits 0-15 of display start address
;              DX      = Bits 16-31 of display start address
;              ES      = Selector for memory mapped registers
;
vbe_biosfn_set_get_display_start:
  cmp  bl, 80h
  je   set_display_start_wait
  cmp  bl, 1
  je   get_display_start
  jb   set_display_start
  mov  ax, 0100h
  ret
set_display_start_wait:
  call wait_not_vsync
  call wait_vsync
set_display_start:
  mov  ax, cx
  call dispi_set_x_offset
  mov  ax, dx
  call dispi_set_y_offset
  mov  ax, 004Fh
  ret
get_display_start:
  call dispi_get_x_offset
  mov  cx, ax
  call dispi_get_y_offset
  mov  dx, ax
  xor  bh, bh
  mov  ax, 004Fh
  ret


; Function 08h - Set/Get Dac Palette Format
;
; Input:
;              AX      = 4F08h
;              BL      = 00h set DAC palette width
;                      = 01h get DAC palette width
;              BH      = If BL=00h: desired number of bits per primary color
; Output:
;              AX      = VBE Return Status
;              BH      = current number of bits per primary color (06h = standard VGA)
;
vbe_biosfn_set_get_dac_palette_format:
  cmp  bl, 1
  je   get_dac_palette_format
  jb   set_dac_palette_format
  mov  ax, 0100h
  ret
set_dac_palette_format:
  call dispi_get_enable
  cmp  bh, 6
  je   set_normal_dac
  cmp  bh, 8
  jne  vbe_08_unsupported
  or   ax, VBE_DISPI_8BIT_DAC
  jnz  set_dac_mode
set_normal_dac:
  and  ax, NOT VBE_DISPI_8BIT_DAC
set_dac_mode:
  call dispi_set_enable_
get_dac_palette_format:
  mov  bh, 6
  call dispi_get_enable
  and  ax, VBE_DISPI_8BIT_DAC
  jz   vbe_08_ok
  mov  bh, 8
vbe_08_ok:
  mov  ax, 004Fh
  ret
vbe_08_unsupported:
  mov  ax, 014Fh
  ret


; Function 09h - Set/Get Palette Data
;
; Input:
;              AX      = 4F09h
;     (16-bit) BL      = 00h Set palette data
;                      = 01h Get palette data
;                      = 02h Set secondary palette data
;                      = 03h Get secondary palette data
;                      = 80h Set palette data during VRetrace
;              CX      = Number of entries to update (<= 256)
;              DX      = First entry to update
;              ES:DI   = Table of palette values
; Output:
;              AX      = VBE Return Status
;
; Notes:
;     Secondary palette support is a "future extension".
;     Attempts to set/get it should return status 02h.
;
;     In VBE 3.0, reading palette data is optional and
;     subfunctions 01h and 03h may return failure.
;
;     The format of palette entries is as follows:
;
;     PaletteEntry struc
;     Blue     db  ?   ; Blue channel value (6 or 8 bits)
;     Green    db  ?   ; Green channel value (6 or 8 bits)
;     Red      db  ?   ; Red channel value (6 or 8 bits)
;     Padding  db  ?   ; DWORD alignment byte (unused)
;     PaletteEntry ends
;
;     Most applications use VGA DAC registers directly to
;     set/get palette in VBE modes. However, subfn 4F09h is
;     required for NonVGA controllers (eg. XGA).
;
vbe_biosfn_set_get_palette_data:
  test bl, bl
  jz   set_palette_data
  cmp  bl, 01
  je   get_palette_data
  cmp  bl, 03
  jbe  vbe_09_nohw
  cmp  bl, 80h
  jne  vbe_09_unsupported
if 0
      ; this is where we could wait for vertical retrace
endif
set_palette_data:
  DO_pushad
  push  ds
  push  es
  pop   ds
  mov   al, dl
  mov   dx, VGAREG_DAC_WRITE_ADDRESS
  out   dx, al
  inc   dx
  mov   si, di
set_pal_loop:
if VBOX_BIOS_CPU ge 80386
  lodsd
  ror   eax, 16
  out   dx, al
  rol   eax, 8
  out   dx, al
  rol   eax, 8
  out   dx, al
else
  lodsw
  mov   bx, ax
  lodsw
  out   dx, al
  mov   al, bh
  out   dx, al
  mov   al, bl
  out   dx, al
endif
  loop  set_pal_loop
  pop   ds
  DO_popad
vbe_09_ok:
  mov  ax, 004Fh
  ret

get_palette_data:
  DO_pushad
  mov   al, dl
  mov   dx, VGAREG_DAC_READ_ADDRESS
  out   dx, al
  add   dl, 2
if VBOX_BIOS_CPU ge 80386
get_pal_loop:
  xor   eax, eax
  in    al, dx
  shl   eax, 8
  in    al, dx
  shl   eax, 8
  in    al, dx
  stosd
else
  xor   bx, bx
get_pal_loop:
  in    al, dx
  mov   bl, al
  in    al, dx
  mov   ah, al
  in    al, dx
  stosw
  mov   ax, bx
  stosw
endif
  loop  get_pal_loop
  DO_popad
  jmp   vbe_09_ok

vbe_09_unsupported:
  mov  ax, 014Fh
  ret
vbe_09_nohw:
  mov  ax, 024Fh
  ret


; Function 0Ah - Return VBE Protected Mode Interface
;
; Input:    AX   = 4F0Ah   VBE 2.0 Protected Mode Interface
;           BL   = 00h          Return protected mode table
; Output:   AX   =         Status
;           ES   =         Real Mode Segment of Table
;           DI   =         Offset of Table
;           CX   =         Length of Table including protected mode code
;                          (for copying purposes)
;
vbe_biosfn_return_protected_mode_interface:
  test bl, bl
  jnz _fail
  push cs
  pop es
  mov di, offset vesa_pm_start
  mov cx, vesa_pm_end - vesa_pm_start
  mov ax, 004Fh
  ret
_fail:
  mov ax, 014fh
  ret

VGAROM  ends

;;
;; 32-bit VBE interface
;;

.386

public  vesa_pm_start
public  vesa_pm_end

VBE32   segment public use32 'CODE'

        align   2

vesa_pm_start:
  dw vesa_pm_set_window - vesa_pm_start
  dw vesa_pm_set_display_start - vesa_pm_start
  dw vesa_pm_unimplemented - vesa_pm_start
  dw vesa_pm_io_ports_table - vesa_pm_start
vesa_pm_io_ports_table:
  dw VBE_DISPI_IOPORT_INDEX
  dw VBE_DISPI_IOPORT_INDEX + 1
  dw VBE_DISPI_IOPORT_DATA
  dw VBE_DISPI_IOPORT_DATA + 1
  dw 3B6h
  dw 3B7h
  dw 0FFFFh
  dw 0FFFFh

vesa_pm_set_window:
  cmp  bx, 0
  je  vesa_pm_set_display_window1
  mov  ax, 0100h
  ret
vesa_pm_set_display_window1:
  mov  ax, dx
  push dx
  push ax
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_BANK
  out  dx, ax
  pop  ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  out  dx, ax
  in   ax, dx
  pop  dx
  cmp  dx, ax
  jne  illegal_window
  mov  ax, 004Fh
  ret
illegal_window:
  mov  ax, 014Fh
  ret
vesa_pm_set_display_start:
  cmp  bl, 80h
  je   vesa_pm_set_display_start1_wait
  cmp  bl, 00
  je   vesa_pm_set_display_start1
  mov  ax, 0100h
  ret
vesa_pm_set_display_start1_wait:
  push edx
  mov  dx, 03DAh        ; @todo: use symbolic constant
wnv_loop_32:
  in   al, dx
  test al, 8
  jnz  wnv_loop_32
wv_loop_32:
  in   al, dx
  test al, 8
  jz   wv_loop_32
  pop  edx
vesa_pm_set_display_start1:
; convert offset to (X, Y) coordinate
; (would be simpler to change Bochs VBE API...)
  push eax
  push ecx
  push edx
  push esi
  push edi
  shl edx, 16
  and ecx, 0FFFFh
  or ecx, edx
  shl ecx, 2
  mov eax, ecx
  push eax
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_VIRT_WIDTH
  out  dx, ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  in   ax, dx
  movzx ecx, ax
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_BPP
  out  dx, ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  in   ax, dx
  movzx esi, ax
  pop  eax

  cmp esi, 4
  jz bpp4_mode
  add esi, 7
  shr esi, 3
  imul ecx, esi
  xor edx, edx
  div ecx
  mov edi, eax
  mov eax, edx
  xor edx, edx
  div esi
  jmp set_xy_regs

bpp4_mode:
  shr ecx, 1
  xor edx, edx
  div ecx
  mov edi, eax
  mov eax, edx
  shl eax, 1

set_xy_regs:
  push dx
  push ax
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_X_OFFSET
  out  dx, ax
  pop  ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  out  dx, ax
  pop  dx

  mov  ax, di
  push dx
  push ax
  mov  dx, VBE_DISPI_IOPORT_INDEX
  mov  ax, VBE_DISPI_INDEX_Y_OFFSET
  out  dx, ax
  pop  ax
  mov  dx, VBE_DISPI_IOPORT_DATA
  out  dx, ax
  pop  dx

  pop edi
  pop esi
  pop edx
  pop ecx
  pop eax
  mov  ax, 004fh
  ret

vesa_pm_unimplemented:
  mov ax, 014Fh
  ret
vesa_pm_end:

VBE32   ends

        end
