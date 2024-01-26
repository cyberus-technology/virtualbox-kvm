// ============================================================================================
//
//  Copyright (C) 2002 Jeroen Janssen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// ============================================================================================
//
//  This VBE is part of the VGA Bios specific to the plex86/bochs Emulated VGA card.
//  You can NOT drive any physical vga card with it.
//
// ============================================================================================
//
//  This VBE Bios is based on information taken from :
//   - VESA BIOS EXTENSION (VBE) Core Functions Standard Version 3.0 located at www.vesa.org
//
// ============================================================================================


/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#include <inttypes.h>
#include <stddef.h>
#include "vbe.h"
#include "vgadefs.h"
#include "inlines.h"

// disable VESA/VBE2 check in vbe info
//#define VBE2_NO_VESA_CHECK

// use bytewise i/o (Longhorn beta issue, not in released Vista)
#define VBE_BYTEWISE_IO

#ifdef VBE_BYTEWISE_IO
    extern void do_out_dx_ax();
    #pragma aux do_out_dx_ax "*";
    extern void out_w(uint16_t port, uint16_t value);
    #pragma aux out_w =     \
        "call do_out_dx_ax" \
        parm [dx] [ax] modify nomemory;
    extern void do_in_ax_dx();
    #pragma aux do_in_ax_dx "*";
    extern uint16_t in_w(uint16_t port);
    #pragma aux in_w =     \
        "call do_in_ax_dx" \
        parm [dx] value [ax] modify nomemory;
#else
    #define out_w       outw
    #define in_w        inw
#endif


/* VESA signatures as integer constants. */
#define SIG_VBE2    0x32454256  /* 'VBE2' */
#define SIG_VESA    0x41534556  /* 'VESA' */


/* Implemented in assembler. */
extern void     __cdecl vga_compat_setup(void);
extern void     dispi_set_enable(uint16_t enable);
extern void     dispi_set_bank(uint16_t bank);
extern uint16_t __cdecl dispi_get_max_bpp(void);
extern void     __cdecl dispi_set_bank_farcall(void);

// The current OEM Software Revision of this VBE Bios
#define VBE_OEM_SOFTWARE_REV 0x0003

// FIXME: 'merge' these (c) etc strings with the vgabios.c strings?
char vbebios_copyright[]        = "VirtualBox VESA BIOS";
char vbebios_vendor_name[]      = VBOX_VENDOR;
char vbebios_product_name[]     = VBOX_PRODUCT " VBE Adapter";
char vbebios_product_revision[] = VBOX_PRODUCT " Version " VBOX_VERSION_STRING;

char vbebios_info_string[]    = "VirtualBox VBE Display Adapter enabled\r\n\r\n";
char no_vbebios_info_string[] = "No VirtualBox VBE support available!\r\n\r\n";

#ifdef VGA_DEBUG
char msg_vbe_init[] = "VirtualBox Version " VBOX_VERSION_STRING " VBE Display Adapter\r\n";
#endif

static void dispi_set_xres(uint16_t xres)
{
#ifdef VGA_DEBUG
    printf("vbe_set_xres: %04x\n", xres);
#endif
    out_w(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_XRES);
    out_w(VBE_DISPI_IOPORT_DATA, xres);
}

static void dispi_set_yres(uint16_t yres)
{
#ifdef VGA_DEBUG
    printf("vbe_set_yres: %04x\n", yres);
#endif
    out_w(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_YRES);
    out_w(VBE_DISPI_IOPORT_DATA, yres);
}

static uint16_t dispi_get_yres(void)
{
    out_w(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_YRES);
    return in_w(VBE_DISPI_IOPORT_DATA);
}

static void dispi_set_bpp(uint16_t bpp)
{
#ifdef VGA_DEBUG
    printf("vbe_set_bpp: %02x\n", bpp);
#endif
    out_w(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BPP);
    out_w(VBE_DISPI_IOPORT_DATA, bpp);
}

static uint16_t dispi_get_bpp(void)
{
    out_w(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BPP);
    return in_w(VBE_DISPI_IOPORT_DATA);
}

static void dispi_set_virt_width(uint16_t vwidth)
{
#ifdef VGA_DEBUG
    printf("vbe_set_virt_width: %04x\n", vwidth);
#endif
    out_w(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_VIRT_WIDTH);
    out_w(VBE_DISPI_IOPORT_DATA, vwidth);
}

static uint16_t dispi_get_virt_width(void)
{
    out_w(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_VIRT_WIDTH);
    return in_w(VBE_DISPI_IOPORT_DATA);
}

static uint16_t dispi_get_virt_height(void)
{
    out_w(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_VIRT_HEIGHT);
    return in_w(VBE_DISPI_IOPORT_DATA);
}

uint16_t in_word(uint16_t port, uint16_t addr)
{
    outw(port, addr);
    return inw(port);
}

uint8_t in_byte(uint16_t port, uint16_t addr)
{
    outw(port, addr);
    return inb(port);
}

/* Display "chip" identification helpers. */
static uint16_t dispi_get_id(void)
{
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ID);
    return inw(VBE_DISPI_IOPORT_DATA);
}

static void dispi_set_id(uint16_t chip_id)
{
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ID);
    outw(VBE_DISPI_IOPORT_DATA, chip_id);
}

/* VBE Init - Initialise the VESA BIOS Extension (VBE) support
 * This function does a sanity check on the host side display code interface.
 */
void vbe_init(void)
{
    dispi_set_id(VBE_DISPI_ID0);
    if (dispi_get_id() == VBE_DISPI_ID0) {
        /* VBE support was detected. */
        write_byte(BIOSMEM_SEG, BIOSMEM_VBE_FLAG, 1);
        dispi_set_id(VBE_DISPI_ID4);
    }
#ifdef DEBUG_VGA
    printf(msg_vbe_init);
#endif
}

/* Find the offset of the desired mode, given its number. */
static uint16_t mode_info_find_mode(uint16_t mode, Boolean using_lfb)
{
    uint16_t    sig, vmode, attrs;
    uint16_t    cur_info_ofs;   /* Current offset in mode list. */

    /* Read and check the VBE Extra Data signature. */
    sig = in_word(VBE_EXTRA_PORT, 0);
    if (sig != VBEHEADER_MAGIC) {
#ifdef DEBUG_VGA
        printf("Signature NOT found! %x\n", sig);
#endif
        return 0;
    }

    /* The LFB may be disabled. If so, LFB modes must not be reported. */
    if (using_lfb) {
        uint16_t    lfb_addr_hi;

        out_w(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_FB_BASE_HI);
        lfb_addr_hi = in_w(VBE_DISPI_IOPORT_DATA);
        if (!lfb_addr_hi) {
#ifdef DEBUG_VGA
            printf("LFB disabled, LFB modes unavailable!\n");
#endif
            return 0;
        }
    }

    cur_info_ofs = sizeof(VBEHeader);

    vmode = in_word(VBE_EXTRA_PORT, cur_info_ofs + offsetof(ModeInfoListItem, mode)/*&cur_info->mode*/);
    while (vmode != VBE_VESA_MODE_END_OF_LIST)
    {
        attrs = in_word(VBE_EXTRA_PORT, /*&cur_info->info.ModeAttributes*/cur_info_ofs + offsetof(ModeInfoListItem, info.ModeAttributes) );

        if (vmode == mode)
        {
            if (!using_lfb)
                return cur_info_ofs;
            else if (attrs & VBE_MODE_ATTRIBUTE_LINEAR_FRAME_BUFFER_MODE)
                return cur_info_ofs;
            else {
                cur_info_ofs += sizeof(ModeInfoListItem);
                vmode = in_word(VBE_EXTRA_PORT, /*&cur_info->mode*/cur_info_ofs + offsetof(ModeInfoListItem, mode));
            }
        } else {
            cur_info_ofs += sizeof(ModeInfoListItem);
            vmode = in_word(VBE_EXTRA_PORT, /*&cur_info->mode*/cur_info_ofs + offsetof(ModeInfoListItem, mode));
        }
    }
    return 0;
}

#ifndef VBOX
; VBE Display Info - Display information on screen about the VBE

vbe_display_info:
  call _vbe_has_vbe_display
  test ax, ax
  jz   no_vbe_flag
  mov  ax, #0xc000
  mov  ds, ax
  mov  si, #_vbebios_info_string
  jmp  _display_string
no_vbe_flag:
  mov  ax, #0xc000
  mov  ds, ax
  mov  si, #_no_vbebios_info_string
  jmp  _display_string
#endif

/** Function 00h - Return VBE Controller Information
 *
 * Input:
 *              AX      = 4F00h
 *              ES:DI   = Pointer to buffer in which to place VbeInfoBlock structure
 *                        (VbeSignature should be VBE2 when VBE 2.0 information is desired and
 *                        the info block is 512 bytes in size)
 * Output:
 *              AX      = VBE Return Status
 *
 */
void vbe_biosfn_return_controller_information(uint16_t STACK_BASED *AX, uint16_t ES, uint16_t DI)
{
    uint16_t            status;
    uint16_t            vbe2_info;
    uint16_t            cur_mode = 0;
    uint16_t            cur_ptr=34;
    uint16_t            cur_info_ofs;
    uint16_t            sig, vmode;
    uint16_t            max_bpp = dispi_get_max_bpp();
    VbeInfoBlock __far  *info_block;

    info_block = ES :> (VbeInfoBlock *)DI;

    /* Read VBE Extra Data signature */
    sig = in_word(VBE_EXTRA_PORT, 0);
    if (sig != VBEHEADER_MAGIC)
    {
        *AX = 0x0100;
#ifdef DEBUG_VGA
        printf("Signature NOT found\n");
#endif
        return;
    }
    cur_info_ofs = sizeof(VBEHeader);
    status = *AX;

#ifdef VGA_DEBUG
    printf("VBE vbe_biosfn_return_vbe_info ES%x DI%x AX%x\n",ES,DI,status);
#endif

    vbe2_info = 0;

    /* Don't use a local copy of VbeInfoBlock on the stack; it's too big.
     * The Ubuntu 8.04 64 bits splash screen emulator can't handle this.
     */
#ifdef VBE2_NO_VESA_CHECK
#else  /* !VBE2_NO_VESA_CHECK */
    // check for VBE2 signature
    if (info_block->VbeSignature.Sig32 == SIG_VBE2 || info_block->VbeSignature.Sig32 == SIG_VESA)
    {
        vbe2_info = 1;
#ifdef VGA_DEBUG
        printf("VBE correct VESA/VBE2 signature found\n");
#endif
    }
#endif /* !VBE2_NO_VESA_CHECK */

    /* VBE Signature - the compiler will optimize this into something sane. */
    info_block->VbeSignature.SigChr[0] = 'V';
    info_block->VbeSignature.SigChr[1] = 'E';
    info_block->VbeSignature.SigChr[2] = 'S';
    info_block->VbeSignature.SigChr[3] = 'A';

    /* VBE Version supported. */
    info_block->VbeVersion = 0x0200;    /* Version 2.0. */

    /* OEM String. */
    info_block->OemString.Ptr = &vbebios_copyright;

    /* Capabilities if this implementation. */
    info_block->Capabilities[0] = VBE_CAPABILITY_8BIT_DAC;
    info_block->Capabilities[1] = 0;
    info_block->Capabilities[2] = 0;
    info_block->Capabilities[3] = 0;

    /* Video mode list pointer (dynamically generated). */
    info_block->VideoModePtr_Seg = ES;
    info_block->VideoModePtr_Off = DI + 34;

    /* Total controller memory in 64K units. */
    info_block->TotalMemory = in_word(VBE_EXTRA_PORT, 0xffff);

    if (vbe2_info)
    {
        /* OEM information. */
        info_block->OemSoftwareRev     = VBE_OEM_SOFTWARE_REV;
        info_block->OemVendorName.Ptr  = &vbebios_vendor_name;
        info_block->OemProductName.Ptr = &vbebios_product_name;
        info_block->OemProductRev.Ptr  = &vbebios_product_revision;
    }

    do
    {
        uint8_t     data_b;

        data_b = in_byte(VBE_EXTRA_PORT, cur_info_ofs + offsetof(ModeInfoListItem, info.BitsPerPixel) /*&cur_info->info.BitsPerPixel*/);
        if (data_b <= max_bpp)
        {
            vmode = in_word(VBE_EXTRA_PORT, cur_info_ofs + offsetof(ModeInfoListItem, mode)/*&cur_info->mode*/);
#ifdef VGA_DEBUG
            printf("VBE found mode %x => %x\n", vmode, cur_mode);
#endif
            write_word(ES, DI + cur_ptr, vmode);
            cur_mode++;
            cur_ptr+=2;
        }
        cur_info_ofs += sizeof(ModeInfoListItem);
        vmode = in_word(VBE_EXTRA_PORT, cur_info_ofs + offsetof(ModeInfoListItem, mode)/*&cur_info->mode*/);
    } while (vmode != VBE_VESA_MODE_END_OF_LIST);

    // Add vesa mode list terminator
    write_word(ES, DI + cur_ptr, vmode);
    *AX = 0x004F;
}

/** Function 01h - Return VBE Mode Information
 *
 * Input:
 *              AX      = 4F01h
 *              CX      = Mode Number
 *              ES:DI   = Pointer to buffer in which to place ModeInfoBlock structure
 * Output:
 *              AX      = VBE Return Status
 *
 */
void vbe_biosfn_return_mode_information(uint16_t STACK_BASED *AX, uint16_t CX, uint16_t ES, uint16_t DI)
{
    uint16_t            result = 0x0100;
    uint16_t            cur_info_ofs;
    Boolean             using_lfb;
    uint8_t             win_attr;

#ifdef VGA_DEBUG
    printf("VBE vbe_biosfn_return_mode_information ES%x DI%x CX%x\n",ES,DI,CX);
#endif

    using_lfb = ((CX & VBE_MODE_LINEAR_FRAME_BUFFER) == VBE_MODE_LINEAR_FRAME_BUFFER);
    CX = (CX & 0x1ff);

    cur_info_ofs = mode_info_find_mode(CX, using_lfb);

    if (cur_info_ofs) {
        uint16_t    i;
#ifdef VGA_DEBUG
        printf("VBE found mode %x\n",CX);
#endif
        memsetb(ES, DI, 0, 256);    // The mode info size is fixed
        for (i = 0; i < sizeof(ModeInfoBlockCompact); i++) {
            uint8_t b;

            b = in_byte(VBE_EXTRA_PORT, cur_info_ofs + offsetof(ModeInfoListItem, info) + i/*(char *)(&(cur_info->info)) + i*/);
            write_byte(ES, DI + i, b);
        }
        win_attr = read_byte(ES, DI + offsetof(ModeInfoBlock, WinAAttributes));
        if (win_attr & VBE_WINDOW_ATTRIBUTE_RELOCATABLE) {
            write_word(ES, DI + offsetof(ModeInfoBlock, WinFuncPtr), (uint16_t)(dispi_set_bank_farcall));
            // If BIOS not at 0xC000 -> boom
            write_word(ES, DI + offsetof(ModeInfoBlock, WinFuncPtr) + 2, 0xC000);
        }
        // Update the LFB physical address which may change at runtime
        out_w(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_FB_BASE_HI);
        write_word(ES, DI + offsetof(ModeInfoBlock, PhysBasePtr) + 2, in_w(VBE_DISPI_IOPORT_DATA));

        result = 0x4f;
    } else {
#ifdef VGA_DEBUG
        printf("VBE *NOT* found mode %x\n",CX);
#endif
        result = 0x100;
    }

    *AX = result;
}

/** Function 02h - Set VBE Mode
 *
 * Input:
 *              AX      = 4F02h
 *              BX      = Desired Mode to set
 *              ES:DI   = Pointer to CRTCInfoBlock structure
 * Output:
 *              AX      = VBE Return Status
 *
 */
void vbe_biosfn_set_mode(uint16_t STACK_BASED *AX, uint16_t BX, uint16_t ES, uint16_t DI)
{
    uint16_t            result;
    uint16_t            cur_info_ofs;
    Boolean             using_lfb;
    uint8_t             no_clear;
    uint8_t             lfb_flag;

    using_lfb = ((BX & VBE_MODE_LINEAR_FRAME_BUFFER) == VBE_MODE_LINEAR_FRAME_BUFFER);
    lfb_flag  = using_lfb ? VBE_DISPI_LFB_ENABLED : 0;
    no_clear  = ((BX & VBE_MODE_PRESERVE_DISPLAY_MEMORY) == VBE_MODE_PRESERVE_DISPLAY_MEMORY) ? VBE_DISPI_NOCLEARMEM : 0;

    BX = (BX & 0x1ff);

    // check for non vesa mode
    if (BX < VBE_MODE_VESA_DEFINED)
    {
        uint8_t mode;

        dispi_set_enable(VBE_DISPI_DISABLED);
        // call the vgabios in order to set the video mode
        // this allows for going back to textmode with a VBE call (some applications expect that to work)
        mode = (BX & 0xff);
        biosfn_set_video_mode(mode);
        result = 0x4f;
        goto leave;
    }

    cur_info_ofs = mode_info_find_mode(BX, using_lfb);

    if (cur_info_ofs != 0)
    {
        uint16_t    xres, yres;
        uint8_t     bpp;

        xres = in_word(VBE_EXTRA_PORT, cur_info_ofs + offsetof(ModeInfoListItem, info.XResolution) /*&cur_info->info.XResolution*/);
        yres = in_word(VBE_EXTRA_PORT, cur_info_ofs + offsetof(ModeInfoListItem, info.YResolution) /*&cur_info->info.YResolution*/);
        bpp  = in_byte(VBE_EXTRA_PORT, cur_info_ofs + offsetof(ModeInfoListItem, info.BitsPerPixel) /*&cur_info->info.BitsPerPixel*/);

#ifdef VGA_DEBUG
        printf("VBE found mode %x, setting:\n", BX);
        printf("\txres%x yres%x bpp%x\n", xres, yres, bpp);
#endif

        // first disable current mode (when switching between vesa modi)
        dispi_set_enable(VBE_DISPI_DISABLED);

        if (bpp == 4)
        {
            biosfn_set_video_mode(0x6a);
        }

        dispi_set_bpp(bpp);
        dispi_set_xres(xres);
        dispi_set_yres(yres);
        dispi_set_bank(0);
        dispi_set_enable(VBE_DISPI_ENABLED | no_clear | lfb_flag);
        vga_compat_setup();

        write_word(BIOSMEM_SEG,BIOSMEM_VBE_MODE,BX);
        write_byte(BIOSMEM_SEG,BIOSMEM_VIDEO_CTL,(0x60 | no_clear));

        result = 0x4f;
    }
    else
    {
#ifdef VGA_DEBUG
        printf("VBE *NOT* found mode %x\n" , BX);
#endif
        result = 0x100;
    }

leave:
    *AX = result;
}

uint16_t vbe_biosfn_read_video_state_size(void)
{
    return 9 * 2;
}

void vbe_biosfn_save_video_state(uint16_t ES, uint16_t BX)
{
    uint16_t    enable, i;

    outw(VBE_DISPI_IOPORT_INDEX,VBE_DISPI_INDEX_ENABLE);
    enable = inw(VBE_DISPI_IOPORT_DATA);
    write_word(ES, BX, enable);
    BX += 2;
    if (!(enable & VBE_DISPI_ENABLED))
        return;
    for(i = VBE_DISPI_INDEX_XRES; i <= VBE_DISPI_INDEX_Y_OFFSET; i++) {
        if (i != VBE_DISPI_INDEX_ENABLE) {
            outw(VBE_DISPI_IOPORT_INDEX, i);
            write_word(ES, BX, inw(VBE_DISPI_IOPORT_DATA));
            BX += 2;
        }
    }
}


void vbe_biosfn_restore_video_state(uint16_t ES, uint16_t BX)
{
    uint16_t    enable, i;

    enable = read_word(ES, BX);
    BX += 2;

    if (!(enable & VBE_DISPI_ENABLED)) {
        outw(VBE_DISPI_IOPORT_INDEX,VBE_DISPI_INDEX_ENABLE);
        outw(VBE_DISPI_IOPORT_DATA, enable);
    } else {
        outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_XRES);
        outw(VBE_DISPI_IOPORT_DATA, read_word(ES, BX));
        BX += 2;
        outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_YRES);
        outw(VBE_DISPI_IOPORT_DATA, read_word(ES, BX));
        BX += 2;
        outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BPP);
        outw(VBE_DISPI_IOPORT_DATA, read_word(ES, BX));
        BX += 2;
        outw(VBE_DISPI_IOPORT_INDEX,VBE_DISPI_INDEX_ENABLE);
        outw(VBE_DISPI_IOPORT_DATA, enable);

        for(i = VBE_DISPI_INDEX_BANK; i <= VBE_DISPI_INDEX_Y_OFFSET; i++) {
            outw(VBE_DISPI_IOPORT_INDEX, i);
            outw(VBE_DISPI_IOPORT_DATA, read_word(ES, BX));
            BX += 2;
        }
    }
}

/** Function 04h - Save/Restore State
 *
 * Input:
 *              AX      = 4F04h
 *              DL      = 00h Return Save/Restore State buffer size
 *                        01h Save State
 *                        02h Restore State
 *              CX      = Requested states
 *              ES:BX   = Pointer to buffer (if DL <> 00h)
 * Output:
 *              AX      = VBE Return Status
 *              BX      = Number of 64-byte blocks to hold the state buffer (if DL=00h)
 *
 */
void vbe_biosfn_save_restore_state(uint16_t STACK_BASED *AX, uint16_t CX, uint16_t DX,
                                   uint16_t ES, uint16_t STACK_BASED *BX)
{
    uint16_t    result, val;

    result = 0x004F;
    switch(GET_DL()) {
    case 0x00:
        val = biosfn_read_video_state_size2(CX);
#ifdef VGA_DEBUG
        printf("VGA state size=%x\n", val);
#endif
        if (CX & 8)
            val += vbe_biosfn_read_video_state_size();
        *BX = (val + 63) / 64;
        break;
    case 0x01:
        val = *BX;
        val = biosfn_save_video_state(CX, ES, val);
#ifdef VGA_DEBUG
        printf("VGA save_state offset=%x\n", val);
#endif
        if (CX & 8)
            vbe_biosfn_save_video_state(ES, val);
        break;
    case 0x02:
        val = *BX;
        val = biosfn_restore_video_state(CX, ES, val);
#ifdef VGA_DEBUG
        printf("VGA restore_state offset=%x\n", val);
#endif
        if (CX & 8)
            vbe_biosfn_restore_video_state(ES, val);
        break;
    default:
        // function failed
        result = 0x100;
        break;
    }
    *AX = result;
}

/** Function 06h - Set/Get Logical Scan Line Length
 *
 *  Input:
 *              AX      = 4F06h
 *              BL      = 00h Set Scan Line Length in Pixels
 *                      = 01h Get Scan Line Length
 *                      = 02h Set Scan Line Length in Bytes
 *                      = 03h Get Maximum Scan Line Length
 *              CX      = If BL=00h Desired Width in Pixels
 *                        If BL=02h Desired Width in Bytes
 *                        (Ignored for Get Functions)
 *
 *  Output:
 *              AX      = VBE Return Status
 *              BX      = Bytes Per Scan Line
 *              CX      = Actual Pixels Per Scan Line (truncated to
 *                        nearest complete pixel)
 *              DX      = Maximum Number of Scan Lines
 */
void vbe_biosfn_get_set_scanline_length(uint16_t STACK_BASED *AX, uint16_t STACK_BASED *BX,
                                        uint16_t STACK_BASED *CX, uint16_t STACK_BASED *DX)
{
    uint16_t    val;
    uint16_t    result;
    uint8_t     bpp;
    uint8_t     subfn;
    uint16_t    old_vw;

    bpp    = dispi_get_bpp();
    bpp    = bpp == 15 ? 16 : bpp;
    old_vw = dispi_get_virt_width();
    result = 0x004F;
    val    = *CX;
    subfn  = *BX & 0xFF;
#ifdef VGA_DEBUG
    printf("VBE get/set scanline len fn=%x, CX=%x\n", subfn, *CX);
#endif
    switch(subfn) {
    case 0x02:
        if (bpp == 4)
            val = val * 8;
        else
            val = val / (bpp / 8);
        /* fall through */
    case 0x00:
        dispi_set_virt_width(val);
        /* fall through */
    case 0x01:
        val = dispi_get_virt_width();
        *CX = val;                          /* Width in pixels. */
        if (bpp == 4)
            val = val / 8;
        else
            val = val * (bpp / 8);
        val = (val + 3) & ~3;
        *BX = val;                          /* Bytes per scanline. */
        *DX = dispi_get_virt_height();      /* Height in lines. */
        if (*DX < dispi_get_yres()) {
            dispi_set_virt_width(old_vw);
            result = 0x200;
        }
        break;
    default:
        // function failed
        result = 0x100;
        break;
    }
    *AX = result;
}


/* We would very much like to avoid dragging in the long multiply library
 * routine, and we really just need to multiply two 16-bit numbers to
 * obtain a 32-bit result, so...
 */
uint32_t mul32_16x16(uint16_t a, uint16_t b);
#pragma aux mul32_16x16 =   \
    "mul    dx"             \
    parm [ax] [dx] modify nomemory;


/** Private INT 10h function 5642h - Manage custom video modes using X/Y
 *  resolution and bit depth rather than mode number
 *
 *  Input:
 *              AX      = 5642h ('VB')
 *              BL      = 00h Set video mode
 *              BH      = If BL=00h Desired bit depth in pixels
 *              CX      = If BL=00h Desired width in pixels
 *              DX      = If BL=00h Desired height in pixels
 *
 *  Output:
 *              AX      = VBE style return status
 */
void private_biosfn_custom_mode(uint16_t STACK_BASED *AX, uint16_t STACK_BASED *BX,
                                uint16_t STACK_BASED *CX, uint16_t STACK_BASED *DX)
{
    uint16_t    result;
    uint8_t     subfn;
    uint8_t     bpp;
    uint8_t     lfb_flag;
    uint16_t    xres;
    uint16_t    yres;
    uint16_t    line_size;
    uint32_t    vram_size;
    uint32_t    mode_size;

    result = 0x004F;
    subfn  = *BX & 0xFF;
    switch (subfn) {
    case 0x00:
        xres = *CX;
        yres = *DX;
        bpp  = (*BX >> 8) & 0x7F;
#ifdef VGA_DEBUG
        printf("Set custom mode %04x by %04x %xbpp\n", xres, yres, bpp);
#endif
        /* Only allow 32/16/8bpp. */
        if (bpp != 8 && bpp != 16 && bpp != 32) {
            result = 0x100;
            break;
        }

        /* Determine the LFB flag. */
        lfb_flag = *BX & 0x8000 ? VBE_DISPI_LFB_ENABLED : 0;

        /* Cap the resolution to something not insanely high or low. */
        if (xres < 640)
            xres = 640;
        else if (xres > 2560)
            xres = 2560;
        if (yres < 480)
            yres = 480;
        else if (yres > 1920)
            yres = 1920;
#ifdef VGA_DEBUG
        printf("Adjusted resolution %04x by %04x\n", xres, yres);
#endif

        /* Calculate the VRAM size in bytes. */
        vram_size = (uint32_t)in_word(VBE_EXTRA_PORT, 0xffff) << 16;

        /* Calculate the scanline size in bytes. */
        line_size = xres * (bpp / 8);
        line_size = (line_size + 3) & ~3;
        /* And now the memory required for the mode. */
        mode_size = mul32_16x16(line_size, yres);

        if (mode_size > vram_size) {
            /* No can do. Don't have that much VRAM. */
            result = 0x200;
            break;
        }

        /* Mode looks valid, let's get cracking. */
        dispi_set_enable(VBE_DISPI_DISABLED);
        dispi_set_bpp(bpp);
        dispi_set_xres(xres);
        dispi_set_yres(yres);
        dispi_set_bank(0);
        dispi_set_enable(VBE_DISPI_ENABLED | lfb_flag);
        vga_compat_setup();
        break;

    default:
        // unsupported sub-function
        result = 0x100;
        break;
    }
    *AX = result;
}
