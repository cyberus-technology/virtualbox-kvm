;; ============================================================================================
;;
;;  Copyright (C) 2001,2002 the LGPL VGABios developers Team
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
;;  This VGA Bios is specific to the plex86/bochs Emulated VGA card.
;;  You can NOT drive any physical vga card with it.
;;
;; ============================================================================================
;;


; Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
; other than GPL or LGPL is available it will apply instead, Oracle elects to use only
; the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
; a choice of LGPL license versions is made available with the language indicating
; that LGPLv2 or any later version may be used, or where a choice of which version
; of the LGPL is applied is otherwise unspecified.

include vgadefs.inc
include commondefs.inc

public  vgabios_int10_handler

VGAROM  segment public 'CODE'

; Implemented in C
extrn   _int10_func:near
extrn   _vgabios_init_func:near

ifdef VBE
; Implemented in separate assembly module
extrn   vbe_biosfn_return_current_mode:near
extrn   vbe_biosfn_display_window_control:near
extrn   vbe_biosfn_set_get_display_start:near
extrn   vbe_biosfn_set_get_dac_palette_format:near
extrn   vbe_biosfn_set_get_palette_data:near
extrn   vbe_biosfn_return_protected_mode_interface:near
endif

ifdef VGA_DEBUG
extrn   _int10_debugmsg:near
extrn   _printf:near
extrn   _unimplemented:near
extrn   _unknown:near
endif

vgabios_start:

db      055h, 0AAh      ; ROM signature, required for expansion ROMs
db      40h             ; ROM module length in units of 512 bytes */


vgabios_entry_point:

  jmp vgabios_init

        org     1Eh

        db      'IBM',0

vgabios_init:
;; We have to set ds to access the right data segment
  push  cs
  pop   ds
  cld
  jmp _vgabios_init_func

;;
;;  int10 handled here
;;

SET_DEFAULT_CPU_286

vgabios_int10_handler:
  pushf
ifdef VGA_DEBUG
  push es
  push ds
  DO_pusha
  push  cs
  pop   ds
  cld
  call _int10_debugmsg
  DO_popa
  pop ds
  pop es
endif
  cmp   ah, 0Fh
  jne   int10_test_1A
  call  biosfn_get_video_mode
  jmp   int10_end
int10_test_1A:
  cmp   ah, 1Ah
  jne   int10_test_0B
  call  biosfn_group_1A
  jmp   int10_end
int10_test_0B:
  cmp   ah, 0Bh
  jne   int10_test_1103
  call  biosfn_group_0B
  jmp   int10_end
int10_test_1103:
  cmp   ax, 1103h
  jne   int10_test_12
  call  biosfn_set_text_block_specifier
  jmp   int10_end
int10_test_12:
  cmp   ah, 12h
  jne   int10_test_101B
  cmp   bl, 10h
  jne   int10_test_BL30
  call  biosfn_get_ega_info
  jmp   int10_end
int10_test_BL30:
  cmp   bl, 30h
  jne   int10_test_BL31
  call  biosfn_select_vert_res
  jmp   int10_end
int10_test_BL31:
  cmp   bl, 31h
  jne   int10_test_BL32
  call  biosfn_enable_default_palette_loading
  jmp   int10_end
int10_test_BL32:
  cmp   bl, 32h
  jne   int10_test_BL33
  call  biosfn_enable_video_addressing
  jmp   int10_end
int10_test_BL33:
  cmp   bl, 33h
  jne   int10_test_BL34
  call  biosfn_enable_grayscale_summing
  jmp   int10_end
int10_test_BL34:
  cmp   bl, 34h
  jne   int10_normal
  call  biosfn_enable_cursor_emulation
  jmp   int10_end
int10_test_101B:
  cmp   ax, 101Bh
  je    int10_normal
  cmp   ah, 10h
ifndef VBE
  jne   int10_normal
else
  jne   int10_test_4F
endif
  call  biosfn_group_10
  jmp   int10_end
ifdef VBE
int10_test_4F:
  cmp   ah, 4Fh
  jne   int10_normal
  cmp   al, 3
  jne   int10_test_vbe_05
  call  vbe_biosfn_return_current_mode
  jmp   int10_end
int10_test_vbe_05:
  cmp   al, 5
  jne   int10_test_vbe_07
  call  vbe_biosfn_display_window_control
  jmp   int10_end
int10_test_vbe_07:
  cmp   al, 7
  jne   int10_test_vbe_08
  call  vbe_biosfn_set_get_display_start
  jmp   int10_end
int10_test_vbe_08:
  cmp   al, 8
  jne   int10_test_vbe_09
  call  vbe_biosfn_set_get_dac_palette_format
  jmp   int10_end
int10_test_vbe_09:
  cmp   al, 9
  jne   int10_test_vbe_0A
  call  vbe_biosfn_set_get_palette_data
  jmp   int10_end
int10_test_vbe_0A:
  cmp   al, 0Ah
  jne   int10_normal
  call  vbe_biosfn_return_protected_mode_interface
  jmp   int10_end
endif

int10_normal:
  push es
  push ds
  DO_pusha

;; We have to set ds to access the right data segment
  push  cs
  pop   ds
  cld
  call _int10_func

  DO_popa
  pop ds
  pop es
int10_end:
  popf
  iret

;;--------------------------------------------------------------------------------------------

biosfn_group_0B:
  cmp   bh, 0
  je    biosfn_set_border_color
  cmp   bh, 1
  je    biosfn_set_palette
ifdef VGA_DEBUG
  call  _unknown
endif
  ret
biosfn_set_border_color:
  push  ax
  push  bx
  push  cx
  push  dx
  push  ds
  mov   dx, BIOSMEM_SEG
  mov   ds, dx
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  cmp   byte ptr ds:[BIOSMEM_CURRENT_MODE], 3
  jbe   set_border_done
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, 00h
  out   dx, al
  mov   al, bl
  and   al, 0Fh
  test  al, 08h
  jz    set_low_border
  add   al, 08h
set_low_border:
  out   dx, al
  mov   cl, 1
  and   bl, 10h
set_intensity_loop:
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, cl
  out   dx, al
  mov   dx, VGAREG_ACTL_READ_DATA
  in    al, dx
  and   al, 0EFh
  or    al, bl
  mov   dx, VGAREG_ACTL_ADDRESS
  out   dx, al
  inc   cl
  cmp   cl, 4
  jne   set_intensity_loop
set_border_done:
  mov   al, 20h
  out   dx, al
ifdef VBOX
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
endif ; VBOX
  pop   ds
  pop   dx
  pop   cx
  pop   bx
  pop   ax
  ret
biosfn_set_palette:
  push  ax
  push  bx
  push  cx
  push  dx
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   cl, 01
  and   bl, 01
set_cga_palette_loop:
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, cl
  out   dx, al
  mov   dx, VGAREG_ACTL_READ_DATA
  in    al, dx
  and   al, 0FEh
  or    al, bl
  mov   dx, VGAREG_ACTL_ADDRESS
  out   dx, al
  inc   cl
  cmp   cl, 4
  jne   set_cga_palette_loop
  mov   al, 20h
  out   dx, al
ifdef VBOX
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
endif ; VBOX
  pop   dx
  pop   cx
  pop   bx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_get_video_mode:
  push  ds
  mov   ax, BIOSMEM_SEG
  mov   ds, ax
  push  bx
  mov   bx, BIOSMEM_CURRENT_PAGE
  mov   al, [bx]
  pop   bx
  mov   bh, al
  push  bx
  mov   bx, BIOSMEM_VIDEO_CTL
  mov   ah, [bx]
  and   ah, 80h
  mov   bx, BIOSMEM_CURRENT_MODE
  mov   al, [bx]
  or    al, ah
  mov   bx, BIOSMEM_NB_COLS
  mov   ah, [bx]
  pop   bx
  pop   ds
  ret

;--------------------------------------------------------------------------------------------

biosfn_group_10:
  cmp   al, 0
  jne   int10_test_1001
  jmp   biosfn_set_single_palette_reg
int10_test_1001:
  cmp   al, 1
  jne   int10_test_1002
  jmp   biosfn_set_overscan_border_color
int10_test_1002:
  cmp   al, 2
  jne   int10_test_1003
  jmp   biosfn_set_all_palette_reg
int10_test_1003:
  cmp   al, 3
  jne   int10_test_1007
  jmp   biosfn_toggle_intensity
int10_test_1007:
  cmp   al, 7
  jne   int10_test_1008
  jmp   biosfn_get_single_palette_reg
int10_test_1008:
  cmp   al, 8
  jne   int10_test_1009
  jmp   biosfn_read_overscan_border_color
int10_test_1009:
  cmp   al, 9
  jne   int10_test_1010
  jmp   biosfn_get_all_palette_reg
int10_test_1010:
  cmp   al, 10h
  jne   int10_test_1012
  jmp  biosfn_set_single_dac_reg
int10_test_1012:
  cmp   al, 12h
  jne   int10_test_1013
  jmp   biosfn_set_all_dac_reg
int10_test_1013:
  cmp   al, 13h
  jne   int10_test_1015
  jmp   biosfn_select_video_dac_color_page
int10_test_1015:
  cmp   al, 15h
  jne   int10_test_1017
  jmp   biosfn_read_single_dac_reg
int10_test_1017:
  cmp   al, 17h
  jne   int10_test_1018
  jmp   biosfn_read_all_dac_reg
int10_test_1018:
  cmp   al, 18h
  jne   int10_test_1019
  jmp   biosfn_set_pel_mask
int10_test_1019:
  cmp   al, 19h
  jne   int10_test_101A
  jmp   biosfn_read_pel_mask
int10_test_101A:
  cmp   al, 1Ah
  jne   int10_group_10_unknown
  jmp   biosfn_read_video_dac_state
int10_group_10_unknown:
ifdef VGA_DEBUG
  call  _unknown
endif
  ret

biosfn_set_single_palette_reg:
  cmp   bl, 14h
  ja    no_actl_reg1
  push  ax
  push  dx
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, bl
  out   dx, al
  mov   al, bh
  out   dx, al
  mov   al, 20h
  out   dx, al
ifdef VBOX
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
endif ; VBOX
  pop   dx
  pop   ax
no_actl_reg1:
  ret

;--------------------------------------------------------------------------------------------

biosfn_set_overscan_border_color:
  push  bx
  mov   bl, 11h
  call  biosfn_set_single_palette_reg
  pop   bx
  ret

;--------------------------------------------------------------------------------------------

biosfn_set_all_palette_reg:
  push  ax
  push  bx
  push  cx
  push  dx
  mov   bx, dx
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   cl, 0
  mov   dx, VGAREG_ACTL_ADDRESS
set_palette_loop:
  mov   al, cl
  out   dx, al
  mov   al, es:[bx]
  out   dx, al
  inc   bx
  inc   cl
  cmp   cl, 10h
  jne   set_palette_loop
  mov   al, 11h
  out   dx, al
  mov   al, es:[bx]
  out   dx, al
  mov   al, 20h
  out   dx, al
ifdef VBOX
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
endif ; VBOX
  pop   dx
  pop   cx
  pop   bx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_toggle_intensity:
  push  ax
  push  bx
  push  dx
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, 10h
  out   dx, al
  mov   dx, VGAREG_ACTL_READ_DATA
  in    al, dx
  and   al, 0F7h
  and   bl, 01
if VBOX_BIOS_CPU gt 8086
  shl   bl, 3
else
  shl   bl, 1
  shl   bl, 1
  shl   bl, 1
endif
  or    al, bl
  mov   dx, VGAREG_ACTL_ADDRESS
  out   dx, al
  mov   al, 20h
  out   dx, al
ifdef VBOX
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
endif ; VBOX
  pop   dx
  pop   bx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_get_single_palette_reg:
  cmp   bl, 14h
  ja    no_actl_reg2
  push  ax
  push  dx
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, bl
  out   dx, al
  mov   dx, VGAREG_ACTL_READ_DATA
  in    al, dx
  mov   bh, al
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, 20h
  out   dx, al
ifdef VBOX
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
endif ; VBOX
  pop   dx
  pop   ax
no_actl_reg2:
  ret

;;--------------------------------------------------------------------------------------------

biosfn_read_overscan_border_color:
  push  ax
  push  bx
  mov   bl, 11h
  call  biosfn_get_single_palette_reg
  mov   al, bh
  pop   bx
  mov   bh, al
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_get_all_palette_reg:
  push  ax
  push  bx
  push  cx
  push  dx
  mov   bx, dx
  mov   cl, 0
get_palette_loop:
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, cl
  out   dx, al
  mov   dx, VGAREG_ACTL_READ_DATA
  in    al, dx
  mov   es:[bx], al
  inc   bx
  inc   cl
  cmp   cl, 10h
  jne   get_palette_loop
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, 11h
  out   dx, al
  mov   dx, VGAREG_ACTL_READ_DATA
  in    al, dx
  mov   es:[bx], al
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, 20h
  out   dx, al
ifdef VBOX
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
endif ; VBOX
  pop   dx
  pop   cx
  pop   bx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_set_single_dac_reg:
  push  ax
  push  dx
  mov   dx, VGAREG_DAC_WRITE_ADDRESS
  mov   al, bl
  out   dx, al
  mov   dx, VGAREG_DAC_DATA
  pop   ax
  push  ax
  mov   al, ah
  out   dx, al
  mov   al, ch
  out   dx, al
  mov   al, cl
  out   dx, al
  pop   dx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_set_all_dac_reg:
  push  ax
  push  bx
  push  cx
  push  dx
  mov   dx, VGAREG_DAC_WRITE_ADDRESS
  mov   al, bl
  out   dx, al
  pop   dx
  push  dx
  mov   bx, dx
  mov   dx, VGAREG_DAC_DATA
set_dac_loop:
  mov   al, es:[bx]
  out   dx, al
  inc   bx
  mov   al, es:[bx]
  out   dx, al
  inc   bx
  mov   al, es:[bx]
  out   dx, al
  inc   bx
  dec   cx
  jnz   set_dac_loop
  pop   dx
  pop   cx
  pop   bx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_select_video_dac_color_page:
  push  ax
  push  bx
  push  dx
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, 10h
  out   dx, al
  mov   dx, VGAREG_ACTL_READ_DATA
  in    al, dx
  and   bl, 01
  jnz   set_dac_page
  and   al, 07Fh
if VBOX_BIOS_CPU gt 8086
  shl   bh, 7
else
  shl   bh, 1
  shl   bh, 1
  shl   bh, 1
  shl   bh, 1
  shl   bh, 1
  shl   bh, 1
  shl   bh, 1
endif
  or    al, bh
  mov   dx, VGAREG_ACTL_ADDRESS
  out   dx, al
  jmp   set_actl_normal
set_dac_page:
  push  ax
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, 14h
  out   dx, al
  pop   ax
  and   al, 80h
  jnz   set_dac_16_page
if VBOX_BIOS_CPU gt 8086
  shl   bh, 2
else
  shl   bh, 1
  shl   bh, 1
endif
set_dac_16_page:
  and   bh, 0Fh
  mov   al, bh
  out   dx, al
set_actl_normal:
  mov   al, 20h
  out   dx, al
ifdef VBOX
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
endif ; VBOX
  pop   dx
  pop   bx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_read_single_dac_reg:
  push  ax
  push  dx
  mov   dx, VGAREG_DAC_READ_ADDRESS
  mov   al, bl
  out   dx, al
  pop   ax
  mov   ah, al
  mov   dx, VGAREG_DAC_DATA
  in    al, dx
  xchg  al, ah
  push  ax
  in    al, dx
  mov   ch, al
  in    al, dx
  mov   cl, al
  pop   dx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_read_all_dac_reg:
  push  ax
  push  bx
  push  cx
  push  dx
  mov   dx, VGAREG_DAC_READ_ADDRESS
  mov   al, bl
  out   dx, al
  pop   dx
  push  dx
  mov   bx, dx
  mov   dx, VGAREG_DAC_DATA
read_dac_loop:
  in    al, dx
  mov   es:[bx], al
  inc   bx
  in    al, dx
  mov   es:[bx], al
  inc   bx
  in    al, dx
  mov   es:[bx], al
  inc   bx
  dec   cx
  jnz   read_dac_loop
  pop   dx
  pop   cx
  pop   bx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_set_pel_mask:
  push  ax
  push  dx
  mov   dx, VGAREG_PEL_MASK
  mov   al, bl
  out   dx, al
  pop   dx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_read_pel_mask:
  push  ax
  push  dx
  mov   dx, VGAREG_PEL_MASK
  in    al, dx
  mov   bl, al
  pop   dx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_read_video_dac_state:
  push  ax
  push  dx
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, 10h
  out   dx, al
  mov   dx, VGAREG_ACTL_READ_DATA
  in    al, dx
  mov   bl, al
if VBOX_BIOS_CPU gt 8086
  shr   bl, 7
else
  shr   bl, 1
  shr   bl, 1
  shr   bl, 1
  shr   bl, 1
  shr   bl, 1
  shr   bl, 1
  shr   bl, 1
endif
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, 14h
  out   dx, al
  mov   dx, VGAREG_ACTL_READ_DATA
  in    al, dx
  mov   bh, al
  and   bh, 0Fh
  test  bl, 01
  jnz   get_dac_16_page
if VBOX_BIOS_CPU gt 8086
  shr   bh, 2
else
  shr   bh, 1
  shr   bh, 1
endif
get_dac_16_page:
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
  mov   dx, VGAREG_ACTL_ADDRESS
  mov   al, 20h
  out   dx, al
ifdef VBOX
  mov   dx, VGAREG_ACTL_RESET
  in    al, dx
endif ; VBOX
  pop   dx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_set_text_block_specifier:
  push  ax
  push  dx
  mov   dx, VGAREG_SEQU_ADDRESS
  mov   ah, bl
  mov   al, 03
  out   dx, ax
  pop   dx
  pop   ax
  ret

;;--------------------------------------------------------------------------------------------

biosfn_get_ega_info:
  push  ds
  push  ax
  mov   ax, BIOSMEM_SEG
  mov   ds, ax
  xor   ch, ch
  mov   bx, BIOSMEM_SWITCHES
  mov   cl, [bx]
  and   cl, 0Fh
  mov   bx, BIOSMEM_CRTC_ADDRESS
  mov   ax, [bx]
  mov   bx, 0003h
  cmp   ax, VGAREG_MDA_CRTC_ADDRESS
  jne   mode_ega_color
  mov   bh, 01
mode_ega_color:
  pop   ax
  pop   ds
  ret

;;--------------------------------------------------------------------------------------------

biosfn_select_vert_res:

; res : 00 200 lines, 01 350 lines, 02 400 lines

  push  ds
  push  bx
  push  dx
  mov   dl, al
  mov   ax, BIOSMEM_SEG
  mov   ds, ax
  mov   bx, BIOSMEM_MODESET_CTL
  mov   al, [bx]
  mov   bx, BIOSMEM_SWITCHES
  mov   ah, [bx]
  cmp   dl, 1
  je    vert_res_350
  jb    vert_res_200
  cmp   dl, 2
  je    vert_res_400
ifdef VGA_DEBUG
  mov   al, dl
  xor   ah, ah
  push  ax
  mov   bx, msg_vert_res
  push  bx
  call  _printf
  add   sp, 4
endif
  jmp   set_retcode
vert_res_400:

  ; reset modeset ctl bit 7 and set bit 4
  ; set switches bit 3-0 to 09

  and   al, 07Fh
  or    al, 010h
  and   ah, 0F0h
  or    ah, 009h
  jnz   set_vert_res
vert_res_350:

  ; reset modeset ctl bit 7 and bit 4
  ; set switches bit 3-0 to 09

  and   al, 06Fh
  and   ah, 0F0h
  or    ah, 009h
  jnz   set_vert_res
vert_res_200:

  ; set modeset ctl bit 7 and reset bit 4
  ; set switches bit 3-0 to 08

  and   al, 0EFh
  or    al, 080h
  and   ah, 0F0h
  or    ah, 008h
set_vert_res:
  mov   bx, BIOSMEM_MODESET_CTL
  mov   [bx], al
  mov   bx, BIOSMEM_SWITCHES
  mov   [bx], ah
set_retcode:
  mov   ax, 1212h
  pop   dx
  pop   bx
  pop   ds
  ret

ifdef VGA_DEBUG
msg_vert_res:
db "Select vert res (%02x) was discarded", 13, 10, 0
endif


biosfn_enable_default_palette_loading:
  push  ds
  push  bx
  push  dx
  mov   dl, al
  and   dl, 01
if VBOX_BIOS_CPU gt 8086
  shl   dl, 3
else
  shl   dl, 1
  shl   dl, 1
  shl   dl, 1
endif
  mov   ax, BIOSMEM_SEG
  mov   ds, ax
  mov   bx, BIOSMEM_MODESET_CTL
  mov   al, [bx]
  and   al, 0F7h
  or    al, dl
  mov   [bx], al
  mov   ax, 1212h
  pop   dx
  pop   bx
  pop   ds
  ret


biosfn_enable_video_addressing:
  push  bx
  push  dx
  mov   bl, al
  and   bl, 01
  xor   bl, 01
  shl   bl, 1
  mov   dx, VGAREG_READ_MISC_OUTPUT
  in    al, dx
  and   al, 0FDh
  or    al, bl
  mov   dx, VGAREG_WRITE_MISC_OUTPUT
  out   dx, al
  mov   ax, 1212h
  pop   dx
  pop   bx
  ret


biosfn_enable_grayscale_summing:
  push  ds
  push  bx
  push  dx
  mov   dl, al
  and   dl, 01h
  xor   dl, 01h
  shl   dl, 1
  mov   ax, BIOSMEM_SEG
  mov   ds, ax
  mov   bx, BIOSMEM_MODESET_CTL
  mov   al, [bx]
  and   al, 0FDh
  or    al, dl
  mov   [bx], al
  mov   ax, 1212h
  pop   dx
  pop   bx
  pop   ds
  ret


biosfn_enable_cursor_emulation:
  push  ds
  push  bx
  push  dx
  mov   dl, al
  and   dl, 01
  xor   dl, 01
  mov   ax, BIOSMEM_SEG
  mov   ds, ax
  mov   bx, BIOSMEM_MODESET_CTL
  mov   al, [bx]
  and   al, 0FEh
  or    al, dl
  mov   [bx], al
  mov   ax, 1212h
  pop   dx
  pop   bx
  pop   ds
  ret

;;--------------------------------------------------------------------------------------------

biosfn_group_1A:
  cmp   al, 0
  je    biosfn_read_display_code
  cmp   al, 1
  je    biosfn_set_display_code
ifdef VGA_DEBUG
  call  _unknown
endif
  ret
biosfn_read_display_code:
  push  ds
  push  ax
  mov   ax, BIOSMEM_SEG
  mov   ds, ax
  mov   bx, BIOSMEM_DCC_INDEX
  mov   al, [bx]
  mov   bl, al
  xor   bh, bh
  pop   ax
  mov   al, ah
  pop   ds
  ret
biosfn_set_display_code:
  push  ds
  push  ax
  push  bx
  mov   ax, BIOSMEM_SEG
  mov   ds, ax
  mov   ax, bx
  mov   bx, BIOSMEM_DCC_INDEX
  mov   [bx], al
ifdef VGA_DEBUG
  mov   al, ah
  xor   ah, ah
  push  ax
  mov   bx, msg_alt_dcc
  push  bx
  call  _printf
  add   sp, 4
endif
  pop   bx
  pop   ax
  mov   al, ah
  pop   ds
  ret

ifdef VGA_DEBUG
msg_alt_dcc:
db "Alternate Display code (%02x) was discarded", 13, 10, 0
endif

VGAROM  ends

        end
